// References
// https://developer.android.com/develop/background-work/services/fgs/launch
//  import android.widget.Toast;

// ------------------------------------------

package org.libsdl.app;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;

import android.content.pm.ServiceInfo;

import org.sthaid.ezApp.R;  // needed to access R.drawable.ic_notifcation_icon

public class ezapp_fgsvc extends Service {

    private static final String TAG = "EZAPP";
    private final        IBinder mBinder = new InnerBinder();
    private static int   running_count;

    public class InnerBinder extends Binder {
        ezapp_fgsvc getService() {
            return ezapp_fgsvc.this;
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String CHANNEL_ID          = "my_channel_id";
        String CHANNEL_NAME        = "My Channel";
        String CHANNEL_DESCRIPTION = "Description for My Channel";

        running_count = running_count + 1;
        Log.i(TAG, "starting fgsvc, running_count=" + running_count);

        NotificationManager notificationManager = (NotificationManager) this.getSystemService(this.NOTIFICATION_SERVICE);

        // Create a Notification Channel
        NotificationChannel channel = 
            new NotificationChannel(CHANNEL_ID, CHANNEL_NAME, NotificationManager.IMPORTANCE_DEFAULT);
        channel.setDescription(CHANNEL_DESCRIPTION);
        channel.enableVibration(true);
        channel.setLockscreenVisibility(Notification.VISIBILITY_PUBLIC); 
        notificationManager.createNotificationChannel(channel);

        // Build the notification
        Notification.Builder builder;
        builder = new Notification.Builder(this, CHANNEL_ID);
        Notification notification = builder
                .setContentTitle("foreground enabled")
                 //.setContentText("more text here if needed")
                .setSmallIcon(R.drawable.ic_notifcation_icon)
                .setOngoing(true)
                .setPriority(Notification.PRIORITY_DEFAULT)
                .setAutoCancel(true)
                .setVisibility(Notification.VISIBILITY_PUBLIC)
                .build();

        // startForeground, '100' is the notification id  xxx review
        this.startForeground(100, notification, 
                  ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION |
                  ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PROJECTION
                                            );

        // Service will be restarted if killed by the system
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void onDestroy() {
        running_count = running_count - 1;
        Log.i(TAG, "stopping fgsvc, running_count=" + running_count);

        stopForeground(true);
        stopSelf();
    }
}
