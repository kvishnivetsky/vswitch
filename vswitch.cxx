#include "precomp.h"
#include <linux/if.h>
#include <linux/if_tun.h>

#define PERROR(x) do { perror(x); return false; } while (0)

static int maxFD = 0;

int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  /* Arguments taken by the function:
   *
   * char *dev: the name of an interface (or '\0'). MUST have enough
   *   space to hold the interface name if '\0' is passed
   * int flags: interface flags (eg, IFF_TUN etc.)
   */

   /* open the clone device */
   if( (fd = open(clonedev, O_RDWR)) < 0 ) {
     return fd;
   }

   /* preparation of the struct ifr, of type "struct ifreq" */
   memset(&ifr, 0, sizeof(ifr));

   ifr.ifr_flags = flags;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

   if (*dev) {
     /* if a device name was specified, put it in the structure; otherwise,
      * the kernel will try to allocate the "next" device of the
      * specified type */
     strncpy(ifr.ifr_name, dev, IFNAMSIZ);
   }

   /* try to create the device */
   if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
     close(fd);
     return err;
   }

  /* if the operation was successful, write back the name of the
   * interface to the variable "dev", so the caller can know
   * it. Note that the caller MUST reserve space in *dev (see calling
   * code below) */
  strcpy(dev, ifr.ifr_name);

  /* this is the special file descriptor that the caller will use to talk
   * with the virtual interface */
  return fd;
}

bool config_create_link(int argc, char ** argv) {
    int link_fd = -1;
    int i = 0;
    while (i < argc) {
	VSPORT Port;memset(&Port,0,sizeof(VSPORT));
	if (strcmp(argv[i], "tap") == 0) {
	    Port.FDType = 0;
	    i++;
	    char tap_name[255];memset(tap_name,0,sizeof(tap_name));
	    sprintf(tap_name, "tap%s", argv[i]);
	    link_fd = tun_alloc(tap_name, IFF_TAP);  /* tap interface */
	    maxFD = link_fd+1;
	    links.insert(std::pair<int,VSPORT>(link_fd, Port));
	    continue;
	}
	if (strcmp(argv[i], "sock") == 0) {
	    Port.FDType = 1;
	    i++;
	    struct sockaddr_in sa_local;memset(&sa_local, 0, sizeof(sa_local));
	    struct sockaddr_in sa_peer;memset(&sa_local, 0, sizeof(sa_peer));
	    if ((link_fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) PERROR("socket");
	    sa_local.sin_family = AF_INET;
	    sa_local.sin_addr.s_addr = inet_addr(argv[i]);
	    i++;
	    sa_local.sin_port = htons(atoi(argv[i]));
	    i++;
	    if (bind(link_fd, (sockaddr*)&sa_local, sizeof(sa_local))==-1) {
		close(link_fd);
		PERROR("bind");
	    }
	    sa_peer.sin_family = AF_INET;
	    sa_peer.sin_addr.s_addr = inet_addr(argv[i]);
	    i++;
	    sa_peer.sin_port = htons(atoi(argv[i]));
	    i++;
	    if( connect(link_fd, (struct sockaddr*)&sa_peer, sizeof(sa_peer)) < 0 ) {
		close(link_fd);
		PERROR("connect");
	    }
	    maxFD = link_fd+1;
	    links.insert(std::pair<int,VSPORT>(link_fd, Port));
	    continue;
	}
	i++;
    }
    return true;
}

bool config_parse_link(const char * arg) {
    char * args[255];
    char in[65536];
    strcpy(in, arg);
    char * d = in;
    int n = 0;
    while(d = strtok(d, "/-:")) {
	switch(n) {
	    case 0:
		printf("Link: type %s ", d);
	    break;
	    case 1:
		printf("params %s\n", d);
	    break;
	}
	args[n++] = d;
	d += strlen(d)+1;
    }
    return config_create_link(n, args);
}

bool config_parse_links(const char * arg) {
    char in[65536];
    strcpy(in, arg);
    char * l = in;
    while(l = strtok(l, ",")) {
	config_parse_link(l);
	l += strlen(l)+1;
    }
    return true;
}

bool config_from_file(const char * cfgFile) {
    return true;
}

bool config_from_args(int argc, char ** argv) {
    for (int i=1;i<argc-1;i++) {
	if (argv[i][0] == '-') {
	    if (argv[i][1] == 'l') {
		config_parse_links(argv[i+1]);
	    }
	}
    }
    return true;
}

int getLinkCount() {
    return links.size();
}

int start_loop() {
    while(true) {
	fd_set linkset;FD_ZERO(&linkset);
	for(std::map<int,VSPORT>::iterator it = links.begin(); it!=links.end(); it++) {
	    FD_SET(it->first, &linkset);
	}
	int nready = 0;
	if ( (nready = select(maxFD, &linkset, NULL, NULL, NULL)) < 0) {
	    if (errno == EINTR)
		continue;
	    /* back to for() */
	    else
		PERROR("select");
	}
	for(std::map<int,VSPORT>::iterator it = links.begin(); it!=links.end(); it++) {
	    if (FD_ISSET(it->first, &linkset)) {
		char msg[65535];
		if (it->second.FDType == 0) {
		    int n = read(it->first, msg, sizeof(msg));
		    if (n > 0) {
			printf("Got %d octet(s) from link 0x%.8x\n", n, it->first);
			for(std::map<int,VSPORT>::iterator its = links.begin(); its!=links.end(); its++) {
			    if (its->first != it->first) {
				printf("Sending %d on link 0x%.8x\n", n, its->first);
				switch(its->second.FDType) {
				    case 0:
					write(its->first, msg, n);
				    break;
				    case 1:
					send(its->first, msg, n, 0);
				    break;
				}
			    }
			}
		    } else {
			perror("recv");
			continue;
		    }
		    continue;
		}
		if (it->second.FDType == 1) {
		    int n = recv(it->first, msg, sizeof(msg), 0);
		    if (n > 0) {
			printf("Got %d octet(s) from link 0x%.8x\n", n, it->first);
		    } else {
			perror("recv");
			continue;
		    }
		    continue;
		}
	    }
	}
    }
}
