// -----------------  ANDROID  ------------------------------------

#ifdef ANDROID

#include <sdlx.h>
#include <utils.h>
#include <logging.h>

#include <SDL3/SDL.h>
#include <jni.h>
#include <unistd.h>

// The following comment is copied from here:
//   https://wiki.libsdl.org/SDL3/SDL_GetAndroidActivity
// Warning (and discussion of implementation details of SDL for Android):
// Local references are automatically deleted if a native function called
// from Java side returns. For SDL this native function is main() itself.
// Therefore references need to be manually deleted because otherwise the
// references will first be cleaned if main() returns (application exit).

// Notes on altitude, from Google AI Overview:
//  "GPS altitude is a height above the WGS84 reference ellipsoid,
//   which is an approximation of the Earth's surface. This value is
//   not the same as height above mean sea level and may require a
//   correction"

// JNI based mehtod signatures:
// References:
//  https://udaniweeraratne.wordpress.com/2016/07/10/how-to-generate-jni-based-method-signature/
//
// goolge search "what are the args to GetMethodID" ...
//   Example of a method signature:
//   - (I)V: A method that takes an int as a parameter and returns void.
//   - (Ljava/lang/String;)I: A method that takes a String object as a parameter and returns an int.
//   - (Ljava/lang/String;I)V: A method that takes a String and an int as parameters and returns void.

// prototype of common routine to call java method
static double call_java1(const char *method_name);
static double call_java2(const char *method_name, char *str);
static double call_java3(const char *method_name, short *array, int num_array_elements);

// android utils init & destroy
void util_android_utils_init(void)
{
    call_java1("android_utils_init");
}

void util_android_utils_destroy(void)
{
    call_java1("android_utils_destroy");
}

// location
// note: altitude is in meters above the WGS84 ellipsoid; 
//       which can be more than 100 meters different than mean sea level
//       in some places
void util_get_location(double *latitude, double *longitude, double *altitude) 
{
    int         ms = 0;
    bool        failed, retries_allowd;
    static bool first_call = true;

    // retries are allowd only on the first call
    retries_allowd = first_call;
    first_call = false;

    // loop, allowing retries on the first call
    while (true) {
        // call android java code to get lat/long/alt
        failed = false;
        if (latitude) {
            *latitude = call_java1("get_latitude");
            if (*latitude == INVALID_NUMBER) failed = true;
        }
        if (longitude) {
            *longitude = call_java1("get_longitude");
            if (*longitude == INVALID_NUMBER) failed = true;
        }
        if (altitude) {
            *altitude = call_java1("get_altitude");
            if (*altitude == INVALID_NUMBER) failed = true;
        }

        // if retries are not allowed (this is not the first call) OR
        //    if the lat/long/alt values are valid
        // then return
        if (retries_allowd == false || failed == false) {
            return;
        }

        // delay and try again, with 2 sec timeout
        if (ms > 2000) {
            ERROR("timedout\n");
            return;
        }
        usleep(10000);
        ms += 10;
    }
}

// text to speech
void util_text_to_speech(char *text) {
    call_java2("text_to_speech", text);
}
void util_text_to_speech_stop(void) {
    char text[1] = { '\0' };
    call_java2("text_to_speech", text);
}

// foreground service
void util_start_foreground(void) {
    call_java1("start_foreground");
}
void util_stop_foreground(void) {
    call_java1("stop_foreground");
}
bool util_is_foreground_enabled(void) {
    return call_java1("is_foreground_enabled") == 1;
}

// flashlight
void util_turn_flashlight_on(void) {
    call_java1("turn_flashlight_on");
}
void util_turn_flashlight_off(void) {
    call_java1("turn_flashlight_off");
}
void util_toggle_flashlight(void) {
    call_java1("toggle_flashlight");
}
bool util_is_flashlight_on(void) {
    return call_java1("is_flashlight_on") == 1;
}

// playbackcapture
int util_start_playbackcapture(void) {
    return call_java1("start_playbackcapture");
}
void util_stop_playbackcapture(void) {
    call_java1("stop_playbackcapture");
}
int util_get_playbackcapture_audio(short *array, int num_array_elements) {
    return call_java3("get_playbackcapture_audio", array, num_array_elements);
}

// -----------------  COMMON ROUTINES TO CALL JAVA METHOD  -------------------------

// returns:
// - INVALID_NUMBER, when failed, or
// - method specific result value, such as:
//   - latitude, longitude, or altitude
//   - 0 or 1 for boolean
//   - 0 for success

// call method 'double proc()'
static double call_java1(const char *method_name)
{
    jmethodID method_id = 0;
    double method_ret_double = INVALID_NUMBER;

    // retrieve the JNI environment.,
    // retrieve the Java instance of the SDLActivity,
    // find the Java class of the activity. It should be SDLActivity or a subclass of it.
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    jclass clazz(env->GetObjectClass(activity));

    // get the method_id, print message if failed
    method_id = env->GetMethodID(clazz, method_name, "()D");

    // if got the method_id then call the method
    if (method_id != 0) {
        method_ret_double = env->CallDoubleMethod(activity, method_id);
    }

    // print error messages
    if (method_id == 0) {
        ERROR("failed to get method_id for %s\n", method_name);
    } else if (method_ret_double == INVALID_NUMBER) {
        ERROR("%s method returned failure\n", method_name);
    }

    // clean up
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);

    // return method result
    return method_ret_double;
}

// call method 'double proc(String s)'
static double call_java2(const char *method_name, char *arg_str)
{
    jmethodID method_id = 0;
    double method_ret_double = INVALID_NUMBER;

    // retrieve the JNI environment.,
    // retrieve the Java instance of the SDLActivity,
    // find the Java class of the activity. It should be SDLActivity or a subclass of it.
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    jclass clazz(env->GetObjectClass(activity));

    // get the method_id, print message if failed
    method_id = env->GetMethodID(clazz, method_name, "(Ljava/lang/String;)D");

    // if got the method_id then ...
    if (method_id != 0) {
        // Convert C string 'arg_str' to Java String
        // Note - When using JNI's NewStringUTF function, you are creating a new java.lang.String
        //        object within the Java Virtual Machine (JVM). This jstring is a local reference,
        //        and its memory management is handled by the JVM's garbage collector.
        jstring java_string = env->NewStringUTF(arg_str);

        // call method
        method_ret_double = env->CallDoubleMethod(activity, method_id, java_string);
    }

    // print error messages
    if (method_id == 0) {
        ERROR("failed to get method_id for %s\n", method_name);
    } else if (method_ret_double == INVALID_NUMBER) {
        ERROR("%s method returned failure\n", method_name);
    }

    // clean up
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);

    // return method result
    return method_ret_double;
}

// call method 'short[] proc(int arg_unused)' 
double call_java3(const char *method_name, short *caller_array, int num_array_elements)
{
    jmethodID method_id = 0;
    int arg_unused = 0;

    // retrieve the JNI environment.,
    // retrieve the Java instance of the SDLActivity,
    // find the Java class of the activity. It should be SDLActivity or a subclass of it.
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    jclass clazz(env->GetObjectClass(activity));

    // get the method_id, check for failure
    method_id = env->GetMethodID(clazz, method_name, "(I)[S");
    if (method_id == 0) {
        ERROR("failed to get method_id for %s\n", method_name);
        env->DeleteLocalRef(activity);
        env->DeleteLocalRef(clazz);
        return INVALID_NUMBER;
    }

    // call the java method, which will return the array of short elements
    jshortArray array = (jshortArray) env->CallObjectMethod(activity, method_id, arg_unused);
    if (array == nullptr) {
        ERROR("%s method failed\n", method_name);
        env->DeleteLocalRef(activity);
        env->DeleteLocalRef(clazz);
        return INVALID_NUMBER;
    }

    // extract array length and elements from the method returned jshortArray
    jsize length = env->GetArrayLength(array);
    jshort* array_elements = env->GetShortArrayElements(array, nullptr);
    if (length != num_array_elements) {
        ERROR("%s method returned unexpected length=%d, expected=%d\n", 
              method_name, length, num_array_elements);
        env->ReleaseShortArrayElements(array, array_elements, JNI_ABORT);
        env->DeleteLocalRef(activity);
        env->DeleteLocalRef(clazz);
        return INVALID_NUMBER;
    }

    // return array_elements to caller
    memcpy(caller_array, array_elements, num_array_elements*sizeof(short));

    // Release the array elements
    // - JNI_ABORT means changes made to array_elements are not copied back to the Java array.
    // - JNI_COMMIT would copy changes back.
    // - 0 means copy back changes and free the buffer (if a copy was made)
    env->ReleaseShortArrayElements(array, array_elements, JNI_ABORT);

    // clean up
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);

    // return success
    return 0;
}

#else

// -----------------  NOT ANDROID - TEST CODE  ---------------------------

#include <utils.h>

void util_android_utils_init(void) { }

void util_android_utils_destroy(void) { }

void util_get_location(double *latitude, double *longitude, double *altitude)
{
    #define BOLTON_MASS_LATITUDE     42.4334
    #define BOLTON_MASS_LONGITUDE   -71.6078
    #define BOLTON_MASS_ELEVATION    100    // Bolton elevation range is 63 to 201 meters

    if (latitude) {
        *latitude = BOLTON_MASS_LATITUDE;
    }
    if (longitude) {
        *longitude = BOLTON_MASS_LONGITUDE;
    }
    if (altitude) {
        *altitude = BOLTON_MASS_ELEVATION;
    }
}

void util_text_to_speech(char *text) { }
void util_text_to_speech_stop(void) { }

void util_start_foreground(void) { }
void util_stop_foreground(void) { }
bool util_is_foreground_enabled(void) { return false; }

void util_turn_flashlight_on(void) { }
void util_turn_flashlight_off(void) { }
void util_toggle_flashlight(void) { }
bool util_is_flashlight_on(void) { return false; }

int util_start_playbackcapture(void) { }
void util_stop_playbackcapture(void) { }
int util_get_playbackcapture_audio(short *array, int num_array_elements) { }

#endif
