#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <dirent.h>

#include <readline/readline.h>
#include <readline/history.h>

//
// defines
//

#define NOT_A_SPECIAL_CMD -9999

#define MAX_ALIAS 100

//
// typedefs
//

typedef struct {
    char *cmd;
    char *alias;
} alias_t;

//
// variables
//

// these are obtained from the ezsh.cfg file
char    ipaddr[200];
int     port;
char    password[200];
alias_t alias_tbl[MAX_ALIAS];
int     max_alias;

// current working directory
char    cwd[200];
char    cwd_initial[200];

// fp used to communicate to server process running on android
FILE   *sockfp;

//
// prototypes
//

// main
void help(void);
void read_ezsh_cfg(void);
void connect_to_android(void);
void substitue_alias(char *cmdline);
int run_cmd(char *cmdline);

// run special cmd
int run_special_cmd(char *cmdline);
int special_cmd_put(char *src, char *dest);
int special_cmd_get(char *src, char *dest);
int special_cmd_cd(char *path);
int special_cmd_pwd(void);
int special_cmd_alias(void);
int special_cmd_vi(char *android_path);
int special_cmd_local(char *cmdline);

// run cmd on android
int run_cmd_on_android(char *cmdline, char *short_cmdline,
                       char *data_out, int data_out_len,
                       char **data_in, int *data_in_len);

// utils
void sanitize(char *s);
void put_fmt(FILE *fp, char *fmt, ...);
char *get_str(FILE *fp, char *s, int s_len);
int read_file(char *fn, void **buf, int *buf_len);
int write_file(char *fn, void *buf, int len);

// -----------------  MAIN  -------------------------------------------------

int main(int argc, char **argv)
{
    char *cmdline;
    int   status;

    // if '-h' option then display help and exit
    if (argc == 2 && strcmp(argv[1], "-h") == 0) {
        help();
        return 0;
    }

    // read ezsh.cfg file, to get ipaddr, port, password, and cmd aliases
    read_ezsh_cfg();

    // connect to android: also validates password and gets curr-working-dir (cwd)
    connect_to_android();

    // if args provided then use them for cmdline, and end program
    if (argc >= 2) {
        char cmd[1000];
        char *p = cmd;
        for (int i = 1; i < argc; i++) {
            p += sprintf(p, "%s ", argv[i]);
        }
        substitue_alias(cmd);
        status = run_cmd(cmd);
        return status != 0 ? 1 : 0;
    }

    // runtime loop
    while (true) {
        // construct prompt
        char prompt[200], local_cwd[200], android_cwd[200];
        getcwd(local_cwd, sizeof(local_cwd));
        strcpy(android_cwd, cwd);
        snprintf(prompt, sizeof(prompt), "ezsh lcl:%s android:%s> ", basename(local_cwd), basename(android_cwd));

        // read cmdline
        // - if NULL then end program
        // - remove leading and trailing spaces and trailing newline
        // - if blank line then continue
        // - if 'q' or 'exit' then end program
        // - add cmdline to history
        cmdline = readline(prompt);
        if (cmdline == NULL) {
            break;
        }
        sanitize(cmdline);
        if (cmdline[0] == '\0') {
            continue;
        }
        if (strcmp(cmdline, "q") == 0 || strcmp(cmdline, "exit") == 0) {
            break;
        }
        add_history(cmdline);

        // substitue alias
        substitue_alias(cmdline);

        // process the cmdline
        run_cmd(cmdline);

        // free cmdline
        free(cmdline);
    }
}

void help(void)
{
    char help_text[] = "\
Ezsh runs on the Linux host, simulating a shell running on the Android device.\n\
\n\
The ezsh.cfg file, found in the same directory as the ezsh executable, provides the\n\
Android device IP address, ezapp developer mode TCP/IP port, and password.\n\
The ezsh.cfg may also contain optional cmd aliases.\n\
\n\
To use ezsh, the ezapp must have Settings Devel_Mode = ON. Also the Devel_Port and\n\
Devel_Password must be set in ezapp, matching the values supplied in ezsh.cfg.\n\
The password provides minimal security. It is recommended to enable ezapp Devel_Mode\n\
on a trusted network.\n\
\n\
Commands entered to ezsh are first checked if they require special processing;\n\
if not then the command is passed to ezapp, which will run the command on the \n\
Android device.\n\
\n\
Commands that require special processing are:\n\
- cd    : Ezsh maintains the Android current working directory (cwd) path.\n\
          When a command is executed on Android, the Android directory is\n\
          first set to the cwd.\n\
          Example: cd apps/Clock\n\
- pwd   : Prints the cwd\n\
- get   : Copy file from Android.\n\
          Example: get apps/Clock/clock.c\n\
- put   : Copy file to Android.\n\
          Example: put clock.c apps/Clock\n\
- vi    : Edit a file on Android. The file is fist copied to the host tmp dir,\n\
          edited there, and finally copied back to the Android.\n\
          Example: vi apps/Clock/clock.c\n\
- alias : Print the command aliases which are provided in the ezsh.cfg file.\n\
- local : Execute a command on the host.\n\
";

    printf("%s", help_text);
}

void read_ezsh_cfg(void)
{
    char  self_path[200], ezsh_cfg_path[200], *self_dir, s[200];
    FILE *fp;
    int   line_num=0;

    // get path to ezsh.cfg file
    readlink("/proc/self/exe", self_path, sizeof(self_path));
    self_dir = dirname(self_path);
    sprintf(ezsh_cfg_path, "%s/ezsh.cfg", self_dir);

    // open ezsh.cfg file
    fp = fopen(ezsh_cfg_path, "r");
    if (fp == NULL) {
        printf("ERROR: failed to open %s\n", ezsh_cfg_path);
        exit(1);
    }

    // read and process lines from ezsh.cfg file
    while (fgets(s, sizeof(s), fp) != NULL) {
        line_num++;

        // remove leading and trailing spaces and trailing newline
        sanitize(s);

        // skip blank lines and lines begining with '#'
        if (s[0] == '\0' || s[0] == '#') {
            continue;
        }

        // tokenize line
        char *id = strtok(s, " ");
        char *val = strtok(NULL, " ");
        char *rest = strtok(NULL, "");

        // must have at least id and val
        if (id == NULL || val == NULL) {
            continue;
        }

        // parse the line
        if (strcmp(id, "ipaddr") == 0) {
            strcpy(ipaddr, val);
        } else if (strcmp(id, "port") == 0) {
            sscanf(val, "%d", &port);
        } else if (strcmp(id, "password") == 0) {
            strcpy(password, val);
        } else if (strcmp(id, "alias") == 0) {
            if (max_alias == MAX_ALIAS) {
                printf("ERROR: alias tbl is full, ezsh.cfg line %d\n", line_num);
                continue;
            }
            if (rest == NULL) {
                printf("ERROR: invalid alias, ezsh.cfg line %d\n", line_num);
                continue;
            }
            sanitize(rest);
            alias_tbl[max_alias].cmd = strdup(val);
            alias_tbl[max_alias].alias = strdup(rest);
            max_alias++;
        } 
    }

    // close fp
    fclose(fp);

    // verify ipaddr, port and password are set
    if (ipaddr[0] == '\0') {
        printf("ERROR: ippadr needed in ezsh.cfg\n");
        exit(1);
    }
    if (port == 0) {
        printf("ERROR: port needed in ezsh.cfg\n");
        exit(1);
    }
    if (password[0] == '\0') {
        printf("ERROR: password needed in ezsh.cfg\n");
        exit(1);
    }
}

void connect_to_android(void)
{
    int                ret, len, sockfd;
    struct sockaddr_in addr;
    socklen_t          addrlen;
    char               response[200];

    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("ERROR: socket\n");
        exit(1);
    }

    // connect
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = inet_addr(ipaddr);
    addrlen = sizeof(addr);
    ret = connect(sockfd, (struct sockaddr*)&addr, addrlen);
    if (ret != 0) {
        printf("ERROR: connect %s:%d, %s\n", ipaddr, port, strerror(errno));
        exit(1);
    }

    // create fp for socket fd
    sockfp = fdopen(sockfd, "w+");

    // send password, and get response
    put_fmt(sockfp, "%s\n", password);
    get_str(sockfp, response, sizeof(response));
    if (strcmp(response, "password okay") != 0) {
        printf("ERROR: invalid password\n");
        exit(1);   
    }

    // get the ezApp current working dir
    get_str(sockfp, cwd, sizeof(cwd));

    // ensure cwd includes terminating '/'
    len = strlen(cwd);
    if (len == 0) {
        strcpy(cwd, "/");
    } else if (cwd[len-1] != '/') {
        strcat(cwd, "/");
    }

    // save initial cwd
    strcpy(cwd_initial, cwd);
}

void substitue_alias(char *cmdline)
{
    char *p, temp[1000];
    int   i;

    p = strchr(cmdline, ' ');
    if (p) *p = '\0';

    for (i = 0; i < max_alias; i++) {
        if (strcmp(cmdline, alias_tbl[i].cmd) == 0) {
            if (p) *p = ' ';
            strcpy(temp, cmdline+strlen(alias_tbl[i].cmd));
            strcpy(cmdline, alias_tbl[i].alias);
            strcat(cmdline, temp);
            return;
        }
    }

    if (p) *p = ' ';
}

int run_cmd(char *cmdline)
{
    int status;
    char cd_plus_cmdline[1000];

    status = run_special_cmd(cmdline);

    if (status == NOT_A_SPECIAL_CMD) {
        sprintf(cd_plus_cmdline, "cd %s; %s", cwd, cmdline);
        status = run_cmd_on_android(cd_plus_cmdline, cmdline, NULL, 0, NULL, 0);
    }

    return status;
}

// -----------------  RUN SPECIAL CMD  ----------------------------------

int run_special_cmd(char *cmdline)
{
    char cmd[200], arg1[200], arg2[200];

    // extract cmd, arg1, and arg2 from cmdline
    cmd[0] = arg1[0] = arg2[0] = '\0';
    sscanf(cmdline, "%s %s %s", cmd, arg1, arg2);
    if (cmd[0] == '\0') {
        printf("ERROR: run_special_cmd cmdline arg invalid\n");
        exit(1);   
    }

    // process special cmds
    if (strcmp(cmd, "cd") == 0) {
        return special_cmd_cd(arg1);
    } else if (strcmp(cmd, "pwd") == 0) {
        return special_cmd_pwd();
    } else if (strcmp(cmd, "alias") == 0) {
        return special_cmd_alias();
    } else if (strcmp(cmd, "put") == 0) {
        return special_cmd_put(arg1, arg2);
    } else if (strcmp(cmd, "get") == 0) {
        return special_cmd_get(arg1, arg2);
    } else if (strcmp(cmd, "vi") == 0) {
        return special_cmd_vi(arg1);
    } else if (strcmp(cmd, "local") == 0) {
        return special_cmd_local(cmdline);
    } else {
        // not a special cmd
        return NOT_A_SPECIAL_CMD;
    }

    // not reached
}

// copy file to android:
// - src: path to src file on develsys
// - dest: path to dest file or dest dir on android,
//   . will be prepended with cwd
//   . may be empty str
int special_cmd_put(char *src, char *dest)
{
    char  cmdline[1000];
    char  temp[200];
    char  dest_path[200];
    char *src_filename;
    void *data;
    int   data_len;
    int   status;

    // src is required
    if (src[0] == '\0') {
        printf("ERROR: src required\n");
        return -EINVAL;
    }

    // extract src_filename from src
    strcpy(temp, src);
    src_filename = basename(temp);

    // construct dest_path
    if (dest[0] == '/') {
        strcpy(dest_path, dest);
    } else {
        sprintf(dest_path, "%s%s", cwd, dest);
    }

    // read src file
    status = read_file(src, &data, &data_len);
    if (status != 0) {
        printf("ERROR: read_file %s, %s\n", src, strerror(errno));
        free(data);
        return status;
    }

    // run 'put <dest_path> <src_filename>' on android
    sprintf(cmdline, "put %s %s", dest_path, src_filename);
    status = run_cmd_on_android(cmdline, cmdline, data, data_len, NULL, 0);

    // free data
    free(data);

    // return status
    return status;
}

// copy file from android:
// - src: path to src file on android
// - dest: path to dest file or dest dir on develsys,
//   . if empty str then will be replaced with "."
int special_cmd_get(char *src, char *dest)
{
    char  src_path[200], dest_path[200], cmdline[1000];
    DIR  *dir;
    int   status, data_len;
    char *data;

    // src is required
    if (src[0] == '\0') {
        printf("ERROR: src required\n");
        return -EINVAL;
    }

    // construct src_path by prepending src with cwd
    if (src[0] == '/') {
        strcpy(src_path, src);
    } else {
        sprintf(src_path, "%s%s", cwd, src);
    }

    // construct dest_path
    if (dest[0] != '\0') {
        strcpy(dest_path, dest);
    } else {
        strcpy(dest_path, ".");
    }
    dir = opendir(dest_path);
    if (dir != NULL) {
        char *src_filename, temp[200];
        int   len;

        len = strlen(dest_path);
        if (len > 0 && dest_path[len-1] != '/') {
            strcat(dest_path, "/");
        }
        strcpy(temp, src_path);
        src_filename = basename(temp);
        strcat(dest_path, src_filename);
        closedir(dir);
    }

    // debug print
    //printf("src_path '%s'\n", src_path);
    //printf("dest_path '%s'\n", dest_path);

    // run 'get <src_path>' on android
    sprintf(cmdline, "get %s", src_path);
    status = run_cmd_on_android(cmdline, cmdline, NULL, 0, &data, &data_len);
    if (status != 0) {
        free(data);
        return status;
    }

    // write file to dest_path on develsys
    status = write_file(dest_path, data, data_len);
    if (status != 0) {
        printf("ERROR: failed to create '%s', %s\n", dest_path, strerror(-status));
        free(data);
        return status;
    }

    // free data, and return status
    free(data);
    return status;
}

// This routine updates cwd; and will always terminate cwd with '/'.
int special_cmd_cd(char *path)
{
    int   len;
    char  new_cwd[200];
    char *token;

    // sanity check that cwd begins and ends with '/'
    len = strlen(cwd);
    if (cwd[0] != '/' || cwd[len-1] != '/') {
        printf("ERROR: invalid cwd '%s'\n", cwd);
        exit(1);
    }

    // use new_cwd as work area, until it is validated
    strcpy(new_cwd, cwd);

    // update new_cwd, using the supplied path
    if (path[0] == '\0') {
        strcpy(new_cwd, cwd_initial);
    } else if (path[0] == '/') {
        strcpy(new_cwd, path);
    } else {
        // ensure path terminates with '/'
        len = strlen(path);
        if (path[len-1] != '/') {
            strcat(path, "/");
        }

        // tokenize path using '/' separator;
        // and update new_cwd using the value of each token
        while ((token = strtok(path, "/"))) {
            path = NULL;
            if (strcmp(token, ".") == 0) {
                // do nothing
            } else if (strcmp(token, "..") == 0) {
                if (strcmp(new_cwd, "/") != 0) {
                    int idx = strlen(new_cwd) - 2;
                    while (new_cwd[idx] != '/') idx--;
                    new_cwd[idx+1] = '\0';
                }
            } else {
                strcat(new_cwd, token);
                strcat(new_cwd, "/");
            }
        }
    }

    // ensure new_cwd terminates with '/'
    len = strlen(new_cwd);
    if (len > 0 && new_cwd[len-1] != '/') {
        strcat(new_cwd, "/");
    }

    // run_cmd_on_android to verify new_cwd is an android directory;
    // if so then the new_cwd will be used
    char cmd[300];
    int  status;
    sprintf(cmd, "if [ ! -d %s ]; then exit 1; else exit 0; fi", new_cwd);
    status = run_cmd_on_android(cmd, cmd, NULL, 0, NULL, 0);
    if (status == 0) {
        strcpy(cwd, new_cwd);
    }

    // return status
    return status;
}

int special_cmd_pwd(void)
{
    printf("%s\n", cwd);
    return 0;
}

int special_cmd_alias(void)
{
    int i;

    for (i = 0; i < max_alias; i++) {
        printf("%-16s %s\n", alias_tbl[i].cmd, alias_tbl[i].alias);
    }
    return 0;
}

int special_cmd_vi(char *android_path)
{
    char temp[200], tmp_path[200], vi_cmd[1000];
    int status;

    // android_path is required
    if (android_path[0] == '\0') {
        printf("ERROR: android_path required\n");
        return -EINVAL;
    }

    // construct /tmp path
    strcpy(temp, android_path);
    sprintf(tmp_path, "/tmp/%s", basename(temp));

    // copy android file to tmp
    status = special_cmd_get(android_path, tmp_path);
    if (status != 0) {
        if (status == -ENOENT) {
            unlink(tmp_path);
        } else {
            return status;
        }
    }

    // edit tmp_path file
    sprintf(vi_cmd, "vi %s", tmp_path);
    system(vi_cmd);

    // copy tmp file back to android
    status = special_cmd_put(tmp_path, android_path);
    if (status != 0) {
        return status;
    }

    // success
    return 0;
}

int special_cmd_local(char *cmdline)
{
    char *p, *local_cmd, *home;
    int   status = 0;
    char  dir[200];

    p = strchr(cmdline, ' ');
    if (p == NULL) {
        printf("ezsh: executing bash ...\n");
        system("bash");
        return 0;
    } 
    local_cmd = p+1;

    if (strcmp(local_cmd, "cd") == 0) {
        home = getenv("HOME");
        if (home == NULL) {
            home = "/";
        }
        status = chdir(home);
        if (status != 0) {
            status = -errno;
        }
    } else if (sscanf(local_cmd, "cd %s", dir) == 1) {
        status = chdir(dir);
        if (status != 0) {
            status = -errno;
            printf("ERROR: cd %s, %s\n", dir, strerror(-status));
        }
    } else {
        status = system(local_cmd);
        status = WEXITSTATUS(status);
    }

    return status;
}

// -----------------  RUN CMD ON ANDROID  -----------------------------------

int run_cmd_on_android(char *cmdline, char *short_cmdline,
                       char *data_out, int data_out_len,
                       char **data_in, int *data_in_len)
{
    char  s[200];
    char *p;
    int   rc;
    int   status = 99;

    // send 'run' and cmdline to android
    put_fmt(sockfp, "run\n");
    put_fmt(sockfp, "%s\n", cmdline);

    // if data_out is provided then send data buffer to android 
    if (data_out != NULL) {
        put_fmt(sockfp, "data_len %d\n", data_out_len);

        rc = fwrite(data_out, 1, data_out_len, sockfp);
        if (rc != data_out_len) {
            printf("ERROR: fwrite failed, %s\n", strerror(errno));
            exit(1);
        }

        get_str(sockfp, s, sizeof(s));
        if (strncmp(s, "CMD_COMPLETE ", 13) != 0) {
            printf("ERROR: did not recv CMD_COMPLETE\n");
            exit(1);
        }
        sscanf(s+13, "%d", &status);

    // if data_in is provided then recv data buffer from adnroid
    } else if (data_in != NULL) {
        char *buf;
        int   buf_len;

        // get buf_len from android
        get_str(sockfp, s, sizeof(s));
        if (sscanf(s, "data_len %d", &buf_len) != 1) {
            printf("ERROR: did not recv data_len\n");
            exit(1);
        }

        // allocate buf
        buf = calloc(buf_len, 1);

        // read data from android
        rc = fread(buf, 1, buf_len, sockfp);
        if (rc != buf_len) {
            printf("ERROR: fread failed, %s\n", strerror(errno));
            free(buf);
            exit(1);
        }

        // read cmd completion string from android
        get_str(sockfp, s, sizeof(s));
        if (strncmp(s, "CMD_COMPLETE ", 13) != 0) {
            printf("ERROR: did not recv CMD_COMPLETE\n");
            free(buf);
            exit(1);
        }

        // return data to caller; caller must free data
        *data_in = buf;
        *data_in_len = buf_len;

        // parse the status returned from android
        sscanf(s+13, "%d", &status);

    // otherwise android will have run this cmd using popen;
    // read and print the provided output from popen
    } else {
        while (true) {
            // get string from android, and print
            get_str(sockfp, s, sizeof(s));

            // check for cmd complete, and parse status
            if ((p = strstr(s, "CMD_COMPLETE "))) {
                sscanf(p+13, "%d", &status);
                *p = '\0';
                if (strlen(s) > 0) {
                    printf("%s\n", s);
                }
                break;
            }

            printf("%s\n", s);
        }
    }

    // print status
    // - status == 0 : success
    // - status > 0  : is an exitcode from the cmdline executed on android
    //                 - 127 : cmd not found
    // - status < 0  : is an errno
    if (status == 127) {
        printf("ezsh: Command '%s' not found.\n", short_cmdline);
    } else if (status > 0) {
        printf("ERROR: exit_status %d\n", status);
    } else if (status < 0) {
        printf("ERROR: %s\n", strerror(-status));
    }

    // return status
    return status;
}

// -----------------  UTILS  ----------------------------------------

void sanitize(char *s)
{
    char *p = s;
    int   len;

    // return if s is NULL
    if (s == NULL) {
        return;
    }

    // remove leading spaces
    while (*p == ' ') {
        p++;
    }
    len = strlen(p);
    memmove(s, p, len+1);

    // if length of s is 0 then return
    if (len == 0) {
        return;
    }

    // remove trailing spaces and newline chars
    p = &s[len-1];
    while (p >= s && (*p == ' ' || *p == '\n')) {
        *p = '\0';
        p--;
    }
}

void put_fmt(FILE *fp, char *fmt, ...)
{
    va_list ap;
    int rc;

    va_start(ap, fmt);

    rc = vfprintf(fp, fmt, ap);
    if (rc < 0) {
        printf("ERROR: lost connection to android, ezsh terminating\n");
        exit(1);
    }

    rc = fflush(fp);
    if (rc == EOF) {
        printf("ERROR: lost connection to android, ezsh terminating\n");
        exit(1);
    }

    va_end(ap);
}

char *get_str(FILE *fp, char *s, int s_len)
{
    char *p;
    int len;

    s[0] = '\0';

    p = fgets(s, s_len, fp);
    if (p == NULL) {
        printf("ERROR: lost connection to android, ezsh terminating\n");
        exit(1);
    }

    len = strlen(s);
    if (len > 0 && s[len-1] == '\n') {
        s[len-1] = '\0';
    }

    return s;
}

int read_file(char *fn, void **buf_arg, int *buf_len_arg)
{
    int fd, ret;
    struct stat statbuf;
    char *buf;

    *buf_len_arg = 0;
    *buf_arg = NULL;

    ret = stat(fn, &statbuf);
    if (ret < 0) {
        return errno ? -errno : -EINVAL;
    }
    
    fd = open(fn, O_RDONLY);
    if (fd < 0) {
        return errno ? -errno : -EINVAL;
    }

    buf = malloc(statbuf.st_size);
    if (buf == NULL) {
        close(fd);
        return errno ? -errno : -EINVAL;
    }
    
    ret = read(fd, buf, statbuf.st_size);
    if (ret != statbuf.st_size) {
        free(buf);
        close(fd);
        return errno ? -errno : -EINVAL;
    }

    close(fd);

    *buf_arg = buf;
    *buf_len_arg = statbuf.st_size;
    return 0;  
}

int write_file(char *fn, void *buf, int len)
{
    int fd, ret;

    fd = open(fn, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (fd < 0) {
        return errno ? -errno : -EINVAL;
    }

    ret = write(fd, buf, len);
    if (ret != len) {
        return errno ? -errno : -EINVAL;
    }

    close(fd);
    return 0;
}

