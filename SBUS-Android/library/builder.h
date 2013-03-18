// builder.h - DMI - 17-2-2007

enum NodeType { SInt, SDouble, SText, SBinary, SBool, SDateTime, SLocation,
	SStruct, SList, SValue, SEmpty };

extern const char *SNull;

const char *node_type_to_string(NodeType t);

class snode
{
	public:
			
	// Constructors:		
	snode(int n, const char *name = NULL);
	snode(const char *s, const char *name = NULL);
	snode(const void *data, int len, const char *name = NULL);
	snode(double x, const char *name = NULL);
	snode(sdatetime *time, const char *name = NULL);
	snode(slocation *loc, const char *name = NULL);
	static snode *create_bool(int n, const char *name = NULL);
	static snode *create_value(const char *s, const char *name = NULL);
	snode(snode *sn, const char *override_name = NULL); // Cloning operation
	~snode();

	// XML import - returns NULL (explanation in *err) on syntax error:
	static snode *import(const char *s, const char **err);
	static snode **import_multi(const char *s, int *n, const char **err);
	static snode *import_file(const char *path, const char **err);
	static snode **import_file_multi(const char *path, int *n, const char **err);
	/* static snode *import_search_file(const char *path, const char **err); */
	// XML export:
	char *toxml(int pretty);
	// Debugging:
	void dump(int offset = 0);
	
	// Packing (most of these functions are global):	
	snode *append(snode *n);
	
	// Extracting:
	int count();
	snode *extract_item(int item);
	snode *extract_item(const char *name);
	snode **extract_array();
	const char *get_name(int item = -1); // Shallow copy

	/* "item" may be -1 for the current item, >= 0 for a child index */
	int extract_int(int item = -1);
	int extract_flg(int item = -1);
	double extract_dbl(int item = -1);
	const char *extract_txt(int item = -1); // Shallow copy
	const void *extract_bin(int item = -1); // Shallow copy
	int num_bytes(int item = -1);
	sdatetime *extract_clk(int item = -1);
	slocation *extract_loc(int item = -1);
	const char *extract_value(int item = -1); // Shallow copy
	int extract_enum(int item = -1);
	// The latter is only defined on values which have been validated

	int extract_int(const char *name);
	int extract_flg(const char *name);
	double extract_dbl(const char *name);
	const char *extract_txt(const char *name); // Shallow copy
	const void *extract_bin(const char *name); // Shallow copy
	int num_bytes(const char *name);
	sdatetime *extract_clk(const char *name);
	slocation *extract_loc(const char *name);
	const char *extract_value(const char *name); // Shallow copy
	int extract_enum(const char *name);
	// The latter is only defined on values which have been validated

	NodeType get_type();

	// Searching:
	int exists(const char *name); // Returns 0 or 1
	snode *find(const char *name);
	snode *follow_path(svector *path); // Returns NULL if no such path
	int equals(snode *sn); // -1 if incomparable due to types
	int lessthan(snode *sn); // -1 if incomparable due to types

	private:
	
	snode();
	
	NodeType type;
	const char *name; // NULL if not specified
	
	int n; // SInt, SBool, SValue (cooked)
	double x; // SDouble
	int len; // SBinary
	char *data; // SBinary
	char *s; // SText, SValue (raw)
	sdatetime *time; // SDateTime
	slocation *loc; // SLocation
	snodevector *children; // SStruct, SList

	static void spaces(int n);
	static void spaces(int n, StringBuf *sb);
	static void dump_text(const char *s, int offset);
	static int string_needs_quotes(const char *s);
	static int string_is_multiline(const char *s);
	static const char *escape_string(const char *s);
	const char *name_string();
	snode *indirect(int item);
	snode *indirect(const char *name);
	void append(StringBuf *sb, int offset);
	void initialise();
	static snodevector *do_import(const char *s, const char **err, int single);

	/* Lots of friend declarations now: */
	
	// From builder.cpp:
	friend snode *mklist(const char *opt_name);
	friend snode *pack(snode **array, int n, const char *opt_name);

	friend snode *pack(snode *n1, const char *opt_name); //single elements should be to have a higher lvl struture
	friend snode *pack(snode *n1, snode *n2, const char *opt_name);
	friend snode *pack(snode *n1, snode *n2, snode *n3, const char *opt_name);
	friend snode *pack(snode *n1, snode *n2, snode *n3, snode *n4,
		const char *opt_name);
	friend snode *pack(snode *n1, snode *n2, snode *n3, snode *n4, snode *n5,
		const char *opt_name);
	friend snode *pack(snode *n1, snode *n2, snode *n3, snode *n4, snode *n5,
		snode *n6, const char *opt_name);
	friend snode *pack(snode *n1, snode *n2, snode *n3, snode *n4, snode *n5,
			snode *n6, snode *n7, const char *opt_name);
	
	// From marshall.cpp:
	friend int do_marshall(snode *node, litmus *lit, StringBuf *sb,
		svector *symbol_table);
	friend void marshall_primitive(snode *node, litmus *lit, StringBuf *sb,
		svector *symbol_table);
	friend void marshall_list(snode *node, litmus *lit, StringBuf *sb,
		svector *symbol_table);
	friend void marshall_composite(snode *node, litmus *lit, StringBuf *sb,
		svector *symbol_table);
	
	// From validate.cpp:
	friend int do_validate(snode *node, litmus *lit, svector *symbol_table);
	friend void validate_primitive(snode *node, litmus *lit,
		svector *symbol_table);
	friend void validate_list(snode *node, litmus *lit, svector *symbol_table);
	friend void validate_composite(snode *node, litmus *lit,
		svector *symbol_table);
	
	// From unmarshall.cpp:
	friend snode *do_unmarshall(const unsigned char *data, int bytes,
		litmus *lit, svector *symbol_table, int *consumed);
};

// Packing primitive types:
snode *pack(int n, const char *name = NULL); // int
snode *pack_bool(int n, const char *name = NULL); // flg
snode *pack(const char *s, const char *name = NULL); // txt, value
snode *pack(const void *data, int len, const char *name = NULL);
snode *pack(double x, const char *name = NULL);
snode *pack(sdatetime *time, const char *name = NULL);
snode *pack(slocation *loc, const char *name = NULL);

// int get_id(const char *value);

// Packing composite types (struct, lists):
snode *mklist(const char *name = NULL);
snode *pack(snode **array, int n, const char *name = NULL);
snode *pack(snode *n1, snode *n2, const char *name = NULL);
snode *pack(snode *n1, snode *n2, snode *n3, const char *name = NULL);
snode *pack(snode *n1, snode *n2, snode *n3, snode *n4,
		const char *name = NULL);
snode *pack(snode *n1, snode *n2, snode *n3, snode *n4, snode *n5,
		const char *name = NULL);
snode *pack(snode *n1, snode *n2, snode *n3, snode *n4, snode *n5, snode *n6,
		const char *name = NULL);
