/*

  program to get the information about a specific photo

*/

/*
#undef DEBUG
#define DEBUG
*/

/*
#undef DATADUMP
#define DATADUMP
*/

#include <stdio.h>
#include <strings.h>

#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#define MAXDATASIZE 4096
#define MAXHEADERSIZE 1024
#define MAXRECEIVESIZE  8196
/*  #define MAXRECEIVESIZE 16384  */
/*  #define MAXRECEIVESIZE 65536  */


int main(int argc, char *argv[]) {
  int ZenSocket, type, ZenSD, numbytes;
  int RemotePort = 443;
  int NumBytes = 0;
  int BytesRead = 0;
  u_short portbase = 0;
  char buf[MAXRECEIVESIZE];
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

  FILE *tempfile;
  FILE *xmlfile;
  int Tempfile, XMLfile;
  char ch;

  /* arg checking */

  if ( argc != 3) {
    printf("Need an AuthToken and a PhotoSet\n");
    return(1);
  }

#ifdef DEBUG
  printf("\nGetPhotoSetXML:  ENTRY\n");
#endif

  /*  open the temp file to dump the reply into */

  tempfile = fopen("/tmp/dougee", "w");

  strcpy(AuthToken,argv[1]);

  /********************************************************************/
  /*                                                                  */
  /*                        initialize SSL                            */
  /*                                                                  */
  /********************************************************************/

  SSL_library_init();
#ifdef DEBUG
  printf("dougee01\n");
#endif

  OpenSSL_add_all_algorithms();
#ifdef DEBUG
  printf("dougee02\n");
#endif

  SSL_load_error_strings();
#ifdef DEBUG
  printf("dougee03\n");
#endif

  ZenCTX = SSL_CTX_new(SSLv23_client_method());
#ifdef DEBUG
  printf("dougee04\n");
#endif

  if ( ZenCTX == NULL ) {
    ERR_print_errors_fp(stderr);
    abort();
  }
#ifdef DEBUG
  printf("dougee05\n");
#endif


  /********************************************************************/
  /*                                                                  */
  /*                  open the unsecured socket                       */
  /*                                                                  */
  /********************************************************************/

  /*  open the connection  */

  if ( (ZenHost = gethostbyname(RemoteHost)) == NULL )
    {
      perror(RemoteHost);
      abort();
    }
#ifdef DEBUG
  printf("dougee06 - sizeof(h_addr_list) is %d\n", sizeof(ZenHost->h_addr_list) );
  printf("dougee06 - %d %d %d %d %d %d %d %d\n",
	 ZenHost->h_addr_list[0], ZenHost->h_addr_list[1], ZenHost->h_addr_list[2],
	 ZenHost->h_addr_list[3], ZenHost->h_addr_list[4], ZenHost->h_addr_list[5],
	 ZenHost->h_addr_list[6], ZenHost->h_addr_list[7] );
  /*  printf("dougee06 - host is %s\n", ZenHost->h_addr_list[0]);*/
  printf("dougee06 - addr type is %d\n", ZenHost->h_addrtype);
  printf("dougee06 - addr length is %d\n", ZenHost->h_length);
#endif
  ZenSD = socket(AF_INET, SOCK_STREAM, 0);
#ifdef DEBUG
  printf("dougee07 - socket FD %d\n", ZenSD);
#endif
  bzero(&RemoteAddr, sizeof(RemoteAddr));
#ifdef DEBUG
  printf("dougee71 - AF_NET is %d\n", AF_INET);
#endif
  RemoteAddr.sin_family = AF_INET;
#ifdef DEBUG
  printf("dougee72\n");
#endif
  RemoteAddr.sin_port = htons(RemotePort);
#ifdef DEBUG
  printf("dougee73\n");
#endif
  RemoteAddr.sin_addr.s_addr = *(long*)(ZenHost->h_addr);
#ifdef DEBUG
  printf("dougee08 - %d\n", ZenHost->h_addr);
#endif

  if ( connect(ZenSD, (struct sockaddr *)&RemoteAddr, sizeof(RemoteAddr)) != 0 )
    {
      close(ZenSD);
      perror(RemoteHost);
      printf("Exiting due to failed connect\n");
      exit(1);
    }
#ifdef DEBUG
  printf("dougee09\n");
#endif

  ioctl(ZenSD, FIONBIO);

  int keepalive_enabled = 1;
  int keepalive_time = 180000;
  
  setsockopt(ZenSD, SOL_SOCKET, SO_KEEPALIVE, (const void *)keepalive_enabled, (socklen_t)sizeof(keepalive_enabled));
  setsockopt(ZenSD, IPPROTO_TCP, TCP_KEEPALIVE, (const void *)keepalive_time, (socklen_t)sizeof keepalive_time);


  ZenSSL = SSL_new(ZenCTX);
#ifdef DEBUG
  printf("dougee10\n");
#endif

  SSL_set_mode(ZenSSL, SSL_MODE_AUTO_RETRY);
  SSL_set_connect_state(ZenSSL);


  SSL_set_fd(ZenSSL, ZenSD);
#ifdef DEBUG
  printf("dougee11\n");
#endif

  if ( SSL_connect(ZenSSL) == -1 ) {
    ERR_print_errors_fp(stderr);
    abort();
  }

#ifdef DEBUG
  printf("Connected with %s encryption\n", SSL_get_cipher(ZenSSL));
  printf("Connected with %s encryption\n", SSL_get_cipher_version(ZenSSL));
#endif


  /********************************************************************/
  /*                                                                  */
  /*            connection is open.  prepare the data                 */
  /*                                                                  */
  /********************************************************************/

  strcpy(Content,"photosetId=");
  strcat(Content,argv[2]);

  strcat(Content,"&level=Full");
  /*strcat(Content,"&includePhotos=false");*/
  strcat(Content,"&includePhotos=true");

  ContentLength = strlen(Content);
  sprintf(ContentLengthstr,"%d",ContentLength);
#ifdef DEBUG
  printf("++  Content (%d): %s\n",ContentLength, Content);
#endif


  /*  strcpy(Header,"POST /zf/api/zfapi.asmx/LoadPhotoSet HTTP/1.1\n");*/
  strcpy(Header,"POST /api/1.8/zfapi.asmx/LoadPhotoSet HTTP/1.1\n");
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
  printf("Header written = %d\n",type);
#endif
  type = SSL_write(ZenSSL, Content, ContentLength);
#ifdef DEBUG
  printf("Content written = %d\n",type);
#endif

  /********************************************************************/
  /*                                                                  */
  /*                      set up select queues                        */
  /*                                                                  */
  /********************************************************************/

#ifdef DEBUG
  printf("dougee01\n");
#endif
  struct fd_set ReadSockets;
  struct fd_set WriteSockets;
  struct fd_set ErrorSockets;
  int SelNumFDs;
  int NumFDs;
  struct timeval SelTimeout;

#ifdef DEBUG
  printf("dougee02\n");
#endif
  SelNumFDs = ZenSD + 1;

#ifdef DEBUG
  printf("dougee03 - will check FDs from 0 to %d\n", SelNumFDs);
#endif
  FD_ZERO(&ReadSockets);
  FD_ZERO(&WriteSockets);
  FD_ZERO(&ErrorSockets);

#ifdef DEBUG
  printf("dougee04\n");
#endif
  FD_SET(ZenSD, &ReadSockets);
  FD_SET(ZenSD, &WriteSockets);
  FD_SET(ZenSD, &ErrorSockets);

#ifdef DEBUG
  printf("dougee05\n");
#endif
  SelTimeout.tv_sec = 30;
  SelTimeout.tv_usec = 0;

#ifdef DEBUG
  printf("dougee06\n");
#endif
  NumFDs = select(SelNumFDs, &ReadSockets, &WriteSockets, &ErrorSockets, &SelTimeout);
#ifdef DEBUG
  printf("dougee07 - %d\n", NumFDs);
  printf("select says %d FDs are ready for something\n", NumFDs);
  printf("FD_ISSET (read)  returns %d\n", FD_ISSET(ZenSD, &ReadSockets));
  printf("FD_ISSET (error) returns %d\n", FD_ISSET(ZenSD, &ErrorSockets));
#endif

  for (;;) {

#ifdef DEBUG
    printf("\n====== TOP of for loop\n");
#endif

    BytesRead = SSL_read(ZenSSL, buf, MAXRECEIVESIZE);

#ifdef DEBUG
    printf("  %d bytes read\n", BytesRead);
#endif

    if ( BytesRead <= 0 ) 
      break;

    if ( BytesRead == 0 ) {
      /* Handle closed connection */
      printf("SSL connection closed (1) - bailing...\n");
      SSL_CTX_free(ZenCTX);
      fclose(tempfile);
      exit(8);
    }
    else 
      if ( BytesRead < 0 ) {
	printf("%d error (1) BytesRead\n", BytesRead);
	exit(9);
      }


#ifdef DEBUG
    printf("  === SSL_read = %d ===\n",BytesRead);
#endif

#ifdef DATADUMP
    printf("  Writing data to file:\n");
#endif

    for (type=0;type<BytesRead;type++) {
      fprintf(tempfile,"%c",buf[type]);
#ifdef DATADUMP
      printf("%c",buf[type]);
      fflush(stdout);
#endif
    }  /*  write datafile loop  */

    FD_ZERO(&ReadSockets);
    FD_ZERO(&WriteSockets);
    FD_ZERO(&ErrorSockets);
    FD_SET(ZenSD, &ReadSockets);
    FD_SET(ZenSD, &ErrorSockets);
    NumFDs = select(SelNumFDs, &ReadSockets, &WriteSockets, &ErrorSockets, &SelTimeout);

    if ( FD_ISSET(ZenSD, &ReadSockets) == 0 ) {
#ifdef DEBUG
      printf("\nSelect returned nothing ready to read...\n");
      printf("select says %d FDs are ready for something\n", NumFDs);
      printf("FD_ISSET (read)  returns %d\n", FD_ISSET(ZenSD, &ReadSockets));
      printf("FD_ISSET (error) returns %d\n", FD_ISSET(ZenSD, &ErrorSockets));
#endif

      /*      printf("\n\nTrying again, just to be sure...\n");*/
      /*      FD_ZERO(&ReadSockets);*/
      /*      FD_ZERO(&WriteSockets);*/
      /*      FD_ZERO(&ErrorSockets);*/
      /*      FD_SET(ZenSD, &ReadSockets);*/
      /*      FD_SET(ZenSD, &ErrorSockets);*/
      /*      NumFDs = select(SelNumFDs, &ReadSockets, &WriteSockets, &ErrorSockets, &SelTimeout);*/
      /*#ifdef DEBUG*/
      /*      printf("select says %d FDs are ready for something\n", NumFDs);*/
      /*      printf("FD_ISSET (read)  returns %d\n", FD_ISSET(ZenSD, &ReadSockets));*/
      /*      printf("FD_ISSET (error) returns %d\n", FD_ISSET(ZenSD, &ErrorSockets));*/
      /*#endif*/


      /*      if ( FD_ISSET(ZenSD, &ReadSockets) == 0 ) {*/
      break;
      /*      }*/
    }

#ifdef DEBUG
    printf("====== BOTTOM of for loop ======\n");
    type = SSL_pending(ZenSSL);
    printf("  %d SSL bytes pending\n", type);
    printf("  select says %d FDs are ready for something\n", NumFDs);
    printf("  FD_ISSET (read)  returns %d\n", FD_ISSET(ZenSD, &ReadSockets));
    printf("  FD_ISSET (error) returns %d\n", FD_ISSET(ZenSD, &ErrorSockets));
#endif

  }  /*  for loop to read packets  */


  /*  close the temp file */

  fclose(tempfile);

  /*  remove the header from the temp file and create the final XML file */

  Tempfile = open("/tmp/dougee", O_RDONLY | O_CREAT);
  XMLfile = open("XML-PhotoSet.xml", O_WRONLY | O_CREAT);

#ifdef DEBUG
  printf("Tempfile, XMLfile: %d %d\n",Tempfile, XMLfile);
#endif

  while( read(Tempfile, &ch, 1) ) {

    if ( ch == '<' ) {
      write(XMLfile, &ch, 1);
      while( read(Tempfile, &ch, 1) ) 
	write(XMLfile, &ch, 1);
    }

  }  /*  while reading the temp file */


  close(Tempfile);
  close(XMLfile);


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
  close(ZenSD);

#ifdef DEBUG
  printf("GetPhotoSetXML:  EXIT\n");
#endif

}
