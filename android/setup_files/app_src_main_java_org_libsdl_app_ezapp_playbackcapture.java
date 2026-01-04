package org.libsdl.app;

import android.content.Context;
import android.util.Log;
import android.content.Intent;

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioPlaybackCaptureConfiguration;
import android.media.AudioRecord;
import android.media.projection.MediaProjection;
import android.os.ParcelFileDescriptor;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

import android.media.projection.MediaProjectionManager;
import android.os.SystemClock;

import android.media.AudioPlaybackCaptureConfiguration;

import java.io.File;  // xxx del and cleanup this file
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.charset.StandardCharsets;

// ... inside your Activity or Service ...

public class ezapp_playbackcapture {

    private static final String TAG = "SDL";

    private AudioRecord audioRecord = null;
    private Thread recordingThread = null;
    private boolean isRecording = false;

    // Configuration for the audio format
    private static final int SAMPLE_RATE = 44100;
    private static final int CHANNEL_CONFIG = AudioFormat.CHANNEL_IN_MONO;  // xxx stereo
    private static final int AUDIO_FORMAT = AudioFormat.ENCODING_PCM_16BIT;
    private int bufferSizeInBytes;

    private Context cx;

    private static final int PERMISSION_CODE = 1234; //xxx what is this
    private MediaProjectionManager mProjectionManager;
    private MediaProjection mediaProjection; // Assume this is initialized from a MediaProjectionManager request

    private Intent mdata;

    public void on_result(int requestCode, int resultCode, Intent data) {
        Log.v(TAG, "XXXXXXXXXX in onActivityResult requestCode " + requestCode);
        Log.v(TAG, "XXXXXXXXXX in onActivityResult resultCode " + resultCode);

        if (requestCode != PERMISSION_CODE) {
            // Handle error or unknown request code
            return;
        }
        if (resultCode != -1) {  // xxx RESULT_OK
            // User denied permission
            return;
        }

        mdata = data;
    }

    public void startPlaybackCapture(SDLActivity mSingleton, Context cxarg) {
        Log.v(TAG, "XXXXXXXX startPlaybackCapture starting");
        cx = cxarg;

        if (mdata != null) {
            Log.v(TAG, "XXXXXXXX ERROR startPlaybackCapture already running");
            return;
        }

        mProjectionManager = 
            (MediaProjectionManager) cx.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
        Log.v(TAG, "XXXXXXXXXX mProjectionManger = " + mProjectionManager);

        mSingleton.startActivityForResult(mProjectionManager.createScreenCaptureIntent(), PERMISSION_CODE);
        Log.v(TAG, "XXXXXXXXXX after  mSingleton.startActivityForResult");

        while (mdata == null) { //xxx needs timeout
            SystemClock.sleep(2000);
        }

        mediaProjection = mProjectionManager.getMediaProjection(-1, mdata);
        if (mediaProjection == null) {
            Log.v(TAG, "XXXXXXXXXX getMediaProjection failed\n");
            return;
        }

        Log.v(TAG, "XXXXXXXX 1");
        bufferSizeInBytes = AudioRecord.getMinBufferSize(SAMPLE_RATE, CHANNEL_CONFIG, AUDIO_FORMAT);

        // 1. Build the AudioPlaybackCaptureConfiguration
        Log.v(TAG, "XXXXXXXX 2");
        AudioPlaybackCaptureConfiguration captureConfig =
            new AudioPlaybackCaptureConfiguration.Builder(mediaProjection)
                .addMatchingUsage(AudioAttributes.USAGE_MEDIA) // Capture media playback audio
                //.addMatchingUsage(AudioAttributes.USAGE_GAME)  // Capture game audio
                .build();

        // 2. Configure the AudioFormat
        Log.v(TAG, "XXXXXXXX 3");
        AudioFormat format = new AudioFormat.Builder()
            .setSampleRate(SAMPLE_RATE)
            .setChannelMask(CHANNEL_CONFIG)
            .setEncoding(AUDIO_FORMAT)
            .build();

        // 3. Initialize AudioRecord with the configuration
        Log.v(TAG, "XXXXXXXX 4");
        audioRecord = new AudioRecord.Builder()
            //.setAudioSource(MediaRecorder.AudioSource.DEFAULT) // Default source is fine for playback capture
            .setAudioFormat(format)
            .setBufferSizeInBytes(bufferSizeInBytes)
            .setAudioPlaybackCaptureConfig(captureConfig) // **Key step**
            .build();

        // Start recording
        Log.v(TAG, "XXXXXXXX 5");
        audioRecord.startRecording();
        isRecording = true;

        // Start a thread to read the audio data
        //Log.v(TAG, "XXXXXXXX 6");
        //recordingThread = new Thread(new Runnable() {
        //    @Override
        //    public void run() {
        //        writeAudioDataToFile();
        //    }
        //}, "AudioRecorderThread");
        //recordingThread.start();
        Log.v(TAG, "XXXXXXXX 7777");
    }

//NEW
//https://developer.android.com/reference/java/io/RandomAccessFile
//https://developer.android.com/reference/java/nio/MappedByteBuffer

    private void writeAudioDataToFile() {
        File file = new File(cx.getFilesDir(), "captured_audio.pcm");
        long fileSize = 0x100000;

        try (RandomAccessFile randomAccessFile = new RandomAccessFile(file, "rw");
             FileChannel fileChannel = randomAccessFile.getChannel()) {

            // Map the file into memory in READ_WRITE mode
            MappedByteBuffer mappedByteBuffer = fileChannel.map(
                FileChannel.MapMode.READ_WRITE, 
                0,      // Position in the file to start mapping
                fileSize // Size of the region to map
            );


            int bufferSizeInBytes = 10000;
            byte[] buffer = new byte[bufferSizeInBytes];
            int bytesRead;

            bytesRead = audioRecord.read(buffer, 0, bufferSizeInBytes);
            Log.v(TAG, "XXXX bytesRead " + bytesRead);
            //if (bytesRead > 0) {
                //fos.write(buffer, 0, bytesRead);
            //}

            // Write data to the memory-mapped buffer
            mappedByteBuffer.put(buffer);
            
            // Force the changes to be written to the underlying file
            mappedByteBuffer.force(); 
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
        }
    }
            
    public void stopPlaybackCapture() {
        isRecording = false;
        if (recordingThread != null) {
            try {
                recordingThread.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        if (audioRecord != null) {
            audioRecord.stop();
            audioRecord.release();
            audioRecord = null;
        }
        if (mediaProjection != null) {
            mediaProjection.stop();
            mediaProjection = null;
        }

        mdata = null;
    }

    public short[] get_playbackcapture_audio(int num_array_elements) {
        short[] array = new short[num_array_elements];
        int shorts_read;

        shorts_read = audioRecord.read(array, 0, num_array_elements);
        Log.v(TAG, "XXXX shorts_read " + shorts_read);

        return array;
    }
}
