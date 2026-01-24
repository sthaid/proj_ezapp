package org.libsdl.app;

import android.content.Context;
import android.util.Log;
import android.content.Intent;

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioPlaybackCaptureConfiguration;
import android.media.AudioRecord;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;

import android.os.SystemClock;

public class ezapp_playbackcapture {
    private static final int SAMPLE_RATE    = 48000;
    private static final int CHANNEL_CONFIG = AudioFormat.CHANNEL_IN_STEREO;
    private static final int AUDIO_FORMAT   = AudioFormat.ENCODING_PCM_16BIT;

    private static final String TAG             = "EZAPP";
    private static final int    PERMISSION_CODE = 1234; //xxx what is this

    private AudioRecord         audioRecord     = null;
    private MediaProjection     mediaProjection = null;
    private Intent              intent          = null;

    public void on_result(int requestCode, int resultCode, Intent data) {
        if (requestCode != PERMISSION_CODE) {
            // Handle error or unknown request code
            // xxx print error
            return;
        }

        if (resultCode != -1) {  // xxx RESULT_OK
            // User denied permission
            // xxx test this path; what should be done
            return;
        }

        intent = data;
    }

    public void startPlaybackCapture(SDLActivity mSingleton, Context cxarg) {
        Context                cx = cxarg;
        int                    bufferSizeInBytes;
        MediaProjectionManager projectionManager;

        if (intent != null) {
            Log.e(TAG, "startPlaybackCapture already running");
            return;
        }

        projectionManager = 
            (MediaProjectionManager) cx.getSystemService(Context.MEDIA_PROJECTION_SERVICE);

        mSingleton.startActivityForResult(projectionManager.createScreenCaptureIntent(), PERMISSION_CODE);

        while (intent == null) { //xxx needs timeout
            SystemClock.sleep(2000); // xxx ??
        }

        mediaProjection = projectionManager.getMediaProjection(-1, intent);
        if (mediaProjection == null) {
            Log.e(TAG, "getMediaProjection failed\n");
            return;
        }

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
            .setAudioPlaybackCaptureConfig(captureConfig) // **Key step**
            .build();

        // Start recording
        audioRecord.startRecording();
    }

    public void stopPlaybackCapture() {
        if (audioRecord != null) {
            audioRecord.stop();
            audioRecord.release();
            audioRecord = null;
        }
        if (mediaProjection != null) {
            mediaProjection.stop();
            mediaProjection = null;
        }

        intent = null;
    }

    public short[] get_playbackcapture_audio(int num_array_elements) {
        short[] array = new short[num_array_elements];
        int shorts_read;

        shorts_read = audioRecord.read(array, 0, num_array_elements);
        return array;
    }
}
