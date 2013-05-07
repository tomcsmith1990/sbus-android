// dimension.h - DMI - 19-2-2007

class sdatetime
{
	public:
	
	int year;
	int month;
	int day;
	int secs;
	int micros;
	
	sdatetime(int y, int m, int d, int s, int u);
	sdatetime();
	sdatetime(const char *s);
	sdatetime(sdatetime *copy);
	
	const char *tostring();
	unsigned char *encode();
	void from_binary(const unsigned char *s);
	void set_now();
	
	int compare(sdatetime *d);	// Returns -1 (lt), 0 (eq), 1 (gt)
	int address_field(const char *fieldname); // -1 if no such field

	static int typematch(const char *s);
};

class slocation
{
	public:
	
	float lat, lon, elev;

	slocation(float latitude, float longitude, float elevation);
	slocation(float latitude, float longitude);
	slocation();
	slocation(const char *s);
	slocation(slocation *copy);
		
	const char *tostring();
	unsigned char *encode();
	void from_binary(const unsigned char *s);
	int height_defined();
	
	int equals(slocation *l);
	int approxequals(slocation *l);
	double address_field(const char *fieldname); // -10001 if no such field
		
	static int typematch(const char *s);
};

const char *eat_whitespace(const char *s);
const char *eat_optional_sign(const char *s);
const char *eat_digits(const char *s);
const char *eat_optional_fraction(const char *s);
const char *eat_integer(const char *s);
const char *eat_comma(const char *s);
const char *eat_literal(const char *s, char c);
int is_digit(char c);
int is_alphabetic(char c);
int is_alphanumeric(char c);
int is_hex(char c);
int read_hexbyte(const char *s); // Returns -1 if not a hexbyte
