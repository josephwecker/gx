#ifndef _GX_STRING_H
#define _GX_STRING_H
/// For doing very fast inline sprintfs. tstr = tmpstring
#define $BUFSIZE 4096
typedef struct gx_strbuf {
    char  buf[$BUFSIZE];
    char *p;
} gx_strbuf;
static gx_strbuf _gx_tstr_buf = {.buf={0},.p=NULL};
static char  _gx_tstr_empty[]   = "";
#define $(FMT,...) $S(_gx_tstr_buf, FMT, ##__VA_ARGS__)
#define $reset() $Sreset(_gx_tstr_buf)

#define $S(STRBUF, FMT, ...) (char *)({                                   \
        if(rare(!(STRBUF).p)) (STRBUF).p = (STRBUF).buf;                  \
        char *_res = (STRBUF).p;                                          \
        int   _rem = sizeof((STRBUF).buf) - ((STRBUF).p - (STRBUF).buf);  \
        if(rare(_rem < 25)) _res = _gx_tstr_empty;                        \
        else {                                                            \
            int _len = snprintf((STRBUF).p,                               \
                sizeof((STRBUF).buf) - ((STRBUF).p - (STRBUF).buf),       \
                FMT, ##__VA_ARGS__);                                      \
            if(freq(_len > 0)) (STRBUF).p += _len;                        \
            if(rare((STRBUF).p > (STRBUF).buf + sizeof((STRBUF).buf)))    \
                (STRBUF).p = &((STRBUF).buf[sizeof((STRBUF).buf) - 2]);   \
            (STRBUF).p[0] = (STRBUF).p[1] = '\0';                         \
            (STRBUF).p += 1;                                              \
        }                                                                 \
        _res;                                                             \
    })

#define $Sreset(STRBUF) do {                                              \
    (STRBUF).buf[0] = '\0';                                               \
    (STRBUF).p = (STRBUF).buf;                                            \
} while(0);


#endif
