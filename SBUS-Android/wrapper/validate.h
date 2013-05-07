// validate.h - DMI - 10-4-2007

int validate(snode *node, Schema *schema, const char **err);

/* Returns 1 if valid, else returns 0 and places an error message in err
	Also makes types more specific, suitable for unambiguous marshalling */
