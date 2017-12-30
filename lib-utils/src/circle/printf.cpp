/**
 * @file printf.c
 *
 */
/* Copyright (C) 2017 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <circle/logger.h>
#include <circle/stdarg.h>
#include <circle/types.h>
#include <circle/util.h>

#include <assert.h>

extern "C" {

void printf(const char *fmt, ...) {
	assert(fmt != 0);

	size_t fmtlen = strlen(fmt);
	char fmtbuf[fmtlen + 1];

	strcpy(fmtbuf, fmt);

	if (fmtbuf[fmtlen - 1] == '\n') {
		fmtbuf[fmtlen - 1] = '\0';
	}

	va_list var;
	va_start(var, fmt);

	CLogger::Get()->WriteV("", LogNotice, fmtbuf, var);

	va_end(var);
}

}
