/* Test-only compatibility for the FreeBSD descriptor header on a host. */
#ifndef __aligned
#define __aligned(x) __attribute__((aligned(x)))
#endif
