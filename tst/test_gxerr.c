#include "./test_gxerr_help.h"
#include <errno.h>


gx_error_initialize(GX_DEBUG)  // TODO: remove after old gx_error.h migrated out of gx.h

void *test_chain1();
int test_chain2();
int test_chain3();

int test_esys(int desired_errno) {
    errno = desired_errno;
    return -1;
}

void *test_chain1() {
    if_esys(test_chain2()) _eraise(NULL);
    return &test_chain1;
}

int test_chain2() {
    if_enz(test_chain3()) _eraise(-1);
    return 0;
}

int test_chain5() {
    errno = EACCES;
    if_esys(test_esys(EFAULT)) _eraise(0);
    return 0;
}

int test_chain4() {
    errno = EACCES;
    if_ez(test_chain5()) _eraise(0);
    return 0;
}

int test_chain3() {
    if_ez(test_chain4()) _eraise(EBADF);
    return 0;
}

int main(int argc, char **argv) {
    printf("Originally:  %d\n", _gx_error_cidx);
    _gx_error_cidx = 10;
    printf("Self set:    %d\n", _gx_error_cidx);
    set_loglevel(15);
    printf("Call set:    %d\n", _gx_error_cidx);
    _gx_error_cidx = 0;
    set_loglevel(0);

    if_esys(0)  return __LINE__;
    if_esys(5)  return __LINE__;
    if_esys(-5) return __LINE__;
    if_esys(test_esys(EBADF)) printf("Error occurred correctly\n");
    if_esys(test_esys(EAGAIN)) gx_error_dump_all();

    if_enull(test_chain1()) gx_error_dump_all();
    if_esys(test_esys(ENOMEM)) gx_error_dump_all();
    _eclear();
    if_esys(test_esys(ENOMEM)) gx_error_dump_all();
    return 0;
}





