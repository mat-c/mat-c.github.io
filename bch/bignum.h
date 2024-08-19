/* 
   Copyright 2010 Matthieu castet <castet.matthieu@free.fr>

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <assert.h>
#include <stdio.h>

#if 1
typedef unsigned long reg_t;

#define REG_SIZE (sizeof(reg_t)*8)
#define BG_SPLIT ((BG_SIZE+REG_SIZE-1)/REG_SIZE)
typedef struct {
	reg_t d[BG_SPLIT];
} bg2_t;

#define REG_OFFSET (REG_SIZE*BG_SPLIT - BG_SIZE)

#define BG2_INIT(a) { .d = {((reg_t)(a)) << REG_OFFSET, } }

/* ^ */
static inline bg2_t bg2_xor(bg2_t a, bg2_t b)
{
	unsigned int i;
	for (i = 0; i < BG_SPLIT; i++)
		a.d[i] = a.d[i] ^ b.d[i];
	return a;
}

static inline bg2_t bg2_xor5(bg2_t a, bg2_t b, bg2_t c, bg2_t d, bg2_t e)
{
	unsigned int i;
	for (i = 0; i < BG_SPLIT; i++)
		a.d[i] = a.d[i] ^ b.d[i] ^ c.d[i] ^ d.d[i] ^ e.d[i];
	return a;
}

static inline bg2_t bg2_xor51(bg2_t a, bg2_t b, bg2_t c, bg2_t d, bg2_t e)
{
	int i;
	for (i = BG_SPLIT - 1; i >= 1; i--)
		a.d[i] = a.d[i-1] ^ b.d[i] ^ c.d[i] ^ d.d[i] ^ e.d[i];
	a.d[0] = b.d[0] ^ c.d[0] ^ d.d[0] ^ e.d[0];
	return a;
}

/* | */
static inline bg2_t bg2_or_low(bg2_t a, reg_t low)
{
	a.d[0] |= low << REG_OFFSET;
	return a;
}

#if 0
/* & */
static inline bg2_t bg2_mask_high(bg2_t a)
{
	if (BG_SIZE%REG_SIZE)
		a.d[BG_SPLIT-1] &= (1 << (BG_SIZE%REG_SIZE)) - 1;
	return a;
}
#endif

static inline reg_t bg2_get_bit(bg2_t a, unsigned int bit)
{
	bit += REG_OFFSET;
	/* XXX check compiler generate mask and shift */
	int idx = bit / REG_SIZE;
	int offset = bit % REG_SIZE;
	assert(idx < (int)BG_SPLIT);
	return (a.d[idx] & (1UL<<offset)) >> offset;
}

/* << */
static inline bg2_t bg2_lsl(bg2_t a, unsigned int s)
{
	int i;
	assert(s < REG_SIZE);
	for (i = BG_SPLIT -1; i >= 1; i--) {
		reg_t low = a.d[i-1] >> (REG_SIZE-s);
		a.d[i] = (a.d[i] << s) | low;
	}
	a.d[0] = a.d[0] << s;
	return a;
}

static inline bg2_t bg2_lsl_big(bg2_t a, unsigned int s)
{
	int idx = s / REG_SIZE;
	int offset = s % REG_SIZE;

	if (idx) {
		int i;
		assert(idx!=0);
		assert(idx < (int)BG_SPLIT);
		for (i = BG_SPLIT -1; i >= idx; i--) {
			a.d[i] = a.d[i-idx];
		}
		for (; i >=0; i--) {
			a.d[i] = 0;
		}
	}
	if (offset)
		a = bg2_lsl(a, offset);
	return a;
}

static inline reg_t bg2_lsr_high(bg2_t a, unsigned int s)
{
	s += REG_OFFSET;
	assert(s / REG_SIZE == BG_SPLIT - 1);
	s = s % REG_SIZE; 
	return a.d[BG_SPLIT-1] >> s;
}

#if 0
static inline reg_t bg2_lsr_high_big(bg2_t a, unsigned int s)
{
	assert(0);
	if (s / REG_SIZE == BG_SPLIT - 1)
		return bg2_lsr_high(a, s);
	a = bg2_lsl(a, (BG_SPLIT - 1) * REG_SIZE - s);
	return a.d[BG_SPLIT-1];
}
#endif

static inline reg_t bg2_null(bg2_t a)
{
	unsigned int i;
	for (i = 0; i < BG_SPLIT; i++) {
		if (a.d[i])
			return 0;
	}
	return 1;
}

static inline void bg2_dump(bg2_t a)
{
	int i;
	for (i = BG_SPLIT -1; i >= 0; i--) {
		printf("%lx ", a.d[i]);
	}
	printf("\n");
}

static inline void bg2_to_mem(char *m, bg2_t a)
{
	int i;
	assert(sizeof(a) != BG_SPLIT * REG_SIZE);
	for (i = BG_SPLIT - 1; i >= 0; i--) {
		reg_t word = a.d[i];
		int byte;
		for (byte = 3; byte >=0 && i < BG_SIZE/8; byte--) {
			*m = word >> 24;
			word <<= 8;
			m++;
			if (i == 0 && byte*8 < REG_OFFSET)
				break;
		}
	}
}

static inline bg2_t bg2_from_mem(const char *m)
{
	int i;
	bg2_t a;
	for (i = BG_SPLIT - 1; i >=0;i--) {
		reg_t word = 0;
		int byte;
		for (byte = 3; byte >= 0 && i >= 0; byte--) {
			word <<= 8;
			if (i == 0 && (byte+1)*8 <= REG_OFFSET)
				continue;
			word |= *m & 0xff;
			m++;
		}
		a.d[i] = word;
	}
	return a;
}

#else
typedef unsigned long reg_t;
#define REG_SIZE (sizeof(reg_t)*8)
#if BG_SIZE <= 32
typedef unsigned long bg2_t;
#elif BG_SIZE <= 64
typedef unsigned long long bg2_t;
#else
#error not supported
#endif
#define REG_OFFSET (sizeof(bg2_t)*8 - BG_SIZE)

#define BG2_INIT(a) ((a)<<REG_OFFSET)

/* ^ */
static inline bg2_t bg2_xor(bg2_t a, bg2_t b)
{
	return a ^ b;
}

/* | */
static inline bg2_t bg2_or_low(bg2_t a, reg_t low)
{
	return a | (low << REG_OFFSET);
}

static inline reg_t bg2_get_bit(bg2_t a, unsigned int bit)
{
	bit += REG_OFFSET;
	return !!(a & (1ULL<<bit));
}

/* << */
static inline bg2_t bg2_lsl(bg2_t a, unsigned int s)
{
	assert(s < REG_SIZE);
	return a << s;
}

static inline bg2_t bg2_lsl_big(bg2_t a, unsigned int s)
{
	return a << s;
}

static inline reg_t bg2_lsr_high(bg2_t a, unsigned int s)
{
	s += REG_OFFSET;
	return a >> s;
}

#if 0
static inline reg_t bg2_lsr_high_big(bg2_t a, unsigned int s)
{
	return a >> s;
}
#endif

static inline reg_t bg2_null(bg2_t a)
{
	return a == 0;
}

static inline void bg2_dump(bg2_t a)
{
	printf("%llx\n", a);
}

static inline void bg2_to_mem(char *m, bg2_t a)
{
	memcpy(m, &a, sizeof(a));
}

static inline bg2_t bg2_from_mem(const char *m)
{
	bg2_t a;
	memcpy(&a, m, sizeof(a));
	return a;
}
#endif
