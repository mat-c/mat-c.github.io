#define POLY_LSB 0
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
#define POLY_ORDER 13
#define T 4
//#define T 8
//#define T 1
//#define T 2
//#define T 16

/* speed size tunning */
/* encoding : 
   table size = POLY_ORDER*T*(2^(BIG_TABLE_ORDER)-1)*BIG_TABLE_SPLIT
 */
#define BIG_TABLE_ORDER 8
#define BIG_TABLE_SPLIT 4

//#define COMPAT_TABLE
/* error correction
   table size = POLY_ORDER*(2^(TABLE_ORDER)-1)*k
 */
#if T <= 5
/* full table for simba and chien */
#define TABLE_ORDER (2*T-1)
#elif T <= 8
/* full table for chien */
#define TABLE_ORDER T
#else
#define TABLE_ORDER 8
//#error no supported
#endif

/* do not change anything after this */

#define BIG_POLY_ORDER (POLY_ORDER*T)
#define BG_SIZE BIG_POLY_ORDER
#include "bignum.h"


void bch_enc_init(void);
bg2_t bch_enc(const char *p, int len);

void bch_correct_init(int len);
int bch_correct(char *p, bg2_t org_poly, bg2_t cal_poly);

#include <time.h>
#define dprintf(x...) printf(x)
//#define dprintf(x...)
#define STAT3 0


