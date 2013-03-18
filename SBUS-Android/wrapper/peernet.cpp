// peernet.cpp - DMI - 9-5-2009

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include "../compat/ifaddrs.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "../library/error.h"
#include "../library/datatype.h"
#include "../library/dimension.h"
#include "../library/multiplex.h"
#include "../library/builder.h"
#include "../library/hash.h"
#include "../library/component.h"
#include "../library/lowlevel.h"
#include "../library/net.h"
#include "peernet.h"

typedef unsigned char uchar;

/* scomm */

scomm::scomm()
{
	source = target = src_endpoint = tgt_endpoint = NULL;
	topic = NULL;
	hc = NULL;
	data = NULL;
	oob_goodbye = NULL; oob_divert = NULL;
	seq = 666; // Uninitialised
	terminate_disposable = false;
}

scomm::scomm(svisitor *visitor)
{
	source = sdup(visitor->src_cpt);
	src_endpoint = new char[20];
	sprintf(src_endpoint, "visitor%d", visitor->dispose_id);
	target = sdup(visitor->tgt_cpt);
	tgt_endpoint = sdup(visitor->required_endpoint);
	topic = NULL; // Visitor messages don't have topics
	seq = 0;
	
	if(visitor->ep_type == EndpointServer) type = MessageServer;
	else if(visitor->ep_type == EndpointSink) type = MessageSink;
	else error("Unknown visitor type '%s'", endpoint_type[visitor->ep_type]);
	
	hc = new HashCode(visitor->msg_hc);
	
	length = visitor->length;
	data = new uchar[length];
	memcpy(data, visitor->data, length);

	oob_goodbye = NULL;
	oob_divert = NULL;
	terminate_disposable = false;
}

scomm::~scomm()
{
	if(source != NULL) delete[] source;
	if(target != NULL) delete[] target;
	if(src_endpoint != NULL) delete[] src_endpoint;
	if(tgt_endpoint != NULL) delete[] tgt_endpoint;
	if(topic != NULL) delete[] topic;

	if(hc != NULL) delete hc;
	if(data != NULL) delete[] data;
	
	if(oob_goodbye != NULL) delete oob_goodbye;
	if(oob_divert != NULL) delete oob_divert;
}

int scomm::reveal(AbstractMessage *abst)
{
	// Might read sgoodbye, sresub or sdivert instead
	unsigned char *buf, *pos;
	
	type = abst->get_type();
	
	if(type != MessageSink && type != MessageServer &&
			type != MessageClient &&
			type != MessageGoodbye && type != MessageResubscribe &&
			type != MessageDivert)
	{
		return -1;
	}
	
	buf = abst->get_data();
	pos = buf;
	
	if(type == MessageGoodbye)
	{
		oob_goodbye = new sgoodbye();
		oob_goodbye->src_cpt = decode_string(&pos);
		oob_goodbye->src_ep  = decode_string(&pos);
		oob_goodbye->tgt_cpt = decode_string(&pos);
		oob_goodbye->tgt_ep  = decode_string(&pos);
	}
	else if(type == MessageResubscribe)
	{
		oob_resub = new sresub();
		oob_resub->src_cpt = decode_string(&pos);
		oob_resub->src_ep  = decode_string(&pos);
		oob_resub->tgt_cpt = decode_string(&pos);
		oob_resub->tgt_ep  = decode_string(&pos);
		oob_resub->subscription = decode_string(&pos);
		oob_resub->topic   = decode_string(&pos);
	}
	else if(type == MessageDivert)
	{
		oob_divert = new sdivert();
		oob_divert->src_cpt = decode_string(&pos);
		oob_divert->src_ep  = decode_string(&pos);
		oob_divert->tgt_cpt = decode_string(&pos);
		oob_divert->tgt_ep  = decode_string(&pos);
		oob_divert->new_cpt = decode_string(&pos);
		oob_divert->new_ep  = decode_string(&pos);
	}
	else
	{
		source       = decode_string(&pos);
		src_endpoint = decode_string(&pos);
		target       = decode_string(&pos);
		tgt_endpoint = decode_string(&pos);
		topic        = decode_string(&pos);
		seq          = decode_count(&pos);
		decode_count(&pos); // Ignore source sequence number
		hc           = decode_hashcode(&pos);

		length = abst->get_length() - (pos - buf);
		data = new uchar[length + 1];
		memcpy(data, pos, length);
		pos += length;
		data[length] = '\0';
	}
	return 0;
}

AbstractMessage *scomm::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	sb = begin_msg(type);
	sb->cat_string(source);
	sb->cat_string(src_endpoint);
	sb->cat_string(target);
	sb->cat_string(tgt_endpoint);
	sb->cat_string(topic);
	sb->cat(seq);
	sb->cat(0); // source seq ID - unknown as yet	
	sb->cat(hc);
	sb->cat((const void *)data, length);
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* svisitor */

svisitor::svisitor()
{
	src_cpt = tgt_cpt = src_instance = NULL;
	required_endpoint = NULL;
	msg_hc = reply_hc = NULL;
	data = NULL;
}

svisitor::~svisitor()
{
	if(src_cpt != NULL) delete[] src_cpt;
	if(src_instance != NULL) delete[] src_instance;
	if(tgt_cpt != NULL) delete[] tgt_cpt;
	if(required_endpoint != NULL) delete[] required_endpoint;
	if(msg_hc != NULL) delete msg_hc;
	if(reply_hc != NULL) delete reply_hc;
	if(data != NULL) delete[] data;
}

AbstractMessage *svisitor::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	sb = begin_msg(MessageVisitor);
	sb->cat_string(src_cpt);
	sb->cat_string(src_instance);
	sb->cat(dispose_id);
	sb->cat_string(tgt_cpt);
	sb->cat_string(required_endpoint);
	sb->cat_byte(ep_type);
	sb->cat(msg_hc);
	sb->cat(reply_hc);
	sb->cat((const void *)data, length);
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* sgoodbye */

sgoodbye::sgoodbye()
{
	src_cpt = src_ep = NULL;
	tgt_cpt = tgt_ep = NULL;
}

sgoodbye::~sgoodbye()
{
	if(src_cpt != NULL) delete[] src_cpt;
	if(src_ep != NULL) delete[] src_ep;
	if(tgt_cpt != NULL) delete[] tgt_cpt;
	if(tgt_ep != NULL) delete[] tgt_ep;
}

AbstractMessage *sgoodbye::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	sb = begin_msg(MessageGoodbye);
	sb->cat_string(src_cpt);
	sb->cat_string(src_ep);
	sb->cat_string(tgt_cpt);
	sb->cat_string(tgt_ep);
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* sresub */

sresub::sresub()
{
	src_cpt = src_ep = NULL;
	tgt_cpt = tgt_ep = NULL;
	
	subscription = topic = NULL;
}

sresub::~sresub()
{
	if(src_cpt != NULL) delete[] src_cpt;
	if(src_ep != NULL) delete[] src_ep;
	if(tgt_cpt != NULL) delete[] tgt_cpt;
	if(tgt_ep != NULL) delete[] tgt_ep;
	
	if(subscription != NULL) delete[] subscription;
	if(topic != NULL) delete[] topic;
}

AbstractMessage *sresub::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	sb = begin_msg(MessageResubscribe);
	sb->cat_string(src_cpt);
	sb->cat_string(src_ep);
	sb->cat_string(tgt_cpt);
	sb->cat_string(tgt_ep);
	
	sb->cat_string(subscription);
	sb->cat_string(topic);
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* sdivert */

sdivert::sdivert()
{
	src_cpt = src_ep = NULL;
	tgt_cpt = tgt_ep = NULL;
	new_cpt = new_ep = NULL;
}

sdivert::~sdivert()
{
	if(src_cpt != NULL) delete[] src_cpt;
	if(src_ep != NULL) delete[] src_ep;
	if(tgt_cpt != NULL) delete[] tgt_cpt;
	if(tgt_ep != NULL) delete[] tgt_ep;
	if(new_cpt != NULL) delete[] new_cpt;
	if(new_ep != NULL) delete[] new_ep;

}

AbstractMessage *sdivert::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	sb = begin_msg(MessageDivert);
	sb->cat_string(src_cpt);
	sb->cat_string(src_ep);
	sb->cat_string(tgt_cpt);
	sb->cat_string(tgt_ep);
	sb->cat_string(new_cpt);
	sb->cat_string(new_ep);
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* sflow */

sflow::sflow()
{
	src_cpt = src_ep = NULL;
	tgt_cpt = tgt_ep = NULL;
}

sflow::~sflow()
{
	if(src_cpt != NULL) delete[] src_cpt;
	if(src_ep != NULL) delete[] src_ep;
	if(tgt_cpt != NULL) delete[] tgt_cpt;
	if(tgt_ep != NULL) delete[] tgt_ep;
}

AbstractMessage *sflow::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	sb = begin_msg(MessageFlow);
	sb->cat_string(src_cpt);
	sb->cat_string(src_ep);
	sb->cat_string(tgt_cpt);
	sb->cat_string(tgt_ep);
	sb->cat(window);
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

int sflow::reveal(AbstractMessage *abst)
{
	unsigned char *pos;
	
	if(abst->get_type() != MessageFlow)
		return -1;
	
	pos = abst->get_data();
	src_cpt = decode_string(&pos);
	src_ep = decode_string(&pos);
	tgt_cpt = decode_string(&pos);
	tgt_ep = decode_string(&pos);
	window = decode_count(&pos);

	return 0;
}

/* shello */

shello::shello()
{
	source = target = NULL;
	from_address = from_endpoint = required_endpoint = from_instance = NULL;
	msg_hc = reply_hc = NULL;
	subs = topic = NULL;
	oob_visitor = NULL;
	from_ep_id = required_ep_id = flexible_matching = 0;
}

shello::~shello()
{
	if(source != NULL) delete[] source;
	if(target != NULL) delete[] target;
	if(from_address != NULL) delete[] from_address;
	if(from_instance != NULL) delete[] from_instance;
	if(from_endpoint != NULL) delete[] from_endpoint;
	if(required_endpoint != NULL) delete[] required_endpoint;
	if(msg_hc != NULL) delete msg_hc;
	if(reply_hc != NULL) delete reply_hc;
	if(subs != NULL) delete[] subs;
	if(topic != NULL) delete[] topic;
	if(oob_visitor != NULL) delete oob_visitor;
}

int shello::reveal(AbstractMessage *abst)
{
	// May read svisitor instead
	// Returns -1 if wrong msg type, else 0
	unsigned char *buf, *pos;
	MessageType msg_type;

	msg_type = abst->get_type();	
	if(msg_type != MessageHello && msg_type != MessageVisitor)
		return -1;
	
	buf = abst->get_data();
	pos = buf;
	
	if(msg_type == MessageVisitor)
	{
		int len;		
		oob_visitor = new svisitor();
		oob_visitor->src_cpt    = decode_string(&pos);
		oob_visitor->src_instance = decode_string(&pos);
		oob_visitor->dispose_id = decode_count(&pos);
		oob_visitor->tgt_cpt    = decode_string(&pos);
		oob_visitor->required_endpoint = decode_string(&pos);
		oob_visitor->ep_type    = (EndpointType)decode_byte(&pos);
		oob_visitor->msg_hc     = decode_hashcode(&pos);
		oob_visitor->reply_hc   = decode_hashcode(&pos);
		len = abst->get_length() - (pos - buf);
		oob_visitor->length = len;
		oob_visitor->data = new uchar[len + 1];
		memcpy(oob_visitor->data, pos, len);
		pos += len;
		oob_visitor->data[len] = '\0';
	}
	else
	{
		source        = decode_string(&pos);
		from_address  = decode_string(&pos);
		from_instance = decode_string(&pos);
		from_endpoint = decode_string(&pos);
		from_ep_id    = decode_count(&pos);
		target        = decode_string(&pos);
		required_endpoint = decode_string(&pos);
		required_ep_id    = decode_count(&pos);
		target_type = (EndpointType)decode_byte(&pos);
		flexible_matching = decode_byte(&pos);
		subs     = decode_string(&pos);
		topic    = decode_string(&pos);
		msg_hc   = decode_hashcode(&pos);
		reply_hc = decode_hashcode(&pos);
	}
	return 0;
}

AbstractMessage *shello::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	sb = begin_msg(MessageHello);
	sb->cat_string(source);
	sb->cat_string(from_address);
	sb->cat_string(from_instance);
	sb->cat_string(from_endpoint);
	sb->cat(from_ep_id);
	sb->cat_string(target); // May be NULL
	sb->cat_string(required_endpoint);
	sb->cat(required_ep_id);
	sb->cat_byte(target_type);	
	sb->cat_byte(flexible_matching);
	sb->cat_string(subs);
	sb->cat_string(topic);
	sb->cat(msg_hc);
	sb->cat(reply_hc);
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}

/* swelcome */

swelcome::swelcome()
{
	cpt_name = endpoint = subs = topic = instance = address = NULL;
	msg_poly = reply_poly = 0;
	msg_hc = reply_hc = NULL;
}

swelcome::~swelcome()
{
	if(cpt_name != NULL) delete[] cpt_name;
	if(instance != NULL) delete[] instance;
	if(endpoint != NULL) delete[] endpoint;
	if(subs != NULL) delete[] subs;
	if(topic != NULL) delete[] topic;
	if(address != NULL) delete[] address;
	if(msg_hc != NULL) delete msg_hc;
	if(reply_hc != NULL) delete reply_hc;
}

int swelcome::reveal(AbstractMessage *abst)
{
	// returns -1 if wrong msg type, else 0
	unsigned char *pos;
	MessageType msg_type;

	msg_type = abst->get_type();
	if(msg_type != MessageWelcome)
		return -1;
	
	pos = abst->get_data();
	code = (AcceptanceCode)decode_byte(&pos);
	cpt_name = decode_string(&pos);
	instance = decode_string(&pos);
	endpoint = decode_string(&pos);
	address = decode_string(&pos);
	subs = decode_string(&pos);
	topic = decode_string(&pos);
	msg_poly = decode_byte(&pos);
	reply_poly = decode_byte(&pos);
	msg_hc   = decode_hashcode(&pos);
	reply_hc = decode_hashcode(&pos);
			
	return 0;
}

AbstractMessage *swelcome::wrap(int fd)
{
	StringBuf *sb;
	AbstractMessage *abst;
	
	sb = begin_msg(MessageWelcome);
	sb->cat_byte(code);
	sb->cat_string(cpt_name);
	sb->cat_string(instance);
	sb->cat_string(endpoint);
	sb->cat_string(address);
	sb->cat_string(subs); // May be NULL
	sb->cat_string(topic); // May be NULL
	sb->cat_byte(msg_poly);
	sb->cat_byte(reply_poly);
	sb->cat(msg_hc);
	sb->cat(reply_hc);
	abst = new AbstractMessage(fd, sb);
	delete sb;
	return abst;
}
