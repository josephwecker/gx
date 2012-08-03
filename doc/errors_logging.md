
* **all**
  - context
  - file
  - line
  - function
  - stack-trace          (possibly only if class warrants it?)

* **error**
  - error-value          (such as errno)
  - error-label          (via specified lookup)
  - error-description    (via specified lookup)
  - reference-expression (well, usually just the called function)
  - message-class        (usually severity)
  - recovery-action      (? if possible- would be nice to be explicit)

* **log**
  - message-class
  - description
  - label
  - (possibly number?)
  - [custom k/v pairs]

- - - - - - -

### Error Checking ###


Possibly allow it to be defined dynamically as well?

Would need to know:
  - Return-value-error-condition (as in, a logical statement or something)
  - ?? Actions allowed / disallowed and/or Message-classes allowed / disallowed
  - Code for return-value translation to error-number


Thought:

    Xi(...) == X(...) X_IGNORE;



| macro | returnval-error-condition | pre-action     | to-err\_num       | label-trans  | msg-trans    | post-action |
| ----- | ------------------------- | -------------- | ----------------- | ------------ | ------------ | ----------- |
| X     | `(int)(EXPR) == -1`       | `errno=0`      | `err_num=errno`   | `errno_h`    | `strerror_r` | `errno=0`   |
| Xn    | `(void *)(EXPR) == NULL`  | `errno=EFAULT` | `err_num=errno`   | `




### message-class ###
Specifies:
  - Automatically attached data to include / exclude
  - Label lookup function
  - Description lookup function


### Contexts

`GX_LOGCTX_STRING_F(these_objects_to_string);`

  - if nothing set, defaults to a handler that emits the %p of the pointer

`GX_LOGCTX(&the_obj);`


* Would be awesome if they turned into nops if nothing in the subsequent code
  could utilize contexts.
* Not sure how to make this thread-safe (automatically use per-thread storage?)


### Multithreading + Reentrancy

Library in charge of properly isolating error value etc., all of the
implications of multi-threading, etc.- so they don't have to be worried about
when doing lookup functions or logger implementations, specifications etc.


- - - - - - -

 - Remove initialization dependency (use lazy instantiation instead?)
 - Some basic loggers?


- - - - - - -

    X_<msg-class>
    X_LOG(<msg-class>)

    (need the ability to indicate different error msg translation functions)
    {msg, fmt, ...}
    OR
    {msg_function, function_name}  e.g.,   {msg_function, pam_strerror}

    GX_LOG(<msg-class>, [<data>])
    GX_LOG_<msg-class>([<data>])


  They'll all only do anything if that specific class is being listened to by a
  logger - otherwise disappear at compile-time.

  logger(name, callback, classes...)





  err\_code (errno and any int)  ---> (strerror-like-lookup-function)   ---> log-message
                                 ---> (label lookup function) ---> 


error output
 - number
 - label
 - static-message
