package org.libsdl.app;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;
import android.content.Context;
import android.os.SystemClock;
import android.content.pm.ServiceInfo;  // xxx are these all needed

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;

import org.sthaid.ezApp.R;  // needed to access R.drawable.ic_notifcation_icon

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioPlaybackCaptureConfiguration;
import android.media.AudioRecord;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;

public class ezapp_media_fgsvc extends Service {

    private static final     String TAG      = "EZAPP";
    private final            IBinder mBinder = new InnerBinder();
    private int              start_id;      
 
    private MediaProjection  mediaProjection = null;
    static  AudioRecord      audioRecord     = null;        // xxx static

    private static final int STATE_IDLE         = 0;
    private static final int STATE_INITIALIZING = 1;
    private static final int STATE_RECORDING    = 2;
    private static final int STATE_FAILED       = 3;
    static  int              state              = STATE_IDLE;  // xxx static  private?

    public class InnerBinder extends Binder {
        ezapp_media_fgsvc getService() {
            return ezapp_media_fgsvc.this;
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int start_id_arg) {
        String CHANNEL_ID          = "ezapp_channel";
        String CHANNEL_NAME        = "ezapp";
        String CHANNEL_DESCRIPTION = "description";
        int    NOTIFICATION_ID     = 101;

        int SAMPLE_RATE    = 48000;
        int CHANNEL_CONFIG = AudioFormat.CHANNEL_IN_STEREO;
        int AUDIO_FORMAT   = AudioFormat.ENCODING_PCM_16BIT;
        int bufferSizeInBytes;
        MediaProjectionManager projectionManager;

        int    resultCode = intent.getIntExtra("resultCode", 0);  // xxx RESULT_CANCELED);
        Intent data       = intent.getParcelableExtra("data");

        // print starting msg, and save start_id to be used when stopping this service
        Log.i(TAG, "starting media fgsvc, start_id = " + start_id_arg);
        start_id = start_id_arg;

        // if recording then print message and return
        if (state == STATE_RECORDING) {
            Log.e(TAG, "recording in progress");
            return START_NOT_STICKY;
        }

        // set state to initializing
        state = STATE_INITIALIZING;

        // Create a Notification Channel
        NotificationManager notificationManager =
            (NotificationManager) this.getSystemService(this.NOTIFICATION_SERVICE);
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

        // startForeground
        this.startForeground(NOTIFICATION_ID, notification, 
                  ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PROJECTION);

        // access mediaProjection
        projectionManager = (MediaProjectionManager) getSystemService(Context.MEDIA_PROJECTION_SERVICE);
        mediaProjection = projectionManager.getMediaProjection(resultCode, data);
        if (mediaProjection == null) {
            Log.e(TAG, "getMediaProjection failed\n");
            state = STATE_FAILED;;
            return START_NOT_STICKY;
        }

        // get minimum buffer size
        bufferSizeInBytes = AudioRecord.getMinBufferSize(SAMPLE_RATE, CHANNEL_CONFIG, AUDIO_FORMAT);

        // Build the AudioPlaybackCaptureConfiguration
        AudioPlaybackCaptureConfiguration captureConfig =
            new AudioPlaybackCaptureConfiguration.Builder(mediaProjection)
                .addMatchingUsage(AudioAttributes.USAGE_MEDIA) // Capture media playback audio
                //.addMatchingUsage(AudioAttributes.USAGE_GAME)  // Capture game audio   xxx why not ?
                .build();

        // Configure the AudioFormat
        AudioFormat format = new AudioFormat.Builder()
            .setSampleRate(SAMPLE_RATE)
            .setChannelMask(CHANNEL_CONFIG)
            .setEncoding(AUDIO_FORMAT)
            .build();

        // Initialize AudioRecord with the configuration
        audioRecord = new AudioRecord.Builder()
            //.setAudioSource(MediaRecorder.AudioSource.DEFAULT) // Default source is fine for playback capture
            .setAudioFormat(format)
            .setBufferSizeInBytes(bufferSizeInBytes)
            .setAudioPlaybackCaptureConfig(captureConfig) // key step
            .build();

        // Start recording
        Log.i(TAG, "start recording");
        audioRecord.startRecording();
        state = STATE_RECORDING;

        // do not restart service if killed by system
        return START_NOT_STICKY;
    }

    static public short[] get_playbackcapture_audio(int num_array_elements) {
        short[] array = new short[num_array_elements];
        int shorts_read = 0;

        while (true) {
            if (state == STATE_RECORDING) {
                break;
            }
            if (state == STATE_FAILED) {
                Log.e(TAG, "recording has failed");
                return array;  // xxx return an error
            }
            SystemClock.sleep(1000);
        }

        shorts_read = audioRecord.read(array, 0, num_array_elements);
        return array;  // xxx how to return an error
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "stopping media fgsvc");

        if (audioRecord != null) {
            audioRecord.stop();
            audioRecord.release();
            audioRecord = null;
        }

        if (mediaProjection != null) {
            mediaProjection.stop();
            mediaProjection = null;
        }

        state = STATE_IDLE;

        stopSelf(start_id);
    }
}
