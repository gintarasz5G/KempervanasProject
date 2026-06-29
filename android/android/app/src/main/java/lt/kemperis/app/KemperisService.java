package lt.kemperis.app;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.IBinder;
import androidx.core.app.NotificationCompat;

/**
 * KemperisService — Native Foreground Service
 *
 * Tikslas: užtikrinti, kad Android 14+ sistema nelaikytų programos kaip
 * "nedeklaruotos fono paslaugos" ir jos nenužudytų. Paleidžiamas iš
 * MainActivity.onCreate() prieš cordova-plugin-background-mode.
 *
 * Tipas: specialUse (saugos stebėjimas — dujų, CO2 aliarmai).
 * START_STICKY — sistema paleidžia iš naujo jei proceso atmintis atlaisvinama.
 */
public class KemperisService extends Service {

    static final String CHANNEL_ID = "kemperis_fg";
    static final int NOTIF_ID = 1001;

    @Override
    public void onCreate() {
        super.onCreate();
        createChannel();
        Notification notif = buildNotification();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            // Android 14 (API 34): startForeground() privaloma nurodyti tipą
            startForeground(NOTIF_ID, notif, ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE);
        } else {
            startForeground(NOTIF_ID, notif);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // START_STICKY: sistema paleidžia servisą iš naujo jei jį sunaikina
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null; // Bound service nenaudojamas
    }

    // ── Notifikacijos kanalas (Android 8+) ──────────────────────────────────
    private void createChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel ch = new NotificationChannel(
                CHANNEL_ID,
                "Kemperis stebėjimas",
                NotificationManager.IMPORTANCE_LOW // LOW = be garso, tik juostoje
            );
            ch.setDescription("Kemperio sensorių stebėjimas fone (dujų, CO2 aliarmai)");
            ch.setShowBadge(false);
            NotificationManager nm = getSystemService(NotificationManager.class);
            if (nm != null) nm.createNotificationChannel(ch);
        }
    }

    // ── Nuolatinė notifikacija ───────────────────────────────────────────────
    private Notification buildNotification() {
        Intent launchIntent = new Intent(this, MainActivity.class);
        launchIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);

        PendingIntent pi = PendingIntent.getActivity(
            this, 0, launchIntent,
            PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE
        );

        return new NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("Kemperis aktyvus")
            .setContentText("Stebimi sensoriai fone")
            .setSmallIcon(android.R.drawable.ic_dialog_info)
            .setContentIntent(pi)
            .setOngoing(true)          // Negalima stumti/pašalinti
            .setShowWhen(false)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .build();
    }
}
