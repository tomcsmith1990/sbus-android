// express.cpp - DMI - 1-3-2007

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "../library/error.h"
#include "../library/datatype.h"
#include "../library/dimension.h"
#include "../library/builder.h"
#include "express.h"

enum Token
{
	TK_OPEN_PAREN, TK_CLOSE_PAREN, TK_AND, TK_OR,
	TK_EQ, TK_NE, TK_LT, TK_GT,
	TK_EXCLAIM, TK_QUESTION,
	TK_INT, TK_FLOAT, TK_STRING, TK_VARIABLE, TK_PATH,
	TK_EOF
};

class exprtoken
{
	public:

	exprtoken(Token type);
			
	Token type;
	
	char *s;       // TK_STRING, TK_VARIABLE, TK_PATH
	int n;         // TK_INT
	double x;      // TK_FLOAT
	svector *path; // TK_PATH
};

void subscription::synerror(const char *format, ...)
{
	static const int MAX_ERR_LEN = 200;
	char c[MAX_ERR_LEN];
	va_list args;

	va_start(args, format);
	vsnprintf(c, MAX_ERR_LEN, format, args);
	va_end(args);
	c[MAX_ERR_LEN - 1] = '\0';
	
	throw SubscriptionException(c);
}

char *subscription::tostring()
{
	return sdup(plaintext);
}

char *subscription::dump_plaintext(snode *lookup, svector *more, pvector *less)
{
	StringBuf *buf = new StringBuf();
	dump_plaintext(tree, 0, buf, lookup, more, less);
	char *plaintext = buf->extract();
	delete buf;
	return plaintext;
}

void subscription::dump_tokens()
{
	int n = token->count();
	exprtoken *tok;
	
	printf("%d tokens:\n", n);
	for(int i = 0; i < n; i++)
	{
		printf("%3d. ", i);
		tok = token->item(i);
		if(tok->type == TK_PATH)
		{
			int parts = tok->path->count();
			printf("PATH(");
			for(int j = 0; j < parts; j++)
			{
				printf("%s", tok->path->item(j));
				if(j != parts - 1)
					printf("/");
			}
			if(tok->s != NULL)
				printf(".%s", tok->s);
			printf(")");
		}
		else if(tok->type == TK_STRING)
			printf("STRING(%s)", tok->s);
		else if(tok->type == TK_VARIABLE)
			printf("VARIABLE(%s)", tok->s);
		else if(tok->type == TK_INT)
			printf("INTEGER(%d)", tok->n);
		else if(tok->type == TK_FLOAT)
			printf("FLOAT(%g)", tok->x);
		else
		{
			switch(tok->type)
			{
				case TK_OPEN_PAREN: printf("OPEN_PAREN"); break;
				case TK_CLOSE_PAREN: printf("TK_CLOSE_PAREN"); break;
				case TK_AND: printf("TK_AND"); break;
				case TK_OR: printf("TK_OR"); break;
				case TK_EQ: printf("TK_EQ"); break;
				case TK_NE: printf("TK_NE"); break;
				case TK_LT: printf("TK_LT"); break;
				case TK_GT: printf("TK_GT"); break;
				case TK_EXCLAIM: printf("TK_EXCLAIM"); break;
				case TK_QUESTION: printf("TK_QUESTION"); break;
				case TK_EOF: printf("TK_EOF"); break;
				default:
					error("Impossible token.");
			}
		}
		printf("\n");
	}
}

void subscription::dump_tree()
{
	dump_expr(tree, 0);
}

void subscription::dump_path(sexpr *e)
{
	int parts = e->path->count();

	for(int i = 0; i < parts; i++)
	{
		printf("%s", e->path->item(i));
		if(i != parts - 1)
			printf("/");
	}
	if(e->s != NULL)
		printf("#%s", e->s);
}

void subscription::dump_expr(sexpr *e, int depth)
{
	if(e->type == XEqual || e->type == XNotEqual || e->type == XLessThan ||
			e->type == XGtThan)
	{
		spaces(depth);
		dump_expr(e->left, -1);
		switch(e->type)
		{
			case XEqual: printf(" equals "); break;
			case XNotEqual: printf(" not equal "); break;
			case XLessThan: printf(" less than "); break;
			case XGtThan: printf(" greater than "); break;
			default:
				error("Impossible switch error in dump_expr");
		}
		dump_expr(e->right, -1);
		printf("\n");
	}
	else if(e->type == XOr || e->type == XAnd)
	{
		dump_expr(e->left, depth + 1);
		spaces(depth);
		switch(e->type)
		{
			case XOr: printf("or\n"); break;
			case XAnd: printf("and\n"); break;
			default:
				error("Impossible switch error in dump_expr");
		}
		dump_expr(e->right, depth + 1);
	}
	else if(e->type == XNot)
	{
		spaces(depth);
		printf("not\n");
		dump_expr(e->left, depth + 1);
	}
	else if(e->type == XPath)
	{
		spaces(depth);
		dump_path(e);
		if(depth != -1) printf("\n");
	}
	else if(e->type == XExists)
	{
		spaces(depth);
		printf("there exists ");
		dump_path(e);
		printf("\n");
	}
	else if(e->type == XInt)
	{
		spaces(depth);
		printf("%d", e->n);
		if(depth != -1) printf("\n");
	}
	else if(e->type == XFloat)
	{
		spaces(depth);
		printf("%g", e->x);
		if(depth != -1) printf("\n");
	}
	else if(e->type == XString)
	{
		spaces(depth);
		printf("'%s'", e->s);
		if(depth != -1) printf("\n");
	}
	else if(e->type == XVar)
	{
		spaces(depth);
		printf("$%s", e->s);
		if(depth != -1) printf("\n");
	}
}


void subscription::dump_plaintext(sexpr *e, int depth, StringBuf *buf, snode *lookup, svector *more, pvector *less)
{
	if(e->type == XEqual || e->type == XNotEqual || e->type == XLessThan ||
			e->type == XGtThan)
	{
		dump_plaintext(e->left, -1, buf, lookup, more, less);
		switch(e->type)
		{
			case XEqual: buf->cat(" = "); break;
			case XNotEqual: buf->cat(" ~ "); break;
			case XLessThan: buf->cat(" < "); break;
			case XGtThan: buf->cat(" > "); break;
			default:
				error("Impossible switch error in dump_plaintext");
		}
		dump_plaintext(e->right, -1, buf, lookup, more, less);
	}
	else if(e->type == XOr || e->type == XAnd)
	{
		dump_plaintext(e->left, depth + 1, buf, lookup, more, less);
		switch(e->type)
		{
			case XOr: buf->cat(" | "); break;
			case XAnd: buf->cat(" | "); break;
			default:
				error("Impossible switch error in dump_plaintext");
		}
		dump_plaintext(e->right, depth + 1, buf, lookup, more, less);
	}
	else if(e->type == XNot)
	{
		buf->cat(" ! ");
		dump_plaintext(e->left, depth + 1, buf, lookup, more, less);
	}
	else if(e->type == XPath)
	{
		int parts = e->path->count();
		qualify_path(e, buf, lookup, more, less);

		/**
		  * If either our schema doesn't have a longer path
		  * or we haven't skipped all the parts while skipping levels.
		  */
		if (less == NULL || parts > less->count())
		{
			if(e->s != NULL)
			{
				buf->cat("#");
				buf->cat(e->s);
			}
		}
		/**
		  *	We've skipped all the parts, therefore this field does not exist in the other schema.
		  * That means that it must evaluate to false.
		  * Cannot evaluate the path 'NULL' thus won't match.
		  */
		else
			buf->cat("NULL");
	}	
	else if(e->type == XExists)
	{
		int parts = e->path->count();
		qualify_path(e, buf, lookup, more, less);

		/**
		  * If either our schema doesn't have a longer path
		  * or we haven't skipped all the parts while skipping levels.
		  */
		if (less == NULL || parts > less->count())
		{
			if(e->s != NULL)
			{
				buf->cat("#");
				buf->cat(e->s);
			}
		
			buf->cat(" ? ");
		}
		/**
		  *	We've skipped all the parts, therefore this field does not exist in the other schema.
		  * That means that it must evaluate to false.
		  * Cannot evaluate the path 'NULL' thus won't match.
		  */
		else
			buf->cat("NULL ? ");
	}
	else if(e->type == XInt)
	{
		buf->cat_int(e->n);
	}
	else if(e->type == XFloat)
	{
		buf->cat(e->x);
	}
	else if(e->type == XString)
	{
		buf->cat("'");
		buf->cat(e->s);
		buf->cat("'");
	}
	else if(e->type == XVar)
	{
		buf->cat("$");
		buf->cat(e->s);
	}
}

void subscription::qualify_path(sexpr *e, StringBuf *buf, snode *lookup, svector *more, pvector *less)
{
	int parts = e->path->count();

	for(int i = 0; i < parts; i++)
	{
		// If our schema has a longer path, skip however many extra levels we have.
		if (less != NULL)
			if (i < less->count())
				continue;
			
		// If their schema has a longer path, add the extra path before anything.
		if (i == 0 && more != NULL)
		{
			for (int j = 0; j < more->count(); j++)
			{
				buf->cat(more->item(j));
				buf->cat("/");
			}
		}
		
		// Any item which is actually in our schema should be in the lookup table (I think).
		if (lookup->exists(e->path->item(i)))
			buf->cat(lookup->extract_txt(e->path->item(i)));
		// Need to add the original string if it's not (e.g. some random field which isn't in our schema).
		// Otherwise the subscription will be an invalid logic statement.
		else
		{
			warning("%s not in lookup table in subscription::qualify_path - should always be in it\n", e->path->item(i));
			buf->cat(e->path->item(i));
		}
			
		if(i != parts - 1)
			buf->cat("/");
	}
}

void subscription::spaces(int depth)
{
	for(int i = 0; i < depth * 3; i++)
		printf(" ");
}
	
void subscription::finish_symbol(char c)
{
	exprtoken *tok;

	switch(c)
	{		
		case ' ': case '\t': case '\n': return; break;
		case '(': tok = new exprtoken(TK_OPEN_PAREN); break;
		case ')': tok = new exprtoken(TK_CLOSE_PAREN); break;
		case '&': tok = new exprtoken(TK_AND); break;
		case '|': tok = new exprtoken(TK_OR); break;
		case '=': tok = new exprtoken(TK_EQ); break;
		case '~': tok = new exprtoken(TK_NE); break;
		case '<': tok = new exprtoken(TK_LT); break;
		case '>': tok = new exprtoken(TK_GT); break;
		case '!': tok = new exprtoken(TK_EXCLAIM); break;
		case '?': tok = new exprtoken(TK_QUESTION); break;
		default:
			synerror("Invalid character %02x '%c' in subscription.", c, c);
	}
	token->add(tok);
}

void subscription::finish_string(const char *word, int word_len)
{
	exprtoken *tok;
	
	tok = new exprtoken(TK_STRING);
	tok->s = new char[word_len + 1];
	memcpy(tok->s, word, word_len);
	tok->s[word_len] = '\0';
	token->add(tok);
}

void subscription::finish_variable(const char *word, int word_len)
{
	exprtoken *tok;
	
	tok = new exprtoken(TK_VARIABLE);
	tok->s = new char[word_len + 1];
	memcpy(tok->s, word, word_len);
	tok->s[word_len] = '\0';
	token->add(tok);
}

void subscription::finish_path(svector *path, char *word, int word_len)
{
	exprtoken *tok;
	tok = new exprtoken(TK_PATH);
	if(word_len > 0)
	{
		word[word_len] = '\0';
		if(word[0] == '.')
		{
			tok->s = sdup(word + 1);
		}
		else
		{
			path->add(word);
			tok->s = NULL;
		}
	}
	tok->path = path;
	token->add(tok);
}

void subscription::finish_number(int minus, int n, int fract, int denom)
{
	exprtoken *tok;
	if(denom == 0)
	{
		tok = new exprtoken(TK_INT);
		tok->n = minus ? -n : n;
	}
	else
	{
		tok = new exprtoken(TK_FLOAT);
		tok->x = (double)n + (double)fract / (double)denom;
		if(minus)
			tok->x = -tok->x;
	}
	token->add(tok);
}

/*
	Regular expressions for tokens:
	
	d = digit 0-9
	a = alphabetic a-z, A-Z, _
	n = alphanumeric (either of the above)
	x = anything except '
	
	Number:   [-]d+[.d+]
	String:   'x*'
	Variable: $n+
	Path:     ((an* | #d+)/)* (an* | #d+) [.an*]
	Symbols:  ( ) ! ? & | = ~ < >
*/

void subscription::lex(const char *subs)
{
	enum State { StateStart, StateNumber, StateString, StateVariable,
			StatePath };
	const int max_word_len = 200;

	State state = StateStart;
	char c;
	char *word;
	int word_len;
	int minus, n, fract, denom;
	int subs_len;
	svector *path;
	int seen_field;
	
	subs_len = strlen(subs);
	word = new char[max_word_len + 1];
	
	for(int i = 0; i < subs_len; i++)
	{
		c = subs[i];
		if(state == StateStart)
		{
			if(c == '\'')
			{
				word_len = 0;
				state = StateString;
			}
			else if(c == '$')
			{
				word_len = 0;
				state = StateVariable;
			}
			else if(is_digit(c) || c == '-')
			{
				if(c == '-')
				{
					minus = 1;
					n = 0;
				}
				else
				{
					minus = 0;
					n = c - '0';
				}
				fract = 0;
				denom = 0;
				state = StateNumber;
			}
			else if(is_alphabetic(c))
			{
				path = new svector();
				word_len = 0;
				word[word_len++] = c;
				seen_field = 0;
				state = StatePath;
			}
			else
			{
				finish_symbol(c);
			}
		}
		else if(state == StateNumber)
		{
			if(is_digit(c))
			{
				if(denom == 0)
				{
					n = n * 10 + c - '0';
				}
				else
				{
					fract = fract * 10 + c - '0';
					denom *= 10;
				}
			}
			else if(c == '.' && denom == 0)
			{
				denom = 1;
			}
			else
			{
				finish_number(minus, n, fract, denom);
				state = StateStart;
				i--; // Reprocess character
			}
		}
		else if(state == StateString)
		{
			if(c == '\'')
			{
				finish_string(word, word_len);
				state = StateStart;
			}
			else
			{
				word[word_len++] = c;
				if(word_len >= max_word_len)
					synerror("Word too long in schema.");
			}
		}
		else if(state == StateVariable)
		{
			if(is_alphanumeric(c))
			{
				word[word_len++] = c;
				if(word_len >= max_word_len)
					synerror("Word too long in schema.");
			}
			else
			{
				finish_variable(word, word_len);
				state = StateStart;
				i--; // Reprocess character
			}
		}
		else if(state == StatePath)
		{
			if(c == '/')
			{
				if(seen_field)
				{
					// No more components allowed after field name:
					finish_path(path, word, word_len);
					state = StateStart;
					i--; // Reprocess character
				}
				else
				{
					// Next path component:
					word[word_len] = '\0';
					path->add(word);
					word_len = 0;
				}
			}
			else if(c == '.')
			{
				if(seen_field)
				{
					// Already had a field name:
					finish_path(path, word, word_len);
					state = StateStart;
					i--; // Reprocess character
				}
				else
				{
					word[word_len] = '\0';
					path->add(word);
					word_len = 0;
					word[word_len++] = '.';
					seen_field = 1;
				}
			}
			else if(c == '#')
			{
				if(word_len == 0)
					word[word_len++] = '#';
				else
				{
					// Hash can only occur at beginning of path component:
					finish_path(path, word, word_len);
					state = StateStart;
					i--; // Reprocess character
				}
			}
			else if(is_digit(c))
			{
				if(word_len == 0)
				{
					// Path components cannot start with a digit:
					finish_path(path, word, word_len);
					state = StateStart;
					i--; // Reprocess character
				}
				else
					word[word_len++] = c;
			}
			else if(is_alphabetic(c))
			{
				if(word_len > 0 && word[0] == '#')
				{
					// Only digits can follow '#':
					finish_path(path, word, word_len);
					state = StateStart;
					i--; // Reprocess character
				}
				else
					word[word_len++] = c;
			}
			else // Other symbol
			{
				finish_path(path, word, word_len);
				state = StateStart;
				i--; // Reprocess character
			}
		}
	}
	
	// Finish unfinished tokens:
	if(state == StateString)
		finish_string(word, word_len);
	else if(state == StateVariable)
		finish_variable(word, word_len);
	else if(state == StateNumber)
		finish_number(minus, n, fract, denom);
	else if(state == StatePath)
		finish_path(path, word, word_len);

	exprtoken *tok = new exprtoken(TK_EOF);
	token->add(tok);
	delete[] word;
}

exprtoken *subscription::next_token()
{
	if(pos >= token->count())
		error("Error: ran out of tokens");
	return token->item(pos);
}

exprtoken *subscription::lookahead()
{
	if(pos + 1 >= token->count())
		error("Error: ran out of tokens");
	return token->item(pos + 1);
}

void subscription::advance()
{
	pos++;
}

subscription::subscription(const char *subs)
{
	plaintext = sdup(subs);
	token = new exprtokenvector();
	tree = NULL;
	try
	{
		lex(subs);
		pos = 0;
		if(next_token()->type == TK_EOF)
			tree = NULL;
		else
			tree = parse();
	}
	catch(SubscriptionException e)
	{
		error("Problem parsing subscription expression:\n%s\n", e.msg);
	}
}

subscription::~subscription()
{
	exprtoken *tok;
	
	for(int i = 0; i < token->count(); i++)
	{
		tok = token->item(i);
		/* We don't have to delete tok->s and tok->path here, since they will
			be deleted by the shallow copies in tree */
		delete tok;
	}
	delete token;
	if(tree != NULL)
		delete tree;
	delete[] plaintext;
}

/* Grammar:

	atom ::= TK_PATH | TK_INT | TK_FLOAT | TK_STRING | TK_VARIABLE
	truthval ::= TK_PATH TK_QUESTION |
		TK_OPEN_PAREN <expression> TK_CLOSE_PAREN |
		<atom> TK_EQ <atom> | <atom> TK_NE <atom> |
		<atom> TK_LT <atom> | <atom> TK_GT <atom> |
	expression ::= <truthval> | TK_EXCLAIM <truthval> |
		<truthval> TK_AND <expression> | <truthval> TK_OR <expression> |
		TK_EXCLAIM <truthval> TK_AND <expression> |
		TK_EXCLAIM <truthval> TK_OR <expression>
	start ::= <expression> TK_EOF
*/

sexpr *subscription::parse()
{
	exprtoken *tok;
	sexpr *e;

	e = parse_expression();	
	tok = next_token();
	if(tok->type != TK_EOF)
		synerror("Additional content past end of expression.");
	return e;
}

sexpr *subscription::parse_expression()
{
	exprtoken *tok;
	sexpr *e1 = NULL, *e2;

	while(1)
	{	
		tok = next_token();
		if(tok->type == TK_EXCLAIM)
		{
			e2 = new sexpr(XNot);
			advance();
			e2->left = parse_truthval();
		}
		else
			e2 = parse_truthval();

		// Join onto previous part:		
		if(e1 == NULL)
			e1 = e2;
		else
			e1->right = e2;
		
		tok = next_token();
		if(tok->type == TK_AND || tok->type == TK_OR)
		{
			sexpr *e;
			
			e = new sexpr(tok->type == TK_AND ? XAnd : XOr);
			e->left = e1;
			advance();
			e1 = e;
		}
		else
			break;
	}
	return e1;
}

sexpr *subscription::parse_truthval()
{
	exprtoken *tok;
	sexpr *e;
	
	tok = next_token();
	if(tok->type == TK_OPEN_PAREN)
	{
		advance();
		e = parse_expression();
		if(next_token()->type != TK_CLOSE_PAREN)
			synerror("Expected a close parenthesis");
		advance();
	}
	else if(tok->type == TK_PATH && lookahead()->type == TK_QUESTION)
	{
		e = new sexpr(XExists);
		e->path = tok->path;
		advance();
		advance();
	}
	else if(is_atom(tok))
	{
		sexpr *e1, *e2;
		
		e1 = parse_atom();
		tok = next_token();
		switch(tok->type)
		{
			case TK_EQ: e = new sexpr(XEqual); break;
			case TK_NE: e = new sexpr(XNotEqual); break;
			case TK_LT: e = new sexpr(XLessThan); break;
			case TK_GT: e = new sexpr(XGtThan); break;
			default:
				synerror("Expected a comparison operator");
		}
		e->left = e1;
		advance();
		e2 = parse_atom();
		e->right = e2;
	}
	else
		synerror("Expected a truth value");
	return e;
}

sexpr *subscription::parse_atom()
{
	exprtoken *tok;
	sexpr *e;
	
	tok = next_token();
	switch(tok->type)
	{
		case TK_INT:
			e = new sexpr(XInt);
			e->n = tok->n;
			break;
		case TK_FLOAT:
			e = new sexpr(XFloat);
			e->x = tok->x;
			break;
		case TK_STRING:
			e = new sexpr(XString);
			e->s = tok->s;
			break;
		case TK_VARIABLE:
			e = new sexpr(XVar);
			e->s = tok->s;
			break;
		case TK_PATH:
			e = new sexpr(XPath);
			e->path = tok->path;
			e->s = tok->s;
			break;
		default:
			synerror("Expected an atom");
	}
	advance();
	return e;
}

int subscription::is_atom(exprtoken *tok)
{
	if(tok->type == TK_INT || tok->type == TK_FLOAT ||
			tok->type == TK_STRING || tok->type == TK_VARIABLE ||
			tok->type == TK_PATH)
		return 1;
	else
		return 0;
}

snode *sexpr::evaluate(snode *data)
{
	// Returns NULL if can't be evaluated for this data
	snode *sn;
	NodeType ty;
	
	switch(type)
	{
		case XInt:
			sn = new snode(n);
			break;
		case XFloat:
			sn = new snode(x);
			break;
		case XString:
			/*
		   	May promote strings to SDateTime or SLocation
				May promote strings "true" or "false" to boolean
				May promote strings to enumeration values
			*/
			; // TODO - XXX
			sn = new snode(s);
			break;
		case XVar:
			error("Variable evaluation not implemented yet");
			// TODO
			// variable name in s
			// $srccpt, $destsap, $sequenceid etc
			break;
		case XPath:
			// path contains element path, s contains field name or NULL
			sn = data->follow_path(path);
			if(sn == NULL)
			{
				// Element doesn't exist
				return NULL;
			}
			ty = sn->get_type();
			// Create new snode so it can be deleted later:
			sn = new snode(sn);
			if(s != NULL)
			{
				if(ty == SDateTime)
				{
					// field names: day, month, year, hour, min, sec, micros
					int n;
					
					n = sn->extract_clk()->address_field(s);
					if(n != -1)
					{
						delete sn;
						sn = new snode(n);
					}
				}
				else if(ty == SLocation)
				{
					// field names: lat, lon, elev,
					double x;
					
					x = sn->extract_loc()->address_field(s);
					if(x > -10000.0)
					{
						delete sn;
						sn = new snode(x);
					}
				}
				else if(ty == SStruct || ty == SList)
				{
					if(!strcmp(s, "items"))
					{
						int n = sn->count();
						delete sn;
						sn = new snode(n);
					}
				}
			}
			break;
		default:
			error("Cannot evaluate a non-primitive expression type");
	}		
	return sn;
}

int sexpr::is_primitive()
{
	return (type == XInt || type == XFloat || type == XString ||
			type == XVar || type == XPath);
}

int sexpr::is_comparison()
{
	return (type == XEqual || type == XNotEqual || type == XLessThan ||
			type == XGtThan);
}

int subscription::match(snode *node)
{
	/*
	printf("Matching subscription against message:\n");
	node->dump();
	*/
	return tree->match(node);
}

int sexpr::match(snode *sn)
{
	int truth;
	snode *sn1 = NULL, *sn2 = NULL;
	
	if(is_primitive())
		error("Can't match an expression of primitive type");
	if(is_comparison())
	{
		// Assign left and right expression contents to sn1 and sn2:
		if(!left->is_primitive() || !right->is_primitive())
			error("Comparison operands must be of primitive type");
		sn1 = left->evaluate(sn);
		sn2 = right->evaluate(sn);

		/*		
		printf("Left operand:\n");
		if(sn1 == NULL)
			printf("NULL\n");
		else
			sn1->dump();
		printf("Right operand:\n");
		if(sn2 == NULL)
			printf("NULL\n");
		else
			sn2->dump();
		*/
		
		if(sn1 == NULL || sn2 == NULL)
			return 0; // Can't evaluate an operand, so fail match
		
		// Check sn1 and sn2 are comparable:
		NodeType ty1, ty2;
		ty1 = sn1->get_type();
		ty2 = sn2->get_type();
		/*
			In future:
			May promote integers to floating point
			May promote integers 0 and 1 to boolean
			May promote integers to enumeration values
		*/
		// TODO - XXX
		if(ty1 != ty2)
		{
			delete sn1;
			delete sn2;
			return 0; // Incomparable types, so fail match
		}
	}
	switch(type)
	{
		case XExists:
			truth = ((sn->follow_path(path) == NULL) ? 0 : 1);
			break;
		case XEqual:
			truth = ((sn1->equals(sn2) == 1) ? 1 : 0);
			break;
		case XNotEqual:
			truth = ((sn1->equals(sn2) == 1) ? 0 : 1);
			break;
		case XLessThan:
			truth = ((sn1->lessthan(sn2) == 1) ? 1 : 0);
			break;
		case XGtThan:
			truth = ((sn2->lessthan(sn1) == 1) ? 1 : 0);
			break;
		case XOr:
			truth = left->match(sn) | right->match(sn);
			break;
		case XAnd:
			truth = left->match(sn) & right->match(sn);
			break;
		case XNot:
			truth = 1 - left->match(sn);
			break;
		default:
			error("Switch error in match");
	}
	if(sn1 != NULL)
		delete sn1;
	if(sn2 != NULL)
		delete sn2;
	return truth;
}

sexpr::sexpr(ExprType t)
{
	type = t;
	left = right = NULL;
	s = NULL;
	path = NULL;
}

sexpr::~sexpr()
{
	if(type == XNot)
		delete left;
	if(type == XEqual || type == XNotEqual || type == XLessThan ||
			type == XGtThan || type == XOr || type == XAnd)
	{
		delete left;
		delete right;
	}
	if(s != NULL)
		delete s;
	if(path != NULL)
		delete path;
}

exprtoken::exprtoken(Token type)
{
	this->type = type;
	s = NULL;
	path = NULL;
}
