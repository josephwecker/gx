gx_all : gxe/syserr.h

$(shell mkdir -p gxe)

gxe/syserr.h : gen/build-error-lookups
	$< > $@
