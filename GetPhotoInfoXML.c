/*

  program to get the information about a specific photo

*/

/*#undef DEBUG*/

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

int main(int argc, char *argv[]) {
  int ZenSocket, type, SocketFD, numbytes;
  int RemotePort = 443;
  int NumBytes = 0;
  u_short portbase = 0;
  char buf[MAXRECEIVESIZE];
  /*  char RemoteHost[25] = "www.zenfolio.com";*/
  char RemoteHost[25] = "api.zenfolio.com";

  char Header[MAXHEADERSIZE];
  char Content[MAXDATASIZE];
  int HeaderLength, ContentLength;
  char ContentLengthstr[5];
  char AuthToken[256];

  struct sockaddr_in LocalAddr, RemoteAddr;
  struct servent *ZenService;
  struct hostent *ZenHost;
  struct protoent *ZenProtocol;

  SSL_CTX *ZenCTX;
  SSL *ZenSSL;

  /* arg checking */

  if ( argc != 3) {
    printf("Need an AuthToken and photoID\n");
    return(1);
  }

  strcpy(AuthToken,argv[1]);

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


  /**********************************

    socket is open.  prepare the data

   **********************************/

  strcpy(Content,"photoID=");
  strcat(Content,argv[2]);
  strcat(Content,"\n");
  ContentLength = strlen(Content);
  sprintf(ContentLengthstr,"%d",ContentLength);
#ifdef DEBUG
  printf("++  Content (%d): %s\n",ContentLength, Content);
#endif


  /*  strcpy(Header,"POST /zf/api/zfapi.asmx/LoadPhoto HTTP/1.1\n");*/
  strcpy(Header,"POST /api/1.8/zfapi.asmx/LoadPhoto HTTP/1.1\n");
  /*  strcat(Header,"Host: www.zenfolio.com\n");*/
  strcat(Header,"Host: api.zenfolio.com\n");
  strcat(Header,"Content-Type: application/x-www-form-urlencoded\n");
  strcat(Header,"User-Agent:  Daemony Database Processor\n");
  strcat(Header,"X-Zenfolio-User-Agent: Daemony Database Processor\n");
  strcat(Header,"Content-Length: ");
  strcat(Header,ContentLengthstr);
  strcat(Header,"\nX-Zenfolio-Token: ");
  strcat(Header,argv[1]);
  strcat(Header,"\n\n");
  HeaderLength = strlen(Header);
#ifdef DEBUG
  printf("++  Header (%d): %s\n",HeaderLength, Header);
#endif

  /*  content and header is ready - send it off */

  type =  SSL_write(ZenSSL, Header, HeaderLength);
#ifdef DEBUG
  printf("Header write = %d\n",type);
#endif
  type = SSL_write(ZenSSL, Content, ContentLength);
#ifdef DEBUG
  printf("Content write = %d\n",type);
#endif

  /*  get the reply */

  NumBytes = SSL_pending(ZenSSL);

#ifdef DEBUG
  printf("%d bytes of reply pending\n");
#endif

  type = SSL_read(ZenSSL, buf, 4096);
#ifdef DEBUG
  printf("SSL_read = %d\n",type);
  printf("\n\n====== Response ======\n");
#endif

  printf("%s\n",buf);

  type = SSL_pending(ZenSSL);
#ifdef DEBUG
  printf("======================\nPending: %d\n",type);
#endif




  /**********************************

    we're done - clean up and go home

   **********************************/

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
