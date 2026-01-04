#include <std_hdrs.h>

#include <sdlx.h>
#include <logging.h>
#include <utils.h>

#include <SDL3/SDL.h>

//
// defines
//

#define TEN_MS 10000

//
// variables
//

static int video_init_count;
static int audio_init_count;
static int sensor_init_count;

// -----------------  INIT / EXIT ------------------------

int sdlx_init(int subsys)
{
    int rc;

    if (subsys & SUBSYS_VIDEO) {
        if (video_init_count == 0) {
            rc = sdlx_video_init();
            if (rc != 0) {
                ERROR("failed to init video\n");
                SDL_Quit();
                return -1;
            }
        }
        video_init_count++;
    }

    if (subsys & SUBSYS_AUDIO) {
        if (audio_init_count == 0) {
            rc = sdlx_audio_init();
            if (rc != 0) {
                ERROR("failed to init audio\n");
                SDL_Quit();
                return -1;
            }
        }
        audio_init_count++;
    }

    if (subsys & SUBSYS_SENSOR) {
        if (sensor_init_count == 0) {
            rc = sdlx_sensor_init();
            if (rc != 0) {
                ERROR("failed to init sensor\n");
                SDL_Quit();
                return -1;
            }
        }
        sensor_init_count++;
    }

    return 0;
}

void sdlx_quit(int subsys)
{
    if (subsys & SUBSYS_VIDEO) {
        if (--video_init_count == 0) {
            sdlx_video_quit();
        }
    }

    if (subsys & SUBSYS_AUDIO) {
        if (--audio_init_count == 0) {
            sdlx_audio_quit();
        }
    }

    if (subsys & SUBSYS_SENSOR) {
        if (--sensor_init_count == 0) {
            sdlx_sensor_quit();
        }
    }

    if (video_init_count <= 0 && 
        audio_init_count <= 0 &&
        sensor_init_count <= 0)
    {
        SDL_Quit();
    }
}

// -----------------  MISC ROUTINES NOT MADE AVAILABLE IN PICOC  ---------------------- 

// - - - - - - - - - sdlx_get_storage_path  - - - - - - - - - - 

char *sdlx_get_storage_path(void)
{
#ifdef ANDROID
    return (char*)SDL_GetAndroidInternalStoragePath();
#else  // not Android
    static char storage_path_buff[200];

    if (storage_path_buff[0] == '\0') {
        getcwd(storage_path_buff, sizeof(storage_path_buff));
    }

    return storage_path_buff;
#endif
}

// - - - - - - - - - sdlx_copy_asset_file - - - - - - - - - - - 

void sdlx_copy_asset_file(char *asset_filename, char *dest_dir)
{
    int rc;
    char dest_path[200];

    sprintf(dest_path, "%s/%s", dest_dir, asset_filename);

#ifdef ANDROID
    void  *ptr;
    size_t len;

    // remove dest file, because it may already exist
    unlink(dest_path);

    // read the asset using SDL_LoadFile;
    //
    // Note SDL_LoadFile calls SDL_IOFromFile, which attempts to read
    // the file as follows:
    // - if filename begins with '/' then use fopen
    //   else if filename begins with "content://" then use Android_JNI_OpenFileDescriptor
    //   else fopen of file in SDL_GetAndroidInternalStoragePath
    //   endif
    // - if above failed then try to read the file from assets, using Android_JNI_FileOpen
    ptr = SDL_LoadFile(asset_filename, &len);
    if (ptr == NULL ) {
        ERROR("failed to read apps.tar");
        return;
    }

    // write the asset file to dest_dir
    rc = util_write_file(dest_dir, asset_filename, ptr, len);
    SDL_free(ptr);
    if (rc != 0) {
        ERROR("failed to create %s/%s\n", dest_dir, asset_filename);
        return;
    }
#else  // not Android
    char cmd[250];
    //char *storage_path;  xxx

    //storage_path = sdlx_get_storage_path();
    //sprintf(cmd, "cp %s/../assets/%s %s", storage_path, asset_filename, dest_path);
    sprintf(cmd, "cp ../assets/%s %s", asset_filename, dest_path);
    rc = system(cmd);
    if (rc != 0) {
        ERROR("cmd '%s' failed\n", cmd);
    }
#endif
}

// - - - - - - - - - sdlx_get_permission  - - - - - - - - - - - 

#ifdef ANDROID
#define PERM_NO_RESULT    0
#define PERM_GRANTED      1
#define PERM_NOT_GRANTED  2
static void get_permission_cb(void *userdata, const char *permission, bool granted)
{
    int *perm_result = (int*)userdata;

    INFO("permission=%s  granted=%d\n", permission, granted);
    *perm_result = (granted ? PERM_GRANTED : PERM_NOT_GRANTED);
}
#endif

int sdlx_get_permission(char *name)
{
#ifndef ANDROID
    // when not running on Android return success
    return 0;
#else
    bool succ;
    int perm_result;

    INFO("get_permission %s\n", name);

    // request permission
    perm_result = PERM_NO_RESULT;
    succ = SDL_RequestAndroidPermission(name, get_permission_cb, &perm_result);
    if (!succ) {
        ERROR("SDL_RequestAndroidPermission failed, %s\n", SDL_GetError());
        return -1;
    }

    // wait for permission request to be either granted or not-granted
    while (perm_result == PERM_NO_RESULT) {
        usleep(TEN_MS);
    }

    // if not granted then return error
    if (perm_result != PERM_GRANTED) {
        ERROR("%s not granted\n", name);
        return -1;
    }

    // return success
    return 0;
#endif
}

// - - - - - - - - - sdlx_create_detached_thread_private  - - - - - - - 

// from SDL doc ...
//
// If you want to use threads in your SDL app, it's strongly recommended that you
// do so by creating them using SDL functions. This way, the required attach/detach
// handling is managed by SDL automagically. If you have threads created by other
// means and they make calls to SDL functions, make sure that you call
// Android_JNI_SetupThread() before doing anything else otherwise SDL will attach
// your thread automatically anyway (when you make an SDL call), but it'll never
// detach it.

int sdlx_create_detached_thread_private(int (*thread_fn)(void*), char *thread_name, void *cx)
{
    SDL_Thread *x;

    x = SDL_CreateThread(thread_fn, thread_name, cx);
    if (x == NULL) {
        return -1;
    }

    SDL_DetachThread(x);
    return 0;
}

