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

#ifndef SNFMT_H
#define SNFMT_H

#include <stdarg.h>
#include <stdio.h>

union snfmt_arg {
	long long i;
	unsigned long long u;
	double f;
	const char *s;
	void *p;
};

#ifdef __cplusplus
extern "C" {
#endif

size_t snfmt(char *, size_t, const char *, ...);
size_t snfmt_va(char *, size_t, const char *, va_list);
void snfmt_addfunc(size_t (*)(char *, size_t, union snfmt_arg *), const char *);
void snfmt_rmfunc(size_t (*)(char *, size_t, union snfmt_arg *));

#ifdef __cplusplus
}
#endif

/*
 * MSVC has no attribute to enable argument checks of printf-style
 * functions. However it checks calls to the real snprintf(). So, we add an
 * unreachable call to snprintf() on which MSVC performs the checks.
 */
#define snfmt(p, n, ...) \
	(0 ? (size_t)snprintf((p), (n), __VA_ARGS__) : snfmt((p), (n), __VA_ARGS__))

#endif
