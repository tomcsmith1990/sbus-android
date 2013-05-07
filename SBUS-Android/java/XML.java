import java.util.*;

enum TagType
{
	Opening, Closing, Empty, Body
};

class XMLTag
{
	XMLTag()
	{
		data = null;
	}
			
	void dump()
	{
		switch(type)
		{
			case Opening:
				System.out.print("<" + data + ">"); break;
			case Closing:
				System.out.print("</" + data + ">"); break;
			case Empty:
				System.out.print("<" + data + "/>"); break;
			case Body:
				System.out.print(data); break;
			default:
				Log.error("Unknown XML tag type in dump()");
		}
	}
				
	TagType type;
	String data; // Element name, or content if TagBody
};

class XML extends Node
{
	static void importerror(String msg) throws ImportException
	{
		throw new ImportException(msg);
	}

	/* XML export */
	
	static String toxml(Node node, boolean pretty)
	{
		StringConstructor sc;

		if(node.type == NodeType.Empty)
			return "0";
		sc = new StringConstructor();
		if(pretty)
			flatten(node, sc, 0);
		else
			flatten(node, sc, -1);
		return sc.extract();
	}
	
	private static void flatten(Node node, StringConstructor sc, int offset)
	{
		String name = node.name_string();
		boolean multiline;

		sc.indent(offset);
		if(node.type == NodeType.List && node.children.size() == 0)
		{
			sc.cat('<');
			sc.cat(name);
			sc.cat("/>");
			if(offset != -1)
				sc.cat('\n');
			return;
		}
		sc.cat('<');
		sc.cat(name);
		sc.cat('>');
		switch(node.type)
		{
			case Empty:
				sc.cat('-');
				break;
			case Int:
				sc.cat_int(node.n);
				break;
			case Double:
				sc.cat(node.x);
				break;
			case Text:
				multiline = string_is_multiline(node.s);
				if(multiline)
					sc.cat('\n');
				sc.cat(escape_string(node.s));
				if(multiline)
					sc.cat('\n');
				break;
			case Binary:
				sc.cat('\n');
				sc.indent(offset + 1);
				sc.cat("0x");
				for(int i = 0; i < node.data.length; i++)
				{
					if(((i % 16) == 15) && (i != node.data.length - 1))
					{
						sc.cat('\n');
						sc.indent(offset + 1);
						sc.cat("0x");
					}
					sc.cat_hexbyte(node.data[i]);
				}
				sc.cat('\n');
				sc.indent(offset);
				break;
			case Bool:
				if(node.n == 1)
					sc.cat("true");
				else if(node.n == 0)
					sc.cat("false");
				else
					Log.error("Invalid boolean value.");
				break;
			case Datetime:
				sc.cat(node.time.toString());
				break;
			case Location:
				sc.cat(node.loc.toString());
				break;
			case Struct:
			case List:
				if(offset != -1)
					sc.cat('\n');
				for(int i = 0; i < node.children.size(); i++)
				{
					if(offset == -1)
						flatten(node.children.get(i), sc, -1);
					else
						flatten(node.children.get(i), sc, offset + 1);
				}
				sc.indent(offset);
				break;
			case Value:
				sc.cat('#');
				if(node.s != null)
					sc.cat(node.s);
				else
					sc.cat_int(node.n);
				break;
			default:
				Log.error("Impossible node type.");
		}
		sc.cat("</");
		sc.cat(name);
		sc.cat('>');
		if(offset != -1)
			sc.cat('\n');
	}
	
	private static boolean string_needs_quotes(String s)
	{
		char c;
		
		if(s.length() == 0)
			return true;
		
		c = s.charAt(0);
		if(c == '-' || c == ' ' || c == '#')
			return true;
		if(c >= '0' && c <= '9')
			return true;
		if(s.charAt(s.length() - 1) == ' ')
			return true;
		if(s.equals("true") || s.equals("false"))
			return true;
		return false;
	}

	private static String escape_string(String s)
	{
		StringConstructor sc = new StringConstructor();
		String t;
		char c;
		boolean quotes;
		int len = s.length();

		quotes = string_needs_quotes(s);
		if(quotes)
			sc.cat('"');
		for(int i = 0; i < len; i++)
		{
			c = s.charAt(i);
			if((quotes || (i == 0)) && (c == '"'))
			{
				sc.cat('\\');
				sc.cat('"');
			}
			else if(!quotes && (c == '<'))
			{
				sc.cat('\\');
				sc.cat('<');
			}
			else if(c == '\\')
			{
				sc.cat('\\');
				sc.cat('\\');
			}
			else
				sc.cat(c);
		}
		if(quotes)
			sc.cat('"');
		return sc.extract();
	}

	/* XML import: throw exception e (explanation in e.msg) on syntax error */
	
	static Node fromxml(String s) throws ImportException
	{
		Vector<Node> multi;
		Node node;

		if(s.equals("0")) // Empty message
		{
			node = Node.create_empty();
			return node;
		}

		multi = do_import(s, true);
		node = multi.get(0);
		return node;
	}

	static Node fromxml_file(String path) throws ImportException
	{
		MemFile mf;
		Node data;

		mf = new MemFile(path);
		if(mf.data == null)
			throw new ImportException("Can't open file " + path);
		data = fromxml(new String(mf.data));
		return data;
	}

	static Node[] fromxml_multi(String s) throws ImportException
	{
		Vector<Node> multi;
		Node[] nodearray;

		multi = do_import(s, false);
		
		nodearray = new Node[multi.size()];
		for(int i = 0; i < multi.size(); i++)
			nodearray[i] = multi.get(i);
		return nodearray;
	}

	static Node[] fromxml_file_multi(String path) throws ImportException
	{
		MemFile mf;
		Node[] data;

		mf = new MemFile(path);
		if(mf.data == null)
			throw new ImportException("Can't open file " + path);
		data = fromxml_multi(new String(mf.data));
		return data;
	}

	private static Vector<Node> do_import(String s, boolean single) throws
			ImportException
	{
		Vector<Node> stack;
		Node node, container;
		Vector<Node> tops;
		Vector<XMLTag> list;
		XMLTag tag;
		String open_name; // Name of most recent open tag (not on stack yet)?
		int n;

		/* Note: in general XML, content could occur after an end or
			empty tag. In our version, content can only occur after
			an opening tag, since all primitive-type fields must be named
			and hence wrapped in a tag. */

		tops = new Vector<Node>();
		stack = new Vector<Node>();
		
		list = recognise_tags(s);
		open_name = null;
		n = 0;
		while(n < list.size())
		{
			tag = list.get(n);
			n++;
			switch(tag.type)
			{
				case Opening:
					if(open_name != null)
					{
						node = new Node();
						node.type = NodeType.Struct;
						// Assume struct, until/if converted to list by schema
						node.name = open_name;
						node.children = new Vector<Node>();
						if(stack.size() == 0)
						{
							if(single && tops.size() > 0)
							{
								importerror("Document has multiple top-level " +
										"tags, second is " + open_name);
							}
							tops.add(node);
						}
						else
						{
							container = stack.lastElement();
							container.children.add(node);
						}
						stack.add(node);
					}
					open_name = tag.data;
					break;
				case Closing:
					if(open_name != null)
					{
						importerror("End tag has different name to start <" +
								open_name + ">, and no content");
					}
					if(stack.size() == 0)
						importerror("End tag but no start");
					container = stack.lastElement();
					if(!container.name.equals(tag.data))
					{
						importerror("End tag <" + tag.data + "> has different" +
								" name to start <" + container.name + ">");
					}
					stack.remove(stack.size() - 1); // Pop
					open_name = null;
					break;
				case Body:
					if(open_name == null)
						importerror("XML data must be wrapped by a tag");
					node = infer_type(tag.data);
					node.name = open_name;
					open_name = null;
					if(stack.size() == 0)
					{
						if(single && tops.size() > 0)
							importerror("Document has multiple top-level tags");
						tops.add(node);
					}
					else
					{
						container = stack.lastElement();
						container.children.add(node);
					}
					// Read corresponding end tag:
					if(n >= list.size())
						importerror("Missing end tag");
					tag = list.get(n);
					n++;
					if(tag.type != TagType.Closing)
						importerror("Data must be followed by end tag");
					if(!node.name.equals(tag.data))
					{
						importerror("End tag <" + tag.data + "> has different" +
								" name to start <" + node.name + ">");
					}
					break;
				case Empty:
					if(open_name != null)
					{
						node = new Node();
						node.type = NodeType.Struct;
						// Assume struct, until/if converted to list by schema
						node.name = open_name;
						node.children = new Vector<Node>();
						if(stack.size() == 0)
						{
							if(single && tops.size() > 0)
							{
								importerror("Document has multiple top-level tags," +
										" second is <" + open_name + ">");
							}
							tops.add(node);
						}
						else
						{
							container = stack.lastElement();
							container.children.add(node);
						}
						stack.add(node);
					}
					node = new Node();
					node.type = NodeType.Struct;				
					node.name = tag.data;
					node.children = new Vector<Node>();
					open_name = null;
					if(stack.size() == 0)
					{
						if(single && tops.size() > 0)
							importerror("Document has multiple top-level tags");
						tops.add(node);
					}
					else
					{
						container = stack.lastElement();
						container.children.add(node);
					}
					break;
				default:
					Log.error("Unknown tag type");
			}
		}

		// Check stack of open tags is empty:
		if(open_name != null)
			importerror("Not all tags properly closed.");
		if(stack.size() != 0)
			importerror("Not all tags properly closed.");
		if(tops.size() == 0)
			importerror("XML document is empty");
		if(single && tops.size() != 1)
		{
			Log.error("Failed sanity check in do_import: multiple top-level " +
					"nodes in XML.");
		}
		return tops;
	}

	private static Vector<XMLTag> recognise_tags(String s) throws
			ImportException
	{
		Vector<XMLTag> list;
		XMLTag tag;
		StringScanner ss = new StringScanner(s);

		list = new Vector<XMLTag>();

		while(true)
		{
			ss.eat_whitespace();
			if(ss.eof())
				break; // End of document, mission complete
			tag = new XMLTag();
			if(ss.getchar() != '<')
			{
				tag.type = TagType.Body;
				tag.data = ss.read_tag_content();
			}
			else
			{
				ss.advance();
				ss.eat_whitespace();
				if(ss.getchar() == '/')
				{
					tag.type = TagType.Closing;
					ss.advance();
					ss.eat_whitespace();
				}
				else
				{
					tag.type = TagType.Opening;
					// Could also be TagType.Empty, but we don't know that yet
				}
				// Read tag name:
				tag.data = ss.read_tag_name();

				// End of tag: > or />
				if(ss.getchar() == '/')
				{
					if(tag.type == TagType.Closing)
					{
						importerror("Tag </" + tag.data +
								"/> is both empty and an end-tag");
					}
					tag.type = TagType.Empty;
					ss.advance();
					ss.eat_whitespace();
					if(ss.getchar() != '>')
					{
						importerror("Empty tag <" + tag.data +
								"/ missing closing bracket");
					}
				}
				ss.advance(); // Skip past '>'
				ss.eat_whitespace();
			}
			if(tag.type == TagType.Closing && list.size() > 0)
			{
				XMLTag prev = list.lastElement();

				if(prev.type == TagType.Opening && prev.data.equals(tag.data))
				{
					list.removeElementAt(list.size() - 1);
					tag.type = TagType.Empty;
				}
			}
			list.add(tag);
		}
		return list;
	}

	private static Node infer_type(String s)
	{
		/* Our input has already been stripped of whitespace, but may be quoted
			if it's a string */
		Node node;
		char c;

		if(s.length() == 0)
		{
			// Text
			node = new Node(s);
			return node;
		}
		c = s.charAt(0);
		if(s.equals("true"))
		{
			node = Node.create_bool(true);
		}
		else if(s.equals("false"))
		{
			node = Node.create_bool(false);
		}
		else if(s.equals("-"))
		{
			node = Node.create_empty();
		}
		else if(c == '"')
		{
			int len = s.length();
			if(s.charAt(len - 1) != '"')
				Log.error("Quoted string missing end quote.");
			node = new Node(s.substring(1, len - 1));
		}
		else if(c == '#')
		{
			node = Node.create_value(s.substring(1));
		}
		else if(c == '0' && s.length() >= 2 && s.charAt(1) == 'x')
		{
			StringScanner ss;
			ByteEncoder be;
			byte[] data;
			byte b;

			ss = new StringScanner(s);
			be = new ByteEncoder();
			ss.advance(2);
			ss.eat_whitespace();
			while(!ss.eof())
			{
				if(ss.getchar() == '\n')
				{
					ss.advance();
					ss.eat_whitespace();
					if(ss.getchar() == '0' && ss.lookahead(1) == 'x')
						ss.advance(2);
				}
				b = ss.read_hexbyte();
				if(b == -1)
					break;
				be.cat(b);
			}
			data = be.extract();
			node = new Node(data);
		}
		else if(c != '-' && (c < '0' || c > '9'))
		{
			// Text
			node = new Node(s);
		}
		else if(StringScanner.is_int(s))
		{
			int n;

			n = Integer.valueOf(s);
			node = new Node(n);
		}
		else if(StringScanner.is_dbl(s))
		{
			double d;

			d = Double.valueOf(s);
			node = new Node(d);
		}
		else if(Datetime.typematch(s))
		{
			// clk
			Datetime time;

			time = new Datetime(s);
			node = new Node(time);
		}
		else if(Location.typematch(s))
		{
			// loc
			Location loc;

			loc = new Location(s);
			node = new Node(loc);
		}
		else
		{
			// Text
			node = new Node(s);
		}
		return node;
	}
};
