gx_all : gxe/syserr.h | .silent

$(shell mkdir -p gxe)

gxe/syserr.h : gen/build-error-lookups
	$< > $@

.silent :
	@:

