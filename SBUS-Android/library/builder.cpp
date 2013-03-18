// builder.cpp - DMI - 17-2-2007

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "datatype.h"
#include "dimension.h"
#include "builder.h"
#include "error.h"

typedef snode *snodeptr;

const char *SNull = NULL;

const char *node_type_to_string(NodeType t)
{
	const char *s = "Unknown";
	
	switch(t)
	{
		case SInt: s = "Int"; break;
		case SDouble: s = "Double"; break;
		case SText: s = "Text"; break;
		case SBinary: s = "Binary"; break;
		case SBool: s = "Bool"; break;
		case SDateTime: s = "DateTime"; break;
		case SLocation: s = "Location"; break;
		case SStruct: s = "Struct"; break;
		case SList: s = "List"; break;
		case SValue: s = "Value"; break;
		case SEmpty: s = "Empty"; break;
		default:
			error("Unknown node type %d", t);
			break;
	}
	return s;
}

/* snode */

NodeType snode::get_type()
{
	return type;
}

snode::snode()
{
	// type not specified yet
	initialise();
}

void snode::initialise()
{
	s = NULL;
	n = -1;
	data = NULL;
	name = NULL;
	children = NULL;
	time = NULL;
	loc = NULL;
}

snode::~snode()
{
	sfree(name);
	sfree(s);
	sfree(data);
	if(children != NULL)
	{
		for(int i = 0; i < children->count(); i++)
			delete children->item(i);
		delete children;
	}
	if(time != NULL)
		delete time;
	if(loc != NULL)
		delete loc;
}

snode::snode(snode *sn, const char *override_name)
{
	// Cloning operation:
	type = sn->type;
	name = override_name == NULL ? sdup(sn->name) : sdup(override_name);
	n = sn->n;
	x = sn->x;
	if(sn->s == NULL)
		s = NULL;
	else
		s = sdup(sn->s);
	len = sn->len;
	if(sn->data != NULL)
	{
		data = new char[len];
		memcpy(data, sn->data, len);
	}
	else
		data = NULL;
	if(sn->children != NULL)
	{
		children = new snodevector();
		for(int i = 0; i < sn->children->count(); i++)
			children->add(new snode(sn->children->item(i)));
	}
	else
		children = NULL;

	time = ((sn->time == NULL) ? NULL : new sdatetime(sn->time));
	loc = ((sn->loc == NULL) ? NULL : new slocation(sn->loc));	
}

void snode::spaces(int n)
{
	for(int i = 0; i < n * 3; i++)
		printf(" ");
}

const char *snode::name_string()
{
	static char buf[40];
	
	if(name != NULL)
		return name;
	buf[0] = '-';
	buf[1] = '\0';
	return buf;
}

void snode::dump_text(const char *s, int offset)
{
	char c;
	
	if(!string_is_multiline(s))
	{
		while(*s == ' ' || *s == '\t')
			s++;
		printf("%s", s);
		return;
	}
	printf("\n");
	spaces(offset + 1);
	printf(">");
	while(1)
	{
		c = *s;
		if(c == '\0')
			break;
		if(c == '\n')
		{
			printf("\n");
			spaces(offset + 1);
			printf(">");
			s++;
			/*
			while(*s == ' ' || *s == '\t')
				s++;
			*/
		}
		else
		{
			if(c == '\t')
				printf("   ");
			else
				printf("%c", c);
			s++;
		}
	}
}

void snode::dump(int offset)
{
	const char *repr;
	
	spaces(offset);
	switch(type)
	{
		case SInt:
			printf("Integer (%s): %d\n", name_string(), n);
			break;
		case SDouble:
			printf("Double (%s): %g\n", name_string(), x);
			break;
		case SText:
			printf("Text (%s): ", name_string());
			dump_text(s, offset);
			printf("\n");
			break;
		case SEmpty:
			printf("Empty (%s)\n", name_string());
			break;
		case SBinary:
			printf("Binary (%s): [%d bytes]\n", name_string(), len);
			break;
		case SBool:
			if(n < 0 || n > 1)
				error("Boolean out of range");
			printf("Boolean (%s): %s\n", name_string(), n == 1 ? "true" : "false");
			break;
		case SDateTime:
			repr = time->tostring();
			printf("Datetime (%s): %s\n", name_string(), repr);
			delete repr;
			break;
		case SLocation:
			repr = loc->tostring();
			printf("Location (%s): %s\n", name_string(), repr);
			delete repr;
			break;
		case SStruct:
			printf("Structure (%s):\n", name_string());
			for(int i = 0; i < children->count(); i++)
				children->item(i)->dump(offset + 1);
			break;
		case SList:
			printf("List (%s):\n", name_string());
			for(int i = 0; i < children->count(); i++)
				children->item(i)->dump(offset + 1);
			break;
		case SValue:
			printf("Enumeration (%s): ", name_string());
			if(s != NULL)
				printf("#%s, index %d\n", s, n);
			else
				printf("#%d\n", n);
			break;
		default:
				error("Impossible node type.");
	}
}

snode::snode(int n, const char *opt_name)
{
	initialise();
	type = SInt; // Assume int until converted by schema
	this->n = n;
	name = sdup(opt_name);
}

snode::snode(const char *s, const char *opt_name)
{
	initialise();
	if(s == SNull)
	{
		type = SEmpty;
		this->s = NULL;
	}
	else
	{
		type = SText; // Assume txt until converted by schema
		this->s = sdup(s);
	}
	name = sdup(opt_name);
}

snode::snode(const void *data, int len, const char *opt_name)
{
	initialise();
	type = SBinary;
	name = sdup(opt_name);
	this->data = new char[len];
	memcpy(this->data, data, len);
	this->len = len;
}

snode::snode(double x, const char *opt_name)
{
	initialise();
	type = SDouble;
	name = sdup(opt_name);
	this->x = x;
}

snode::snode(sdatetime *time, const char *opt_name)
{
	initialise();
	type = SDateTime;
	name = sdup(opt_name);
	this->time = new sdatetime(time);
}

snode::snode(slocation *loc, const char *opt_name)
{
	initialise();
	type = SLocation;
	name = sdup(opt_name);
	this->loc = new slocation(loc);
}

snode *snode::create_bool(int n, const char *opt_name)
{
	snode *sn = new snode();
	sn->type = SBool;
	sn->n = n;
	sn->name = sdup(opt_name);
	return sn;
}

snode *snode::create_value(const char *s, const char *opt_name)
{
	snode *sn = new snode();
	sn->type = SValue;
	sn->s = sdup(s);
	sn->name = sdup(opt_name);
	return sn;
}

/* Packing primitive types */

snode *pack(int n, const char *opt_name)
{
	// int
	snode *node = new snode(n, opt_name);
	return node;
}

snode *pack_bool(int n, const char *opt_name)
{
	// flg
	return snode::create_bool(n, opt_name);
}

snode *pack(const char *s, const char *opt_name)
{
	// txt, value; assume txt until converted by schema
	snode *node = new snode(s, opt_name);
	return node;
}

snode *pack(const void *data, int len, const char *opt_name)
{
	// bin
	snode *node = new snode(data, len, opt_name);
	return node;
}

snode *pack(double x, const char *opt_name)
{
	// dbl
	snode *node = new snode(x, opt_name);
	return node;
}

snode *pack(sdatetime *time, const char *opt_name)
{
	// datetime
	snode *node = new snode(time, opt_name);
	return node;
}

snode *pack(slocation *loc, const char *opt_name)
{
	// location
	snode *node = new snode(loc, opt_name);
	return node;
}

/* Packing composite types (struct, lists) */

snode *mklist(const char *opt_name)
{
	// Assume struct until possibly converted to list type by schema
	snode *node = new snode();
	node->type = SStruct;
	node->name = sdup(opt_name);
	node->children = new snodevector();
	return node;
}

snode *snode::append(snode *n)
{
	if(children == NULL)
		error("Internal error: tried to append to non-list.");
	children->add(n);
	return this;
}

snode *pack(snode **array, int n, const char *opt_name)
{
	// Assume struct, will probably be converted to list type by schema
	snode *node = new snode();
	node->type = SStruct;
	node->name = sdup(opt_name);
	node->children = new snodevector();
	for(int i = 0; i < n; i++)
		node->children->add(array[i]);
	return node;
}

//Singular pack
snode *pack(snode *n1, const char *opt_name){
	// Assume struct until possibly converted to list type by schema
	snode *node = new snode();
	node->type = SStruct;
	node->name = sdup(opt_name);
	node->children = new snodevector();
	node->children->add(n1);
	return node;
}

snode *pack(snode *n1, snode *n2, const char *opt_name)
{
	// Assume struct until possibly converted to list type by schema
	snode *node = new snode();
	node->type = SStruct;
	node->name = sdup(opt_name);
	node->children = new snodevector();
	node->children->add(n1);
	node->children->add(n2);
	return node;
}

snode *pack(snode *n1, snode *n2, snode *n3, const char *opt_name)
{
	// Assume struct until possibly converted to list type by schema
	snode *node = new snode();
	node->type = SStruct;
	node->name = sdup(opt_name);
	node->children = new snodevector();
	node->children->add(n1);
	node->children->add(n2);
	node->children->add(n3);
	return node;
}

snode *pack(snode *n1, snode *n2, snode *n3, snode *n4, const char *opt_name)
{
	// Assume struct until possibly converted to list type by schema
	snode *node = new snode();
	node->type = SStruct;
	node->name = sdup(opt_name);
	node->children = new snodevector();
	node->children->add(n1);
	node->children->add(n2);
	node->children->add(n3);
	node->children->add(n4);
	return node;
}

snode *pack(snode *n1, snode *n2, snode *n3, snode *n4, snode *n5,
		const char *opt_name)
{
	// Assume struct until possibly converted to list type by schema
	snode *node = new snode();
	node->type = SStruct;
	node->name = sdup(opt_name);
	node->children = new snodevector();
	node->children->add(n1);
	node->children->add(n2);
	node->children->add(n3);
	node->children->add(n4);
	node->children->add(n5);
	return node;
}

snode *pack(snode *n1, snode *n2, snode *n3, snode *n4, snode *n5,
		snode *n6, const char *opt_name)
{
	// Assume struct until possibly converted to list type by schema
	snode *node = new snode();
	node->type = SStruct;
	node->name = sdup(opt_name);
	node->children = new snodevector();
	node->children->add(n1);
	node->children->add(n2);
	node->children->add(n3);
	node->children->add(n4);
	node->children->add(n5);
	node->children->add(n6);
	return node;
}

snode *pack(snode *n1, snode *n2, snode *n3, snode *n4, snode *n5,
		snode *n6, snode *n7, const char *opt_name)
{
	// Assume struct until possibly converted to list type by schema
	snode *node = new snode();
	node->type = SStruct;
	node->name = sdup(opt_name);
	node->children = new snodevector();
	node->children->add(n1);
	node->children->add(n2);
	node->children->add(n3);
	node->children->add(n4);
	node->children->add(n5);
	node->children->add(n6);
	node->children->add(n7);
	return node;
}

/* Extracting */

int snode::count()
{
	if(children != NULL)
		return children->count();
	return -1;
}

snode *snode::extract_item(int item)
{
	return indirect(item);
}

snode *snode::extract_item(const char *name)
{
	return indirect(name);
}

snode **snode::extract_array()
{
	snode **array;
	int size;
	
	if(children == NULL)
		return NULL;
	size = children->count();
	array = new snodeptr[size];
	for(int i = 0; i < size; i++)
		array[i] = children->item(i);
	return array;
}

const char *snode::get_name(int opt_item)
{
	return indirect(opt_item)->name;
}

/* "item" may be -1 for the current item, >= 0 for a child index */

snode *snode::indirect(int item)
{
	snode *node;
	
	if(item == -1)
	{
		return this;
	}
	else if(item >= 0)
	{
		if(type != SStruct && type != SList)
		{
			error("Extracting by index from a type <%s> with no sub-items.",
					name);
		}
		if(item >= children->count())
		{
			error("Sub-item index for extraction out of range in element <%s>.",
					name);
		}
		node = children->item(item);
	}
	else
		error("Invalid sub-item indirection.");
	return node;
}

snode *snode::indirect(const char *name)
{
	snode *node;
	
	if(type != SStruct)
	{
		error("Extracting by name <%s> from a non-structure type <%s>", name,
				this->name);
	}
	if(name == NULL)
		error("Selected NULL sub-item.");
	// Fast check:
	for(int i = 0; i < children->count(); i++)
	{
		node = children->item(i);
		if(node->name == name)
			return node;
	}
	// Slow check:
	for(int i = 0; i < children->count(); i++)
	{
		node = children->item(i);
		if(!strcmp(node->name, name))
			return node;
	}
	error("No sub-item with specified name '%s' in structure '%s'", name,
			this->name);
	return NULL; // Never happens
}

int snode::extract_int(int opt_item)
{
	if(opt_item == -1)
	{
		if(type != SInt)
		{
			error("Type mismatch: type %s for element %s is not an integer.",
					node_type_to_string(type), get_name());
		}
		return n;
	}
	return (indirect(opt_item)->extract_int());
}

int snode::extract_flg(int opt_item)
{
	if(opt_item == -1)
	{
		if(type != SBool)
		{
			error("Type mismatch: type %s for element %s is not a boolean.",
					node_type_to_string(type), get_name());
		}
		if(n != 0 && n != 1)
			error("Boolean out of range.");
		return n;
	}
	return (indirect(opt_item)->extract_flg());
}

double snode::extract_dbl(int opt_item)
{
	if(opt_item == -1)
	{
		if(type != SDouble)
		{
			error("Type mismatch: type %s for element %s is not floating point.",
					node_type_to_string(type), get_name());
		}
		return x;
	}
	return (indirect(opt_item)->extract_dbl());
}

const char *snode::extract_txt(int opt_item)
{
	if(opt_item == -1)
	{
		if(type != SText)
		{
			error("Type mismatch: type %s for element %s is not textual.",
					node_type_to_string(type), get_name());
		}
		return s;
	}
	return (indirect(opt_item)->extract_txt());
}

const void *snode::extract_bin(int opt_item)
{
	if(opt_item == -1)
	{
		if(type != SBinary)
		{
			error("Type mismatch: type %s for element %s is not binary.",
					node_type_to_string(type), get_name());
		}
		return data;
	}
	return (indirect(opt_item)->extract_bin());
}

int snode::num_bytes(int opt_item)
{
	if(opt_item == -1)
	{
		if(type != SBinary)
		{
			error("Type mismatch: type %s for element %s is not binary.",
					node_type_to_string(type), get_name());
		}
		return len;
	}
	return (indirect(opt_item)->num_bytes());
}

sdatetime *snode::extract_clk(int opt_item)
{
	if(opt_item == -1)
	{
		if(type != SDateTime)
		{
			error("Type mismatch: type %s for element %s is not a timestamp.",
					node_type_to_string(type), get_name());
		}
		return time;
	}
	return (indirect(opt_item)->extract_clk());
}

slocation *snode::extract_loc(int opt_item)
{
	if(opt_item == -1)
	{
		if(type != SLocation)
		{
			error("Type mismatch: type %s for element %s is not a location.",
					node_type_to_string(type), get_name());
		}
		return loc;
	}
	return (indirect(opt_item)->extract_loc());
}

const char *snode::extract_value(int opt_item)
{
	if(opt_item == -1)
	{
		if(type != SValue)
		{
			error("Type mismatch: type %s for element %s is not an enumeration.",
					node_type_to_string(type), get_name());
		}
		return s;
	}
	return (indirect(opt_item)->extract_value());
}

int snode::extract_enum(int opt_item)
{
	if(opt_item == -1)
	{
		if(type != SValue)
		{
			error("Type mismatch: type %s for element %s is not an enumeration.",
					node_type_to_string(type), get_name());
		}
		return n;
	}
	return (indirect(opt_item)->extract_enum());
}

int snode::extract_int(const char *name)
{
	return (indirect(name)->extract_int());
}

int snode::extract_flg(const char *name)
{
	return (indirect(name)->extract_flg());
}

double snode::extract_dbl(const char *name)
{
	return (indirect(name)->extract_dbl());
}

const char *snode::extract_txt(const char *name)
{
	return (indirect(name)->extract_txt());
}

const void *snode::extract_bin(const char *name)
{
	return (indirect(name)->extract_bin());
}

int snode::num_bytes(const char *name)
{
	return (indirect(name)->num_bytes());
}

sdatetime *snode::extract_clk(const char *name)
{
	return (indirect(name)->extract_clk());
}

slocation *snode::extract_loc(const char *name)
{
	return (indirect(name)->extract_loc());
}

const char *snode::extract_value(const char *name)
{
	return (indirect(name)->extract_value());
}

int snode::extract_enum(const char *name)
{
	return (indirect(name)->extract_enum());
}

int snode::equals(snode *sn)
{
	// Returns -1 if incomparable due to types
	int i;
	
	if(type != sn->type)
		return -1;
	switch(type)
	{
		case SInt:
			return (n == sn->n);
			break;
		case SDouble:
			return (x == sn->x);
			break;
		case SText:
			return (strcmp(s, sn->s) == 0);
			break;
		case SBinary:
			if(len != sn->len)
				return 0;
			return (memcmp(data, sn->data, len) == 0);
			break;
		case SBool:
			return (n == sn->n);
			break;
		case SDateTime:
			return (time->compare(sn->time) == 0);
			break;
		case SLocation:
			return loc->equals(sn->loc);
			break;
		case SStruct:
			if(children->count() != sn->children->count())
				return 0;
			for(i = 0; i < children->count(); i++)
				if(!children->item(i)->equals(sn->children->item(i)))
					return 0;
			return 1;
			break;
		case SList:
			if(children->count() != sn->children->count())
				return 0;
			for(i = 0; i < children->count(); i++)
				if(!children->item(i)->equals(sn->children->item(i)))
					return 0;
			return 1;
			break;
		case SValue:
			if(n != -1 && sn->n != -1)
			{
				// Fast comparison:
				return (n == sn->n);
			}
			else if(s != NULL && sn->s != NULL)
			{
				if(s == sn->s)
					return 1; // Semi-fast comparison
				
				// Slow comparison:
				return (strcmp(s, sn->s) == 0);
			}
			else
			{
				// This can probably happen, but we should fix it so it can't:
				error("Cannot compare enumerations in numeric and string form");
			}
			break;
		case SEmpty:
			return 1; // Empty things are equal
			break;
		default:
			error("switch error in node comparison");
	}
	return -1;
}

int snode::lessthan(snode *sn)
{
	// Returns -1 if incomparable due to types
	
	if(type != sn->type)
		return -1;
	switch(type)
	{
		case SInt:
			return (n < sn->n);
			break;
		case SDouble:
			return (x < sn->x);
			break;
		case SText:
			return (strcmp(s, sn->s) < 0);
			break;
		case SBinary:
			return -1; // No comparison function defined on arbitrary bits
			break;
		case SBool:
			return -1; // Doesn't make sense to order just true and false
			break;
		case SDateTime:
			return (time->compare(sn->time) < 0);
			break;
		case SLocation:
			return -1; // 2D/3D space isn't ordered
			break;
		case SStruct:
			return -1; // Structures aren't comparable
			break;
		case SList:
			// We interpret this as comparing list *length*:
			return (children->count() < sn->children->count());
			break;
		case SValue:
			if(n != -1 && sn->n != -1)
			{
				// Fine, have numeric values for both, can do comparison:
				return (n < sn->n);
			}
			else
			{
				// This can probably happen, but we should fix it so it can't:
				error("Enumerations must be numeric for order comparisons");
			}
			break;
		case SEmpty:
			return -1; // No order relation on empty things
			break;
		default:
			error("switch error in node comparison");
	}
	return -1;
}

int snode::exists(const char *name)
{
	snode *node;
	
	if(type != SStruct || name == NULL)
		return 0;
	// Fast check:
	for(int i = 0; i < children->count(); i++)
	{
		node = children->item(i);
		if(node->name == name)
			return (node->type == SEmpty ? 0 : 1);
	}
	// Slow check:
	for(int i = 0; i < children->count(); i++)
	{
		node = children->item(i);
		if(!strcmp(node->name, name))
			return (node->type == SEmpty ? 0 : 1);
	}
	return 0;
}

snode *snode::find(const char *name)
{
	snode *node;
	
	if(type != SStruct || name == NULL)
		return 0;
	// Fast check:
	for(int i = 0; i < children->count(); i++)
	{
		node = children->item(i);
		if(node->name == name && node->type != SEmpty)
			return node;
	}
	// Slow check:
	for(int i = 0; i < children->count(); i++)
	{
		node = children->item(i);
		if(!strcmp(node->name, name) && node->type != SEmpty)
			return node;
	}
	for (int i = 0; i < children->count(); i++)
	{
		node = children->item(i);
		snode *found = node->find(name);
		if (found != NULL) return found;
	}
	return NULL;
}


snode *snode::follow_path(svector *path)
{
	// Returns NULL if no such path
	snode *sn = this;
	snodevector *children;
	const char *branch;
	int index;
	
	if(sn->type != SStruct && sn->type != SList && path->count() == 1 &&
			!strcmp(sn->name, path->item(0)))
	{
		// Single element with matching name, that's OK:
		return sn;
	}
	for(int i = 0; i < path->count(); i++)
	{
		if(sn->type != SStruct && sn->type != SList)
			return NULL;
		children = sn->children;
		
		branch = path->item(i);
		if(branch[0] == '#')
		{
			index = atoi(branch + 1);
			if(index < 0 || index >= children->count())
				return NULL;
			sn = children->item(index);
		}
		else
		{
			snode *subn;
			
			sn = NULL;
			for(int j = 0; j < children->count(); j++)
			{
				subn = children->item(j);
				if(!strcmp(subn->name, branch))
				{
					sn = subn;
					break;
				}
			}
			if(sn == NULL)
				return NULL;
		}
	}
	return sn;
}
