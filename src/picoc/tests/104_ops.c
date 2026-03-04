#include <stdio.h>

void test_s32_s32(int x, int y)
{
    printf("---- s32 s32 ----\n");

    printf("%x %s %x = %x\n", x, "+", y, x + y);
    printf("%x %s %x = %x\n", x, "-", y, x - y);
    printf("%x %s %x = %x\n", x, "*", y, x * y);
    printf("%x %s %x = %x\n", x, "/", y, x / y);
    printf("%x %s %x = %x\n", x, "&", y, x & y);
    printf("%x %s %x = %x\n", x, "|", y, x | y);
    printf("%x %s %x = %x\n", x, "^", y, x ^ y);

    if ((unsigned)y < 32) {
        printf("%x %s %x = %x\n", x, "<<", y, x << y);
        printf("%x %s %x = %x\n", x, ">>", y, x >> y);
    }

    printf("%s%x = %x\n", "~", x, ~x);
}

void test_u32_u32(unsigned int x, unsigned int y)
{
    printf("---- u32 u32 ----\n");

    printf("%x %s %x = %x\n", x, "+", y, x + y);
    printf("%x %s %x = %x\n", x, "-", y, x - y);
    printf("%x %s %x = %x\n", x, "*", y, x * y);
    printf("%x %s %x = %x\n", x, "/", y, x / y);
    printf("%x %s %x = %x\n", x, "&", y, x & y);
    printf("%x %s %x = %x\n", x, "|", y, x | y);
    printf("%x %s %x = %x\n", x, "^", y, x ^ y);

    if ((unsigned)y < 32) {
        printf("%x %s %x = %x\n", x, "<<", y, x << y);
        printf("%x %s %x = %x\n", x, ">>", y, x >> y);
    }

    printf("%s%x = %x\n", "~", x, ~x);
}

void test_s64_s64(long x, long y)
{
    printf("---- s64 s64 ----\n");

    printf("%lx %s %lx = %lx\n", x, "+", y, x + y);
    printf("%lx %s %lx = %lx\n", x, "-", y, x - y);
    printf("%lx %s %lx = %lx\n", x, "*", y, x * y);
    printf("%lx %s %lx = %lx\n", x, "/", y, x / y);
    printf("%lx %s %lx = %lx\n", x, "&", y, x & y);
    printf("%lx %s %lx = %lx\n", x, "|", y, x | y);
    printf("%lx %s %lx = %lx\n", x, "^", y, x ^ y);

    if ((unsigned)y < 64) {
        printf("%lx %s %lx = %lx\n", x, "<<", y, x << y);
        printf("%lx %s %lx = %lx\n", x, ">>", y, x >> y);
    }

    printf("%s%lx = %lx\n", "~", x, ~x);
}

void test_u64_u64(unsigned long x, unsigned long y)
{
    printf("---- u64 u64 ----\n");

    printf("%lx %s %lx = %lx\n", x, "+", y, x + y);
    printf("%lx %s %lx = %lx\n", x, "-", y, x - y);
    printf("%lx %s %lx = %lx\n", x, "*", y, x * y);
    printf("%lx %s %lx = %lx\n", x, "/", y, x / y);
    printf("%lx %s %lx = %lx\n", x, "&", y, x & y);
    printf("%lx %s %lx = %lx\n", x, "|", y, x | y);
    printf("%lx %s %lx = %lx\n", x, "^", y, x ^ y);

    if ((unsigned)y < 64) {
        printf("%lx %s %lx = %lx\n", x, "<<", y, x << y);
        printf("%lx %s %lx = %lx\n", x, ">>", y, x >> y);
    }

    printf("%s%lx = %lx\n", "~", x, ~x);
}

int main()
{
    test_s32_s32(1, 1);
    test_s32_s32(1, -1);
    test_s32_s32(-1, 1);
    test_s32_s32(-1, -1);
    test_s32_s32(0x7fffffff, 1);
    test_s32_s32(1, 0x7fffffff);
    test_s32_s32(0xffffffff, 1);
    test_s32_s32(1, 0xffffffff);

    test_u32_u32(1, 1);
    test_u32_u32(1, -1);
    test_u32_u32(-1, 1);
    test_u32_u32(-1, -1);
    test_u32_u32(0x7fffffff, 1);
    test_u32_u32(1, 0x7fffffff);
    test_u32_u32(0xffffffff, 1);
    test_u32_u32(1, 0xffffffff);

    test_s64_s64(1, 1);
    test_s64_s64(1, -1);
    test_s64_s64(-1, 1);
    test_s64_s64(-1, -1);
    test_s64_s64(0x7fffffffffffffff, 1);
    test_s64_s64(1, 0x7fffffffffffffff);
    test_s64_s64(0xffffffffffffffff, 1);
    test_s64_s64(1, 0xffffffffffffffff);

    test_u64_u64(1, 1);
    test_u64_u64(1, -1);
    test_u64_u64(-1, 1);
    test_u64_u64(-1, -1);
    test_u64_u64(0x7fffffffffffffff, 1);
    test_u64_u64(1, 0x7fffffffffffffff);
    test_u64_u64(0xffffffffffffffff, 1);
    test_u64_u64(1, 0xffffffffffffffff);

    return 0;
}
