// error.cpp - DMI - 17-2-2007

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "datatype.h"
#include "error.h"

const int LogNothing   = 0;
const int LogErrors    = 1 << 0;
const int LogWarnings  = 1 << 1;
const int LogMessages  = 1 << 2;
const int LogDebugging = 1 << 3;
const int LogDefault = LogErrors | LogWarnings | LogMessages | LogDebugging;
const int EchoDefault = LogErrors | LogWarnings | LogMessages;
const int LogAll = LogErrors | LogWarnings | LogMessages | LogDebugging;

int log_level = LogDefault;
int echo_level = EchoDefault;

FILE *fp_log = NULL;

// Prototypes:
void init_levels();

void sassert(int t, const char *msg)
{
	if(t)
		return;
	error("Assertion failed: %s", msg);
}

SchemaException::SchemaException(const char *s, int l)
{
	msg = sdup(s);
	line = l;
}

ProtocolException::ProtocolException(const char *s)
{ msg = sdup(s); }

ValidityException::ValidityException(const char *s)
{ msg = sdup(s); }

SubscriptionException::SubscriptionException(const char *s)
{ msg = sdup(s); }

ImportException::ImportException(const char *s)
{ msg = sdup(s); }

char *get_sbus_dir()
{
	char *dir;
	struct stat buf;
	int ret;
	
	dir = sdup(getenv("SBUS_DIR"));
	if(dir == NULL)
	{
		// Get home dir:
		struct passwd *pwd;
		char *home;
		uid_t id;
		
		id = getuid();
		pwd = getpwuid(id);
		if(pwd == NULL)
			error("Can't get password structure");
		#ifndef __ANDROID__
		home = pwd->pw_dir;
		#else
		home = "/data/data/uk.ac.cam.tcs40.sbus.sbus/files";
		#endif
		dir = new char[strlen(home) + 20];
		sprintf(dir, "%s/.sbus", home);
	}
	ret = stat(dir, &buf);
	if(ret < 0)
	{
		warning("SBUS directory %s doesn't exist; creating it", dir);
		ret = mkdir(dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
		if(ret < 0)
		{
			if(errno != EEXIST)
				error("Failed to create SBUS directory %s", dir);
		}
	}
	return dir;
}

void init_logfile(const char *cpt_name, const char *instance_name, int wrapper)
{
	// Set fp_log:
	char *s, *dir;
	char type;
	struct stat buf;
	int ret;

	dir = get_sbus_dir();

	s = new char[strlen(dir) + strlen(cpt_name) + 25 +
			(instance_name == NULL ? 0 : strlen(instance_name))];
	sprintf(s, "%s/log", dir);
	ret = stat(s, &buf);
	if(ret < 0)
	{
		warning("Log directory %s doesn't exist; creating it", s);
		ret = mkdir(s, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
		if(ret < 0)
		{
			if(errno != EEXIST)
				error("Failed to create log directory %s", s);
		}
	}
	
	if(wrapper)
		type = 'W';
	else
		type = 'L';
	if(instance_name == NULL || !strcmp(cpt_name, instance_name))
		sprintf(s, "%s/log/%s-%c.log", dir, cpt_name, type);
	else
		sprintf(s, "%s/log/%s-%s-%c.log", dir, cpt_name, instance_name, type);

	fp_log = fopen(s, "w");
	if(fp_log == NULL)
		error("Can't open log file %s", s);
	delete[] s;
	delete[] dir;

	init_levels();
}

void init_levels()
{
	// Read SBUS_LOG_LEVEL and SBUS_ECHO_LEVEL to set log_level & echo_level
	char *s;
	
	s = getenv("SBUS_LOG_LEVEL");
	if(s != NULL)
		log_level = atoi(s);
	s = getenv("SBUS_ECHO_LEVEL");
	if(s != NULL)
		echo_level = atoi(s);
}

void error(const char *format, ...)
{
	const int MAX_ERR_LEN = 200;
	va_list args;
	char *c = new char[MAX_ERR_LEN];
	char *d = new char[MAX_ERR_LEN + 20];

	va_start(args, format);
	vsnprintf(c, MAX_ERR_LEN, format, args);
	va_end(args);
	c[MAX_ERR_LEN - 1] = '\0';
	sprintf(d, "%s\n", c);

	if(log_level & LogErrors)
	{
		if(fp_log == NULL)
			syslog(LOG_INFO, "%s", d);
		else
		{
			fwrite(d, strlen(d), 1, fp_log);
			fflush(fp_log);
		}
	}
	if(echo_level & LogErrors)
	{
		fprintf(stderr, "%s", d);
		// printf(d);
	}
	
	exit(-1);
}

void log(const char *format, ...)
{
	#ifdef __ANDROID__
	// this is to stop large log files in internal memory of phone.
	log_level = echo_level = LogErrors | LogWarnings;
	#endif
	
	const int MAX_ERR_LEN = 200;
	va_list args;
	char *c = new char[MAX_ERR_LEN];
	char *d = new char[MAX_ERR_LEN + 20];

	va_start(args, format);
	vsnprintf(c, MAX_ERR_LEN, format, args);
	va_end(args);
	c[MAX_ERR_LEN - 1] = '\0';
	sprintf(d, "%s\n", c);
	
	if(log_level & LogMessages)
	{
		if(fp_log == NULL)
			syslog(LOG_INFO, "%s", d);
		else
		{
			fwrite(d, strlen(d), 1, fp_log);
			fflush(fp_log);
		}
	}
	if(echo_level & LogMessages)
	{
		fprintf(stderr, "%s", d);
		// printf(d);
	}
	delete[] c;
	delete[] d;
}

void warning(const char *format, ...)
{
	const int MAX_ERR_LEN = 200;
	va_list args;
	char *c = new char[MAX_ERR_LEN];
	char *d = new char[MAX_ERR_LEN + 20];

	va_start(args, format);
	vsnprintf(c, MAX_ERR_LEN, format, args);
	va_end(args);
	c[MAX_ERR_LEN - 1] = '\0';
	sprintf(d, "%s\n", c);
	
	if(log_level & LogWarnings)
	{
		if(fp_log == NULL)
			syslog(LOG_INFO, "%s", d);
		else
		{
			fwrite(d, strlen(d), 1, fp_log);
			fflush(fp_log);
		}
	}
	if(echo_level & LogWarnings)
	{
		fprintf(stderr, "%s", d);
		// printf(d);
	}
	syslog(LOG_INFO, "%s", d);
	delete[] c;
	delete[] d;
}

void debug(const char *format, ...)
{
	const int MAX_ERR_LEN = 200;
	va_list args;
	char *c = new char[MAX_ERR_LEN];
	char *d = new char[MAX_ERR_LEN + 20];

	va_start(args, format);
	vsnprintf(c, MAX_ERR_LEN, format, args);
	va_end(args);
	c[MAX_ERR_LEN - 1] = '\0';
	sprintf(d, "%s\n", c);

	if(log_level & LogDebugging)
	{
		if(fp_log == NULL)
			syslog(LOG_INFO, "%s", d);
		else
		{
			fwrite(d, strlen(d), 1, fp_log);
			fflush(fp_log);
		}
	}
	if(echo_level & LogDebugging)
	{
		fprintf(stderr, "%s", d);
		// printf(d);
	}
	delete[] c;
	delete[] d;
}

char *sformat(const char *format, ...)
{
	const int MAX_ERR_LEN = 200;
	va_list args;
	char *c;

	c = new char[MAX_ERR_LEN];
	va_start(args, format);
	vsnprintf(c, MAX_ERR_LEN, format, args);
	va_end(args);
	c[MAX_ERR_LEN - 1] = '\0';
	return c;
}
