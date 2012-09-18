

$(shell mkdir -p gxe)

gxe/syserr.h : gen/build-errlist
	gen/build-errlist > gxe/syserr.h
