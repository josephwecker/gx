#include <assert.h>
#include "../gx.h"
#include "../gx_pool.h"

typedef struct X {
    GX_POOL_OBJECT(struct X);
    int a;
    int b;
} X;

int N = 0;

int X_allocate(X *x)
{
    assert(x != NULL);

    x->a = 10;
    x->b = 0;
    return 0;
}

int X_deallocate(X *x)
{
    assert(x != NULL);
    assert(x->a == 10);
    assert(x->b == 0);

    return 0;
}

int X_construct(X *x)
{
    assert(x != NULL);
    assert(x->a == 10);

    x->b = N++;
    return 0;
}

int X_destroy(X *x)
{
    assert(x != NULL);
    assert(x->a == 10);

    x->b = 0;
    return 0;
}

// Generates the code for the memory pool of X objects.
gx_pool_init_simple(X, X_allocate, X_deallocate, X_construct, X_destroy)

X_pool *pool = NULL;

int main(int argc, char **argv)
{
    pool = new_X_pool(10);

    X *x1 = acquire_X(pool);
    X *x2 = acquire_X(pool);
    X *x3 = NULL;

    assert(x1 != x2);
    assert(x1->a == 10);
    assert(x1->b == 0);
    assert(x2->a == 10);
    assert(x2->b == 1);

    release_X(pool, x1);

    x3 = acquire_X(pool);

    assert(x3 != x2);
    assert(x3->a == 10);
    assert(x3->b == 2);

    release_X(pool, x2);
    release_X(pool, x3);
    return 0;
}
