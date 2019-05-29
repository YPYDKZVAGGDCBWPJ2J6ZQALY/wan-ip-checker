/* This file belongs to the public domain. */
/* 2019 Fumu-no-Kagomeko. */

#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

union address
{
  struct sockaddr_in ip;
  struct sockaddr    any;
};

union address  addr;
char           text[16];
char const     data[16];

int
main (int argc, char** argv)
{
  FILE* prog;
  size_t size;
  int srv_fd;
  int cln_fd;

  if (argc < 2)
  {
    fputs("Usage: wan-ip-checker <dns-update-program>\n", stderr);
    fputs("The <dns-update-program> must exit with 0\n", stderr);
    fputs("and return an IPv4 address via stdout.\n", stderr);

    return 1;
  }

  openlog("wan-ip-checker", 0, LOG_DAEMON);
  syslog(LOG_INFO, "Program started.");

  srv_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

  if (srv_fd == -1)
  {
    syslog(LOG_ERR, "Failed to create the server socket.");
    closelog();

    return 1;
  }

  addr.ip.sin_family = AF_INET;
  addr.ip.sin_port = 0xFFFF;
  addr.ip.sin_addr.s_addr = 0x03010000 | (168 << 8) | 192; // INADDR_ANY;

  if (bind(srv_fd, &addr.any, sizeof(addr.ip)) == -1)
  {
    syslog(LOG_ERR, "Failed to bind the server socket.");

    goto error;
  }

  if (listen(srv_fd, 32) == -1)
  {
    syslog(LOG_ERR, "Failed to listen on the server socket.");

    goto error;
  }

update_addr:
  prog = popen(argv[1], "r");
  size = fread(text, 1, 14, prog);
  cln_fd = pclose(prog);

  if (cln_fd || size < 12)
  {
    syslog(LOG_ERR, "Failed to obtain the IPv4 address.");

    goto try_again;
  }

  text[size] = '\0';

  if (inet_aton(text, &addr.ip.sin_addr) == 0)
  {
    syslog(LOG_ERR, "Invalid IPv4 address: %s", text);

    goto error;
  }

  syslog(LOG_INFO, "WAN IP is now: %s", text);

check:
  cln_fd = open("/dev/urandom", O_RDONLY);

  if (cln_fd != -1)
  {
    read(cln_fd, data, 16);
    close(cln_fd);
  }

  cln_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (cln_fd == -1)
  {
    syslog(LOG_ERR, "Failed to create the client socket.");

    goto error;
  }

  if (connect(cln_fd, &addr.any, sizeof(addr.ip)) == -1)
  {
    syslog(LOG_INFO, "Failed to connect to the server socket.");
    close(cln_fd);

    goto update_addr;
  }

  if (send(cln_fd, data, 16, 0) == -1)
  {
    syslog(LOG_INFO, "Failed to send data.");
    close(cln_fd);

    goto update_addr;
  }

  close(cln_fd);
  sleep(1);
  cln_fd = accept(srv_fd, NULL, NULL);

  if (cln_fd == -1)
  {
    syslog(LOG_INFO, "Failed to accept a connection.");

    goto update_addr;
  }

  if (recv(cln_fd, text, 16, 0) == -1)
  {
    syslog(LOG_INFO, "Failed to receive data.");
    close(cln_fd);

    goto update_addr;
  }

  close(cln_fd);

  if (memcmp(text, data, 16) != 0)
  {
    syslog(LOG_INFO, "Received and sent data do not match.");

    goto update_addr;
  }

try_again:
  sleep(600);

  goto check;

error:
  close(srv_fd);
  closelog();

  return 1;
}
