import java.util.*;

class Datetime
{
	int year;   // e.g. 2009 (valid range 0-4095)
	int month;  // 1-12
	int day;    // 1-31
	int secs;   // 0-((3600*24)-1 = 86399 < 2^17)
	int micros; // 0-999999 < 2^20
	
	Datetime(int y, int m, int d, int s, int u)
	{
		year = y;
		month = m;
		day = d;
		secs = s;
		micros = u;
	}

	Datetime()
	{
		year = month = day = secs = micros = 0;
	}
	
	Datetime(Datetime copy)
	{
		year = copy.year;
		month = copy.month;
		day = copy.day;
		secs = copy.secs;
		micros = copy.micros;
	}
	
	Datetime(String s)
	{
		if(s.length() == 8)
		{
			from_binary(s.getBytes());
			return;
		}

		year = month = day = secs = micros = 0;
		Vector<Segment> v = Segment.separate(s);
		
		if(v.size() == 3 && v.get(0).delim == '/' &&
				v.get(1).delim == '/')
		{
			// Date only:
			day = v.get(0).value;
			month = v.get(1).value;
			year = v.get(2).value;
		}
		else if(v.size() == 3 && v.get(0).delim == ':' &&
			v.get(1).delim == ':')
		{
			// Time only, no subseconds:
			secs = v.get(0).value * 3600 + v.get(1).value * 60 +
					v.get(2).value;
		}
		else if(v.size() == 6 && v.get(0).delim == '/' &&
			v.get(1).delim == '/' && v.get(2).delim == ',' &&
			v.get(3).delim == ':' && v.get(4).delim == ':')
		{
			// Time and date, date first, no subseconds:
			day = v.get(0).value;
			month = v.get(1).value;
			year = v.get(2).value;
			secs = v.get(3).value * 3600 + v.get(4).value * 60 +
					v.get(5).value;
		}
		else if(v.size() == 6 && v.get(0).delim == ':' &&
			v.get(1).delim == ':' && v.get(2).delim == ',' &&
			v.get(3).delim == '/' && v.get(4).delim == '/')
		{
			// Time and date, time first, no subseconds:
			secs = v.get(0).value * 3600 + v.get(1).value * 60 +
					v.get(2).value;
			day = v.get(3).value;
			month = v.get(4).value;
			year = v.get(5).value;
		}
		else if(v.size() == 7 && v.get(0).delim == '/' &&
			v.get(1).delim == '/' && v.get(2).delim == ',' &&
			v.get(3).delim == ':' && v.get(4).delim == ':' &&
			v.get(5).delim == '.')
		{
			// Time and date, date first, with subseconds:
			day = v.get(0).value;
			month = v.get(1).value;
			year = v.get(2).value;
			secs = v.get(3).value * 3600 + v.get(4).value * 60 +
					v.get(5).value;
			micros = v.get(6).value * (1000000 / v.get(6).denom);
		}
		else if(v.size() == 7 && v.get(0).delim == ':' &&
			v.get(1).delim == ':' && v.get(2).delim == '.' &&
			v.get(3).delim == ',' && v.get(4).delim == '/' &&
			v.get(5).delim == '/')
		{
			// Time and date, time first, with subseconds:
			secs = v.get(0).value * 3600 + v.get(1).value * 60 +
					v.get(2).value;
			micros = v.get(3).value * (1000000 / v.get(3).denom);		
			day = v.get(4).value;
			month = v.get(5).value;
			year = v.get(6).value;
		}
		else
		{
			// Badformat, fields remain at zero
		}
	}

	public String toString()
	{
		String s;
		
		s = String.format("%02d:%02d:%02d.%06d,%02d/%02d/%04d",
				secs / 3600, (secs / 60) % 60, secs % 60, micros,
				day, month, year);

		return s;
	}

	byte[] encode()
	{
		byte[] data = new byte[8];

		data[0] = (byte)day;
		data[1] = (byte)((month << 4) | (year >>> 8));
		data[2] = (byte)(year & 0xFF);

		data[3] = (byte)((micros >>> 12) & 0xFF);
		data[4] = (byte)((micros >>> 4) & 0xFF);
		data[5] = (byte)(((micros & 0xF) << 4) | ((secs >> 16) & 0x1));
		data[6] = (byte)((secs >>> 8) & 0xFF);
		data[7] = (byte)(secs & 0xFF);

		return data;
	}
	
	void from_binary(byte[] data)
	{
		day = data[0];
		month = ((int)data[1] & 0xF0) >>> 4;
		year = (((int)data[1] & 0xF) << 8) | (int)data[2];
		micros = ((int)data[3] << 12) | ((int)data[4] << 4) |
				(((int)data[5] & 0xF0) >>> 4);
		secs = (((int)data[5] & 0x1) << 16) | ((int)data[6] << 8) | (int)data[7];
	}

	void set_now()
	{
		Calendar cal = new GregorianCalendar();
		
		year = cal.get(Calendar.YEAR);
		month = 1 + cal.get(Calendar.MONTH); // >= 0
		day = cal.get(Calendar.DAY_OF_MONTH); // >= 1
		secs = cal.get(Calendar.HOUR_OF_DAY) * 3600 +
				cal.get(Calendar.MINUTE) * 60 +
				cal.get(Calendar.SECOND);
		micros = cal.get(Calendar.MILLISECOND) * 1000;
	}
	
	int compare(Datetime d) // Returns -1 (lt), 0 (eq), 1 (gt)
	{
		if(year < d.year) return -1;
		if(year > d.year) return 1;
		if(month < d.month) return -1;
		if(month > d.month) return 1;
		if(day < d.day) return -1;
		if(day > d.day) return 1;
		if(secs < d.secs) return -1;
		if(secs > d.secs) return 1;
		if(micros < d.micros) return -1;
		if(micros > d.micros) return 1;
		return 0;
	}

	int address_field(String fieldname) // -1 if no such field
	{
		// field names: day, month, year, hour, min, sec, micros
		if(fieldname.equals("year"))
			return year;
		else if(fieldname.equals("month"))
			return month;
		else if(fieldname.equals("day"))
			return day;
		else if(fieldname.equals("hour"))
			return (secs / 3600);
		else if(fieldname.equals("min"))
			return ((secs % 3600) / 60);
		else if(fieldname.equals("sec"))
			return (secs % 60);
		else if(fieldname.equals("micros"))
			return micros;
		return -1;
	}

	static boolean typematch(String s)
	{
		// Pattern: d+/d+/d+ | d+:d+:d+[.d+] | d+:d+:d+[.d+],d+/d+/d+
		// Ignore whitespace around commas
		
		StringScanner ss = new StringScanner(s);
		try
		{
			ss.eat_integer();
			if(ss.eof()) return false;
			if(ss.getchar() == '/')
			{
				// Date first
				ss.advance();
				ss.eat_integer();
				ss.eat_literal('/');
				ss.eat_integer();
				if(ss.eof())
					return true;
				// Time second
				ss.eat_comma();
				ss.eat_integer();
				ss.eat_literal(':');
				ss.eat_integer();
				ss.eat_literal(':');
				ss.eat_integer();
				ss.eat_optional_fraction();
				if(ss.eof())
					return true;
				else
					return false;
			}
			else if(ss.getchar() == ':')
			{
				// Time first
				ss.advance();
				ss.eat_integer();
				ss.eat_literal(':');
				ss.eat_integer();
				ss.eat_optional_fraction();
				if(ss.eof())
					return true;
				// Date second
				ss.eat_comma();
				ss.eat_integer();
				ss.eat_literal('/');
				ss.eat_integer();
				ss.eat_literal('/');
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
};

class Location
{
	float lat, lon, elev;

	Location(float latitude, float longitude, float elevation)
	{
		lat = latitude;
		lon = longitude;
		elev = elevation;
	}
	
	Location(float latitude, float longitude)
	{
		lat = latitude;
		lon = longitude;
		elev = Float.NaN;
	}

	Location()
	{
		lat = lon = 0.0F;
		elev = Float.NaN;
	}
	
	Location(Location copy)
	{
		lat = copy.lat;
		lon = copy.lon;
		elev = copy.elev;
	}

	Location(String s)
	{
		lat = lon = 0.0F;
		elev = Float.NaN;
		String[] parts = s.split(",");
		
		if(parts.length < 2 || parts.length > 3)
			return; // No commas, or too many commas

		lat = Float.valueOf(parts[0]);
		lon = Float.valueOf(parts[1]);
		if(parts.length == 3)
			elev = Float.valueOf(parts[2]);
		else
			elev = Float.NaN;
	}
	
	public String toString()
	{
		String s;
		if(Float.isNaN(elev))
			s = String.format("%f,%f", lat, lon);
		else
			s = String.format("%f,%f,%f", lat, lon, elev);
		return s;
	}
			
	byte[] encode()
	{
		byte[] data = new byte[12];
		byte[] buf;

		buf = int_to_bytes(Float.floatToIntBits(lat));
		System.arraycopy(buf, 0, data, 0, 4);
		buf = int_to_bytes(Float.floatToIntBits(lon));
		System.arraycopy(buf, 0, data, 4, 4);
		buf = int_to_bytes(Float.floatToIntBits(elev));
		System.arraycopy(buf, 0, data, 8, 4);
		return data;
	}
	
	void from_binary(byte[] buf)
	{
		lat = Float.intBitsToFloat(bytes_to_int(buf, 0));
		lon = Float.intBitsToFloat(bytes_to_int(buf, 4));
		elev = Float.intBitsToFloat(bytes_to_int(buf, 8));
	}
	
	static byte[] int_to_bytes(int x)
	{
		byte[] buf = new byte[4];
		
		buf[0] = (byte)((x >>> 24) & 0xFF);
		buf[1] = (byte)((x >>> 16) & 0xFF);
		buf[2] = (byte)((x >>> 8) & 0xFF);
		buf[3] = (byte)((x >>> 0) & 0xFF);
		
		return buf;
	}
	
	static int bytes_to_int(byte[] b, int pos)
	{
		int x = 0;
		
		x += (int)b[0] << 24;
		x += (int)b[1] << 16;
		x += (int)b[2] << 8;
		x += (int)b[3] << 0;
		return x;
	}

	boolean height_defined()
	{
		if(Float.isNaN(elev))
			return false;
		return true;
	}
	
	boolean equals(Location l)
	{
		if(lat != l.lat || lon != l.lon)
			return false;
		if((!Float.isNaN(elev) || !Float.isNaN(l.elev)) && elev != l.elev)
			return false;
		return true;
	}

	boolean approxequals(Location l)
	{
		if(!floats_equal(lat, l.lat) || !floats_equal(lon, l.lon))
			return false;
		if((!Float.isNaN(elev) || !Float.isNaN(l.elev)) &&
				!floats_equal(elev, l.elev))
			return false;
		return true;
	}

	double address_field(String fieldname) // -10001 if no such field
	{
		if(fieldname.equals("lat"))
			return lat;
		else if(fieldname.equals("lon"))
			return lon;
		else if(fieldname.equals("elev"))
			return elev;
		return -10001.0;
	}
		
	static boolean typematch(String s)
	{
		// Pattern: [s]d+[.d+],[s]d+[.d+][,[s]d+[.d+]]
		// Ignore whitespace around commas

		StringScanner ss = new StringScanner(s);		

		try
		{
			ss.eat_optional_sign();
			ss.eat_integer();
			ss.eat_optional_fraction();
			ss.eat_comma();
			ss.eat_optional_sign();
			ss.eat_integer();
			ss.eat_optional_fraction();
			if(ss.eof())
				return true;
			ss.eat_comma();
			ss.eat_optional_sign();
			ss.eat_integer();
			ss.eat_optional_fraction();
			return true;
		}
		catch(MatchException e)
		{
			return false;
		}
	}

	boolean floats_equal(Float f1, Float f2)
	{
		Float temp;

		if(f1 > f2)
		{
			temp = f1;
			f1 = f2;
			f2 = temp;
		}
		f1 *= 1.00001F;
		if(f1 < f2)
			return false;
		else
			return true;
	}
};

// Private utility class:

class Segment
{
	char delim; // Delimiter character after this value/denom
	int value;
	int denom;
	
	static Vector<Segment> separate(String s)
	{
		Vector<Segment> v;
		int pos = 0, len = s.length();
		Segment seg;

		v = new Vector<Segment>();
		while(s.charAt(pos) == ' ')
		{
			pos++;
			if(pos >= len) return v;
		}
		while(true)
		{
			seg = new Segment();
			seg.value = 0;
			seg.denom = 1;
			while(pos < len && ((s.charAt(pos) >= '0' && s.charAt(pos) <= '9') ||
				s.charAt(pos) == ' '))
			{
				if(s.charAt(pos) == ' ')
				{
					pos++;
					continue;
				}
				seg.value = seg.value * 10 + (s.charAt(pos) - '0');
				seg.denom *= 10;
				pos++;
			}
			if(pos >= len)
			{
				seg.delim = '\0';
				v.add(seg);
				break;
			}
			seg.delim = s.charAt(pos);		
			v.add(seg);
			pos++;
		}
		return v;
	}
};
