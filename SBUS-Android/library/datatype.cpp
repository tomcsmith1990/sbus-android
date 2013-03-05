// datatype.cpp - DMI - 17-1-2002

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <pwd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "datatype.h"
#include "error.h"
#include "hash.h"

extern int fixed_write(int sock, const unsigned char *buf, int nbytes);

typedef unsigned char uchar;
typedef void *voidptr;
typedef char *charptr;

/* string functions */

char *sdup(const char *s)
{
	if(s == NULL)
		return NULL;
	char *t;
	int len = strlen(s);
	t = new char[len + 1];
	strcpy(t, s);
	return t;
}

void sfree(const char *s)
{
	if(s != NULL)
		delete[] s;
}

char *strmerge(const char *format, ...)
{
	const int MAX_STR_LEN = 200;
	va_list args;
	char *s = new char[MAX_STR_LEN];

	va_start(args, format);
	vsnprintf(s, MAX_STR_LEN, format, args);
	va_end(args);
	s[MAX_STR_LEN - 1] = '\0';
	return s;
}

char *sjoin(const char *s1, char c, const char *s2)
{
	int len1, len2;
	char *s;
	
	len1 = strlen(s1);
	len2 = strlen(s2);
	s = new char[len1 + len2 + 2];
	memcpy(s, s1, len1);
	s[len1] = c;
	memcpy(s + len1 + 1, s2, len2);
	s[len1 + len2 + 1] = '\0';
	return s;
}

/* file utility functions */

int fexists(const char *file_path)
{
	struct stat buf;
	if(stat(file_path, &buf) == 0)
		return 1;
	else
		return 0;
}

char *gethomedir()
{
	uid_t id;
	struct passwd *pw;
	
	id = getuid();
	pw = getpwuid(id);
	if(pw == NULL)
		error("Can't lookup home directory");
	return sdup(pw->pw_dir);
}

/* intvector */

intvector::intvector()
{
	capacity = 10;
	data = new int[10];
	size = 0;
}

intvector::~intvector()
{
	delete[] data;
}

void intvector::add(int x)
{
	if(size == capacity)
		expand_capacity();
	
	data[size++] = x;
}

void intvector::add_unique(int x)
{
	if(find(x) == -1)
		add(x);
}

void intvector::set(int n, int x)
{
	if(n < 0 || n >= size)
		return;
	
	data[n] = x;
}

void intvector::del(int n)
{
	if(n < 0 || n >= size)
		return;
	
	for(int i = n; i < size - 1; i++)
		data[i] = data[i + 1];
	
	size--;
}

void intvector::remove(int x)
{
	int pos;
	
	pos = find(x);
	if(pos != -1)
		del(pos);
}

int intvector::find(int x)
{
	for(int i = 0; i < size; i++)
		if(data[i] == x)
			return i;
	
	return -1;
}

void intvector::expand_capacity()
{
	int *new_data;
	
	capacity *= 2;
	new_data = new int[capacity];
	for(int i = 0; i < size; i++)
	{
		new_data[i] = data[i];
	}
	delete[] data;
	data = new_data;
}

int intvector::item(int n)
{
	if(n < 0 || n >= size)
		return -1;
	
	return data[n];
}

int intvector::count()
{
	return size;
}

/* pvector */

pvector::pvector()
{
	capacity = 10;
	data = new voidptr[10];
	size = 0;
}

pvector::~pvector()
{
	delete[] data;
}

void pvector::clear()
{
	// Keep the same capacity
	size = 0;
}

void pvector::add(void *x)
{
	if(size == capacity)
		expand_capacity();
	
	data[size++] = x;
}

int pvector::find(void *x)
{
	for(int i = 0; i < size; i++)
		if(data[i] == x)
			return i;
	return -1;
}

void pvector::set(int n, void *x)
{
	if(n < 0 || n >= size)
		return;
	
	data[n] = x;
}

void pvector::del(int n)
{
	if(n < 0 || n >= size)
		return;
	
	for(int i = n; i < size - 1; i++)
		data[i] = data[i + 1];
	
	size--;
}

void pvector::remove(void *x)
{
	int pos;
	
	pos = find(x);
	if(pos != -1)
		del(pos);
}

void pvector::expand_capacity()
{
	void **new_data;
	
	capacity *= 2;
	new_data = new voidptr[capacity];
	for(int i = 0; i < size; i++)
	{
		new_data[i] = data[i];
	}
	delete[] data;
	data = new_data;
}

void *pvector::item(int n)
{
	if(n < 0 || n >= size)
		return NULL;
	
	return data[n];
}

void *pvector::top()
{
	if(size == 0)
		return NULL;
	
	return data[size - 1];
}

void *pvector::pop()
{
	if(size == 0)
		return NULL;

	size--;	
	return data[size];
}

int pvector::count()
{
	return size;
}

/* svector */
		
svector::svector()
{
	capacity = 10;
	data = new charptr[10];
	size = 0;
}

svector::svector(svector *src)
{
	capacity = src->capacity;
	size = src->size;
	data = new charptr[capacity];
	for(int i = 0; i < size; i++)
		data[i] = sdup(src->data[i]);
}

svector::~svector()
{
	for(int i = 0; i < size; i++)
		delete[] data[i];
	
	delete[] data;
}

void svector::clear()
{
	for(int i = 0; i < size; i++)
		delete[] data[i];
	size = 0;
}

int svector::add(const char *s)
{
	if(size == capacity)
		expand_capacity();
	
	data[size] = new char[strlen(s) + 1];
	strcpy(data[size], s);
	size++;
	return (size - 1);
}

int svector::add(const char *s, int nchars)
{
	if(size == capacity)
		expand_capacity();
	
	data[size] = new char[nchars + 1];
	strncpy(data[size], s, nchars);
	data[size][nchars] = '\0';
	size++;
	return (size - 1);
}

char *svector::remove(const char *s)
{
	if (size == 0)
		return NULL;
	int item = find(s);

	if(item < 0 || item >= size)
		return NULL;

	char *result = new char[strlen(data[item])];
	for (int i = 0; i < strlen(data[item]); i++)
		result[i] = data[item][i];

	for(int i = item; i < size - 1; i++)
		data[i] = data[i + 1];
		
	return result;
}

char *svector::pop()
{
	if(size == 0)
		return NULL;
	size--;
	return data[size];
}

void svector::expand_capacity()
{
	char **new_data;
	
	capacity *= 2;
	new_data = new charptr[capacity];
	for(int i = 0; i < size; i++)
	{
		new_data[i] = data[i];
	}
	delete[] data;
	data = new_data;
}

int svector::add_noduplicates(const char *s)
{
	int pos = find(s);
	if(pos == -1)
		return add(s);
	else
		return pos;
}

int svector::find(const char *s)
{
	for(int i = 0; i < size; i++)
		if(!strcmp(s, data[i]))
			return i;
	return -1;
}

char *svector::item(int n)
{
	if(n < 0 || n >= size)
		return NULL;
	
	return data[n];
}

int svector::count()
{
	return size;
}

/* StringBuf */

StringBuf::StringBuf()
{
	capacity = 50;
	buf = new uchar[capacity];
	used = 0;
	varbuf = spcbuf = NULL;
	maxspcbuf = 0;
	maxvarbuf = 100;
}

StringBuf::StringBuf(StringBuf *sb)
{
	used = sb->used;
	capacity = sb->capacity;
	buf = new uchar[capacity];
	memcpy(buf, sb->buf, used);
	
	varbuf = spcbuf = NULL;
	maxspcbuf = 0;
	maxvarbuf = 100;
}

StringBuf::~StringBuf()
{
	delete[] buf;
	if(varbuf != NULL) delete[] varbuf;
	if(spcbuf != NULL) delete[] spcbuf;
}

void StringBuf::clear()
{
	// Keep the same capacity
	used = 0;
}

int StringBuf::length()
{
	return used;
}

void StringBuf::check_expand(int required)
{
	required++; // Space for a NULL terminator, just when needed by peek()
	if(required > capacity)
	{
		unsigned char *new_buf;
		int new_cap;

		new_cap = required * 2;	
		new_buf = new uchar[new_cap];
		memcpy(new_buf, buf, used);
		capacity = new_cap;
		delete[] buf;
		buf = new_buf;
	}
}

char StringBuf::getcharacter(int n)
{
	if(n < 0 || n >= used)
		error("StringBuf::getcharacter() index out of range");
	return ((char)(buf[n]));
}

void StringBuf::append(StringBuf *sb)
{
	cat(sb->extract());
}

void StringBuf::cat(const char *s)
{
	int len = strlen(s);
	check_expand(used + len);
	memcpy(buf + used, s, len);
	used += len;
}

void StringBuf::cat_string(const char *s)
{
	int bytes;

	if(s == NULL)
	{
		// NULL is converted into the empty string:
		cat(0);
		return;
	}
	bytes = strlen(s);
	cat(bytes); // Count
	cat(s, bytes); // Data
}

void StringBuf::catf(const char *format, ...)
{
	va_list args;
	int bytes;
	int retry;

	if(varbuf == NULL)
		varbuf = new char[maxvarbuf];

	while(1)
	{
		retry = 0;
		va_start(args, format);
		bytes = vsnprintf(varbuf, maxvarbuf, format, args);
		if(bytes < 0)
			error("Error in StringBuf::catf()");
		if(bytes >= maxvarbuf)
		{
			delete[] varbuf;
			maxvarbuf = (maxvarbuf + bytes) * 2;
			varbuf = new char[maxvarbuf];
			retry = 1;
		}
		va_end(args);
		if(!retry)
			break;
	}
	cat(varbuf);
}

void StringBuf::cat(const void *data, int bytes)
{
	if(bytes == 0)
		return;
	check_expand(used + bytes);
	memcpy(buf + used, data, bytes);
	used += bytes;
}

void StringBuf::cat_spaces(int n)
{
	if(n < 1)
		return;
	if(n > maxspcbuf)
	{
		if(spcbuf != NULL)
			delete[] spcbuf;
		maxspcbuf = n * 2;
		spcbuf = new char[maxspcbuf];
		for(int i = 0; i < maxspcbuf; i++)
			spcbuf[i] = ' ';
	}
	check_expand(used + n);
	memcpy(buf + used, spcbuf, n);
	used += n;
}

void StringBuf::cat(HashCode *hc)
{
	unsigned char *hash;
	
	hash = hc->tobinary();
	cat((const void *)hash, 6);
	delete[] hash;
}

void StringBuf::cat_byte(int n)
{
	check_expand(used + 1);
	buf[used++] = (char)n;
}

void StringBuf::cat_hexbyte(unsigned char c)
{
	unsigned char d;
	
	check_expand(used + 2);
	d = c >> 4;
	if(d < 10)
		buf[used++] = d + '0';
	else
		buf[used++] = d - 10 + 'A';
	d = c & 0xF;
	if(d < 10)
		buf[used++] = d + '0';
	else
		buf[used++] = d - 10 + 'A';
}

void StringBuf::skip(int n)
{
	check_expand(used + n);
	used += n;
}

void StringBuf::overwrite(int pos, char c)
{
	if(pos < 0 || pos >= used)
		return;
	buf[pos] = c;
}

void StringBuf::overwrite_word(int pos, int n)
{
	if(pos < 0 || pos + 3 >= used)
		return;
	buf[pos++] = n >> 24;
	buf[pos++] = (n >> 16) & 0xFF;
	buf[pos++] = (n >> 8) & 0xFF;
	buf[pos++] = n & 0xFF;
}
	
void StringBuf::cat(double d)
{
	int len;
	int integer = 1;
	
	check_expand(used + 25);
	len = sprintf((char *)(buf + used), "%g", d);
	for(int i = 0; i < len; i++)
	{
		if(buf[used + i] == '.' || buf[used + i] == 'e')
		{
			integer = 0;
			break;
		}
	}
	used += len;
	if(integer)
	{
		buf[used++] = '.';
		buf[used++] = '0';
	}
}

void StringBuf::cat_int(int n)
{
	int len;
	
	check_expand(used + 15);
	len = sprintf((char *)(buf + used), "%d", n);
	used += len;
}

void StringBuf::cat_word(int n)
{
	check_expand(used + 4);
	buf[used++] = n >> 24;
	buf[used++] = (n >> 16) & 0xFF;
	buf[used++] = (n >> 8) & 0xFF;
	buf[used++] = n & 0xFF;
}

void StringBuf::cat(int n)
{
	check_expand(used + 5);
	if(n < 0)
	{
		error("Counts cannot be negative (%d).", n);
	}
	if(n < 254)
	{
		buf[used++] = n;
	}
	else if(n < 65536)
	{
		buf[used++] = 254;
		buf[used++] = n >> 8;
		buf[used++] = n & 0xFF;
	}
	else
	{
		buf[used++] = 255;
		buf[used++] = n >> 24;
		buf[used++] = (n >> 16) & 0xFF;
		buf[used++] = (n >> 8) & 0xFF;
		buf[used++] = n & 0xFF;
	}
}

char StringBuf::pop()
{
	if(used == 0)
		error("Can't pop an empty StringBuf.");
	used--;
	return buf[used];
}

void StringBuf::truncate_spaces()
{
	char c;
	
	while(used > 0)
	{
		c = buf[used - 1];
		if(c != ' ' && c != '\t' && c != '\n')
			break;
		used--;
	}
}

void StringBuf::cat(char c)
{
	check_expand(used + 1);
	buf[used++] = c;
}

char *StringBuf::extract()
{
	char *s = new char[used + 1];
	if(used > 0)
		memcpy(s, buf, used);
	s[used] = '\0';
	return s;
}

int StringBuf::iswhitespace()
{
	char c;
	
	for(int i = 0; i < used; i++)
	{
		c = buf[i];
		if(c != ' ' && c != '\t' && c != '\n')
			return 0;
	}
	return 1;
}

const unsigned char *StringBuf::peek()
{
	buf[used] = '\0';
	return buf;
}

int StringBuf::transmit(int fd)
{
	return fixed_write(fd, buf, used);
}

void StringBuf::print()
{
	// Temporarily place an end of string marker, without increasing "used":
	check_expand(used + 1);
	buf[used] = '\0';
	
	printf("%s", buf);
}

void StringBuf::dump()
{
	for(int i = 0; i < used; i++)
	{
		if(buf[i] < 32 || buf[i] > 126)
			printf(".");
		else
			printf("%c", buf[i]);
	}
	printf("\n");
}

void StringBuf::log()
{
	// Temporarily place an end of string marker, without increasing "used":
	check_expand(used + 1);
	buf[used] = '\0';
	
	::log("%s", buf);
}

int StringBuf::save(const char *filename)
{
	FILE *fp;
	int ret;
	
	fp = fopen(filename, "w");
	if(fp == NULL)
		return -1;
	ret = fwrite(buf, used, 1, fp);
	if(ret != 1)
	{
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 0;
}

/* memfile */

memfile::memfile(const char *filename)
{
	FILE *fp;
	int bytes;
	int pos;

	fp = fopen(filename, "r");
	if(!fp)
	{
		data = NULL;
		len = -1;
		return;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	rewind(fp);
		
	data = new char[len + 1];
	pos = 0;
	
	while(1)
	{
		bytes = fread(data + pos, 1, len - pos, fp);
		if(bytes == 0)
			break;
		pos += bytes;
	}
	if(pos != len)
		error("Error in memfile.\n");
	data[len] = '\0'; // If we're sure no NULLs in data, can use as a string
	
	fclose(fp);
}

memfile::~memfile()
{
	delete[] data;
}

/* stdinfile */

void stdinfile::check_expand(int extra)
{
	char *newbuf;
	
	if(len + extra < capacity)
		return;
	capacity = (len + extra) * 2;
	newbuf = new char[capacity];
	memcpy(newbuf, data, len);
	delete[] data;
	data = newbuf;
}

stdinfile::stdinfile()
{
	const int blocksize = 100;
	int bytes;
	
	capacity = blocksize + 5;
	data = new char[capacity];
	len = 0;
	
	while(1)
	{
		check_expand(blocksize);
		bytes = read(STDIN_FILENO, data + len, blocksize);
		if(bytes < 0)
			error("Error reading from stdin");
		if(bytes == 0)
			break;
		len += bytes;
	}

	check_expand(1);
	data[len] = '\0'; // If we're sure no NULLs in data, can use as a string
}

stdinfile::~stdinfile()
{
	delete[] data;
}

/* linefile */

int linefile::readline(FILE *fp, char *buf, int max_len)
{
	int len;
	
	if(feof(fp))
		return -1;
	buf[0] = '\0';
	fgets(buf, max_len, fp);
	len = strlen(buf);
	if(len > 0 && buf[len - 1] == '\n')
		buf[--len] = '\0';
	return len;
}

linefile::linefile(const char *filename, int filter_blanks)
{
	FILE *fp;
	char *buf;
	int len;
	const int MAX_LINE_LEN = 1000;
	
	v = new svector();
	
	file_ok = 0;
	fp = fopen(filename, "r");
	if(!fp)
		return;
	file_ok = 1;
	
	buf = new char[MAX_LINE_LEN];
	while(1)
	{
		len = readline(fp, buf, MAX_LINE_LEN);
		if(len == -1)
			break;
		if(filter_blanks)
		{
			int whitespace = 1;
			if(len == 0 || buf[0] == '#')
				continue;
			for(int i = 0; i < len; i++)
			{
				if(buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\n' &&
						buf[i] != '\0')
				{
					whitespace = 0;
					break;
				}
			}
			if(whitespace)
				continue;
		}
		v->add(buf);
	}
	
	delete[] buf;
	fclose(fp);
}

int linefile::valid()
{
	return file_ok;
}

linefile::~linefile()
{
	delete v;
}

int linefile::count()
{
	return v->count();
}

const char *linefile::getline(int n)
{
	if(n < 0 || n >= v->count())
		return NULL;
		
	return v->item(n);
}

const char *linefile::item(int n)
{
	if(n < 0 || n >= v->count())
		return NULL;
	
	return v->item(n);
}

int linefile::search(char *line)
{
	for(int i = 0; i < v->count(); i++)
		if(!strcmp(line, v->item(i)))
			return 1;
	return 0;
}

/* PtrStack */

PtrStack::PtrStack()
{
	capacity = 10;
	used = 0;
	stack = new voidptr[capacity];
}

PtrStack::~PtrStack()
{
	delete[] stack;
}

void PtrStack::push(void *p)
{
	if(used == capacity)
	{
		void **enlarged = new voidptr[capacity * 2];
		for(int i = 0; i < capacity; i++)
			enlarged[i] = stack[i];
		delete[] stack;
		stack = enlarged;
		capacity *= 2;
	}
	stack[used] = p;
	used++;
}

void *PtrStack::pop()
{
	if(used == 0)
		return NULL;
	
	used--;
	return stack[used];
}

void *PtrStack::top()
{
	if(used == 0)
		return NULL;
	return stack[used - 1];
}

int PtrStack::items()
{
	return used;
}

/* intqueue */

class qinode
{
	public:
	
	int data;
	qinode *next, *prev;
};

intqueue::intqueue()
{
	head = tail = NULL;
}

intqueue::~intqueue()
{
	qinode *qn, *next;
	
	qn = head;
	while(qn != NULL)
	{
		next = qn->next;
		delete qn;
		qn = next;
	}
}

int intqueue::isempty()
{
	if(head == NULL)
		return 1;
	else
		return 0;
}

void intqueue::add(int x)
{
	qinode *qn;
	
	qn = new qinode;
	qn->data = x;
	if(tail == NULL)
	{
		head = tail = qn;
		qn->next = qn->prev = NULL;
	}
	else
	{
		tail->next = qn;
		qn->prev = tail;
		qn->next = NULL;
		tail = qn;
	}
}

int intqueue::preview()
{
	if(head == NULL)
		return -1;
	return head->data;
}

int intqueue::remove()
{
	int x;
	qinode *second;
	
	if(head == NULL)
		return -1;
	x = head->data;
	second = head->next;
	delete head;
	if(second == NULL)
		head = tail = NULL;
	else
	{
		second->prev = NULL;
		head = second;
	}
	return x;
}

void intqueue::do_unlink(qinode *qn)
{
	if(qn->prev == NULL)
		head = qn->next;
	else
		qn->prev->next = qn->next;
	if(qn->next == NULL)
		tail = qn->prev;
	else
		qn->next->prev = qn->prev;
	delete qn;
}

void intqueue::begin()
{
	iter = head;
}

int intqueue::unlink_and_next()
{
	qinode *elim;
	
	if(iter == NULL)
		elim = tail;
	else
		elim = iter->prev;
	do_unlink(elim);
	
	return next();
}

int intqueue::next()
{
	int x;
	
	if(iter == NULL)
		return -1;
	x = iter->data;
	iter = iter->next;
	return x;
}

/* ptrqueue */

class qnode
{
	public:
	
	void *data;
	qnode *next, *prev;
};

ptrqueue::ptrqueue()
{
	head = tail = NULL;
}

ptrqueue::~ptrqueue()
{
	qnode *qn, *next;
	
	qn = head;
	while(qn != NULL)
	{
		next = qn->next;
		delete qn;
		qn = next;
	}
}

int ptrqueue::isempty()
{
	if(head == NULL)
		return 1;
	else
		return 0;
}

void ptrqueue::add(void *x)
{
	qnode *qn;
	
	qn = new qnode;
	qn->data = x;
	if(tail == NULL)
	{
		head = tail = qn;
		qn->next = qn->prev = NULL;
	}
	else
	{
		tail->next = qn;
		qn->prev = tail;
		qn->next = NULL;
		tail = qn;
	}
}

void *ptrqueue::preview()
{
	if(head == NULL)
		return NULL;
	return head->data;
}

void *ptrqueue::remove()
{
	void *x;
	qnode *second;
	
	if(head == NULL)
		return NULL;
	x = head->data;
	second = head->next;
	delete head;
	if(second == NULL)
		head = tail = NULL;
	else
	{
		second->prev = NULL;
		head = second;
	}
	return x;
}

void ptrqueue::do_unlink(qnode *qn)
{
	if(qn->prev == NULL)
		head = qn->next;
	else
		qn->prev->next = qn->next;
	if(qn->next == NULL)
		tail = qn->prev;
	else
		qn->next->prev = qn->prev;
	delete qn;
}

void ptrqueue::begin()
{
	iter = head;
}

int ptrqueue::count()
{
	qnode *qn = head;
	int n = 0;
	
	while(qn != NULL)
	{
		n++;
		qn = qn->next;
	}
	return n;
}

void *ptrqueue::unlink_and_next()
{
	qnode *elim;
	
	if(iter == NULL)
		elim = tail;
	else
		elim = iter->prev;
	do_unlink(elim);
	
	return next();
}

void *ptrqueue::next()
{
	void *x;
	
	if(iter == NULL)
		return NULL;
	x = iter->data;
	iter = iter->next;
	return x;
}

/* stringsplit */

stringsplit::stringsplit(const char *s, char delim)
{
	int start, pos;
	char *buf = new char[strlen(s) + 1];
	
	v = new svector();
	start = pos = 0;
	while(1)
	{
		while(s[pos] != '\0' && s[pos] != delim)
			pos++;
		if(pos != start) // Forget about zero-length sub-strings
		{
			memcpy(buf, s + start, pos - start);
			buf[pos - start] = '\0';
			v->add(buf);
		}
		if(s[pos] == '\0')
			break;
		pos++;
		start = pos;
	}
	delete[] buf;
}

stringsplit::~stringsplit()
{
	delete v;
}

int stringsplit::count()
{
	return v->count();
}

char *stringsplit::item(int n)
{
	if(n < 0 || n >= v->count())
		return NULL;
	return v->item(n);
}
