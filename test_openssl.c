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

#include <stdio.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

int main(int argc, char *argv[])
{
  int ZenSocket, type;
  u_short portbase = 0;

  struct sockaddr_in ZenAddr;
  struct servent *ZenServ;
  struct hostent *ZenHost;
  struct protoent *ZenProto;

  SSL_CTX *ZenCTX;
  SSL *ZenSSL;

  /* open the socket */

  memset(&ZenAddr, 0, sizeof(ZenAddr));
  ZenAddr.sin_family=AF_INET;
  ZenAddr.sin_addr.s_addr=INADDR_ANY;

  /* map service name to a port number */

  if (ZenServ = getservbyname("https","tcp"))
    ZenAddr.sin_port = htons(ntohs((u_short)ZenServ->s_port) + portbase);
  else if((ZenAddr.sin_port = htons((u_short)atoi("https"))) == 0) {
    printf("can't get \"%s\" service entry\n", "https");
    exit(0);
  }

  /* map protocol name to protocol number */

  if((ZenProto = getprotobyname("TCP")) == 0) {
    printf("can't get \"%s\" protocol entry\n", "TCP");
    exit(0);
  }

  /* use protocol to choose a socket type */

  if(strcmp("TCP", "udp") == 0)
    type = SOCK_DGRAM;
  else
    type = SOCK_STREAM;

  /* allocate a socket */

  ZenSocket = socket(PF_INET,type,ZenProto->p_proto);
  if(ZenSocket < 0) {
    printf("can't create socket: %s\n", strerror(errno));
    exit(0);
  }

  /* bind the socket */
  /*
  if(bind(ZenSocket, (struct sockaddr *)&ZenAddr, sizeof(ZenAddr)) < 0) {
    printf("can't bind to %s port: %s\n", "https", strerror(errno));
    exit(0);
  }
  */

  if (connect(ZenSocket,(struct sockaddr *)&ZenAddr,sizeof(ZenAddr)) < 0) {
    printf("error on connect %d\n",errno);
    exit(0);
  }

  printf("Socket %d\n",ZenSocket);

  /*
     pretend we know what we're doing with SSL
  */

  SSL_library_init();
  SSL_load_error_strings();

  ZenCTX = SSL_CTX_new(SSLv23_client_method());
  printf("CTX %d\n",(int)ZenCTX);

  ZenSSL = SSL_new(ZenCTX);
  printf("SSL %d\n",(int)ZenSSL);
  /*  sets to client mode */
  SSL_set_connect_state(ZenSSL);  




  SSL_free(ZenSSL);
  SSL_CTX_free(ZenCTX);


  /* close up socket stuff */

  endhostent();


}


