# snfmt - safe and minimal snprintf-style formatted conversion

Example:

    snfmt(myfunc, buf, size, "x = %d, ptr = {myobj:%p}", x, ptr);
    snfmt(myfunc, buf, size, "blob = {hexdump:%p,%u}", blob, sizeof(blob));

The `snfmt()` function produces a string according to the given format,
similar to `snprintf()`. It supports custom conversions through the
call-back function given as its first argument.

A user-registered conversion specifier (ex. `"{myobj:%p}"`) is composed
by the conversion name, a collon, and the comma-separated list of
specifiers used to fetch the function arguments from the `snfmt()` variable
argument list.

In the above example, the `myfunc` function is called by `snfmt` whenever
a custom conversion needs to be performed.

    union snfmt_arg {
            long long i;
            unsigned long long u;
            double f;
            const char *s;
            void *p;
    };

    size_t myfunc(char *str, size_t size, const char *fmt, union snfmt_arg *args);

the `args` array contains a copy of the values fetched from the `snfmt()`
arguments, they correspond to the specifiers list in the curly brakets.
The `fmt` is set to the string between curly brakets with any
modifier removed (ex. `%08llx` is replaced by `%x`). So `fmt` can be used
inside `myfunc` to determine the conversion to perform.

The %-specifiers define the argument type, while the {}-specifiers
set the method to represent it as a string. Consequently, only
the `%a`, `%c`, `%d`, `%e`, `%f`, `%g`, `%u`, `%x`, `%o`, `%p`, and `%s`
conversion specifiers are needed and are the only supported ones.
Integer specifiers may be prefixed by a size modifier (`l`, `ll`, `j`, `t`,
and `z`). No other modifiers are supported, but a `{}`-based conversion
may be defined to use any non-default width or precision. 

The format syntax makes any `snfmt()` format string also work
with `snprintf()`.  Most compilers will generate the same errors if it
detects a misuse (gcc, clang, and msvc do). If `snfmt()` is used for
debug purposes only, it could be easily replaced by `snprintf()` in
the final program version.

As for `snprintf()`, above functions write at most `size - 1` characters
to `str`, followed by a terminating 0. If `size` is zero, no
characters are written.

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
