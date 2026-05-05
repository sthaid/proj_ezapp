package org.libsdl.app;

import android.os.Build;
import android.content.Context;
import android.util.Log;
import android.os.Binder;
import android.os.IBinder;

import android.speech.tts.TextToSpeech;
import java.util.Locale;

import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationCallback;
import com.google.android.gms.location.LocationResult;
import android.location.Location;
import android.location.altitude.AltitudeConverter;
import android.os.Looper;

import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraManager;
import android.view.Gravity;

import java.io.IOException;

public class ezapp_utils {
    private static final String                TAG = "EZAPP";
    private static final int                   INVALID_NUMBER = 999999999;
    private static final double                METERS_TO_FEET = 3.28084;
    private static final int                   MSL   = 0;
    private static final int                   WGS84 = 1;

    private static TextToSpeech                mTts;
    private static boolean                     isTtsInitialized = false;

    private static FusedLocationProviderClient fusedLocationClient;
    private static LocationCallback            locationCallback;
    private static double                      latitude    = INVALID_NUMBER;
    private static double                      longitude   = INVALID_NUMBER;
    private static double                      altitude_ft = INVALID_NUMBER;
    private static int                         alt_type;

    private static CameraManager               cameraManager;
    private static String                      cameraId;
    private static boolean                     flashlight_is_on = false;

    //
    // constructor
    //

    public ezapp_utils(Context cx) {
        Log.i(TAG, "utils init");

        //
        // Initialize TextToSpeech support
        //

        mTts = new TextToSpeech(cx, new TextToSpeech.OnInitListener() {
            @Override
            public void onInit(int status) { 
                if (status != TextToSpeech.ERROR) {
                    int result = mTts.setLanguage(Locale.US);
                    if (result == TextToSpeech.LANG_MISSING_DATA || result == TextToSpeech.LANG_NOT_SUPPORTED) {
                        Log.e(TAG, "TTS Lang not supported");
                    } else {
                        isTtsInitialized = true;
                    }
                }
            } 
        });

        //
        // Initialize Location support
        //

        fusedLocationClient = LocationServices.getFusedLocationProviderClient(cx);
        AltitudeConverter altitudeConverter = new AltitudeConverter();

        locationCallback = new LocationCallback() {
            @Override
            public void onLocationResult(LocationResult locationResult) {
                if (locationResult == null) {
                    latitude    = INVALID_NUMBER;
                    longitude   = INVALID_NUMBER;
                    altitude_ft = INVALID_NUMBER;
                    return;
                }

                for (Location location : locationResult.getLocations()) {
                    // get current latitude and longitude
                    latitude = location.getLatitude();
                    longitude = location.getLongitude();

                    // try to attach altitude Mean-Sea-Lvel (MSL) converter;
                    // when attached, getMslAltitudeMeters can be called, which provides
                    // more accurate altitude than getAltitude
                    try {
                        altitudeConverter.addMslAltitudeToLocation(cx, location);
                    } catch (IOException e) {
                        Log.e(TAG, "addMsAltitudeToLocation failed");
                    }

                    // get altitude, first try to get MSL altitude, 
                    // if MSL not available then try to get WGS84 altitude;
                    if (location.hasMslAltitude()) {
                        altitude_ft = location.getMslAltitudeMeters() * METERS_TO_FEET;
                        alt_type = MSL;
                    } else if (location.hasAltitude()) {
                        altitude_ft = location.getAltitude() * METERS_TO_FEET;
                        alt_type = WGS84;
                    } else {
                        altitude_ft = INVALID_NUMBER;
                    }
                        
                    // debug print location/altitude result
                    Log.i(TAG, "lat/long/alt = " + 
                          latitude + " " + longitude + " " + 
                          altitude_ft + " ft " + (alt_type == MSL ? "MSL" : "WGS84"));
                }
            }
        };

        LocationRequest locationRequest = LocationRequest.create();
        locationRequest.setInterval(180*1000);  // 3 minute update interval
        locationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY);
        fusedLocationClient.requestLocationUpdates(locationRequest, locationCallback, Looper.getMainLooper());

        //
        // Initialize flashlight support
        //

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            cameraManager = (CameraManager) cx.getSystemService(Context.CAMERA_SERVICE);
            try {
                // get the first camera ID
                cameraId = cameraManager.getCameraIdList()[0];
            } catch (CameraAccessException e) {
                Log.e(TAG, "CameraAccessException");
            }
        }
    }

    //
    // cleanup
    //

    public void destroy() {
        Log.i(TAG, "utils destroy");
        if (mTts != null) {
            mTts.stop();
            mTts.shutdown();
            mTts = null;
        }

        // xxx other stuff to do here?
    }

    //
    // text to speech
    //

    // return 0 on success, INVALID_NUMBER on failure
    public int text_to_speech(String message) {
        int status;

        if (isTtsInitialized && mTts != null) {
            if (message.length() > 0) {
                Log.i(TAG, "tts speaking: " + message);
                status = mTts.speak(message, TextToSpeech.QUEUE_FLUSH, null, "utteranceId1"); // xxx what are the args
            } else {
                Log.i(TAG, "tts stopping");
                status = mTts.stop();
            }
            return status == 0 ? 0 : INVALID_NUMBER;
        } else {
            return INVALID_NUMBER;
        }
    }

    //
    // location
    //

    public double get_latitude() {
        return latitude;
    }

    public double get_longitude() {
        return longitude;
    }

    public double get_altitude() {
        if (alt_type == MSL) {
            return altitude_ft;
        } else {  // alt_type == WGS84
            // caller must check for the added 1000000,
            // which indicates altitude type is WGS84
            return altitude_ft + 1000000;
        }
    }

    //
    // flashlight
    //

    public void turn_flashlight_on() {
        if (cameraManager == null || cameraId == null) {
            Log.e(TAG, "flashlight not supported");
            return;
        }

        try {
            Log.i(TAG, "turning flashlight on");
            cameraManager.setTorchMode(cameraId, true);
            flashlight_is_on = true;
            SDLActivity.showToast("Flashlight On", 0, Gravity.CENTER, 0, 0);
        } catch (CameraAccessException e) {
            Log.e(TAG, "CameraAccessException");
        }
    }

    public void turn_flashlight_off() {
        if (cameraManager == null || cameraId == null) {
            Log.e(TAG, "flashlight not supported");
            return;
        }

        try {
            Log.i(TAG, "turning flashlight off");
            cameraManager.setTorchMode(cameraId, false);
            flashlight_is_on = false;
            SDLActivity.showToast("Flashlight Off", 0, Gravity.CENTER, 0, 0);
        } catch (CameraAccessException e) {
            Log.e(TAG, "CameraAccessException");
        }
    }

    public boolean is_flashlight_on() {
        return flashlight_is_on;
    }

    public void toggle_flashlight() {
        if (flashlight_is_on) {
            turn_flashlight_off();
        } else {
            turn_flashlight_on();
        }
    }
}
