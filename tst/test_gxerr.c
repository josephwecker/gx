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
    if_esys(test_chain2()) E_RAISE(NULL);
    return &test_chain1;
}

int test_chain2() {
    if_enz(test_chain3()) E_RAISE(-1);
    return 0;
}

int test_chain5() {
    errno = EACCES;
    if_esys(test_esys(EFAULT)) E_RAISE(0);
    return 0;
}

int test_chain4() {
    errno = EACCES;
    if_ez(test_chain5()) E_RAISE(0);
    return 0;
}

int test_chain3() {
    if_ez(test_chain4()) E_RAISE(EBADF);
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
    gx_log_update_sysinfo();
    gx_log_set_prog("gx-error/gx-log test", "VERSION who knows");
    //gx_log_set_version("---BEST VERSION 0.1!---");
    if_esys(0)  return __LINE__;
    if_esys(5)  return __LINE__;
    if_esys(-5) return __LINE__;
    if_esys(test_esys(EAGAIN)) E_EMERGENCY();
    if_enull(test_chain1()) gx_error_dump_all();
    if_esys(test_esys(ENOMEM)) gx_error_dump_all();
    E_CLEAR();
    if_esys(test_esys(ENOMEM)) gx_error_dump_all();

    // The following would all fail to build:
    //  _(-1) E_ERROR ("something",     "n", K_src_expression, "wowzers!", K_desired_filename, "george/man");
    //  _(-1) E_ERROR (0,               "n", K_src_expression, "wowzers!", K_desired_filename, "george/man");
    //  _(-1) E_ERROR (&_gx_error_cidx, "n", K_src_expression, "wowzers!", K_desired_filename, "george/man");
    //  _(-1) E_ERROR (K_err_brief,     "n", K_src_expression, "wowzers!", K_desired_filename); // no val

    _(-1) E_NOTICE(K_err_brief,   $("first crazy stuff %d %d", 23, 0xDEADE));
    _(-1) E_NOTICE(K_err_brief,   $("some crazy stuff %d %d",  23, 0xDEAD));
    _(-1) E_ERROR (K_err_brief,   "nothing", K_src_expression, "wowzers!", K_desired_filename, "george/man");
    _(-1) E_ERROR (K_err_another, "another", K_src_line,       "wow!");
    _(-1) E_ERROR (K_err_brief,   "nothing", K_src_expression, "wowzers!", K_desired_file,     "georgie");
    _(-1) E_ERROR (K_err_brief,   "nothing", K_src_expression, "wowzers!", K_desired_filename, "george/man");

    gx_sleep(2,0);

    if_enull(test_chain1()) E_NOTICE();

    return 0;
}





