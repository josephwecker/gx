#include <gx/gx.h>
#include <gx/gx_mfd.h>

gx_error_initialize(GX_DEBUG);

static gx_mfd_pool  *mfd_pool;

int main(int argc, char **argv) {
    gx_mfd       *mfd;
    Xn( mfd_pool = new_gx_mfd_pool(1)                  ){X_ERROR; X_EXIT;}
    Xn( mfd = acquire_gx_mfd(mfd_pool)                 ){X_ERROR; X_EXIT;}
    X(  gx_mfd_open_writer(mfd, "tmp/writer-file.dat") ){X_ERROR; X_EXIT;}


}
