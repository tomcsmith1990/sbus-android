// litmus.h - DMI - 31-1-2007

class litmus;
class litmustoken;
class tsection;

char *path_lookup(const char *filename);
MetaType get_metatype(const char *schema_source);

enum LitmusType
{
   LITMUS_INT, LITMUS_DBL, LITMUS_FLG, LITMUS_TXT,
   LITMUS_BIN, LITMUS_CLK, LITMUS_LOC,
   LITMUS_STRUCT, LITMUS_LIST, LITMUS_SEQ, LITMUS_ARRAY,
   LITMUS_OPT, LITMUS_CHOICE, LITMUS_ENUM
};

enum Token
{
	TK_NAME, TK_NUMBER, TK_STRING,
	TK_INT, TK_DBL, TK_FLG, TK_TXT, TK_BIN, TK_CLK, TK_LOC,
	TK_OPEN_BRACE, TK_CLOSE_BRACE, TK_OPEN_ANGLE, TK_CLOSE_ANGLE,
	TK_OPEN_PAREN, TK_CLOSE_PAREN, TK_OPEN_SQUARE, TK_CLOSE_SQUARE,
	TK_PLUS, TK_AT, TK_CARET, TK_HASH, TK_HYPHEN,
	TK_EOF
};

class Schema
{
	public:
	
	svector *symbol_table; // Element names and literal values   
	litmus *tree;
	
	// These three functions return NULL in case of error:
	static Schema *load(const char *pathname, const char **err);
	static Schema *load(memfile *mf, const char **err);
	static Schema *create(const char *s, const char **err);

	Schema(); // Not used publicly
	Schema(Schema *sch); // Makes a copy (costly)
	~Schema();

	char *orig_string();
	char *canonical_string();
	
	void dump_tokens();
	void dump_tree(int initial_indent = 0, int log = 0);
	
	int construct_lookup(Schema *sch, snode *lookup_forward, snode *lookup_backward);
	
	int match_constraints(snode *constraints);
	int construct_lookup(Schema *convert_to, snode *constraints, snode *lookup_forward, snode *lookup_backward);

	HashCode *hc, *type_hc;
	snode *hashes;
	MetaType meta;
	
	private:

	StringBuf *source;
	litmustokenvector *tokens;
	tokensectionvector *sections;

	int read(const char *s, const char **err); // Returns 0 if OK, -1 on error
		
	litmus *parse(litmusvector *multi);
	litmus *parse_type();
	intvector *parse_names();
	int multiple_names();
	void dump_litmus(StringBuf *sb, litmus *l, int offset, int defn, snode *sn = NULL, StringBuf *tsb = NULL);
	
	int match_constraints(snode *want, snode *have);
	int construct_lookup(snode *want, snode *have, snode *target_hashes, snode *lookup_forward, snode *lookup_backward);
	void construct_lookup(snode *want, snode *have, snode *lookup_forward, snode *lookup_backward);
	
	void build_sections();
	void scan_sections();
	void parseloop();
	
	int pos, line;
	litmustokenvector *pool;
	
	Token next_token();
	Token lookahead();
	int next_value();
	int next_lineno();
	void advance();
	
	void synerror(int line, const char *msg);
	void synerror(const char *format, ...);
	
	void lex();
	void finish_word(char *word, int word_len);
	void finish_string(char *word, int word_len);
	void finish_number(int n);
	void finish_symbol(char c);
	void do_dump_tokens(litmustokenvector *v);
};

class litmus
{
	public:
	
   LitmusType type; // Always valid
	
   int namesym; // For all except LITMUS_OPT and LITMUS_CHOICE
   litmus *content; // LITMUS_LIST, LITMUS_SEQ, LITMUS_ARRAY, LITMUS_OPT
   litmusvector *children; // LITMUS_STRUCT and LITMUS_CHOICE only
   cpvector *values; // LITMUS_ENUM only
   int arraylen; // LITMUS_ARRAY only

	litmus(LitmusType t);
	~litmus();
	
	litmus *clone();
};

/* Move everything below here to litmus.cpp after code finished - XXXXX */

class litmustoken
{
	public:

	litmustoken() {};	
	litmustoken(Token token, int param, int lineno);
	
	Token token;
	int param;
	int lineno;
};

class tokensection
{
	public:
	
	tokensection();
			
	litmustokenvector *token;
	intvector *dependencies;
	litmus *parsed; // NULL if not parsed yet, else it's a tree
};
