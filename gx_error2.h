/**
   @file      gx_error.h
   @brief     Lots of error-handling/logging to help unobscure C code.

   @author    Joseph A Wecker <joseph.wecker@gmail.com>
   @copyright
     Except where otherwise noted, Copyright (C) 2012 Joseph A Wecker

     MIT License

     Permission is hereby granted, free of charge, to any person obtaining a
     copy of this software and associated documentation files (the "Software"),
     to deal in the Software without restriction, including without limitation
     the rights to use, copy, modify, merge, publish, distribute, sublicense,
     and/or sell copies of the Software, and to permit persons to whom the
     Software is furnished to do so, subject to the following conditions:

     The above copyright notice and this permission notice shall be included in
     all copies or substantial portions of the Software.

     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
     IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
     FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
     THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
     LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
     FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
     DEALINGS IN THE SOFTWARE.
*/

#ifndef GX_ERROR_H
#define GX_ERROR_H

/// Stringifies the current line of code
#define __STR2(x) #x
#define __STR(x) __STR2(x)
#define __LINE_STR__ __STR(__LINE__)

// TODO: move to gx.h
#define _rare(X) __builtin_expect(!!(X), 0)

#define _gx_mrk(EXPR_STR) \
    ({ _gx_mark_err_do((errno ? errno : ( _e ? _e : EINVAL )), \
           __FILE__, __LINE_STR__, __FUNCTION, EXPR_STR); 1; })

#define _reset_e() errno=0; int _e=0

/*
   if_esys    // Error if expression is -1         errno set
   if_emap    // Error if expression is MAP_FAILED errno set
   if_enz     // Error if expression is non-zero   value as errno
   if_ez      // Error if expression is zero       errno set
   if_enull   // Error if expression is null       errno possibly set
   if_eio     // (NYI) stdio ferror style errors (doesn't trigger on feof)
   if_eavc    // (NYI) avconv-style errors
*/

#define _esys(E)  ({ _reset_e(); (_rare(    (int)(E) == -1         )) ? _gx_mrk(#E) : 0; })
#define _emap(E)  ({ _reset_e(); (_rare(    (int)(E) == MAP_FAILED )) ? _gx_mrk(#E) : 0; })
#define _enz(E)   ({ _reset_e(); (_rare((_e=(int)(E))!= 0          )) ? _gx_mrk(#E) : 0; })
#define _ez(E)    ({ _reset_e(); (_rare(         (E) == 0          )) ? _gx_mrk(#E) : 0; })
#define _enull(E) ({ _reset_e(); (_rare( (void *)(E) ==(void *)NULL)) ? _gx_mrk(#E) : 0; })
// TODO: _eio  - stdio ferror style errors (distinguish feof)
// TODO: _eavc - libavconv/ffmpeg style error codes

#define if_esys(E) if( _esys(E) )
#define if_emap(E) if( _emap(E) )


#endif
