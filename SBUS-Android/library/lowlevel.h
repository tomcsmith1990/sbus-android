// lowlevel.h - DMI - 9-5-2009

const int ACTIVE_SOCK_NORMAL = 0;
const int ACTIVE_SOCK_SILENT = 1;
const int ACTIVE_SOCK_NONBLOCK = 2;

const int DEFAULT_PORT = 9219;

// passivesock returns socket FD. *port = -1 for any. Actual returned in *port
int passivesock(int *port);
int acceptsock(int master_sock, char **remote_address = NULL);
// acceptsock returns -1 on error

// The three activesock() variants all return socket FD's, or -1 on error
int activesock(const char *address, int flags = ACTIVE_SOCK_NORMAL);
// Address string may encode host and port
int activesock(int port, int flags = ACTIVE_SOCK_NORMAL);
int activesock(int port, const char *hostname, int flags = ACTIVE_SOCK_NORMAL);

void sock_nodelay(int sock); // Already done for you
void sock_noblock(int sock); // Use for asynch-type I/O in wrapper
// if noblocking, read or write return -1 and errno is set to EAGAIN

// char *get_local_ip(int& family);
char *get_local_ip();
char *get_local_ip_socket();
char *get_local_address(int sock);

// fixed_read & fixed_write return -1 on error/EOF, 0 on success:
int fixed_read(int sock, unsigned char *buf, int nbytes);
int fixed_write(int sock, const unsigned char *buf, int nbytes);
