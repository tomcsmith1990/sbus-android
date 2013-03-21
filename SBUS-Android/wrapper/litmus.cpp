// litmus.cpp - DMI - 31-1-2007

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "../library/error.h"
#include "../library/datatype.h"
#include "../library/dimension.h"
#include "../library/builder.h"
#include "../library/hash.h"
#include "litmus.h"

const char *token_to_str(Token token);

void Schema::synerror(int line, const char *msg)
{
	throw SchemaException(msg, line);
}

void Schema::synerror(const char *format, ...)
{
	static const int MAX_ERR_LEN = 200;
	char c[MAX_ERR_LEN];
	va_list args;

	va_start(args, format);
	vsnprintf(c, MAX_ERR_LEN, format, args);
	va_end(args);
	c[MAX_ERR_LEN - 1] = '\0';
	
	throw SchemaException(c, -1);
}

void Schema::dump_tree(int initial_indent, int log)
{
	StringBuf *sb;
	sb = new StringBuf;
	switch(meta)
	{
		case SCHEMA_NORM:
			if(tree == NULL) // Paranoia
				error("Can't dump_tree() when tree is uninitialised");
			dump_litmus(sb, tree, initial_indent, 1);
			break;
		case SCHEMA_EMPTY:
			sb->cat_spaces(initial_indent * 3);
			sb->cat("empty\n");
			break;
		case SCHEMA_POLY:
			sb->cat_spaces(initial_indent * 3);
			sb->cat("polymorphic\n");
			break;
		case SCHEMA_EXPR:
			sb->cat_spaces(initial_indent * 3);
			sb->cat("expression\n");
			break;
		case SCHEMA_NA:
			sb->cat_spaces(initial_indent * 3);
			sb->cat("inapplicable\n");
			break;
		default: error("Impossible switch error in dump_tree");
	}
	if(log)
		sb->log();
	else
		sb->print();
	delete sb;
}

int Schema::construct_lookup(Schema *target, snode *constraints, snode *lookup_forward, snode *lookup_backward, pvector *extra, svector *path)
{
	if (target == NULL || constraints == NULL || lookup_forward == NULL || lookup_backward == NULL)
		return -1;

	/*
	 * Check the schema matches the constraints, and construct lookup table for:
	 * 		- any fields mentioned in a constraint.
	 *		- any fields which are children of those constraints.
	 */
	int match = construct_lookup(constraints, hashes->extract_item(0), target->hashes, lookup_forward, lookup_backward);

	if (!match)
		return match;
	
	// Find the path from the top level to the first constraint in converting to schema.
	svector *target_path = new svector();
	target->hashes->find(constraints->extract_item(0)->extract_txt("name"), target_path);
	
	// Find the path from the top level to the first constraint in converting from schema.
	svector *current_path = new svector();
	hashes->find(lookup_forward->extract_txt(constraints->extract_item(0)->extract_txt("name")), current_path);

	const char *target_name, *current_name;

	// Working backwards up the path (i.e. from constraint to the top), 
	// add levels to the lookup tables until we reach the top of one of the schemas.
	int count = (target_path->count() <= current_path->count()) ? target_path->count() : current_path->count();
	int i;
	for (i = 0; i < count; i++)
	{
		target_name = target_path->item(i);
		current_name = current_path->item(i);
		
		if (!lookup_forward->exists(target_name))
			lookup_forward->append(pack(current_name, target_name));
			
		if (!lookup_backward->exists(current_name))
			lookup_backward->append(pack(target_name, current_name));
	}
	
	// If the schema we're converting to has the longer path,
	// save the rest of our path - this is read off to construct dummy outer layers.
	if (i < target_path->count())
	{
		for (int j = target_path->count() - 1; j >= i; j--)
		{
			svector *foo = new svector();
			//path->add(target_path->item(j));
			foo->add(target_path->item(j));
			snode *sn = target->hashes->find(target_path->item(j));
			// -2 because of "has" and "similar".
			for (int k = 0; k < sn->count() - 2; k++)
				foo->add(sn->extract_item(k)->get_name());
			extra->add((void *)foo);
		}
	}
	// If the schema we're converting from has the longer path,
	// save the rest of their path - this path is followed to find the top level node to repack.
	else if (i < current_path->count())
	{
		for (int j = current_path->count() - 1; j > i; j--)
			path->add(current_path->item(j));
		path->add(current_path->item(i - 1));
	}
	
	delete target_path;
	delete current_path;

	return match;
}

void Schema::construct_lookup(snode *want, snode *have, snode *lookup_forward, snode *lookup_backward)
{
	if (want == NULL || have == NULL || want->count() != have->count())
		return;

	if (!lookup_forward->exists(want->get_name()))
		lookup_forward->append(pack(have->get_name(), want->get_name())); // <my-name>their-name</my-name>

	if (!lookup_backward->exists(have->get_name()))
	{
		lookup_backward->append(pack(want->get_name(), have->get_name())); // <their-name>my-name</their-name>
	}

	// -2 because we don't want to do a lookup for the "has" and "similar" hash fields.
	for (int i = 0; i < want->count() - 2; i++)
	{
		construct_lookup(want->extract_item(i), have->extract_item(i), lookup_forward, lookup_backward);
	}
	return;
}

int Schema::construct_lookup(snode *want, snode *have, snode *target_hashes, snode *lookup_forward, snode *lookup_backward)
{
	/*
	 *	Returns 1 if the schema (in hash format) matches the constraints we have, 0 if not.
	 *	Will also fill lookup_forward and lookup_backward as lookup tables against target_hashes, if all are not NULL.
	 *	This lookup table will contain all of the matched field names, as well as any children of that field.
	 */
	 
	// If there are no constraints, we match.
	if (want->count() == 0) return 1;

	int match = 0;
	snode *constraint;
	int exact;
	const char *hash;

	// Loop through all of the hash constraints.
	for (int i = 0; i < want->count(); i++)
	{
		match = 0;
		
		constraint = want->extract_item(i);
		exact = constraint->extract_flg("exact");
		hash = constraint->extract_txt("hash");

		// If 'have' has a hash.
		if (have->exists((exact) ? "has" : "similar"))
		{	
			// If it's the hash we want.
			if (!strcmp(hash, have->extract_txt((exact) ? "has" : "similar")))
			{
				if (exact)
				{
					// If this is an exact match, no need to check the children.
					match = 1;
				}
				
				// If there are constraints within this one.
				else if (constraint->exists("children"))
				{
					// If there are no fields within 'have', we definitely fail.
					if (have->count() == 0)
						match = 0;
					else
					{
						// For each child constraint.
						for (int j = 0; j < constraint->extract_item("children")->count(); j++)
						{
							snode *sn = mklist("schema");
							sn->append(constraint->extract_item("children")->extract_item(j));
							
							// Try to match within any of the children of 'have'.
							for (int k = 0; k < have->count(); k++)
							{
								match = construct_lookup(sn, have->extract_item(k), target_hashes, lookup_forward, lookup_backward);
								// If we've matched, don't bother checking other children of 'have'.
								if (match)
									break;
							}
							
							// If we haven't matched, don't both checking other child constraints.
							if (!match)
								break;
						}
					}	
				}
				
				else
				{
					// If there are no child constraints and it's similar, we have a match.
					match = 1;
				}
				
				// If this matches, move on to the next constraint.
				if (match)
				{
					if (target_hashes != NULL && lookup_forward != NULL && lookup_backward != NULL)
					{
						snode *want;
						if (!strcmp(target_hashes->get_name(), constraint->extract_txt("name")))
							want = target_hashes;
						else 
							want = target_hashes->find(constraint->extract_txt("name"));
						
						construct_lookup(want, have, lookup_forward, lookup_backward);

						continue;
					}
				}
			}
		}

		// Still no match - depth first check the children of 'have'.
		for (int j = 0; j < have->count(); j++)
		{
			snode *sn = mklist("schema");
			sn->append(constraint);
			// If constraint is matched somewhere in a child, check next constraint.
			if (construct_lookup(sn, have->extract_item(j), target_hashes, lookup_forward, lookup_backward))
			{
				match = 1;
				break;
			}
		}
		
		// If we still haven't matched this constraint, we fail.
		if (!match) break;
	}
	
	return match;
}

int Schema::match_constraints(snode *constraints)
{
	return construct_lookup(constraints, hashes, NULL, NULL, NULL);
}

char *Schema::canonical_string()
{
	StringBuf *sb;
	char *s;
	
	sb = new StringBuf;
	switch(meta)
	{
		case SCHEMA_NORM:
			dump_litmus(sb, tree, 0, 1, hashes, new StringBuf());
			break;
		case SCHEMA_EMPTY:
			sb->cat("0");
			break;
		case SCHEMA_POLY:
			sb->cat("*");
			break;
		case SCHEMA_EXPR:
			sb->cat("?");
			break;
		case SCHEMA_NA:
			sb->cat("!");
			break;
		default: error("Impossible switch error in canonical_string");
	}
	s = sb->extract();
	delete sb;
	return s;
}

char *Schema::orig_string()
{
	return source->extract();
}

void Schema::dump_litmus(StringBuf *sb, litmus *l, int offset, int defn, snode *sn, StringBuf *tsb)
{
	// If sn is not NULL, we build up an snode matching this schema, where each field contains its hash.
	const char *name;
	
	if(l->type == LITMUS_INT || l->type == LITMUS_DBL ||
			l->type == LITMUS_FLG || l->type == LITMUS_TXT ||
			l->type == LITMUS_BIN || l->type == LITMUS_CLK ||
			l->type == LITMUS_LOC)
	{
		sb->cat_spaces(offset * 3);
		if(defn) sb->cat('@');
		StringBuf *sub = new StringBuf();
		StringBuf *tbuf = new StringBuf();
		switch(l->type)
		{
   		case LITMUS_INT: tbuf->cat("int"); break;
			case LITMUS_DBL: tbuf->cat("dbl"); break;
			case LITMUS_FLG: tbuf->cat("flg"); break;
			case LITMUS_TXT: tbuf->cat("txt"); break;
   		case LITMUS_BIN: tbuf->cat("bin"); break;
			case LITMUS_CLK: tbuf->cat("clk"); break;
			case LITMUS_LOC: tbuf->cat("loc"); break;
			default:
				error("Impossible switch error in dump_litmus");
		}
		
		sub->append(tbuf);
		
		name = symbol_table->item(l->namesym);
		sub->cat(' ');
		sub->cat(name);

		// Add the hashes of this field.
		if (sn != NULL)
		{	
		
			const char *subschema;
			HashCode *hash = new HashCode();
			
			snode *list = mklist(symbol_table->item(l->namesym));
			
			subschema = tbuf->extract();
			hash->fromschema(subschema);
			list->append(pack(hash->tostring(), "similar"));
			if (tsb != NULL)
			{
				tsb->append(tbuf);
				tsb->cat('\n');
			}
		
			subschema = sub->extract();
			hash->fromschema(subschema);
			list->append(pack(hash->tostring(), "has"));

			sn->append(list);
			
			delete hash;
			delete subschema;
		}
		delete tbuf;

		sb->append(sub);
		
		delete sub;
		
		sb->cat('\n');
	}
	else if(l->type == LITMUS_STRUCT)
	{
		sb->cat_spaces(offset * 3);
		// Create a separate StringBuf and append, so we can get hashes for substructures.
		StringBuf *sub = new StringBuf();
		StringBuf *tbuf = new StringBuf();
		
		if(defn) sb->cat('@');
		sub->cat(symbol_table->item(l->namesym));
		sub->cat('\n');
		sub->cat_spaces(offset * 3);
		sub->cat("{\n");

		tbuf->cat('\n');
		tbuf->cat_spaces(offset * 3);
		tbuf->cat("{\n");
		
		snode *list = (sn == NULL) ? NULL : mklist(symbol_table->item(l->namesym));
		for(int i = 0; i < l->children->count(); i++)
		{
			dump_litmus(sub, l->children->item(i), offset + 1, 0, list, tbuf);
		}
		sub->cat_spaces(offset * 3);
		sub->cat("}\n");

		tbuf->cat_spaces(offset * 3);
		tbuf->cat("}\n");
		
		// Add the hashes of this field.
		if (sn != NULL)
		{
			const char *subschema;
			HashCode *hash = new HashCode();
			
			subschema = tbuf->extract();
			hash->fromschema(subschema);
			list->append(pack(hash->tostring(), "similar"));
			if (tsb != NULL)
			{
				tsb->append(tbuf);
			}
		
			subschema = sub->extract();
			hash->fromschema(subschema);
			list->append(pack(hash->tostring(), "has"));

			sn->append(list);
			
			delete hash;
			delete subschema;
		}
		delete tbuf;
		
		sb->append(sub);
		delete sub;
	}
	else if(l->type == LITMUS_LIST || l->type == LITMUS_SEQ ||
		l->type == LITMUS_ARRAY)
	{
		sb->cat_spaces(offset * 3);
		// Create a separate StringBuf and append, so we can get hashes for substructures.
		StringBuf *sub = new StringBuf();
		StringBuf *tbuf = new StringBuf();
		
		if(defn) sb->cat('@');
		sub->cat(symbol_table->item(l->namesym));
		sub->cat('\n');
		sub->cat_spaces(offset * 3);
		if(l->type == LITMUS_SEQ)
			sub->cat("(+\n");
		else if(l->type == LITMUS_LIST)
			sub->cat("(\n");
		else
			sub->catf("(%d\n", l->arraylen);
			
		tbuf->cat('\n');
		tbuf->cat_spaces(offset * 3);
		
		if(l->type == LITMUS_SEQ)
			tbuf->cat("(+\n");
		else if(l->type == LITMUS_LIST)
			tbuf->cat("(\n");
		else
			tbuf->catf("(%d\n", l->arraylen);

		snode *list = (sn == NULL) ? NULL : mklist(symbol_table->item(l->namesym));
		dump_litmus(sub, l->content, offset + 1, 0, list, tbuf);
		sub->cat_spaces(offset * 3);
		sub->cat(")\n");
	
		tbuf->cat_spaces(offset * 3);
		tbuf->cat(")\n");

		// Add the hashes of this field.
		if (sn != NULL)
		{
			const char *subschema;
			HashCode *hash = new HashCode();
			
			subschema = tbuf->extract();
			hash->fromschema(subschema);
			list->append(pack(hash->tostring(), "similar"));
			if (tsb != NULL)
			{
				tsb->append(tbuf);
			}
		
			subschema = sub->extract();
			hash->fromschema(subschema);
			list->append(pack(hash->tostring(), "has"));

			sn->append(list);
			
			delete hash;
			delete subschema;
		}
		delete tbuf;
		
		sb->append(sub);
		delete sub;
	}
	else if(l->type == LITMUS_OPT)
	{
		sb->cat_spaces(offset * 3);
	
		// Create a separate StringBuf and append, so we can get hashes for substructures.
		StringBuf *sub = new StringBuf();
		StringBuf *tbuf = new StringBuf();
	
		if(defn) sb->cat('@');
		sub->cat("[\n");
		tbuf->cat("[\n");
		snode *list = (sn == NULL) ? NULL : mklist(symbol_table->item(l->namesym));
		dump_litmus(sub, l->content, offset + 1, 0, list, tbuf);
		sub->cat_spaces(offset * 3);
		sub->cat("]\n");
		tbuf->cat_spaces(offset * 3);
		tbuf->cat("]\n");
		
		// Add the hashes of this field.
		if (sn != NULL)
		{
			const char *subschema;
			HashCode *hash = new HashCode();
			
			subschema = tsb->extract();
			hash->fromschema(subschema);
			list->append(pack(hash->tostring(), "similar"));
			if (tsb != NULL)
			{
				tsb->append(tbuf);
			}
		
			subschema = sub->extract();
			hash->fromschema(subschema);
			list->append(pack(hash->tostring(), "has"));

			sn->append(list);
			
			delete hash;
			delete subschema;
		}
		delete tbuf;
		
		sb->append(sub);
		delete sub;
	}
	else if(l->type == LITMUS_CHOICE)
	{
		sb->cat_spaces(offset * 3);
		
		// Create a separate StringBuf and append, so we can get hashes for substructures.
		StringBuf *sub = new StringBuf();
		StringBuf *tbuf = new StringBuf();
		
		if(defn) sb->cat('@');
		sub->cat("<\n");
		tbuf->cat("<\n");
		snode *list = (sn == NULL) ? NULL : mklist(symbol_table->item(l->namesym));
		for(int i = 0; i < l->children->count(); i++)
		{
			dump_litmus(sub, l->children->item(i), offset + 1, 0, list, tbuf);
		}
		sub->cat_spaces(offset * 3);
		sub->cat(">\n");
		
		tbuf->cat_spaces(offset * 3);
		tbuf->cat(">\n");
		
		const char *subschema = sub->extract();
		HashCode *hash = new HashCode();
		hash->fromschema(subschema);
		
		// Add the hashes of this field.
		if (sn != NULL)
		{
			const char *subschema;
			HashCode *hash = new HashCode();
			
			subschema = tsb->extract();
			hash->fromschema(subschema);
			list->append(pack(hash->tostring(), "similar"));
			if (tsb != NULL)
			{
				tsb->append(tbuf);
			}
		
			subschema = sub->extract();
			hash->fromschema(subschema);
			list->append(pack(hash->tostring(), "has"));

			sn->append(list);
			
			delete hash;
			delete subschema;
		}
		delete tbuf;
		
		sb->append(sub);
		delete sub;
	}
	else if(l->type == LITMUS_ENUM)
	{
		sb->cat_spaces(offset * 3);
		
		// Create a separate StringBuf and append, so we can get hashes for substructures.
		StringBuf *sub = new StringBuf();
		StringBuf *tbuf = new StringBuf();
		
		if(defn) sb->cat('@');
		sub->cat(symbol_table->item(l->namesym));
		sub->cat(' ');
		tbuf->cat("< ");
		for(int i = 0; i < l->values->count(); i++)
		{
			tbuf->catf("#%s ", l->values->item(i));
		}
		tbuf->cat(">\n");
		
		sub->append(tbuf);
		
		// Add the hashes of this field.
		if (sn != NULL)
		{		
			const char *subschema;
			HashCode *hash = new HashCode();
			
			snode *list = mklist(symbol_table->item(l->namesym));
			
			subschema = tbuf->extract();
			hash->fromschema(subschema);
			list->append(pack(hash->tostring(), "similar"));
			if (tsb != NULL)
			{
				tsb->append(tbuf);
			}
		
			subschema = sub->extract();
			hash->fromschema(subschema);
			list->append(pack(hash->tostring(), "has"));

			sn->append(list);
			
			delete hash;
			delete subschema;
		}
		delete tbuf;
		
		sb->append(sub);
		delete sub;
	}
}

char *path_lookup(const char *filename)
{
	#ifndef __ANDROID__
	const char *default_litmus_dir = "/usr/local/share/litmus";
	#else
	const char *default_litmus_dir = "/data/data/uk.ac.cam.tcs40.sbus.sbus/files/idl";
	#endif
	char *litmus_path;
	stringsplit *ss;
	char *test;

	if(fexists(filename))
		return sdup(filename);
		
	litmus_path = getenv("LITMUS_PATH");

	if(litmus_path != NULL)
	{
		ss = new stringsplit(litmus_path, ',');
		for(int i = 0; i < ss->count(); i++)
		{
			test = sjoin(ss->item(i), '/', filename);
			if(fexists(test))
			{
				delete ss;
				return test;
			}
			delete[] test;
		}
		delete ss;
	}
	
	// Try default locations (N.B. already checked current dir):
	char *home, *path;

	// Home LITMUS directory:
	home = gethomedir();
	path = sjoin(home, '/', "share/litmus");
	test = sjoin(path, '/', filename);
	delete[] path;
	delete[] home;
	if(fexists(test))
		return test;
	delete[] test;

	// System LITMUS directory:
	test = sjoin(default_litmus_dir, '/', filename);
	if(fexists(test))
		return test;
	delete[] test;
	
	/* OK, it doesn't exist, so we're going to get an error trying
		to open it, hence we'll return the original file name so that
		a subsequent error message can be more explicit: */
	return sdup(filename);
}

Schema *Schema::load(const char *pathname, const char **err)
{
	Schema *sch;
	memfile *mf;
	char *located;

	sch = new Schema();
	located = path_lookup(pathname);
	mf = new memfile(located);
	delete[] located;
	if(mf->data == NULL)
	{
		*err = sformat("Can't open schema from file %s\n", pathname);
		delete sch;
		delete mf;
		return NULL;
	}
	if(sch->read(mf->data, err) < 0)
	{
		delete sch;
		delete mf;
		return NULL;
	}
	delete mf;
	
	return sch;
}

Schema *Schema::load(memfile *mf, const char **err)
{
	Schema *sch = new Schema();
	if(sch->read(mf->data, err) < 0)
	{
		delete sch;
		return NULL;
	}
	return sch;
}

Schema *Schema::create(const char *s, const char **err)
{
	Schema *sch = new Schema();
	if(sch->read(s, err) < 0)
	{
		delete sch;
		return NULL;
	}
	return sch;
}

MetaType get_metatype(const char *schema_source)
{
	const char *s = schema_source;
	char c;
	
	// Check for special cases 0, *, ! and ?
	while(*s == ' ' || *s == '\t' || *s == '\n') s++;
	c = *s++;
	if(c == '0' || c == '*' || c == '?' || c == '!')
	{
		while(*s == ' ' || *s == '\t' || *s == '\n') s++;
		if(*s == '\0')
		{
			switch(c)
			{
				case '0': return SCHEMA_EMPTY;
				case '*': return SCHEMA_POLY;
				case '?': return SCHEMA_EXPR;
				case '!': return SCHEMA_NA;
				default: error("Impossible switch error in get_metatype()");
			}
		}
	}
	return SCHEMA_NORM;
}

int Schema::read(const char *s, const char **err)
{
	source = new StringBuf();
	source->cat(s);
	hc = new HashCode();	

	meta = get_metatype(s);	
	if(meta != SCHEMA_NORM)
	{
		hc->frommeta(meta);
		return 0; // OK, it's special, so we're done
	}
	
	tokens = new litmustokenvector();

	symbol_table->add("-"); // The unnamed element is symbol zero
	try
	{	
		lex();
		build_sections();
		scan_sections();
		parseloop();
	}
	catch(SchemaException e)
	{
		*err = sformat("Problem parsing schema:\n%s\n", e.msg);
		return -1;
	}

	// Compute hash code:
	char *canon;
	canon = canonical_string();
	hc->fromschema(canon);
	delete[] canon;
	
	return 0;
}

Schema::Schema()
{
	hc = NULL;
	symbol_table = new svector();
	hashes = mklist("hashes");
	tokens = NULL;
	tree = NULL;
	source = NULL;
	sections = NULL;
}

Schema::Schema(Schema *sch) // Makes a copy (costly)
{
	hc = new HashCode(sch->hc);
	meta = sch->meta;
	
	symbol_table = new svector();
	for(int i = 0; i < sch->symbol_table->count(); i++)
		symbol_table->add(sch->symbol_table->item(i));
		
	hashes = new snode(sch->hashes);
	
	if(sch->tree == NULL)
		tree = NULL;
	else
		tree = sch->tree->clone();	
	
	source = new StringBuf(sch->source);

	// Don't bother copying low-level stuff; should be unnecessary after init:	
	tokens = NULL;
	sections = NULL;
}

Schema::~Schema()
{
	tokensection *tsect;
	
	if(symbol_table != NULL) delete symbol_table;
	if(hashes != NULL) delete hashes;
	if(tokens != NULL)
	{
		for(int i = 0; i < tokens->count(); i++)
			delete tokens->item(i);
		delete tokens;
	}
	if(sections != NULL)
	{
		for(int i = 0; i < sections->count(); i++)
		{
			tsect = sections->item(i);
			delete tsect->token;
			// Individual tokens in "token" will have been deleted above
			delete tsect->dependencies;
			delete tsect;
		}	
		delete sections;
	}
	if(tree != NULL) delete tree;
	if(hc != NULL) delete hc;
	if(source != NULL) delete source;
}

void Schema::build_sections()
{
	tokensection *tsect;
	litmustoken *tk;
	int len;
	
	sections = new tokensectionvector();
	
	len = tokens->count();
	if(tokens->item(0)->token != TK_AT)
		synerror("Schema did not start with an '@'");
	tsect = new tokensection();
	for(int i = 1; i < len; i++)
	{
		tk = tokens->item(i);
		if(tk->token == TK_AT)
		{
			tsect->token->add(new litmustoken(TK_EOF, 0, 0));
			sections->add(tsect);
			tsect = new tokensection();
		}
		else if(tk->token == TK_EOF)
			break;
		else
			tsect->token->add(tk);
	}
	// Add final section
	if(tsect->token->count() != 0)
	{
		tsect->token->add(new litmustoken(TK_EOF, 0, 0));
		sections->add(tsect);
	}
}

void Schema::scan_sections()
{
	tokensection *tsect;
	litmustoken *tk;
	int id;
	
	// printf("%d sections\n", sections->count());
	for(int i = 0; i < sections->count(); i++)
	{
		tsect = sections->item(i);
		/*
		printf("Section %d:\n", i + 1);
		do_dump_tokens(tsect->token);
		*/
		for(int j = 0; j < tsect->token->count(); j++)
		{
			tk = tsect->token->item(j);
			if(tk->token == TK_CARET)
			{
				if(j == tsect->token->count() - 1)
					synerror("Caret at end of section");
				j++;
				tk = tsect->token->item(j);
				if(tk->token != TK_NAME)
					synerror("Caret not followed by a name");
				id = tk->param;
				tsect->dependencies->add_unique(id);
			}
		}
		/*
		printf("Section %d has %d dependencies:\n", i + 1,
				tsect->dependencies->count());
		for(int j = 0; j < tsect->dependencies->count(); j++)
		{
			printf("%d) %s\n", j,
					symbol_table->item(tsect->dependencies->item(j)));
		}
		*/
	}
}

void Schema::parseloop()
{
	int sects = sections->count();
	tokensection *tsect, *root;
	int n, id;
	
	root = sections->item(0);
	while(root->parsed == NULL)
	{
		// Find an unparsed tokensection with no dependencies:
		for(n = 0; n < sects; n++)
		{
			tsect = sections->item(n);
			if(tsect->parsed == NULL && tsect->dependencies->count() == 0)
				break;
		}
		if(n == sects)
		{
			// Run out of things we know how to parse, and root still not done:
			StringBuf *err = new StringBuf();
			char *s;
			
			err->cat("One of these dependent types is unresolved:\n");
			for(int j = 0; j < root->dependencies->count(); j++)
			{
				err->cat(symbol_table->item(root->dependencies->item(j)));
				err->cat(' ');
			}
			s = err->extract();
			delete err;
			synerror(s);
		}
		
		// Parse it:
		pool = tsect->token;
		pos = 0;
		tsect->parsed = parse(NULL);
		if(next_token() != TK_EOF)
			synerror("Unexpected characters past end of schema");
		if(tsect->parsed->type == LITMUS_OPT ||
				tsect->parsed->type == LITMUS_CHOICE)
		{
			synerror("Cannot create top-level option or choice types");
		}
		
		// Remove its label namesym from all dependency lists:
		id = tsect->parsed->namesym;
		for(int j = 0; j < sects; j++)
			sections->item(j)->dependencies->remove(id);
	}
	tree = root->parsed;
}

Token Schema::next_token()
{
	if(pos >= pool->count())
		error("Error: ran out of tokens.");
	return pool->item(pos)->token;
}

Token Schema::lookahead()
{
	if(pos + 1 >= pool->count())
		error("Error: ran out of tokens.");
	return pool->item(pos + 1)->token;
}

int Schema::next_value()
{
	if(pos >= pool->count())
		error("Error: ran out of token values.");
	return pool->item(pos)->param;
}

int Schema::next_lineno()
{
	if(pos >= pool->count())
		error("Error: ran out of token values.");
	return pool->item(pos)->lineno;
}

void Schema::advance()
{
	pos++;
}

int Schema::multiple_names()
{
	if(lookahead() == TK_PLUS)
		return 1;
	return 0;
}

litmus *Schema::parse(litmusvector *multi)
{
	// If multi == NULL return elt, else add elt[s] to multi
	litmus *l = NULL;
	intvector *list;
	Token tok;
	int id;
	
	tok = next_token();
	if(tok == TK_NAME)
	{
		if(multiple_names())
		{
			litmus *lcopy;
			
			if(multi == NULL)
			{
				synerror("Multiple declaration not wrapped by structure"
						" on line %d", next_lineno());
			}
			list = parse_names();
			
			l = parse_type();
			for(int i = 0; i < list->count(); i++)
			{
				lcopy = l->clone();
				lcopy->namesym = list->item(i);
				multi->add(lcopy);
			}
			delete list;
			delete l;
			l = NULL;
		}
		else
		{
			id = next_value();
			advance();
			
			l = parse_type();
			if(l->type == LITMUS_OPT || l->type == LITMUS_CHOICE)
			{
				synerror("Optional and choice blocks cannot be named (line %d)",
						next_lineno());
			}
			if(id == 0 && l->namesym != -1)
			{
				// Unnamed element, with a named type
			}
			else
				l->namesym = id;
			if(multi != NULL)
				multi->add(l);
		}
	}
	else
	{
		l = parse_type();
		if(l->type != LITMUS_OPT && l->type != LITMUS_CHOICE)
		{
			if(multiple_names())
			{
				litmus *lcopy;
				
				if(multi == NULL)
				{
					synerror("Multiple declaration not wrapped by structure"
							" on line %d", next_lineno());
				}
				list = parse_names();
				for(int i = 0; i < list->count(); i++)
				{
					lcopy = l->clone();
					lcopy->namesym = list->item(i);
					multi->add(lcopy);
				}
				delete list;
				delete l;
				l = NULL;
			}
			else
			{
				if(next_token() != TK_NAME)
				{
					synerror("Missing name following type on line %d",
							next_lineno());
				}
				if(next_value() == 0 && l->namesym != -1)
				{
					// Unnamed element, with a named type
				}
				else
					l->namesym = next_value();
				advance();
				if(multi != NULL)
					multi->add(l);
			}
		}
		else
		{
			if(multi != NULL)
				multi->add(l);
		}
	}
	return l;
}

litmus *Schema::parse_type()
{
	litmus *l = NULL;
	Token tok = next_token();
	
	if(tok == TK_INT || tok == TK_DBL || tok == TK_FLG || tok == TK_TXT
		|| tok == TK_BIN || tok == TK_CLK || tok == TK_LOC)
	{
		switch(tok)
		{
			case TK_INT: l = new litmus(LITMUS_INT); break;
			case TK_DBL: l = new litmus(LITMUS_DBL); break;
			case TK_FLG: l = new litmus(LITMUS_FLG); break;
			case TK_TXT: l = new litmus(LITMUS_TXT); break;
			case TK_BIN: l = new litmus(LITMUS_BIN); break;
			case TK_CLK: l = new litmus(LITMUS_CLK); break;
			case TK_LOC: l = new litmus(LITMUS_LOC); break;
			default: break; // Never happens
		}
		advance();
	}
	else if(tok == TK_OPEN_SQUARE)
	{
		l = new litmus(LITMUS_OPT);
		advance();
		l->content = parse(NULL);
		if(next_token() != TK_CLOSE_SQUARE)
			synerror("Optional element not terminated on line %d", next_lineno());
		advance();
	}
	else if(tok == TK_OPEN_ANGLE)
	{
		if(lookahead() == TK_HASH)
		{
			l = new litmus(LITMUS_ENUM);
			l->values = new cpvector();
			advance();
			while(1)
			{
				if(next_token() == TK_CLOSE_ANGLE)
					break;
				if(next_token() != TK_HASH)
				{
					synerror("Improper value in enumeration on line %d",
							next_lineno());
				}
				advance();
				if(next_token() != TK_NAME)
				{
					synerror("Improper value in enumeration on line %d",
							next_lineno());
				}
				l->values->add(symbol_table->item(next_value()));
				advance();
			}
			advance();
		}
		else
		{
			l = new litmus(LITMUS_CHOICE);
			l->children = new litmusvector();
			advance();
			while(next_token() != TK_CLOSE_ANGLE)
				l->children->add(parse(NULL));
			advance();
		}
	}
	else if(tok == TK_OPEN_BRACE)
	{
		l = new litmus(LITMUS_STRUCT);
		l->children = new litmusvector();
		advance();
		while(next_token() != TK_CLOSE_BRACE)
			parse(l->children);
		advance();
	}
	else if(tok == TK_OPEN_PAREN)
	{
		advance();
		if(next_token() == TK_PLUS)
		{
			l = new litmus(LITMUS_SEQ);
			advance();
		}
		else if(next_token() == TK_NUMBER)
		{
			l = new litmus(LITMUS_ARRAY);
			l->arraylen = next_value();
			advance();
		}
		else
			l = new litmus(LITMUS_LIST);
		l->content = parse(NULL);
		if(next_token() != TK_CLOSE_PAREN)
			synerror("List definition not terminated on line %d", next_lineno());
		advance();
	}
	else if(tok == TK_CARET)
	{
		// User-defined type:
		tokensection *tsect;
		
		advance();
		if(next_token() != TK_NAME)
			synerror("Unnamed user-defined type on line %d", next_lineno());
		int id = next_value();
		advance();
		for(int i = 0; i < sections->count(); i++)
		{
			tsect = sections->item(i);
			if(tsect->parsed != NULL && tsect->parsed->namesym == id)
			{
				l = tsect->parsed->clone();
				// Name will be overridden with the instance name later
				break;
			}
		}
		if(l == NULL)
		{
			synerror("User-defined type '%s' not found in environment",
					symbol_table->item(id)); // Now impossible
		}
	}
	else
		synerror("Token %s can't begin a type (line %d)", token_to_str(tok),
				next_lineno());

	return l;
}

intvector *Schema::parse_names()
{
	intvector *list = new intvector();
	Token tok;
	int id;
	
	tok = next_token();
	id = next_value();
	if(tok != TK_NAME)
		synerror("Missing name on line %d", next_lineno());
	list->add(id);
	advance();
	
	while(1)
	{
		tok = next_token();
		if(tok != TK_PLUS)
			break;
		advance();
		tok = next_token();
		id = next_value();
		if(tok != TK_NAME)
			synerror("Missing name after plus sign on line %d", next_lineno());
		list->add(id);
		advance();
	}
	return list;
}

void Schema::finish_word(char *word, int word_len)
{
	litmustoken *tk;
	
	word[word_len] = '\0';
	
	if(!strcmp(word, "int"))
		tk = new litmustoken(TK_INT, 0, line);
	else if(!strcmp(word, "dbl"))
		tk = new litmustoken(TK_DBL, 0, line);
	else if(!strcmp(word, "flg"))
		tk = new litmustoken(TK_FLG, 0, line);
	else if(!strcmp(word, "txt"))
		tk = new litmustoken(TK_TXT, 0, line);
	else if(!strcmp(word, "bin"))
		tk = new litmustoken(TK_BIN, 0, line);
	else if(!strcmp(word, "clk"))
		tk = new litmustoken(TK_CLK, 0, line);
	else if(!strcmp(word, "loc"))
		tk = new litmustoken(TK_LOC, 0, line);
	else
	{
		int id;
		
		id = symbol_table->find(word);
		if(id == -1)
			id = symbol_table->add(word);
		tk = new litmustoken(TK_NAME, id, line);
	}
	tokens->add(tk);
}

void Schema::finish_string(char *word, int word_len)
{
	litmustoken *tk;
	int id;
	
	word[word_len] = '\0';
		
	id = symbol_table->find(word);
	if(id == -1)
		id = symbol_table->add(word);
	tk = new litmustoken(TK_STRING, id, line);
	tokens->add(tk);
}
	
void Schema::finish_number(int n)
{
	tokens->add(new litmustoken(TK_NUMBER, n, line));
}

void Schema::finish_symbol(char c)
{
	litmustoken *tk;
	
	tk = new litmustoken();
	switch(c)
	{
		case ' ':
		case '\t':
			delete tk;
			return; break;
		case '\n':
			delete tk;
			line++;
			return; break;
		case '{': tk->token = TK_OPEN_BRACE; break;
		case '}': tk->token = TK_CLOSE_BRACE; break;
		case '(': tk->token = TK_OPEN_PAREN; break;
		case ')': tk->token = TK_CLOSE_PAREN; break;
		case '[': tk->token = TK_OPEN_SQUARE; break;
		case ']': tk->token = TK_CLOSE_SQUARE; break;
		case '<': tk->token = TK_OPEN_ANGLE; break;
		case '>': tk->token = TK_CLOSE_ANGLE; break;
		case '+': tk->token = TK_PLUS; break;
		case '@': tk->token = TK_AT; break;
		case '^': tk->token = TK_CARET; break;
		case '#': tk->token = TK_HASH; break;
		case '-': tk->token = TK_HYPHEN; break;
		default:
			delete tk;
			synerror("Invalid character %02x '%c' in schema on line %d", c, c,
					line);
	}
	tk->param = 0;
	tk->lineno = line;	
	tokens->add(tk);
}

void Schema::lex()
{
	enum State { StateStart, StateWord, StateNumber, StateComment, StateString };
	
	State s = StateStart;
	char c;
	int number;
	const int max_word_len = 100;
	char word[max_word_len + 1];
	int word_len;
	int len;
	int imports = 0;
	
	line = 1;
	len = source->length();
	for(int i = 0; i < len; i++)
	{
		c = source->getcharacter(i);
		if(c == '\0')
		{
			// Start of imported file
			line = 1;
			continue;
		}
		
		if(s == StateComment)
		{
			if(c == '\n')
			{
				s = StateStart;
				line++;
			}
		}
		else if(s == StateWord)
		{
			if(c == '*')
			{
				finish_word(word, word_len);
				s = StateComment;
			}
			else if(c == '"')
			{
				finish_word(word, word_len);
				word_len = 0;
				s = StateString;
			}
			else if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9') || c == '-' || c == '_')
			{
				word[word_len++] = c;
				if(word_len >= max_word_len)
					synerror("Word too long in schema (line %d)", line);
			}
			else
			{
				finish_word(word, word_len);
				finish_symbol(c);
				s = StateStart;
			}
		}
		else if(s == StateString)
		{
			if(c == '"')
			{
				/* This section uses grammar as well as lexer rules, which
					violates layering slightly but helps efficiency a lot: */
				if(tokens->count() == 0)
					synerror("Strings cannot begin a schema");
				if(tokens->top()->token == TK_AT)
				{
					tokens->pop();
					word[word_len] = '\0';
					
					// Import file, and append to source:
					memfile *mf;
					char *located;

					located = path_lookup(word);
					mf = new memfile(located);
					delete[] located;
					if(mf->data == NULL)
					{
						delete mf;
						synerror("Can't import types from file %s\n", word);
					}
					source->cat('\0'); // Marker so we can reset the line counter
					source->cat(mf->data);
					delete mf;
					len = source->length(); // Recalculate total source length
					
					imports++;
					if(imports > 20)
					{
						error("More than 20 imports processed in single schema;\n"
								"possible mutually recursive import statements?");
					}
				}
				else
					synerror("Strings may only occur after an @ sign in schemas"
							" (error on line %d)", line);
					
				// We never need to actually record the string, now:
				// finish_string(word, word_len);
				s = StateStart;
			}
			else if(c == '\n')
			{
				synerror("Multiline strings not allowed in schemas"
						" (error on line %d)", line);
			}
			else
			{
				word[word_len++] = c;
				if(word_len >= max_word_len)
					synerror("Word too long in schema on line %d", line);
			}
		}
		else if(s == StateNumber)
		{
			if(c == '*')
			{
				finish_number(number);
				s = StateComment;
			}
			else if(c == '"')
			{
				finish_number(number);
				word_len = 0;
				s = StateString;
			}
			else if(c >= '0' && c <= '9')
			{
				number = number * 10 + c - '0';
			}
			else
			{
				finish_number(number);
				if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
					c == '-' || c == '_')
				{
					word[0] = c;
					word_len = 1;
					s = StateWord;
				}
				else
				{
					finish_symbol(c);
					s = StateStart;
				}
			}
		}
		else if(s == StateStart)
		{
			if(c == '*')
			{
				s = StateComment;
			}
			else if(c == '"')
			{
				word_len = 0;
				s = StateString;
			}
			else if(c >= '0' && c <= '9')
			{
				number = c - '0';
				s = StateNumber;
			}
			else if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
				c == '-' || c == '_')
			{
				word[0] = c;
				word_len = 1;
				s = StateWord;
			}
			else
			{
				finish_symbol(c);
			}
		}
	}
	if(s == StateWord)
		finish_word(word, word_len);
	if(s == StateNumber)
		finish_number(number);
	if(s == StateString)
		synerror("Unterminated string in schema");
	tokens->add(new litmustoken(TK_EOF, 0, line));
}

void Schema::do_dump_tokens(litmustokenvector *v)
{
	int n = v->count();
	Token tok;
	int id, val;
	
	printf("%d tokens:\n", n);
	for(int i = 0; i < n; i++)
	{
		printf("%3d. ", i);
		tok = v->item(i)->token;
		if(tok == TK_NAME)
		{
			id = v->item(i)->param;
			printf("NAME(%d = %s)", id, symbol_table->item(id));
		}
		else if(tok == TK_STRING)
		{
			id = v->item(i)->param;
			printf("STRING(%d = %s)", id, symbol_table->item(id));
		}
		else if(tok == TK_NUMBER)
		{
			val = v->item(i)->param;
			printf("NUMBER(%d)", val);
		}
		else
		{
			switch(tok)
			{
				case TK_INT: printf("INT"); break;
				case TK_DBL: printf("DBL"); break;
				case TK_FLG: printf("FLG"); break;
				case TK_TXT: printf("TXT"); break;
				case TK_BIN: printf("BIN"); break;
				case TK_CLK: printf("CLK"); break;
				case TK_LOC: printf("LOC"); break;
				case TK_OPEN_BRACE: printf("OPEN_BRACE"); break;
				case TK_CLOSE_BRACE: printf("CLOSE_BRACE"); break;
				case TK_OPEN_ANGLE: printf("OPEN_ANGLE"); break;
				case TK_CLOSE_ANGLE: printf("CLOSE_ANGLE"); break;
				case TK_OPEN_PAREN: printf("OPEN_PAREN"); break;
				case TK_CLOSE_PAREN: printf("CLOSE_PAREN"); break;
				case TK_OPEN_SQUARE: printf("OPEN_SQUARE"); break;
				case TK_CLOSE_SQUARE: printf("CLOSE_SQUARE"); break;
				case TK_PLUS: printf("PLUS"); break;
				case TK_AT: printf("AT"); break;
				case TK_CARET: printf("CARET"); break;
				case TK_HASH: printf("HASH"); break;
				case TK_HYPHEN: printf("HYPHEN"); break;
				case TK_EOF: printf("EOF"); break;
				default:
					error("Impossible token.");
			}
		}
		printf("\n");
	}
}

void Schema::dump_tokens()
{
	if(meta != SCHEMA_NORM)
	{
		printf("1 token: ");
		switch(meta)
		{
			case SCHEMA_EMPTY: printf("0\n"); break;
			case SCHEMA_POLY:  printf("*\n"); break;
			case SCHEMA_EXPR:  printf("?\n"); break;
			case SCHEMA_NA:    printf("!\n"); break;
			default: printf("Unknown schema meta type\n"); break;
		}
		return;
	}
	if(tokens == NULL) // Might happen if called after Schema copy constructor
		error("Can't dump_tokens() when tokens are uninitialised");
	do_dump_tokens(tokens);
}

const char *token_to_str(Token token)
{
	switch(token)
	{
		case TK_NAME: return("NAME");
		case TK_NUMBER: return("NUMBER");
		case TK_STRING: return("STRING");
		case TK_INT: return("INT");
		case TK_DBL: return("DBL");
		case TK_FLG: return("FLG");
		case TK_TXT: return("TXT");
		case TK_BIN: return("BIN");
		case TK_CLK: return("CLK");
		case TK_LOC: return("LOC");
		case TK_OPEN_BRACE: return("OPEN_BRACE");
		case TK_CLOSE_BRACE: return("CLOSE_BRACE");
		case TK_OPEN_ANGLE: return("OPEN_ANGLE");
		case TK_CLOSE_ANGLE: return("CLOSE_ANGLE");
		case TK_OPEN_PAREN: return("OPEN_PAREN");
		case TK_CLOSE_PAREN: return("CLOSE_PAREN");
		case TK_OPEN_SQUARE: return("OPEN_SQUARE");
		case TK_CLOSE_SQUARE: return("CLOSE_SQUARE");
		case TK_PLUS: return("PLUS");
		case TK_AT: return("AT");
		case TK_CARET: return("CARET");
		case TK_HASH: return("HASH");
		case TK_HYPHEN: return("HYPHEN");
		case TK_EOF: return("EOF");
		default: return("UNKNOWN");
	}
}

/* litmus: */

litmus::litmus(LitmusType t)
{
	type = t;
	content = NULL;
	children = NULL;
	values = NULL;
	namesym = -1;
}

litmus *litmus::clone()
{
	litmus *l = new litmus(type);
	l->namesym = namesym;
	if(type == LITMUS_LIST || type == LITMUS_SEQ || type == LITMUS_ARRAY
			|| type == LITMUS_OPT)
	{
		if(content == NULL)
			error("Paranoia: trying to clone empty content.");
		l->content = content->clone();
	}
	if(type == LITMUS_STRUCT || type == LITMUS_CHOICE)
	{
		if(children == NULL)
			error("Paranoia: trying to clone empty children set.");
		l->children = new litmusvector();
		for(int i = 0; i < children->count(); i++)
			l->children->add(children->item(i)->clone());
	}
	if(type == LITMUS_ENUM)
	{
		if(values == NULL)
			error("Paranoia: trying to clone missing enumeration.");
		l->values = new cpvector();
		for(int i = 0; i < values->count(); i++)
			l->values->add(values->item(i));
	}
	l->arraylen = arraylen;
	return l;
}

litmus::~litmus()
{
	if(values != NULL)
		delete values;
	if(content != NULL)
		delete content;
	if(children != NULL)
	{
		litmus *l;
		
		for(int i = 0; i < children->count(); i++)
		{
			l = children->item(i);
			delete l;
		}
		delete children;
	}
}

litmustoken::litmustoken(Token token, int param, int lineno)
{
	this->token = token;
	this->param = param;
	this->lineno = lineno;
}

tokensection::tokensection()
{
	token = new litmustokenvector();
	dependencies = new intvector();
	parsed = NULL;
}
