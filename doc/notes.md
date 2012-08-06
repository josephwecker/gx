



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

- Misc. (not really a part of the header files)
  - Better / way-better syntax highlighting for vim - guess at keyword groups,
    even if it's wrong it will help.
