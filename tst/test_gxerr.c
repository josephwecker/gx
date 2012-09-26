#include "./test_gxerr_help.h"
#include <errno.h>


//gx_error_initialize(GX_DEBUG)  // TODO: remove after old gx_error.h migrated out of gx.h

void *test_chain1();
int test_chain2();
int test_chain3();

int test_esys(int desired_errno) {
    errno = desired_errno;
    return -1;
}

void *test_chain1() {
    if_esys(test_chain2()) _raise(NULL);
    return &test_chain1;
}

int test_chain2() {
    if_enz(test_chain3()) _raise(-1);
    return 0;
}

int test_chain5() {
    errno = EACCES;
    if_esys(test_esys(EFAULT)) _raise(0);
    return 0;
}

int test_chain4() {
    errno = EACCES;
    if_ez(test_chain5()) _raise(0);
    return 0;
}

int test_chain3() {
    if_ez(test_chain4()) _raise(EBADF);
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
    if_esys(test_esys(EAGAIN)) _emergency();
    if_enull(test_chain1()) gx_error_dump_all();
    if_esys(test_esys(ENOMEM)) gx_error_dump_all();
    _clear();
    if_esys(test_esys(ENOMEM)) gx_error_dump_all();

    // The following would all fail to build:
    //  _(-1) _error ("something",     "n", K_src_expression, "wowzers!", K_desired_filename, "george/man");
    //  _(-1) _error (0,               "n", K_src_expression, "wowzers!", K_desired_filename, "george/man");
    //  _(-1) _error (&_gx_error_cidx, "n", K_src_expression, "wowzers!", K_desired_filename, "george/man");
    //  _(-1) _error (K_err_brief,     "n", K_src_expression, "wowzers!", K_desired_filename); // no val

    _(test_esys(EAGAIN)) _emergency();
    _(test_esys(ENOTSUP)) _alert (K_err_brief,   "nothing", K_src_expression, "wowzers!", K_desired_filename, "george/man");
    _(test_esys(EUSERS)) _critical(K_err_brief,   $("first crazy stuff %d %d", 23, 0xDEADE));
    _(test_esys(EDEADLK)) _error(K_err_brief,   $("some crazy stuff %d %d",  23, 0xDEAD));
    //_(-1) E_UNKNOWN (K_err_brief,   "nothing", K_src_expression, "wowzers!", K_desired_filename, "george/man");
    _(test_esys(ENOMEM)) _warning (K_err_brief,   "nothing", K_src_expression, "wowzers!", K_desired_filename, "george/man");
    _(test_esys(ETIME)) _notice (K_err_another, "another", K_src_line,       "wow!");
    _(test_esys(ELOOP)) _stat (K_err_brief,   "nothing", K_src_expression, "wowzers!", K_desired_file,     "georgie");
    _(test_esys(EINVAL)) _info (K_err_brief,   "nothing", K_src_expression, "wowzers!", K_desired_file,     "georgie");
    _(test_esys(EAFNOSUPPORT)) _debug (K_err_another, "another", K_src_line,       "wow!");

    gx_sleep(2);

    if_enull(test_chain1()) _notice();

    return 0;
}





