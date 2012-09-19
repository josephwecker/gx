gx_all : gxe/syserr.h | .silent


include gen/common.mk
-include $T/*.d

$(shell mkdir -p gxe)

B=./tst
T=./tst

gxe/syserr.h : gen/build-error-lookups
	$< > $@

KVKEYS=$B/.kv_keys.h
test : $(KVKEYS) $B/test_gxerr
	$B/test_gxerr

$(KVKEYS) : gen/auto-kv-keys
	$< kv_log_keys $T/*.c $T/*.h > $@

$B/test_gxerr : $B/test_gxerr.o $B/test_gxerr_2.o
	$(call LINK)
$B/test_gxerr.o : $T/test_gxerr.c $B/.kv_keys.h $T/test_gxerr_help.h
	$(call gcc, -I.. -c)
$B/test_gxerr_2.o : $T/test_gxerr_2.c $(KVKEYS) $T/test_gxerr_help.h
	$(call gcc, -I.. -c)
