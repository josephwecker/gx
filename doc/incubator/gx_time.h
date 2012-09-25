/**
 * Time vaporware.
 *
 *
 *
 *
 * GOALS:
 *   - SPEED + PRECISION + PARALLELISM especially for TIMING + LOGGING
 *
 *   - Minimize syscalls as much as possible, make as parallel as possible
 *     - shared-memory
 *     - cpu timing instructions
 *     - cpu atomic increment instructions
 *   - Very fast stamping- great for internal timing / intervals
 *   - Pretty-darn-monotonic (see tradeoffs)
 *   - High resolution - nanoseconds
 *   - Synced at initialization to walltime UTC
 *   - ISO-8601 string output- hopefully native so no need for strftime/gmtime etc.
 *     - Optimize for now + future
 *     - Possibly use an epoch that's past the year 2000 to eliminate several
 *       cycles.
 *     - If, for example, it sees that it's not going to be a leap-year for
 *       several years, it may drop into a version of the function at
 *       initialization that doesn't try to calculate it every time, etc.
 *     - (or, in other words, will probably be easier to populate the
 *       calendar-time structure at the beginning and on subsequent invocations
 *       update it with a much smaller algorithm then the full from-epoch
 *       calculation...)
 *   - Tradeoffs (things it [sometimes deliberately] will _NOT_ try to do well:
 *     - While precise, not necessarily accurate (will probably skip things like
 *       leap-seconds etc.)
 *     - Real monotonicity- probably allow NTP adjustments and won't try to be
 *       monotonic across multiple cpus/cores, etc.
 *     - Possibly won't support datetimes prior to 2000
 *     - Definitely not trying to be a fullfeatured time library! Specially
 *       optimzed for the usecases stated (interval timing & log-entry
 *       timestamping [ISO-8601 formatted])
 *
 */


typedef struct gx_datetime {


} gx_datetime;

