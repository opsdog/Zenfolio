/*

  open an SSL connection and get the page

*/


/*

  basic flow according to the docs:

    SSL_library_init()  -  initialize the library
    SSL_CTX_new()  -       create CTX object - set cert options
    SSL_new  -             assigns netwok connection to an SSL object
    SSL_accept/SSL_connect - handshake
    SSL_read/SSL_write  -  read and write data
    SSL_shutdown  -        shuts down the SSL connection

*/

/*
#define DEBUG
*/

#include <stdio.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#define MAXDATASIZE 4096
#define MAXHEADERSIZE 1024
#define MAXRECEIVESIZE 20480

int main(int argc, char *argv[])
{
  int ZenSocket, type, SocketFD, numbytes;
  int RemotePort = 443;
  u_short portbase = 0;
  char buf[MAXRECEIVESIZE];
  /*  char RemoteHost[25] = "www.zenfolio.com";*/
  char RemoteHost[25] = "api.zenfolio.com";

  char Header[MAXHEADERSIZE];
  char Content[MAXDATASIZE];
  int HeaderLength, ContentLength;
  char ContentLengthstr[5];

  struct sockaddr_in LocalAddr, RemoteAddr;
  struct servent *ZenService;
  struct hostent *ZenHost;
  struct protoent *ZenProtocol;

  SSL_CTX *ZenCTX;
  SSL *ZenSSL;

  /* open the socket */

  /* get the host info */

  if((ZenHost=gethostbyname(RemoteHost)) == NULL) {
    perror("gethostbyname()");
    exit(1);
  }
#ifdef DEBUG
  else
    printf("Client-The remote host is: %s\n", RemoteHost);
#endif

  if((SocketFD = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket()");
    exit(1);
  }
#ifdef DEBUG
  else
    printf("Client-The socket() sockfd is OK...\n");
#endif

  /* host byte order*/

  RemoteAddr.sin_family = AF_INET;

  /* short, network byte order*/

#ifdef DEBUG
  printf("Server-Using %s and port %d...\n", RemoteHost, RemotePort);
#endif

  RemoteAddr.sin_port = htons(RemotePort);
  RemoteAddr.sin_addr = *((struct in_addr *)ZenHost->h_addr);

  /* zero the rest of the struct */

  memset(&(RemoteAddr.sin_zero), '\0', 8);

  if(connect(SocketFD, (struct sockaddr *)&RemoteAddr, sizeof(struct sockaddr)) == -1) {
    perror("connect()");
    exit(1);
  }
#ifdef DEBUG
  else
    printf("Client-The connect() is OK...\n");
#endif

  /*
     pretend we know what we're doing with SSL
  */

  SSL_library_init();
  SSL_load_error_strings();

  ZenCTX = SSL_CTX_new(SSLv23_client_method());
  ZenSSL = SSL_new(ZenCTX);
  SSL_set_connect_state(ZenSSL);  
  type = SSL_set_fd(ZenSSL, SocketFD);
  type = SSL_connect(ZenSSL);

#ifdef DEBUG
  printf("CTX %d\n",(int)ZenCTX);
  printf("SSL %d\n",(int)ZenSSL);
  printf("SSL_set_fd = %d\n",type);
  printf("SSL_connect = %d %d\n",type, SSL_get_error(ZenSSL, type));
#endif

  strcpy(Content,"loginName=opsdog&password=");
  ContentLength = strlen(Content);
  sprintf(ContentLengthstr,"%d",ContentLength);
#ifdef DEBUG
  printf("++  Content (%d): %s\n",ContentLength, Content);
#endif

  /*  strcpy(Header,"POST /zf/api/zfapi.asmx/AuthenticatePlain HTTP/1.1\n");*/
  strcpy(Header,"POST /api/1.7/zfapi.asmx/AuthenticatePlain HTTP/1.1\n");
  /*  strcat(Header,"Host: www.zenfolio.com\n");*/
  strcat(Header,"Host: api.zenfolio.com\n");
  strcat(Header,"Content-Type: application/x-www-form-urlencoded\n");
  strcat(Header,"User-Agent:  Daemony Database Processor\n");
  strcat(Header,"Content-Length: ");
  strcat(Header,ContentLengthstr);
  strcat(Header,"\n\n");
  HeaderLength = strlen(Header);
#ifdef DEBUG
  printf("++  Header (%d): %s\n",HeaderLength, Header);
#endif

  /*   works for non-SSL
  send(SocketFD, Header, HeaderLength, 0);
  send(SocketFD, Content, ContentLength, 0);
  */

#ifdef DEBUG
  printf("\n\n====== Start Full Message ======\n");
  printf("%s",Header);
  printf("%s",Content);
  printf("====== End Full Message ======\n\n");
#endif

  type =  SSL_write(ZenSSL, Header, HeaderLength);
#ifdef DEBUG
  printf("Header write = %d\n",type);
#endif
  type = SSL_write(ZenSSL, Content, ContentLength);
#ifdef DEBUG
  printf("Content write = %d\n",type);
#endif

  /*  get the reply */

  /*  works for non-SSL
  if((numbytes = recv(SocketFD, buf, MAXDATASIZE-1, 0)) == -1) {
    perror("recv()");
    exit(1);
    }
#ifdef DEBUG
    else
      printf("Client-The recv() is OK...\n");
#endif

  buf[numbytes] = '\0';
  printf("\n\n====== Response ======\n%s\n", buf);
  */

  type = SSL_read(ZenSSL, buf, 4096);
#ifdef DEBUG
  printf("SSL_read = %d\n",type);
  printf("buf len  = %d\n",strlen(buf));
  printf("\n====== Response ======\n");
#endif

  printf("%s\n",buf);

  type = SSL_pending(ZenSSL);
#ifdef DEBUG
  printf("======================\nPending: %d\n",type);
#endif

  /* close up SSL stuff */

  SSL_shutdown(ZenSSL);
  SSL_free(ZenSSL);
  SSL_CTX_free(ZenCTX);

  /* close up socket stuff */

#ifdef DEBUG
  printf("Client-Closing sockfd\n");
#endif
  close(SocketFD);


}
