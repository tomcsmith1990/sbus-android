// hash.h - DMI - 12-4-2007

enum MetaType
{
	SCHEMA_NORM, SCHEMA_EMPTY, SCHEMA_POLY, SCHEMA_EXPR, SCHEMA_NA
};
	
class HashCode
{
	public:
	
	static unsigned char *hash(const char *s); // Canonical strings (from Schema)

	HashCode();
	HashCode(HashCode *hc);
	~HashCode();
			
	unsigned char *tobinary(); // Caller must delete result
	char *tostring();          // Caller must delete result
	void frombinary(const unsigned char *s);
	void fromstring(const char *s);
	
	void frommeta(MetaType meta);
	void fromschema(const char *source); // Canonical strings (from Schema) only
	
	int equals(HashCode *hc);
	int isempty();
	int ispolymorphic();
	int isapplicable();
	int toint(); // Quick conversion to a rather less unique 24 bit number
	
	private:
	
	unsigned char *buf; // Binary representation
};
