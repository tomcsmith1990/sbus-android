// marshall.h - DMI - 20-2-2007

unsigned char *marshall(snode *node, Schema *schema, int *length,
	const char **err);

/* Return value is binary data. Number of bytes returned in length.
	In case of error, returns NULL and places an error message in err */
