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

#include <string.h>
#include <stdio.h>
#include "bch.h"

#if POLY_ORDER == 13
#define POLY_ 0x201BUL
#define POLY_INV POLY_INT(POLY_>>1)
#else
#error no supported
#endif

#define POLY_INT(x) (x)

#if T <= TABLE_ORDER
#define USE_CGFM_NEG 0
#else
#define USE_CGFM_NEG 1
#if T/2 > TABLE_ORDER
#warning slow chien
#endif
#endif


#define POLY_MASK ((1UL<<(POLY_ORDER))-1)
#define POLY POLY_INT(POLY_ & POLY_MASK)
#define POLY_TOP (1UL<<(POLY_ORDER-1))
/* 
 * store a polynom where x^0 is LSB
 * for bch 32 bit should be enough (up to 4GB of data...)
 */
typedef unsigned int poly_t;

/* utils to generate table */
/* find alpha^(-1) from POLY_ :
   POLY_(alpha) = 0
   shift left POLY_ until there is underflow
   in this case we got POLY_INV(alpha) + alpha^(-1) = 0 
 */
static poly_t find_inv(void)
{
	poly_t poly = POLY_;
	while ((poly & 1) == 0) {
		poly >>= 1;
	}
	poly >>= 1;
	return poly;
}

#define TABLE_SIZE (TABLE_ORDER?1<<(TABLE_ORDER):0)
#define TABLE_SPLIT 1
#define TABLE_NEG_SIZE (USE_CGFM_NEG?TABLE_SIZE:0)

#ifndef COMPAT_TABLE
#define POLY_MASK_SH 0
static poly_t poly_table[TABLE_SPLIT][TABLE_SIZE];
static poly_t invpoly_table[TABLE_SPLIT][TABLE_NEG_SIZE];
#else
#warning using compat table
#define POLY_MASK_SH 16
static unsigned short poly_table[TABLE_SPLIT][TABLE_SIZE];
static unsigned short invpoly_table[TABLE_SPLIT][TABLE_NEG_SIZE];
#endif

static void poly_gen(int split)
{
	int i;
	for (i=0; i < TABLE_SIZE; i++) {
		poly_t pre = 0;
		int bit;
		poly_t byte = i << (split*TABLE_ORDER);
		for (bit = (split+1)*TABLE_ORDER-1; bit >= 0 ; bit--) {
			pre <<= 1;
			if (byte & (1<<((split+1)*TABLE_ORDER-1))) {
				pre ^= POLY;
				byte <<= 1;
				byte ^= POLY >> (POLY_ORDER-(split+1)*TABLE_ORDER);
			}
			else
				byte <<= 1;
		}
		//printf("pre[%d]=%x\n", i, pre);
		pre = pre & POLY_MASK;
		/* mask elimination */
		if (POLY_ORDER != sizeof(byte)*8) {
			byte = i << (split*TABLE_ORDER);
			byte <<= POLY_ORDER;
			pre |= byte;
		}
		poly_table[split][i] = pre;
	}
}

static void invpoly_gen(int split)
{
	int i;
	for (i=0; i < TABLE_NEG_SIZE; i++) {
		poly_t pre = 0;
		int bit;
		poly_t byte = i << (split*TABLE_ORDER);
		for (bit = (split+1)*TABLE_ORDER-1; bit >= 0 ; bit--) {
			pre >>= 1;
			if (byte & 1) {
				pre ^= POLY_INV;
				byte >>= 1;
				byte ^= POLY_INV;
			}
			else
				byte >>= 1;
		}
		//printf("pre[%d]=%x\n", i, pre);
		//pre = pre & POLY_MASK;
		/* mask elimination */
		invpoly_table[split][i] = pre;
	}
}

/* end utils */

/* lut version where exp <= TABLE_ORDER */
static inline poly_t cgfm_pf(poly_t y, unsigned int exp)
{
	return ((y << (exp+POLY_MASK_SH))>>POLY_MASK_SH) ^ poly_table[0][y>>(POLY_ORDER-exp)];
}

/* lut version */
static poly_t cgfm_p(poly_t y, unsigned int exp)
{
	while (exp > TABLE_ORDER) {
		y = cgfm_pf(y, TABLE_ORDER);
		/* no need for mask */
		exp -= TABLE_ORDER;
	}
	y = cgfm_pf(y, exp);
	return y;
}

/*
   compute y^(exp). Serial version without lut.
   we use the fact that 
   y^1 = (y << 1) ^ POLY if y << 1 overflow
   y^1 = (y << 1) if y << 1 doesn't overflow
 */
static poly_t cgfm_s(poly_t y, unsigned int exp)
{
	if (y == 0)
		return 0;
	for (;exp > 0; exp--) {
		poly_t over = (y & POLY_TOP);
		y = (y << 1);
		if (over) {
			/* if sizeof y == ORDER we can't use ME */
			if (sizeof(y) == POLY_ORDER)
				y = y ^ POLY;
			else
				y = y ^ (POLY|(1<<POLY_ORDER));
		}
	}
	return y;
}

/* exp is in [1,2*T-1] range for syndrome */
/* exp is in [1,T] range for chien */
/* exp is in [1,T/2] range for chien with USE_CGFM_NEG */
//#define cgfm(y, exp, m) cgfm_pd(y, exp, m)
static inline poly_t cgfm(poly_t y, unsigned int exp, int max)
{
	//no check for y==0, caller should make this unlikely
	//no special case for max=1, cgfm_pf is fastest
	if (TABLE_SIZE == 0)
		return cgfm_s(y, exp);
	if (max > TABLE_ORDER)
		return cgfm_p(y, exp);
	else
		return cgfm_pf(y, exp);
}

static inline poly_t cgfm_neg_pf(poly_t y, unsigned int exp)
{
	return (y >> exp) ^ (invpoly_table[0][(y & ((1<<exp)-1))<<(TABLE_ORDER-exp)]);
}

/* lut version */
static poly_t cgfm_neg_p(poly_t y, unsigned int exp)
{
	while (exp > TABLE_ORDER) {
		y = cgfm_neg_pf(y, TABLE_ORDER);
		/* no need for mask */
		exp -= TABLE_ORDER;
	}
	y = cgfm_neg_pf(y, exp);
	return y;
}

/*
   compute y^(-exp). Serial version without lut.
 */
static poly_t cgfm_neg_s(poly_t y, int exp)
{
	for (;exp > 0; exp--) {
		poly_t over = (y & POLY_INT(1));
		y = (y >> 1);
		if (over) {
			y = y ^ POLY_INV;
			//XXX mask if POLY_LSB is set
		}
	}
	return y;
}

static poly_t cgfm_neg(poly_t y, int exp, int max)
{
	assert(USE_CGFM_NEG == 1);
	//no check for y==0, caller should make this unlikely
	//no special case for max=1, cgfm_pf is fastest
	if (0)
		return cgfm_neg_s(y, exp);
	if (max > TABLE_ORDER)
		return cgfm_neg_p(y, exp);
	else
		return cgfm_neg_pf(y, exp);
}

/* 
   compute Y(x) * Z(x) = y(0)*Z(x) + x(y(1)*Z(x) + ... x(y(m-1)*Z(x)) ... ) 
 
   time = O(m)
 */
static poly_t ggfm(poly_t y, poly_t z)
{
	int ybit;
	poly_t yz;
	//XXX optimise special case ?
	if (y == 0 || z == 0)
		return 0;
	/* y(0)*Z(x) */
	yz = (y & POLY_INT(1)) ? z : 0;
	for (ybit = 1; ybit < POLY_ORDER; ybit++) {
		/* y(n) = y(n+1), n = 0..order */
		y >>= 1;
		/* Zb(x)=x*Zb(x)=x^(ybit)*Z(x) */
		z = cgfm(z, 1, 1);
		/* if y(ybit) is set add x^(ybit)*Z(x) */
		if (y & POLY_INT(1))
			yz ^= z;
	}
	return yz;
}

/*
   compute Y / Z

   time = O((2m-1)*m)
 */
static poly_t ggfd(poly_t y, poly_t z)
{
	int ybit;
	//XXX optimise special case ?
	if (y == 0 || z == 0)
		return 0;

	y = ggfm(y, y);
	for (ybit = 1; ybit < POLY_ORDER; ybit++) {
		y = ggfm(y, z);
		y = ggfm(y, y);
	}
	return y;
}

/*
   S(i) = s(0) + x^i(s(1) + x^i(s(2) + ... + x^i(s(mt-1)...))) i = 1...2t
   S(2i) = S(i) * S(i)

   time = O(2*m*t)
 */
static void synd_gen(bg2_t synd_poly, poly_t synd[2*T])
{
	int bit;
	int s_idx; /* i = s_idx + 1 */
	//XXX we could skip the leading 0 here, but this make
	//the loop not constant anymore...

	/* init S(i) = s(mt-1) */
	for (s_idx = 0; s_idx < 2*T; s_idx+=2) {
		int sbit = bg2_get_bit(synd_poly, BIG_POLY_ORDER-1);
		synd[s_idx] = POLY_INT(sbit);
	}
	for (bit = BIG_POLY_ORDER - 2; bit >= 0; bit--) {
		int sbit;
		sbit = bg2_get_bit(synd_poly, bit);
		for (s_idx = 0; s_idx < 2*T; s_idx+=2) {
			/* S_tmp(i) = x^i*S(i) */
			synd[s_idx] = cgfm(synd[s_idx], s_idx+1, 2*T-1);
			/* S(i) = s(bit) + x^i*S_tmp(i) */
			synd[s_idx] ^= POLY_INT(sbit);
		}
	}
	for (s_idx = 1; s_idx < 2*T; s_idx+=2) {
		synd[s_idx] = ggfm(synd[s_idx>>1], synd[s_idx>>1]);
	}
}

/* Berlekamp-Massey algorithm without inversion
 
   time = O(T*T*2)
 */
static int simba(const poly_t synd[2*T], poly_t lamda[T+1])
{
	poly_t beta[T+1];
	poly_t delta;
	int r, k;

	lamda[0] = POLY_INT(1);
	memset(lamda + 1, 0, T*sizeof(poly_t));
	beta[0] = POLY_INT(1);
	memset(beta + 1, 0, T*sizeof(poly_t));
	delta = POLY_INT(1);

	k = 0;

	for (r = 0; r < T; r++) {
		int i;
		poly_t gama;

		gama = 0;
		for (i = 0; i <= T; i++) {
			gama ^= ggfm(synd[2*r-i], lamda[i]);
			if (2*r-i == 0)
				break;
		}

		/* delta never null */
		if (gama != 0 && k >=0) {
			int i;
			/* gama not null */
			for (i = T; i > 0; i--) {
				lamda[i] = ggfm(delta, lamda[i]) ^ ggfm(gama, beta[i-1]);
				beta[i] = lamda[i-1];
			}
			lamda[0] = ggfm(delta, lamda[0]);
			delta = gama;
			k = -k;
		}
		else {
			for (i = T; i > 1; i--) {
				lamda[i] = ggfm(delta, lamda[i]) ^ ggfm(gama, beta[i-1]);
				beta[i] = beta[i-2];
			}
			lamda[1] = ggfm(delta, lamda[1]) ^ ggfm(gama, beta[0]);
			lamda[0] = ggfm(delta, lamda[0]);
			beta[1] = 0;

			//delta = delta;
			k = k + 1;
		}
		beta[0] = 0;
	}

	k = T-k;
	return k;
}

static struct {
	unsigned int data_size;
	poly_t pol_init[T];
} chien_state;

static void chien_init(int len)
{
	unsigned int data_size = len*8 + BIG_POLY_ORDER;
	unsigned int data_offset = ((1<<POLY_ORDER) - 1) - data_size;
	//int data_offset = 506*8-5;
	int i;
	if (data_size >= POLY_MASK || data_offset <= POLY_ORDER) {
		printf("size too big\n");
		return;
	}

	/* XXX we could make chien init independant of the block len, if we do a reverse chien.
	   But this means we always need cgfm_neg
	 */
	for (i = 1; i <= T; i++) {
		chien_state.pol_init[i-1] = cgfm(POLY_INT(POLY), i*data_offset-POLY_ORDER, (1<<POLY_ORDER) - 1);
	}
	chien_state.data_size = data_size;
}

/* return number of corrected error or -1 if uncorrectable */
static int chien(poly_t lamda[T+1], poly_t roots[T], int num_root_lamda)
{
	unsigned int data_size = chien_state.data_size;

	int error_cnt = 0;
	unsigned int i;
	for (i = 1; i <= num_root_lamda; i++) {
		lamda[i] = ggfm(lamda[i], chien_state.pol_init[i-1]);
		/* we could remove null lamda[i] were i < num_root_lamda,
		   but the probability is very low
		else if (num_root_lamda != 0)
			printf("null lamda\n");
		 */
	}
	//printf("nb error : %d\n", num_root_lamda);
	for (i = 0; i < data_size; i++) {
		poly_t sum;
		int j;
		/*
		 * compute A(alpha^i) = sum(lamda(j)*alpha^(i*j)), j=0..t
		 * and check if result is null
		 */
		if (USE_CGFM_NEG) {
			/*
			 * note that A(alpha^i)*alpha^(-H*i) =
			 *           sum(lamda(j)*alpha^((j-H)*i), j=0..H-1)
			 *         + lamda(H)
			 *         + sum(lamda(j)*alpha^((j-H)*i), j=H+1..t)
			 *         where 0 < H < t
			 *
			 * with H=t/2, this reduce the order of cgfm by
			 * supporting negative exponent
			 */
			sum = lamda[T/2];
			for (j = 0; j < T/2; j++) {
				lamda[j] = cgfm_neg(lamda[j], T/2 - j, T/2);
				sum ^= lamda[j];
				
				/* skip trailling 0 lamda */
				if (j >= num_root_lamda)
					break;
			}
			for (j = T/2 + 1; j <= T; j++) {
				lamda[T-j] = cgfm(lamda[T-j], T/2 - j, T/2);
				sum ^= lamda[T-j];

				/* skip trailling 0 lamda */
				if (j >= num_root_lamda)
					break;
			}
		}
		else {
			sum = lamda[0];
			for (j = 1; j <= T; j++) {
				/* lamda[j] = x^j lamda[j] */
				lamda[j] = cgfm(lamda[j], j, T);
				sum ^= lamda[j];
				/* skip trailling 0 lamda */
				if (j >= num_root_lamda)
					break;
			}
		}
		if (sum == 0) {
			roots[error_cnt] = i;
			error_cnt++;
			/* we found all error, not need to continue */
			if (num_root_lamda == error_cnt)
				break;
		}
	}
	if (num_root_lamda != error_cnt) {
		dprintf("decoding failed\n");
		return -1;
	}
	return num_root_lamda;
}

void bch_correct_init(int len)
{
	poly_gen(0);
	invpoly_gen(0);
	chien_init(len);

	printf("alpha^(ORDER)=%lx, %lx\n", POLY, POLY_INV);
}

/* find error position, store them in roots and return number of error */
int bch_correct_find(bg2_t org_poly, bg2_t cal_poly, poly_t roots[T])
{
	poly_t synd[2*T];
	poly_t lamda[T+1];
	int num_root_lamda;
	int ret;
	int i;

	struct timespec ts1, ts2;

	cal_poly = bg2_xor(org_poly, cal_poly);
	if (STAT3)
		bg2_dump(cal_poly);
	/* no error */
	if (bg2_null(cal_poly))
		return 0;
	if (STAT3) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts1);
	synd_gen(cal_poly, synd);
	if (STAT3) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts2);
	if (STAT3) printf("synd %dns\n", (int)(ts2.tv_nsec-ts1.tv_nsec));
	for (i = 1; i < 2*T; i+=2) {
		dprintf("synd %x %x\n", synd[i-1], synd[i]);
	}

	if (STAT3) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts1);
	num_root_lamda = simba(synd, lamda);
	if (STAT3) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts2);
	if (STAT3) printf("simba %dns\n", (int)(ts2.tv_nsec-ts1.tv_nsec));
	dprintf("nb error : %d\n", num_root_lamda);
	if (num_root_lamda == 0)
		return 0;
	if (num_root_lamda > T)
		return -1;
	for (i = 0; i <= num_root_lamda; i++) {
		dprintf("lamda %x\n", lamda[i]);
	}
	if (STAT3) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts1);
	ret = chien(lamda, roots, num_root_lamda);
	if (STAT3) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts2);
	if (STAT3) printf("chien %dns\n", (int)(ts2.tv_nsec-ts1.tv_nsec));

	return ret;
}

void bch_correct_data(poly_t roots[T], int error_cnt, char *data)
{
	int i;
	for (i = 0; i < error_cnt; i++) {
		unsigned int pos = roots[i];
		unsigned int data_size = chien_state.data_size;
		dprintf("error %d %d\n", pos>>3, 7-(pos&7));
		if (pos > data_size - BIG_POLY_ORDER)
			dprintf("error in ecc\n");
		else if (data)
			data[pos>>3] ^= 1 << (7-(pos&7));
	}
}

int bch_correct(char *data, bg2_t org_poly, bg2_t cal_poly)
{
	poly_t roots[T];
	int ret;
	ret = bch_correct_find(org_poly, cal_poly, roots);
	bch_correct_data(roots, ret, data);
	return ret;
}

