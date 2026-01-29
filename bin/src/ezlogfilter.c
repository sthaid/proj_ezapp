#include <stdio.h>
#include <string.h>
#include <signal.h>

#define RED      "\033[31m"
#define GREEN    "\033[32m"
#define RESET    "\033[0m"

int main(int argc, char **argv)
{
    char s[1000];
    char *color;
    struct sigaction act;

    setlinebuf(stdout);

    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    sigaction(SIGINT, &act, NULL);

    while (fgets(s, sizeof(s), stdin) != NULL) {
        if (strncmp(s, "I ", 2) == 0) { 
            color = GREEN;
        } else if ((strncmp(s, "E ", 2) == 0) ||
                   (strcasestr(s, "error")) ||
                   (strcasestr(s, "fail")))
        {
            color = RED;
        } else {
            color = "";
        }

        if (color[0] == '\0') {
            printf("%s", s);
        } else {
            printf("%s%s%s", color, s, RESET);
        }
    }

    fflush(stdout);

    return 0;
}

