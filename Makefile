gx_all : gxe/syserr.h | .silent


include gen/common.mk
-include $T/*.d

$(shell mkdir -p gxe)

B=./tst
T=./tst

gxe/syserr.h : gen/build-error-lookups
	$< > $@

KVKEYS=$B/.kv_keys.h
ENUM_MAPS=$B/.test_enums.h
test : $(KVKEYS) $(ENUM_MAPS) $B/test_gxerr
	$B/test_gxerr

$(ENUM_MAPS) : gen/build-enum-maps.rb gen/perfect_map.rb
	$< $T/*.h $T/.*.h > $@

$(KVKEYS) : gen/auto-kv-keys
	$< kv_log_keys $T/*.c $T/*.h > $@

$B/test_gxerr : $B/test_gxerr.o $B/test_gxerr_2.o
	$(call LINK)
$B/test_gxerr.o : $T/test_gxerr.c $B/.kv_keys.h $T/test_gxerr_help.h
	$(call gcc, -I.. -c)
$B/test_gxerr_2.o : $T/test_gxerr_2.c $(KVKEYS) $T/test_gxerr_help.h
	$(call gcc, -I.. -c)
