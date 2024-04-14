#pragma once

// https://www.rfc-editor.org/rfc/rfc1321
// to match reference implementation, make "+" in round_[1234] mod2^64 instead of mod2^32

#ifndef MD5_H_
#define MD5_H_
void md5(char *message, unsigned char digest[16]);
void md5_print(unsigned char digest[16]);
#endif // MD5_H_

#ifdef MD5_IMPLEMENTATION
#include <stdio.h>
#include <stdint.h>

typedef unsigned char MD5_BYTE; // 8-message_len_bits, big-endianess
typedef uint32_t MD5_WORD; // 32-bits, little-endianess, uint4 (i.e. 4xMD5_BYTE=MD5_WORD)

MD5_BYTE MD5_PADDING[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#define MD5_S11 7
#define MD5_S12 12
#define MD5_S13 17
#define MD5_S14 22
#define MD5_S21 5
#define MD5_S22 9
#define MD5_S23 14
#define MD5_S24 20
#define MD5_S31 4
#define MD5_S32 11
#define MD5_S33 16
#define MD5_S34 23
#define MD5_S41 6
#define MD5_S42 10
#define MD5_S43 15
#define MD5_S44 21

size_t md5_strlen(const unsigned char *s) {
	char *s_cpy = (char *)s;
	size_t n = 0;
	for (; (*s++) != '\0'; ++n) {
		;
	}
	return n;
}

void md5_memcpy(unsigned char *dest, unsigned char *src, size_t len) {
	for (int i=0; i<len; ++i) {
		dest[i] = src[i];
	}
}

void md5_memset(unsigned char *dest, unsigned char c, size_t len) {
	for (int i=0; i<len; ++i) {
		dest[i] = c;
	}
}

// expects: len(out) == 4*iters
// expects: len(in) == iters
void md5_encode(unsigned char *out, MD5_WORD *in, int iters) {
	for (int i=0; i<iters; ++i) {
		out[4*i+0] = (unsigned char)((in[i] >> (8*0)));
		out[4*i+1] = (unsigned char)((in[i] >> (8*1)));
		out[4*i+2] = (unsigned char)((in[i] >> (8*2)));
		out[4*i+3] = (unsigned char)((in[i] >> (8*3)));
	}
}

MD5_WORD md5_rot_left(MD5_WORD a, int s) {
	return (a << s) | (a >> (32-s));
}

MD5_WORD md5_f(MD5_WORD x, MD5_WORD y, MD5_WORD z) {
	return (((x) & (y)) | ((~x) & (z)));
}

MD5_WORD md5_g(MD5_WORD x, MD5_WORD y, MD5_WORD z) {
	return ((x) & (z)) | ((y) & (~z));
}

MD5_WORD md5_h(MD5_WORD x, MD5_WORD y, MD5_WORD z) {
	return (x) ^ (y) ^ (z);
}

MD5_WORD md5_i(MD5_WORD x, MD5_WORD y, MD5_WORD z) {
	return (y) ^ ((x) | (~z));
}

MD5_WORD round_1(MD5_WORD a, MD5_WORD b, MD5_WORD c, MD5_WORD d, MD5_WORD x_k, int s, MD5_WORD t_i) {
	return b + md5_rot_left(a + md5_f(b, c, d) + x_k + t_i, s);
}

MD5_WORD round_2(MD5_WORD a, MD5_WORD b, MD5_WORD c, MD5_WORD d, MD5_WORD x_k, int s, MD5_WORD t_i) {
	return b + md5_rot_left(a + md5_g(b, c, d) + x_k + t_i, s);
}

MD5_WORD round_3(MD5_WORD a, MD5_WORD b, MD5_WORD c, MD5_WORD d, MD5_WORD x_k, int s, MD5_WORD t_i) {
	return b + md5_rot_left(a + md5_h(b, c, d) + x_k + t_i, s);
}

MD5_WORD round_4(MD5_WORD a, MD5_WORD b, MD5_WORD c, MD5_WORD d, MD5_WORD x_k, int s, MD5_WORD t_i) {
	return b + md5_rot_left(a + md5_i(b, c, d) + x_k + t_i, s);	
}

void md5_print(unsigned char digest[16]) {
	for (int i=0; i<16; ++i) {
		printf("%02x", digest[i]);
	}
	putchar('\n');
}

void md5(char *m, unsigned char digest[16]) {
	unsigned char *message = (unsigned char *)m;
	size_t message_len = md5_strlen((const unsigned char *)message);
	size_t padding_len = message_len%56 == 0 ? 64 : 56-message_len%56;

	unsigned char message_len_bits[8];
	md5_memset(message_len_bits, 0, 8);
	MD5_WORD bits[2] = { (message_len << 3) & 0x0ffffffff, ((message_len << 3) >> 32) & 0x0ffffffff};
	md5_encode(message_len_bits, bits, 2);

	size_t message_pp_len = message_len+padding_len+8;
	unsigned char message_pp[message_pp_len];

	md5_memcpy(message_pp, message, message_len);
	md5_memcpy(message_pp+message_len, MD5_PADDING, padding_len);
	md5_memcpy(message_pp+message_len+padding_len, message_len_bits, 8);

	MD5_WORD state[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};

	for (int i=0; i<message_pp_len/64; ++i) {

		MD5_WORD x[16];
		md5_memset((unsigned char *)x, 0, 16);
		for (int j=0; j<16; ++j) {
			int idx = 64*i + 4*j;
			x[j] = 	(((MD5_WORD)message_pp[idx+0]) << (8*0)) |
					(((MD5_WORD)message_pp[idx+1]) << (8*1)) |
					(((MD5_WORD)message_pp[idx+2]) << (8*2)) |
					(((MD5_WORD)message_pp[idx+3]) << (8*3)) ;
		}

		MD5_WORD a = state[0];
		MD5_WORD b = state[1];
		MD5_WORD c = state[2];
		MD5_WORD d = state[3];

		a = round_1(a, b, c, d, x[ 0], MD5_S11, 0xd76aa478);
		d = round_1(d, a, b, c, x[ 1], MD5_S12, 0xe8c7b756);
		c = round_1(c, d, a, b, x[ 2], MD5_S13, 0x242070db);
		b = round_1(b, c, d, a, x[ 3], MD5_S14, 0xc1bdceee);
		a = round_1(a, b, c, d, x[ 4], MD5_S11, 0xf57c0faf);
		d = round_1(d, a, b, c, x[ 5], MD5_S12, 0x4787c62a);
		c = round_1(c, d, a, b, x[ 6], MD5_S13, 0xa8304613);
		b = round_1(b, c, d, a, x[ 7], MD5_S14, 0xfd469501);
		a = round_1(a, b, c, d, x[ 8], MD5_S11, 0x698098d8);
		d = round_1(d, a, b, c, x[ 9], MD5_S12, 0x8b44f7af);
		c = round_1(c, d, a, b, x[10], MD5_S13, 0xffff5bb1);
		b = round_1(b, c, d, a, x[11], MD5_S14, 0x895cd7be);
		a = round_1(a, b, c, d, x[12], MD5_S11, 0x6b901122);
		d = round_1(d, a, b, c, x[13], MD5_S12, 0xfd987193);
		c = round_1(c, d, a, b, x[14], MD5_S13, 0xa679438e);
		b = round_1(b, c, d, a, x[15], MD5_S14, 0x49b40821);

		a = round_2(a, b, c, d, x[ 1], MD5_S21, 0xf61e2562);
		d = round_2(d, a, b, c, x[ 6], MD5_S22, 0xc040b340);
		c = round_2(c, d, a, b, x[11], MD5_S23, 0x265e5a51);
		b = round_2(b, c, d, a, x[ 0], MD5_S24, 0xe9b6c7aa);
		a = round_2(a, b, c, d, x[ 5], MD5_S21, 0xd62f105d);
		d = round_2(d, a, b, c, x[10], MD5_S22, 0x02441453);
		c = round_2(c, d, a, b, x[15], MD5_S23, 0xd8a1e681);
		b = round_2(b, c, d, a, x[ 4], MD5_S24, 0xe7d3fbc8);
		a = round_2(a, b, c, d, x[ 9], MD5_S21, 0x21e1cde6);
		d = round_2(d, a, b, c, x[14], MD5_S22, 0xc33707d6);
		c = round_2(c, d, a, b, x[ 3], MD5_S23, 0xf4d50d87);
		b = round_2(b, c, d, a, x[ 8], MD5_S24, 0x455a14ed);
		a = round_2(a, b, c, d, x[13], MD5_S21, 0xa9e3e905);
		d = round_2(d, a, b, c, x[ 2], MD5_S22, 0xfcefa3f8);
		c = round_2(c, d, a, b, x[ 7], MD5_S23, 0x676f02d9);
		b = round_2(b, c, d, a, x[12], MD5_S24, 0x8d2a4c8a);

		a = round_3(a, b, c, d, x[ 5], MD5_S31, 0xfffa3942);
		d = round_3(d, a, b, c, x[ 8], MD5_S32, 0x8771f681);
		c = round_3(c, d, a, b, x[11], MD5_S33, 0x6d9d6122);
		b = round_3(b, c, d, a, x[14], MD5_S34, 0xfde5380c);
		a = round_3(a, b, c, d, x[ 1], MD5_S31, 0xa4beea44);
		d = round_3(d, a, b, c, x[ 4], MD5_S32, 0x4bdecfa9);
		c = round_3(c, d, a, b, x[ 7], MD5_S33, 0xf6bb4b60);
		b = round_3(b, c, d, a, x[10], MD5_S34, 0xbebfbc70);
		a = round_3(a, b, c, d, x[13], MD5_S31, 0x289b7ec6);
		d = round_3(d, a, b, c, x[ 0], MD5_S32, 0xeaa127fa);
		c = round_3(c, d, a, b, x[ 3], MD5_S33, 0xd4ef3085);
		b = round_3(b, c, d, a, x[ 6], MD5_S34, 0x04881d05);
		a = round_3(a, b, c, d, x[ 9], MD5_S31, 0xd9d4d039);
		d = round_3(d, a, b, c, x[12], MD5_S32, 0xe6db99e5);
		c = round_3(c, d, a, b, x[15], MD5_S33, 0x1fa27cf8);
		b = round_3(b, c, d, a, x[ 2], MD5_S34, 0xc4ac5665);

		a = round_4(a, b, c, d, x[ 0], MD5_S41, 0xf4292244);
		d = round_4(d, a, b, c, x[ 7], MD5_S42, 0x432aff97);
		c = round_4(c, d, a, b, x[14], MD5_S43, 0xab9423a7);
		b = round_4(b, c, d, a, x[ 5], MD5_S44, 0xfc93a039);
		a = round_4(a, b, c, d, x[12], MD5_S41, 0x655b59c3);
		d = round_4(d, a, b, c, x[ 3], MD5_S42, 0x8f0ccc92);
		c = round_4(c, d, a, b, x[10], MD5_S43, 0xffeff47d);
		b = round_4(b, c, d, a, x[ 1], MD5_S44, 0x85845dd1);
		a = round_4(a, b, c, d, x[ 8], MD5_S41, 0x6fa87e4f);
		d = round_4(d, a, b, c, x[15], MD5_S42, 0xfe2ce6e0);
		c = round_4(c, d, a, b, x[ 6], MD5_S43, 0xa3014314);
		b = round_4(b, c, d, a, x[13], MD5_S44, 0x4e0811a1);
		a = round_4(a, b, c, d, x[ 4], MD5_S41, 0xf7537e82);
		d = round_4(d, a, b, c, x[11], MD5_S42, 0xbd3af235);
		c = round_4(c, d, a, b, x[ 2], MD5_S43, 0x2ad7d2bb);
		b = round_4(b, c, d, a, x[ 9], MD5_S44, 0xeb86d391);

		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;
	}
	md5_encode(digest, state, 4);
}
#endif // MD5_IMPLEMENTATION