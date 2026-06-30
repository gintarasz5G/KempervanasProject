package lt.kemperis.app;

import android.Manifest;
import android.app.DownloadManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
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
import android.provider.Settings;
import android.provider.Telephony;
import android.app.Activity;
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
import android.content.ContentValues;
import android.provider.MediaStore;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class MainActivity extends BridgeActivity {

    private static final int PERMISSION_REQUEST_CODE = 123;

    static final String VERSION_JSON_URL =
        "https://raw.githubusercontent.com/gintarasz5G/KempervanasProject/main/version.json";
    static final int CURRENT_VERSION = 20;

    private Network boundNetwork = null;
    private volatile boolean autoBindPaused = false;
    private ConnectivityManager.NetworkCallback wifiCallback = null;

    // SMS receiver registruotas dinamiškai
    private BroadcastReceiver smsReceiver = null;
    // UpdateBridge — reikia onDestroy cleanup
    private UpdateBridge updateBridge = null;
    // TTS bridge — reikia onDestroy cleanup (tts.shutdown())
    private KempTtsBridge ttsBridge = null;

    // ─────────────────────────────────────────────────────────────────────────
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Foreground Service — paleidžiamas pirmas (Android 14 reikalavimas)
        ContextCompat.startForegroundService(this, new Intent(this, KemperisService.class));

        WebSettings settings = this.getBridge().getWebView().getSettings();
        settings.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);

        this.getBridge().getWebView().addJavascriptInterface(new VolBridge(this),    "KemperisVol");
        this.getBridge().getWebView().addJavascriptInterface(new WifiBridge(this),   "KemperisWifi");
        ttsBridge = new KempTtsBridge(this, this.getBridge().getWebView());
        this.getBridge().getWebView().addJavascriptInterface(ttsBridge,               "KempTts");
        updateBridge = new UpdateBridge(this);
        this.getBridge().getWebView().addJavascriptInterface(updateBridge,           "KemperisUpdate");
        this.getBridge().getWebView().addJavascriptInterface(new SmsBridge(this),    "KemperisSms");
        this.getBridge().getWebView().addJavascriptInterface(new KemperisFileBridge(this), "KemperisFile");

        requestNecessaryPermissions();
        registerAutoWifiCallback();
        registerSmsReceiver();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (wifiCallback != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            try {
                ConnectivityManager cm = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
                cm.unregisterNetworkCallback(wifiCallback);
            } catch (Exception ignored) {}
        }
        if (smsReceiver != null) {
            try { unregisterReceiver(smsReceiver); } catch (Exception ignored) {}
        }
        if (updateBridge != null) {
            updateBridge.cleanup();
        }
        if (ttsBridge != null) {
            ttsBridge.shutdown();
        }
        // KemperisService NESUSTABDOMAS čia — servisas turi gyventi
        // nepriklausomai nuo Activity lifecycle (monitoringo tikslas).
        // Servisas miršta tik kai sistema nužudo visą procesą (force-stop).
    }

    // ─── SMS gavimas ─────────────────────────────────────────────────────────
    private void registerSmsReceiver() {
        smsReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                try {
                    if (!Telephony.Sms.Intents.SMS_RECEIVED_ACTION.equals(intent.getAction())) return;
                    android.os.Bundle extras = intent.getExtras();
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
                    String from = sender;
                    final String js = "window.onSmsReceived && window.onSmsReceived("
                        + JSONObject.quote(from) + "," + JSONObject.quote(body) + ")";
                    runOnUiThread(() -> {
                        try {
                            getBridge().getWebView().evaluateJavascript(js, null);
                        } catch (Exception ignored) {}
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

    // ─── Auto WiFi bind ───────────────────────────────────────────────────────
    private void registerAutoWifiCallback() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return;
        ConnectivityManager cm = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);

        wifiCallback = new ConnectivityManager.NetworkCallback() {
            @Override
            public void onAvailable(@NonNull Network network) {
                if (autoBindPaused) return;
                NetworkCapabilities caps = cm.getNetworkCapabilities(network);
                if (caps == null) return;
                boolean isWifi = caps.hasTransport(NetworkCapabilities.TRANSPORT_WIFI);
                boolean hasNet = caps.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
                              && caps.hasCapability(NetworkCapabilities.NET_CAPABILITY_VALIDATED);
                if (isWifi && !hasNet) {
                    android.net.wifi.WifiInfo wifiInfo =
                        (android.net.wifi.WifiInfo) caps.getTransportInfo();
                    if (wifiInfo != null) {
                        String ssid = wifiInfo.getSSID();
                        if (ssid.startsWith("\"") && ssid.endsWith("\""))
                            ssid = ssid.substring(1, ssid.length() - 1);
                        // Android 10+ grąžina "<unknown ssid>" kai GPS išjungtas.
                        // Tokiu atveju leidžiame prisirišti — WiFi be interneto greičiausiai yra Kemperis AP.
                        boolean unknownSsid = ssid.isEmpty() || "<unknown ssid>".equals(ssid);
                        if (!unknownSsid && !"Kemperis-Valdymas".equals(ssid)) return;
                    }
                    boundNetwork = network;
                    cm.bindProcessToNetwork(network);
                    runOnUiThread(() -> Toast.makeText(
                        MainActivity.this, "🔌 Pririšta prie Kemperio AP", Toast.LENGTH_SHORT).show());
                }
            }

            @Override
            public void onLost(@NonNull Network network) {
                if (network.equals(boundNetwork)) {
                    boundNetwork = null;
                    cm.bindProcessToNetwork(null);
                }
            }
        };

        // SVARBU: removeCapability(NET_CAPABILITY_INTERNET) — be šito numatytasis
        // NetworkRequest reikalauja interneto, todėl callback'as NEPAGAUNA ESP WiFi
        // (jis be interneto) ir su įjungtais mobiliais duomenimis srautas neina į 192.168.4.1.
        NetworkRequest req = new NetworkRequest.Builder()
            .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
            .removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
            .build();
        cm.registerNetworkCallback(req, wifiCallback);
    }

    // ─── Permissions ──────────────────────────────────────────────────────────
    private void requestNecessaryPermissions() {
        List<String> permissions = new ArrayList<>();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
            permissions.add(Manifest.permission.POST_NOTIFICATIONS);
        permissions.add(Manifest.permission.ACCESS_FINE_LOCATION);
        permissions.add(Manifest.permission.SEND_SMS);
        permissions.add(Manifest.permission.RECEIVE_SMS);
        permissions.add(Manifest.permission.READ_PHONE_STATE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
            permissions.add(Manifest.permission.READ_PHONE_NUMBERS);
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q)
            permissions.add(Manifest.permission.WRITE_EXTERNAL_STORAGE);

        List<String> needed = new ArrayList<>();
        for (String p : permissions)
            if (ContextCompat.checkSelfPermission(this, p) != PackageManager.PERMISSION_GRANTED)
                needed.add(p);
        if (!needed.isEmpty())
            ActivityCompat.requestPermissions(this, needed.toArray(new String[0]), PERMISSION_REQUEST_CODE);
    }

    // ─── SMS bridge ───────────────────────────────────────────────────────────
    public class SmsBridge {
        private final Context ctx;
        SmsBridge(Context c) { ctx = c; }

        @JavascriptInterface
        public boolean hasPermission() {
            return ContextCompat.checkSelfPermission(ctx, Manifest.permission.SEND_SMS)
                == PackageManager.PERMISSION_GRANTED;
        }

        @JavascriptInterface
        public void send(String number, String text) {
            if (number == null || number.isEmpty() || text == null || text.isEmpty()) return;
            if (ContextCompat.checkSelfPermission(ctx, Manifest.permission.SEND_SMS)
                    != PackageManager.PERMISSION_GRANTED) {
                runOnUiThread(() -> Toast.makeText(ctx,
                    "⚠️ Nėra SMS leidimo", Toast.LENGTH_SHORT).show());
                return;
            }
            try {
                SmsManager sm;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                    sm = ctx.getSystemService(SmsManager.class);
                } else {
                    int subId = SubscriptionManager.getDefaultSmsSubscriptionId();
                    sm = (subId != SubscriptionManager.INVALID_SUBSCRIPTION_ID)
                        ? SmsManager.getSmsManagerForSubscriptionId(subId)
                        : SmsManager.getDefault();
                }
                if (sm == null) {
                    runOnUiThread(() -> Toast.makeText(ctx,
                        "⚠️ SIM kortelė nerasta", Toast.LENGTH_SHORT).show());
                    return;
                }
                sm.sendTextMessage(number, null, text, null, null);
            } catch (Exception e) {
                final String err = e.getMessage() != null ? e.getMessage() : "klaida";
                runOnUiThread(() -> Toast.makeText(ctx,
                    "SMS klaida: " + err, Toast.LENGTH_SHORT).show());
            }
        }

        @JavascriptInterface
        public String getOwnPhoneNumber() {
            try {
                TelephonyManager tm = (TelephonyManager)
                    ctx.getSystemService(Context.TELEPHONY_SERVICE);
                if (tm == null) return "";
                boolean hasPerm;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                    hasPerm = ContextCompat.checkSelfPermission(ctx,
                        Manifest.permission.READ_PHONE_NUMBERS)
                        == PackageManager.PERMISSION_GRANTED;
                } else {
                    hasPerm = ContextCompat.checkSelfPermission(ctx,
                        Manifest.permission.READ_PHONE_STATE)
                        == PackageManager.PERMISSION_GRANTED;
                }
                if (!hasPerm) return "";
                String num = tm.getLine1Number();
                return num != null ? num.trim() : "";
            } catch (Exception e) {
                return "";
            }
        }
    }

    // ─── Update bridge ────────────────────────────────────────────────────────
    public class UpdateBridge {
        private final Context ctx;
        private final WebView webView;
        private volatile long downloadId = -1;
        private BroadcastReceiver downloadReceiver = null;

        void cleanup() {
            if (downloadReceiver != null) {
                try { ctx.unregisterReceiver(downloadReceiver); } catch (Exception ignored) {}
                downloadReceiver = null;
            }
        }

        UpdateBridge(Context c) {
            ctx = c;
            webView = MainActivity.this.getBridge().getWebView();
        }

        @JavascriptInterface
        public int getCurrentVersion() { return CURRENT_VERSION; }

        @JavascriptInterface
        public void checkUpdate() {
            new Thread(() -> {
                autoBindPaused = true;
                ConnectivityManager cm = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
                    cm.bindProcessToNetwork(null);
                try {
                    URL url = new URL(VERSION_JSON_URL);
                    HttpURLConnection conn = (HttpURLConnection) url.openConnection();
                    conn.setConnectTimeout(15_000);
                    conn.setReadTimeout(15_000);
                    conn.setRequestProperty("User-Agent", "KempervanasApp-v20");
                    conn.setInstanceFollowRedirects(true);
                    conn.connect();
                    int status = conn.getResponseCode();
                    if (status == HttpURLConnection.HTTP_MOVED_TEMP || status == HttpURLConnection.HTTP_MOVED_PERM) {
                        String newUrl = conn.getHeaderField("Location");
                        conn.disconnect();
                        conn = (HttpURLConnection) new URL(newUrl).openConnection();
                        conn.setConnectTimeout(10_000);
                        conn.setReadTimeout(10_000);
                    }
                    BufferedReader br = new BufferedReader(new InputStreamReader(conn.getInputStream()));
                    StringBuilder sb = new StringBuilder();
                    String line;
                    while ((line = br.readLine()) != null) sb.append(line);
                    br.close();
                    try { conn.disconnect(); } catch (Exception ignored) {}

                    JSONObject json = new JSONObject(sb.toString());
                    int remoteVer = json.getInt("version");
                    String apkUrl = json.getString("apk_url");
                    String notes  = json.optString("notes", "");

                    final String jsResult;
                    if (remoteVer > CURRENT_VERSION) {
                        jsResult = String.format(
                            "window.onUpdateResult && window.onUpdateResult('update',%s,%s,%s)",
                            JSONObject.quote(String.valueOf(remoteVer)),
                            JSONObject.quote(apkUrl),
                            JSONObject.quote(notes));
                    } else {
                        jsResult = "window.onUpdateResult && window.onUpdateResult('latest',null,null)";
                    }
                    runOnUiThread(() -> webView.evaluateJavascript(jsResult, null));
                } catch (Exception e) {
                    final String err = e.getMessage() != null ? e.getMessage() : "klaida";
                    runOnUiThread(() -> webView.evaluateJavascript(
                        "window.onUpdateResult && window.onUpdateResult('error',null," + JSONObject.quote(err) + ")", null));
                } finally {
                    autoBindPaused = false;
                    ConnectivityManager cm2 = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && boundNetwork != null)
                        cm2.bindProcessToNetwork(boundNetwork);
                }
            }).start();
        }

        @JavascriptInterface
        public void downloadAndInstall(String apkUrl) {
            autoBindPaused = true;
            ConnectivityManager cm = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
                cm.bindProcessToNetwork(null);

            File apkFile = new File(ctx.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS), "kemperis_update.apk");
            if (apkFile.exists()) apkFile.delete();

            DownloadManager dm = (DownloadManager) ctx.getSystemService(Context.DOWNLOAD_SERVICE);
            DownloadManager.Request req = new DownloadManager.Request(Uri.parse(apkUrl))
                .setTitle("Kemperis atnaujinimas")
                .setDescription("Parsisiunčiama nauja versija...")
                .setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED)
                .setDestinationInExternalFilesDir(ctx, Environment.DIRECTORY_DOWNLOADS, "kemperis_update.apk")
                .setAllowedOverMetered(true)
                .setAllowedOverRoaming(true);

            try {
                downloadId = dm.enqueue(req);
            } catch (Exception e) {
                autoBindPaused = false;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && boundNetwork != null)
                    cm.bindProcessToNetwork(boundNetwork);
                final String err = e.getMessage() != null ? e.getMessage().replace("'", "") : "enqueue klaida";
                runOnUiThread(() -> webView.evaluateJavascript(
                    "window.onUpdateResult && window.onUpdateResult('install_error',null,'" + err + "')", null));
                return;
            }

            downloadReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    long id = intent.getLongExtra(DownloadManager.EXTRA_DOWNLOAD_ID, -1);
                    if (id != downloadId) return;
                    cleanup();
                    autoBindPaused = false;
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && boundNetwork != null)
                        cm.bindProcessToNetwork(boundNetwork);
                    try {

                    DownloadManager.Query q = new DownloadManager.Query().setFilterById(downloadId);
                    android.database.Cursor cursor = dm.query(q);
                    try {
                        if (cursor != null && cursor.moveToFirst()) {
                            int statusCol = cursor.getColumnIndex(DownloadManager.COLUMN_STATUS);
                            if (statusCol >= 0 && cursor.getInt(statusCol) == DownloadManager.STATUS_SUCCESSFUL) {
                                installApk(apkFile);
                            } else {
                                runOnUiThread(() -> webView.evaluateJavascript(
                                    "window.onUpdateResult && window.onUpdateResult('install_error',null,'Parsisiuntimas nepavyko')", null));
                            }
                        } else {
                            // DownloadManager grąžino tuščią rezultatą — nežinoma klaida
                            runOnUiThread(() -> webView.evaluateJavascript(
                                "window.onUpdateResult && window.onUpdateResult('install_error',null,'DownloadManager neatsakė')", null));
                        }
                    } catch (Exception ex) {
                        final String emsg = ex.getMessage() != null ? ex.getMessage().replace("'","") : "klaida";
                        runOnUiThread(() -> webView.evaluateJavascript(
                            "window.onUpdateResult && window.onUpdateResult('install_error',null,'" + emsg + "')", null));
                    } finally {
                        if (cursor != null) try { cursor.close(); } catch (Exception ignored) {}
                    }
                    } catch (Exception recvEx) {
                        final String re = recvEx.getMessage() != null ? recvEx.getMessage().replace("'","") : "onReceive klaida";
                        runOnUiThread(() -> webView.evaluateJavascript(
                            "window.onUpdateResult && window.onUpdateResult('install_error',null,'" + re + "')", null));
                    }
                }
            };
            // ACTION_DOWNLOAD_COMPLETE siunčia SISTEMA (išorinis procesas) —
            // RECEIVER_EXPORTED būtinas, nes RECEIVER_NOT_EXPORTED blokuoja sistemos broadcastus.
            // Šaltinis: https://issuetracker.google.com/issues/299327276
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                ctx.registerReceiver(downloadReceiver,
                    new IntentFilter(DownloadManager.ACTION_DOWNLOAD_COMPLETE),
                    Context.RECEIVER_EXPORTED);
            } else {
                ctx.registerReceiver(downloadReceiver,
                    new IntentFilter(DownloadManager.ACTION_DOWNLOAD_COMPLETE));
            }
            runOnUiThread(() -> webView.evaluateJavascript(
                "window.onUpdateResult && window.onUpdateResult('downloading',null,null)", null));
        }

        private void installApk(File apkFile) {
            if (!ctx.getPackageManager().canRequestPackageInstalls()) {
                runOnUiThread(() -> {
                    Intent si = new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                        Uri.parse("package:" + ctx.getPackageName()));
                    si.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    ctx.startActivity(si);
                    webView.evaluateJavascript(
                        "window.onUpdateResult && window.onUpdateResult('install_error',null," +
                        "'Leiskite diegti iš nežinomų šaltinių ir bandykite dar kartą')", null);
                });
                return;
            }
            Uri uri = FileProvider.getUriForFile(ctx, ctx.getPackageName() + ".fileprovider", apkFile);
            Intent installIntent = new Intent(Intent.ACTION_VIEW)
                .setDataAndType(uri, "application/vnd.android.package-archive")
                .setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_ACTIVITY_NEW_TASK);

            // Android 12+ blokuoja startActivity() iš BroadcastReceiver/fono.
            // Rodome pranešimą — vartotojas paspaudžia ir diegia.
            android.app.PendingIntent pi = android.app.PendingIntent.getActivity(
                ctx, 0, installIntent,
                android.app.PendingIntent.FLAG_UPDATE_CURRENT |
                android.app.PendingIntent.FLAG_IMMUTABLE);

            android.app.NotificationManager nm =
                (android.app.NotificationManager) ctx.getSystemService(Context.NOTIFICATION_SERVICE);
            String channelId = "kemperis_install";
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                android.app.NotificationChannel ch = new android.app.NotificationChannel(
                    channelId, "Kemperis Diegimas",
                    android.app.NotificationManager.IMPORTANCE_HIGH);
                nm.createNotificationChannel(ch);
            }
            // NotificationCompat.Builder — suderinama su API 24+ (ne tik API 26+)
            android.app.Notification notif = new androidx.core.app.NotificationCompat.Builder(ctx, channelId)
                .setSmallIcon(android.R.drawable.ic_dialog_info)
                .setContentTitle("Kemperis atnaujinimas paruoštas")
                .setContentText("Paspauskite norėdami įdiegti naują versiją")
                .setAutoCancel(true)
                .setContentIntent(pi)
                .setPriority(androidx.core.app.NotificationCompat.PRIORITY_HIGH)
                .build();
            nm.notify(9001, notif);

            // Bandome paleisti iš karto (veikia kai app yra priekiniame plane)
            try { ctx.startActivity(installIntent); } catch (Exception ignored) {}

            runOnUiThread(() -> webView.evaluateJavascript(
                "window.onUpdateResult && window.onUpdateResult('install_ready',null,null)", null));
        }
    }

    // ─── Native TTS bridge ────────────────────────────────────────────────────
    public static class KempTtsBridge implements TextToSpeech.OnInitListener {
        private final Context ctx;
        private final Activity activity;
        private final WebView webView;
        private TextToSpeech tts;
        private volatile boolean ready = false;

        KempTtsBridge(Activity act, WebView wv) {
            ctx = act;
            activity = act;
            webView = wv;
            tts = new TextToSpeech(act, this);
        }

        @Override
        public void onInit(int status) {
            if (status == TextToSpeech.SUCCESS) {
                ready = true;
                tts.setOnUtteranceProgressListener(new UtteranceProgressListener() {
                    @Override public void onStart(String uid) {}
                    @Override public void onDone(String uid) {
                        activity.runOnUiThread(() ->
                            webView.evaluateJavascript(
                                "window.onTtsDone && window.onTtsDone()", null));
                    }
                    @Override public void onError(String uid) {
                        activity.runOnUiThread(() ->
                            webView.evaluateJavascript(
                                "window.onTtsDone && window.onTtsDone()", null));
                    }
                });
                SharedPreferences prefs = ctx.getSharedPreferences("kemperis", Context.MODE_PRIVATE);
                applyLang(prefs.getString("tts_lang", "en"));
            }
        }

        private void applyLang(String lang) {
            if (!ready) return;
            Locale locale = "lt".equals(lang) ? new Locale("lt", "LT") : Locale.ENGLISH;
            int res = tts.setLanguage(locale);
            if (res == TextToSpeech.LANG_MISSING_DATA || res == TextToSpeech.LANG_NOT_SUPPORTED)
                tts.setLanguage(Locale.ENGLISH);
        }

        @JavascriptInterface
        public void speak(String text) {
            if (!ready || text == null || text.isEmpty()) return;
            tts.speak(text, TextToSpeech.QUEUE_FLUSH, null, "k_" + System.currentTimeMillis());
        }

        @JavascriptInterface
        public void setLang(String lang) {
            applyLang(lang);
            SharedPreferences prefs = ctx.getSharedPreferences("kemperis", Context.MODE_PRIVATE);
            prefs.edit().putString("tts_lang", lang).apply();
        }

        @JavascriptInterface
        public void stop() { if (ready) tts.stop(); }

        @JavascriptInterface
        public boolean isReady() { return ready; }

        void shutdown() {
            ready = false;
            if (tts != null) { tts.stop(); tts.shutdown(); tts = null; }
        }
    }

    // ─── Volume bridge ────────────────────────────────────────────────────────
    public static class VolBridge {
        private final Context ctx;
        VolBridge(Context c) { ctx = c; }

        @JavascriptInterface
        public int getVolumePct() {
            AudioManager am = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);
            int max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
            int cur = am.getStreamVolume(AudioManager.STREAM_MUSIC);
            return max > 0 ? Math.round(cur * 100f / max) : 0;
        }

        @JavascriptInterface
        public void setVolumePct(int pct) {
            AudioManager am = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);
            int max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
            am.setStreamVolume(AudioManager.STREAM_MUSIC,
                Math.round(max * Math.max(0, Math.min(100, pct)) / 100f), 0);
        }
    }

    // ─── File bridge — išsaugo tekstinius failus į Downloads ─────────────────
    public class KemperisFileBridge {
        private final Context ctx;
        KemperisFileBridge(Context c) { ctx = c; }

        @JavascriptInterface
        public String saveTextFile(String filename, String mimeType, String content) {
            try {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    // Android 10+ — MediaStore.Downloads, leidimo nereikia
                    ContentValues values = new ContentValues();
                    values.put(MediaStore.MediaColumns.DISPLAY_NAME, filename);
                    values.put(MediaStore.MediaColumns.MIME_TYPE, mimeType);
                    values.put(MediaStore.MediaColumns.IS_PENDING, 1);
                    Uri collection = Uri.parse("content://media/external/downloads");
                    Uri itemUri = ctx.getContentResolver().insert(collection, values);
                    if (itemUri == null) return "error: insert failed";
                    try (OutputStream out = ctx.getContentResolver().openOutputStream(itemUri)) {
                        if (out == null) {
                            ctx.getContentResolver().delete(itemUri, null, null);
                            return "error: output stream null";
                        }
                        out.write(content.getBytes(StandardCharsets.UTF_8));
                    }
                    values.clear();
                    values.put(MediaStore.MediaColumns.IS_PENDING, 0);
                    ctx.getContentResolver().update(itemUri, values, null, null);
                } else {
                    // Android 9 ir senesnė — tiesioginė failų sistema
                    if (ContextCompat.checkSelfPermission(ctx, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                            != PackageManager.PERMISSION_GRANTED) return "error: no WRITE permission";
                    File dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
                    if (!dir.exists() && !dir.mkdirs()) return "error: mkdir failed";
                    File file = new File(dir, filename);
                    try (FileOutputStream fos = new FileOutputStream(file)) {
                        fos.write(content.getBytes(StandardCharsets.UTF_8));
                    }
                }
                final String fname = filename;
                runOnUiThread(() -> Toast.makeText(ctx, "✅ Išsaugota: Downloads/" + fname, Toast.LENGTH_SHORT).show());
                return "ok";
            } catch (Exception e) {
                String msg = e.getMessage() != null ? e.getMessage() : "nežinoma klaida";
                return "error: " + msg;
            }
        }
    }

    // ─── WiFi bridge ──────────────────────────────────────────────────────────
    public class WifiBridge {
        private final Context ctx;
        WifiBridge(Context c) { ctx = c; }

        @JavascriptInterface
        public String getCurrentSSID() {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                try {
                    ConnectivityManager cm = (ConnectivityManager)
                        ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
                    for (Network n : cm.getAllNetworks()) {
                        NetworkCapabilities caps = cm.getNetworkCapabilities(n);
                        if (caps == null || !caps.hasTransport(NetworkCapabilities.TRANSPORT_WIFI))
                            continue;
                        android.net.wifi.WifiInfo wi =
                            (android.net.wifi.WifiInfo) caps.getTransportInfo();
                        if (wi == null) continue;
                        String ssid = wi.getSSID();
                        if (ssid == null) continue;
                        if (ssid.startsWith("\"") && ssid.endsWith("\""))
                            ssid = ssid.substring(1, ssid.length() - 1);
                        if (!ssid.isEmpty() && !ssid.equals("<unknown ssid>"))
                            return ssid;
                    }
                } catch (Exception ignored) {}
            }
            try {
                WifiManager wm = (WifiManager) ctx.getApplicationContext()
                    .getSystemService(Context.WIFI_SERVICE);
                WifiInfo info = wm.getConnectionInfo();
                if (info == null) return "";
                String ssid = info.getSSID();
                if (ssid == null) return "";
                if (ssid.startsWith("\"") && ssid.endsWith("\""))
                    ssid = ssid.substring(1, ssid.length() - 1);
                if (ssid.equals("<unknown ssid>")) return "";
                return ssid;
            } catch (Exception e) { return ""; }
        }

        @JavascriptInterface
        public void suggestWifi(String ssid, String password) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                WifiNetworkSuggestion s = new WifiNetworkSuggestion.Builder()
                    .setSsid(ssid).setWpa2Passphrase(password).build();
                List<WifiNetworkSuggestion> list = new ArrayList<>();
                list.add(s);
                WifiManager wm = (WifiManager) ctx.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
                if (wm.addNetworkSuggestions(list) == WifiManager.STATUS_NETWORK_SUGGESTIONS_SUCCESS)
                    runOnUiThread(() -> Toast.makeText(ctx,
                        "📶 Kemperio WiFi įtrauktas į automatinio jungimosi sąrašą",
                        Toast.LENGTH_SHORT).show());
            }
        }

        @JavascriptInterface
        public void openWifiSettings() {
            ctx.startActivity(new Intent(android.provider.Settings.ACTION_WIFI_SETTINGS)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        }

        @JavascriptInterface
        public void unbindNetwork() {
            autoBindPaused = true;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                ConnectivityManager cm = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
                cm.bindProcessToNetwork(null);
            }
        }

        @JavascriptInterface
        public void rebindNetwork() {
            autoBindPaused = false;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && boundNetwork != null) {
                ConnectivityManager cm = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
                cm.bindProcessToNetwork(boundNetwork);
            }
        }
    }
}


