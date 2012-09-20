gx_all : gxe/syserr.h gxe/gx_enum_lookups.h gxe/gx_log_table.h | .silent


include gen/common.mk
-include $T/*.d

$(shell mkdir -p gxe)

B=./tst
T=./tst

gxe/gx_enum_lookups.h : gen/build-enum-maps.rb gen/perfect_map.rb
	$< ./*.h > $@
gxe/syserr.h :          gen/build-error-lookups
	$< > $@
gxe/gx_log_table.h :    gen/build-gx_log_table.rb ./gx_log.h
	$< > $@

KVKEYS=$B/.kv_keys.h
ENUM_MAPS=$B/.test_enums.h
test : gx_all $(KVKEYS) $(ENUM_MAPS) $B/test_gxerr ./*.h gx*.h
	$B/test_gxerr
	@rm -f $B/test_gxerr
	@rm -f $B/*.o

$(ENUM_MAPS) : gen/build-enum-maps.rb gen/perfect_map.rb $(KVKEYS)
	$< $T/*.h $T/.*.h > $@

$(KVKEYS) : gen/auto-kv-keys $T/*.c $T/*.h
	$< kv_log_keys $T/*.c $T/*.h > $@

$B/test_gxerr : $B/test_gxerr.o $B/test_gxerr_2.o
	$(call LINK)
$B/test_gxerr.o : $T/test_gxerr.c $B/.kv_keys.h $T/test_gxerr_help.h
	$(call gcc, -I.. -I. -c)
$B/test_gxerr_2.o : $T/test_gxerr_2.c $(KVKEYS) $T/test_gxerr_help.h
	$(call gcc, -I.. -I. -c)
