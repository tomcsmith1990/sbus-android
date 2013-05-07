// unmarshall.h - DMI - 23-2-2007

snode *unmarshall(const unsigned char *data, int bytes, Schema *schema,
	const char **err);

/* In case of error, unmarshall() returns NULL and places an error
	message into err */
