// dimension.cpp - DMI - 19-2-2007

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include <sys/time.h>

#include <limits>

#include "datatype.h"
#include "dimension.h"
#include "error.h"

typedef unsigned char uchar;

int floats_equal(float f1, float f2);

// Private utility functions:
struct segment
{
	char delim;
	int value;
	int denom;
};
	
static segmentvector *separate(const char *s);
static void deallocate(segmentvector *v);

static segmentvector *separate(const char *s)
{
	segmentvector *v;
	int pos = 0;
	segment *seg;
	
	v = new segmentvector();
	while(s[pos] == ' ')
		pos++;
	if(s[pos] == '\0')
		return v;
	while(1)
	{
		seg = new segment;
		seg->value = 0;
		seg->denom = 1;
		while((s[pos] >= '0' && s[pos] <= '9') || s[pos] == ' ')
		{
			if(s[pos] == ' ')
			{
				pos++;
				continue;
			}
			seg->value = seg->value * 10 + s[pos] - '0';
			seg->denom *= 10;
			pos++;
		}
		seg->delim = s[pos];		
		v->add(seg);
		if(s[pos] == '\0')
			break;
		pos++;
	}
	return v;
}

static void deallocate(segmentvector *v)
{
	for(int i = 0; i < v->count(); i++)
		delete v->item(i);
	delete v;
}

/* sdatetime */

sdatetime::sdatetime(int y, int m, int d, int s, int u)
{
	year = y;
	month = m;
	day = d;
	secs = s;
	micros = u;
}

sdatetime::sdatetime(sdatetime *copy)
{
	year = copy->year;
	month = copy->month;
	day = copy->day;
	secs = copy->secs;
	micros = copy->micros;
}

void sdatetime::set_now()
{
	struct timeval tv;
	time_t epoch_secs;
	struct tm *timevals;
	
	gettimeofday(&tv, NULL);
	micros = tv.tv_usec;
	epoch_secs = tv.tv_sec;
	
	timevals = gmtime(&epoch_secs);
	year = timevals->tm_year + 1900;
	month = timevals->tm_mon + 1;
	day = timevals->tm_mday;
	secs = timevals->tm_hour * 3600 + timevals->tm_min * 60 + timevals->tm_sec;
}

sdatetime::sdatetime()
{
	year = month = day = secs = micros = 0;
}

sdatetime::sdatetime(const char *s)
{
	if(strlen(s) == 8)
	{
		from_binary((const unsigned char *)s);
		return;
	}
	
	year = month = day = secs = micros = 0;
	segmentvector *v = separate(s);
	if(v->count() == 3 && v->item(0)->delim == '/' && v->item(1)->delim == '/')
	{
		// Date only:
		day = v->item(0)->value;
		month = v->item(1)->value;
		year = v->item(2)->value;
	}
	else if(v->count() == 3 && v->item(0)->delim == ':' &&
		v->item(1)->delim == ':')
	{
		// Time only, no subseconds:
		secs = v->item(0)->value * 3600 + v->item(1)->value * 60 +
				v->item(2)->value;
	}
	else if(v->count() == 6 && v->item(0)->delim == '/' &&
		v->item(1)->delim == '/' && v->item(2)->delim == ',' &&
		v->item(3)->delim == ':' && v->item(4)->delim == ':')
	{
		// Time and date, date first, no subseconds:
		day = v->item(0)->value;
		month = v->item(1)->value;
		year = v->item(2)->value;
		secs = v->item(3)->value * 3600 + v->item(4)->value * 60 +
				v->item(5)->value;
	}
	else if(v->count() == 6 && v->item(0)->delim == ':' &&
		v->item(1)->delim == ':' && v->item(2)->delim == ',' &&
		v->item(3)->delim == '/' && v->item(4)->delim == '/')
	{
		// Time and date, time first, no subseconds:
		secs = v->item(0)->value * 3600 + v->item(1)->value * 60 +
				v->item(2)->value;
		day = v->item(3)->value;
		month = v->item(4)->value;
		year = v->item(5)->value;
	}
	else if(v->count() == 7 && v->item(0)->delim == '/' &&
		v->item(1)->delim == '/' && v->item(2)->delim == ',' &&
		v->item(3)->delim == ':' && v->item(4)->delim == ':' &&
		v->item(5)->delim == '.')
	{
		// Time and date, date first, with subseconds:
		day = v->item(0)->value;
		month = v->item(1)->value;
		year = v->item(2)->value;
		secs = v->item(3)->value * 3600 + v->item(4)->value * 60 +
				v->item(5)->value;
		micros = v->item(6)->value * (1000000 / v->item(6)->denom);
	}
	else if(v->count() == 7 && v->item(0)->delim == ':' &&
		v->item(1)->delim == ':' && v->item(2)->delim == '.' &&
		v->item(3)->delim == ',' && v->item(4)->delim == '/' &&
		v->item(5)->delim == '/')
	{
		// Time and date, time first, with subseconds:
		secs = v->item(0)->value * 3600 + v->item(1)->value * 60 +
				v->item(2)->value;
		micros = v->item(3)->value * (1000000 / v->item(3)->denom);		
		day = v->item(4)->value;
		month = v->item(5)->value;
		year = v->item(6)->value;
	}
	else
	{
		// Badformat, fields remain at zero
	}
	deallocate(v);
}

int sdatetime::compare(sdatetime *d)
{
	if(year < d->year) return -1;
	if(year > d->year) return 1;
	if(month < d->month) return -1;
	if(month > d->month) return 1;
	if(day < d->day) return -1;
	if(day > d->day) return 1;
	if(secs < d->secs) return -1;
	if(secs > d->secs) return 1;
	if(micros < d->micros) return -1;
	if(micros > d->micros) return 1;
	return 0;
}

int sdatetime::address_field(const char *fieldname)
{
	// field names: day, month, year, hour, min, sec, micros
	if(!strcmp(fieldname, "year"))
		return year;
	else if(!strcmp(fieldname, "month"))
		return month;
	else if(!strcmp(fieldname, "day"))
		return day;
	else if(!strcmp(fieldname, "hour"))
		return (secs / 3600);
	else if(!strcmp(fieldname, "min"))
		return ((secs % 3600) / 60);
	else if(!strcmp(fieldname, "sec"))
		return (secs % 60);
	else if(!strcmp(fieldname, "micros"))
		return micros;
	return -1;
}

const char *sdatetime::tostring()
{
	static char s[80];
	
	sprintf(s, "%02d:%02d:%02d.%06d,%02d/%02d/%04d",
			secs / 3600, (secs / 60) % 60, secs % 60, micros, day, month, year);
	
	return sdup(s);
}

void sdatetime::from_binary(const unsigned char *data)
{
	day = data[0] & 0x1F;
	month = (data[1] & 0xF0) >> 4;
	year = ((data[1] & 0xF) << 8) | data[2];
	micros = (data[3] << 12) | (data[4] << 4) | ((data[5] & 0xF0) >> 4);
	secs = ((data[5] & 0x1) << 16) | (data[6] << 8) | data[7];
}

unsigned char *sdatetime::encode()
{
	unsigned char *data;
	
	data = new uchar[8];
	
	data[0] = day & 0x1F;
	data[1] = ((month & 0xF) << 4) | ((year >> 8) & 0xF);
	data[2] = year & 0xFF;

	data[3] = (micros >> 12) & 0xFF;
	data[4] = (micros >> 4) & 0xFF;
	data[5] = ((micros & 0xF) << 4) | ((secs >> 16) & 0x1);
	data[6] = (secs >> 8) & 0xFF;
	data[7] = secs & 0xFF;
		
	return data;
}

int sdatetime::typematch(const char *s)
{
	// Pattern: d+/d+/d+ | d+:d+:d+[.d+] | d+:d+:d+[.d+],d+/d+/d+
	// Ignore whitespace around commas

	try
	{
		s = eat_integer(s);
		if(s[0] == '/')
		{
			// Date first
			s++;
			s = eat_integer(s);
			s = eat_literal(s, '/');
			s = eat_integer(s);
			if(s[0] == '\0')
				return 1;
			// Time second
			s = eat_comma(s);
			s = eat_integer(s);
			s = eat_literal(s, ':');
			s = eat_integer(s);
			s = eat_literal(s, ':');
			s = eat_integer(s);
			s = eat_optional_fraction(s);
			if(s[0] == '\0')
				return 1;
			else
				return 0;
		}
		else if(s[0] == ':')
		{
			// Time first
			s++;
			s = eat_integer(s);
			s = eat_literal(s, ':');
			s = eat_integer(s);
			s = eat_optional_fraction(s);
			if(s[0] == '\0')
				return 1;
			// Date second
			s = eat_comma(s);
			s = eat_integer(s);
			s = eat_literal(s, '/');
			s = eat_integer(s);
			s = eat_literal(s, '/');
			s = eat_integer(s);
			if(s[0] == '\0')
				return 1;
			else
				return 0;
		}
		else
			return 0;
	}
	catch(MatchException e)
	{
		return 0;
	}
}

/* slocation */

slocation::slocation(float latitude, float longitude, float elevation)
{
	lat = latitude;
	lon = longitude;
	elev = elevation;
}

slocation::slocation(slocation *copy)
{
	lat = copy->lat;
	lon = copy->lon;
	elev = copy->elev;
}

slocation::slocation(float latitude, float longitude)
{
	lat = latitude;
	lon = longitude;
	elev = std::numeric_limits<float>::quiet_NaN();
}

slocation::slocation()
{
	lat = lon = 0.0;
	elev = std::numeric_limits<float>::quiet_NaN();
}

slocation::slocation(const char *s)
{
	int len = strlen(s);
	int first_comma = -1, second_comma = -1;
	lat = lon = 0.0;
	elev = std::numeric_limits<float>::quiet_NaN();	
	
	for(int i = 0; i < len; i++)
	{
		if(s[i] == ',')
		{
			if(first_comma == -1)
				first_comma = i;
			else if(second_comma == -1)
				second_comma = i;
			else
				return; // Too many commas
		}
	}
	if(first_comma == -1)
		return; // No commas
	sscanf(s, "%f", &lat);
	sscanf(s + first_comma + 1, "%f", &lon);
	if(second_comma != -1)
		sscanf(s + second_comma + 1, "%f", &elev);
	else
		elev = std::numeric_limits<float>::quiet_NaN();
}

int slocation::equals(slocation *l)
{
	if(lat != l->lat || lon != l->lon)
		return 0;
	if((!isnan(elev) || !isnan(l->elev)) && elev != l->elev)
		return 0;
	return 1;
}

int slocation::approxequals(slocation *l)
{
	if(!floats_equal(lat, l->lat) || !floats_equal(lon, l->lon))
		return 0;
	if((!isnan(elev) || !isnan(l->elev)) && !floats_equal(elev, l->elev))
		return 0;
	return 1;
}

double slocation::address_field(const char *fieldname)
{
	if(!strcmp(fieldname, "lat"))
		return lat;
	else if(!strcmp(fieldname, "lon"))
		return lon;
	else if(!strcmp(fieldname, "elev"))
		return elev;
	return -10001.0;
}

int slocation::height_defined()
{
	if(isnan(elev))
		return 0;
	return 1;
}

void slocation::from_binary(const unsigned char *s)
{
	float *f = (float *)s;
	lat = f[0];
	lon = f[1];
	elev = f[2];
}

int slocation::typematch(const char *s)
{
	// Pattern: [s]d+[.d+],[s]d+[.d+][,[s]d+[.d+]]
	// Ignore whitespace around commas
	
	try
	{
		s = eat_optional_sign(s);
		s = eat_integer(s);
		s = eat_optional_fraction(s);
		s = eat_comma(s);
		s = eat_optional_sign(s);
		s = eat_integer(s);
		s = eat_optional_fraction(s);
		if(s[0] == '\0')
			return 1;
		s = eat_comma(s);
		s = eat_optional_sign(s);
		s = eat_integer(s);
		s = eat_optional_fraction(s);
		return 1;
	}
	catch(MatchException e)
	{
		return 0;
	}
}

const char *slocation::tostring()
{
	static char s[80];
	
	if(isnan(elev))
		sprintf(s, "%f,%f", lat, lon);
	else
		sprintf(s, "%f,%f,%f", lat, lon, elev);
	return sdup(s);
}

unsigned char *slocation::encode()
{
	unsigned char *data;
	
	data = new uchar[12];
	float *f = (float *)data;
	f[0] = lat;
	f[1] = lon;
	f[2] = elev;
	return(data);
}

const char *eat_whitespace(const char *s)
{
	while(s[0] == ' ' || s[0] == '\t' || s[0] == '\n')
		s++;
	return s;
}

const char *eat_optional_sign(const char *s)
{
	if(s[0] == '-' || s[0] == '+')
		s++;
	return s;
}

const char *eat_digits(const char *s)
{
	while(is_digit(s[0]))
		s++;
	return s;
}

const char *eat_optional_fraction(const char *s)
{
	if(s[0] != '.')
		return s;
	s++;
	if(!is_digit(s[0]))
		throw MatchException();
	s = eat_digits(s);
	return s;
}

const char *eat_integer(const char *s)
{
	if(!is_digit(s[0]))
		throw MatchException();
	s = eat_digits(s);
	return s;
}

const char *eat_comma(const char *s)
{
	s = eat_whitespace(s);
	if(s[0] != ',')
		throw MatchException();
	s++;
	s = eat_whitespace(s);
	return s;
}

const char *eat_literal(const char *s, char c)
{
	if(s[0] == c)
		return s + 1;
	else
		throw MatchException();
}

int is_digit(char c)
{
	if(c >= '0' && c <= '9')
		return 1;
	else
		return 0;
}

int is_alphabetic(char c)
{
	if(c >= 'a' && c <= 'z')
		return 1;
	if(c >= 'A' && c <= 'Z')
		return 1;
	if(c == '_')
		return 1;
	return 0;
}

int is_alphanumeric(char c)
{
	if(is_digit(c) || is_alphabetic(c))
		return 1;
	else
		return 0;
}

int is_hex(char c)
{
	if(c >= '0' && c <= '9')
		return 1;
	else if(c >= 'A' && c <= 'F')
		return 1;
	else if(c >= 'a' && c <= 'f')
		return 1;
	else
		return 0;
}

int hex_val(char c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if(c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return -1;
}

int read_hexbyte(const char *s)
{
	int n;
	
	if(!is_hex(s[0]) || !is_hex(s[1]))
		return -1;
	n = hex_val(s[0]) * 16 + hex_val(s[1]);
	return n;
}

int floats_equal(float f1, float f2)
{
	float temp;
	
	if(f1 > f2)
	{
		temp = f1;
		f1 = f2;
		f2 = temp;
	}
	f1 *= 1.000001;
	if(f1 < f2)
		return 0;
	else
		return 1;
}
