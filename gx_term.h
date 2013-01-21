#ifndef _GX_TERM_H
#define _GX_TERM_H

/// For quickly writing const stuff to stderr and not caring if it fails
#define C_NRM  "\e[0m"
#define C_TTL  "\e[38;5;196m"
#define C_GR1  "\e[38;5;248m"
#define C_GR2  "\e[38;5;240m"
#define WSTDE(CONTENT)        _( write(STDERR_FILENO, (CONTENT), sizeof(CONTENT)) ) _ignore()
#define WSTDEx(CONTENT,TIMES) ({ int wstde_j; int wstde_mx=(TIMES); for(wstde_j=0; wstde_j<wstde_mx; wstde_j++) WSTDE(CONTENT); })

#endif
