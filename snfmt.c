/*
 * Copyright (c) 2023 Alexandre Ratchov <alex@caoua.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "snfmt.h"

/*
 * Max number of args any function may take
 */
#define SNFMT_NARG		8

/*
 * Max size of "{foobar:%x,%y,%z}" string
 */
#define SNFMT_NAMEMAX		64

/*
 * We want the real function to be defined
 */
#ifdef snfmt
#undef snfmt
#endif

struct snfmt_ctx {
	const char *fmt;
	va_list ap;
};

struct snfmt_func {
	const char *name;
	size_t (*func)(char *, size_t, union snfmt_arg *);
	struct snfmt_func *next;
};

struct snfmt_func *snfmt_func_list;

/*
 * Register the given conversion function.
 *
 * The first two arguments are the destination buffer and its size,
 * the third arguments is the conversion specifier (c, d, u, x, p, or s)
 * and the last argument is the value to convert. It's converted
 * to union snfmt_arg by the caller.
 */
void
snfmt_addfunc(size_t (*func)(char *, size_t, union snfmt_arg *), const char *name)
{
	struct snfmt_func *f;

	f = malloc(sizeof(struct snfmt_func));
	if (f == NULL)
		return;

	f->name = name;
	f->func = func;
	f->next = snfmt_func_list;
	snfmt_func_list = f;
}

/*
 * Unegisterer the given conversion function
 */
void
snfmt_rmfunc(size_t (*func)(char *, size_t, union snfmt_arg *))
{
	struct snfmt_func *f, **pf;

	pf = &snfmt_func_list;
	for (;;) {
		if ((f = *pf) == NULL)
			return;
		if (f->func == func)
			break;
		pf = &f->next;
	}

	*pf = f->next;
	free(f);
}

/*
 * Find the function pointer corresponding to the given name
 */
static struct snfmt_func *
snfmt_getfunc(const char *name)
{
	struct snfmt_func *f = NULL;

	for (f = snfmt_func_list; f != NULL; f = f->next) {
		if (strcmp(f->name, name) == 0)
			return f;
	}

	return NULL;
}

/*
 * Parse the characters after '%': modifier followed by a conversion character
 * and convert the va_list argument to union snfmt_arg.
 */
static int
snfmt_scanpct(struct snfmt_ctx *rctx, union snfmt_arg *rarg)
{
	va_list ap;
	const char *fmt;
	union snfmt_arg arg;
	size_t size;
	int c;

	va_copy(ap, rctx->ap);
	fmt = rctx->fmt;

	/*
	 * skip common flags, width, and precision modifiers
	 */
	while (1) {
		c = *fmt++;
		if (c != ' ' && c != '#' && c != '+' && c != '-' &&
		    c != '.' && !(c >= '0' && c <= '9'))
			break;
	}

	/*
	 * parse optional size specifier (l, ll, z, ...)
	 */
	switch (c) {
	case 'l':
		c = *fmt++;
		if (c == 'l') {
			c = *fmt++;
			size = sizeof(long long);
		} else
			size = sizeof(long);
		break;
	case 'j':
		c = *fmt++;
		size = sizeof(intmax_t);
		break;
	case 't':
		c = *fmt++;
		size = sizeof(ptrdiff_t);
		break;
	case 'z':
		c = *fmt++;
		size = sizeof(size_t);
		break;
	default:
		size = sizeof(int);
	}

	/*
	 * convert the argument to union snfmt_arg
	 */
	switch (c) {
	case 'c':
		arg.i = va_arg(ap, int);
		break;
	case 'd':
	case 'u':
	case 'x':
	case 'o':
		if (size == sizeof(int))
			arg.i = va_arg(ap, int);
		else if (size == sizeof(long))
			arg.i = va_arg(ap, long);
		else
			arg.i = va_arg(ap, long long);
		break;
	case 'a':
	case 'e':
	case 'f':
	case 'g':
		arg.f = va_arg(ap, double);
		break;
	case 's':
	case 'p':
		arg.p = va_arg(ap, void *);
		break;
	default:
		fprintf(stderr, "%s: %c: unsupported fmt\n", __func__, c);
		abort();
	}

	*rarg = arg;
	va_copy(rctx->ap, ap);
	rctx->fmt = fmt;

	va_end(ap);
	return c;
}

/*
 * Parse conversion function name and arguments in curly braces
 */
static struct snfmt_func *
snfmt_scanfunc(struct snfmt_ctx *rctx, union snfmt_arg *arg)
{
	struct snfmt_ctx ctx;
	struct snfmt_func *f = NULL;
	char name[SNFMT_NAMEMAX];
	size_t n = 0, index = 0;
	int c;

	va_copy(ctx.ap, rctx->ap);
	ctx.fmt = rctx->fmt;

	/*
	 * copy up to the ':' char
	 */
	do {
		if ((c = *ctx.fmt++) == 0 || n == SNFMT_NAMEMAX - 1)
			goto ctx_free;
		if (c == '}')
			goto found;
		name[n++] = c;
	} while (c != ':');

	/*
	 * copy %<conv_char>[,%<conv_char>]...
	 */
	while (1) {
		if ((c = *ctx.fmt++) != '%' || n == SNFMT_NAMEMAX - 1)
			goto ctx_free;
		name[n++] = c;
		if (index == SNFMT_NARG || n == SNFMT_NAMEMAX - 1)
			goto ctx_free;
		name[n++] = snfmt_scanpct(&ctx, arg + index++);

		if ((c = *ctx.fmt++) == '}')
			break;
		if (c != ',' || n == SNFMT_NAMEMAX - 1)
			goto ctx_free;
		name[n++] = c;
	}

found:
	/* terminate the string */
	name[n] = 0;

	f = snfmt_getfunc(name);
	if (f == NULL)
		goto ctx_free;

	va_copy(rctx->ap, ctx.ap);
	rctx->fmt = ctx.fmt;
ctx_free:
	va_end(ctx.ap);
	return f;
}

/*
 * Format the given string using snprintf(3)-style semantics, with the
 * following differences:
 *
 * - A custom conversion function may used by suffixing the conversion
 *   specifier with a tag between cruly braces (ex. "{foobar:%p}"). Any
 *   conversion function may be registered during initilization using
 *   the snfmt_addfunc() function.
 *
 * - Only the a, c, d, e, f, g, o, p, s, u, and x conversion specifiers
 *   are supported, possibly prefixed by a size modifier (l, ll, j, t, and z).
 *   No other snprintf(3)-style modifiers are supported.
 */
size_t
snfmt(char *buf, size_t bufsz, const char *fmt, ...)
{
	va_list ap;
	size_t len;

	va_start(ap, fmt);
	len = snfmt_va(buf, bufsz, fmt, ap);
	va_end(ap);
	return len;
}

size_t
snfmt_va(char *buf, size_t bufsz, const char *fmt, va_list ap)
{
	struct snfmt_ctx ctx;
	struct snfmt_func *f;
	char name[SNFMT_NAMEMAX];
	union snfmt_arg arg[SNFMT_NARG];
	char *p = buf, *end = buf + bufsz;
	size_t n;
	int c;

	va_copy(ctx.ap, ap);
	ctx.fmt = fmt;

	while ((c = *ctx.fmt++) != 0) {

		/* number of available chars in the output buffer */
		n = (p < end) ? end - p : 0;

		switch (c) {
		case '{':
			f = snfmt_scanfunc(&ctx, arg);
			if (f == NULL)
				goto copy;
			p += f->func(p, n, arg);
			break;
		case '%':
			if (ctx.fmt[0] == '%') {
				c = *ctx.fmt++;
				goto copy;
			}
			name[0] = '%';
			name[1] = snfmt_scanpct(&ctx, arg);
			name[2] = 0;
			f = snfmt_getfunc(name);
			if (f)
				p += f->func(p, n, arg);
			else {
				switch (name[1]) {
				case 'c':
					p += snprintf(p, n, "%c", (int)arg->i);
					break;
				case 'd':
					p += snprintf(p, n, "%lld", arg->i);
					break;
				case 'u':
					p += snprintf(p, n, "%llu", arg->u);
					break;
				case 'x':
					p += snprintf(p, n, "%llx", arg->i);
					break;
				case 'o':
					p += snprintf(p, n, "%llo", arg->i);
					break;
				case 'a':
					p += snprintf(p, n, "%a", arg->f);
					break;
				case 'e':
					p += snprintf(p, n, "%e", arg->f);
					break;
				case 'f':
					p += snprintf(p, n, "%f", arg->f);
					break;
				case 'g':
					p += snprintf(p, n, "%g", arg->f);
					break;
				case 'p':
					p += snprintf(p, n, "%p", arg->p);
					break;
				case 's':
					p += snprintf(p, n, "%s", arg->s);
				}
			}
			break;
		default:
		copy:
			if (n > 0)
				*p = c;
			p++;
		}
	}

	/*
	 * add terminating '\0' only if there's enough space
	 */
	if (bufsz > 0)
		*((p < end) ? p : buf + bufsz - 1) = 0;

	va_end(ctx.ap);
	return p - buf;
}
