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
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "snfmt.h"

/*
 * convert binary blob
 */
static size_t hexdump_fmt(char *buf, size_t size, unsigned char *blob, size_t blob_size)
{
	char *p = buf, *end = buf + size;
	const char *sep = "";
	int i;

	for (i = 0; i < blob_size; i++) {
		p += snprintf(p, p < end ? end - p : 0, "%s%02x", sep, blob[i]);
		sep = ", ";
	}

	return p - buf;
}

/*
 * same as %c, but with unicode allowed
 */
static size_t uchar_fmt(char *buf, size_t bufsz, int c)
{
	char ustr[8], *p = ustr;

	if ((c > 0xd7ff && c < 0xe000) || c > 0x10ffff)
		return 0;

	if (c < 0x80) {
		*p++ = c;
	} else if (c < 0x800) {
		*p++ = 0xc0 | (c >> 6);
		*p++ = 0x80 | (c & 0x3f);
	} else if (c < 0x10000) {
		*p++ = 0xe0 | (c >> 12);
		*p++ = 0x80 | ((c >> 6) & 0x3f);
		*p++ = 0x80 | (c & 0x3f);
	} else {
		*p++ = 0xf0 | (c >> 18);
		*p++ = 0x80 | ((c >> 12) & 0x3f);
		*p++ = 0x80 | ((c >> 6) & 0x3f);
		*p++ = 0x80 | (c & 0x3f);
	}
	*p++ = 0;
	return snprintf(buf, bufsz, "%s", ustr);
}

static size_t fmt_cb(char *buf, size_t size, const char *fmt, union snfmt_arg *args)
{
	if (strcmp(fmt, "hexdump:%p,%u") == 0)
		return hexdump_fmt(buf, size, args[0].p, args[1].u);
	if (strcmp(fmt, "%c") == 0)
		return uchar_fmt(buf, size, args[0].i);
	return 0;
}

/*
 * log on stderr using snfmt()
 */
static void logx(const char *fmt, ...)
{
	va_list ap;
	char buf[64];

	va_start(ap, fmt);
	snfmt_va(fmt_cb, buf, sizeof(buf), fmt, ap);
	va_end(ap);

	fprintf(stderr, "%s\n", buf);
}

int main(void)
{
	unsigned char blob[] = {0xaa, 0xbb, 0xcc, 0xcc};

	/* prints: <Hello world!>, 10, 0xa */
	logx("<%s>, %03d, 0x%02x 0%04o %.15g", "Hello world!", 10, 10, 10, 1. / 7);

	/* handling of *-based width and precision */
	logx("%0*.*g", 15, 3, 1. / 7);

	/* prints: aa, bb, cc, cc */
	logx("blob: {hexdump:%p,%zu}", blob, sizeof(blob));

	/* unicode chars */
	logx("chars: %% (pct), %c (pi), %c (m acute), %c (chess queen)", 0x3c0, 0x1e3f, 0x1fa01);

	/* truncated string */
	logx("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

	/* unknown tag */
	logx("{unknown}, {unknown:%s}", "hello!");


	/* overflow */
	logx("oveflow %000000000000000000000000000000d, %d", 1, 123);

	/* bad */
	logx("bad fmt: %y, {hexdump:%y} %d", 123);

	return 0;
}
