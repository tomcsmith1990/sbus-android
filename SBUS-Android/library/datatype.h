// datatype.h - DMI - 17-1-2002

char *sdup(const char *s);
char *strmerge(const char *format, ...);
char *sjoin(const char *s1, char c, const char *s2);
void sfree(const char *s);

int fexists(const char *file_path);
char *gethomedir();

class intvector
{
	public:
			
		intvector();
		~intvector();
		
		void add(int x);
		void add_unique(int x); // Does not allow duplicates
		void set(int n, int x);
		void del(int n);
		void remove(int x);
		
		int item(int n);
		int find(int x); // Returns -1 if not found
		
		int count();
	
	private:
			
		int capacity;
		int size;
		int *data;
		
		void expand_capacity();
};

class pvector // void pointer vector
{
	public:
			
		pvector();
		~pvector();
		
		void add(void *x);
		void set(int n, void *x);
		void del(int n);
		void remove(void *x);
		
		void *item(int n);
		void *top();
		void *pop();
		int find(void *x); // Returns -1 if not found
		
		int count();
		void clear();
	
	private:
			
		int capacity;
		int size;
		void **data;
		
		void expand_capacity();
};

class cpvector : public pvector // wrapper class for char ptrs
{
	public:
			
		void add(const char *s) { pvector::add((void *)s); }
		void set(int n, const char *s) { pvector::set(n, (void *)s); }
		char *item(int n) { return (char *)pvector::item(n); }
		int find(const char *s) { return pvector::find((void *)s); }
};

class litmus;
class snode;
class segment; // For dimension.cpp only
class snumber;
class XMLTag;
class HashCode;
class sendpoint;
class sbuiltin;
class smidpoint;
class speer;
//class spermission;
class Schema;
class exprtoken;
class litmustoken;
class tokensection;
class scomm;




class litmusvector : public pvector // wrapper class
{
	public:
			
		void add(litmus *x) { pvector::add((void *)x); }
		void set(int n, litmus *x) { pvector::set(n, (void *)x); }		
		litmus *item(int n) { return (litmus *)pvector::item(n); }
};

class scommvector : public pvector // wrapper class
{
	public:
			
		void add(scomm *x) { pvector::add((void *)x); }
		void set(int n, scomm *x) { pvector::set(n, (void *)x); }		
		scomm *item(int n) { return (scomm *)pvector::item(n); }
};

class litmustokenvector : public pvector // wrapper class
{
	public:
			
		void add(litmustoken *x) { pvector::add((void *)x); }
		void set(int n, litmustoken *x) { pvector::set(n, (void *)x); }		
		litmustoken *item(int n) { return (litmustoken *)pvector::item(n); }
		litmustoken *top() { return (litmustoken *)pvector::top(); }
		litmustoken *pop() { return (litmustoken *)pvector::pop(); }
};

class tokensectionvector : public pvector // wrapper class
{
	public:
			
		void add(tokensection *x) { pvector::add((void *)x); }
		void set(int n, tokensection *x) { pvector::set(n, (void *)x); }		
		tokensection *item(int n) { return (tokensection *)pvector::item(n); }
};

class snodevector : public pvector // wrapper class
{
	public:
			
		void add(snode *x) { pvector::add((void *)x); }
		void set(int n, snode *x) { pvector::set(n, (void *)x); }		
		snode *item(int n) { return (snode *)pvector::item(n); }
		snode *top() { return (snode*)pvector::top(); }
};

class snumbervector : public pvector // wrapper class
{
	public:
			
		void add(snumber *x) { pvector::add((void *)x); }
		void set(int n, snumber *x) { pvector::set(n, (void *)x); }		
		snumber *item(int n) { return (snumber *)pvector::item(n); }
};

class segmentvector : public pvector // wrapper class
{
	public:
			
		void add(segment *x) { pvector::add((void *)x); }
		void set(int n, segment *x) { pvector::set(n, (void *)x); }		
		segment *item(int n) { return (segment *)pvector::item(n); }
};

class sendpointvector : public pvector // wrapper class
{
	public:
			
		void add(sendpoint *x) { pvector::add((void *)x); }
		void set(int n, sendpoint *x) { pvector::set(n, (void *)x); }		
		sendpoint *item(int n) { return (sendpoint *)pvector::item(n); }
};

class sbuiltinvector : public pvector // wrapper class
{
	public:
			
		void add(sbuiltin *x) { pvector::add((void *)x); }
		void set(int n, sbuiltin *x) { pvector::set(n, (void *)x); }		
		sbuiltin *item(int n) { return (sbuiltin *)pvector::item(n); }
};

class smidpointvector : public pvector // wrapper class
{
	public:
			
		void add(smidpoint *x) { pvector::add((void *)x); }
		void set(int n, smidpoint *x) { pvector::set(n, (void *)x); }		
		smidpoint *item(int n) { return (smidpoint *)pvector::item(n); }
};

class speervector : public pvector // wrapper class
{
	public:
			
		void add(speer *x) { pvector::add((void *)x); }
		void set(int n, speer *x) { pvector::set(n, (void *)x); }		
		speer *item(int n) { return (speer *)pvector::item(n); }
		speer *top() { return (speer *)pvector::top(); }
		speer *pop() { return (speer *)pvector::pop(); }
		void remove(speer *x) { pvector::remove((void *)x); }
};



class HashCodeVector : public pvector // wrapper class
{
	public:
			
		void add(HashCode *x) { pvector::add((void *)x); }
		void set(int n, HashCode *x) { pvector::set(n, (void *)x); }		
		HashCode *item(int n) { return (HashCode *)pvector::item(n); }
};

class SchemaVector : public pvector // wrapper class
{
	public:
			
		void add(Schema *x) { pvector::add((void *)x); }
		void set(int n, Schema *x) { pvector::set(n, (void *)x); }		
		Schema *item(int n) { return (Schema *)pvector::item(n); }
};

class exprtokenvector : public pvector // wrapper class
{
	public:
			
		void add(exprtoken *x) { pvector::add((void *)x); }
		void set(int n, exprtoken *x) { pvector::set(n, (void *)x); }		
		exprtoken *item(int n) { return (exprtoken *)pvector::item(n); }
};

class tagvector : public pvector // wrapper class
{
	public:
			
		void add(XMLTag *x) { pvector::add((void *)x); }
		void set(int n, XMLTag *x) { pvector::set(n, (void *)x); }		
		XMLTag *item(int n) { return (XMLTag *)pvector::item(n); }
		XMLTag *top() { return (XMLTag *)pvector::top(); }
		XMLTag *pop() { return (XMLTag *)pvector::pop(); }
};

class qnode; // Helper for ptrqueue
class qinode; // Helper for intqueue

class smessage;
class AbstractMessage;

class ptrqueue
{
	public:
	
	ptrqueue();
	~ptrqueue();
	
	int isempty();
	int count();       // Warning: this operation is slow on ptrqueues
	void add(void *x); // Adds to tail of queue
	void *remove();    // Removes from head of queue (or NULL if queue empty)
	void *preview();   // Returns next thing to be remove()'d
	
	void begin(); // Initialises internal iterator
	void *next(); // Iterates (returns NULL when at end of queue)
	void *unlink_and_next(); // Removes current, then iterates
	
	private:
	
	void do_unlink(qnode *qn);
	
	qnode *head, *tail;
	qnode *iter;
};

class intqueue
{
	public:
	
	intqueue();
	~intqueue();
	
	int isempty();	
	void add(int x); // Adds to tail of queue
	int remove();    // Removes from head of queue (or -1 if queue empty)
	int preview();   // Returns next thing to be remove()'d
	
	void begin(); // Initialises internal iterator
	int next();   // Iterates (returns -1 when at end of queue)
	int unlink_and_next(); // Removes current, then iterates
	
	private:
	
	void do_unlink(qinode *qn);
	
	qinode *head, *tail;
	qinode *iter;
};

class scommqueue : public ptrqueue // wrapper class
{
	public:
	
	void add(scomm *x) { ptrqueue::add((void *)x); }
	scomm *remove() { return (scomm *)ptrqueue::remove(); }
	scomm *preview() { return (scomm *)ptrqueue::preview(); }
	scomm *next() { return (scomm *)ptrqueue::next(); }
	scomm *unlink_and_next() { return (scomm *)ptrqueue::unlink_and_next(); }
};

class smessagequeue : public ptrqueue // wrapper class
{
	public:
	
	void add(smessage *x) { ptrqueue::add((void *)x); }
	smessage *remove() { return (smessage *)ptrqueue::remove(); }
	smessage *preview() { return (smessage *)ptrqueue::preview(); }
	smessage *next() { return (smessage *)ptrqueue::next(); }
	smessage *unlink_and_next()
			{ return (smessage *)ptrqueue::unlink_and_next(); }
};

class AbstractMessageQueue : public ptrqueue // wrapper class
{
	public:
	
	void add(AbstractMessage *x) { ptrqueue::add((void *)x); }
	AbstractMessage *remove() { return (AbstractMessage *)ptrqueue::remove(); }
	AbstractMessage *preview() {return (AbstractMessage *)ptrqueue::preview();}
	AbstractMessage *next() { return (AbstractMessage *)ptrqueue::next(); }
};

/* An svector is different from a cpvector, because it allocates
	storage for strings which are added, performs deep copies, and
	frees them when it is destroyed:
*/

class svector
{
	public:
			
		svector();
		svector(svector *src);
		~svector();
		
		int add(const char *s); // Returns index of new item
		int add_noduplicates(const char *s);
		int add(const char *s, int nchars);
		char *item(int n); // Returns a shallow copy
		char *remove(const char *s); // Removes and returns the first instance of s.
		char *pop(); // Returns the last item added, and removes it
		int find(const char *s);
		
		int count();
		void clear();
	
	private:
			
		int capacity;
		int size;
		char **data;
		
		void expand_capacity();
};

class StringBuf
{
	public:
			
		StringBuf();
		StringBuf(StringBuf *sb);
		~StringBuf();
	
		char *extract();
		/* The string returned by extract() must be deleted by the caller,
			and is *not* removed when the StringBuf itself is deleted. */
		const unsigned char *peek();
		/* The string returned by peek() must *not* be deleted or modified */
		int save(const char *filename); // Returns 0 if OK, else -1
		int transmit(int fd); // Returns 0 if OK, else -1
		void print();
		void dump();
		void log();
		char getcharacter(int n);
					
		void cat(const char *s); // NULL-terminated
		void cat(const void *data, int bytes); // Ignore NULLs
		void cat(int n); // Encodes as a count; 1 to 5 bytes
		void cat(char c);
		void cat(double d);
		void cat_int(int n);
		void cat_word(int n); // Encodes as 4 bytes
		void cat_byte(int n);
		void cat_hexbyte(unsigned char c); // Encodes as 2 characters
		void cat_string(const char *s); // Encodes as a count followed by data
		void cat_spaces(int n);
		void cat(HashCode *hc);
		void catf(const char *format, ...);
		void skip(int n);
		
		char pop();
		void truncate_spaces();
		void overwrite(int pos, char c);
		void overwrite_word(int pos, int n);
		
		void clear();
		int length();
		int iswhitespace(); // Empty strings count as whitespace
		
	private:
			
		unsigned char *buf; // Internal buffer - not null terminated
		int used, capacity;
		
		char *varbuf, *spcbuf;
		int maxvarbuf, maxspcbuf;
		
		void check_expand(int required);
};

class stdinfile
{
	public:
	
		stdinfile();
		~stdinfile();
		
		char *data;
		int len;
	
	private:
		
		int capacity;
		void check_expand(int extra);
};

class memfile
{
	public:
			
		memfile(const char *filename);
		~memfile();
		
		char *data; // NULL if file could not be opened
		int len;
};

class linefile
{
	public:
			
		linefile(const char *filename, int filter_blanks);
		~linefile();
		
		int valid();
		int count();
		/* The valid() method (which tests the private file_ok flag set by
			the constructor) shows if the file could be opened. If not, the
			other methods will still work since v will exist anyway
			(but contain no items). */
		
		const char *item(int n);
		const char *getline(int n); // Synonym for item
		int search(char *line);
	
	private:
			
		svector *v;
		int file_ok;
	
		int readline(FILE *fp, char *buf, int max_len);
};

class PtrStack
{
	public:
		
		PtrStack();
		~PtrStack();
		
		void push(void *p);
		void *pop();
		void *top();
		int items();
	
	private:
		
		int capacity, used;
		void **stack;
};

class snodestack : public PtrStack // wrapper class
{
	public:
			
		void push(snode *sn) { PtrStack::push((void *)sn); }
		snode *pop() { return (snode *)PtrStack::pop(); }
		snode *top() { return (snode *)PtrStack::top(); }
};

class stringsplit
{
	public:
			
		stringsplit(const char *s, char delim);
		~stringsplit();
		
		int count();
		char *item(int n);
		
	private:
		
		svector *v;
};
