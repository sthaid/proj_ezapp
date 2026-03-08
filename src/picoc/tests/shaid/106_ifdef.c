#include <stdio.h>

#define IS_DEF

int main()
{
#if 0
    printf("ERROR shouldnt print 1\n");
#else
    printf("should print 1\n");
#endif

#if 1
    printf("should print 2\n");
#endif

#ifdef NOT_DEF
    printf("ERROR shouldnt print 3\n");
#else
    printf("should print 3\n");
#endif

#ifdef IS_DEF
    printf("should print 4\n");
#else
    printf("ERROR shouldnt print 4\n");
#endif

#ifdef IS_DEF
    #if 1
    printf("should print 5\n");
    #endif
#endif

    return 0;
}
