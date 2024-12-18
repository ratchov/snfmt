# snfmt - user-defined conversions for snprintf()

## Description

The `snfmt()` function formats strings by calling `snprintf()`, but
allows the caller to define additional conversions. For each occurrence of
an user-defined conversion in the format string, `snfmt()` invokes the
user-provided call-back function that implements the conversions.

Example:

        snfmt(myfunc, buf, size, "blob = {hexdump:%p,%zu}", blob, sizeof(blob));

The user-defined conversions in the format strings are between curly brackets.
They are composed by a name, and an optional colon followed by a comma-separated
list of `%`-based specifiers that match `snfmt()` variadic arguments. This
syntax makes `snfmt()` format strings work with `snprintf()` as well, allowing
compilers to report format string errors.

The call-back function is passed as the first `snfmt()` argument. It is
defined as follows:

        union snfmt_arg {
                long long i;
                unsigned long long u;
                double f;
                const char *s;
                void *p;
        };

        int myfunc(char *buf, size_t size, const char *fmt, union snfmt_arg *arg);

The `arg` array contains the values fetched from the `snfmt()` variadic
argument list, promoted to above types. They correspond to the specifiers list
in the curly brackets. The `fmt` string is set to the string between curly
brackets with the flags and modifiers removed (ex. `%08llx` is replaced
by `%x`). The `fmt` string will be used by `myfunc` to determine the conversion
to perform; if no conversion could be performed, it should return -1, informing
`snfmt()` to call `snprintf()` instead.

`snfmt()` and the call-back function write at most `size - 1` characters
to `buf`, followed by a terminating 0. If `size` is zero, no characters are
written. The functions return the number of bytes that would have been output
if `size` was unlimited.

## Limitations

Argument reordering (a.k.a `<number>$` immediately after the `%` character) is
not supported.

Non-standard `%`-specifiers are not supported (ex. `%D`, `%O`, `%C`, `%S` and
probably others).

If `snfmt()` fails to parse the format string (syntax error or unsupported
`%`-conversion specifier), the output string is truncated, and the size of
the truncated string is returned.

The size of user-defined conversions is limited to 64 characters. User-defined
conversions may have at most 8 arguments.

## Example

Example (pseudo-code) of typical use:

        int debug_fmt(char *buf, size_t size, const char *fmt, union snfmt_arg *args)
        {
                if (strcmp(fmt, "hexdump:%p,%u") == 0) {
                        /* convert using args[0].p and args[1].u */
                } else if (...) {
                        ...
                } else
                        return -1;
        }

        void debug(const char *fmt, ...)
        {
                va_list ap;
                char buf[256];

                va_start(ap, fmt);
                snfmt_va(debug_fmt, buf, sizeof(buf), fmt, ap);
                va_end(ap);

                fputs(buf, stderr);
        }

        int main(void)
        {
                ...
                debug("x = %d, blob = {hexdump:%p,%zu}\n", x, blob, sizeof(blob));
                ...
        }
