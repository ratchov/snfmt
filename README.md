# snfmt - user-defined conversions for snprintf()

## Description

The `snfmt()` function formats strings by calling `snprintf()`, but
allows the caller to define additional conversions. For each occurrence of
an user-defined conversion in the format string, `snfmt()` invokes the
user-provided call-back function that implements the conversions.

Examples:

        snfmt(myfunc, buf, size, "x = %d, ptr = {myobj:%p}", x, ptr);
        snfmt(myfunc, buf, size, "blob = {hexdump:%p,%zu}", blob, sizeof(blob));

The user-defined conversion specifiers in the format strings are between
curly brackets. They are composed by a name, and an optional colon followed
by a comma-separated list of `%`-based specifiers that match `snfmt()`
variadic arguments. This syntax makes `snfmt()` format strings work
with `snprintf()` as well, allowing compilers to report format string errors.

The call-back function is passed as the first `snfmt()` argument. It is
defined as follows:

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
the flags and modifiers removed (ex. `%08llx` is replaced by `%x`). The `fmt`
string will be used inside `myfunc` to determine the conversion to perform.

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

## Limitations

Argument reordering (a.k.a `<number>$` immediately after the `%` character) is
not supported.
