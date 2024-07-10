# snfmt - safe and minimal snprintf-style formatted conversion

Example:

    snfmt(str, size, "x = %d, ptr = {myobj:%p}", x, ptr);
    snfmt(str, size, "blob = {hexdump:%p,%u}", blob, sizeof(blob));

The `snfmt()` function produces a string according to the given format,
similar to `snprintf()`. It supports custom conversion functions and
has few restrictions.

A user-registered conversion specifier (ex. `"{myobj:%p}"`) is composed
by the conversion name, a collon, and the comma-separated list of
specifiers used to fetch the function arguments from `snfmt()` variable
argument list.

In the above example, the conversion functions are registered as
follows:

    snfmt_addfunc(myobj_fmt, "myobj:%p");
    snfmt_addfunc(hexdump_fmt, "hexdump:%p,%u");

where the `myobj_fmt()` and `hexdump_fmt()` are the user functions
performing the conversion. Both use the same prototype:

    size_t myobj_fmt(char *str, size_t size, union snfmt_arg *args);
    size_t hexdump_fmt(char *str, size_t size, union snfmt_arg *args);

the `args` array contains a copy of the values fetched from the `snfmt()`
arguments, they correspond to the specifiers list in the curly brakets.

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

    void debug(const char *fmt, ...)
    {
            va_list ap;
            char buf[256];

            va_start(ap, fmt);
            snfmt_va(buf, sizeof(buf), fmt, ap);
            va_end(ap);

            fprintf(stderr, "%s\n", buf);
    }

    int main(void)
    {
            snfmt_addfunc(myobj_fmt, "myobj:%p");
            snfmt_addfunc(hexdump_fmt, "hexdump:%p,%u");

            ...

            debug("ptr = {myobj:%p}", ptr);
            debug("blob = {hexdump:%p,%zu}", blob, sizeof(blob));

            ...

            snfmt_rmfunc(myobj_fmt, "foobar:%p");
            snfmt_rmfunc(hexdump_fmt, "hexblob:%p,%u");
            return 0;
    }
