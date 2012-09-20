#include "./.kv_keys.h"
#include "./.test_enums.h"
#include "../gx_error2.h"
#include <stdio.h>

GX_LOG_SET_FKEYSTR(&$kv_log_keys);

//gx_log_keystring_callback = &$kv_log_keys;
//gx_log_keystring_callback = NULL;

void set_loglevel(int);
