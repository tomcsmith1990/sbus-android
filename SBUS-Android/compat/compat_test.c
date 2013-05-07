#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>

int main(int argc, char **argv) {
	struct ifaddrs *ifas, *ifa;
	int i;

	i = getifaddrs(&ifas);

	fprintf(stderr, "return is %d ifas is 0x%x\n", i, ifas);
	for(ifa = ifas; ifa->ifa_next != NULL; ifa = ifa->ifa_next) {
		fprintf(stderr, "%s at 0x%x\n", ifa->ifa_name, ifa->ifa_name);
		fprintf(stderr, "addr is 0x%x\n", ifa->ifa_addr);
	}

	return 0;
}
