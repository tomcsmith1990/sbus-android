// net.cpp - DMI - 14-4-2007

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "error.h"
#include "datatype.h"
#include "dimension.h"
#include "multiplex.h"
#include "builder.h"
#include "hash.h"
#include "component.h"
#include "lowlevel.h"
#include "net.h"

const char *acceptance_code[] =
{
	// Must match AcceptanceCode in net.h
	"AcceptOK",
	"AcceptWrongCpt",
	"AcceptNoEndpoint",
	"AcceptWrongType",
	"AcceptNotCompatible",
	"AcceptNoAccess",
	"AcceptProtocolError",
	"AcceptAlreadyMapped"
};

const int default_rdc_port = 50123;

typedef unsigned char uchar;

const int header_length = 10;

int decode_count(unsigned char **pos)
{
	int n;
	unsigned char *s;
	
	s = (unsigned char *)*pos;
	if(*s == 255)
	{
		n = (s[1] << 24) | (s[2] << 16) | (s[3] << 8) | s[4];
		*pos += 5;
	}
	else if(*s == 254)
	{
		n = (s[1] << 8) | s[2];
		*pos += 3;
	}
	else
	{
		n = s[0];
		*pos += 1;
	}
	return n;
}

char *decode_string(unsigned char **pos)
{
	int len;
	char *str;

	len = decode_count(pos);
	if(len == 0)
		return NULL; // The empty string is converted back to NULL
	str = new char[len + 1];
	memcpy(str, *pos, len);
	*pos += len;
	str[len] = '\0';
	return str;
}

int decode_byte(unsigned char **pos)
{
	char c = **pos;
	*pos += 1;
	return c;
}

HashCode *decode_hashcode(unsigned char **pos)
{
	HashCode *hc;
	
	hc = new HashCode();	
	hc->frombinary(*pos);
	*pos += 6;
	return hc;
}

StringBuf *begin_msg(MessageType type)
{
	StringBuf *sb;
	
	sb = new StringBuf();
	sb->cat("sBuS"); // Magic string
	sb->cat_byte(PROTOCOL_VERSION);
	sb->skip(4); // Placeholder for message length
	sb->cat_byte(type);	
	return sb;
}

// Returns message length, or -1 on disconnect or -2 on protocol error:
int read_header(int sock, MessageType *type)
{
	unsigned char *buf;
	int msg_length;
	
	buf = new uchar[header_length];
	if(fixed_read(sock, buf, 10) < 0)
		return -1;
	
	if(buf[0] != 's' || buf[1] != 'B' || buf[2] != 'u' || buf[3] != 'S')
	{
		// "Incorrect magic number"
		delete[] buf;
		return -2;
	}
	if(buf[4] != PROTOCOL_VERSION)
	{
		// "Incorrect protocol version"
		delete[] buf;
		return -2;
	}
	msg_length = (buf[5] << 24) | (buf[6] << 16) | (buf[7] << 8) | buf[8];
	*type = (MessageType)buf[9];
	delete[] buf;
	
	return msg_length;
}

/* AbstractMessage */

AbstractMessage::~AbstractMessage()
{
	if(header != NULL) delete[] header;
	if(body != NULL) delete[] body;
	if(address != NULL) delete[] address;
};

AbstractMessage::AbstractMessage(int fd)
{
	// Inbound
	this->fd = fd;
	header = new uchar[10];
	body = NULL;
	state = StateHeader;
	filled = 0;
	outbound = 0;
	diverting = 0;
	address = NULL;
	purpose = NonVisit;
};

AbstractMessage::AbstractMessage(int fd, StringBuf *sb)
{
	// Outbound
	this->fd = fd;
	header = NULL;
	length = sb->length();
	sb->overwrite_word(5, length); // Patch in message length
	body = (unsigned char *)sb->extract();
	state = StateBody;
	filled = 0;
	outbound = 1;
	diverting = 0;
	address = NULL;
	purpose = NonVisit;
};

int AbstractMessage::ready(int timeout_usec) // -1 on timeout, else 0
{
	fd_set fdset;
	struct timeval tv;
	int ret;
	
	tv.tv_sec = 0;
	tv.tv_usec = timeout_usec;
	
	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);
	if(outbound)
		ret = select(fd + 1, NULL, &fdset, NULL, timeout_usec == 0 ? NULL : &tv);
	else
		ret = select(fd + 1, &fdset, NULL, NULL, timeout_usec == 0 ? NULL : &tv);
	if(ret < 0)
		error("select() error in ready()");
	return ((ret == 0) ? -1 : 0);
}

int AbstractMessage::blockadvance()
{
	// Returns 1 if complete, -1 on disconnect or -2 on protocol error
	return do_advance(1, 0);
}
		
int AbstractMessage::blockadvance(int timeout_usec)
{
	/* Returns 1 if complete, -1 on disconnect, -2 on protocol error,
		or 0 on timeout */
	return do_advance(1, timeout_usec);
}
		
int AbstractMessage::advance()
{
	/* Returns 1 if complete, 0 if incomplete, -1 on disconnect
		or -2 on protocol error (the latter only possible for inbound data) */
	return do_advance(0, 0);
}

int AbstractMessage::partial_advance()
{
	return do_advance(0, 0, 4); // Just write a few (4) bytes
}
	
int AbstractMessage::do_advance(int block, int timeout_usec, int max_bytes)
{
	int bytes;
	int ret;
	int target;
		
	if(state == StateDone) error("Tried to advance a completed message");
	if(outbound)
	{
		while(filled < length && (max_bytes == -1 || filled < max_bytes))
		{
			target = length - filled;
			if(max_bytes != -1 && target > max_bytes) target = max_bytes;
			bytes = write(fd, body + filled, target);
			if(bytes < 0)
			{
				if(errno == EAGAIN)
				{
					if(block)
					{
						ret = ready(timeout_usec);
						if(ret < 0) return 0;
						continue;
					}
					else
						return 0;
				}
				else
				{
					log("Outbound message error = %s, fd = %d",
							strerror(errno), fd);
					return -1;
				}
			}
			filled += bytes;
		}
		if(filled == length)
			state = StateDone;
	}
	else
	{
		// Inbound
		if(state == StateHeader)
		{
			while(filled < header_length)
			{
				bytes = read(fd, header + filled, header_length - filled);
				if(bytes == 0)
				{
					// printf("DBG> fd %d disconnected in header\n", fd);
					return -1; // EOF - disconnected
				}
				if(bytes < 0)
				{
					if(errno == EAGAIN)
					{
						if(block)
						{
							ret = ready(timeout_usec);
							if(ret < 0) return 0;
							continue;
						}
						else
							return 0;
					}
					else
					{
						log("Inbound message error = %s, fd = %d",
								strerror(errno), fd);
						return -1;
					}
				}
				filled += bytes;
			}

			// Parse header:
			if(header[0] != 's' || header[1] != 'B' || header[2] != 'u' ||
					header[3] != 'S')
				return -2; // Incorrect magic number
			if(header[4] != PROTOCOL_VERSION)
				return -2; // Incorrect protocol version
			length = (header[5] << 24) | (header[6] << 16) | (header[7] << 8) |
					header[8];
			type = (MessageType)header[9];
			state = StateBody;
			body = new uchar[length];
			filled = 0;
		}
		// StateBody
		while(filled < length - header_length)
		{

			bytes = read(fd, body + filled, length - header_length - filled);
			if(bytes == 0)
			{
				// printf("DBG> disconnected in body\n");
				return -1; // EOF - disconnected
			}
			if(bytes < 0)
			{
				if(errno == EAGAIN)
				{
					if(block)
					{
						ret = ready(timeout_usec);
						if(ret < 0) return 0;
						continue;
					}
					else
						return 0;
				}
				else
				{
					log("Inbound message error = %s, fd = %d",
							strerror(errno), fd);
					return -1;
				}
			}
			filled += bytes;
		}
		state = StateDone;
	}
	return 1;
}
	
unsigned char *AbstractMessage::get_data()
{
	return body;
}

int AbstractMessage::get_length()
{
	return (length - header_length);
}

MessageType AbstractMessage::get_type()
{
	return type;
}

/* sproto */

int sproto::read(int sock) // -1 on disconnect, -2 on bad protocol, else 0
{
	AbstractMessage *abst;
	int ret;
	
	abst = new AbstractMessage(sock);

	ret = abst->blockadvance();
	if(ret < 0) // Disconnect (-1) or protocol error (-2)
	{
		printf("error msg %d\n",ret);
		delete abst;
		return ret;
	}
	ret = reveal(abst);
	delete abst;
	if(ret < 0) // Wrong msg type (-1)
		return -2;
	return 0;
}

int sproto::write(int sock) // -1 if disconnected, else 0
{
	AbstractMessage *abst;
	int ret;
	
	abst = wrap(sock);
	ret = abst->blockadvance();
	if(ret == 1) // Completed OK
		return 0;
	return ret;
}

/* smessage */

smessage::smessage()
{
	source_cpt = source_inst = source_ep = NULL;
	topic = NULL;
	source_ep_id = 0;
	seq = 0;
	hc = NULL;
	xml = NULL;
	tree = NULL;
	reason = 0;
	oob_returncode = NULL;
}

smessage::~smessage()
{
	if(source_cpt != NULL) delete[] source_cpt;
	if(source_inst != NULL) delete[] source_inst;
	if(source_ep != NULL) delete[] source_ep;
	if(topic != NULL) delete[] topic;
	if(hc != NULL) delete hc;
	if(xml != NULL) delete[] xml;
	if(tree) delete tree;
	if(oob_returncode != NULL) delete oob_returncode;
}

int smessage::reveal(AbstractMessage *abst)
{
	unsigned char *buf, *pos;
	int len;
	
	type = abst->get_type();
	if(type != MessageRcv && type != MessageResponse &&
			type != MessageUnavailable && type != MessageReturnCode)
	{
		return -1;
	}
	
	buf = abst->get_data();
	pos = buf;
	if(type == MessageUnavailable)
	{
		reason = decode_byte(&pos);
	}
	else if(type == MessageReturnCode)
	{
		oob_returncode = new sreturncode();
		oob_returncode->retcode = decode_count(&pos);
		oob_returncode->address = decode_string(&pos);
	}
	else
	{
		source_cpt = decode_string(&pos);
		source_inst = decode_string(&pos);
		source_ep = decode_string(&pos);
		topic = decode_string(&pos);
		source_ep_id = decode_count(&pos);
		seq = decode_count(&pos);
		hc = decode_hashcode(&pos);

		len = abst->get_length() - (pos - buf);
		xml = new char[len + 1];
		memcpy(xml, pos, len);
		pos += len;
		xml[len] = '\0';

		// Fill in tree:
		const char *err;
		if(tree != NULL)
			delete tree;
		tree = snode::import(xml, &err);
		if(tree == NULL)
		{
			// "Error converting payload from XML: %s", err
			return -2;
		}
	}
	
	return 0;
}

AbstractMessage *smessage::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	// Convert payload to XML:
	if(xml == NULL && tree != NULL)
		xml = tree->toxml(0);
		
	sb = begin_msg(type);
	if(type == MessageUnavailable)
	{
		sb->cat_byte(reason);
	}
	else
	{
		sb->cat_string(source_cpt);
		sb->cat_string(source_inst);
		sb->cat_string(source_ep);
		sb->cat_string(topic);
		sb->cat(source_ep_id);
		sb->cat(seq);
		sb->cat(hc);
		sb->cat(xml);
	}
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* sinternal */

sinternal::sinternal()
{
	topic = NULL;
	hc = NULL;
	xml = NULL;
	seq = 0;
	
	oob_ctrl = NULL;
}

sinternal::~sinternal()
{
	if(topic != NULL) delete[] topic;
	if(hc != NULL) delete hc;
	if(xml != NULL) delete[] xml;
	
	if(oob_ctrl != NULL) delete oob_ctrl;
}

int sinternal::reveal(AbstractMessage *abst)
{
	unsigned char *buf, *pos;
	type = abst->get_type();
	if(type != MessageReply && type != MessageEmit && type != MessageRPC
			&& type != MessageMap && type != MessageUnmap && type != MessageIsmap && type != MessageMapPolicy
			&& type != MessageSubscribe && type != MessagePrivilege && type !=  MessageLoadPrivileges)
		return -1;

	buf = abst->get_data();
	pos = buf;
	
	if(type == MessageSubscribe)
	{
		oob_ctrl = new scontrol();
		oob_ctrl->subs = decode_string(&pos);
		oob_ctrl->topic = decode_string(&pos);
		oob_ctrl->peer = decode_string(&pos);
	}
	else if(type == MessageMap || type == MessageUnmap || type == MessageIsmap || type == MessageMapPolicy)
	{
		oob_ctrl = new scontrol();
		oob_ctrl->address = decode_string(&pos);
		oob_ctrl->target_endpoint = decode_string(&pos);
		oob_ctrl->constraints = decode_string(&pos);
		oob_ctrl->sensor = decode_count(&pos);
		oob_ctrl->condition = decode_count(&pos);
		oob_ctrl->value = decode_count(&pos);
	}
	else if (type==MessagePrivilege){
		oob_ctrl = new scontrol();
		oob_ctrl->target_endpoint = decode_string(&pos);
		oob_ctrl->principal_cpt = decode_string(&pos);
		oob_ctrl->principal_inst = decode_string(&pos);
		oob_ctrl->adding_permission = decode_byte(&pos);
	}
	else if (type == MessageLoadPrivileges)
	{
		oob_ctrl = new scontrol();
		oob_ctrl->filename = decode_string(&pos);
	}
	else // MessageReply, MessageEmit, MessageRPC
	{
		// Normal data message from library to wrapper:
		int len;
				
		topic = decode_string(&pos);
		seq = decode_count(&pos);
		hc = decode_hashcode(&pos);

		len = abst->get_length() - (pos - buf);
		xml = new char[len + 1];
		memcpy(xml, pos, len);
		pos += len;
		xml[len] = '\0';
	}
	return 0;
}

AbstractMessage *sinternal::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	sb = begin_msg(type);
	if(type == MessageEmit)
		sb->cat_string(topic); // May be NULL
	else
		sb->cat(0); // Empty topic string
	sb->cat(seq);
	sb->cat(hc);
	sb->cat(xml);
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* shook */

shook::shook(MessageType t)
{
	hc = NULL;
	tree = NULL;
	type = t;
}

shook::shook()
{
	hc = NULL;
	tree = NULL;
	type = MessageUnknown;
}

shook::~shook()
{
	if(hc != NULL) delete hc;
	if(tree != NULL) delete tree;
}

int shook::reveal(AbstractMessage *abst)
{
	// Returns -1 if wrong msg type, else 0
	unsigned char *buf, *pos;
	char *xml;
	int len;

	type = abst->get_type();	
	if(type != MessageGetStatus && type != MessageGetSchema &&
			type != MessageDeclare)
		return -1;
	
	if(tree != NULL)
	{
		delete tree;
		tree = NULL;
	}
	
	buf = abst->get_data();
	pos = buf;
	hc = decode_hashcode(&pos);
	len = abst->get_length() - (pos - buf);
	if(len == 0)
		return 0; // tree remains NULL
	xml = new char[len + 1];
	memcpy(xml, pos, len);
	pos += len;
	xml[len] = '\0';
	
	// Fill in tree:
	const char *err;
	tree = snode::import(xml, &err);
	delete[] xml;
	if(tree == NULL)
	{
		// "Error converting payload from XML: %s", err
		return -2;
	}
	return 0;	
}

AbstractMessage *shook::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	char *xml;
	
	if(hc == NULL)
	{
		hc = new HashCode();
		hc->frommeta(SCHEMA_EMPTY);
	}
	if(tree != NULL)
		xml = tree->toxml(0);
	
	sb = begin_msg(type);
	sb->cat(hc);
	if(tree != NULL)
	{
		sb->cat(xml);
		delete[] xml;
	}
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* sgeneric */

sgeneric::sgeneric(MessageType t)
{
	hc = NULL;
	tree = NULL;
	type = t;
}

sgeneric::sgeneric()
{
	hc = NULL;
	tree = NULL;
	type = MessageUnknown;
}

sgeneric::~sgeneric()
{
	if(hc != NULL) delete hc;
	if(tree != NULL) delete tree;
}

int sgeneric::reveal(AbstractMessage *abst)
{
	// Returns -1 if wrong msg type, else 0
	unsigned char *buf, *pos;
	char *xml;
	int len;

	type = abst->get_type();	
	if(type != MessageStatus && type != MessageSchema && type != MessageHash)
		return -1;
	
	buf = abst->get_data();
	pos = buf;
	hc = decode_hashcode(&pos);
	len = abst->get_length() - (pos - buf);
	xml = new char[len + 1];
	memcpy(xml, pos, len);
	pos += len;
	xml[len] = '\0';
	
	// Fill in tree:
	const char *err;
	if(tree != NULL)
		delete tree;
	tree = snode::import(xml, &err);
	delete[] xml;
	if(tree == NULL)
	{
		// "Error converting payload from XML: %s", err
		return -2;
	}
	
	return 0;
}

AbstractMessage *sgeneric::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	char *xml;
	
	xml = tree->toxml(0);	
	sb = begin_msg(type);
	sb->cat(hc);
	sb->cat(xml);
	delete[] xml;
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* sstopwrapper */

int sstopwrapper::reveal(AbstractMessage *abst)
{
	// Returns -1 if wrong msg type, else 0
	unsigned char *pos;
	MessageType type;

	type = abst->get_type();	
	if(type != MessageStop)
		return -1;
	
	pos = abst->get_data();
	reason = decode_count(&pos);
	
	return 0;
}

AbstractMessage *sstopwrapper::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	sb = begin_msg(MessageStop);
	sb->cat(reason);
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* saddendpoint, sstopwrapper and shook: */

int read_bootupdate(AbstractMessage *abst, saddendpoint *add,
		sstopwrapper *stop, shook *hook, srdc *rdc)
{
	/* Returns:
		MessageAddEndpoint	= filled add,
		MessageStop			= filled stop,
		MessageGetStatus	= filled hook,
		MessageGetSchema	= filled hook,
		MessageDeclare		= filled hook,
		MessageRdc			= filled rdc,
		-2					= none of these message types matches */
	unsigned char *buf, *pos;
	MessageType t;

	t = abst->get_type();
	buf = abst->get_data();
	pos = buf;
	
	if(t == MessageAddEndpoint)
	{
		add->endpoint = decode_string(&pos);
		add->type = (EndpointType)decode_byte(&pos);
		add->msg_hc = decode_hashcode(&pos);
		add->reply_hc = decode_hashcode(&pos);
		add->flexible_matching = decode_byte(&pos);
	}
	else if(t == MessageStop)
	{
		stop->reason = decode_count(&pos);
	}
	else if(t == MessageGetStatus || t == MessageGetSchema ||
		t == MessageDeclare)
	{
		int len;
		char *xml;
		const char *err;
		
		hook->type = t;
		hook->hc = decode_hashcode(&pos);
		len = abst->get_length() - (pos - buf);
		if(len > 0)
		{
			xml = new char[len + 1];
			memcpy(xml, pos, len);
			pos += len;
			xml[len] = '\0';
			hook->tree = snode::import(xml, &err);
			delete[] xml;
			if(hook->tree == NULL)
				return -2;
		}
	}
	else if(t == MessageRdc)
	{
		rdc->address = decode_string(&pos);
		rdc->arrived = decode_byte(&pos);
		rdc->notify = decode_byte(&pos);
		rdc->autoconnect = decode_byte(&pos);
	}
	else
	{
		return -2;
	}
	
	return t;
}

int read_bootupdate(int sock, saddendpoint *add, sstopwrapper *stop,
		shook *hook, srdc *rdc)
{
	/* Returns:
		MessageAddEndpoint	= filled add,
		MessageStop			= filled stop,
		MessageGetStatus	= filled hook,
		MessageGetSchema	= filled hook,
		MessageDeclare		= filled hook,
		MessageRdc			= filled rdc,
		-1					= disconnect,
		-2					= bad protocol */
	unsigned char *buf, *pos;
	int msg_length;
	MessageType t;

	msg_length = read_header(sock, &t);
	if(msg_length < 0)
		return msg_length; // Error inside header
	
	if(msg_length > 10)
	{	
		buf = new uchar[msg_length - 10];
		if(fixed_read(sock, buf, msg_length - 10) < 0)
			return -1;
		pos = buf;
	}
	else
	{
		pos = buf = NULL;
	}

	if(t == MessageAddEndpoint)
	{
		add->endpoint = decode_string(&pos);
		add->type = (EndpointType)decode_byte(&pos);
		add->msg_hc = decode_hashcode(&pos);
		add->reply_hc = decode_hashcode(&pos);
		add->flexible_matching = decode_byte(&pos);
	}
	else if(t == MessageStop)
	{
		stop->reason = decode_count(&pos);
	}	
	else if(t == MessageGetStatus || t == MessageGetSchema ||
		t == MessageDeclare)
	{
		int len;
		char *xml;
		const char *err;
		
		hook->type = t;
		hook->hc = decode_hashcode(&pos);
		len = msg_length - 10 - (pos - buf);
		if(len > 0)
		{
			xml = new char[len + 1];
			memcpy(xml, pos, len);
			pos += len;
			xml[len] = '\0';
			hook->tree = snode::import(xml, &err);
			delete[] xml;
			if(hook->tree == NULL)
				return -2;
		}
	}
	else if(t == MessageRdc)
	{
		rdc->address = decode_string(&pos);
		rdc->arrived = decode_byte(&pos);
		rdc->notify = decode_byte(&pos);
		rdc->autoconnect = decode_byte(&pos);
	}
	else
	{
		if(buf != NULL)
			delete[] buf;
		return -2;
	}
	
	if(buf != NULL)
		delete[] buf;
	return t;
}

/* srunning */

srunning::srunning()
{
	builtins = NULL;
	address = NULL;
	xml = NULL;
}

srunning::~srunning()
{
	if(builtins != NULL) delete builtins;
	if(xml != NULL) delete[] xml;
	if(address != NULL) delete[] address;
}

int srunning::reveal(AbstractMessage *abst)
{
	// Returns -1 if wrong msg type, else 0
	unsigned char *buf, *pos;
	MessageType type;
	int len;

	type = abst->get_type();	
	if(type != MessageRunning)
		return -1;
	
	buf = abst->get_data();
	pos = buf;
	
	listen_port = decode_count(&pos);
	address = decode_string(&pos);
	
	len = abst->get_length() - (pos - buf);
	xml = new char[len + 1];
	memcpy(xml, pos, len);
	pos += len;
	xml[len] = '\0';
	
	// Fill in tree:
	const char *err;
	if(builtins != NULL)
		delete builtins;
	builtins = snode::import(xml, &err);
	if(builtins == NULL)
	{
		// "Error converting payload from XML: %s", err
		return -2;
	}
	
	return 0;
}

AbstractMessage *srunning::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	// Convert payload to XML:
	if(xml == NULL && builtins != NULL)
		xml = builtins->toxml(0);
	if(xml == NULL)
		error("Failed sanity check in srunning::wrap()");
		
	sb = begin_msg(MessageRunning);
	sb->cat(listen_port);
	sb->cat_string(address);
	sb->cat(xml);
	
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* saddendpoint and sstartwrapper */

saddendpoint::saddendpoint()
{
	endpoint = NULL;
	msg_hc = reply_hc = NULL;
	flexible_matching = 0;
}

void saddendpoint::clear()
{
	if(endpoint != NULL) delete[] endpoint;
	if(msg_hc != NULL) delete msg_hc;
	if(reply_hc != NULL) delete reply_hc;
	endpoint = NULL;
	msg_hc = reply_hc = NULL;
}

saddendpoint::~saddendpoint()
{
	if(endpoint != NULL) delete[] endpoint;
	if(msg_hc != NULL) delete msg_hc;
	if(reply_hc != NULL) delete reply_hc;
}

sstartwrapper::sstartwrapper()
{
	cpt_name = instance_name = creator = metadata_address = NULL;
	rdc = new svector();
	unique = UniqueMultiple;
	rdc_update_notify = false;
	rdc_update_autoconnect = false;
}

sstartwrapper::~sstartwrapper()
{
	if(cpt_name != NULL) delete[] cpt_name;
	if(instance_name != NULL) delete[] instance_name;
	if(creator != NULL) delete[] creator;
	if(metadata_address != NULL) delete[] metadata_address;
	if(rdc != NULL) delete rdc;
}

AbstractMessage *saddendpoint::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	sb = begin_msg(MessageAddEndpoint);
	sb->cat_string(endpoint);
	sb->cat_byte(type);
	sb->cat(msg_hc);
	sb->cat(reply_hc);
	sb->cat_byte(flexible_matching);
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

int saddendpoint::write(int sock)
{
	StringBuf *sb;
	int ret;
	
	sb = begin_msg(MessageAddEndpoint);
	sb->cat_string(endpoint);
	sb->cat_byte(type);
	sb->cat(msg_hc);
	sb->cat(reply_hc);
	sb->cat_byte(flexible_matching);

	// Patch in message length:
	sb->overwrite_word(5, sb->length());
	ret = fixed_write(sock, sb->peek(), sb->length());
	delete sb;
	return ret;
}

AbstractMessage *sstartwrapper::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	char *s;

	sb = begin_msg(MessageStart);
	sb->cat_string(cpt_name);
	sb->cat_string(instance_name);
	sb->cat_string(creator);
	sb->cat_string(metadata_address); // May be NULL
	sb->cat(listen_port);
	sb->cat_byte(unique);
	sb->cat_byte(log_level);
	sb->cat_byte(echo_level);
	
	// RDCs:
	sb->cat(rdc->count());
	for(int i = 0; i < rdc->count(); i++)
	{
		s = rdc->item(i);
		sb->cat_string(s);
	}

	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

int sstartwrapper::write(int sock)
{
	StringBuf *sb;
	int ret;
	char *s;
	
	sb = begin_msg(MessageStart);
	sb->cat_string(cpt_name);
	sb->cat_string(instance_name);
	sb->cat_string(creator);
	sb->cat_string(metadata_address); // May be NULL
	sb->cat(listen_port);
	sb->cat_byte(unique);
	sb->cat_byte(log_level);
	sb->cat_byte(echo_level);
	sb->cat_byte(rdc_register);
	sb->cat_byte(rdc_update_notify);
	sb->cat_byte(rdc_update_autoconnect);
	// RDCs:
	sb->cat(rdc->count());
	for(int i = 0; i < rdc->count(); i++)
	{
		s = rdc->item(i);
		sb->cat_string(s);
	}

	// Patch in message length:
	sb->overwrite_word(5, sb->length());
	ret = fixed_write(sock, sb->peek(), sb->length());
	delete sb;
	return ret;
}

int read_startup(AbstractMessage *abst, saddendpoint *add, sstartwrapper *start)
{
	// Returns 0 = filled add, 1 = filled start, -1 = neither type matches
	unsigned char *pos;
	MessageType t;

	t = abst->get_type();
	if(t != MessageAddEndpoint && t != MessageStart)
		return -1;

	pos = abst->get_data();	
	if(t == MessageAddEndpoint)
	{
		add->endpoint = decode_string(&pos);
		add->type = (EndpointType)decode_byte(&pos);
		add->msg_hc = decode_hashcode(&pos);
		add->reply_hc = decode_hashcode(&pos);
		add->flexible_matching = decode_byte(&pos);
		return 0;
	}
	else // MessageStart
	{
		int num_rdc;
		char *s;
		
		start->cpt_name = decode_string(&pos);
		start->instance_name = decode_string(&pos);
		start->creator = decode_string(&pos);
		start->metadata_address = decode_string(&pos);
		start->listen_port = decode_count(&pos);
		start->unique = (CptUniqueness)decode_byte(&pos);
		start->log_level = decode_byte(&pos);
		start->echo_level = decode_byte(&pos);
		start->rdc_register = decode_byte(&pos);
		start->rdc_update_notify = decode_byte(&pos);
		start->rdc_update_autoconnect = decode_byte(&pos);
		num_rdc = decode_count(&pos);
		start->rdc->clear();
		for(int i = 0; i < num_rdc; i++)
		{
			s = decode_string(&pos);
			start->rdc->add(s);
			delete s;
		}
		return 1;
	}	
	return -2; // Never happens
}

int read_startup(int sock, saddendpoint *add, sstartwrapper *start)
{
	/* Returns 0 = filled add, 1 = filled start, -1 = disconnect,
		-2 = bad protocol */
	unsigned char *buf, *pos;
	int msg_length;
	MessageType t;

	msg_length = read_header(sock, &t);
	if(msg_length < 0)
		return msg_length; // Error inside header
	
	if(t != MessageAddEndpoint && t != MessageStart)
		return -2;
	
	buf = new uchar[msg_length - 10];
	if(fixed_read(sock, buf, msg_length - 10) < 0)
		return -1;
	pos = buf;

	if(t == MessageAddEndpoint)
	{
		add->endpoint = decode_string(&pos);
		add->type = (EndpointType)decode_byte(&pos);
		add->msg_hc = decode_hashcode(&pos);
		add->reply_hc = decode_hashcode(&pos);
		add->flexible_matching = decode_byte(&pos);

		delete buf;
		return 0;
	}
	else // MessageStart
	{
		int num_rdc;
		char *s;
		
		start->cpt_name = decode_string(&pos);
		start->instance_name = decode_string(&pos);
		start->creator = decode_string(&pos);
		start->metadata_address = decode_string(&pos);
		start->listen_port = decode_count(&pos);
		start->unique = (CptUniqueness)decode_byte(&pos);
		start->log_level = decode_byte(&pos);
		start->echo_level = decode_byte(&pos);
		start->rdc_register = decode_byte(&pos);
		start->rdc_update_notify = decode_byte(&pos);
		start->rdc_update_autoconnect = decode_byte(&pos);
		num_rdc = decode_count(&pos);
		start->rdc->clear();
		for(int i = 0; i < num_rdc; i++)
		{
			s = decode_string(&pos);
			start->rdc->add(s);
			delete s;
		}

		delete buf;
		return 1;
	}	
	
	return -2; // Never happens
}

/* scontrol */

scontrol::scontrol()
{
	address = target_endpoint = constraints = NULL;
	subs = topic = peer = filename = NULL;
	principal_cpt = strdup(""); //because NULLs are transmitted as empty strings...
	principal_inst = strdup("");

}

scontrol::~scontrol()
{
	if(address != NULL) delete[] address;
	if(target_endpoint != NULL) delete[] target_endpoint;
	if(constraints != NULL) delete[] constraints;
	if(subs != NULL) delete[] subs;
	if(topic != NULL) delete[] topic;
	if(peer != NULL) delete[] peer;
	if (principal_cpt!=NULL) delete[] principal_cpt;
	if (principal_inst!=NULL) delete[] principal_inst;
	if (filename!=NULL) delete[] filename;
}

int scontrol::reveal(AbstractMessage *abst)
{
	unsigned char *pos;
	
	type = abst->get_type();
	if(type != MessageMap && type != MessageUnmap && type != MessageIsmap && type != MessageMapPolicy
			&& type != MessageSubscribe && type != MessagePrivilege && type != MessageLoadPrivileges)
	{
		return -1;
	}
	
	pos = abst->get_data();
	if(type == MessageSubscribe)
	{
		subs = decode_string(&pos);
		topic = decode_string(&pos);
		peer = decode_string(&pos);
	}
	else if (type == MessagePrivilege)
	{
		target_endpoint = decode_string(&pos);
		principal_cpt = decode_string(&pos);
		principal_inst = decode_string(&pos);
		adding_permission = decode_byte(&pos);
	}
	else if (type == MessageLoadPrivileges)
	{
		printf("got a message load privs\n");
		filename = decode_string(&pos);
	}
	else
	{
		address = decode_string(&pos);
		target_endpoint = decode_string(&pos);
		constraints = decode_string(&pos);
		sensor = decode_count(&pos);
		condition = decode_count(&pos);
		value = decode_count(&pos);
	}
	
	return 0;
}

AbstractMessage *scontrol::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	sb = begin_msg(type);
	if(type == MessageSubscribe)
	{
		sb->cat_string(subs); // May be NULL
		sb->cat_string(topic); // May be NULL
		sb->cat_string(peer); // May be NULL
	}
	else if (type == MessagePrivilege){
		sb->cat_string(target_endpoint);
		sb->cat_string(principal_cpt);
		sb->cat_string(principal_inst);
		sb->cat_byte(adding_permission);
	}
	else if (type == MessageLoadPrivileges)
		sb->cat_string(filename);
	else
	{
		sb->cat_string(address); // May be NULL
		sb->cat_string(target_endpoint); // May be NULL
		sb->cat_string(constraints); // May be NULL
		sb->cat(sensor);
		sb->cat(condition);
		sb->cat(value);
	}
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* srdc */

srdc::srdc()
{
	address = NULL;
	arrived = notify = autoconnect = -1;
}

srdc::~srdc()
{
	if(address != NULL) delete[] address;
}

int srdc::reveal(AbstractMessage *abst)
{
	unsigned char *pos;
	
	type = abst->get_type();
	if(type != MessageRdc)
	{
		return -1;
	}
	
	pos = abst->get_data();
	
	address = decode_string(&pos);
	arrived = decode_byte(&pos);
	notify = decode_byte(&pos);
	autoconnect = decode_byte(&pos);
	
	return 0;
}

AbstractMessage *srdc::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	sb = begin_msg(MessageRdc);
	sb->cat_string(address);
	sb->cat_byte(arrived);
	sb->cat_byte(notify);
	sb->cat_byte(autoconnect);
	
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* sreturncode */

sreturncode::sreturncode()
{
	address = NULL;
}

sreturncode::~sreturncode()
{
	if(address != NULL) delete[] address;
}

int sreturncode::reveal(AbstractMessage *abst)
{
	unsigned char *pos;
	
	if(abst->get_type() != MessageReturnCode)
		return -1;
	
	pos = abst->get_data();
	retcode = decode_count(&pos);
	address = decode_string(&pos);
	
	return 0;
}

AbstractMessage *sreturncode::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	sb = begin_msg(MessageReturnCode);
	sb->cat(retcode);
	sb->cat_string(address);
	
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* ipaddress */

char *ipaddress::check_add_port(const char *addr, int port)
{
	char *s;

	if(has_port(addr))
	{
		s = new char[strlen(addr) + 1];
		strcpy(s, addr);
	}
	else
	{
		const char *pos;
		
		s = new char[strlen(addr) + 15];
		pos = strchr(addr, ':');
		if(pos == NULL)
			sprintf(s, "%s:%d", addr, port);
		else
			sprintf(s, "%s%d", addr, port);
	}
	return s;	
}

int ipaddress::has_port(const char *addr)
{
	const char *pos = addr;
	while(*pos != '\0')
	{
		if(*pos == ':')
		{
			pos++;
			while(*pos != '\0')
			{
				if(*pos >= '0' && *pos <= '9')
					return 1;
				pos++;
			}
			return 0;
		}
		pos++;
	}
	return 0;
}
