/*
 * Copyright (c) 2008 Matthieu CASTET <castet.matthieu@free.fr
 *
 * This software is available to you under a choice of one of two
 * licenses. You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "exp.h"


EXP_TLS sigjmp_buf exp_data;
#define BACKTRACE 1

#ifdef BACKTRACE
/* need glibc and -rdynamic CFLAGS */
#include <execinfo.h>
static EXP_TLS void *backtrace_buffer[100];
static EXP_TLS int backtrace_nptrs;

/* XXX doesn't work when in signal ...
 * look what gdb does ???
 * use -fasynchronous-unwind-tables ???
 */
static void backtrace_get()
{
	backtrace_nptrs = backtrace(backtrace_buffer, sizeof(backtrace_buffer));
}

static void backtrace_print()
{
	char **strings;
	strings = backtrace_symbols(backtrace_buffer, backtrace_nptrs);
	if (strings) {
		int j;
		for (j = 0; j < backtrace_nptrs; j++)
			printf("%s\n", strings[j]);
	}
	//system("cat /proc/self/maps");
	free(strings);
}
#else
static void backtrace_print() {}
static void backtrace_get() {}
#endif

/* XXX if siglongjmp is not implemented, we can do a trivial emulation */

void throw_exp_int(int value, int nobacktrace)
{
	void *data = exp_data;
	assert(value != 0);
	/* check if it is the exp_data is not the one in bss (set to 0)*/
	/* first do a quick test by testing the first bytes */
	if (*((int*)exp_data) == 0) {
		int i;
		/* printf("may be last\n"); */
		/* test the remaning bytes */
		for (i = sizeof(int*);
				i < sizeof(exp_data) && ((char *)data)[i] == 0;
				i++);
		if (i == sizeof(exp_data)) {
			printf("Not handled expection %d\n", value);
			backtrace_print();
			abort();
		}
	}
	if (nobacktrace == 0)
		backtrace_get();
	siglongjmp(exp_data, value);
}


void throw_exp(int value)
{
	throw_exp_int(value, 0);
}
