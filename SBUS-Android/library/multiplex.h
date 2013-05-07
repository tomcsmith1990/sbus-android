// multiplex.h - DMI - 14-9-02

#include <sys/time.h>

enum multi_mode { MULTI_READ, MULTI_WRITE };

class multiplex
{
	public:
		
		multiplex(int capacity = 10);
		~multiplex();
		
		void add(int fd, multi_mode mode = MULTI_READ,
				const char *comment = NULL);
		void add(sendpoint *ep); // Also enables check for ep->message_waiting()
		void remove(int fd, multi_mode mode = MULTI_READ, int silent = 0);
		void remove(sendpoint *ep, int silent = 0);
		void remove(const char *ep_name, int silent);
		int contains(int fd, multi_mode mode = MULTI_READ);
		void clear();

		// These functions return a FD, or -1 if none ready before timeout:
		int poll();
		int wait();
		int wait(int us);
		
		multi_mode last_mode();
		
		void trace(const char *label);
		
	private:
		
		const char *label;
			
		fd_set read_fds, write_fds;
		int max_fd;
		struct timeval tv;
		multi_mode lmode;
		
		int *rlist, *wlist;
		int num_read, num_write, capacity;
		int pending;
		sendpointvector *checked;
		
		void init_sets();
		int output(); // Returns -1 if nothing ready after all
		void expand_capacity();
		int fd_waiting();
		
		void check_validity();
};
