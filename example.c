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
#include <string.h>
#include "snfmt.h"

/*
 * convert the binary blob to string
 */
static size_t hexdump_fmt(char *buf, size_t size, unsigned char *blob, size_t blob_size)
{
	char *p = buf, *end = buf + size;
	const char *sep = "";
	size_t i;

	for (i = 0; i < blob_size; i++) {
		p += snprintf(p, p < end ? end - p : 0, "%s%02x", sep, blob[i]);
		sep = ", ";
	}

	return p - buf;
}

/*
 * add the 'hexdump' conversion to logx()
 */
static int debug_fmt(char *buf, size_t size, const char *fmt, union snfmt_arg *args)
{
	if (strcmp(fmt, "hexdump:%p,%u") == 0)
		return hexdump_fmt(buf, size, args[0].p, args[1].u);
	return -1;
}

/*
 * log on stderr using snfmt()
 */
static void debug(const char *fmt, ...)
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
	unsigned char blob[] = {0xaa, 0xbb, 0xcc, 0xcc};
	int x = 123;

	/* example including user-defined format */
	debug("x = %d, blob = [{hexdump:%p,%zu}]\n", x, blob, sizeof(blob));

	return 0;
}
