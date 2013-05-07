import java.util.*;
import java.io.*;

class MemFile
{
	byte[] data; // null if file could not be opened
	
	static final int bufsize = 100;
	
	MemFile(String filename)
	{
		FileInputStream fis;
		int bytes;
		byte[] buf;
		ByteEncoder be;
		
		buf = new byte[bufsize];
		be = new ByteEncoder();
		try
		{
			fis = new FileInputStream(filename);
			while(true)
			{
				bytes = fis.read(buf);
				if(bytes < 0)
					break; // EOF
				if(bytes > 0)
					be.cat(buf, bytes);
			}
			fis.close();
		}
		catch(IOException e)
		{
			data = null;
			return;
		}
		data = be.extract();
	}
};

class StringScanner
{
	private String s;
	private int pos, len;

	StringScanner(String s)
	{
		this.s = s;
		len = s.length();
		pos = 0;
	}
	
	char getchar()
	{
		if(pos >= len)
			return '\0';
		return s.charAt(pos);
	}
	
	char lookahead(int n)
	{
		if(pos + n >= len)
			return '\0';
		return s.charAt(pos + n);
	}
	
	boolean eof()
	{
		if(pos < len)
			return false;
		return true;
	}
	
	void advance()
	{
		pos++;
	}
	
	void advance(int n)
	{
		pos += n;
	}
	
	int getpos()
	{
		return pos;
	}
	
	String substring(int from, int to) // from inclusive, to exclusive
	{
		return s.substring(from, to);
	}

	void eat_whitespace()
	{
		while(pos < len && (s.charAt(pos) == ' ' ||
				s.charAt(pos) == '\t' || s.charAt(pos) == '\n'))
			pos++;
	}

	void eat_optional_sign()
	{
		if(pos >= len) return;
		if(s.charAt(pos) == '-' || s.charAt(pos) == '+')
			pos++;
	}

	void eat_digits()
	{
		while(pos < len && is_digit(s.charAt(pos)))
			pos++;
	}

	void eat_optional_fraction() throws MatchException
	{
		if(pos >= len) return;
		if(s.charAt(pos) != '.')
			return;
		pos++;
		if(pos >= len) return;
		if(!is_digit(s.charAt(pos)))
			throw new MatchException();
		eat_digits();
	}

	void eat_integer() throws MatchException
	{
		if(pos >= len || !is_digit(s.charAt(pos)))
			throw new MatchException();
		eat_digits();
	}

	void eat_comma() throws MatchException
	{
		eat_whitespace();
		if(pos >= len || s.charAt(pos) != ',')
			throw new MatchException();
		pos++;
		eat_whitespace();
	}

	void eat_literal(char c) throws MatchException
	{
		if(pos >= len || s.charAt(pos) != c)
			throw new MatchException();
		pos++;
	}
	
	static boolean is_digit(char c)
	{
		if(c >= '0' && c <= '9')
			return true;
		else
			return false;
	}

	static boolean is_int(String s)
	{
		int len = s.length(), pos = 0;
		
		if(pos >= len) return false;
		if(s.charAt(pos) == '-')
			pos++;
		if(pos >= len) return false;
		while(pos < len)
		{
			if(!is_digit(s.charAt(pos)))
				return false;
			pos++;
		}
		return true;
	}

	static boolean is_dbl(String s)
	{
		// Pattern: [s]d+[.d+][e[s]d+]
		
		StringScanner ss = new StringScanner(s);
		try
		{
			ss.eat_optional_sign();
			ss.eat_integer();
			ss.eat_optional_fraction();
			if(ss.eof())
				return true;
			else if(ss.getchar() == 'e')
			{
				ss.advance();
				ss.eat_optional_sign();
				ss.eat_integer();
				if(ss.eof())
					return true;
				else
					return false;
			}
			else
				return false;
		}
		catch(MatchException e)
		{
			return false;
		}
	}
	
	static boolean is_alphabetic(char c)
	{
		if(c >= 'a' && c <= 'z')
			return true;
		if(c >= 'A' && c <= 'Z')
			return true;
		if(c == '_')
			return true;
		return false;
	}

	static boolean is_alphanumeric(char c)
	{
		if(is_digit(c) || is_alphabetic(c))
			return true;
		else
			return false;
	}

	static boolean is_hex(char c)
	{
		if(c >= '0' && c <= '9')
			return true;
		else if(c >= 'A' && c <= 'F')
			return true;
		else if(c >= 'a' && c <= 'f')
			return true;
		else
			return false;
	}
	
	static int hex_val(char c)
	{
		if(c >= '0' && c <= '9')
			return (c - '0');
		if(c >= 'A' && c <= 'F')
			return (c - 'A') + 10;
		if(c >= 'a' && c <= 'f')
			return (c - 'a') + 10;
		return -1;
	}

	byte read_hexbyte() // Returns -1 if not a hexbyte
	{
		byte b;

		if(pos + 1 >= len)
			return -1;
		if(!is_hex(s.charAt(pos)) || !is_hex(s.charAt(pos + 1)))
			return -1;
		b = (byte)(hex_val(s.charAt(pos)) << 4);
		b |= (byte)hex_val(s.charAt(pos + 1));
		pos += 2;
		return b;
	}
	
	String read_tag_content() throws ImportException
	{
		// Body data
		boolean escape_char = false, inside_quotes = false;
		StringConstructor sc = new StringConstructor();
		char c;

		while(true)
		{
			if(eof())
				importerror("End-of-file inside unterminated data");
			c = s.charAt(pos);
			if(c == '<')
			{
				if(escape_char || inside_quotes)
				{
					sc.cat('<');
					escape_char = false;
				}
				else
					break;
			}
			else if(c == '"')
			{
				sc.cat('"');
				if(escape_char)
					escape_char = false;
				else
					inside_quotes = !inside_quotes;
			}
			else if(c == '\\')
			{
				if(escape_char)
				{
					sc.cat('\\');
					escape_char = false;
				}
				else
					escape_char = true;
			}
			else
				sc.cat(c);
			pos++;
		}
		sc.truncate_spaces();
		return sc.extract();
	}

	String read_tag_name() throws ImportException
	{
		StringConstructor sc = new StringConstructor();
		char c;
		
		// Element name:
		while(true)
		{
			if(eof())
				importerror("End-of-file in middle of tag");
			c = s.charAt(pos);
			if(c == '>' || c == '/' || c == ' ' || c == '\t' || c == '\n')
				break;
			sc.cat(c);
			pos++;
		}
		eat_whitespace();
		if(eof())
			importerror("Tag name '" + sc.extract() + "' ends string");
		if(s.charAt(pos) != '>' && s.charAt(pos) != '/')
		{
			importerror("Tag name '" + sc.extract() + "' must not terminate " +
					"with '" + s.charAt(pos) + "'");
		}
		return sc.extract();
	}
	
	static void importerror(String msg) throws ImportException
	{
		throw new ImportException(msg);
	}
};

class StringConstructor
{
	StringConstructor()
	{
		sb = new StringBuilder();
	}
	
	void cat(char c)
	{
		sb.append(c);
	}
	
	void cat(String s)
	{
		sb.append(s);
	}
	
	void cat(double d)
	{
		String s = String.valueOf(d);
		if(!s.contains(".") && !s.contains("e") && !s.contains("E"))
			s = s + ".0";
		sb.append(s);
	}
	
	void cat_int(int n)
	{
		sb.append(String.valueOf(n));
	}
	
	void cat_hexbyte(byte b)
	{
		int d;

		d = b >>> 4;
		if(d < 10)
			sb.append((char)d + '0');
		else
			sb.append((char)(d - 10) + 'A');
		d = b & (byte)0xF;
		if(d < 10)
			sb.append((char)d + '0');
		else
			sb.append((char)(d - 10) + 'A');
	}

	void indent(int n)
	{
		if(n < 1) // -1 or 0
			return;
		for(int i = 0; i < n * 3; i++)
			sb.append(' ');
	}

	String extract()
	{
		return sb.toString();
	}
	
	void truncate_spaces()
	{
		int used = sb.length();
		char c;

		while(used > 0)
		{
			c = sb.charAt(used - 1);
			if(c != ' ' && c != '\t' && c != '\n')
				break;
			sb.deleteCharAt(used - 1);
			used--;
		}
	}

	private StringBuilder sb;
};

class ByteDecoder
{
	private byte[] buf;
	private int pos, len;

	ByteDecoder(byte[] b)
	{
		buf = b;
		len = b.length;
		pos = 0;
	}
	
	byte[] tail() // Returns all the remaining bytes
	{
		if(pos >= len)
			return new byte[0];
		return Arrays.copyOfRange(buf, pos, len);
	}
	
	int decode_count()
	{
		int n;
		int b;

		b = Utils.btoi(buf[pos]);
		if(b == 255)
		{
			n = (Utils.btoi(buf[pos + 1]) << 24) |
					(Utils.btoi(buf[pos + 2]) << 16) |
					(Utils.btoi(buf[pos + 3]) << 8) | Utils.btoi(buf[pos + 4]);
			pos += 5;
		}
		else if(b == 254)
		{
			n = (Utils.btoi(buf[pos + 1]) << 8) | Utils.btoi(buf[pos + 2]);
			pos += 3;
		}
		else
		{
			n = b;
			pos += 1;
		}
		return n;
	}

	String decode_string()
	{
		int len;
		String str;

		len = decode_count();
		if(len == 0)
			return null; // The empty string is converted back to null
		str = new String(buf, pos, len);
		pos += len;
		return str;
	}

	byte decode_byte()
	{
		byte b = buf[pos];
		pos++;
		return b;
	}

	HashCode decode_hashcode()
	{
		HashCode hc;

		hc = new HashCode();
		hc.frombinary(Arrays.copyOfRange(buf, pos, pos + 6));
		pos += 6;
		return hc;
	}

	boolean eof()
	{
		if(pos < len)
			return false;
		return true;
	}
	
	void advance()
	{
		pos++;
	}
	
	void advance(int n)
	{
		pos += n;
	}
	
	int getpos()
	{
		return pos;
	}
};

class ByteEncoder
{
	private byte[] buf; // Internal buffer
	private int used, capacity;

	/*
	private char *varbuf, *spcbuf;
	private int maxvarbuf, maxspcbuf;
	*/

	ByteEncoder()
	{
		capacity = 50;
		buf = new byte[capacity];
		used = 0;
		/*
		varbuf = spcbuf = null;
		maxspcbuf = 0;
		maxvarbuf = 100;
		*/
	}

	ByteEncoder(ByteEncoder be)
	{
		used = be.used;
		capacity = be.capacity;
		buf = be.buf.clone();
		/*
		varbuf = spcbuf = null;
		maxspcbuf = 0;
		maxvarbuf = 100;
		*/
	}

	byte[] extract()
	{
		return Arrays.copyOf(buf, used);
	}

	/*
	int save(const char *filename); // Returns 0 if OK, else -1
	boolean transmit(int fd); // Returns true if OK, else false
	void print();
	void dump();
	void log();
	char getcharacter(int n);
	void cat(char c);
	void cat(double d);
	void cat_int(int n);
	void cat_hexbyte(unsigned char c); // Encodes as 2 characters
	void cat_spaces(int n);
	void catf(const char *format, ...);
	char pop();
	void truncate_spaces();
	int iswhitespace(); // Empty strings count as whitespace
	*/	

	void cat(String s)
	{
		byte[] data = s.getBytes();
		cat(data);
	}

	void cat(byte[] data)
	{
		int len = data.length;
		
		if(len == 0)
			return;
		check_expand(used + len);
		System.arraycopy(data, 0, buf, used, len);
		used += len;
	}

	void cat(byte[] data, int count)
	{
		if(count < 1)
			return;
		if(count > data.length)
			count = data.length;
		check_expand(used + count);
		System.arraycopy(data, 0, buf, used, count);
		used += count;
	}

	void cat_byte(byte b)
	{
		cat(b);
	}
	
	void cat_byte(int n)
	{
		cat((byte)n);
	}
	
	void cat(byte b)
	{
		check_expand(used + 1);
		buf[used++] = b;
	}
	
	void cat(int n) // Encodes as a count; 1 to 5 bytes
	{
		check_expand(used + 5);
		if(n < 0)
		{
			Log.error("Counts cannot be negative (" + n + ").");
		}
		if(n < 254)
		{
			buf[used++] = (byte)n;
		}
		else if(n < 65536)
		{
			buf[used++] = (byte)254;
			buf[used++] = (byte)(n >>> 8);
			buf[used++] = (byte)(n & 0xFF);
		}
		else
		{
			buf[used++] = (byte)255;
			buf[used++] = (byte)(n >>> 24);
			buf[used++] = (byte)((n >>> 16) & 0xFF);
			buf[used++] = (byte)((n >>> 8) & 0xFF);
			buf[used++] = (byte)(n & 0xFF);
		}
	}

	void cat_word(int n) // Encodes as 4 bytes
	{
		check_expand(used + 4);
		buf[used++] = (byte)(n >>> 24);
		buf[used++] = (byte)((n >>> 16) & 0xFF);
		buf[used++] = (byte)((n >>> 8) & 0xFF);
		buf[used++] = (byte)(n & 0xFF);
	}

	void cat_string(String s) // Encodes as a count followed by data
	{
		if(s == null)
		{
			// null is converted into the empty string:
			cat(0);
			return;
		}
		cat(s.length()); // Count
		cat(s); // Data
	}
	
	void cat(HashCode hc)
	{
		byte[] hash;

		hash = hc.tobinary();
		cat(hash);
	}

	void skip(int n)
	{
		check_expand(used + n);
		used += n;
	}

	void overwrite(int pos, byte b)
	{
		if(pos < 0 || pos >= used)
			return;
		buf[pos] = b;
	}

	void overwrite_word(int pos, int n)
	{
		if(pos < 0 || pos + 3 >= used)
			return;
		buf[pos++] = (byte)(n >>> 24);
		buf[pos++] = (byte)((n >>> 16) & 0xFF);
		buf[pos++] = (byte)((n >>> 8) & 0xFF);
		buf[pos++] = (byte)(n & 0xFF);
	}

	void clear()
	{
		// Keep the same capacity
		used = 0;
	}

	int length()
	{
		return used;
	}

	private void check_expand(int required)
	{
		if(required > capacity)
		{
			byte[] new_buf;
			int new_cap;

			new_cap = required * 2;	
			new_buf = new byte[new_cap];
			System.arraycopy(buf, 0, new_buf, 0, used);
			capacity = new_cap;
			buf = new_buf;
		}
	}
};
