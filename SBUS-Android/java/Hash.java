enum MetaType
{
	SCHEMA_NORM, SCHEMA_EMPTY, SCHEMA_POLY, SCHEMA_EXPR, SCHEMA_NA
};
	
class HashCode
{
	static byte[] hash(String s) // Canonical strings (from Schema)
	{
		byte[] result = new byte[6];

		/*
		2^48 = 281,474,976,710,656
		2^48 - 59 = prime = 281,474,976,710,597 = 16777215:16777157
		2^24 = 16,777,216
		*/
		
		int len = s.length();
		int c, carry;
		int x = 0, y = 0;
		int mod = 16777216;
		int p1 = 16777215, p2 = 16777157;
		int[] p1mult, p2mult;
		boolean comment = false;

		p1mult = new int[7];
		p2mult = new int[7];
		p1mult[0] = p1;
		p2mult[0] = p2;
		for(int i = 1; i < 7; i++)
		{
			p2mult[i] = p2mult[i - 1] * 2;
			p1mult[i] = p1mult[i - 1] * 2;
			if(p2mult[i] > mod)
			{
				p2mult[i] -= mod;
				p1mult[i]++;
			}
		}

		for(int i = 0; i < len; i++)
		{
			if(comment)
			{
				if(s.charAt(i) == '\n')
					comment = false;
				continue;
			}
			if(s.charAt(i) == ' ' || s.charAt(i) == '\t' || s.charAt(i) == '\n')
				continue; // Ignore whitespace
			if(s.charAt(i) == '*')
			{
				comment = true;
				continue;
			}
			// Shift and merge in next character:
			c = s.charAt(i) & 0x7F;
			carry = y >>> 17;
			y &= 0x1FFFF;
			y <<= 7;
			y |= c;
			x <<= 7;
			x |= carry;

			// Take modulo prime:
			for(int j = 6; j >= 0; j--)
			{
				if(x > p1mult[j] || (x == p1mult[j] && y >= p2mult[j]))
				{
					x -= p1mult[j];
					y -= p2mult[j];
					if(y < 0)
					{
						y += mod;
						x--;
					}
				}
			}

			// Sanity check:
			if(x >= mod || y >= mod)
				Log.error("Hash algorithm failure.\n");
		}

		result[0] = (byte)(x >>> 16);
		result[1] = (byte)((x >>> 8) & 0xFF);
		result[2] = (byte)(x & 0xFF);
		result[3] = (byte)(y >>> 16);
		result[4] = (byte)((y >>> 8) & 0xFF);
		result[5] = (byte)(y & 0xFF);

		return result;
	}

	HashCode()
	{
		buf = new byte[6];
	}

	HashCode(HashCode hc)
	{
		buf = new byte[6];
		if(hc == null)
		{
			for(int i = 0; i < 6; i++)
				buf[i] = (byte)0xEE;
		}
		else
		{
			for(int i = 0; i < 6; i++)
				buf[i] = hc.buf[i];
		}
	}

	byte[] tobinary()
	{
		byte[] result = new byte[6];
		for(int i = 0; i < 6; i++)
			result[i] = buf[i];
		return result;
	}

	public String toString()
	{
		StringBuilder sb = new StringBuilder();

		for(int i = 0; i < 6; i++)
		{
			sb.append(hex(byte_to_int(buf[i]) >>> 4));
			sb.append(hex(byte_to_int(buf[i]) & 0xF));
		}
		return sb.toString();
	}

	private int byte_to_int(byte b)
	{
		if(b < 0)
			return 256 + b;
		else
			return b;
	}
	
	void frombinary(byte[] b)
	{
		for(int i = 0; i < 6; i++)
			buf[i] = b[i];
	}

	void fromstring(String s)
	{
		for(int i = 0; i < 6; i++)
		{
			buf[i] = (byte)((dec(s.charAt(i * 2)) << 4) |
					dec(s.charAt(i * 2 + 1)));
		}
	}
	
	void frommeta(MetaType meta)
	{
		switch(meta)
		{
			case SCHEMA_EMPTY: fromstring("000000000000"); return;
			case SCHEMA_POLY:  fromstring("FFFFFFFFFFFF"); return;
			case SCHEMA_EXPR:  fromstring("111111111111"); return;
			case SCHEMA_NA:    fromstring("EEEEEEEEEEEE"); return;
			default: Log.error("Not a meta-schema in HashCode::frommeta()");
		}
	}

	void fromschema(String source) // Canonical strings (from Schema) only
	{
		byte[] hsh;

		hsh = hash(source);
		for(int i = 0; i < 6; i++)
			buf[i] = hsh[i];
	}
	
	boolean equals(HashCode hc)
	{
		for(int i = 0; i < 6; i++)
			if(buf[i] != hc.buf[i])
				return false;
		return true;
	}

	boolean isempty()
	{
		for(int i = 0; i < 6; i++)
			if(buf[i] != (byte)0)
				return false;
		return true;
	}

	boolean ispolymorphic()
	{
		for(int i = 0; i < 6; i++)
			if(buf[i] != (byte)0xFF)
				return false;
		return true;
	}

	boolean isapplicable()
	{
		for(int i = 0; i < 6; i++)
			if(buf[i] != (byte)0xEE)
				return true;
		return false;
	}
	
	int toint() // Quick conversion to a rather less unique 24 bit number
	{
		int x;

		x = (int)(buf[0] ^ buf[3]);
		x |= (int)(buf[1] ^ buf[4]) << 8;
		x |= (int)(buf[2] ^ buf[5]) << 16;
		return x;
	}
	
	private byte[] buf; // Binary representation

	private char hex(int d)
	{
		if(d >= 0 && d <= 9) return (char)(d + '0');
		if(d >= 10 && d <= 15) return (char)(d - 10 + 'A');
		Log.error("Not a hex digit in decimal to hex conversion (value " +
				d + ")");
		return '\0'; // Never happens
	}

	private int dec(char c)
	{
		if(c >= '0' && c <= '9') return c - '0';
		if(c >= 'A' && c <= 'F') return c - 'A' + 10;
		if(c >= 'a' && c <= 'f') return c - 'a' + 10;
		Log.error("Not a hex digit in hex to decimal conversion");
		return -1; // Never happens
	}
};
