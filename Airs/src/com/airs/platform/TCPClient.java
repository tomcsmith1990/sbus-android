/*
Copyright (C) 2004-2006 Nokia Corporation
Copyright (C) 2008-2011, Dirk Trossen, airs@dirk-trossen.de

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation as version 2.1 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
 */
package com.airs.platform;

import java.io.OutputStream;
import java.io.InputStream;
import java.net.Socket;

public class TCPClient
{
	private Socket socket = null;
	private OutputStream out = null;
	private InputStream in = null;
	public boolean 	 connected=false;
	public String IMEI=null;

	protected void sleep(long millis) 
	{
		try 
		{
			Thread.sleep(millis);
		} 
		catch (InterruptedException ignore) 
		{
		}
	}

	protected static void debug(String msg) 
	{
		System.out.println(msg);
	}

	public boolean startTCP(Socket sock)
	{
		try
		{
			// now connect to given IP address
			connect(sock);

			byte[] IMEI_bytes = new byte[15];
			// read IMEI after connecting
			readBytes(IMEI_bytes);
			IMEI = new String(IMEI_bytes);
		}
		catch (Exception ignored)
		{
			debug("TCPClient: something went wrong with connect()");
			return false;
		}	
		return true;
	}


	public void connect(Socket sock)
	{		
		try {
			socket = sock;
			// set TCP keep alives
			socket.setKeepAlive(true);
			debug("TCPClient::connect:Socket Connected");

			// Open stream for sending & receiving 
			out = socket.getOutputStream();
			in = socket.getInputStream();

			connected=true;
		} catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	/** 
	 * disconnecting from application server with eventually restarting Imlet
	 */
	public synchronized void disconnect()
	{
		try
		{
			if (out!=null)
				out.close();
			if (in!=null)
				in.close();
			if (socket!=null)
			{
				socket.shutdownInput();
				socket.shutdownOutput();
			}

			connected=false;
			in   = null;
			out  = null;
			socket = null;

			debug("TCPClient::disconnect:closed all resources -> you need to restart now");
		}
		catch (Exception e) 
		{
			e.printStackTrace();
		} 
	}

	/** 
	 * writes Method to TCP connection
	 */
	public synchronized boolean write(Method method)
	{
		int length=0;

		// is there is no output stream, let's leave right away
		if (out==null)
			return false;

		// first count the bytes!
		try
		{
			length += 2;						// method type
			length += 2;						// FROM.length
			length += method.FROM.length;		// FROM.string
			length += 2;						// TO.length
			length += method.TO.length;			// TO.string
			length += 2;						// event_name.length
			length += method.event_name.length;	// event_name.string
			switch(method.method_type)
			{
			case method_type.method_SUBSCRIBE:
				length += 2 + 2 + 4;
				break;
			case method_type.method_NOTIFY:
				length += 2 + 2;
				break;
			case method_type.method_PUBLISH:
				length += 4 + 4;
				break;
			case method_type.method_CONFIRM:
				length += 2;
				switch(method.conf.confirm_type)
				{
				case confirm_type.confirm_OTHERS:
					length += 2 + 2;
					break;
				case confirm_type.confirm_PUBLISH:
					length += 4;
				}
				length += 4;		// lifetime
				length += 2;		// ret_code.length
				length += method.conf.ret_code.length;	// ret_code.string
				break;
			case method_type.method_BYE:
				length += 2 + 2 + 2;
				length += method.BYE.reason.length;
				break;
			}

			length += 4;						// event_body.length
			length += method.event_body.length; // event_body.string
		}
		catch (Exception e) 
		{
			e.printStackTrace();
		} 

		// create output stream
		try
		{
			// write length of method first
			writeInt(length);
			// count bytes transferred
			//airs.bytes_sent += length;

			// write method type first
			writeShort(method.method_type);
			// then the FROM octetstring
			writeShort(method.FROM.length);
			if (method.FROM.length>0)
				writeBytes(method.FROM.string);
			//then the TO octetstring
			writeShort(method.TO.length);
			if (method.TO.length>0)
				writeBytes(method.TO.string);
			//then the event_name octetstring
			writeShort(method.event_name.length);
			if (method.event_name.length>0)
				writeBytes(method.event_name.string);
			// now the method-specific fields
			switch(method.method_type)
			{
			case method_type.method_SUBSCRIBE:
				// dialog_id and CSeq and lifetime
				writeShort(method.sub.dialog_id);
				writeShort(method.sub.CSeq);
				writeInt(method.sub.Expires);
				break;
			case method_type.method_NOTIFY:
				// only dialog_id and CSeq
				writeShort(method.not.dialog_id);
				writeShort(method.not.CSeq);
				break;
			case method_type.method_PUBLISH:
				// e_tag and lifetime
				writeInt(method.pub.e_tag);
				writeInt(method.pub.Expires);
				break;
			case method_type.method_CONFIRM:
				// depending on confirmation type
				writeShort(method.conf.confirm_type);
				// differentiate confirmation type
				switch(method.conf.confirm_type)
				{
				// first for the other confirmation than PUBLISH
				case confirm_type.confirm_OTHERS:
					writeShort(method.conf.dialog.dialog_id);
					writeShort(method.conf.dialog.CSeq);
					break;
					// then for PUBLISH
				case confirm_type.confirm_PUBLISH:
					writeInt(method.conf.e_tag);
					break;
				}
				// lifetime
				writeInt(method.conf.Expires);
				// return code
				writeShort(method.conf.ret_code.length);
				if (method.conf.ret_code.length !=0)
					writeBytes(method.conf.ret_code.string);
				break;
			case method_type.method_BYE:
				// dialog_id and CSeq and reason
				writeShort(method.BYE.dialog_id);
				writeShort(method.BYE.CSeq);
				writeShort(method.BYE.reason.length);
				if (method.BYE.reason.length !=0)
					writeBytes(method.BYE.reason.string);
				break;
			}
			// add the length of the body
			writeInt(method.event_body.length);
			// write body only if length greater zero
			if (method.event_body.length !=0)
				writeBytes(method.event_body.string);
			// now flush the stream to send it definitely
			out.flush();
		}
		catch (Exception e) 
		{
			e.printStackTrace();
		} 
		return true;
	}

	/** 
	 * reads Method from TCP connection
	 */
	public Method read()
	{
		Method method=null;
		try
		{
			// is there is no output stream, let's leave right away
			if (in==null)
				throw new Exception("no Input stream available");

			// get memory to read into
			method = new Method();

			// read length of message.
			readInt();

			// read method type
			method.method_type				= readShort();
			// read FROM octetstring
			method.FROM.length				= readShort();
			method.FROM.string				= new byte[method.FROM.length];
			if (readBytes(method.FROM.string)	!=	method.FROM.length)
				throw new Exception("wrong length from FROM");
			// read TO octetstring
			method.TO.length				= readShort();
			method.TO.string				= new byte[method.TO.length];
			if (readBytes(method.TO.string)	!=	method.TO.length)
				throw new Exception("wrong length from TO");
			// read event_name octetstring
			method.event_name.length		= readShort();
			// is there an event name?
			if (method.event_name.length!=0)
			{
				method.event_name.string		= new byte[method.event_name.length];
				if (readBytes(method.event_name.string)	!=	method.event_name.length)
					throw new Exception("wrong length from event_name");
			}
			else
				method.event_name.string = null;

			switch(method.method_type)
			{
			case method_type.method_SUBSCRIBE:
				// dialog_id and CSeq and lifetime
				method.sub.dialog_id		= readShort();
				method.sub.CSeq				= readShort();
				method.sub.Expires			= readInt();
				break;
			case method_type.method_NOTIFY:
				// only dialog_id and CSeq
				method.not.dialog_id		= readShort();
				method.not.CSeq				= readShort();
				break;
			case method_type.method_PUBLISH:
				// e_tag and lifetime
				method.pub.e_tag 			= readInt();
				method.pub.Expires 			= readInt();
				break;
			case method_type.method_CONFIRM:
				// depending on confirmation type
				method.conf.confirm_type	= readShort();
				// differentiate confirmation type
				switch(method.conf.confirm_type)
				{
				// first for the other confirmation than PUBLISH
				case confirm_type.confirm_OTHERS:
					method.conf.dialog.dialog_id	= readShort();
					method.conf.dialog.CSeq			= readShort();
					break;
					// then for PUBLISH
				case confirm_type.confirm_PUBLISH:
					method.conf.e_tag				= readInt();
					break;
				}
				// lifetime
				method.conf.Expires			= readInt();
				// return code
				method.conf.ret_code.length = readShort();
				method.conf.ret_code.string = new byte[method.conf.ret_code.length];
				if (readBytes(method.conf.ret_code.string)	!=	method.conf.ret_code.length)
					throw new Exception("wrong length from conf.ret_code");
				break;
			case method_type.method_BYE:
				// dialog_id and CSeq and reason
				method.BYE.dialog_id		= readShort();
				method.BYE.CSeq				= readShort();
				// reason code
				method.BYE.reason.length 	= readShort();
				method.BYE.reason.string 	= new byte[method.BYE.reason.length];
				if (readBytes(method.BYE.reason.string)	!=	method.BYE.reason.length)
					throw new Exception("wrong length from BYE.reason");
				break;
			}

			// read event_body octetstring
			method.event_body.length		= readInt();
			// anything to read?
			if (method.event_body.length != 0)
			{
				method.event_body.string		= new byte[method.event_body.length];
				if (readBytes(method.event_body.string)	!=	method.event_body.length)
					throw new Exception("wrong length from event_body");
			}
			else
				method.event_body.string = null;

			debug("TCPClient::read:read new method");
			return method;
		}
		catch (Exception exception) 
		{
			exception.printStackTrace();
		}

		return null;
	}

	// implement the typed write() and read() functions myself since they didn't work properly
	private void writeBytes(byte value[])
	{
		try
		{
			out.write(value);
		}
		catch (Exception e) 
		{
			e.printStackTrace();
		}
	}

	private void writeShort(short value)
	{
		byte sent[] = new byte[2];

		sent[0] = (byte)((value>>8) & 0xff);
		sent[1] = (byte)(value & 0xff);
		try
		{
			out.write(sent);
		}
		catch (Exception e) 
		{
			e.printStackTrace();
		}
	}

	private void writeInt(int value)
	{
		byte sent[] = new byte[4];

		sent[0] = (byte)((value>>24) & 0xff);
		sent[1] = (byte)((value>>16) & 0xff);
		sent[2] = (byte)((value>>8)  & 0xff);
		sent[3] = (byte)(value & 0xff);
		try
		{
			out.write(sent);
		}
		catch (Exception e) 
		{
			e.printStackTrace();
		}
	}

	private int readBytes(byte[] value)
	{
		try
		{
			int read = 0;
			while (read < value.length)
				read += in.read(value, read, value.length - read);

			return read;
		}
		catch (Exception e) 
		{
			e.printStackTrace();
			return 0;
		}
	}

	private short readShort()
	{
		byte received[] = new byte[2];

		try
		{
			readBytes(received);
		}
		catch (Exception e) 
		{
			e.printStackTrace();
		}
		return (short)((received[0]<<8)|(received[1] & 0xff ));
	}

	private int readInt()
	{
		byte received[] = new byte[4];

		try
		{
			readBytes(received);
		}
		catch (Exception e) 
		{
			e.printStackTrace();
		}
		return (int)((received[0]<<24)|(received[1]<<16)|(received[2]<<8)|(received[3] & 0xff ));
	}
}
