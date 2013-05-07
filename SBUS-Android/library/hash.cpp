// hash.cpp - DMI - 8-4-2007

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "datatype.h"
#include "error.h"
#include "hash.h"
	
typedef unsigned char uchar;

/*
	2^48 = 281,474,976,710,656
	2^48 - 59 = prime = 281,474,976,710,597 = 16777215:16777157
	2^24 = 16,777,216
*/

unsigned char *HashCode::hash(const char *s)
{
	int len = strlen(s);
	unsigned int c, carry;
	unsigned char *result;
	int x = 0, y = 0;
	int mod = 16777216;
	int p1 = 16777215, p2 = 16777157;
	int p1mult[7], p2mult[7];
	int comment = 0;

	// printf("Hashing %s\n", s);
	
	p1mult[0] = p1;
	p2mult[0] = p2;
	// printf("p1mult[0] = %d, p2mult[0] = %d\n", p1mult[0], p2mult[0]);
	for(int i = 1; i < 7; i++)
	{
		p2mult[i] = p2mult[i - 1] * 2;
		p1mult[i] = p1mult[i - 1] * 2;
		if(p2mult[i] > mod)
		{
			p2mult[i] -= mod;
			p1mult[i]++;
		}
		// printf("p1mult[%d] = %d, p2mult[%d] = %d\n", i, p1mult[i], i, p2mult[i]);
	}
		
	result = new uchar[6];

	for(int i = 0; i < len; i++)
	{
		if(comment)
		{
			if(s[i] == '\n')
				comment = 0;
			continue;
		}
		if(s[i] == ' ' || s[i] == '\t' || s[i] == '\n')
			continue; // Ignore whitespace
		if(s[i] == '*')
		{
			comment = 1;
			continue;
		}
		// printf("x = %d, y = %d\n", x, y);
		// Shift and merge in next character:
		c = s[i] & 0x7F;
		carry = y >> 17;
		y &= 0x1FFFF;
		y <<= 7;
		y |= c;
		x <<= 7;
		x |= carry;
		// printf("x = %d, y = %d\n", x, y);
		
		// Take modulo prime:
		for(int j = 6; j >= 0; j--)
		{
			// printf("Reducing: x = %d, y = %d\n", x, y);
			if(x > p1mult[j] || (x == p1mult[j] && y >= p2mult[j]))
			{
				x -= p1mult[j];
				y -= p2mult[j];
				if(y < 0)
				{
					y += mod;
					x--;
				}
			}
		}
		
		// Sanity check:
		if(x >= mod || y >= mod)
			error("Hash algorithm failure.\n");
	}
	
	result[0] = x >> 16;
	result[1] = (x >> 8) & 0xFF;
	result[2] = x & 0xFF;
	result[3] = y >> 16;
	result[4] = (y >> 8) & 0xFF;
	result[5] = y & 0xFF;
	
	return result;
}

char hex(int d)
{
	if(d >= 0 && d <= 9) return d + '0';
	if(d >= 10 && d <= 15) return d - 10 + 'A';
	error("Not a hex digit in decimal to hex conversion");
	return '\0'; // Never happens
}

int dec(char c)
{
	if(c >= '0' && c <= '9') return c - '0';
	if(c >= 'A' && c <= 'F') return c - 'A' + 10;
	if(c >= 'a' && c <= 'f') return c - 'a' + 10;
	error("Not a hex digit in hex to decimal conversion");
	return -1; // Never happens
}

HashCode::HashCode()
{
	buf = new uchar[6];
}

HashCode::HashCode(HashCode *hc)
{
	buf = new uchar[6];
	if(hc == NULL)
	{
		for(int i = 0; i < 6; i++)
			buf[i] = 0xEE;
	}
	else
	{
		memcpy(buf, hc->buf, 6);
	}
}

int HashCode::toint()
{
	// Quick conversion to a rather less unique 24 bit number
	int x;
	
	x = buf[0] ^ buf[3];
	x |= (buf[1] ^ buf[4]) << 8;
	x |= (buf[2] ^ buf[5]) << 16;
	return x;
}

HashCode::~HashCode()
{
	delete[] buf;
}

unsigned char *HashCode::tobinary()
{
	uchar *result = new uchar[6];
	memcpy(result, buf, 6);
	return result;
}

char *HashCode::tostring()
{
	char *s = new char[13];
	
	for(int i = 0; i < 6; i++)
	{
		s[i * 2] = hex(buf[i] >> 4);
		s[i * 2 + 1] = hex(buf[i] & 0xF);
	}
	s[12] = '\0';
	return s;
}

void HashCode::frombinary(const unsigned char *s)
{
	memcpy(buf, s, 6);
}

void HashCode::fromstring(const char *s)
{
	for(int i = 0; i < 6; i++)
		buf[i] = (uchar)(dec(s[i * 2]) << 4 | dec(s[i * 2 + 1]));
}

void HashCode::frommeta(MetaType meta)
{
	switch(meta)
	{
		case SCHEMA_EMPTY: fromstring("000000000000"); return;
		case SCHEMA_POLY:  fromstring("FFFFFFFFFFFF"); return;
		case SCHEMA_EXPR:  fromstring("111111111111"); return;
		case SCHEMA_NA:    fromstring("EEEEEEEEEEEE"); return;
		default: error("Not a meta-schema in HashCode::frommeta()");
	}
}

void HashCode::fromschema(const char *source)
{
	unsigned char *hsh;
	
	hsh = hash(source);
	memcpy(buf, hsh, 6);
	delete[] hsh;
}

int HashCode::equals(HashCode *hc)
{
	if(memcmp(buf, hc->buf, 6) != 0)
		return 0;
	else
		return 1;
}

int HashCode::isempty()
{
	for(int i = 0; i < 6; i++)
		if(buf[i] != 0)
			return 0;
	return 1;
}

int HashCode::ispolymorphic()
{
	for(int i = 0; i < 6; i++)
		if(buf[i] != 0xFF)
			return 0;
	return 1;
}

int HashCode::isapplicable()
{
	for(int i = 0; i < 6; i++)
		if(buf[i] != 0xEE)
			return 1;
	return 0;
}
