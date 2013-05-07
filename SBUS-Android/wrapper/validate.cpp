// validate.cpp - DMI - 10-4-2007

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
#include "validate.h"

int do_validate(snode *node, litmus *lit, svector *symbol_table);
void validate_list(snode *node, litmus *lit, svector *symbol_table);
void validate_primitive(snode *node, litmus *lit, svector *symbol_table);
void validate_composite(snode *node, litmus *lit, svector *symbol_table);

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

int validate(snode *node, Schema *schema, const char **err)
{
	// Returns 1 if valid, else 0 and places an error message into err
	// Also makes types more specific, suitable for unambiguous marshalling
	
	litmus *lit = schema->tree;

	if(schema->hc->isempty())
	{
		if(node->get_type() == SEmpty)
		{
			// This is legitmate - schema and message are both empty
			return 1;
		}
		else
		{
			*err = sformat("Validation problem: non-empty message, but "
					"schema only accepts empty data");
			return 0;
		}
	}
	try
	{		
		do_validate(node, lit, schema->symbol_table);
	}
	catch(ValidityException e)
	{
		*err = sdup(e.msg);
		return 0;
	}	
	return 1;
}

int do_validate(snode *node, litmus *lit, svector *symbol_table)
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
				return 0;
			}
		}
		// Checks passed: names/types consistent with optional elt present:
		do_validate(node, content, symbol_table);
		return 1;
	}
		
	if(node->name != NULL && t != LITMUS_OPT && t != LITMUS_CHOICE)
	{
		// Check element name:
		
		if(strcmp(expected_name, "-") && strcmp(node->name, "-") &&
				node->name != expected_name && strcmp(node->name, expected_name))
		{
			validityerror("Expected '%s', found '%s'.", expected_name, node->name);
		}
	}
	
	if(t == LITMUS_INT || t == LITMUS_DBL || t == LITMUS_FLG ||
			t == LITMUS_TXT || t == LITMUS_BIN ||
			t == LITMUS_CLK || t == LITMUS_LOC)
		validate_primitive(node, lit, symbol_table);
	else if(t == LITMUS_LIST || t == LITMUS_SEQ || t == LITMUS_ARRAY)
		validate_list(node, lit, symbol_table);
	else
		validate_composite(node, lit, symbol_table);
	
	return 1;
}

void validate_list(snode *node, litmus *lit, svector *symbol_table)
{
	LitmusType formal = lit->type;
	NodeType actual = node->type;
	char *name = symbol_table->item(lit->namesym);
	int num_items;

	// printf("DBG validate_list %s\n", name);
	if(actual == SStruct)
	{
		// Type coercion:
		node->type = SList;
	}
	else if(actual != SList && actual != SEmpty)
		validityerror("Expected a list '%s'", name);
	
	if(actual == SEmpty)
	{
		// Type coercion:
		num_items = 0;
		node->type = SList;
		node->children = new snodevector();
	}
	else
		num_items = node->children->count();
	
	if(formal == LITMUS_ARRAY && num_items != lit->arraylen)
		validityerror("Incorrect array length for '%s'", name);
	for(int i = 0; i < num_items; i++)
		do_validate(node->children->item(i), lit->content, symbol_table);
}

void validate_composite(snode *node, litmus *lit, svector *symbol_table)
{
	LitmusType formal = lit->type;
	NodeType actual = node->type;
	char *name = symbol_table->item(lit->namesym);
	
	// printf("DBG validate_composite %s\n", name);
	switch(formal)
	{
		case LITMUS_ENUM:
			{
				cpvector *values = lit->values;
				
				if(actual == SText)
				{
					// Type coercion:
					node->type = SValue;
				}
				else if(actual != SValue)
				{
					validityerror("Expected an enumeration value for '%s',"
						" got type %s", name, node_type_to_string(actual));
				}
				
				node->n = -1;
				for(int i = 0; i < values->count(); i++)
				{
					if(!strcmp(values->item(i), node->s))
					{
						node->n = i;
						break;
					}
				}
				if(node->n == -1)
					validityerror("Invalid enumeration value '%s' for '%s'.",
							node->s, name);
			}
			break;
		case LITMUS_STRUCT:
			{
				if(actual != SStruct)
				{
					validityerror("Expected a structure for '%s', got type %s.",
							name, node_type_to_string(actual));
				}
				
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
					consumed = do_validate(children_actual->item(next_available),
						children_formal->item(i), symbol_table);
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
				do_validate(node, alternate, symbol_table);
			}
			break;
		default:
			error("Switch error.");
	}
}

void validate_primitive(snode *node, litmus *lit, svector *symbol_table)
{
	LitmusType formal = lit->type;
	NodeType actual = node->type;
	char *name = symbol_table->item(lit->namesym);

	// printf("DBG validate_primitive %s\n", name);
	switch(formal)
	{
		case LITMUS_INT:
			if(actual != SInt)
				validityerror("Expected an integer (%s).", name);
			break;
		case LITMUS_DBL:
			if(actual != SDouble)
				validityerror("Expected a double (%s).", name);
			break;
		case LITMUS_FLG:
			if(actual == SInt)
			{
				// Type coercion:
				node->type = SBool;
			}
			else if(actual != SBool)
				validityerror("Expected a boolean (%s).", name);
			if(node->n != 0 && node->n != 1)
				validityerror("Non-boolean value %d (%s).", node->n, name);
			break;
		case LITMUS_TXT:
			if(actual == SEmpty)
			{
				// Type coercion:
				node->type = SText;
				char *s = new char[1];
				s[0] = '\0';
				node->s = s;
			}
			else if(actual != SText)
				validityerror("Expected a string (%s).", name);
			break;
		case LITMUS_BIN:
			if(actual != SBinary)
				validityerror("Expected binary data (%s).", name);
			break;
		case LITMUS_CLK:
			if(actual != SDateTime)
				validityerror("Expected a timestamp (%s).", name);
			break;
		case LITMUS_LOC:
			if(actual != SLocation)
				validityerror("Expected a location (%s).", name);
			break;
		default:
			error("Switch error.");
	}
}
