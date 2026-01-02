#include <std_hdrs.h>

#include <utils.h>
#include <logging.h>

#include "../cJSON/cJSON.h"
//#include "../lodepng/lodepng.h"

#define PAGE_SIZE2 (getpagesize())

// ----------------- TIME --------------------

long util_microsec_timer(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC,&ts);
    return  ((long)ts.tv_sec * 1000000) + ((long)ts.tv_nsec / 1000);
}

long util_get_real_time_microsec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME,&ts);
    return ((long)ts.tv_sec * 1000000) + ((long)ts.tv_nsec / 1000);
}

char *util_time2str(char * str, long us, int gmt, int display_ms, int display_date)
{
    struct tm tm;
    time_t secs;
    int cnt;
    char * s = str;

    secs = us / 1000000;

    if (gmt) {
        gmtime_r(&secs, &tm);
    } else {
        localtime_r(&secs, &tm);
    }

    if (display_date) {
        cnt = sprintf(s, "%02d/%02d/%02d ",
                         tm.tm_mon+1, tm.tm_mday, tm.tm_year%100);
        s += cnt;
    }

    cnt = sprintf(s, "%02d:%02d:%02d",
                     tm.tm_hour, tm.tm_min, tm.tm_sec);
    s += cnt;

    if (display_ms) {
        cnt = sprintf(s, ".%03d", (int)((us % 1000000) / 1000));
        s += cnt;
    }

    if (gmt) {
        strcpy(s, " GMT");
    }

    return str;
}

// -----------------  FILE READ/WRITE  -----------------------

static char *concat(char *s1, char *s2, char *result)
{
    if (s1 && s2) {
        sprintf(result, "%s/%s", s1, s2); 
    } else if (s1) {
        strcpy(result, s1);
    } else if (s2) {
        strcpy(result, s2);
    } else {
        ERROR("both s1 and s2 are null\n");
        result[0] = '\0';
    }

    return result;
}

// xxx maybe separating dir is too confusing
// xxx maybe a concat util instead
int util_write_file(char *dir, char *fn, void *buf, int len)
{
    int fd, ret;
    char path[200];

    concat(dir, fn, path);
    INFO("writing file %s\n", path);

    fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (fd < 0) {
        return -1;
    }

    ret = write(fd, buf, len);
    if (ret != len) {
        return -1;
    }

    close(fd);
    return 0;
}

// note: an extra '\0' byte is added to the end of the data buffer;
//       this extra char is not included in len_ret
void *util_read_file(char *dir, char *fn, int *len_ret)
{
    int fd, ret;
    struct stat statbuf;
    char *buf;
    char path[200];

    concat(dir, fn, path);
    INFO("reading file %s\n", path);

    *len_ret = 0;

    ret = stat(path, &statbuf);
    if (ret < 0) {
        return NULL;
    }

    buf = malloc(statbuf.st_size+1);
    if (buf == NULL) {
        return NULL;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        free(buf);
        return NULL;
    }

    ret = read(fd, buf, statbuf.st_size);
    if (ret != statbuf.st_size) {
        free(buf);
        return NULL;
    }

    buf[statbuf.st_size] = '\0';

    close(fd);

    *len_ret = statbuf.st_size;
    return buf;
}

void util_delete_file(char *dir, char *fn)
{
    char path[200];

    concat(dir, fn, path);
    INFO("deleting file %s\n", path);

    unlink(path);
}

bool util_file_exists(char *dir, char *fn)
{
    char path[200];
    struct stat statbuf;
    int rc;

    concat(dir, fn, path);

    rc = stat(path, &statbuf);
    return rc == 0;
}

// return unix epoch time in microsecs
long util_file_mtime(char *dir, char *fn)
{
    char path[200];
    struct stat statbuf;
    int rc;

    concat(dir, fn, path);

    rc = stat(path, &statbuf);
    if (rc != 0) {
        return 0;
    }

    return statbuf.st_mtim.tv_sec * 1000000L + statbuf.st_mtim.tv_nsec / 1000L;
}

long util_file_size(char *dir, char *fn)
{
    char path[200];
    struct stat statbuf;
    int rc;

    concat(dir, fn, path);

    rc = stat(path, &statbuf);
    return (rc == 0 ? statbuf.st_size : 0);
}

// -----------------  DIRECTORY UTILS  -----------------------

void util_delete_dir(char *dir, char *dir_to_delete)
{
    char path[200], cmd[220];

    concat(dir, dir_to_delete, path);
    INFO("deleting dir %s\n", path);

    sprintf(cmd, "rm -rf %s", path);
    system(cmd);
}

// -----------------  FILE MAP -------------------------------

void *util_map_file(char *dir, char *file, int len_arg, bool create_if_needed, 
                    bool read_only, int *created_flag)
{
    char   path[100];
    int    fd, rc, file_len, len;
    bool   file_exists;
    void  *addr = NULL;
    struct stat statbuf;
    char   *zero;

    concat(dir, file, path);
    INFO("mapping file %s\n", path);

    // do not allow create_if_needed and read_only both true
    if (create_if_needed && read_only) {
        ERROR("create_if_needed and read_only can not both be set\n");
        goto done;  
    }

    // round len up to multiple of pagesize
    len = (len_arg + PAGE_SIZE2 - 1) & ~(PAGE_SIZE2-1);

    // preset 'created_flag' return to false
    if (created_flag != NULL) {
        *created_flag = false;
    }

    // print message
    INFO("mapping %s len_arg=0x%x adjusted_len=0x%x create_if_needed=%d\n", path, len_arg, len, create_if_needed);

    // stat the file to determine if it exists, and its length
    rc = stat(path, &statbuf);
    file_exists = (rc == 0) && S_ISREG(statbuf.st_mode);
    file_len    = file_exists ? statbuf.st_size : 0;
    INFO("file exists=%d len=%d\n", file_exists, file_len);

    // if the file doesnt exist, or has the wrong length then
    //   if create flag is set
    //     unlink the file
    //     create the file with the proper length
    //     set 'created_flag'
    //   else
    //     return error
    //   endif
    // endif
    if (!file_exists || file_len != len) {
        if (create_if_needed) {
            INFO("creating %s len=%d\n", path, len);
            unlink(path);
            fd = open(path, O_CREAT|O_EXCL|O_RDWR, 0666);
            if (fd < 0) {
                ERROR("failed to create %s, %s\n", path, strerror(errno));
                goto done;  
            }
            zero = calloc(len, 1);
            rc = write(fd, zero, len);
            free(zero);
            if (rc != len) {
                ERROR("write zero to %s, len=%d, failed, %s\n", path, len, strerror(errno));
                close(fd);
                goto done;  
            }
            close(fd);
            if (created_flag != NULL) {
                *created_flag = true;
            }
        } else {
            ERROR("file doesnt exist or has wrong len, file_exists=%d file_len=%d len=%d\n", 
                   file_exists, file_len, len);
            goto done;  
        }
    }

    // map the file
    INFO("mapping %s len=%d\n", path, len);
    fd = open(path, read_only ? O_RDONLY : O_RDWR);
    if (fd < 0) {
        ERROR("failed to open %s, %s\n", path, strerror(errno));
        goto done;  
    }
    addr = mmap(NULL, len, 
                read_only ? PROT_READ : PROT_READ|PROT_WRITE, 
                MAP_SHARED, fd, 0);
    if (addr == NULL) {
        ERROR("mmap %s failed, %s\n", path, strerror(errno));
        close(fd);
        goto done;  
    }
    INFO("mmap returned addr %p\n", addr);
    close(fd);

done:
    // return mapped addr
    return addr;
}

void util_unmap_file(void *addr, int len_arg)
{
    int len, rc;

    // round len up to multiple of pagesize
    len = (len_arg + PAGE_SIZE2 - 1) & ~(PAGE_SIZE2-1);

    // print starting msg
    INFO("unmapping addr %p, len_arg=%x adjusted_len=%x\n", addr, len_arg, len);

    // sanity check addr arg
    if (addr == NULL) {
        ERROR("addr is NULL\n");
        return;
    }

    // unmap
    rc = munmap(addr, len);
    if (rc != 0) {
        ERROR("munmap failed, addr=%p len=0x%x, %s\n", addr, len, strerror(errno));
    }
}

void util_sync_file(void *addr, int len)
{
    int           rc;
    unsigned long first_page    = (unsigned long)addr & ~(PAGE_SIZE2-1);
    unsigned long last_page     = ((unsigned long)addr + len - 1) & ~(PAGE_SIZE2-1);
    void         *adjusted_addr = (void*)first_page;
    int           adjusted_len  = last_page - first_page + PAGE_SIZE2;

    // INFO("addr=%p len=0x%x 0x%x - adjusted addr=%p len=%d\n", 
    //      addr, len, adjusted_addr, adjusted_len);

    rc = msync(adjusted_addr, adjusted_len, MS_SYNC);
    if (rc != 0) {
        ERROR("msync %p %d failed, %s\n", adjusted_addr, adjusted_len, strerror(errno));
    }
}

// -----------------  GET / SET PARAMS  ----------------------

#define MAX_PARAMS 32

// xxx keep multiple copies for each data_dir
static struct {
    char *name;
    char *value;
} params[MAX_PARAMS];
static int max_params;
static char params_dir[100];

static void remove_trailing_newline(char *s)
{
    int len = strlen(s);

    if (len > 0 && s[len-1] == '\n') {
        s[len-1] = '\0';
    }
}

static void read_params_file(char *dir)
{
    char s[200], name[100], params_path[100];
    int cnt, n;
    FILE *fp;

    if (strcmp(dir, params_dir) == 0) {
        return;
    }
    strcpy(params_dir, dir);

    INFO("reading params file in dir '%s'\n", dir);

    memset(params, 0, sizeof(params));
    max_params = 0;

    sprintf(params_path, "%s/params", dir);
    fp = fopen(params_path, "r");
    if (fp == NULL) {
        INFO("params file does not exist\n");
        return;
    }

    while (fgets(s, sizeof(s), fp) != NULL) {
        remove_trailing_newline(s);
        n = 0;
        cnt = sscanf(s, "%s = %n", name, &n);
        if (cnt != 1 || n == 0) {
            ERROR("read_params_file '%s'\n", s);
            fclose(fp);
            return;
        }

        // xxx is this a mem leak
        params[max_params].name = strdup(name);
        params[max_params].value = strdup(s+n);
        max_params++;
    }

    fclose(fp);

    INFO("max_params=%d\n", max_params);
    for (int i = 0; i < max_params; i++) {
        INFO("  %s = %s\n", params[i].name, params[i].value);
    }
}

static void write_params_file(char *dir)
{
    FILE *fp;
    char params_path[100];

    if (strcmp(dir, params_dir) != 0) {
        ERROR("write_params_file, dir=%s params_dir=%s\n", dir, params_dir);
        return;
    }

    INFO("writing params file in dir '%s'\n", dir);
    INFO("max_params=%d\n", max_params);
    for (int i = 0; i < max_params; i++) {
        INFO("  %s = %s\n", params[i].name, params[i].value);
    }

    sprintf(params_path, "%s/params", dir);
    fp = fopen(params_path, "w");
    if (fp == NULL) {
        ERROR("write_params_file, fopen failed, %s\n", strerror(errno));
        return;
    }

    for (int i = 0; i < max_params; i++) {
        fprintf(fp, "%-16s = %s\n", params[i].name, params[i].value);
    }

    fclose(fp);
}

char *util_get_str_param(char *dir, char *name, char *default_value)
{
    int i;

    // xxx locking needed

    // if haven't read the params file then do so
    read_params_file(dir);

    // search for matching name
    for (i = 0; i < max_params; i++) {
        if (strcmp(name, params[i].name) == 0) {
            break;
        }
    }

    // if found then 
    //   return value
    // else
    //   add param, set to default value, and write file
    // endif
    if (i < max_params) {
        return params[i].value;
    } else {
        if (max_params >= MAX_PARAMS) {
            ERROR("params tbl is full\n");
            return default_value;
        }
        params[max_params].name = strdup(name);
        params[max_params].value = strdup(default_value);
        max_params++;
        write_params_file(dir);
        return default_value;
    }
}

void util_set_str_param(char *dir, char *name, char *value)
{
    int i;

    // xxx locking needed

    // if haven't read the params file then do so
    read_params_file(dir);

    // search for matching name
    for (i = 0; i < max_params; i++) {
        if (strcmp(name, params[i].name) == 0) {
            break;
        }
    }

    // if found then
    //   if no change then return
    //   replace its value
    // else
    //   add param to the end
    // endif
    if (i < max_params) {
        if (strcmp(params[i].value, value) == 0) {
            return;
        }
        free(params[i].value);
        params[i].value = strdup(value);
    } else {
        if (max_params >= MAX_PARAMS) {
            ERROR("params tbl is full\n");
            return;
        }
        params[max_params].name = strdup(name);
        params[max_params].value = strdup(value);
        max_params++;
    }

    // write the params file
    write_params_file(dir);
}

double util_get_numeric_param(char *dir, char *name, double dflt_val)
{
    char   dflt_val_str[100];
    char  *value_str;
    double value;
    int    cnt;

    // create the default value string, and
    // call util_get_str_param to get the value_str
    sprintf(dflt_val_str, "%G", dflt_val);
    value_str = util_get_str_param(dir, name, dflt_val_str);

    // convert value_str, returned by util_get_str_param, to 'value'
    cnt = sscanf(value_str, "%lf", &value);

    // the conversion can fail if value_str is not a number,
    // if the conversion fails then call util_set_numeric_param, and 
    // return the default value
    if (cnt != 1) {
        util_set_numeric_param(dir, name, dflt_val);
        return dflt_val;
    }

    // return the param value
    return value;
}

void util_set_numeric_param(char *dir, char *name, double value)
{
    char value_str[100];

    // create value string, and
    // call util_set_str_param to set it
    sprintf(value_str, "%G", value);
    util_set_str_param(dir, name, value_str);
}    

void util_print_params(char *dir)
{
    int i;

    // xxx locking

    read_params_file(dir);

    INFO("max_params=%d\n", max_params);
    for (i = 0; i < max_params; i++) {
        INFO("  %s = %s\n", params[i].name, params[i].value);
    }
}

// -----------------  NETWORK  -------------------------------

char *util_get_ipaddr(void)
{
    static char ipaddr[20];
    int rc, a=0, b=0, c=0, d=0;
    unsigned int addr;
    struct ifaddrs *ifap, *ifap_orig;;

    strcpy(ipaddr, "xxx.xxx.xxx.xxx");

    rc = getifaddrs(&ifap_orig);
    if (rc != 0) {
        ERROR("getifaddrs, %s\n", strerror(errno));
        return ipaddr;
    }

    ifap = ifap_orig;
    while (ifap) {
        //printf("ifa_name = %s\n", ifap->ifa_name);
        if (ifap->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *x = (struct sockaddr_in*)ifap->ifa_addr;

            addr = htonl(x->sin_addr.s_addr);

            if (((addr >> 24) & 0xff) == 127) {
                goto next;
            }

            a = (addr >> 24) & 0xff;
            b = (addr >> 16) & 0xff;
            c = (addr >>  8) & 0xff;
            d = (addr >>  0) & 0xff;

            if (a == 192 || a == 10) {
                sprintf(ipaddr, "%d.%d.%d.%d", a,b,c,d);
                break;
            }
        }
            
next:
        ifap = ifap->ifa_next;
    }

    freeifaddrs(ifap_orig);

    if (ipaddr[0] == 'x' && a != 0) {
        sprintf(ipaddr, "%d.%d.%d.%d", a,b,c,d);
    }

    return ipaddr;
}

// ----------------- JSON --------------------

void *util_json_parse(char *str, char **end_ptr)
{
    if (str == NULL || end_ptr == NULL) {
        return NULL;
    }

    return cJSON_ParseWithOpts(str, (const char **)end_ptr, 0);
}

void util_json_free(void *json_root)
{
    if (json_root == NULL) {
        return;
    }

    cJSON_Delete((cJSON*)json_root);
}

json_value_t *util_json_get_value(void *json_item, ...)
{
    cJSON              *item = (cJSON*)json_item;
    va_list             ap;
    char               *arg;
    int                 cnt, array_idx;
    static json_value_t value;

    memset(&value, 0, sizeof(value));

    if (json_item == NULL) {
        value.type = JSON_TYPE_UNDEFINED;
        return &value;
    }

    va_start(ap, json_item);

    while ((arg = va_arg(ap, char*)) != NULL) {
        cnt = sscanf(arg, "%d", &array_idx);
        if (cnt == 0) {
            item = cJSON_GetObjectItem(item, arg);
        } else {
            item = cJSON_GetArrayItem(item, array_idx);
        }
        if (item == NULL) {
            break;
        }
    }

    va_end(ap);

    if (item == NULL) {
        value.type = JSON_TYPE_UNDEFINED;
        return &value;
    }

    switch (item->type) {
    case cJSON_False:
    case cJSON_True:
        value.type = JSON_TYPE_FLAG;
        value.u.flag = (item->type == cJSON_True);
        break;
    case cJSON_Number:
        value.type = JSON_TYPE_NUMBER;
        value.u.number = item->valuedouble;
        break;
    case cJSON_String:
        value.type = JSON_TYPE_STRING;
        value.u.string = item->valuestring;
        break;
    case cJSON_Array:
        value.type = JSON_TYPE_ARRAY;
        value.u.array = item;
        break;
    case cJSON_Object:
        value.type = JSON_TYPE_OBJECT;
        value.u.object = item;
        break;
    default:
        value.type = JSON_TYPE_UNDEFINED;
        break;
    }

    return &value;
}

// ----------------- PNG  --------------------

#if 0
int util_read_png_file(char *dir, char *filename, unsigned char **pixels, int *w, int *h)
{
    char path[200];
    int rc;

    concat(dir, filename, path);
    INFO("reading png file %s\n", path);

    rc = lodepng_decode32_file(pixels, (unsigned int*)w, (unsigned int*)h, path); // xxx check this doent allocate pixels on err
    if (rc != 0) {
        ERROR("lodepng_decode32_file %s failed, rc=%d\n", path, rc);
        return -1;
    }

    return 0;
}

int util_write_png_file(char *dir, char *filename, unsigned char *pixels, int w, int h)
{
    char path[200];
    int rc;

    concat(dir, filename, path);
    INFO("writing png file %s\n", path);

    rc = lodepng_encode32_file(path, pixels, w, h);
    if (rc != 0) {
        ERROR("lodepng_encode32_file %s w=%d h=%d failed, rc=%d\n", path, w, h, rc);
        return -1;
    }

    return 0;
}
#endif
