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
#include <signal.h>

enum exp {
	TEST_SIGFPE=1,
	TEST_SIGSEGV,
	TEST_USER0,
};

#define EXP_NAME(exp) [exp] = #exp
char *exp_name [] = {
	EXP_NAME(TEST_SIGFPE),
	EXP_NAME(TEST_SIGSEGV),
	EXP_NAME(TEST_USER0)
};

/*#define DEFINITION_EXP TEST_EXP=2 */
#include "exp.h"

/* remove static to see it in the backtrace ... */
/*static*/ void signal_handler(int sig)
{
	/* don't work : we need to exit the signal, throw_exp make never exiting
	 * them
	 */
	switch(sig) {
		case SIGFPE:
			throw_exp(TEST_SIGFPE);
			break;
		case SIGSEGV:
			throw_exp(TEST_SIGSEGV);
			break;
	}
}

void user()
{
	BEGIN_EXP {
		throw_exp(TEST_USER0);
	}
	SWITCH_EXP(exp) {
		case TEST_USER0:
			printf("USER0 occurs\n");
			BREAK_EXP;
	} END_EXP;
}

void user_not_catched()
{
	BEGIN_EXP {
		throw_exp(TEST_USER0);
	}
	SWITCH_EXP(exp) {
	} END_EXP;
}

void div0()
{
	BEGIN_EXP {
		static int zero;
		printf("%d\n", 1/zero);
	}
	SWITCH_EXP(exp) {
		case TEST_SIGFPE:
			printf("div by 0\n");
			BREAK_EXP;
	} END_EXP;
}

void div0_not_catched()
{
	BEGIN_EXP {
		static int zero;
		printf("%d\n", 1/zero);
	}
	SWITCH_EXP(exp) {
	} END_EXP;
}


void invalid_mem()
{
	BEGIN_EXP {
		static char *null;
		printf("%s\n", null);
	}
	SWITCH_EXP(exp) {
		case TEST_SIGSEGV:
			printf("invalid_mem\n");
			BREAK_EXP;
	} END_EXP;
}

int main()
{
	signal(SIGFPE, signal_handler); 
	signal(SIGSEGV, signal_handler); 
	BEGIN_EXP {
		div0();
		invalid_mem();
		user();
		div0_not_catched();
	/* if there is no exception,
	 * the code should reach here, so no return, ... before
	 */
	}
	SWITCH_EXP(exp) {
		default:
			printf("default : %s(%d) occured\n", exp_name[exp], exp);
			BREAK_EXP;
	} END_EXP;

	BEGIN_EXP {
#if 1
		div0_not_catched();
#else
		user_not_catched();
#endif
	}
	SWITCH_EXP(exp) {
	} END_EXP;
	return 0;
}
