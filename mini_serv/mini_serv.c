#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netdb.h>

typedef struct s_client {
    int fd;
    int id;
    int new;
    struct s_client *next;
} t_client;

int sockfd;
int gid = 0;
t_client *clients = NULL;
fd_set allfds, readfds, writefds;
char msg[100];

void fatal()
{
    char *err = "Fatal error\n";
    write(2, err, strlen(err));
    exit(1);
}

int getid(int fd)
{
    t_client *tmp = clients;
    while (tmp)
    {
        if (tmp->fd == fd)
            return (tmp->id);
        tmp = tmp->next;
    }
    return (-1);
}

int getmaxfd()
{
    int max = sockfd;
    t_client *tmp = clients;
    while (tmp)
    {
        if (tmp->fd > max)
            max = tmp->fd;
        tmp = tmp->next;
    }
    return (max);
}

void sendall(int fd, const char *chat)
{
    t_client *tmp = clients;
    while (tmp)
    {
        if (tmp->fd != fd && FD_ISSET(tmp->fd, &writefds))
            send(tmp->fd, chat, strlen(chat), 0);
        tmp = tmp->next;
    }
}

void addclient_tolist(t_client *new)
{
    t_client *tmp = clients;
    if (!tmp)
    {
        clients = new;
        return ;
    }
    while (tmp->next)
        tmp = tmp->next;
    tmp->next = new;
}

void addclient()
{
    int clientfd;

    if ((clientfd = accept(sockfd, NULL, NULL)) < 0)
        fatal();
    t_client *new = calloc(1, sizeof(t_client));
    if (!new)
        fatal();
    new->fd = clientfd;
    new->id = gid++;
    new->new = 1;
    sprintf(msg, "server: client %d just arrived\n", new->id);
    sendall(clientfd, msg);
    addclient_tolist(new);
    FD_SET(clientfd, &allfds);
}

int main(int ac, char **av)
{
    if (ac != 2)
    {
        char *err = "Wrong number of arguments\n";
        write(2, err, strlen(err));
        exit(1);
    }
    uint16_t port = atoi(av[1]);

    struct sockaddr_in servaddr; 

	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		fatal(); 
	} 
	bzero(&servaddr, sizeof(servaddr)); 

	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port);
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		fatal();
	} 
	if (listen(sockfd, 10) != 0) {
		fatal();
	}

    FD_ZERO(&allfds);
    FD_SET(sockfd, &allfds);

    while(1)
    {
        readfds = writefds = allfds;
        int maxfd = getmaxfd() + 1;
        if (select(maxfd, &readfds, &writefds, NULL, NULL) < 0)
            continue;
        for (int fd = 0; fd < maxfd; fd++)
        {
            if (FD_ISSET(fd, &readfds))
            {
                if (fd == sockfd)
                {
                    addclient();
                    break;
                }
            }
        }
    }
}
