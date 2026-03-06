#include <stdio.h>

// this declaration does not work;
// to workaround, either:
// - don't use 'static'
// - include the array size, change [] to [2]
//static char *x1[] = {"hello", "world"};  //xxx

// removing 'static' resolves the issue
char *x2[] = {"hello", "world"};

int main()
{
//  int n1 = sizeof(x1) / sizeof(x1[0]);
//  printf("n1 = %d\n", n1);
//  if (n1 == 2) printf("%s %s\n", x1[0], x1[1]);

    int n2 = sizeof(x2) / sizeof(x2[0]);
    printf("n2 = %d\n", n2);
    if (n2 == 2) printf("%s %s\n", x2[0], x2[1]);

    return 0;
}

