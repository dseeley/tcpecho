/**
  @file

  @brief A forking TCP echo server.

  @author Dougal
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <syslog.h>


#define BACKLOG 10     // how many pending connections queue will hold

#define MAX_RECV_BUFF_SIZE 256 // max number of bytes we can get at once
#define MAX_SEND_BUFF_SIZE 256 // max number of bytes we can send at once

#ifndef SYSLOGID
#define SYSLOGID "tcpecho"
#endif


static void terminate_server ();


/**
  @brief get sockaddr, IPV4 or IPV6
  @param struct sockaddr *sa
  @return
 */
static void *get_in_addr (struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}


/**
  @brief Sends on the socket
  @param connected_fd
  @param *string_to_send
  @param flags
  @return
 */
static void my_socket_send (int connected_fd, char *string_to_send, int flags)
{
    if (send(connected_fd, string_to_send, strlen(string_to_send), 0) == -1)
        syslog(LOG_CRIT, "send error");
}


/**
  @brief Cleans up prior to exit
 */
static void terminate_server ()
{
    syslog(LOG_INFO, "Server terminated.");
    closelog();
}

/**
  @brief entry point
  @param
  @return
 */
int main (int argc, char *argv[])
{
    int sock_fd, connected_fd; /*listen on sock_fd, new connection on connected_fd */
    struct addrinfo hints = {0}, *servinfo = NULL, *servinfo_item = NULL;
    struct sockaddr_storage their_addr; /*connector's address information */
    socklen_t sin_size;
    struct sigaction s4_sigaction;
    char remote_ip[INET6_ADDRSTRLEN];
    int getaddrinfo_retval;
    char local_ip[INET6_ADDRSTRLEN];

    if (argc < 2) 
    {
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        terminate_server();
        exit(EXIT_FAILURE);
    }
    char *server_port = (argv[1]);
    
    openlog(SYSLOGID, LOG_PID, LOG_DAEMON);

    hints.ai_socktype = SOCK_STREAM;

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        syslog(LOG_INFO, "socket() failure %s", strerror(errno));
        terminate_server();
        exit(EXIT_FAILURE);
    }
    else
    {
        if ((getaddrinfo_retval = getaddrinfo("0.0.0.0", server_port, &hints, &servinfo)) != 0)
        {
            syslog(LOG_CRIT, "getaddrinfo %s", gai_strerror(getaddrinfo_retval));
            terminate_server();
            exit(EXIT_FAILURE);
        }
        else
        {
            int option_name = 1;
            if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &option_name, sizeof (int)) == -1)
            {
                syslog(LOG_CRIT, "setsockopt() failure: %s", strerror(errno));
                exit(EXIT_FAILURE);
            }
            else
            {
                inet_ntop(AF_INET, &(((struct sockaddr_in *) servinfo->ai_addr)->sin_addr), local_ip, sizeof (local_ip));
                if ((bind(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen)) == -1)
                {
                    syslog(LOG_INFO, "bind(): could not bind to %s: %s", local_ip, strerror(errno));
                    terminate_server();
                    exit(EXIT_FAILURE);
                }
                else
                {
                    syslog(LOG_INFO, "bind(): bound to %s", local_ip);
                }
            }
        }
    }

    /* Free the servinfo structure */
    freeaddrinfo(servinfo);

    if (listen(sock_fd, BACKLOG) == -1)
    {
        fprintf(stderr, "listen failure\n");
        syslog(LOG_EMERG, "listen failure: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("TCP echo server: waiting for connections...\n");

    /* main accept() loop */
    while (1)
    {
        sin_size = sizeof their_addr;
        if ((connected_fd = accept(sock_fd, (struct sockaddr *) &their_addr, &sin_size)) == -1)
        {
            fprintf(stderr, "accept failure [%i]\n", connected_fd);
            syslog(LOG_CRIT, "accept failure");
        }
        else
        {
            inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *) &their_addr), remote_ip, sizeof remote_ip);
            syslog(LOG_INFO, "accepted from %s", remote_ip);
            printf("server: accepted from %s\n", remote_ip);
            my_socket_send(connected_fd, "Socket connected.\n", MSG_EOR);

            /*Create a child process to talk to each client*/
            if (!fork())
            {
                int numbytes = 0;
                char recv_buff[MAX_RECV_BUFF_SIZE] = {0};

                close(sock_fd);     // child doesn't need the socket listener

                while (1)
                {
                    if ((numbytes = recv(connected_fd, recv_buff, MAX_RECV_BUFF_SIZE - 1, 0)) == -1)
                    {
                        fprintf(stderr, "recv failure [%i] [%s]\n", connected_fd, recv_buff);
                        syslog(LOG_CRIT, "recv failure");
                        exit(EXIT_FAILURE);
                    }
                
                    if (strcmp(recv_buff, "quit") == 0 || strcmp(recv_buff, "bye") == 0 || strcmp(recv_buff, "exit") == 0)
                    {
                        break;
                    }
                    else
                    {
                        char send_buff_max[MAX_SEND_BUFF_SIZE] = {0};
                        snprintf(send_buff_max, MAX_SEND_BUFF_SIZE, "%s", recv_buff);
                        my_socket_send(connected_fd, send_buff_max, MSG_EOR);
                    }
                }
                
                my_socket_send(connected_fd, "Disconnected from server\n", MSG_EOR);
                printf("server: Disconnected from %s\n", remote_ip);

                close(connected_fd);
                exit(EXIT_SUCCESS);
            }
        }

        close(connected_fd);    // parent doesn't need this
    }

    return 0;
}
