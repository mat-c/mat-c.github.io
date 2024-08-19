this is an optimised bch encoder/decoder with limited footprint (ROM, bootloader, ...).
You could select the size of the lookup table or even use a slow implementation without any table.
The code realy on heavy inlining for encoder (TODO make it better at 02).
The error correction is a bit slower at Os (2K of text) than O3 (6K of text).

For fast encoder/decoder with bigger RAM requirement see http://marc.info/?l=linux-kernel&m=129708790902029&w=2 .

bch computation is done on a galois field where each number
in [1,2^(m)-1] can be seen as alpha^i, i in [0,2^(m)-2]

add/sub is a xor


multiplication :
ggfm(y, z)=
zz = pow(log(z)+log(z)) = alpha^(iy) + alpha^(iz) = alpha^(iy+iz)
cgfm(y, iz) = pow(log(y)+iz)
this need pow and log LUT of size (2^(m)-1)*(m+7)/8 bytes 
ie 2*32KB for m=13

another way is too use the fact that
Y(x) * Z(x) = y(0)*Z(x) + x(y(1)*Z(x) + ... x(y(m-1)*Z(x)) ... ) */
and that x^(exp)*Z(x) is shifting Z to left and subtrating polynome 
if it overflow
This is similar to crc computation and can be computed with LUT :
(z << exp) ^ poly_table[0][z>>(POLY_ORDER-exp)];

YZ need m iteration to be computed but with no LUT.
If we now we log of Z, we can do direct computation (cgfm) from the LUT.

In pratice the max cgfm exp is 
- 2*T-1 for O(T) algo (simba)
- T or T/2 for O(2^(m)-1) algo (chien)

the ideal max size is ((2^(2*T-1)-1)*(m+7)/8 (ie 512B for T=4, 128KB for T=8)
the best (chien algo)  may size is ((2^(T)-1)*(m+7)/8 (ie 64B for T=4, 512B for T=8, 128KB for T=16)
the best (chien2 algo)  may size is 2*( ((2^(T/2)-1)*(m+7)/8 ) (ie 2*16B for T=4, 2*64B for T=8, 2*512B for T=16, 2*128KB for T=32)
in pratice we limit the LUT size to someting that fit in the cache, and do
n iteration.



Some ideas are based on paper from :
Efficient Software-Based Encoding and Decoding of BCH Codes
by Junho Cho and Wonyong Sung

More to come in a next version


06 December 2010 : first version
