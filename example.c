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
#include "snfmt/snfmt.h"

/*
 * linked list element with simple data
 */
struct point {
	struct point *next;
	const char *name;
	int x, y;
};

/*
 * convert a point structure to a string
 */
size_t snfmt_point(char *buf, size_t size, union snfmt_arg *args)
{
	struct point *p = args[0].p;

	return snprintf(buf, size, "%s(%d,%d)", p->name, p->x, p->y);
}

/*
 * convert a linked list of point structures to string
 */
size_t snfmt_list(char *buf, size_t size, union snfmt_arg *args)
{
	char *s = buf, *end = buf + size;
	struct point *p;

	for (p = args[0].p; p != NULL; p = p->next)
		s += snfmt(s, (s >= end) ? 0 : end - s, "{point:%p} ", p);

	return s - buf;
}

/*
 * same as %c, but with unicode allowed
 */
size_t snfmt_uchar(char *buf, size_t bufsz, union snfmt_arg *args)
{
	char ustr[8], *p = ustr;
	int c = args[0].i;

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

/*
 * log on stderr using snfmt()
 */
void logx(const char *fmt, ...)
{
	va_list ap;
	char buf[64];

	va_start(ap, fmt);
	snfmt_va(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	fprintf(stderr, "%s\n", buf);
}

/*
 * create a linked list of point structures
 */
struct point *point_new(const char *name, int x, int y, struct point *next)
{
	struct point *p;

	p = malloc(sizeof(struct point));
	if (p == NULL) {
		perror("malloc");
		exit(1);
	}

	p->x = x;
	p->y = y;
	p->name = name;
	p->next = next;

	return p;
}

int main(void)
{
	struct point *point_list;

	/* register handlers */
	snfmt_addfunc(snfmt_point, "point:%p");
	snfmt_addfunc(snfmt_list, "list:%p");
	snfmt_addfunc(snfmt_uchar, "%c");

	/*
	 * create a simple linked list with 3 elements
	 */
	point_list = point_new("a", 1, 2,
	    point_new("c", 3, 4,
	    point_new("d", 5, 6,
	    NULL)));

	/* prints: <Hello world!>, 10, 0xa */
	logx("<%s>, %03d, 0x%02x 0%04o %.15g", "Hello world!", 10, 10, 10, 1. / 7);

	/* prints: the first point is a(1,2) */
	logx("the first point is {point:%p}", point_list);

	/* prints: the list of points is a(1,2) c(3,4) d(5,6) */
	logx("the list of points is {list:%p}", point_list);

	/* unicode chars */
	logx("chars: %c (pi), %c (m acute), %c (chess queen)", 0x3c0, 0x1e3f, 0x1fa01);

	/* truncated string */
	logx("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

	snfmt_rmfunc(snfmt_list);
	snfmt_rmfunc(snfmt_point);

	return 0;
}
