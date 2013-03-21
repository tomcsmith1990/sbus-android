// express.h - DMI - 1-3-2007

class snode;
class sexpr;

enum ExprType
{
	XInt, XFloat, XString, XVar,
	XPath,
	XEqual, XNotEqual, XLessThan, XGtThan,
	XOr, XAnd, XNot, XExists
};

class subscription
{
	public:

	subscription(const char *subs);
	~subscription();
						
	int match(snode *node);
	void dump_tokens();
	void dump_tree();
	char *tostring();
	char *dump_plaintext(snode *lookup = NULL, svector *extra = NULL);
	
	private:
			
	sexpr *tree;
	
	void lex(const char *subs);
	sexpr *parse();
	sexpr *parse_atom();
	sexpr *parse_expression();
	sexpr *parse_truthval();
	int is_atom(exprtoken *tok);

	char *plaintext;	
	exprtokenvector *token;
	
	int pos;
	exprtoken *next_token();
	exprtoken *lookahead();
	void advance();

	void finish_symbol(char c);
	void finish_string(const char *word, int word_len);
	void finish_variable(const char *word, int word_len);
	void finish_path(svector *path, char *word, int word_len);
	void finish_number(int minus, int n, int fract, int denom);
	
	void synerror(const char *format, ...);
	void dump_expr(sexpr *e, int depth);
	void dump_plaintext(sexpr *e, int depth, StringBuf *buf, snode *lookup, svector *extra);
	void dump_path(sexpr *e);
	void spaces(int depth);
};
	
class sexpr
{
	public:
			
	sexpr(ExprType type);
	~sexpr();
	
	int match(snode *sn);
	
	ExprType type;
	
	sexpr *left;   // XEqual, XNotEqual, XLessThan, XGtThan, XOr, XAnd, XNot
	sexpr *right;  // XEqual, XNotEqual, XLessThan, XGtThan, XOr, XAnd
	int n;         // XInt
	double x;      // XFloat
	const char *s; // XString, XVar, XPath (field name)	
	svector *path; // XPath, XExists
	
	private:
	
	snode *evaluate(snode *data);
	int is_primitive();
	int is_comparison();
};
