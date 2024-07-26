#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "headers/main.h"
#include "headers/remote.h"
#include "headers/utils.h"

unsigned char remote_connect(int rsid)
{
	struct sockaddr_in addr;
	util_zero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = settings.remote_host;
	addr.sin_port = settings.remote_port;
	if (rsid == -1) 
    {
        return 0;
    }
    int flags = fcntl(rsid, F_GETFL, 0);
    if (flags == -1) 
    {
		close(rsid);
        return 0;
    }
    flags |= O_NONBLOCK;
    if (fcntl(rsid, F_SETFL, flags) == -1) 
    {
		close(rsid);
        return 0;
    }
    int rc = connect(rsid, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == 0) 
    {
		return 1;
    }
    else
    {
		socklen_t lon = sizeof(int);
		int valopt;
		getsockopt(rsid, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon);
		if (valopt != 0) 
		{
			close(rsid);
			return 0;
		} 
		else 
		{
			flags = fcntl(rsid, F_GETFL, NULL);
			flags &= (~O_NONBLOCK);
			fcntl(rsid, F_SETFL, flags);
			return 1;
		}
		close(rsid);
		return 0;
    }
}

