



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <event.h>

int
main(int argc, char **argv)
{
	
	event_init();

	return (0);
}

