// xml.cpp - DMI - 23-3-2007

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "error.h"
#include "datatype.h"
#include "dimension.h"
#include "builder.h"

typedef snode *snodeptr;

static void importerror(const char *format, ...)
{
	static const int MAX_ERR_LEN = 200;
	char c[MAX_ERR_LEN];
	va_list args;

	va_start(args, format);
	vsnprintf(c, MAX_ERR_LEN, format, args);
	va_end(args);
	c[MAX_ERR_LEN - 1] = '\0';
	
	throw ImportException(c);
}

int is_int(const char *s)
{
	if(s[0] == '-')
		s++;
	if(s[0] == '\0')
		return 0;
	while(s[0] != '\0')
	{
		if(!is_digit(s[0]))
			return 0;
		s++;
	}
	return 1;
}

int is_dbl(const char *s)
{
	// Pattern: [s]d+[.d+][e[s]d+]
	
	try
	{
		s = eat_optional_sign(s);
		s = eat_integer(s);
		s = eat_optional_fraction(s);
		if(s[0] == '\0')
			return 1;
		else if(s[0] == 'e')
		{
			s++;
			s = eat_optional_sign(s);
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

snode *infer_type(const char *s)
{
	/* Our input has already been stripped of whitespace, but may be quoted
		if it's a string */
	
	snode *sn;
	
	if(!strcmp(s, "true"))
	{
		sn = snode::create_bool(1);
	}
	else if(!strcmp(s, "false"))
	{
		sn = snode::create_bool(0);
	}
	else if(!strcmp(s, "-"))
	{
		sn = new snode(SNull);
	}
	else if(s[0] == '"')
	{
		char *str;
		
		int len = strlen(s);
		if(s[len - 1] != '"')
			error("Quoted string missing end quote.");
		str = sdup(s);
		str[len - 1] = '\0'; // Remove end quote
		sn = new snode(str + 1);
		delete str;
	}
	else if(s[0] == '#')
	{
		sn = snode::create_value(s + 1);
	}
	else if(s[0] == '0' && s[1] == 'x')
	{
		StringBuf *sb;
		char *data;
		int n;
		
		sb = new StringBuf();
		s += 2;
		s = eat_whitespace(s);
		while(1)
		{
			if(s[0] == '\n')
			{
				s++;
				s = eat_whitespace(s);
				if(s[0] == '0' && s[1] == 'x')
					s += 2;
			}
			n = read_hexbyte(s);
			if(n == -1)
				break;
			sb->cat((char)n);
			s += 2;
		}
		data = sb->extract();
		sn = new snode((void *)data, sb->length());
		delete data;
	}
	else if(s[0] != '-' && (s[0] < '0' || s[0] > '9'))
	{
		// Text
		sn = new snode(s);
	}
	else if(is_int(s))
	{
		int n;

		n = atoi(s);
		sn = new snode(n);
	}
	else if(is_dbl(s))
	{
		double d;
		
		d = strtod(s, NULL);
		sn = new snode(d);
	}
	else if(sdatetime::typematch(s))
	{
		// clk
		sdatetime *time;
		
		time = new sdatetime(s);
		sn = new snode(time);
	}
	else if(slocation::typematch(s))
	{
		// loc
		slocation *loc;
		
		loc = new slocation(s);
		sn = new snode(loc);
	}
	else
	{
		// Text
		sn = new snode(s);
	}
	
	return sn;
}

enum TagType
{
	TagOpening, TagClosing, TagEmpty, TagBody
};

class XMLTag
{
	public:

	XMLTag();
			
	void dump();
				
	TagType type;
	const char *data; // Element name, or content if TagBody
};

XMLTag::XMLTag()
{
	data = NULL;
}

void XMLTag::dump()
{
	switch(type)
	{
		case TagOpening:
			printf("<%s>", data); break;
		case TagClosing:
			printf("</%s>", data); break;
		case TagEmpty:
			printf("<%s/>", data); break;
		case TagBody:
			printf("%s", data); break;
		default:
			error("Unknown XML tag type in dump()");
	}
}

const char *read_tag_content(const char *s, StringBuf *sb)
{
	// Body data
	int escape_char = 0, inside_quotes = 0;
	
	while(1)
	{
		if(*s == '\0')
			importerror("End-of-file inside unterminated data");
		if(*s == '<')
		{
			if(escape_char || inside_quotes)
			{
				sb->cat('<');
				escape_char = 0;
			}
			else
				break;
		}
		else if(*s == '"')
		{
			sb->cat('"');
			if(escape_char)
				escape_char = 0;
			else
				inside_quotes = 1 - inside_quotes;
		}
		else if(*s == '\\')
		{
			if(escape_char)
			{
				sb->cat('\\');
				escape_char = 0;
			}
			else
				escape_char = 1;
		}
		else
			sb->cat(*s);
		s++;
	}
	sb->truncate_spaces();
	return s;
}

const char *read_tag_name(const char *s, StringBuf *sb)
{
	// Element name:
	while(*s != '>' && *s != '/' && *s != ' ' && *s != '\t' && *s != '\n')
	{
		if(*s == '\0')
			importerror("End-of-file in middle of tag");
		sb->cat(*s);
		s++;
	}
	s = eat_whitespace(s);
	if(*s != '>' && *s != '/')
		importerror("Tag name '%s' must not contain spaces", sb->extract());
	return s;
}

tagvector *recognise_tags(const char *s)
{
	tagvector *list;
	StringBuf *content;
	XMLTag *tag;

	list = new tagvector();
	content = new StringBuf();
	
	while(1)
	{
		s = eat_whitespace(s);
		if(*s == '\0')
			break; // End of document, mission complete
		tag = new XMLTag();
		content->clear();
		if(*s != '<')
		{
			tag->type = TagBody;
			// Read content:
			s = read_tag_content(s, content);
			tag->data = content->extract();
		}
		else
		{
			s++;
			s = eat_whitespace(s);
			if(*s == '/')
			{
				tag->type = TagClosing;
				s++;
				s = eat_whitespace(s);
			}
			else
			{
				tag->type = TagOpening;
				// Could also be TagEmpty, but we don't know that yet
			}
			// Read tag name:
			s = read_tag_name(s, content);
			tag->data = content->extract();
			
			// End of tag: > or />
			if(*s == '/')
			{
				if(tag->type == TagClosing)
				{
					importerror("Tag </%s/> is both empty and an end-tag",
							tag->data);
				}
				tag->type = TagEmpty;
				s++;
				s = eat_whitespace(s);
				if(*s != '>')
				{
					importerror("Empty tag <%s/ missing closing bracket",
							tag->data);
				}
			}
			s++; // Skip past '>'
			s = eat_whitespace(s);
		}
		if(tag->type == TagClosing && list->count() > 0)
		{
			XMLTag *prev = list->top();
			
			if(prev->type == TagOpening && !strcmp(prev->data, tag->data))
			{
				list->pop();
				
				sfree(prev->data);
				delete prev;
				
				tag->type = TagEmpty;
			}
		}
		list->add(tag);
	}
	
	delete content;
	return list;
}

/*
snode *snode::import_search_file(const char *path, const char **err)
{
	char *located;
	snode *sn;

	located = path_lookup(path);
	sn = import_file(located, err);
	delete[] located;
	
	return sn;
}
*/

snode *snode::import_file(const char *path, const char **err)
{
	// XML import - returns NULL (explanation in *err) on syntax error
	
	memfile *mf;
	snode *data;
	
	mf = new memfile(path);
	if(mf->data == NULL)
	{
		char *msg = new char[strlen(path) + 100];
		sprintf(msg, "Can't open file %s\n", path);
		*err = msg;
		return NULL;
	}
	data = snode::import(mf->data, err);
	delete mf;
	return data;
}

snode **snode::import_file_multi(const char *path, int *n, const char **err)
{
	// XML import - returns NULL (explanation in *err) on syntax error
	
	memfile *mf;
	snode **data;
	
	mf = new memfile(path);
	if(mf->data == NULL)
	{
		char *msg = new char[strlen(path) + 100];
		sprintf(msg, "Can't open file %s\n", path);
		*err = msg;
		return NULL;
	}
	data = snode::import_multi(mf->data, n, err);
	delete mf;
	return data;
}

snode *snode::import(const char *s, const char **err)
{
	snodevector *multi;
	snode *sn;
	
	if(!strcmp(s, "0")) // Empty message
	{
		sn = new snode(SNull);
		return sn;
	}
	
	multi = do_import(s, err, 1);
	if(multi == NULL)
		return NULL;
	sn = multi->item(0);
	delete multi;
	return sn;
}

snode **snode::import_multi(const char *s, int *n, const char **err)
{
	snodevector *multi;
	snode **snvec;
	
	multi = do_import(s, err, 0);
	if(multi == NULL)
		return NULL;
	*n = multi->count();
	snvec = new snodeptr[*n];
	for(int i = 0; i < *n; i++)
		snvec[i] = multi->item(i);
	delete multi;
	return snvec;
}

snodevector *snode::do_import(const char *s, const char **err, int single)
{
	// XML import - returns NULL (explanation in *err) on syntax error
	snodestack *stack;
	snode *sn, *container;
	snodevector *tops;
	tagvector *list;
	XMLTag *tag;
	const char *open_name; // Name of most recent open tag (not on stack yet)?
	int n;
	
	/* Note: in general XML, content could occur after an end or
		empty tag. In our version, content can only occur after
		an opening tag, since all primitive-type fields must be named
		and hence wrapped in a tag. */

	tops = new snodevector();
	try
	{
		list = recognise_tags(s);
		/*
		printf("DBG: %d tags:\n", list->count());
		for(int i = 0; i < list->count(); i++)
		{
			printf("%d. ", i);
			list->item(i)->dump();
			printf("\n");
		}
		*/
			
		stack = new snodestack();
		open_name = NULL;
		n = 0;
		while(n < list->count())
		{
			tag = list->item(n);
			n++;
			switch(tag->type)
			{
				case TagOpening:
					if(open_name != NULL)
					{
						sn = new snode();
						sn->type = SStruct;
						// Assume struct, until/if converted to list type by schema
						sn->name = open_name; // Will be deleted by snode::~snode
						sn->children = new snodevector();
						if(stack->items() == 0)
						{
							if(single && tops->count() > 0)
							{
								importerror("Document has multiple top-level tags,"
										" second is <%s>", open_name);
							}
							tops->add(sn);
						}
						else
						{
							container = stack->top();
							container->children->add(sn);
						}
						stack->push(sn);
					}
					open_name = tag->data; // Will be deleted later
					break;
				case TagClosing:
					if(open_name != NULL)
					{
						importerror("End tag has different name to start"
								"<%s>, and no content", open_name);
					}
					if(stack->items() == 0)
						importerror("End tag but no start");
					container = stack->top();
					if(strcmp(container->name, tag->data))
					{
						importerror("End tag <%s> has different name to start <%s>",
								tag->data, container->name);
					}
					stack->pop();
					open_name = NULL;
					sfree(tag->data);
					break;
				case TagBody:
					if(open_name == NULL)
						importerror("XML data must be wrapped by a tag");
					sn = infer_type(tag->data);
					sn->name = open_name; // Will be deleted by snode::~snode
					open_name = NULL;
					if(stack->items() == 0)
					{
						if(single && tops->count() > 0)
							importerror("Document has multiple top-level tags");
						tops->add(sn);
					}
					else
					{
						container = stack->top();
						container->children->add(sn);
					}
					sfree(tag->data);
					// Read corresponding end tag:
					if(n >= list->count())
						importerror("Missing end tag");
					delete tag;
					tag = list->item(n);
					n++;
					if(tag->type != TagClosing)
						importerror("Data must be followed by end tag");
					if(strcmp(sn->name, tag->data))
					{
						importerror("End tag <%s> has different name to start <%s>",
								tag->data, sn->name);
					}
					sfree(tag->data);
					break;
				case TagEmpty:
					if(open_name != NULL)
					{
						sn = new snode();
						sn->type = SStruct;
						// Assume struct, until/if converted to list type by schema
						sn->name = open_name; // Will be deleted by snode::~snode
						sn->children = new snodevector();
						if(stack->items() == 0)
						{
							if(single && tops->count() > 0)
							{
								importerror("Document has multiple top-level tags,"
										" second is <%s>", open_name);
							}
							tops->add(sn);
						}
						else
						{
							container = stack->top();
							container->children->add(sn);
						}
						stack->push(sn);
					}
					sn = new snode();
					sn->type = SStruct;				
					sn->name = tag->data; // Will be deleted by snode::~snode
					sn->children = new snodevector();
					open_name = NULL;
					if(stack->items() == 0)
					{
						if(single && tops->count() > 0)
							importerror("Document has multiple top-level tags");
						tops->add(sn);
					}
					else
					{
						container = stack->top();
						container->children->add(sn);
					}
					break;
				default:
					error("Unknown tag type");
			}
			delete tag;
		}
		delete list;

		// Check stack of open tags is empty:
		if(open_name != NULL)
		{
			delete[] open_name;
			importerror("Not all tags properly closed.");
		}
		if(stack->items() != 0)
			importerror("Not all tags properly closed.");
		if(tops->count() == 0)
			importerror("XML document is empty");
	}
	catch(ImportException e)
	{
		*err = sdup(e.msg);
		delete tops;
		delete stack;
		return NULL;
	}
	if(single && tops->count() != 1)
	{
		error("Failed sanity check in do_import: multiple top-level nodes "
				"in XML.");
	}
	delete stack;
	return tops;
}

void snode::spaces(int n, StringBuf *sb)
{
	if(n < 1) // -1 or 0
		return;
	for(int i = 0; i < n * 3; i++)
		sb->cat(' ');
}

char *snode::toxml(int pretty)
{
	char *s;
	StringBuf *sb;
	
	if(type == SEmpty)
	{
		s = new char[2];
		s[0] = '0';
		s[1] = '\0';
		return s;
	}
	sb = new StringBuf();
	append(sb, pretty ? 0 : -1);
	s = sb->extract();
	delete sb;
	return s;
}

int snode::string_needs_quotes(const char *s)
{
	if(s[0] == '-' || s[0] == ' ' || s[0] == '#' || s[0] == '\0')
		return 1;
	if(s[0] >= '0' && s[0] <= '9')
		return 1;
	if(s[strlen(s) - 1] == ' ')
		return 1;
	if(!strcmp(s, "true") || !strcmp(s, "false"))
		return 1;
	return 0;
}

int snode::string_is_multiline(const char *s)
{
	while(1)
	{
		if(*s == '\n')
			return 1;
		if(*s == '\0')
			break;
		s++;
	}
	return 0;
}

const char *snode::escape_string(const char *s)
{
	StringBuf *sb = new StringBuf();
	const char *t;
	char c;
	int quotes;
	int len = strlen(s);
	
	quotes = string_needs_quotes(s);
	if(quotes)
		sb->cat('"');
	for(int i = 0; i < len; i++)
	{
		c = s[i];
		if((quotes || i == 0) && c == '"')
		{
			sb->cat('\\');
			sb->cat('"');
		}
		else if(!quotes && c == '<')
		{
			sb->cat('\\');
			sb->cat('<');
		}
		else if(c == '\\')
		{
			sb->cat('\\');
			sb->cat('\\');
		}
		else
			sb->cat(c);
	}
	if(quotes)
		sb->cat('"');
	
	t = sb->extract();
	delete sb;
	return t;
}

void snode::append(StringBuf *sb, int offset)
{
	const char *name = name_string();
	const char *repr;
	
	spaces(offset, sb);
	if(type == SList && children->count() == 0)
	{
		sb->cat('<');
		sb->cat(name);
		sb->cat("/>");
		if(offset != -1)
			sb->cat('\n');
		return;
	}
	sb->cat('<');
	sb->cat(name);
	sb->cat('>');
	switch(type)
	{
		case SEmpty:
			sb->cat('-');
			break;
		case SInt:
			sb->cat_int(n);
			break;
		case SDouble:
			sb->cat(x);
			break;
		case SText:
			{
				int multiline = string_is_multiline(s);
				
				if(multiline)
					sb->cat('\n');
				repr = escape_string(s);
				sb->cat(repr);
				delete repr;
				if(multiline)
					sb->cat('\n');
			}
			break;
		case SBinary:
			sb->cat('\n');
			spaces(offset + 1, sb);
			sb->cat("0x");
			for(int i = 0; i < len; i++)
			{
				if(i % 16 == 15 && i != len - 1)
				{
					sb->cat('\n');
					spaces(offset + 1, sb);
					sb->cat("0x");
				}
				sb->cat_hexbyte(((unsigned char *)data)[i]);
			}
			sb->cat('\n');
			spaces(offset, sb);
			break;
		case SBool:
			if(n == 1)
				sb->cat("true");
			else if(n == 0)
				sb->cat("false");
			else
				error("Invalid boolean value.");
			break;
		case SDateTime:
			repr = time->tostring();
			sb->cat(repr);
			delete repr;
			break;
		case SLocation:
			repr = loc->tostring();
			sb->cat(repr);
			delete repr;
			break;
		case SStruct:
			if(offset != -1)
				sb->cat('\n');
			// printf("Structure (%s):\n", name);
			for(int i = 0; i < children->count(); i++)
				children->item(i)->append(sb, offset == -1 ? -1 : offset + 1);
			spaces(offset, sb);
			break;
		case SList:
			if(offset != -1)
				sb->cat('\n');
			// printf("List (%s):\n", name);
			for(int i = 0; i < children->count(); i++)
				children->item(i)->append(sb, offset == -1 ? -1 : offset + 1);
			spaces(offset, sb);
			break;
		case SValue:
			sb->cat('#');
			if(s != NULL)
				sb->cat(s);
			else
				sb->cat_int(n);
			break;
		default:
			error("Impossible node type.");
	}
	sb->cat("</");
	sb->cat(name);
	sb->cat('>');
	if(offset != -1)
		sb->cat('\n');
}
