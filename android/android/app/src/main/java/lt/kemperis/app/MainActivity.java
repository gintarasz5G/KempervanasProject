package lt.kemperis.app;

import android.Manifest;
import android.app.Activity;
import android.app.DownloadManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiNetworkSuggestion;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.provider.Settings;
import android.provider.Telephony;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;
import android.telephony.SmsManager;
import android.telephony.SmsMessage;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.webkit.JavascriptInterface;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.content.FileProvider;

import com.getcapacitor.BridgeActivity;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class MainActivity extends BridgeActivity {

    private static final int PERMISSION_REQUEST_CODE = 123;
    static final String VERSION_JSON_URL = "https://raw.githubusercontent.com/gintarasz5G/KempervanasProject/main/version.json";
    static final int CURRENT_VERSION = 36;

    private Network boundNetwork = null;
    private volatile boolean autoBindPaused = false;
    private ConnectivityManager.NetworkCallback wifiCallback = null;
    private BroadcastReceiver smsReceiver = null;
    private UpdateBridge updateBridge = null;
    private KempTtsBridge ttsBridge = null;
    private WifiManager.WifiLock wifiLock = null;

    private void sendNativeLog(String msg) {
        final String js = "window.onNativeLog && window.onNativeLog(" + JSONObject.quote(msg) + ")";
        runOnUiThread(() -> {
            try {
                if (getBridge() != null && getBridge().getWebView() != null) {
                    getBridge().getWebView().evaluateJavascript(js, null);
                }
            } catch (Exception ignored) {}
        });
    }

    private void acquireWifiLock() {
        try {
            if (wifiLock == null) {
                WifiManager wm = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
                if (wm != null) {
                    wifiLock = wm.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, "KemperisWifiLock");
                    wifiLock.setReferenceCounted(false);
                }
            }
            if (wifiLock != null && !wifiLock.isHeld()) {
                wifiLock.acquire();
                sendNativeLog("🔒 WifiLock ACQUIRED");
            }
        } catch (Exception e) {
            sendNativeLog("⚠️ WifiLock Error: " + e.getMessage());
        }
    }

    private void releaseWifiLock() {
        if (wifiLock != null && wifiLock.isHeld()) {
            wifiLock.release();
            sendNativeLog("🔓 WifiLock RELEASED");
        }
    }

    private HttpURLConnection getSafeConnection(URL url) throws java.io.IOException {
        ConnectivityManager cm = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        Network internetNetwork = null;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            Network[] networks = cm.getAllNetworks();
            for (Network n : networks) {
                NetworkCapabilities caps = cm.getNetworkCapabilities(n);
                if (caps != null && caps.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)) {
                    internetNetwork = n;
                    break;
                }
            }
        }
        if (internetNetwork != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return (HttpURLConnection) internetNetwork.openConnection(url);
        } else {
            return (HttpURLConnection) url.openConnection();
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ContextCompat.startForegroundService(this, new Intent(this, KemperisService.class));

        WebSettings settings = this.getBridge().getWebView().getSettings();
        settings.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);
        settings.setSupportZoom(true);
        settings.setBuiltInZoomControls(true);
        settings.setDisplayZoomControls(false);
        settings.setLoadWithOverviewMode(true);
        settings.setUseWideViewPort(true);

        this.getBridge().getWebView().addJavascriptInterface(new VolBridge(this), "KemperisVol");
        this.getBridge().getWebView().addJavascriptInterface(new WifiBridge(this), "KemperisWifi");
        this.getBridge().getWebView().addJavascriptInterface(new KemperisNetBridge(this), "KemperisNet");
        ttsBridge = new KempTtsBridge(this, this.getBridge().getWebView());
        this.getBridge().getWebView().addJavascriptInterface(ttsBridge, "KempTts");
        updateBridge = new UpdateBridge(this);
        this.getBridge().getWebView().addJavascriptInterface(updateBridge, "KemperisUpdate");
        this.getBridge().getWebView().addJavascriptInterface(new SmsBridge(this), "KemperisSms");
        this.getBridge().getWebView().addJavascriptInterface(new KemperisFileBridge(this), "KemperisFile");

        requestNecessaryPermissions();
        registerAutoWifiCallback();
        registerSmsReceiver();
        acquireWifiLock();
    }

    @Override
    public void onResume() {
        super.onResume();
        acquireWifiLock();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        releaseWifiLock();
        if (wifiCallback != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            try {
                ConnectivityManager cm = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
                cm.unregisterNetworkCallback(wifiCallback);
            } catch (Exception ignored) {}
        }
        if (smsReceiver != null) {
            try { unregisterReceiver(smsReceiver); } catch (Exception ignored) {}
        }
        if (updateBridge != null) updateBridge.cleanup();
        if (ttsBridge != null) ttsBridge.shutdown();
    }

    private void registerSmsReceiver() {
        smsReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                try {
                    if (!Telephony.Sms.Intents.SMS_RECEIVED_ACTION.equals(intent.getAction())) return;
                    Bundle extras = intent.getExtras();
                    if (extras == null) return;
                    Object[] pdus = (Object[]) extras.get("pdus");
                    if (pdus == null) return;
                    String format = intent.getStringExtra("format");
                    StringBuilder full = new StringBuilder();
                    String sender = "";
                    for (Object pdu : pdus) {
                        SmsMessage msg = SmsMessage.createFromPdu((byte[]) pdu, format);
                        if (msg == null) continue;
                        if (sender.isEmpty()) {
                            String s = msg.getOriginatingAddress();
                            if (s != null) sender = s;
                        }
                        String mb = msg.getMessageBody();
                        if (mb != null) full.append(mb);
                    }
                    String body = full.toString();
                    final String js = "window.onSmsReceived && window.onSmsReceived("
                        + JSONObject.quote(sender) + "," + JSONObject.quote(body) + ")";
                    runOnUiThread(() -> {
                        try { getBridge().getWebView().evaluateJavascript(js, null); } catch (Exception ignored) {}
                    });
                } catch (Exception e) {
                    android.util.Log.e("KemperisApp", "onReceive klaida: " + e.getMessage());
                }
            }
        };
        IntentFilter filter = new IntentFilter(Telephony.Sms.Intents.SMS_RECEIVED_ACTION);
        filter.setPriority(999);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(smsReceiver, filter, Context.RECEIVER_EXPORTED);
        } else {
            registerReceiver(smsReceiver, filter);
        }
    }

    private void registerAutoWifiCallback() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return;
        ConnectivityManager cm = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        wifiCallback = new ConnectivityManager.NetworkCallback() {
            @Override
            public void onAvailable(@NonNull Network network) {
                if (autoBindPaused) return;
                NetworkCapabilities caps = cm.getNetworkCapabilities(network);
                if (caps == null) return;
                if (caps.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) && !caps.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)) {
                    boundNetwork = network;
                    cm.bindProcessToNetwork(network);
                    runOnUiThread(() -> {
                        Toast.makeText(MainActivity.this, "🔌 Pririšta prie Kemperio AP", Toast.LENGTH_SHORT).show();
                        getBridge().getWebView().evaluateJavascript("window.onEspNetworkUp && window.onEspNetworkUp()", null);
                    });
                }
            }
            @Override
            public void onLost(@NonNull Network network) {
                if (network.equals(boundNetwork)) {
                    boundNetwork = null;
                    cm.bindProcessToNetwork(null);
                    runOnUiThread(() -> getBridge().getWebView().evaluateJavascript("window.onEspNetworkDown && window.onEspNetworkDown()", null));
                }
            }
        };
        NetworkRequest req = new NetworkRequest.Builder()
            .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
            .removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
            .build();
        cm.registerNetworkCallback(req, wifiCallback);
    }

    private void requestNecessaryPermissions() {
        List<String> permissions = new ArrayList<>();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) permissions.add(Manifest.permission.POST_NOTIFICATIONS);
        permissions.add(Manifest.permission.ACCESS_FINE_LOCATION);
        permissions.add(Manifest.permission.SEND_SMS);
        permissions.add(Manifest.permission.RECEIVE_SMS);
        permissions.add(Manifest.permission.READ_PHONE_STATE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) permissions.add(Manifest.permission.READ_PHONE_NUMBERS);
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) permissions.add(Manifest.permission.WRITE_EXTERNAL_STORAGE);
        permissions.add(Manifest.permission.RECEIVE_BOOT_COMPLETED);

        List<String> toRequest = new ArrayList<>();
        for (String p : permissions) {
            if (ContextCompat.checkSelfPermission(this, p) != PackageManager.PERMISSION_GRANTED) toRequest.add(p);
        }
        if (!toRequest.isEmpty()) ActivityCompat.requestPermissions(this, toRequest.toArray(new String[0]), PERMISSION_REQUEST_CODE);
    }

    // --- BRIDGES ---

    public class KemperisNetBridge {
        private Context ctx;
        public KemperisNetBridge(Context c) { this.ctx = c; }

        @JavascriptInterface
        public String getMode() {
            return ctx.getSharedPreferences("KemperisPrefs", Context.MODE_PRIVATE).getString("network_mode", "auto");
        }

        @JavascriptInterface
        public void setMode(String mode) {
            ctx.getSharedPreferences("KemperisPrefs", Context.MODE_PRIVATE).edit().putString("network_mode", mode).apply();
            sendNativeLog("🌐 Network Mode set to: " + mode);
        }

        @JavascriptInterface
        public void setAutostart(boolean enabled) {
            ctx.getSharedPreferences("KemperisPrefs", Context.MODE_PRIVATE).edit().putBoolean("autostart_enabled", enabled).apply();
            sendNativeLog("🚀 Autostart: " + enabled);
        }

        @JavascriptInterface
        public void openAutostartSettings() {
            Intent[] intents = {
                new Intent().setComponent(new ComponentName("com.huawei.systemmanager", "com.huawei.systemmanager.startupmgr.ui.StartupNormalAppListActivity")),
                new Intent().setComponent(new ComponentName("com.huawei.systemmanager", "com.huawei.systemmanager.optimize.process.ProtectActivity")),
                new Intent().setComponent(new ComponentName("com.miui.securitycenter", "com.miui.permcenter.autostart.AutoStartManagementActivity")),
                new Intent().setComponent(new ComponentName("com.coloros.safecenter", "com.coloros.safecenter.permission.startup.StartupAppListActivity")),
                new Intent().setComponent(new ComponentName("com.vivo.permissionmanager", "com.vivo.permissionmanager.activity.BgStartUpManagerActivity"))
            };
            boolean success = false;
            for (Intent i : intents) {
                try { i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK); ctx.startActivity(i); success = true; break; } catch (Exception ignored) {}
            }
            if (!success) {
                try {
                    Intent i = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                    i.setData(Uri.parse("package:" + ctx.getPackageName()));
                    i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    ctx.startActivity(i);
                    runOnUiThread(() -> Toast.makeText(ctx, "Rankiniu būdu leiskite automatinį paleidimą", Toast.LENGTH_LONG).show());
                } catch (Exception ignored) {}
            }
        }

        @JavascriptInterface
        public void httpGet(String urlStr, String callbackName) {
            new Thread(() -> {
                String result = doHttp(urlStr, "GET", null);
                runOnUiThread(() -> {
                    String js = "window['" + callbackName + "'] && window['" + callbackName + "'](" + JSONObject.quote(result) + ")";
                    if (getBridge() != null && getBridge().getWebView() != null) {
                        getBridge().getWebView().evaluateJavascript(js, null);
                    }
                });
            }).start();
        }

        @JavascriptInterface
        public void httpPost(String urlStr, String body, String callbackName) {
            new Thread(() -> {
                String result = doHttp(urlStr, "POST", body);
                runOnUiThread(() -> {
                    String js = "window['" + callbackName + "'] && window['" + callbackName + "'](" + JSONObject.quote(result) + ")";
                    if (getBridge() != null && getBridge().getWebView() != null) {
                        getBridge().getWebView().evaluateJavascript(js, null);
                    }
                });
            }).start();
        }

        private String doHttp(String urlStr, String method, String body) {
            HttpURLConnection conn = null;
            try {
                URL url = new URL(urlStr);
                conn = getSafeConnection(url);
                conn.setRequestMethod(method);
                conn.setConnectTimeout(10000);
                conn.setReadTimeout(10000);
                if ("POST".equals(method) && body != null) {
                    conn.setDoOutput(true);
                    conn.setRequestProperty("Content-Type", "text/plain");
                    try (OutputStream os = conn.getOutputStream()) {
                        os.write(body.getBytes(StandardCharsets.UTF_8));
                    }
                }
                int code = conn.getResponseCode();
                StringBuilder sb = new StringBuilder();
                try (BufferedReader br = new BufferedReader(new InputStreamReader(
                        code >= 200 && code < 300 ? conn.getInputStream() : conn.getErrorStream()))) {
                    String line;
                    while ((line = br.readLine()) != null) sb.append(line);
                }
                String response = sb.toString();
                if (code >= 200 && code < 300 && !response.trim().startsWith("{") && !response.trim().startsWith("[")) {
                    return "{\"error\":\"Not JSON: " + (response.length() > 50 ? response.substring(0, 50) : response).replace("\"", "'") + "\"}";
                }
                return response;
            } catch (Exception e) {
                return "{\"error\":\"" + e.getMessage() + "\"}";
            } finally {
                if (conn != null) conn.disconnect();
            }
        }
    }

    public class SmsBridge {
        private Context ctx;
        public SmsBridge(Context c) { this.ctx = c; }
        @JavascriptInterface
        public boolean hasPermission() { return ContextCompat.checkSelfPermission(ctx, Manifest.permission.SEND_SMS) == PackageManager.PERMISSION_GRANTED; }
        @JavascriptInterface
        public void send(String phone, String text) {
            try { SmsManager.getDefault().sendTextMessage(phone, null, text, null, null); sendNativeLog("📩 SMS Sent to " + phone); }
            catch (Exception e) { sendNativeLog("❌ SMS Error: " + e.getMessage()); }
        }
        @JavascriptInterface
        public String getOwnPhoneNumber() {
            try {
                TelephonyManager tm = (TelephonyManager) ctx.getSystemService(Context.TELEPHONY_SERVICE);
                if (ActivityCompat.checkSelfPermission(ctx, Manifest.permission.READ_PHONE_NUMBERS) == PackageManager.PERMISSION_GRANTED) {
                    return tm.getLine1Number();
                }
            } catch (Exception ignored) {}
            return "";
        }
        @JavascriptInterface
        public boolean canSendSms() {
            PackageManager pm = ctx.getPackageManager();
            if (!pm.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) return false;
            TelephonyManager tm = (TelephonyManager) ctx.getSystemService(Context.TELEPHONY_SERVICE);
            return tm != null && tm.getSimState() == TelephonyManager.SIM_STATE_READY;
        }
    }

    public class UpdateBridge {
        private Context ctx;
        private long downloadId = -1;
        private BroadcastReceiver downloadReceiver = null;
        public void cleanup() { if (downloadReceiver != null) { try { ctx.unregisterReceiver(downloadReceiver); } catch (Exception ignored) {} } }
        public UpdateBridge(Context c) { this.ctx = c; }
        @JavascriptInterface
        public int getCurrentVersion() { return CURRENT_VERSION; }
        @JavascriptInterface
        public void checkUpdate() {
            new Thread(() -> {
                try {
                    URL url = new URL(VERSION_JSON_URL);
                    HttpURLConnection conn = getSafeConnection(url);
                    conn.setConnectTimeout(15000);
                    conn.setReadTimeout(15000);
                    BufferedReader br = new BufferedReader(new InputStreamReader(conn.getInputStream()));
                    StringBuilder sb = new StringBuilder(); String line;
                    while ((line = br.readLine()) != null) sb.append(line);
                    conn.disconnect();

                    JSONObject jo = new JSONObject(sb.toString());
                    int remoteVer   = jo.optInt("version", -1);
                    String apkUrl   = jo.optString("apk_url", "");
                    String notes    = jo.optString("notes", "");
                    String type     = (remoteVer > CURRENT_VERSION) ? "update" : "latest";

                    final String js = "window.onUpdateResult && window.onUpdateResult("
                        + JSONObject.quote(type) + "," + remoteVer + ","
                        + JSONObject.quote(apkUrl) + "," + JSONObject.quote(notes) + ")";
                    runOnUiThread(() -> getBridge().getWebView().evaluateJavascript(js, null));

                } catch (Exception e) {
                    final String msg = (e.getMessage() == null) ? "nezinoma klaida" : e.getMessage();
                    final String js = "window.onUpdateResult && window.onUpdateResult("
                        + JSONObject.quote("error") + ",0," + JSONObject.quote(msg) + "," + JSONObject.quote("") + ")";
                    runOnUiThread(() -> getBridge().getWebView().evaluateJavascript(js, null));
                }
            }).start();
        }
        @JavascriptInterface
        public void downloadAndInstall(String apkUrl) {
            try {
                DownloadManager.Request request = new DownloadManager.Request(Uri.parse(apkUrl));
                request.setTitle("Kemperis Atnaujinimas");
                request.setDescription("Atsiunčiama nauja versija...");
                request.setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
                request.setDestinationInExternalPublicDir(Environment.DIRECTORY_DOWNLOADS, "kemperis_update.apk");
                DownloadManager dm = (DownloadManager) ctx.getSystemService(Context.DOWNLOAD_SERVICE);
                downloadId = dm.enqueue(request);
                downloadReceiver = new BroadcastReceiver() {
                    @Override
                    public void onReceive(Context context, Intent intent) {
                        long id = intent.getLongExtra(DownloadManager.EXTRA_DOWNLOAD_ID, -1);
                        if (id != downloadId) return;
                        DownloadManager dm = (DownloadManager) ctx.getSystemService(Context.DOWNLOAD_SERVICE);
                        DownloadManager.Query q = new DownloadManager.Query().setFilterById(id);
                        try (android.database.Cursor c = dm.query(q)) {
                            if (c != null && c.moveToFirst()) {
                                int st = c.getInt(c.getColumnIndex(DownloadManager.COLUMN_STATUS));
                                if (st == DownloadManager.STATUS_SUCCESSFUL) {
                                    installApk(new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), "kemperis_update.apk"));
                                } else {
                                    sendNativeLog("❌ Atsisiuntimas nepavyko (status=" + st + ")");
                                }
                            }
                        }
                    }
                };
                ctx.registerReceiver(downloadReceiver, new IntentFilter(DownloadManager.ACTION_DOWNLOAD_COMPLETE));
            } catch(Exception e) { sendNativeLog("Download Error: " + e.getMessage()); }
        }
        private void installApk(File file) {
            Uri apkUri = FileProvider.getUriForFile(ctx, ctx.getPackageName() + ".fileprovider", file);
            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setDataAndType(apkUri, "application/vnd.android.package-archive");
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_ACTIVITY_NEW_TASK);
            ctx.startActivity(intent);
        }
    }

    public class KempTtsBridge {
        private Activity activity;
        private WebView webView;
        private TextToSpeech tts;
        private boolean ready = false;
        private String pendingLang = null;
        private android.media.AudioFocusRequest audioFocusReq = null;
        private String effectiveLang = "en";

        public KempTtsBridge(Activity a, WebView wv) {
            this.activity = a; this.webView = wv;
            tts = new TextToSpeech(a, status -> {
                if (status == TextToSpeech.SUCCESS) {
                    ready = true;
                    AudioAttributes aa = new AudioAttributes.Builder()
                            .setUsage(AudioAttributes.USAGE_MEDIA)
                            .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
                            .build();
                    tts.setAudioAttributes(aa);
                    applyLang(pendingLang != null ? pendingLang : "en");
                    reportLangs();
                    tts.setOnUtteranceProgressListener(new UtteranceProgressListener() {
                        @Override public void onStart(String id) {}
                        @Override public void onDone(String id) { fireDone(); }
                        @Override public void onError(String id) { fireDone(); }
                    });
                } else {
                    logToWeb("variklis nerastas (status=" + status + ")");
                }
            });
        }

        @JavascriptInterface
        public void speak(String text) {
            if (ready) {
                requestDuck();
                android.os.Bundle params = new android.os.Bundle();
                params.putInt(TextToSpeech.Engine.KEY_PARAM_STREAM, AudioManager.STREAM_MUSIC);
                tts.speak(text, TextToSpeech.QUEUE_ADD, params, "uid");
            } else {
                logToWeb("TTS neparuostas - nutylima");
            }
        }

        @JavascriptInterface
        public void setLang(String lang) {
            if (ready) applyLang(lang);
            else pendingLang = lang;
        }

        @JavascriptInterface
        public String getEffectiveLang() { return effectiveLang; }

        private boolean isUsable(Locale loc) {
            int r = tts.isLanguageAvailable(loc);
            return r == TextToSpeech.LANG_AVAILABLE
                    || r == TextToSpeech.LANG_COUNTRY_AVAILABLE
                    || r == TextToSpeech.LANG_COUNTRY_VAR_AVAILABLE;
        }

        private void applyLang(String l) {
            if (!ready) return;
            Locale target;
            if ("lt".equals(l) && isUsable(new Locale("lt","LT"))) {
                target = new Locale("lt", "LT");
                effectiveLang = "lt";
            } else {
                if (isUsable(Locale.US))       target = Locale.US;
                else if (isUsable(Locale.UK))  target = Locale.UK;
                else                           target = Locale.getDefault();
                effectiveLang = "en";
            }
            int r = tts.setLanguage(target);
            if (r == TextToSpeech.LANG_MISSING_DATA) {
                if (!"lt".equals(l) && isUsable(Locale.UK)) { tts.setLanguage(Locale.UK); return; }
                logToWeb(l + " truksta balso duomenu"); promptInstall();
            } else if (r == TextToSpeech.LANG_NOT_SUPPORTED) {
                logToWeb(l + " kalba nepalaikoma");
            }
        }

        private void reportLangs() {
            int en = tts.isLanguageAvailable(Locale.US);
            int lt = tts.isLanguageAvailable(new Locale("lt","LT"));
            logToWeb("TTS EN: " + langStatus(en) + " | LT: " + langStatus(lt));
            if (en == TextToSpeech.LANG_MISSING_DATA || lt == TextToSpeech.LANG_MISSING_DATA) promptInstall();
        }

        private String langStatus(int r) {
            switch (r) {
                case TextToSpeech.LANG_AVAILABLE:
                case TextToSpeech.LANG_COUNTRY_AVAILABLE:
                case TextToSpeech.LANG_COUNTRY_VAR_AVAILABLE: return "OK";
                case TextToSpeech.LANG_MISSING_DATA:  return "truksta duomenu";
                case TextToSpeech.LANG_NOT_SUPPORTED: return "nepalaikoma";
                default: return "nezinoma(" + r + ")";
            }
        }

        private void promptInstall() {
            try {
                Intent i = new Intent(TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA);
                i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                activity.startActivity(i);
            } catch (Exception ignored) {}
        }

        private void fireDone() {
            abandonDuck();
            activity.runOnUiThread(() -> webView.evaluateJavascript("window.onTtsDone && window.onTtsDone()", null));
        }

        private void logToWeb(String msg) {
            final String safe = msg.replace("'", " ");
            activity.runOnUiThread(() -> webView.evaluateJavascript("sysLog && sysLog('[TTS] " + safe + "','warn')", null));
        }

        private void requestDuck() {
            try {
                AudioManager am = (AudioManager) activity.getSystemService(Context.AUDIO_SERVICE);
                if (am == null) return;
                AudioAttributes aa = new AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_MEDIA)
                        .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
                        .build();
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    audioFocusReq = new android.media.AudioFocusRequest.Builder(
                            AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK)
                            .setAudioAttributes(aa)
                            .build();
                    am.requestAudioFocus(audioFocusReq);
                } else {
                    am.requestAudioFocus(null, AudioManager.STREAM_MUSIC,
                            AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK);
                }
            } catch (Exception ignored) {}
        }

        private void abandonDuck() {
            try {
                AudioManager am = (AudioManager) activity.getSystemService(Context.AUDIO_SERVICE);
                if (am == null) return;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    if (audioFocusReq != null) { am.abandonAudioFocusRequest(audioFocusReq); audioFocusReq = null; }
                } else {
                    am.abandonAudioFocus(null);
                }
            } catch (Exception ignored) {}
        }

        public void shutdown() { if (tts != null) tts.shutdown(); }
    }

    public class VolBridge {
        private Context ctx;
        public VolBridge(Context c) { this.ctx = c; }
        @JavascriptInterface
        public int getVolumePct() {
            AudioManager am = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);
            int max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
            return (am.getStreamVolume(AudioManager.STREAM_MUSIC) * 100) / (max > 0 ? max : 1);
        }
        @JavascriptInterface
        public void setVolumePct(int pct) {
            AudioManager am = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);
            int max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
            am.setStreamVolume(AudioManager.STREAM_MUSIC, (pct * max) / 100, 0);
        }
    }

    public class KemperisFileBridge {
        private Context ctx;
        public KemperisFileBridge(Context c) { this.ctx = c; }
        @JavascriptInterface
        public String saveTextFile(String name, String mime, String content) {
            try {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    ContentValues cv = new ContentValues();
                    cv.put(MediaStore.Downloads.DISPLAY_NAME, name);
                    cv.put(MediaStore.Downloads.MIME_TYPE, (mime == null || mime.isEmpty()) ? "text/plain" : mime);
                    cv.put(MediaStore.Downloads.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS);
                    android.net.Uri uri = ctx.getContentResolver().insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, cv);
                    if (uri == null) return "Error: MediaStore insert null";
                    try (OutputStream os = ctx.getContentResolver().openOutputStream(uri)) {
                        os.write(content.getBytes(StandardCharsets.UTF_8));
                    }
                    return "ok";
                } else {
                    File dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
                    if (!dir.exists()) dir.mkdirs();
                    File file = new File(dir, name);
                    if (file.exists() && file.isDirectory()) {
                        File[] kids = file.listFiles();
                        if (kids != null) for (File k : kids) k.delete();
                        file.delete();
                    }
                    try (FileOutputStream fos = new FileOutputStream(file)) {
                        fos.write(content.getBytes(StandardCharsets.UTF_8));
                    }
                    return "ok";
                }
            } catch (Exception e) { return "Error: " + e.getMessage(); }
        }
    }

    public class WifiBridge {
        private Context ctx;
        public WifiBridge(Context c) { this.ctx = c; }
        @JavascriptInterface
        public String getCurrentSSID() {
            WifiManager wm = (WifiManager) ctx.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            WifiInfo info = wm.getConnectionInfo();
            String s = info.getSSID();
            if (s.startsWith("\"") && s.endsWith("\"")) s = s.substring(1, s.length()-1);
            return s;
        }
        @JavascriptInterface
        public void unbindNetwork() {
            ConnectivityManager cm = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) cm.bindProcessToNetwork(null);
        }
        @JavascriptInterface
        public void rebindNetwork() {
            ConnectivityManager cm = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && boundNetwork != null) cm.bindProcessToNetwork(boundNetwork);
        }
    }
}
