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

#ifndef EXP_H
#define EXP_H 1
#include <setjmp.h>
#include <string.h>

/* XXX check for gcc support */
#define EXP_TLS __thread

extern EXP_TLS sigjmp_buf exp_data;
extern void throw_exp(int value);
extern void throw_exp_int(int value, int nobacktrace);
/* XXX talk about our local variable that could conflict :
 * exp_ret, exp_data_copy, end_exp
 * prefix them...
 */

#define BEGIN_EXP \
	do { \
		int exp_ret; \
		/* save the current exp_data */ \
		sigjmp_buf exp_data_copy; \
		memcpy(exp_data_copy, exp_data, sizeof(exp_data)); \
		/* set current exp_data */ \
		exp_ret = sigsetjmp(exp_data, 1); \
		if (exp_ret == 0) /* XXX what to do to catch return ... */

#if 1
#define SWITCH_EXP(exp_variable) \
		/* restore old exp_data */ \
		memcpy(exp_data, exp_data_copy, sizeof(exp_data)); \
		if (exp_ret) { \
			__label__ end_exp; /* XXX local label is not portable */\
			int exp_variable = exp_ret; \
			switch(exp_variable) \


#define END_EXP \
			/* propagate exception */ \
			throw_exp_int(exp_ret, 1); \
			goto end_exp; /* avoid warning when there is no BREAK_EXP */ \
end_exp: \
			(void)0; \
		} /* end if (ret) */ \
	} while(0)

#define BREAK_EXP \
	goto end_exp
#else
#define SWITCH_EXP(exp_variable) \
		/* restore old exp_data */ \
		memcpy(exp_data, exp_data_copy, sizeof(exp_data)); \
		if (exp_ret) { \
			int end_exp = 0; \
			int exp_variable = exp_ret; \
			switch(exp_variable) \


#define END_EXP \
			/* propagate exception */ \
			if (end_exp) \
				throw_exp_int(exp_ret, 0); \
		} /* end if (ret) */ \
	} while(0)

#define WHEN_EXP(val) \
	case val: \
	end_exp = 1
#endif

#endif
