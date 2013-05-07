// unmarshall.cpp - DMI - 23-2-2007

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "../library/datatype.h"
#include "../library/dimension.h"
#include "../library/builder.h"
#include "../library/hash.h"
#include "../library/error.h"
#include "litmus.h"
#include "marshall.h"
#include "unmarshall.h"

snode *do_unmarshall(const unsigned char *data, int bytes, litmus *lit,
		svector *symbol_table, int *consumed);

static void validityerror(const char *format, ...)
{
	static const int MAX_ERR_LEN = 200;
	char c[MAX_ERR_LEN];
	va_list args;

	va_start(args, format);
	vsnprintf(c, MAX_ERR_LEN, format, args);
	va_end(args);
	c[MAX_ERR_LEN - 1] = '\0';
	
	throw ValidityException(c);
}

static void check_available(int bytes, int pos, int n)
{
	if(n > 0 && pos + n > bytes)
		validityerror("Unexpected end of data.");
}

static int read_count(const unsigned char *data, int bytes, int *pos)
{
	int n;
	
	check_available(bytes, *pos, 1);
	n = data[*pos];
	*pos += 1;
	if(n == 254)
	{
		check_available(bytes, *pos, 2);
		n = (data[*pos] << 8) | data[*pos + 1];
		*pos += 2;
	}
	else if(n == 255)
	{
		check_available(bytes, *pos, 4);
		n = (data[*pos] << 24) | (data[*pos + 1] << 16) |
				(data[*pos + 2] << 8) | data[*pos + 3];
		*pos += 4;
	}
	return n;
}

snode *unmarshall(const unsigned char *data, int bytes, Schema *schema,
		const char **err)
{
	snode *node;
	litmus *lit = schema->tree;
	
	if(sizeof(double) != 8 || sizeof(int) != 4 || sizeof(float) != 4)
		error("Word sizes unsuitable on this architecture.");
	
	if(schema->hc->isempty())
	{
		if(bytes == 0)
		{
			// This is legitmate - schema and message are both empty
			node = new snode(SEmpty);
			return node;
		}
		else
		{
			*err = sformat("Unmarshalling error: non-empty message received, "
					"but schema only accepts empty data");
			return NULL;
		}
	}
	
	int pos = 0;
	
	try
	{
		node = do_unmarshall((const unsigned char *)data, bytes, lit,
				schema->symbol_table, &pos);
		if(node == NULL)
			validityerror("No content.");
		if(pos != bytes)
			validityerror("Data extends beyond end of message.");
	}
	catch(ValidityException e)
	{
		*err = sformat("Unmarshalling error: %s\n", e.msg);
		return NULL;
	}
	return node;
}

snode *do_unmarshall(const unsigned char *data, int bytes, litmus *lit,
		svector *symbol_table, int *pos)
{
	// May return NULL if legitimate but empty content, e.g. missing opt
	LitmusType t = lit->type;
	snode *node;
	const char *name;
	int sym;
	int count;
	
	if(t != LITMUS_CHOICE && t != LITMUS_OPT)
	{
		sym = lit->namesym;
		name = symbol_table->item(sym);
	}
	else
	{
		sym = -1;
		name = NULL;
	}

	// printf("Unmarshalling type %d, pos = %d\n", t, *pos);
	switch(t)
	{
		case LITMUS_INT:
			{
				int n;
				
				check_available(bytes, *pos, 4);
				n = (data[*pos] << 24) | (data[*pos + 1] << 16) |
						(data[*pos + 2] << 8) | data[*pos + 3];
				node = new snode(n);
				*pos += 4;
			}
			break;
		case LITMUS_DBL:
			{
				char s[8];
				double d;
				
				check_available(bytes, *pos, 8);
				for(int i = 0; i < 8; i++)
					s[i] = data[*pos + i];
				*pos += 8;
				d = *((double *)s);
				node = new snode(d);
			}
			break;
		case LITMUS_FLG:
			{
				int b;
				
				check_available(bytes, *pos, 1);
				b = data[*pos];
				if(b != 0 && b != 1)
					validityerror("Invalid boolean.");
				node = snode::create_bool(b);
				*pos += 1;
			}
			break;
		case LITMUS_TXT:
			{
				char *str;
				
				count = read_count(data, bytes, pos);
				check_available(bytes, *pos, count);
				str = new char[count + 1];
				strncpy(str, (const char *)data + *pos, count);
				str[count] = '\0';
				node = new snode(str);
				delete [] str;
				*pos += count;
			}
			break;
		case LITMUS_BIN:
			count = read_count(data, bytes, pos);
			check_available(bytes, *pos, count);
			node = new snode(data + *pos, count);
			*pos += count;
			break;
		case LITMUS_CLK:
			{
				sdatetime *clk;
				
				check_available(bytes, *pos, 8);
				clk = new sdatetime();
				clk->from_binary(data + *pos);
				node = new snode(clk);
				*pos += 8;
			}
			break;
		case LITMUS_LOC:
			{
				slocation *loc;
				
				check_available(bytes, *pos, 12);
				loc = new slocation();
				loc->from_binary(data + *pos);
				node = new snode(loc);
				*pos += 12;
			}
			break;
		case LITMUS_STRUCT:
			{
				node = new snode();
				node->type = SStruct;
				node->children = new snodevector();
				litmus *child;
				snode *branch;

				for(int i = 0; i < lit->children->count(); i++)
				{
					child = lit->children->item(i);
					branch = do_unmarshall(data, bytes, child, symbol_table, pos);
					if(branch != NULL)
						node->children->add(branch);
				}
			}
			break;
		case LITMUS_LIST:
			{
				litmus *child = lit->content;
				snode *branch;
				
				count = read_count(data, bytes, pos);
				node = new snode();
				node->type = SList;
				node->children = new snodevector();
				for(int i = 0; i < count; i++)
				{
					branch = do_unmarshall(data, bytes, child, symbol_table, pos);
					if(branch != NULL)
						node->children->add(branch);
				}
			}
			break;
		case LITMUS_SEQ:
			{
				litmus *child = lit->content;
				snode *branch;
				
				count = read_count(data, bytes, pos);
				if(count == 0)
					validityerror("Prohibited zero-length list.");
				node = new snode();
				node->type = SList;
				node->children = new snodevector();
				for(int i = 0; i < count; i++)
				{
					branch = do_unmarshall(data, bytes, child, symbol_table, pos);
					if(branch != NULL)
						node->children->add(branch);
				}
			}
			break;
		case LITMUS_ARRAY:
			{
				litmus *child = lit->content;
				snode *branch;
				
				int size = lit->arraylen;
				node = new snode();
				node->type = SList;
				node->children = new snodevector();
				for(int i = 0; i < size; i++)
				{
					branch = do_unmarshall(data, bytes, child, symbol_table, pos);
					if(branch == NULL)
						validityerror("Empty element of array.");
					node->children->add(branch);
				}
			}
			break;
		case LITMUS_OPT:
			count = read_count(data, bytes, pos);
			if(count == 1)
				node = do_unmarshall(data, bytes, lit->content, symbol_table, pos);
			else if(count == 0)
			{
				node = new snode(SNull);
				sym = lit->content->namesym;
				name = symbol_table->item(sym);
			}
			else
				validityerror("Optional flag value 0x%x is not a boolean.", count);
			break;
		case LITMUS_CHOICE:
			{
				count = read_count(data, bytes, pos);
				if(count >= lit->children->count())
					validityerror("Choice selector out of range.");
				
				litmus *child = lit->children->item(count);
				node = do_unmarshall(data, bytes, child, symbol_table, pos);
			}
			break;
		case LITMUS_ENUM:
			{
				count = read_count(data, bytes, pos);
				if(count >= lit->values->count())
					validityerror("Enumeration value out of range.");
				node = new snode();
				node->type = SValue;
				node->n = count;
				node->s = sdup(lit->values->item(count));
			}
			break;
		default:
			error("Switch error.");
	}
	if(node != NULL)
	{
		// Set name string and symbol:
		if(node->name == NULL)
		{
			// Node hasn't been named yet (not true for OPT and CHOICE nodes):
			node->name = sdup(name);
		}
	}
	return node;
}
