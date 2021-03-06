

check-type
errno-lookup-module



- - - - - - -

* Likely to repeat- configuration/system-settings
* Likely system-temporary
* Likely process-temporary
* Completely transient (e.g., EAGAIN)



- - - - - -

    *-->[Check Functions]-->(Error-stack)-->[Explicit err log rpt]--->[Error-rpt-expander]--->[[_gx_log]]
                                        \-->[Implicit err log rpt]-/   ^
                                                                       |
                                                                /error-class-defs/

    *-->[Adhoc logging / stats]-->[_gx_log]-->[gen-logm expander]-->[log-rpt-dispatch]--*>[...Loggers...]
                                                                            ^
                                                                            |
                                                                     (log-entry common consts etc.)

    [...Loggers...]--->/registered/-->/active-loggers/--->[[log-rpt-dispatch]]
           |                                          \-->[preemptive-filter]-->[[gen-logm expander]]
           |                                                                 |->[[error-rpt-expander]]
           |                                                                 |->[[Explicit err log rpt]]
           v                                                                 \->[[Implicit err log rpt]]
        [...external-systems/files/etc...]



* error-rpt-expander - including optional backtrace only when the situation
  warrants it- i.e., be reactive to system needs.
* error-class-defs can give "min severity"s that upgrade a user's
  severity/msg-class (e.g., all that failed was opening a non-essential file so
  main prog sets severity to warn but error-class-def sees that it's an EFAULT
  so upgrades it to error- every final report being the max(given,defined)
  msg-type.


* Important one: mqueue based w/ user-space backup queue where higher priority
  items can swap out lower-priority ones, msgs about missing messages, etc. can
  be placed.

* Status reports that work like /proc filesystem- simply file entrypoints that
  "query" the server.

* Main implemented loggers:  debug(stderr), stress(syslog), comm(mqueue)

- - - - - - -

logger
  - filter
    - msg-classes
    - error-numbers
    - ...




- - - - -



Central concerns:
  - Error handling
    - Doesn't obscure "sunny-day" flow.
    - Doesn't disturb code optimization & processor-cache/pipeline.
    - Assume they are rare code paths.
  - Logging
    - Smallest performance impact possible
      - Never ever block central flow
    - Assume they are very common code paths and need to be highly optimized.
    - As informative as possible
    - As flexible as possible
    - An error / problem with logging should never affect the server and visa
      versa.

  - If there is no logger that cares about a given message class, processing of
    that message class error report and/or log message should halt as early as
    possible- including being completely compiled away if possible...


- - - - -


Error-logging graceful degredation- especially for "important" classes: If
existing logger fails, try a simpler one (syslog?), and so forth. As a very
last resort, at least logged to some global structure for attempted reporting
later or on exit. Should be robust against:
 1. Network failure
 2. Disk failure
 3. Memory failure
 4. Misc. OS failure

- - - - - - - - 


(from
http://www.trottercashion.com/2012/08/12/how-to-write-good-log-messages.html)


 * host
 * version   (version of the code that generated msg)
 * pid

 * level     (importance)
 * user      (peer, IP or session-id etc.)
 * location  (location in the code that generated msg)
 * message   (descriptive, ideally)
 * variables (k/v pairs)
 * time      (in UTC)


(I would like to have 'message' or 'tag' and then 'report' which can have
newlines / formatting / etc.)

- - - - - - - - - - 




* **all**
  - context (?)
  - location
    - source-version  (commit hash)
    - file
    - line
    - function
    - stack-trace          (possibly only if class warrants it?)
  - utc-time (when string, in ISO 8601 format: YYY-MM-DDThh:mm:ss...Z)
  - pid/tid
  - ppid

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



|macro| desc                           | returnval-error-condition      | pre-expression | to-err-num          | label-trans      | msg-trans    | post-action | expr example           |
| --- | ------------------------------ | ------------------------------ | -------------- | ------------------- | ---------------- | ------------ | ----------- | ---------------------- |
|X[s] | -1 indicates error in errno    | `(int)(EXPR) == -1`            | `errno=0`      | `_err_num=errno`    | `errno_h`        | `strerror_r` | `errno=0`   | `open(...)`            |
|Xne  | NULL indicates error in errno  | `(void *)(EXPR) == NULL`       | `errno=EFAULT` | `_err_num=errno`    | `errno_h`        | `strerror_r` | `errno=0`   | ?                      |
|Xn   | NULL indicates error (assert?) | `(void *)(EXPR) == NULL`       |                | `_err_num=GX_ENULL` | `errno_h`        | `strerror_r` | `errno=0`   | `malloc(...)`          |
|Xm   | Detect mmap errors             | `(void *)(EXPR) == MAP_FAILED` | `errno=ENOMEM` | `_err_num=errno`    | `errno_h`        | `strerror_r` | `errno=0`   | `mmap(...)`            |
|Xz   | Any nonzero is an error value  | `(_res=(EXPR)) != 0`           |                | `_err_num=_res`     | `errno_h`        | `strerror_r` |             | `pthread_create(...)`  |
|Xnz  | Zero indicates error in errno  | `(EXPR) == 0`                  | `errno=0`      | `_err_num=errno`    | `errno_h`        | `strerror_r` | `errno=0`   | `fscanf(...)`          |
| --- | ------------------------------ | ------------------------------ | -------------- | ------------------- | ---------------- | ------------ | ----------- | ---------------------- |
|Xav  | Nonzero is syserror OR custom  |`(int)(_res=(EXPR)) < 0`        |                | `_err_num=-_res`    | `errno_h`        | `strerror_r` |             | libav/ffmpeg functions |
| ... |                                |`(int)(_res) > 0`               |                | `_err_num=_res`     | `avutil_error_h` | `av_strerror`|             |                        |
| --- | ------------------------------ | ------------------------------ | -------------- | ------------------- | ---------------- | ------------ | ----------- | ---------------------- |

TODO:
  * `hstrerror` based
  * `gai_strerror` based
  * etc...


- switch-semantics vs. if-semantics orthogonal to error-check-type
- 
- 


### Error Actions ###



`X_IGNORE`

`X_STORE` (?)
`X_DUMP_CORE` (?)


### Debugging ###


completely orthogonal to these modules?


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
