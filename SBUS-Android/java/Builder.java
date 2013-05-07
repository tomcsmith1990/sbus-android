import java.util.*;

enum NodeType
{
	Int, Double, Text, Binary, Bool, Datetime, Location,
	Struct, List, Value, Empty;
	
	static String to_string(NodeType t)
	{
		String s = "Unknown";

		switch(t)
		{
			case Int:      s = "Int"; break;
			case Double:   s = "Double"; break;
			case Text:     s = "Text"; break;
			case Binary:   s = "Binary"; break;
			case Bool:     s = "Bool"; break;
			case Datetime: s = "DateTime"; break;
			case Location: s = "Location"; break;
			case Struct:   s = "Struct"; break;
			case List:     s = "List"; break;
			case Value:    s = "Value"; break;
			case Empty:    s = "Empty"; break;
			default:
				Log.error("Unknown node type " + t);
				break;
		}
		return s;
	}
};

class Node
{
	// Constructors:
	
	protected Node(int n)
	{
		initialise();
		type = NodeType.Int; // Assume int until converted by schema
		this.n = n;
	}

	protected Node(boolean bool)
	{
		initialise();
		type = NodeType.Bool; // Assume int until converted by schema
		if(bool)
			this.n = 1;
		else
			this.n = 0;
	}

	protected Node(String s)
	{
		initialise();
		type = NodeType.Text; // Assume txt until converted by schema
		this.s = s;
	}

	protected Node(byte[] data)
	{
		initialise();
		type = NodeType.Binary;
		this.data = data.clone();
	}

	protected Node(double x)
	{
		initialise();
		type = NodeType.Double;
		this.x = x;
	}

	protected Node(Datetime time)
	{
		initialise();
		type = NodeType.Datetime;
		this.time = new Datetime(time);
	}

	protected Node(Location loc)
	{
		initialise();
		type = NodeType.Location;
		this.loc = new Location(loc);
	}

	protected Node()
	{
		// type not specified yet
		initialise();
	}
	
	private void initialise()
	{
		s = null;
		n = -1;
		data = null;
		name = null;
		children = null;
		time = null;
		loc = null;
	}
	
	static Node create_empty(String opt_name)
	{
		Node node = new Node();
		node.initialise();
		node.type = NodeType.Empty;
		node.name = opt_name;
		return node;
	}
	static Node create_empty()
	{
		Node node = new Node();
		node.initialise();
		node.type = NodeType.Empty;
		return node;
	}	
	
	static protected Node create_bool(boolean bool, String opt_name)
	{
		Node node = new Node();
		node.type = NodeType.Bool;
		if(bool)
			node.n = 1;
		else
			node.n = 0;
		node.name = opt_name;
		return node;
	}
	static protected Node create_bool(boolean bool)
	{
		Node node = new Node();
		node.type = NodeType.Bool;
		if(bool)
			node.n = 1;
		else
			node.n = 0;
		return node;
	}

	static Node create_value(String s, String opt_name)
	{
		Node node = new Node();
		node.type = NodeType.Value;
		node.s = s;
		node.name = opt_name;
		return node;
	}
	static Node create_value(String s)
	{
		Node node = new Node();
		node.type = NodeType.Value;
		node.s = s;
		return node;
	}
	
	Node(Node sn, String override_name) // Cloning operation
	{
		init_clone(sn);
		name = override_name;
	}
	Node(Node sn) // Cloning operation
	{
		init_clone(sn);
	}
	private void init_clone(Node sn)
	{
		// Cloning operation:
		type = sn.type;
		name = sn.name;
		n = sn.n;
		x = sn.x;
		s = sn.s;
		if(sn.data != null)
			data = sn.data.clone();
		else
			data = null;
		if(sn.children != null)
		{
			children = new Vector<Node>();
			for(int i = 0; i < sn.children.size(); i++)
				children.add(new Node(sn.children.get(i)));
		}
		else
			children = null;

		time = ((sn.time == null) ? null : new Datetime(sn.time));
		loc = ((sn.loc == null) ? null : new Location(sn.loc));	
	}

	// State:
	
	protected NodeType type;
	protected String name;   // null if not specified
	
	protected int n;         // SInt, SBool, SValue (cooked)
	protected double x;      // SDouble
	protected byte[] data;   // SBinary
	protected String s;      // SText, SValue (raw)
	protected Datetime time; // SDateTime
	protected Location loc;  // SLocation
	protected Vector<Node> children; // SStruct, SList
	
	// Debugging:
			
	void dump()
	{
		dump(0);
	}
	void dump(int offset)
	{
		String repr;

		spaces(offset);
		switch(type)
		{
			case Int:
				System.out.printf("Integer (%s): %d\n", name_string(), n);
				break;
			case Double:
				System.out.printf("Double (%s): %g\n", name_string(), x);
				break;
			case Text:
				System.out.printf("Text (%s): ", name_string());
				dump_text(s, offset);
				System.out.println("");
				break;
			case Empty:
				System.out.printf("Empty (%s)\n", name_string());
				break;
			case Binary:
				System.out.printf("Binary (%s): [%d bytes]\n", name_string(),
						data.length);
				break;
			case Bool:
				if(n < 0 || n > 1)
					Log.error("Boolean out of range");
				System.out.printf("Boolean (%s): %s\n", name_string(),
						n == 1 ? "true" : "false");
				break;
			case Datetime:
				repr = time.toString();
				System.out.printf("Datetime (%s): %s\n", name_string(), repr);
				break;
			case Location:
				repr = loc.toString();
				System.out.printf("Location (%s): %s\n", name_string(), repr);
				break;
			case Struct:
				System.out.printf("Structure (%s):\n", name_string());
				for(int i = 0; i < children.size(); i++)
					children.get(i).dump(offset + 1);
				break;
			case List:
				System.out.printf("List (%s):\n", name_string());
				for(int i = 0; i < children.size(); i++)
					children.get(i).dump(offset + 1);
				break;
			case Value:
				System.out.printf("Enumeration (%s): ", name_string());
				if(s != null)
					System.out.printf("#%s, index %d\n", s, n);
				else
					System.out.printf("#%d\n", n);
				break;
			default:
				Log.error("Impossible node type.");
		}
	}
	
	/* Extracting */
	
	int count()
	{
		if(children != null)
			return children.size();
		return -1;
	}

	Node extract_item(int item)
	{
		return indirect(item);
	}

	Node extract_item(String name)
	{
		return indirect(name);
	}

	Node[] extract_array()
	{
		Node[] array;
		int size;

		if(children == null)
			return null;
		size = children.size();
		array = new Node[size];
		for(int i = 0; i < size; i++)
			array[i] = children.get(i);
		return array;
	}

	String get_name(int opt_item)
	{
		return indirect(opt_item).name;
	}
	String get_name() { return get_name(-1); }

	// "item" may be -1 for the current item, >= 0 for a child index
	int extract_int()
	{
		if(type != NodeType.Int)
		{
			Log.error("Type mismatch: type " + type.toString() +
					" for element " + get_name() + " is not an integer.");
		}
		return n;
	}
	int extract_int(int opt_item)
	{
		return (indirect(opt_item).extract_int());
	}

	boolean extract_flg()
	{
		if(type != NodeType.Bool)
		{
			Log.error("Type mismatch: type " + type.toString() +
					" for element " + get_name() + " is not a boolean.");
		}
		if(n == 0)
			return false;
		else if(n == 1)
			return true;
		else
			Log.error("Boolean out of range.");
		return false; // Never occurs
	}
	boolean extract_flg(int opt_item)
	{
		return (indirect(opt_item).extract_flg());
	}

	double extract_dbl()
	{
		if(type != NodeType.Double)
		{
			Log.error("Type mismatch: type " + type.toString() +
					" for element " + get_name() + " is not floating point.");
		}
		return x;
	}
	double extract_dbl(int opt_item)
	{
		return (indirect(opt_item).extract_dbl());
	}

	String extract_txt()
	{
		if(type != NodeType.Text)
		{
			Log.error("Type mismatch: type " + type.toString() +
					" for element " + get_name() + " is not textual.");
		}
		return s;
	}
	String extract_txt(int opt_item)
	{
		return (indirect(opt_item).extract_txt());
	}

	byte[] extract_bin() // Shallow copy
	{
		if(type != NodeType.Binary)
		{
			Log.error("Type mismatch: type " + type.toString() +
					" for element " + get_name() + " is not binary.");
		}
		return data;
	}
	byte[] extract_bin(int opt_item) // Shallow copy
	{
		return (indirect(opt_item).extract_bin());
	}
	
	int num_bytes()
	{
		if(type != NodeType.Binary)
		{
			Log.error("Type mismatch: type " + type.toString() +
					" for element " + get_name() + " is not binary.");
		}
		return data.length;
	}
	int num_bytes(int opt_item)
	{
		return (indirect(opt_item).num_bytes());
	}

	Datetime extract_clk()
	{
		if(type != NodeType.Datetime)
		{
			Log.error("Type mismatch: type " + type.toString() +
					" for element " + get_name() + " is not a timestamp.");
		}
		return time;
	}
	Datetime extract_clk(int opt_item)
	{
		return (indirect(opt_item).extract_clk());
	}

	Location extract_loc()
	{
		if(type != NodeType.Location)
		{
			Log.error("Type mismatch: type " + type.toString() +
					" for element " + get_name() + " is not a location.");
		}
		return loc;
	}
	Location extract_loc(int opt_item)
	{
		return (indirect(opt_item).extract_loc());
	}

	String extract_value()
	{
		if(type != NodeType.Value)
		{
			Log.error("Type mismatch: type " + type.toString() +
					" for element " + get_name() + " is not an enumeration.");
		}
		return s;
	}
	String extract_value(int opt_item)
	{
		return (indirect(opt_item).extract_value());
	}

	int extract_enum()
	{
		if(type != NodeType.Value)
		{
			Log.error("Type mismatch: type " + type.toString() +
					" for element " + get_name() + " is not an enumeration.");
		}
		return n;
	}
	int extract_enum(int opt_item)
	{
		return (indirect(opt_item).extract_enum());
	}
	// extract_enum() is only defined on values which have been validated

	int extract_int(String name)
	{
		return (indirect(name).extract_int());
	}

	boolean extract_flg(String name)
	{
		return (indirect(name).extract_flg());
	}

	double extract_dbl(String name)
	{
		return (indirect(name).extract_dbl());
	}

	String extract_txt(String name)
	{
		return (indirect(name).extract_txt());
	}

	byte[] extract_bin(String name) // Shallow copy
	{
		return (indirect(name).extract_bin());
	}

	int num_bytes(String name)
	{
		return (indirect(name).num_bytes());
	}

	Datetime extract_clk(String name)
	{
		return (indirect(name).extract_clk());
	}

	Location extract_loc(String name)
	{
		return (indirect(name).extract_loc());
	}

	String extract_value(String name)
	{
		return (indirect(name).extract_value());
	}

	int extract_enum(String name)
	{
		return (indirect(name).extract_enum());
	}
	// extract_enum() is only defined on values which have been validated

	NodeType get_type()
	{
		return type;
	}

	// Searching:
	boolean exists(String name)
	{
		Node node;

		if(type != NodeType.Struct || name == null)
			return false;
		for(int i = 0; i < children.size(); i++)
		{
			node = children.get(i);
			if(name.equals(node.name))
				return (node.type == NodeType.Empty ? false : true);
		}
		return false;
	}

	Node follow_path(Vector<String> path) // Returns null if no such path
	{
		Node node = this;
		Vector<Node> children;
		String branch;
		int index;

		if(node.type != NodeType.Struct && node.type != NodeType.List &&
				path.size() == 1 && node.name.equals(path.get(0)))
		{
			// Single element with matching name, that's OK:
			return node;
		}
		for(int i = 0; i < path.size(); i++)
		{
			if(node.type != NodeType.Struct && node.type != NodeType.List)
				return null;
			children = node.children;

			branch = path.get(i);
			if(branch.charAt(0) == '#')
			{
				index = Integer.valueOf(branch.substring(1));
				if(index < 0 || index >= children.size())
					return null;
				node = children.get(index);
			}
			else
			{
				Node subnode;

				node = null;
				for(int j = 0; j < children.size(); j++)
				{
					subnode = children.get(j);
					if(branch.equals(subnode.name))
					{
						node = subnode;
						break;
					}
				}
				if(node == null)
					return null;
			}
		}
		return node;
	}
	
	boolean equals(Node sn) throws IncomparableTypesException
	{
		int i;

		if(type != sn.type)
			throw new IncomparableTypesException();
		switch(type)
		{
			case Int:
				return (n == sn.n);
			case Double:
				return (x == sn.x);
			case Text:
				return s.equals(sn.s);
			case Binary:
				return Arrays.equals(data, sn.data);
			case Bool:
				return (n == sn.n);
			case Datetime:
				return (time.compare(sn.time) == 0);
			case Location:
				return loc.equals(sn.loc);
			case Struct:
				if(children.size() != sn.children.size())
					return false;
				for(i = 0; i < children.size(); i++)
					if(!children.get(i).equals(sn.children.get(i)))
						return false;
				return true;
			case List:
				if(children.size() != sn.children.size())
					return false;
				for(i = 0; i < children.size(); i++)
					if(!children.get(i).equals(sn.children.get(i)))
						return false;
				return true;
			case Value:
				if(n != -1 && sn.n != -1)
				{
					// Fast comparison:
					return (n == sn.n);
				}
				else if(s != null && sn.s != null)
				{
					// Slow comparison:
					return s.equals(sn.s);
				}
				else
				{
					// This can probably happen, but we should fix it so it can't:
					Log.error("Cannot compare enumerations in numeric and " +
							"string form");
				}
				break;
			case Empty:
				return true; // Empty things are equal
			default:
				Log.error("switch error in node comparison");
		}
		throw new IncomparableTypesException();
	}

	int lessthan(Node sn) throws IncomparableTypesException
	{
		int cmp;
		
		if(type != sn.type)
			throw new IncomparableTypesException();
		switch(type)
		{
			case Int:
				if(n < sn.n) return -1;
				if(n > sn.n) return 1;
				return 0;
			case Double:
				if(x < sn.x) return -1;
				if(x > sn.x) return 1;
				return 0;
			case Text:
				cmp = s.compareTo(sn.s);
				if(cmp < 0) return -1;
				if(cmp > 0) return 1;
				return 0;
			case Binary:
				throw new IncomparableTypesException();
				// No comparison function defined on arbitrary bits
			case Bool:
				throw new IncomparableTypesException();
				// Doesn't make sense to order just true and false
			case Datetime:
				return time.compare(sn.time);
			case Location:
				throw new IncomparableTypesException();
				// 2D/3D space isn't ordered
			case Struct:
				throw new IncomparableTypesException();
				// Structures aren't comparable
			case List:
				// We interpret this as comparing list *length*:
				if(children.size() < sn.children.size()) return -1;
				if(children.size() > sn.children.size()) return 1;
				return 0;
			case Value:
				if(n != -1 && sn.n != -1)
				{
					// Fine, have numeric values for both, can do comparison:
					if(n < sn.n) return -1;
					if(n > sn.n) return 1;
					return 0;
				}
				else
				{
					// This can probably happen, but we should fix it so it can't:
					Log.error("Enumerations must be numeric for order comparisons");
				}
				break;
			case Empty:
				throw new IncomparableTypesException();
				// No order relation on empty things
			default:
				Log.error("switch error in node comparison");
		}
		throw new IncomparableTypesException();
	}

	private static void spaces(int n)
	{
		for(int i = 0; i < n * 3; i++)
			System.out.print(" ");
	}

	private static void dump_text(String s, int offset)
	{
		char c;
		int pos = 0, len = s.length();

		if(!string_is_multiline(s))
		{
			while(pos < s.length() &&
					(s.charAt(pos) == ' ' || s.charAt(pos) == '\t'))
				pos++;
			System.out.print(s.substring(pos));
			return;
		}
		System.out.print("\n");
		spaces(offset + 1);
		System.out.print(">");
		while(true)
		{
			if(pos >= len)
				break;
			c = s.charAt(pos);
			if(c == '\n')
			{
				System.out.print("\n");
				spaces(offset + 1);
				System.out.print(">");
				pos++;
				/*
				while(pos < s.length &&
						(s.charAt(pos) == ' ' || s.charAt(pos) == '\t'))
					pos++;
				*/
			}
			else
			{
				if(c == '\t')
					System.out.print("   ");
				else
					System.out.print(c);
				pos++;
			}
		}
	}
	
	static protected boolean string_is_multiline(String s)
	{
		return s.contains("\n");
	}
	
	protected String name_string()
	{
		if(name != null)
			return name;
		return "-";
	}

	// "item" may be -1 for the current item, >= 0 for a child index */

	private Node indirect(int item)
	{
		Node node = null;

		if(item == -1)
		{
			return this;
		}
		else if(item >= 0)
		{
			if(type != NodeType.Struct && type != NodeType.List)
			{
				Log.error("Extracting by index from a type <"
						+ name + "> with no sub-items.");
			}
			if(item >= children.size())
			{
				Log.error("Sub-item index for extraction out of range in element <"
						+ name + ">.");
			}
			node = children.get(item);
		}
		else
			Log.error("Invalid sub-item indirection.");
		return node;
	}

	private Node indirect(String name)
	{
		Node node;

		if(type != NodeType.Struct)
		{
			Log.error("Extracting by name <" + name +
					"> from a non-structure type <" + this.name + ">");
		}
		if(name == null)
			Log.error("Selected null sub-item.");
		for(int i = 0; i < children.size(); i++)
		{
			node = children.get(i);
			if(node.name.equals(name))
				return node;
		}
		Log.error("No sub-item with specified name '" + name +
				"' in structure '" + this.name + "'");
		return null; // Never happens
	}

	/*	
	// From marshall.cpp:
	friend int do_marshall(Node *node, litmus *lit, StringBuf *sb,
		svector *symbol_table);
	friend void marshall_primitive(Node *node, litmus *lit, StringBuf *sb,
		svector *symbol_table);
	friend void marshall_list(Node *node, litmus *lit, StringBuf *sb,
		svector *symbol_table);
	friend void marshall_composite(Node *node, litmus *lit, StringBuf *sb,
		svector *symbol_table);
	
	// From validate.cpp:
	friend int do_validate(Node *node, litmus *lit, svector *symbol_table);
	friend void validate_primitive(Node *node, litmus *lit,
		svector *symbol_table);
	friend void validate_list(Node *node, litmus *lit, svector *symbol_table);
	friend void validate_composite(Node *node, litmus *lit,
		svector *symbol_table);
	
	// From unmarshall.cpp:
	friend Node *do_unmarshall(const unsigned char *data, int bytes,
		litmus *lit, svector *symbol_table, int *consumed);
	*/

	/* Packing primitive types */

	static Node pack(int n, String opt_name)
	{
		// int, flg; assume int until converted by schema
		Node node = new Node(n);
		node.name = opt_name;
		return node;
	}
	static Node pack(int n)
	{ Node node = new Node(n); return node; }

	static Node pack(boolean bool, String opt_name)
	{
		// flg
		Node node = new Node(bool);
		node.name = opt_name;
		return node;
	}
	static Node pack(boolean bool)
	{ Node node = new Node(bool); return node; }

   static Node pack_empty(String opt_name)
	{ return create_empty(opt_name); }
   static Node pack_empty()
	{ return create_empty(); }
	
   static Node pack_value(String s, String opt_name)
	{ return create_value(s, opt_name); }
   static Node pack_value(String s)
	{ return create_value(s); }
	
	static Node pack(String s, String opt_name)
	{
		// txt, value; assume txt until converted by schema
		Node node = new Node(s);
		node.name = opt_name;
		return node;
	}
	static Node pack(String s)
	{ Node node = new Node(s); return node; }

	static Node pack(byte[] data, String opt_name)
	{
		// bin
		Node node = new Node(data);
		node.name = opt_name;
		return node;
	}
	static Node pack(byte[] data)
	{ Node node = new Node(data); return node; }

	static Node pack(double x, String opt_name)
	{
		// dbl
		Node node = new Node(x);
		node.name = opt_name;
		return node;
	}
	static Node pack(double x)
	{ Node node = new Node(x); return node; }

	static Node pack(Datetime time, String opt_name)
	{
		// datetime
		Node node = new Node(time);
		node.name = opt_name;
		return node;
	}
	static Node pack(Datetime time)
	{ Node node = new Node(time); return node; }

	static Node pack(Location loc, String opt_name)
	{
		// location
		Node node = new Node(loc);
		node.name = opt_name;
		return node;
	}
	static Node pack(Location loc)
	{ Node node = new Node(loc); return node; }
	
	/* Packing composite types (struct, lists) */

	static Node mklist(String opt_name)
	{
		// Assume struct until possibly converted to list type by schema
		Node node = new Node();
		node.type = NodeType.Struct;
		node.name = opt_name;
		node.children = new Vector<Node>();
		return node;
	}
	static Node mklist()
	{ return mklist(null); }

	Node append(Node n)
	{
		if(children == null)
			Log.error("Internal error: tried to append to non-list.");
		children.add(n);
		return this;
	}

	static Node pack(Node[] array, String opt_name)
	{
		// Assume struct, will probably be converted to list type by schema
		Node node = new Node();
		node.type = NodeType.Struct;
		node.name = opt_name;
		node.children = new Vector<Node>();
		for(int i = 0; i < array.length; i++)
			node.children.add(array[i]);
		return node;
	}
	static Node pack(Node[] array)
	{ return pack(array, null); }

	static Node pack(Node n1, Node n2, String opt_name)
	{
		// Assume struct until possibly converted to list type by schema
		Node node = new Node();
		node.type = NodeType.Struct;
		node.name = opt_name;
		node.children = new Vector<Node>();
		node.children.add(n1);
		node.children.add(n2);
		return node;
	}
	static Node pack(Node n1, Node n2)
	{ return pack(n1, n2, (String)null); }

	static Node pack(Node n1, Node n2, Node n3, String opt_name)
	{
		// Assume struct until possibly converted to list type by schema
		Node node = new Node();
		node.type = NodeType.Struct;
		node.name = opt_name;
		node.children = new Vector<Node>();
		node.children.add(n1);
		node.children.add(n2);
		node.children.add(n3);
		return node;
	}
	static Node pack(Node n1, Node n2, Node n3)
	{ return pack(n1, n2, n3, (String)null); }

	static Node pack(Node n1, Node n2, Node n3, Node n4, String opt_name)
	{
		// Assume struct until possibly converted to list type by schema
		Node node = new Node();
		node.type = NodeType.Struct;
		node.name = opt_name;
		node.children = new Vector<Node>();
		node.children.add(n1);
		node.children.add(n2);
		node.children.add(n3);
		node.children.add(n4);
		return node;
	}
	static Node pack(Node n1, Node n2, Node n3, Node n4)
	{ return pack(n1, n2, n3, n4, (String)null); }

	static Node pack(Node n1, Node n2, Node n3, Node n4, Node n5,
			String opt_name)
	{
		// Assume struct until possibly converted to list type by schema
		Node node = new Node();
		node.type = NodeType.Struct;
		node.name = opt_name;
		node.children = new Vector<Node>();
		node.children.add(n1);
		node.children.add(n2);
		node.children.add(n3);
		node.children.add(n4);
		node.children.add(n5);
		return node;
	}
	static Node pack(Node n1, Node n2, Node n3, Node n4, Node n5)
	{ return pack(n1, n2, n3, n4, n5, (String)null); }

	static Node pack(Node n1, Node n2, Node n3, Node n4, Node n5, Node n6,
			String opt_name)
	{
		// Assume struct until possibly converted to list type by schema
		Node node = new Node();
		node.type = NodeType.Struct;
		node.name = opt_name;
		node.children = new Vector<Node>();
		node.children.add(n1);
		node.children.add(n2);
		node.children.add(n3);
		node.children.add(n4);
		node.children.add(n5);
		node.children.add(n6);
		return node;
	}
	static Node pack(Node n1, Node n2, Node n3, Node n4, Node n5, Node n6)
	{ return pack(n1, n2, n3, n4, n5, n6, (String)null); }
};
