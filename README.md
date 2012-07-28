GX - Generic eXtra library for C
=================================

This repository is public but "unknown" at the moment. I intend to blog about
it / present it / submit to reddit & hacker news etc. later. If you're reading
this, you're early.

Some of the header files are generally awesome and may find general appeal.

We're using lots of this in some mission-critical systems stuff at Justin.tv/Twitch.tv.

Some of the header files are probably only interesting to me.


### General philosophy:

* Optimized for Linux (2.6.33 +), but compiles/runs well enough on Mac OS.
* Header-files only- no compiled libraries etc.
* Interfaces generally laid out, but specific optimizations and sometimes
  function implementations etc. only implemented when actually needed (by
  myself or at someone else's request).
* C, but not necessarily idiomatic C. Definitely not idiomatic C++, although
  you can probably get it all to work w/ C++.
* Lots of GCC-specific stuff/extensions at the moment, but I have no objection
  to porting to other compilers as needed.
* MIT licensed

### Some of the important current modules:

| Header        | Summary                                                                          |
| ------------- | -------------------------------------------------------------------------------- |
| gx            | Lots of small stuff. Resource pools. Clone wrapper...                            |
| gx\_error     | Awesome error handling/logging idioms & macros to make code look good.           |
| gx\_event     | Very highly tuned event-loop. Like a faster and feature-reduced libev w/ special tcp tuning |
| gx\_ringbuf   | Super-optimized memory-mapped ringbuffers (circular buffers) & ringbuffer-pools. |
| gx\_zerocopy  | IO directives for highly optimized data transfer between fds of various types.   |
| gx\_mfd       | Memory-fd. Mmap fun & selectable/pollable fd's that trigger events when a variable changes. |


**NOTES:**
* Most stuff is born in `gx.h` and then moves into its own header when it either
  gets too big or too esoteric. I suppose some things may eventually get big
  enough or decoupled enough to become its own project/library altogether, but
  at the moment most of the stuff here is pretty interdependent.
* Most of the zerocopy stuff isn't implemented yet. Generic functionality is
  implemented when a specific fd-fd combination is first needed, and then
  optimize later.

### Lesser modules:

| Header        | Summary                                                                          |
| ------------- | -------------------------------------------------------------------------------- |
| gx\_net       | Wrappers for common network/socket needs.                                        |
| gx\_cpus      | (Semi)-portable wrapper for determining # of cpus, cpu-affiliation, timing, etc. |
| gx\_system    | (Semi)-portable wrapper for getting local & system-wide usage & performance etc. |
| gx\_endian    | Runtime-endianness detection and eventually a bunch of utilities...              |
| gx\_pdv       | (Persistent Data Vector) Will be an extension of gx\_mfd for dataflow/pipe programming |
| gx\_token     | Plans on building secure crypto self-referencing auth tokens in a somewhat generic way. |
| gx\_shuttle   | Eventually a generalization of some C hot-code-swapping stuff I'm doing          |
| gx\_tcpserver | DEPRICATED - everything important already moved to gx\_event                     |


TMP TODO
---------

* Clean up file headings, license statements etc.
* ifndef ...\_H wrapper around all
* Possibly C++ wrapper around all
* Mark unfinished functions & deprecated ones
* Fill in some known incomplete functions w/ compile-time-errors
* Remove the nonstarter gx\_ * headers
* Possibly clean up namespace and \_gx conventions...
* Allow for different pool names
* Remove this section from the readme
