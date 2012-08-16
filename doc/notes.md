



### TODO



- General (applies to most or all modules)
  - Consider using gcc zero-length arrays to put some structures directly inside
    (at the head of) the allocated memory- wrt ringbuffers and mmipc- may also
    need __attribute__ ((aligned (PAGE_SIZE))).

- gx.h
  - Standardized function attributes, more of them, and clean up. Consider
    using linux-kernel-like macros:  __inline__ __must_check__, etc.
  - 

- gx\_error.h
  - assertion macros
  - possibly more stuff from here: [JPL C Coding standards](lars-lab.jpl.nasa.gov/JPL_Coding_Standard_C.pdf)

- gx\_event.h
  - Sticky problem is that of consume vs. no-consume
    1. Pass rbuf to handler as-is, with write-pointer at end of fully available
       data, then allow the handler to either:
       - advance the read-pointer itself
       - return the number of bytes to advance the read-pointer
       - return some flag indicating consume / don't consume on the read-pointer
       - (todo: allow less than expected?- or set expected somehow to be "next
         chunk available"- useful for things like http header parsing or even
         possibly amf0 message parsing)
  - probably change the fail case of a handler to be more specific-
    - detect if it's closed?
    - assume rest of current should be dumped to dev/null
    - nothing- already optimal?
    - no such thing as fail? clear rbuf and continue? 
    - need to trigger connection-closed handler on certain socket-read
      errors/bytes-read values? (might take care of the macosx case better)


- Misc. (not really a part of the header files)
  - Better / way-better syntax highlighting for vim - guess at keyword groups,
    even if it's wrong it will help.
