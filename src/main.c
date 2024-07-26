#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include "headers/main.h"
#include "headers/server.h"
#include "headers/poll.h"
#include "headers/utils.h"
#include "config/config.h"
#include "config/configutils.h"

struct program_settings_t settings;

uint32_t retrieve_address(struct config_t *cfg, unsigned char *parent, unsigned char *entry)
{
	uint32_t addr = 0;
	if(config_entry_exists(cfg, parent, entry) == 1)
    {
        char *val = config_entry_get(cfg, parent, entry);
		addr = inet_addr(val);
        free(val);
    }	
	return addr;
}

uint16_t retrieve_port(struct config_t *cfg, unsigned char *parent, unsigned char *entry)
{
	uint16_t port = 0;
	if(config_entry_exists(cfg, parent, entry) == 1)
    {
        char *val = config_entry_get(cfg, parent, entry);
		port = htons(atoi(val));
        free(val);
    }	
	return port;
}

int main(int argc, char **argv)
{
	struct config_t *main_conf = load_config("settings.txt");
	if(argc < 2)
	{
		printf("usage: %s <port>", argv[0]);
		return 1;
	}
	
	util_zero(&settings, sizeof(settings));
	poll_init();
	settings.remote_host = retrieve_address(main_conf, "remote_server", "server_ip");
	settings.local_host = retrieve_address(main_conf, "bind_server", "server_ip");
	settings.remote_port = retrieve_port(main_conf, "remote_server", "server_port");
	settings.local_port = retrieve_port(main_conf, "bind_server", "server_port");
	settings.listening_sock = socket(AF_INET, SOCK_STREAM, 0);
	server_bind(settings.listening_sock, settings.local_host, settings.local_port);
	pthread_t poll_thread;
	pthread_create(&poll_thread, NULL, &poll_process, NULL);
	pthread_t starting_thread;
	pthread_create(&starting_thread, NULL, &server_start, &settings);
	while(1)
	{
		sleep(1);
		//check resource usage every second,
		// halt if overloaded
	}
	return 0;
}

