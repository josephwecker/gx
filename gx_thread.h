#ifndef _GX_THREAD_H
#define _GX_THREAD_H

/// Very "lightweight" fork, calls given function in child. Falls back to
/// fork on systems that don't have anything lighter. Otherwise tries to
/// create a new process with everything shared with the parent except a
/// new, _small_ stack. The parent is not signaled when the child
/// finishes [at least on linux... don't know yet for other...]

#ifdef __LINUX__
#include <sched.h>
#include <sys/prctl.h>
#include <signal.h>

#define _GX_STKSZ 0xffff  // For anything much more than what gx_mfd is doing, use ~ 0x1ffff

typedef struct _gx_clone_stack {
    struct _gx_clone_stack *_next, *_prev;
    int (*child_fn)(void *);
    void *child_fn_arg;
    // Will be able to reduce the stack when we've solidified the clone's actions...
    char stack[_GX_STKSZ];
} __attribute__((aligned)) _gx_clone_stack;

gx_pool_init(_gx_clone_stack);
static _gx_clone_stack_pool *_gx_csp optional = NULL;

static void sigchld_clone_handler(int sig) {
    int status, saved_errno;
    pid_t child_pid;
    saved_errno = errno;
    while((child_pid = waitpid(-1, &status, __WCLONE | WNOHANG)) > 0) {
        finrelease__gx_clone_stack(_gx_csp, child_pid);
    }
    errno = saved_errno;  // For reentrancy
}

/// Wraps the clone target function so we can release the stack
static int _gx_clone_launch(void *arg) {
    _gx_clone_stack *csp = (_gx_clone_stack *)arg;
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    int res = csp->child_fn(csp->child_fn_arg);
    prerelease__gx_clone_stack(_gx_csp, csp);
    return res;
}

static inline int gx_clone(int (*fn)(void *), void *arg) {
    int flags = SIGCHLD | CLONE_FILES   | CLONE_FS      | CLONE_IO      | CLONE_PTRACE |
        CLONE_SYSVSEM | CLONE_VM;
    _gx_clone_stack *cstack;

    if(rare(!_gx_csp)) {
        // "Global" setup
        _N(_gx_csp = new__gx_clone_stack_pool(5)) _raise_alert(-1);
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = sigchld_clone_handler;
        _ (sigaction(SIGCHLD, &sa, NULL)) _raise_alert(-1);
    }
    _N(cstack = acquire__gx_clone_stack(_gx_csp)) _raise_alert(-1);
    cstack->child_fn = fn;
    cstack->child_fn_arg = arg;
    int cres;
    _(cres = clone(&_gx_clone_launch, (void *)&(cstack->stack[_GX_STKSZ - 1]), flags, (void *)cstack)) _raise_alert(-1);
    //_(cres = clone(&_gx_clone_launch, (void *)(cstack->stack) + sizeof(cstack->stack) - 1, flags, (void *)cstack)) _raise_alert(-1);
    return cres;
}
#else
typedef struct _gx_clone_stack {
    struct _gx_clone_stack  *_next, *_prev;
    pthread_t                tid;
    int                    (*child_fn)(void *);
    void                    *child_fn_arg;
    // NO EXPLICIT STACK FOR NOW IN GENERAL CASE
} __attribute__((aligned)) _gx_clone_stack;
gx_pool_init(_gx_clone_stack);
static _gx_clone_stack_pool *_gx_csp optional = NULL;

static void *_gx_clone_launch(void *arg) {
    // Yes, a rather complex way to essentially change a void * callback
    // into an int callback.
    _gx_clone_stack *cs                      = (_gx_clone_stack *)arg;
    int            (*local_child_fn)(void *) = cs->child_fn;
    void            *local_child_fn_arg      = cs->child_fn_arg;
    release__gx_clone_stack(_gx_csp, cs);

    //-------- Actual callback
    local_child_fn(local_child_fn_arg); // TODO: can't do much of anything with the return value...

    return NULL;
}

static inline int gx_clone(int (*fn)(void *), void *arg) {
    pthread_t tid;
    _gx_clone_stack *cstack; // Not really needed for the stack in this context, but for the callbacks
    if(rare(!_gx_csp))
        _N(_gx_csp = new__gx_clone_stack_pool(5)) _raise_alert(-1);

    _N(cstack = acquire__gx_clone_stack(_gx_csp)) _raise_alert(-1);
    cstack->child_fn = fn;
    cstack->child_fn_arg = arg;
    _E(pthread_create(&tid, NULL, _gx_clone_launch, (void *)cstack)) _raise_alert(-1);
    return 0;
}

#endif

#endif
