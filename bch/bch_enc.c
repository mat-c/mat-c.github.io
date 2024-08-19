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
#include "bch.h"

#define N(x) ((unsigned char)(~x))
#if BIG_POLY_ORDER == 13
//0x201B
static const char poly[] = {0x00, 0xd8};
//f4700000
static const char crc_end[] = {N(0xf4), N(0x70)};
#elif BIG_POLY_ORDER == 26
//0x4D5154B
static const char poly[] = {0x35, 0x45, 0x52, 0xc0};
//dfac200
static const char crc_end[] = {0x00, 0x00, 0x00, 0x00};

#elif BIG_POLY_ORDER == 52
//0x14523043AB86ABULL
static const char poly[] = {0x45, 0x23, 0x04, 0x3A, 0xB8, 0x6A, 0xB0};
static const char crc_end[] = {N(0xd7), N(0xec), N(0x33), N(0xc6), N(0x69), N(0x53), N(0x8f)};
#elif BIG_POLY_ORDER == 104
//#define BIG_POLY_ 0x115F914E07B0C138741C5C4FB23
static const char poly[] = {0x15, 0xF9, 0x14, 0xE0, 0x7B, 0X0C, 0x13, 0x87, 0X41, 0xC5, 0xC4, 0xFB, 0x23};
//10 aed1f612 6c653d68 861adb4a
static const char crc_end[] = {N(0x10), N(0xae), N(0xd1), N(0xf6), N(0x12), N(0x6c), N(0x65), N(0x3d), N(0x68), N(0x86), N(0x1a), N(0xdb), N(0x4a)};
#elif BIG_POLY_ORDER == 208
//1CBBE 3F0DBEC5 63B5FB20 FF07F7AA 45FF026F B378A601 CDD0FDD1
static const char poly[] = {0xCB, 0xBE, 0x3F, 0x0D, 0xBE, 0xC5, 0x63, 0xB5, 0xFB, 0x20, 0xFF, 0x07, 0xF7, 0xAA, 0x45, 0xFF, 0x02, 0x6F, 0xB3, 0x78, 0xA6, 0x01, 0xCD, 0xD0, 0xFD, 0xD1};
static const char crc_end[BIG_POLY_ORDER/8];

#else
#error not supported
#endif


/* on x86 or is faster than xor ... */
//#define CRC_OR

static bg2_t BIG_POLY2;
static bg2_t CRC_END;
static const bg2_t CRC_INIT = BG2_INIT(0);

#define BIG_TABLE_SIZE (BIG_TABLE_ORDER?1<<(BIG_TABLE_ORDER):0)
#define BIG_TABLE_MASK (BIG_TABLE_SIZE-1)

static bg2_t big_poly_table[BIG_TABLE_SPLIT][BIG_TABLE_SIZE];
static void big_poly_gen_(int split)
{
	int i;
	for (i=0; i < BIG_TABLE_SIZE; i++) {
		bg2_t pre = CRC_INIT;
		int bit;
		reg_t byte = i << (split*BIG_TABLE_ORDER);
		for (bit = (split+1)*BIG_TABLE_ORDER-1; bit >= 0 ; bit--) {
			pre = bg2_lsl(pre, 1);
			if (byte & (1<<((split+1)*BIG_TABLE_ORDER-1))) {
				pre = bg2_xor(pre, BIG_POLY2);
				byte <<= 1;
				byte ^= bg2_lsr_high(BIG_POLY2, (BIG_POLY_ORDER-(split+1)*BIG_TABLE_ORDER));
			}
			else
				byte <<= 1;
		}
		//printf("pre[%d]=%x\n", i, pre);

		big_poly_table[split][i] = pre;
	}
}

static void big_poly_gen(void)
{
	int i;
	for (i = 0; i < BIG_TABLE_SPLIT; i++) {
		big_poly_gen_(i);
	}
}

static bg2_t crc_simple_p(bg2_t crc, const unsigned char *p, int len)
{
	while (len >= BIG_TABLE_SPLIT) {
		reg_t crc2 = bg2_lsr_high(crc, BIG_POLY_ORDER-BIG_TABLE_ORDER*BIG_TABLE_SPLIT); 
#if BIG_TABLE_SPLIT == 1
		break;
#elif BIG_TABLE_SPLIT == 2
		crc2 ^= (*p<<BIG_TABLE_ORDER)|*(p+1);
		//crc2 ^= *((unsigned short*)p);
		crc = bg2_xor(
				bg2_lsl(crc, BIG_TABLE_ORDER*BIG_TABLE_SPLIT),
				bg2_xor(
		    		big_poly_table[1][crc2>>8],
		    		big_poly_table[0][crc2&BIG_TABLE_MASK]));

		p+=2;
		len-=2;;
#elif BIG_TABLE_SPLIT == 4
		crc2 ^= (*p<<(BIG_TABLE_ORDER*3))|(*(p+1)<<(BIG_TABLE_ORDER*2))|(*(p+2)<<(BIG_TABLE_ORDER*1))|*(p+3);
		//crc2 ^= *((unsigned int*)p);
#if 0
		crc = bg2_xor(
				bg2_lsl_big(crc, BIG_TABLE_ORDER*BIG_TABLE_SPLIT),
				bg2_xor(
					bg2_xor(
		    			big_poly_table[3][crc2>>24],
		    			big_poly_table[2][(crc2>>16)&BIG_TABLE_MASK])
					,
					bg2_xor(
		    			big_poly_table[1][(crc2>>8)&BIG_TABLE_MASK],
		    			big_poly_table[0][crc2&BIG_TABLE_MASK])
			));
#elif 1
		crc = bg2_xor5(
				bg2_lsl_big(crc, BIG_TABLE_ORDER*BIG_TABLE_SPLIT),
		    		big_poly_table[3][crc2>>24],
		    		big_poly_table[2][(crc2>>16)&BIG_TABLE_MASK],
		    		big_poly_table[1][(crc2>>8)&BIG_TABLE_MASK],
		    		big_poly_table[0][crc2&BIG_TABLE_MASK]
			);
#else
		crc = bg2_xor51(
				crc,
		    		big_poly_table[3][crc2>>24],
		    		big_poly_table[2][(crc2>>16)&BIG_TABLE_MASK],
		    		big_poly_table[1][(crc2>>8)&BIG_TABLE_MASK],
		    		big_poly_table[0][crc2&BIG_TABLE_MASK]
			);

#endif

			p+=4;
			len-=4;
#else
#error no supported
#endif
	}
	while (len > 0) {
#ifdef CRC_OR
		reg_t crc2 = bg2_lsr_high(crc, BIG_POLY_ORDER-BIG_TABLE_ORDER*BIG_TABLE_SPLIT); 
		crc = bg2_xor(bg2_or_low(bg2_lsl(crc, BIG_TABLE_ORDER), *p), big_poly_table[0][crc2]);
#else
		reg_t crc2 = bg2_lsr_high(crc, BIG_POLY_ORDER-BIG_TABLE_ORDER*BIG_TABLE_SPLIT) ^ *p; 
		crc = bg2_xor(bg2_lsl(crc, BIG_TABLE_ORDER), big_poly_table[0][crc2]);
#endif
		len--;
		p++;
	}
#ifdef CRC_OR
	/* 0 pad */
	len = BIG_POLY_ORDER;
	while (len >= BIG_TABLE_ORDER) {
		crc = bg2_xor(bg2_lsl(crc, BIG_TABLE_ORDER), big_poly_table[0][bg2_lsr_high(crc, BIG_POLY_ORDER-BIG_TABLE_ORDER)]);
		len -= BIG_TABLE_ORDER;
	}
	if (len) {
		crc = bg2_xor(bg2_lsl(crc, len), big_poly_table[0][bg2_lsr_high(crc, BIG_POLY_ORDER-len)]);
	}
#endif

	/* make erased page happy */
	return bg2_xor(crc, CRC_END);
}

static bg2_t crc_simple_s(bg2_t crc, const unsigned char *p, int len)
{
	while (len > 0) {
		int i = 8;
		unsigned int byte = *p;
		while (i--) {
			int over = bg2_get_bit(crc, BIG_POLY_ORDER-1);
			crc = bg2_lsl(crc, 1);
			crc = bg2_or_low(crc,  (byte>>7));
			if (over) {
				crc = bg2_xor(crc, BIG_POLY2);
			}
			byte = (byte << 1) & 0xff;
		}
		p++;
		len--;
	}

	len = BIG_POLY_ORDER;
	while (len > 0) {
		int over = bg2_get_bit(crc, BIG_POLY_ORDER-1);
		crc = bg2_lsl(crc, 1);
		if (over) {
			crc = bg2_xor(crc, BIG_POLY2);
		}
		len--;
	}

	return bg2_xor(crc, CRC_END);
}

void bch_enc_init(void)
{
	BIG_POLY2 = bg2_from_mem(poly);
	CRC_END = bg2_from_mem(crc_end);
	bg2_dump(BIG_POLY2);
	big_poly_gen();
}
/* TODO build a parity bit to detect T+1 error */
bg2_t bch_enc(const char *p, int len)
{
	const unsigned char *up = (const unsigned char *)p;
	struct timespec ts1, ts2;
	bg2_t ret;
	if (STAT3) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts1);
	if (BIG_TABLE_ORDER == 0)
		ret = crc_simple_s(CRC_INIT, up, len);
	else if (BIG_TABLE_ORDER == 8)
		ret = crc_simple_p(CRC_INIT, up, len);
	else
		while(1);
	if (STAT3) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts2);
	if (STAT3) printf("enc %dns\n", (int)(ts2.tv_nsec-ts1.tv_nsec));
	return ret;
	return crc_simple_s(CRC_INIT, up, len);
}
