#include <std_hdrs.h>
#include <logging.h>
#include <run.h>

#include <picoc_ezapp.h>

int run(char *name, bool is_svc)
{
    char           dir_path[100];
    int            rc;
    DIR           *dir;
    struct dirent *dirent;
    char          *p;
    char           picoc_args[1000];

    // xxx comment
    if (!is_svc) {
        sprintf(dir_path, "apps/%s", name);
    } else {
        sprintf(dir_path, "svcs/%s", name);
    }

    // construct list of *.c files in the dir
    picoc_args[0] = '\0';
    dir = opendir(dir_path);
    if (dir == NULL) {
        ERROR("%s: failed to opendir %s, %s\n", name, dir_path, strerror(errno));
        return 99;
    }
    p = picoc_args;
    while ((dirent = readdir(dir)) != NULL) {
        char *fn = dirent->d_name;
        int len = strlen(fn);
        if (len > 2 && strcmp(fn+len-2, ".c") == 0) {
            p += sprintf(p, "%s/%s ", dir_path, fn);
        }
    }
    closedir(dir);

    // error if no source code found in dir_path
    if (picoc_args[0] == '\0') {
        ERROR("%s: no source code in %s\n", name, dir_path);
        return 99;
    }

    // xxx comment
    p += sprintf(p, " - %s %s", name, dir_path);

    // run the app using the picoc c language interpreter
    INFO("%s: starting, args = %s\n", name, picoc_args);
    rc = picoc_ezapp(picoc_args);
    INFO("%s: completed, rc = %d\n", name, rc);

    // return completion status
    return rc;
}
