/**
  @file

  @brief A threaded TCP echo server.

  @author Dougal Seeley
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>
#include <poll.h>
#include <pthread.h>         //for threading, link with lpthread
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/ioctl.h>      //Specifically for get_ip_for_interface
#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

#define BACKLOG 10              // how many pending connections queue will hold

#define MAX_RECV_BUFF_SIZE 256  // max number of bytes we can get at once
#define MAX_SEND_BUFF_SIZE 256  // max number of bytes we can send at once

#ifndef SYSLOGID
#define SYSLOGID "tcpecho"
#endif


/* protoypes */
int getgatewayandiface(char *, char *);
void *pthread_client_handler(void *);
/*************/


/**
  @brief get first IP for an interface
  @param const char *interface, char *default_ip
 */
static void get_ip_for_interface(const char *interface, char *default_ip)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct ifreq ifr = { .ifr_addr = { .sa_family = AF_INET } };        /* Get an IPv4 IP address */

    strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);                       /* Get an IP address attached to 'interface' */

    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);

    strcpy(default_ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
}


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
  @brief Sends a string on the socket
  @param connected_fd
  @param *fmt
  @param flags
 */
static void my_socket_send (int connected_fd, const char *fmt, ...)
{
    char string_to_send[MAX_SEND_BUFF_SIZE] = {0};

    /* Copy the variadic arguments to a string so they can be sent onwards */
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
    vsnprintf(string_to_send, MAX_SEND_BUFF_SIZE, fmt, arg_ptr);
	va_end(arg_ptr);

    if (send(connected_fd, string_to_send, strlen(string_to_send), 0) == -1)
        syslog(LOG_CRIT, "send error");
}


/**
  @brief Logs a message and cleans up prior to exit
 */
static void fail_exit_server (int log_pri, const char *fmt, ...)
{
    char string_to_log[MAX_SEND_BUFF_SIZE] = {0};

    /* Copy the variadic arguments to a string so they can be sent onwards */
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
    vsnprintf(string_to_log, MAX_SEND_BUFF_SIZE, fmt, arg_ptr);
	va_end(arg_ptr);

    fprintf(stderr, "%s", string_to_log);
    syslog(log_pri, "%s", string_to_log);
    syslog(LOG_INFO, "Server terminated.");
    closelog();
    exit(EXIT_FAILURE);
}

/**
  @brief printf to console and send to syslog
 */
static void printf_and_syslog (int log_pri, const char *fmt, ...)
{
    char string_to_log[MAX_SEND_BUFF_SIZE] = {0};

    /* Copy the variadic arguments to a string so they can be sent onwards */
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
    vsnprintf(string_to_log, MAX_SEND_BUFF_SIZE, fmt, arg_ptr);
	va_end(arg_ptr);

    printf("%s", string_to_log);
    syslog(log_pri, "%s", string_to_log);
}


/**
  @brief entry point
  @param
  @return
 */
int main (int argc, char *argv[])
{
    int sock_fd, connected_fd;              /*listen on sock_fd, new connection on connected_fd */
    struct addrinfo hints = { .ai_socktype = SOCK_STREAM }, *servinfo = NULL;
    char local_ip[INET6_ADDRSTRLEN];

    if (argc < 2)
    {
        fail_exit_server(LOG_WARNING, "Usage: %s [port]\n", argv[0]);
    }
    char *argv_server_port = (argv[1]);

    openlog(SYSLOGID, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_DAEMON);

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        fail_exit_server(LOG_ERR, "socket() failure %s", strerror(errno));
    }
    else
    {
        int getaddrinfo_retval;
        if ((getaddrinfo_retval = getaddrinfo("0.0.0.0", argv_server_port, &hints, &servinfo)) != 0)
        {
            fail_exit_server(LOG_ERR, "getaddrinfo() failure: %s", gai_strerror(getaddrinfo_retval));
        }
        else
        {
            int option_name = 1;
            if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &option_name, sizeof (int)) == -1)
            {
                fail_exit_server(LOG_ERR, "setsockopt() failure: %s", strerror(errno));
            }
            else
            {
                inet_ntop(AF_INET, &(((struct sockaddr_in *) servinfo->ai_addr)->sin_addr), local_ip, sizeof (local_ip));
                if ((bind(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen)) == -1)
                {
                    fail_exit_server(LOG_ERR, "bind() failure: could not bind to %s: error: %s", local_ip, strerror(errno));
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

    /* Put the socket in listening mode */
    if (listen(sock_fd, BACKLOG) == -1)
    {
        fail_exit_server(LOG_ERR, "listen failure: %s", strerror(errno));
    }

    printf_and_syslog(LOG_INFO, "TCP echo server started; accepting connections...\n");

    /* main accept() loop */
    while (1)
    {
        struct sockaddr_in clientaddr;
        socklen_t clientaddr_size = sizeof(clientaddr);
        if ((connected_fd = accept(sock_fd, (struct sockaddr *) &clientaddr, &clientaddr_size)) == -1)
        {
            fail_exit_server(LOG_ERR, "accept() failure");
        }
        else
        {
            /*Create a child process to talk to each client*/
            pthread_t client_handler_thread;
            if (pthread_create(&client_handler_thread, NULL, pthread_client_handler, (void *)(uintptr_t)connected_fd) != 0)
            {
                fail_exit_server(LOG_ERR, "pthread_create() error: Error creating thread\n");
            }
        }
    }

    return 0;
}


void *pthread_client_handler(void *arg)
{
    char remote_ip[INET6_ADDRSTRLEN];
    char gateway_address[INET_ADDRSTRLEN], interface[IF_NAMESIZE], default_ip[INET6_ADDRSTRLEN];

    int connected_fd = (uintptr_t)arg;    //Cast the generic handler arg to the passed connected socket descriptor

    struct sockaddr_storage clientaddr;
    socklen_t clientaddr_size = sizeof(clientaddr);
    getpeername(connected_fd, (struct sockaddr *)&clientaddr, &clientaddr_size);
    inet_ntop(clientaddr.ss_family, get_in_addr((struct sockaddr *) &clientaddr), remote_ip, sizeof remote_ip);

    pthread_t ptid = pthread_self();

    /* A poll() checks whether the socket is ready for reading.  It only becomes ready when something is sent to it.
       Here, we poll for 250ms to test its readiness, then, in that time, if we see if it is ready, (but with nothing on it), we ignore, as this indicates a keepalive.*/
    struct pollfd poll_set[] = { [0].fd = connected_fd, [0].events = POLLIN };
    int pollres = poll(poll_set, 1, 250);
    char recv_buff[MAX_RECV_BUFF_SIZE] = {0};
    if (pollres && recv(connected_fd, recv_buff, MAX_RECV_BUFF_SIZE - 1, MSG_PEEK) <= 0)
    {
        printf_and_syslog(LOG_INFO, "[%li] Null receive (probable keepalive check), not connecting: [remote %s] [socket %i]\n", ptid, remote_ip, connected_fd);
    }
    else
    {
        printf_and_syslog(LOG_INFO, "[%li] Accepted connection [remote %s] [socket %i]", ptid, remote_ip, connected_fd);

        getgatewayandiface(gateway_address, interface);
        get_ip_for_interface(interface, default_ip);

        my_socket_send(connected_fd, "Socket connected to %s. Echoing...\n", default_ip);
        while (1)
        {
            memset(recv_buff, 0, sizeof(recv_buff));
            *recv_buff = '\0';
            int recv_resp = recv(connected_fd, recv_buff, MAX_RECV_BUFF_SIZE - 1, 0);
            if (recv_resp == -1 || recv_resp == 0)
            {
                printf_and_syslog(LOG_INFO, "[%li] Closed connection. [recv()==%i (socket closed)] [remote %s] [socket %i]\n", ptid, recv_resp, remote_ip, connected_fd);
                break;
            }
            else
            {
                my_socket_send(connected_fd, recv_buff);
            }
        }
    }

    close(connected_fd);
    return(0);
}