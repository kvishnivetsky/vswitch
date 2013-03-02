
#include "precomp.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <getopt.h>
#include <sys/ioctl.h>

int main(int argc, char** argv) {

    config_from_args(argc, argv);
    printf("Has %d link(s)\n", getLinkCount());
    start_loop();
    return 0;
}
