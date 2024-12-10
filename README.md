# snfmt - extensible snprintf-like conversion

## Description

The `snfmt()` function produces a string according to the given format,
similarly to `snprintf()`. Custom conversions may be used between curly
brackets. They are performed by the call-back function passed as the
first `snfmt()` argument.

Example:

    snfmt(myfunc, buf, size, "x = %d, ptr = {myobj:%p}", x, ptr);
    snfmt(myfunc, buf, size, "blob = {hexdump:%p,%u}", blob, sizeof(blob));

Custom conversion specifiers (ex. `"myobj:%p"`) are composed by a name,
and an optional Colon followed by a comma-separated list of %-based
specifiers.

The call-back function is called by `snfmt()` whenever a custom
conversion needs to be performed. Its prototype is as follows:

    union snfmt_arg {
            long long i;
            unsigned long long u;
            double f;
            const char *s;
            void *p;
    };

    size_t myfunc(char *buf, size_t size, const char *fmt, union snfmt_arg *arg);

The `arg` array contains a copy of the values fetched from the `snfmt()`
variable argument list, they correspond to the specifiers list in the curly
brackets. The `fmt` string is set to the string between curly brackets with
the modifiers removed (ex. `%08llx` is replaced by `%x`). So `fmt` can be used
inside `myfunc` to determine the conversion to perform.

As for `snprintf()`, `snfmt()` and the call-back function write at
most `size - 1` characters to `buf`, followed by a terminating 0. If `size` is
zero, no characters are written. The functions return the number of bytes that
would have been output if `size` was unlimited.

## Example

Example (pseudo-code) of typical use:

    size_t myfunc(char *str, size_t size, const char *fmt, union snfmt_arg *args)
    {
            if (strcmp(fmt, "myobj:%p") == 0) {
                    ... /* convert using args[0].p */
            } else if (strcmp(fmt, "hexdump:%p,%u") == 0) {
                    ... /* convert using args[0].p and args[1].u */
            } else
                    return 0;
    }

    void debug(const char *fmt, ...)
    {
            va_list ap;
            char buf[256];

            va_start(ap, fmt);
            snfmt_va(myfunc, buf, sizeof(buf), fmt, ap);
            va_end(ap);

            fprintf(stderr, "%s\n", buf);
    }

    int main(void)
    {
            ...
            debug("ptr = {myobj:%p}", ptr);
            debug("blob = {hexdump:%p,%zu}", blob, sizeof(blob));

            ...
    }

## Restrictions

Argument indexes (ex. `5$`) are not supported. Width or precision
may not be passed as arguments with the `*` character. Wide characters
are not supported. `%A`, `%D`, `%E`, `%F`, `%G`, `%O`, `%S` `%U`, `%X`,
and `%n` are not supported.
