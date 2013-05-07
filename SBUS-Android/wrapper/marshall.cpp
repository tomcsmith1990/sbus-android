// marshall.cpp - DMI - 20-2-2007

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

// TODO - Expand hyphens ASAP - XXXXX

int do_marshall(snode *node, litmus *lit, StringBuf *sb,
	svector *symbol_table);
void marshall_list(snode *node, litmus *lit, StringBuf *sb,
	svector *symbol_table);
void marshall_primitive(snode *node, litmus *lit, StringBuf *sb,
	svector *symbol_table);

typedef unsigned char uchar;

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

unsigned char *marshall(snode *node, Schema *schema, int *length,
		const char **err)
{
	StringBuf *sb = new StringBuf();
	litmus *lit = schema->tree;
	unsigned char *s;

	if(sizeof(double) != 8 || sizeof(int) != 4 || sizeof(float) != 4)
		error("Word sizes unsuitable on this architecture.");

	if(schema->hc->isempty())
	{
		if(node->get_type() == SEmpty)
		{
			// This is legitmate - schema and message are both empty
			s = new uchar[1]; // Can't return NULL, so make shortest poss. array
			*length = 0;
			return s;
		}
		else
		{
			*err = sformat("Marshalling problem: non-empty message, but "
					"schema only accepts empty data");
			return NULL;
		}
	}
	try
	{		
		do_marshall(node, lit, sb, schema->symbol_table);
	}
	catch(ValidityException e)
	{
		*err = sformat("Marshalling problem:\n%s", e.msg);
		return NULL;
	}
	
	s = (unsigned char *)sb->extract();
	*length = sb->length();
	delete sb;
	return s;
}

int do_marshall(snode *node, litmus *lit, StringBuf *sb,
		svector *symbol_table)
{
	// Returns 1 if node consumed, 0 if not
	LitmusType t = lit->type;
	const char *expected_name;
	
	if(lit->namesym != -1)
		expected_name = symbol_table->item(lit->namesym);
	else
		expected_name = "";

	if(t == LITMUS_OPT)
	{
		// Possibility of element not being present:
		
		if(node->type == SEmpty)
		{
			sb->cat(0);
			return 1;
		}
			
		litmus *content = lit->content;
		const char *inner_name;
		if(content->namesym != -1)
			inner_name = symbol_table->item(content->namesym);
		else
			inner_name = "";
		
		if(node->name != NULL && strcmp(inner_name, "-"))
		{
			// Check names:
			if(node->name != inner_name && strcmp(node->name, inner_name))
			{
				// Names differ; optional element not present:
				sb->cat(0);
				return 0;
			}
		}
		else
		{
			// Check types:
			int fail = 0;
			switch(content->type)
			{
				case LITMUS_INT:
					if(node->type != SInt) fail = 1; break;
				case LITMUS_DBL:
					if(node->type != SDouble) fail = 1; break;
				case LITMUS_FLG:
					if(node->type != SBool && node->type != SInt) fail = 1; break;
				case LITMUS_TXT:
					if(node->type != SInt) fail = 1; break;
				case LITMUS_BIN:
					if(node->type != SBinary) fail = 1; break;
				case LITMUS_CLK:
					if(node->type != SDateTime) fail = 1; break;
				case LITMUS_LOC:
					if(node->type != SLocation) fail = 1; break;
				case LITMUS_STRUCT:
					if(node->type != SStruct) fail = 1; break;
				case LITMUS_LIST:
					if(node->type != SList && node->type != SStruct) fail = 1; break;
				case LITMUS_SEQ:
					if(node->type != SList && node->type != SStruct) fail = 1; break;
				case LITMUS_ARRAY:
					if(node->type != SList && node->type != SStruct) fail = 1; break;
				case LITMUS_OPT:
					error("Nested options prohibited at present.");
				case LITMUS_CHOICE:
					error("Choice within option prohibited at present.");
				case LITMUS_ENUM:
					if(node->type != SValue && node->type != SInt &&
							node->type != SText) fail = 1; break;
				default:
					error("Switch error.");
			}
			if(fail)
			{
				sb->cat(0);
				return 0;
			}
		}
		// Checks passed: names/types consistent with optional elt present:
		sb->cat(1);
		do_marshall(node, content, sb, symbol_table);
		return 1;
	}
		
	if(node->name != NULL && t != LITMUS_OPT && t != LITMUS_CHOICE)
	{
		// Check element name
		if(strcmp(expected_name, "-") && strcmp(node->name, "-") &&
				node->name != expected_name && strcmp(node->name, expected_name))
		{
			validityerror("Expected '%s', found '%s'.", expected_name, node->name);
		}
	}
	if((node->name == NULL || !strcmp(node->name, "-")) &&
			t != LITMUS_OPT && t != LITMUS_CHOICE &&
			strcmp(expected_name, "-") && expected_name[0] != '\0')
	{
		// Assign a proper name to this unnamed node, from the schema
		sfree(node->name);
		node->name = sdup(expected_name);
	}
	
	if(t == LITMUS_INT || t == LITMUS_DBL || t == LITMUS_FLG ||
			t == LITMUS_TXT || t == LITMUS_BIN ||
			t == LITMUS_CLK || t == LITMUS_LOC)
		marshall_primitive(node, lit, sb, symbol_table);
	else if(t == LITMUS_LIST || t == LITMUS_SEQ || t == LITMUS_ARRAY)
		marshall_list(node, lit, sb, symbol_table);
	else
		marshall_composite(node, lit, sb, symbol_table);
	
	return 1;
}

void marshall_list(snode *node, litmus *lit, StringBuf *sb,
		svector *symbol_table)
{
	LitmusType formal = lit->type;
	NodeType actual = node->type;
	int num_items;

	if(actual == SStruct)
	{
		// Type coercion:
		actual = SList;
	}
	if(actual != SList && actual != SEmpty)
		validityerror("Expected a list.");
	if(actual == SEmpty)
		num_items = 0;
	else
		num_items = node->children->count();
	if(formal == LITMUS_ARRAY)
	{
		if(num_items != lit->arraylen)
			validityerror("Incorrect array length.");
	}
	else
		sb->cat(num_items); // Count
	for(int i = 0; i < num_items; i++)
	{
		do_marshall(node->children->item(i), lit->content, sb,
			symbol_table);
	}
}

void marshall_composite(snode *node, litmus *lit, StringBuf *sb,
		svector *symbol_table)
{
	LitmusType formal = lit->type;
	NodeType actual = node->type;
	char *name = symbol_table->item(lit->namesym);

	switch(formal)
	{
		case LITMUS_ENUM:
			{
				cpvector *values = lit->values;
				int select = -1;
				
				if(actual == SText)
				{
					// Type coercion:
					actual = SValue;
				}
				if(actual != SValue)
					validityerror("Expected an enumeration value for '%s'", name);
				for(int i = 0; i < values->count(); i++)
				{
					if(!strcmp(values->item(i), node->s))
					{
						select = i;
						break;
					}
				}
				if(select == -1)
					validityerror("Invalid enumeration value '%s' for '%s'",
							node->s, name);
				sb->cat(select);
			}
			break;
		case LITMUS_STRUCT:
			{
				if(actual != SStruct)
					validityerror("Expected a structure for '%s'.", name);
				
				litmusvector *children_formal = lit->children;
				snodevector *children_actual = node->children;
				int next_available = 0;
				int consumed;
				
				for(int i = 0; i < children_formal->count(); i++)
				{
					if(next_available == children_actual->count())
					{
						validityerror("Insufficient elements in structure '%s'.",
								name);
					}
					consumed = do_marshall(children_actual->item(next_available),
						children_formal->item(i), sb, symbol_table);
					if(consumed)
						next_available++;
				}
				if(next_available != children_actual->count())
					validityerror("Too many elements in structure '%s'.", name);
			}
			break;
		case LITMUS_CHOICE:
			{
				litmus *alternate;
				int select = -1;
				
				for(int i = 0; i < lit->children->count(); i++)
				{
					alternate = lit->children->item(i);
					if(node->name != NULL && alternate->namesym != -1)
					{
						// Compare names:
						if(!strcmp(node->name,
								symbol_table->item(alternate->namesym)))
						{
							// Match:
							select = i;
							break;
						}
					}
					else
					{
						// Consider types:
						switch(alternate->type)
						{
   						case LITMUS_INT:
								if(node->type == SInt)
									select = i;
								break;
							case LITMUS_DBL:
								if(node->type == SDouble)
									select = i;
								break;
							case LITMUS_FLG:
								if(node->type == SBool || node->type == SInt)
									select = i;
								break;
							case LITMUS_TXT:
								if(node->type == SText || node->type == SEmpty)
									select = i;
								break;
							case LITMUS_BIN:
								if(node->type == SBinary)
									select = i;
								break;
							case LITMUS_CLK:
								if(node->type == SDateTime)
									select = i;
								break;
							case LITMUS_LOC:
								if(node->type == SLocation)
									select = i;
								break;
							case LITMUS_STRUCT:
								if(node->type == SStruct)
									select = i;
								break;
							case LITMUS_LIST:
								if(node->type == SStruct || node->type == SList
										|| node->type == SEmpty)
									select = i;
								break;
							case LITMUS_SEQ:
								if(node->type == SStruct || node->type == SList)
									select = i;
								break;
							case LITMUS_ARRAY:
								if(node->type == SStruct || node->type == SList)
									select = i;
								break;
							case LITMUS_OPT:
								error("Option within a choice too complex to parse.");
								break;
							case LITMUS_CHOICE:
								error("Choice within a choice too complex to parse.");
								break;
							case LITMUS_ENUM:
								if(node->type == SInt || node->type == SText ||
										node->type == SValue)
									select = i;
								break;
							default:
									error("Switch error.");
						}
						if(select != -1)
							break;
					}
				}
				if(select == -1)
				{
					validityerror("None of the alternate options matched for "
							"choice element '%s'.", name);
				}
				sb->cat(select);
				do_marshall(node, alternate, sb, symbol_table);
			}
			break;
		default:
			error("Switch error.");
	}
}

void marshall_primitive(snode *node, litmus *lit, StringBuf *sb,
		svector *symbol_table)
{
	LitmusType formal = lit->type;
	NodeType actual = node->type;
	char *name = symbol_table->item(lit->namesym);
	unsigned char *buf;

	switch(formal)
	{
		case LITMUS_INT:
			if(actual != SInt)
				validityerror("Expected an integer (%s).", name);
			sb->cat((char)(node->n >> 24));
			sb->cat((char)((node->n >> 16) & 0xFF));
			sb->cat((char)((node->n >> 8) & 0xFF));
			sb->cat((char)(node->n & 0xFF));
			break;
		case LITMUS_DBL:
			if(actual != SDouble)
				validityerror("Expected a double (%s).", name);
			sb->cat(&node->x, 8);
			break;
		case LITMUS_FLG:
			if(actual == SInt)
			{
				// Type coercion:
				actual = SBool;
			}
			if(actual != SBool)
				validityerror("Expected a boolean (%s).", name);
			if(node->n != 0 && node->n != 1)
				validityerror("Non-boolean value %d (%s).", node->n, name);
			sb->cat((char)(node->n));
			break;
		case LITMUS_TXT:
			{
				int len;
				
				if(actual == SEmpty)
				{
					sb->cat(0); // Count
					break;
				}
				if(actual != SText)
					validityerror("Expected a string (%s).", name);
				len = strlen(node->s);
				sb->cat(len); // Count
				sb->cat(node->s, len);
			}
			break;
		case LITMUS_BIN:
			if(actual != SBinary)
				validityerror("Expected binary data (%s).", name);
			sb->cat(node->len); // Count
			sb->cat(node->data, node->len);
			break;
		case LITMUS_CLK:
			if(actual != SDateTime)
				validityerror("Expected a timestamp (%s).", name);
			buf = node->time->encode();
			sb->cat(buf, 8);
			delete[] buf;
			break;
		case LITMUS_LOC:
			if(actual != SLocation)
				validityerror("Expected a location. (%s)", name);
			buf = node->loc->encode();
			sb->cat(buf, 12);
			delete[] buf;
			break;
		default:
			error("Switch error.");
	}
}
