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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bch.h"

int main()
{
	bg2_t org_poly;
	char *p = malloc(512+sizeof(bg2_t));
	int i;
	bch_enc_init();
	bch_correct_init(512);
	memset(p, 0xff, 512);
#if 0
	org_poly = bch_enc(p, 512);
	bg2_dump(org_poly);
	for (i=0; i<1000000; i++) {
		bch_enc(p, 512);
	}
	return 0;
#endif
	org_poly = bch_enc(p, 512);
	printf("original crc ");
	bg2_dump(org_poly);
	for (i=0; i<512+(int)sizeof(bg2_t); i++) {
		int num;
		for (num = 1; num <= 4; num++) {
		int j;
		for (j=8-num; j>=0; j--) {
			bg2_t synd_poly;
			char b = p[i];
			int pat = (1<<num) - 1;
			dprintf("==%db error at (%d,%d)\n", num, i, j);
			bg2_to_mem(p+512, org_poly);
			p[i] ^= (pat<<j);
			synd_poly = bch_enc(p, 512);
			if (bch_correct(p, bg2_from_mem(p+512), synd_poly) < 0) {
				exit(1);
			}
			if (i < 512)
				assert(p[i] == b);
		}
		}
	}
	return 0;

	bg2_to_mem(p+512, org_poly);
	//for (i=0; i<512*8; i++) {
	while(1) {
		int n_err = rand() % (T+2);
		int j;
		int res;
		int ecc_err = 0;
		bg2_t synd_poly;
		printf("==random %d ", n_err);
		for (j=0; j<n_err; j++) {
			int pos = rand() % (512+sizeof(bg2_t));
			int bit = rand() % 8;
			printf("(%d,%d) ", pos, bit); 
			p[pos] ^= (1<<bit);
			if (pos >= 512)
				ecc_err = 1;
		}
		printf("\n");
		synd_poly = bch_enc(p, 512);
		res = bch_correct(p, bg2_from_mem(p+512), synd_poly);
		if (res < 0 && n_err <= T) {
			exit(1);
		}
		/* restore ecc */
		if (res > 0)
			bg2_to_mem(p+512, org_poly);
		if (res != n_err && !ecc_err) {
			printf("double flip ?? %d %d\n", res, n_err);
		}
		if (n_err > T) {
			memset(p, 0xff,512);
			bg2_to_mem(p+512, org_poly);
		}

	}
	printf("done\n");

	return 0;
}
