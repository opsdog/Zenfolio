/*

  Loads the Photo information by Gallery (PhotoSet)

  this is a multi-step process to get the full information for each photo

  uses LoadPhotoSetPhotos to get the PhotoId for each photo in each gallery
  then uses LoadPhoto to get the full Photo structure for each Photo

*/

/*
#undef DEBUG
#define DEBUG
*/


#undef DBDEBUG
#define DBDEBUG


/*
#undef SHOWFOUND
#define SHOWFOUND
*/

/*
#undef MEMDEBUG
#define MEMDEBUG
*/

/*
#undef DUMPDATA
#define DUMPDATA
*/

/*
#undef DOUNLINK
*/

#include <stdio.h>
#include <strings.h>

#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <my_global.h>
#include <mysql.h>
#include <my_sys.h>


#include "ZenFolio.h"
#include "LoadPhotosByGallery.h"

#ifdef LIBXML_TREE_ENABLED

int main(int argc, char *argv[]) {

  /*  general vars */

  int i;
  int BytesRead = 0;
  int NumBytes = 0;
  char ch;

  /*  ZenFolio vars */

  int HeaderLength, ContentLength;
  char AuthToken[256];
  char buf[MAXRECEIVESIZE];
  char Header[MAXHEADERSIZE];
  char Content[MAXDATASIZE];
  char ContentLengthstr[5];
  /*  char RemoteHost[25] = "www.zenfolio.com";*/
  char RemoteHost[25] = "api.zenfolio.com";

  /*  socket vars */

  int RemotePort = 443;
  int SocketFD, type;
  struct hostent *ZenHost;
  struct sockaddr_in RemoteAddr;
  SSL_CTX *ZenCTX;
  SSL *ZenSSL;

  /*  XML vars */

  int Tempfile, XMLfile;
  xmlDoc *PSdoc,
    *Pdoc;
  xmlDocPtr PSdoc_ptr,
    Pdoc_ptr;
  xmlChar *key;
  xmlNodePtr PScur_ptr = NULL,
    ADcur_ptr = NULL,
    Pcur_ptr = NULL;
  xmlNode 
    *PScur_node = NULL,
    *Pcur_node = NULL,
    *PSroot_node = NULL,
    *Proot_node = NULL;

  /*  DB vars  */

  FILE *DBvip;
  char DBLocation[20];
  char DBHost[30];
  char DBUser[30];
  char DBPassword[30];
  char DBName[50];
  char Command[2048];
  int CommandLength;

  MYSQL QueryDB;
  MYSQL_RES *QueryResult;
  MYSQL_ROW QueryRow;


  /*  program specific */

  int DBCommandLength;
  char PSTempfileName[50] = "/tmp/XMLish-PhotoSet",
    PTempfileName[50] = "/tmp/XMLish-Photo",
    DBCommand[2048],
    PhotoIdString[50],
    StringVal[256];
  FILE *PStempfile,
    *Ptempfile;

  xmlNode *ElementsNode = NULL,
    *ArrayOfPhotoNode = NULL,
    *ADNode = NULL,
    *KWNode = NULL,
    *CatNode = NULL,
    *ETNode = NULL,
    *PhotoNode = NULL;

  /*
    structure of Photo information
  */

  struct photo *Photo = NULL;
  struct photo *Photo_Top = NULL;
  struct photo *Photo_Bot = NULL;
  struct photo *Photo_Current = NULL;
  struct photo *Photo_Last = NULL;
  struct photo *Photo_Temp = NULL;

  struct photo *PhotoId_Top = NULL;
  struct photo *PhotoId_Current = NULL;

  struct photo *PhotoInDB = NULL;
  struct photo *PhotoInDB_Top = NULL;
  struct photo *PhotoInDB_Current = NULL;

  /*
    structure of PhotoFlags
  */

  struct pflags *PF = NULL;
  struct pflags *PF_Top = NULL;
  struct pflags *PF_Bot = NULL;
  struct pflags *PF_Current = NULL;
  struct pflags *PF_Last = NULL;
  struct pflags *PF_Temp = NULL;

  struct pflags *PFInDB = NULL;
  struct pflags *PFInDB_Top = NULL;
  struct pflags *PFInDB_Current = NULL;

  /*
    structure of Photo Categories
  */

  struct pcats *Cat = NULL;
  struct pcats *Cat_Top = NULL;
  struct pcats *Cat_Temp = NULL;
  struct pcats *Cat_Current = NULL;

  /*
    structure of Photo Keywords
  */

  struct pkeys *Key = NULL;
  struct pkeys *Key_Top = NULL;
  struct pkeys *Key_Temp = NULL;
  struct pkeys *Key_Current = NULL;

  /*
    structure of Photo ExifTags
  */

  struct petags *ETags = NULL;
  struct petags *ETags_Top = NULL;
  struct petags *ETags_Temp = NULL;
  struct petags *ETags_Current = NULL;


  /*
    bit string of 0's to set PhotoFlags elements to all false
  */

  char *strPFfalse;

  strPFfalse=(char *)malloc( (sizeof(struct pflags) + 1) );
  for ( i = 0 ; i < sizeof(struct pflags) ; i++ )
    strPFfalse[i] = 0;

#ifdef MEMDEBUG
  printf("MEM:  PhotoFlags zero string: %d %d\n",i,sizeof(struct pflags));
#endif

  strPFfalse[i]='\0';

  /* arg checking */

  if ( argc != 4) {
    printf("Usage:  LoadPhotosByGallery AuthToken GallID PhotoCount\n");
    return(1);
  }

  /*  open the temp file to dump the reply into */

  PStempfile = fopen(PSTempfileName, "w");

  strcpy(AuthToken,argv[1]);

  /* open the socket */

  printf("    Getting PhotoSetPhotos XML file...\n");

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
  printf("CTX %d\n",(int *)ZenCTX);
  printf("SSL %d\n",(int *)ZenSSL);
  printf("SSL_set_fd = %d\n",type);
  printf("SSL_connect = %d %d\n",type, SSL_get_error(ZenSSL, type));
#endif


  /* *********************************

    socket is open.  prepare the data

   ********************************* */

  strcpy(Content,"photoSetId=");
  strcat(Content,argv[2]);
  strcat(Content,"&startingIndex=0&numberOfPhotos=");
  strcat(Content,argv[3]);


  ContentLength = strlen(Content);
  sprintf(ContentLengthstr,"%d",ContentLength);
#ifdef DEBUG
  printf("++  Content (%d): %s\n",ContentLength, Content);
#endif

  strcpy(Header,"POST /api/1.6/zfapi.asmx/LoadPhotoSetPhotos HTTP/1.1\n");
  /*  strcat(Header,"Host: www.zenfolio.com\n");*/
  strcat(Header,"Host: api.zenfolio.com\n");
  strcat(Header,"Content-Type: application/x-www-form-urlencoded\n");
  /*  strcat(Header,"Content-Type: text/xml; charset=utf-8\n");*/
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

  BytesRead = SSL_read(ZenSSL, buf, MAXRECEIVESIZE);
#ifdef DEBUG
  printf("SSL_read = %d\n",BytesRead);
  printf("\n\n====== Response ======\n");
#endif

  while (BytesRead > 0) {
#ifdef DEBUG
    printf("\n\n=======> top of while\n\n");
#endif
    for (type=0;type<BytesRead;type++)
      fprintf(PStempfile,"%c",buf[type]);
    /*      printf("%c",buf[type]);*/
    /*    if ( BytesRead > 16000 )*/
      BytesRead = SSL_read(ZenSSL, buf, MAXRECEIVESIZE);
      /*    else*/
      /*      BytesRead=0;*/
#ifdef DEBUG
    printf("\n\n========> bottom of while, BytesRead %d\n\n",BytesRead);
    if ( BytesRead == -1 )
      printf("ERROR: %d %d\n",BytesRead,SSL_get_error(ZenSSL,BytesRead));
#endif
  }

  NumBytes = SSL_pending(ZenSSL);

#ifdef DEBUG
  fprintf(stderr,"%d bytes of reply pending\n", NumBytes);
#endif

  type = SSL_pending(ZenSSL);
#ifdef DEBUG
  printf("======================\nPending: %d\n",type);
#endif

#ifdef DEBUG
  printf("Total bytes read %d\n",BytesRead);
#endif

  /*  close the temp file */

  fclose(PStempfile);

  /*  remove the header from the temp file and create the final XML file */

  Tempfile = open(PSTempfileName, O_RDONLY | O_CREAT);
  if ( fchmod(Tempfile, 0000600) == -1 ) {
    printf("Tempfile chmod failed --> %d\n",errno);
  }
  XMLfile = open("XML-PhotoSet.xml", O_WRONLY | O_CREAT);
  if ( fchmod(XMLfile, 0000644) == -1 ) {
    printf("XMLfile chmod failed --> %d\n",errno);
  }

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

#ifdef DOUNLINK
  unlink(PSTempfileName);
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

  /*

    we now have the partial information of Photos in XML-PhotoSet.xml

    extract the PhotoId for each photo - add to memory structure

    for each PhotoId in memory
      use LoadPhoto to get full Photo structure
      parse Photo structure into memory structures
    validate all pointers are referenced
    load Photos into database

  */

  /*  initialize the library and check version compatability */

  LIBXML_TEST_VERSION ;

  /* parse the file */

  PSdoc = xmlReadFile("XML-PhotoSet.xml", NULL, 0);
  if ( PSdoc == NULL ) {
    printf("could not parse file %s\n",argv[1]);
    return(1);
  }

#ifdef DEBUG
  printf("\n\nXML file open - let's roll...\n");
#endif

  PSdoc_ptr = PSdoc;

  /*  get the root element node */

  PSroot_node = xmlDocGetRootElement(PSdoc);
  PScur_node = PSroot_node;

  /*
    first level element is ArrayOfPhoto1 and is the outer container
    Elements first level item holds the Photos
  */

  /*  PScur_node better be ArrayOfPhoto1 - dive in... */

#ifdef DEBUG
  printf("Should be ArrayOfPhotos1: %s\n",PScur_node->name);
#endif

#ifdef DEBUG
  printf("\nprinting PhotoSet first level elements:\n");
  while ( PScur_node != NULL ) {
    printf("  Found: %s\n",PScur_node->name);
    PScur_node = PScur_node->next;
  }  /*  while have nodes  */
#endif

#ifdef DEBUG
  printf("\nprinting PhotoSet second level elements:\n");
  PScur_node = PSroot_node->children;
  while ( PScur_node != NULL ) {
    printf("    Found: %s\n",PScur_node->name);
    PScur_node = PScur_node->next;
  }  /*  while have nodes  */
#endif

#ifdef DEBUG
  printf("\nprinting PhotoSet third level Photo elements:\n");
  PScur_node = PSroot_node->children;
  while ( PScur_node != NULL ) {
    if ( strcmp((char *)PScur_node->name,"Photo") == 0 ) {
      ElementsNode = PScur_node->children;
      while ( ElementsNode != NULL ) {
	printf("      Found: %s\n",ElementsNode->name);
	ElementsNode = ElementsNode->next;
      }  /*  while parsing third level  */
    }  /*  if found second level Photos  */
    PScur_node = PScur_node->next;
  }  /*  while parsing second level  */
#endif

  /*
    parse the XML file
  */

  printf("    Parsing PhotoSetPhotos XML file...\n");

  PScur_node = PSroot_node;
  while ( PScur_node != NULL ) {

    if ( strcmp((char *)PScur_node->name,"ArrayOfPhoto1") == 0 ) {
#ifdef DEBUG
      printf("ArrayOfPhoto1\n");
#endif

      ArrayOfPhotoNode = PScur_node->children;
      while ( ArrayOfPhotoNode != NULL ) {
#ifdef SHOWFOUND
	printf("  Found:  %s\n",ArrayOfPhotoNode->name);
#endif

	if ( strcmp((char *)ArrayOfPhotoNode->name,"Photo") == 0 ) {
#ifdef DEBUG
	  printf("  Photo\n");
#endif
	  Photo_Temp = (struct photo *)malloc(sizeof(struct photo));
	  Photo_Temp->ph_Next = NULL;
	  Photo_Temp->ph_Sequence[0]='\0';

	  PhotoNode = ArrayOfPhotoNode->children;
	  while (PhotoNode != NULL ) {
#ifdef SHOWFOUND
	    printf("    Found:  %s\n",PhotoNode->name);
#endif

	    if ( strcmp((char *)PhotoNode->name,"Id") == 0 ) {
	      PScur_ptr = PhotoNode;
	      key = xmlNodeListGetString(PSdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	      printf("    Id %s\n",(char *)key);
#endif
	      sscanf((char *)key,"%d",&Photo_Temp->ph_ID);
	    }  /*  found a Photo Id  */


	    PhotoNode = PhotoNode->next;
	  }  /*  while parsing a Photo's children  */

	  Photo_Top = AddPhotoId(Photo_Top, Photo_Temp);
	  free(Photo_Temp);

	}  /*  if current element is Photo */

	ArrayOfPhotoNode = ArrayOfPhotoNode->next;
      }  /*  while parsing the ArrayOfPhotoNode's children */
    }  /*  if in ArrayOfPhoto1 */

    PScur_node = PScur_node->next;
  }  /*  while parsing first level elements  */


  /*  clean up XML stuff  */

  xmlFreeDoc(PSdoc);
  xmlCleanupParser();

#ifdef DEBUG
  printf("\n");
#endif

#ifdef DOUNLINK
  unlink("XML-PhotoSet.xml");
#endif


  /*
    this block of code dumps the built data structures
  */

#ifdef DUMPDATA
  printf("\n\nPhotos:\n");
  Photo_Current = Photo_Top;
  while (Photo_Current != NULL ) {
    /*
    printf("  %15d %dx%d\n",Photo_Current->ph_ID, 
	   Photo_Current->ph_Width, Photo_Current->ph_Height);
    */
    printf("  %15d\n",Photo_Current->ph_ID);
    Photo_Current = Photo_Current->ph_Next;
  }  /*  while printing Photos  */
#endif

  /*
    now we have the PhotoIds in memory

    pull the full Photo information
    parse into memory
    load the database
  */

  printf("    Getting full Photo information...\n");

  PhotoId_Top = Photo_Top;
  Photo_Top = NULL;

  PhotoId_Current = PhotoId_Top;
  while ( PhotoId_Current != NULL ) {
    printf("      %d\n",PhotoId_Current->ph_ID);
    printf("        Getting Photo XML...\n");

    /*  call BuildContent  */

    ContentLength=BC_LoadPhoto(PhotoId_Current->ph_ID, "Full", Content);

#ifdef DEBUG
    printf("    Content (%d):  %s\n",ContentLength,Content);
#endif

    /*  call BuildHeader  */

    HeaderLength=BuildZFHeader(argv[1], ContentLength, Header);

#ifdef DEBUG
    printf("    Header (%d):  %s\n",HeaderLength,Header);
#endif

    /*  open the socket and set up ssl */

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
    printf("CTX %d\n",(int *)ZenCTX);
    printf("SSL %d\n",(int *)ZenSSL);
    printf("SSL_set_fd = %d\n",type);
    printf("SSL_connect = %d %d\n",type, SSL_get_error(ZenSSL, type));
#endif

    /*  socket is open - process the photo  */

    /*  send request - get XML with headers  */

    Ptempfile = fopen(PTempfileName, "w");

    type =  SSL_write(ZenSSL, Header, HeaderLength);
#ifdef DEBUG
    printf("\nHeader write = %d\n",type);
#endif
    type = SSL_write(ZenSSL, Content, ContentLength);
#ifdef DEBUG
    printf("Content write = %d\n",type);
#endif

    /*  get the reply */

    BytesRead = SSL_read(ZenSSL, buf, MAXRECEIVESIZE);
#ifdef DEBUG
    printf("SSL_read = %d\n",BytesRead);
    printf("\n====== Response ======\n");
#endif

    while (BytesRead > 0) {
#ifdef DEBUG
      printf("  top of while\n");
#endif
      for (type=0;type<BytesRead;type++)
	fprintf(PStempfile,"%c",buf[type]);
      /*      printf("%c",buf[type]);*/
      /*    if ( BytesRead > 16000 )*/
      BytesRead = SSL_read(ZenSSL, buf, MAXRECEIVESIZE);
      /*    else*/
      /*      BytesRead=0;*/
#ifdef DEBUG
      printf("  bottom of while, BytesRead %d\n",BytesRead);
      if ( BytesRead == -1 )
	printf("ERROR: %d %d\n",BytesRead,SSL_get_error(ZenSSL,BytesRead));
#endif
    }

    NumBytes = SSL_pending(ZenSSL);

#ifdef DEBUG
    fprintf(stderr,"  %d bytes of reply pending\n", NumBytes);
#endif

    type = SSL_pending(ZenSSL);
#ifdef DEBUG
    printf("======================\nPending: %d\n",type);
#endif

    /*  close the temp file */

    fclose(Ptempfile);

    /*  shut down ssl and close the socket  */

    SSL_shutdown(ZenSSL);
    SSL_free(ZenSSL);
    SSL_CTX_free(ZenCTX);

    /* close up socket stuff */

#ifdef DEBUG
    printf("\nClient-Closing sockfd\n");
#endif
    close(SocketFD);

    /*  remove headers  */

    Tempfile = open(PTempfileName, O_RDONLY | O_CREAT);
    if ( fchmod(Tempfile, 0000600) == -1 ) {
      printf("Tempfile chmod failed --> %d\n",errno);
    }
    XMLfile = open("XML-Photo.xml", O_WRONLY | O_CREAT);
    if ( fchmod(XMLfile, 0000644) == -1 ) {
      printf("XMLfile chmod failed --> %d\n",errno);
    }

#ifdef DEBUG
    printf("\nRemoving header:  Tempfile, XMLfile: %d %d\n",Tempfile, XMLfile);
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

#ifdef DOUNLINK
    unlink(PTempfileName);
#endif

    /*  parse XML into memory  */

    Pdoc = xmlReadFile("XML-Photo.xml", NULL, 0);
    if ( Pdoc == NULL ) {
      printf("could not parse file XML-Photo.xml\n");
      return(1);
    }

#ifdef DEBUG
    printf("\n\nXML file open - let's roll...\n");
#endif

    Pdoc_ptr = Pdoc;

    /*  get the root element node */

    Proot_node = xmlDocGetRootElement(Pdoc);
    Pcur_node = Proot_node;

    /*
      first level element is ArrayOfPhoto1 and is the outer container
      Elements first level item holds the Photos
    */

    /*  Pcur_node better be Photo - dive in... */

#ifdef DEBUG
    printf("Should be Photo: %s\n",Pcur_node->name);
#endif

#ifdef DUMPDATA
    printf("\nprinting Photo first level elements:\n");
    while ( Pcur_node != NULL ) {
      printf("  Found: %s\n",Pcur_node->name);
      Pcur_node = Pcur_node->next;
    }  /*  while have nodes  */

    printf("\nprinting Photo second level elements:\n");
    Pcur_node = Proot_node->children;
    while ( Pcur_node != NULL ) {
      printf("    Found: %s\n",Pcur_node->name);
      Pcur_node = Pcur_node->next;
    }  /*  while have nodes  */

    printf("\nprinting Photo third level Photo elements:\n");
    Pcur_node = Proot_node->children;
    while ( Pcur_node != NULL ) {
      if ( strcmp((char *)Pcur_node->name,"Photo") == 0 ) {
	ElementsNode = Pcur_node->children;
	while ( ElementsNode != NULL ) {
	  printf("      Found: %s\n",ElementsNode->name);
	  ElementsNode = ElementsNode->next;
	}  /*  while parsing third level  */
      }  /*  if found second level Photos  */
      Pcur_node = Pcur_node->next;
    }  /*  while parsing second level  */
#endif

    printf("        Parsing Photo XML...\n");

    Pcur_node = Proot_node;
    while ( Pcur_node != NULL ) {

      if ( strcmp((char *)Pcur_node->name,"Photo") == 0 ) {
#ifdef DEBUG
	printf("      entering Photo element...\n");
#endif

	Photo_Temp = (struct photo *)malloc(sizeof(struct photo));
	Photo_Temp->ph_Next = NULL;
	Photo_Temp->ph_Sequence[0] = '\0';
	Photo_Temp->ph_Title[0] = '\0';
	Photo_Temp->ph_Caption[0] = '\0';

	PhotoNode = Pcur_node->children;
	while ( PhotoNode != NULL ) {
#ifdef SHOWFOUND
	  printf("        Found:  %s\n",PhotoNode->name);
#endif

	  if ( strcmp((char *)PhotoNode->name,"Id") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        Id %s\n",(char *)key);
#endif
	    sscanf((char *)key,"%d",&Photo_Temp->ph_ID);
	  }  /*  found a Photo Id  */

	  if ( strcmp((char *)PhotoNode->name,"Width") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        Width %s\n",(char *)key);
#endif
	    sscanf((char *)key,"%d",&Photo_Temp->ph_Width);
	  }  /*  found a Photo Width  */

	  if ( strcmp((char *)PhotoNode->name,"Height") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        Height %s\n",(char *)key);
#endif
	    sscanf((char *)key,"%d",&Photo_Temp->ph_Height);
	  }  /*  found a Photo Height  */

	  if ( strcmp((char *)PhotoNode->name,"Sequence") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        Sequence %s\n",(char *)key);
#endif
	    if ( key != 0 )
	      strcpy(Photo_Temp->ph_Sequence,(char *)key);
	    else
	      Photo_Temp->ph_Sequence[0]='\0';
	  }  /*  found a Photo Sequence  */

	  if ( strcmp((char *)PhotoNode->name,"AccessDescriptor") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        AccessDescriptor\n");
#endif

	    ADNode = PhotoNode->children;
	    while ( ADNode != NULL ) {
#ifdef SHOWFOUND
	      printf("          Found:  %s\n",ADNode->name);
#endif

	      if ( strcmp((char *)ADNode->name,"RealmId") == 0 ) {
		ADcur_ptr = ADNode;
		key = xmlNodeListGetString(Pdoc_ptr, ADcur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		printf("          RealmId %s\n",(char *)key);
#endif
		sscanf((char *)key,"%d",&Photo_Temp->ph_AD);
	      }  /*  found an AccessDescriptor RealmId  */
	      ADNode = ADNode->next;
	    }  /*  while parsing the AccessDescriptor  */

	  }  /*  found a Photo AccessDescriptor  */

	  if ( strcmp((char *)PhotoNode->name,"Caption") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        Caption %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_Caption,(char *)key);
	  }  /*  found a Photo Caption  */

	  if ( strcmp((char *)PhotoNode->name,"FileName") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        FileName %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_FileName,(char *)key);
	  }  /*  found a Photo FileName  */

	  if ( strcmp((char *)PhotoNode->name,"UploadedOn") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        UploadedOn %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_UploadedOn,(char *)key);
	  }  /*  found a Photo UploadedOn  */

	  if ( strcmp((char *)PhotoNode->name,"TakenOn") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        TakenOn %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_TakenOn,(char *)key);
	  }  /*  found a Photo TakenOn  */

	  if ( strcmp((char *)PhotoNode->name,"Owner") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        Owner %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_Owner,(char *)key);
	  }  /*  found a Photo Owner  */

	  if ( strcmp((char *)PhotoNode->name,"Title") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        Title %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_Title,(char *)key);
	  }  /*  found a Photo Title  */

	  if ( strcmp((char *)PhotoNode->name,"Gallery") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        Gallery %s\n",(char *)key);
#endif
	    sscanf((char *)key,"%d",&Photo_Temp->ph_Gallery);
	  }  /*  found a Photo Gallery  */

	  if ( strcmp((char *)PhotoNode->name,"Views") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        Views %s\n",(char *)key);
#endif
	    sscanf((char *)key,"%d",&Photo_Temp->ph_Views);
	  }  /*  found a Photo Views  */

	  if ( strcmp((char *)PhotoNode->name,"Size") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        Size %s\n",(char *)key);
#endif
	    sscanf((char *)key,"%d",&Photo_Temp->ph_Size);
	  }  /*  found a Photo Size  */

	  if ( strcmp((char *)PhotoNode->name,"Rotation") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        Rotation %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_Rotation,(char *)key);
	  }  /*  found a Photo Rotation  */

	  if ( strcmp((char *)PhotoNode->name,"Keywords") == 0 ) {
#ifdef DEBUG
	    printf("        Keywords\n");
#endif
	    /*  NEED TO DIVE IN HERE  */
	    KWNode = PhotoNode->children;
	    while ( KWNode != NULL) {
#ifdef SHOWFOUND
	      printf("          Found:  %s\n",KWNode->name);
#endif

	      if ( strcmp((char *)KWNode->name,"Keyword") == 0 ) {
		key = xmlNodeListGetString(Pdoc_ptr, KWNode->xmlChildrenNode, 1);
#ifdef DEBUG
		printf("          Keyword %s\n",(char *)key);
#endif
		Key_Temp = (struct pkeys *)malloc(sizeof(struct pkeys));
		Key_Temp->pk_Next = NULL;
		Key_Temp->pk_PID = Photo_Temp->ph_ID;
		strcpy(Key_Temp->pk_Keyword,(char *)key);
		Key_Top = AddKey(Key_Top, Key_Temp);
		free(Key_Temp);
	      }  /*  found a Photo Keyword  */

	      KWNode = KWNode->next;
	    }

	  }  /*  found a Photo Keywords  */

	  if ( strcmp((char *)PhotoNode->name,"ExifTags") == 0 ) {
	    key = xmlNodeListGetString(Pdoc_ptr, PhotoNode->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        ExifTags\n");
#endif
	    /*  NEED TO DIVE IN HERE  */
	    ETags_Temp = (struct petags *)malloc(sizeof(struct petags));
	    ETags_Temp->pt_PID = Photo_Temp->ph_ID;

	    ETNode = PhotoNode->children;
	    while ( ETNode != NULL ) {
#ifdef SHOWFOUND
	      printf("          Found:  %s\n",ETNode->name);
#endif

	      if ( strcmp((char *)ETNode->name, "Id") == 0 ) {
		key = xmlNodeListGetString(Pdoc_ptr, ETNode->xmlChildrenNode, 1);
#ifdef DEBUG
		printf("          Id %s\n",(char *)key);
#endif
		sscanf((char *)key,"%d",&ETags_Temp->pt_ID);
	      }  /*  found a ExifTags Id  */

	      if ( strcmp((char *)ETNode->name, "Value") == 0 ) {
		key = xmlNodeListGetString(Pdoc_ptr, ETNode->xmlChildrenNode, 1);
#ifdef DEBUG
		printf("          Value %s\n",(char *)key);
#endif
		strcpy(ETags_Temp->pt_Value,(char *)key);
	      }  /*  found a ExifTags Value  */

	      if ( strcmp((char *)ETNode->name, "DisplayValue") == 0 ) {
		key = xmlNodeListGetString(Pdoc_ptr, ETNode->xmlChildrenNode, 1);
#ifdef DEBUG
		printf("          DisplayValue %s\n",(char *)key);
#endif
		strcpy(ETags_Temp->pt_DisplayValue,(char *)key);
	      }  /*  found a ExifTags DisplayValue  */



	      ETNode = ETNode->next;
	    }  /*  while parsiing the ExifTags  */
	    ETags_Top = AddETags(ETags_Top, ETags_Temp);
	    free(ETags_Temp);
	  }  /*  found a Photo ExifTags  */

	  if ( strcmp((char *)PhotoNode->name,"Categories") == 0 ) {
	    key = xmlNodeListGetString(Pdoc_ptr, PhotoNode->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        Categories\n");
#endif

	    CatNode = PhotoNode->children;
	    while ( CatNode != NULL) {
#ifdef SHOWFOUND
	      printf("          Found:  %s\n",CatNode->name);
#endif

	      if ( strcmp((char *)CatNode->name,"Category") == 0) {
		key = xmlNodeListGetString(Pdoc_ptr, CatNode->xmlChildrenNode, 1);
#ifdef DEBUG
		printf("            Category %s\n",(char *)key);
#endif
		Cat_Temp = (struct pcats *)malloc(sizeof(struct pcats));
		Cat_Temp->pc_Next = NULL;
		sscanf((char *)key,"%d",&Cat_Temp->pc_Category);
		Cat_Temp->pc_PID = Photo_Temp->ph_ID;
		Cat_Top = AddCat(Cat_Top, Cat_Temp);
		free(Cat_Temp);
	      }  /*  found a Categories Category  */

	      CatNode = CatNode->next;
	    }
	  }  /*  found a Photo Categories  */
	  
	  if ( strcmp((char *)PhotoNode->name,"Flags") == 0 ) {
	    key = xmlNodeListGetString(Pdoc_ptr, PhotoNode->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        Flags %s\n",(char *)key);
#endif
	    PF_Top = ProcPF(PF_Top, (char *)key, Photo_Temp->ph_ID);

	  }  /*  found a Photo Flags  */
	  
	  if ( strcmp((char *)PhotoNode->name,"TextCn") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        TextCn %s\n",(char *)key);
#endif
	    sscanf((char *)key,"%d",&Photo_Temp->ph_TextCn);
	  }  /*  found a Photo TextCn  */

	  if ( strcmp((char *)PhotoNode->name,"MimeType") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        MimeType %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_MimeType,(char *)key);
	  }  /*  found a Photo MimeType  */

	  if ( strcmp((char *)PhotoNode->name,"OriginalUrl") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        OriginalUrl %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_OriginalUrl,(char *)key);
	  }  /*  found a Photo OriginalUrl  */

	  if ( strcmp((char *)PhotoNode->name,"UrlCore") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        UrlCore %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_UrlCore,(char *)key);
	  }  /*  found a Photo UrlCore  */

	  if ( strcmp((char *)PhotoNode->name,"UrlHost") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        UrlHost %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_UrlHost,(char *)key);
	  }  /*  found a Photo UrlHost  */

	  if ( strcmp((char *)PhotoNode->name,"UrlToken") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        UrlToken %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_UrlToken,(char *)key);
	  }  /*  found a Photo UrlToken  */

	  if ( strcmp((char *)PhotoNode->name,"Copyright") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        Copyright %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_Copyright,(char *)key);
	  }  /*  found a Photo Copyright  */

	  if ( strcmp((char *)PhotoNode->name,"PageUrl") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        PageUrl %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_PageUrl,(char *) key);
	  }  /*  found a Photo PageUrl  */

	  if ( strcmp((char *)PhotoNode->name,"ShortExif") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        ShortExif %s (%o)\n",(char *)key, key);
#endif
	    if ( key != 0 )
	      strcpy(Photo_Temp->ph_ShortExif,(char *)key);
	    else
	      Photo_Temp->ph_ShortExif[0] = '\0';
	  }  /*  found a Photo ShortExif  */

	  if ( strcmp((char *)PhotoNode->name,"MailboxId") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        MailboxId %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_MailboxId,(char *)key);
	  }  /*  found a Photo MailboxId  */

	  if ( strcmp((char *)PhotoNode->name,"IsVideo") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        IsVideo %s\n",(char *)key);
#endif
	    strcpy(Photo_Temp->ph_IsVideo,(char *)key);
	  }  /*  found a Photo IsVideo  */

	  if ( strcmp((char *)PhotoNode->name,"PricingKey") == 0 ) {
	    PScur_ptr = PhotoNode;
	    key = xmlNodeListGetString(Pdoc_ptr, PScur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	    printf("        PricingKey %s\n",(char *)key);
#endif
	    sscanf((char *)key,"%d",&Photo_Temp->ph_PricingKey);
	  }  /*  found a Photo PricingKey  */






	  PhotoNode = PhotoNode->next;
	}  /*  while parsing a Photo  */


	Photo_Top = AddPhoto(Photo_Top, Photo_Temp);
	free(Photo_Temp);
      }  /*  if first level element is Photo  */


      Pcur_node = Pcur_node->next;
    }  /*  while parsing XML file  */



    /*    return(0);*/
#ifdef DOUNLINK
    unlink("XML-Photo.xml");
#endif
    PhotoId_Current = PhotoId_Current->ph_Next;
  }  /*  while reading PhotoId list  */
  

  /*

    do the database work

    get the Photo ID numbers already in the database - put them in memory

    generate an insert or update for each photo we've pulled full details
    for

    update the database

  */

  printf("\n  Loading the database...\n");

  /*  open the database connection  */

  DBvip=fopen("/tmp/DBvip","r");
  fscanf(DBvip,"%s",DBLocation);

#ifdef DBDEBUG
  printf("\n\nDatabase is running on %s\n",DBLocation);
#endif

  if ( strcmp(DBLocation,"localhost")==0 ) {
#ifdef DBDEBUG
    printf("setting localhost variables...\n");
#endif
    strcpy(DBHost,"localhost");
    strcpy(DBUser,"root");
    strcpy(DBPassword,"");
  }
  else 
    if ( strcmp(DBLocation,"big-mac")==0 ) {
#ifdef DBDEBUG
      printf("setting big-mac variables...\n");
#endif
      strcpy(DBHost,"big-mac");
      strcpy(DBUser,"doug");
      strcpy(DBPassword,"ILikeSex");
    } else {
      printf("Unknown database:  %s\n",DBLocation);
      exit(1);
    }

  fclose(DBvip);

  strcat(DBName,"Zenfolio");

  /*******************************************************************/
  /* open the database */
  /*******************************************************************/

#ifdef DBDEBUG
  printf("Opening database:\n  %s\n  %s\n  %s\n  %s\n",
         DBHost, DBUser, DBPassword, DBName
         );
#endif

  if (mysql_init(&QueryDB) == NULL) {
    fprintf(stderr,"Database not initialized\n");
    exit(-1);
  }

  if (!mysql_real_connect(&QueryDB,DBHost,DBUser,DBPassword,DBName,
                          3306,NULL,0))
    {
      fprintf(stderr, "Connect failed: %s\n",mysql_error(&QueryDB));
      exit(-1);
    }

  /*  load the PhotoInDB memory structure from the DB Photos table */

  strcpy(DBCommand,"select p_id from photo");
  DBCommandLength = strlen(DBCommand);

#ifdef DBDEBUG
  printf("\nCommand (%d):  %s\n",DBCommandLength, DBCommand);
#endif

  if (mysql_real_query(&QueryDB, DBCommand, DBCommandLength)) {
    fprintf(stderr,"Command %s failed: %s\n",DBCommand,
            mysql_error(&QueryDB));
    exit(-1);
  }
  QueryResult=mysql_use_result(&QueryDB);

  if ( (QueryRow=mysql_fetch_row(QueryResult)) != NULL ) {
    while ( QueryRow != NULL ) {

      /*  this query only returns one column  */

      strcpy(PhotoIdString,QueryRow[0]);
#ifdef DBDEBUG
      printf("  %s\n",QueryRow[0]);
#endif

      /*  add this Photo ID into the PhotoInDB structure  */
      PhotoInDB_Top = AddPhotoInDB(PhotoInDB_Top, PhotoIdString);
      QueryRow = mysql_fetch_row(QueryResult);
    }  /*  while we have rows  */
  }  /*  if not NULL query  */

#ifdef DBDEBUG
  printf("freeing QueryResult and closing database\n");
#endif

  mysql_free_result(QueryResult);
  mysql_close(&QueryDB);

#ifdef DBDEBUG
  printf("Opening database:\n  %s\n  %s\n  %s\n  %s\n",
         DBHost, DBUser, DBPassword, DBName
         );
#endif

  if (mysql_init(&QueryDB) == NULL) {
    fprintf(stderr,"Database not initialized\n");
    exit(-1);
  }

  if (!mysql_real_connect(&QueryDB,DBHost,DBUser,DBPassword,DBName,
                          3306,NULL,0))
    {
      fprintf(stderr, "Connect failed: %s\n",mysql_error(&QueryDB));
      exit(-1);
    }

  /*  
      go through the full photo information in Photo_Top
      if the Photo's Id is in PhotoInDB list, generate an update
      if the Photo's Id is not in PhotoInDB list, generate an insert
  */

#ifdef DBDEBUG
  printf("\nGenerating inserts and updates for Photos...\n");
#endif

  Photo = Photo_Top;
  while ( Photo != NULL ) {
    printf("    Photo %d\n",Photo->ph_ID);
    if ( IsPhotoInDB(Photo->ph_ID, PhotoInDB_Top) )  {
#ifdef DEBUG
      printf("      generating update\n");
#endif

      sprintf(DBCommand,"update photo set p_Width = %d, p_Height = %d, p_Sequence = '%s', AD_ID = %d, p_Owner = '%s', p_Title = \"%s\", p_MimeType = '%s', p_Views = %d, p_Size = %d, p_Gallery = %d, p_OriginalURL = '%s', p_UrlCore = '%s', p_UrlHost = '%s', p_UrlToken = '%s', p_PageUrl = '%s', p_MailboxID = '%s', p_TextCn = %d, p_PhotoListCn = %d, p_IsVideo = %s, p_PricingKey = %d, p_Caption = \"%s\", p_FileName = '%s', p_UploadedOn = '%s', p_TakenOn = '%s', p_Copyright = '%s', p_Rotation = '%s', p_ShortExif = '%s' where p_ID = %d", 
	      Photo->ph_Width, Photo->ph_Height, Photo->ph_Sequence, Photo->ph_AD,
	      Photo->ph_Owner, Photo->ph_Title, Photo->ph_MimeType, 
	      Photo->ph_Views, Photo->ph_Size, Photo->ph_Gallery, 
	      Photo->ph_OriginalUrl, Photo->ph_UrlCore, Photo->ph_UrlHost,
	      Photo->ph_UrlToken, Photo->ph_PageUrl, Photo->ph_MailboxId,
	      Photo->ph_TextCn, Photo->ph_PhotoListCn, Photo->ph_IsVideo,
	      Photo->ph_PricingKey, Photo->ph_Caption, Photo->ph_FileName,
	      Photo->ph_UploadedOn, Photo->ph_TakenOn, Photo->ph_Copyright,
	      Photo->ph_Rotation, Photo->ph_ShortExif,

	      Photo->ph_ID);

      DBCommandLength = strlen(DBCommand);

    }   /*  Photo in DB - generate update  */
    else {
#ifdef DEBUG
      printf("      generating insert\n");
#endif

      printf("Photo %d:  %s %s\n",Photo->ph_ID, Photo->ph_Title, Photo->ph_Caption);

      sprintf(DBCommand,"insert into photo (p_ID, p_Width, p_Height, p_Sequence, ad_id, p_Caption, p_FileName, p_UploadedOn, p_TakenOn, p_Owner, p_Title, p_MimeType, p_Views, p_Size, p_Gallery, p_OriginalUrl, p_UrlCore, p_UrlHost, p_UrlToken, p_PageUrl, p_MailboxId, p_TextCn, p_PhotoListCn, p_Copyright, p_Rotation, p_ShortExif, p_IsVideo, p_PricingKey) values (%d, %d, %d, '%s', %d, \"%s\", '%s', '%s', '%s', '%s', \"%s\", '%s', %d, %d, %d, '%s', '%s', '%s', '%s', '%s', '%s', %d, %d, '%s', '%s', '%s', %s, %d)",
	      Photo->ph_ID, Photo->ph_Width, Photo->ph_Height, Photo->ph_Sequence,
	      Photo->ph_AD, Photo->ph_Caption, Photo->ph_FileName, 
	      Photo->ph_UploadedOn, Photo->ph_TakenOn, Photo->ph_Owner,
	      Photo->ph_Title, Photo->ph_MimeType, Photo->ph_Views, Photo->ph_Size,
	      Photo->ph_Gallery, Photo->ph_OriginalUrl, Photo->ph_UrlCore,
	      Photo->ph_UrlHost, Photo->ph_UrlToken, Photo->ph_PageUrl,
	      Photo->ph_MailboxId, Photo->ph_TextCn, Photo->ph_PhotoListCn,
	      Photo->ph_Copyright, Photo->ph_Rotation, Photo->ph_ShortExif,
	      Photo->ph_IsVideo, Photo->ph_PricingKey);
      DBCommandLength = strlen(DBCommand);

#ifdef DBDEBUG
      printf("\nCommand (%d):  %s\n",DBCommandLength, DBCommand);
#endif

    }  /*  Photo not in DB - generate insert  */

    /*  send the command to the DB  */

    if (mysql_real_query(&QueryDB, DBCommand, DBCommandLength)) {

      /*  we did lazy (no) list management - ignore duplicate entries  */

      if ( mysql_errno(&QueryDB) != 1062 ) {
	fprintf(stderr,"Command %s failed: %s\n", DBCommand,
		mysql_error(&QueryDB));
	printf("---->  mysql_errno:  %d\n",mysql_errno(&QueryDB));
	exit(-1);
      }  /*  if not ER_DUP_ENTRY  */

    }  /*  if error on DB Command  */

#ifdef DBDEBUG
    printf("consuming DB result and freeing QueryResult\n");
#endif

    QueryResult=mysql_use_result(&QueryDB);
    mysql_free_result(QueryResult);


    Photo = Photo->ph_Next;
  }  /*  while each Photo  */

  /*
    load PhotoFlags into database
  */

  /*  load the PFInDB memory structure fro teh DB PhotoFlags table */

  strcpy(DBCommand,"select p_id from photoflags");
  DBCommandLength = strlen(DBCommand);

#ifdef DBDEBUG
  printf("\nCommand (%d):  %s\n",DBCommandLength, DBCommand);
#endif

  if (mysql_real_query(&QueryDB, DBCommand, DBCommandLength)) {
    fprintf(stderr,"Command %s failed: %s\n",DBCommand,
            mysql_error(&QueryDB));
    exit(-1);
  }
  QueryResult=mysql_use_result(&QueryDB);

  if ( (QueryRow=mysql_fetch_row(QueryResult)) != NULL ) {
    while ( QueryRow != NULL ) {

      /*  this query only returns one column  */

      strcpy(PhotoIdString,QueryRow[0]);
#ifdef DBDEBUG
      printf("  %s\n",QueryRow[0]);
#endif

      /*  add this Photo ID into the PhotoInDB structure  */
      PFInDB_Top = AddPFInDB(PFInDB_Top, PhotoIdString);
      QueryRow = mysql_fetch_row(QueryResult);
    }  /*  while we have rows  */
  }  /*  if not NULL query  */

#ifdef DBDEBUG
  printf("freeing QueryResult and closing database\n");
#endif

  mysql_free_result(QueryResult);
  mysql_close(&QueryDB);

#ifdef DBDEBUG
  printf("Opening database:\n  %s\n  %s\n  %s\n  %s\n",
         DBHost, DBUser, DBPassword, DBName
         );
#endif

  if (mysql_init(&QueryDB) == NULL) {
    fprintf(stderr,"Database not initialized\n");
    exit(-1);
  }

  if (!mysql_real_connect(&QueryDB,DBHost,DBUser,DBPassword,DBName,
                          3306,NULL,0))
    {
      fprintf(stderr, "Connect failed: %s\n",mysql_error(&QueryDB));
      exit(-1);
    }

#ifdef DBDEBUG
  printf("\nGenerating inserts and updates for PhotoFlags\n");
#endif

  printf("\n");
  PF = PF_Top;
  while ( PF != NULL ) {
    printf("    PhotoFlag %d\n",PF->pf_PID);
    if ( IsPFInDB(PF->pf_PID, PFInDB_Top) ) {
#ifdef DBDEBUG
      printf("      generating update\n");
#endif

      sprintf(DBCommand,"update PhotoFlags set pf_hastitle = %d, pf_hascaption = %d, pf_haskeywords = %d, pf_hascategories = %d, pf_hasexif = %d, pf_hascomments = %d where p_id = %d",
	      PF->pf_HasTitle, PF->pf_HasCaption, PF->pf_HasKeywords,
	      PF->pf_HasCategories, PF->pf_HasExif, PF->pf_HasComments,
	      PF->pf_PID);
      DBCommandLength = strlen(DBCommand);

#ifdef DBDEBUG
      printf("==>  Command (%d):  %s\n",DBCommandLength, DBCommand);
#endif

    }  /*  if PhotoFlags is already in DB  */
    else {
#ifdef DBDEBUG
      printf("      generating insert\n");
#endif

      sprintf(DBCommand,"insert into PhotoFlags (P_ID, PF_HasTitle, PF_HasCaption, PF_HasKeywords, PF_HasCategories, PF_HasExif, PF_HasComments) values (%d, %d, %d, %d, %d, %d, %d)",
	      PF->pf_PID,
	      PF->pf_HasTitle, PF->pf_HasCaption, PF->pf_HasKeywords,
	      PF->pf_HasCategories, PF->pf_HasExif, PF->pf_HasComments);
      DBCommandLength = strlen(DBCommand);

#ifdef DBDEBUG
      printf("==>  Command (%d):  %s\n",DBCommandLength, DBCommand);
#endif


    }  /*  if PhotoFLags not in DB  */

    /*  send the command to the DB  */

    if (mysql_real_query(&QueryDB, DBCommand, DBCommandLength)) {

      /*  we did lazy (no) list management - ignore duplicate entries  */

      if ( mysql_errno(&QueryDB) != 1062 ) {
        fprintf(stderr,"Command %s failed: %s\n", DBCommand,
                mysql_error(&QueryDB));
        printf("---->  mysql_errno:  %d\n",mysql_errno(&QueryDB));
        exit(-1);
      }  /*  if not ER_DUP_ENTRY  */

    }  /*  if error on DB Command  */

#ifdef DBDEBUG
    printf("consuming DB result and freeing QueryResult\n");
#endif

    QueryResult=mysql_use_result(&QueryDB);
    mysql_free_result(QueryResult);


    PF = PF->pf_Next;
  }  /*  while each PhotoFlag in memory  */

  /*
    load Categories into database
  */

  printf("\n");
  Cat = Cat_Top;
  while ( Cat != NULL ) {
    printf("    Category %d\n",Cat->pc_PID);

#ifdef DBDEBUG
    printf("      generating insert\n");
#endif

    sprintf(DBCommand,"insert into p_pcategories (p_id, cat_code) values (%d, %d)",
	    Cat->pc_PID, Cat->pc_Category);
    DBCommandLength = strlen(DBCommand);

#ifdef DBDEBUG
    printf("==>  Command (%d):  %s\n",DBCommandLength, DBCommand);
#endif

    /*  send the command to the DB  */

    if (mysql_real_query(&QueryDB, DBCommand, DBCommandLength)) {

      /*  we did lazy (no) list management - ignore duplicate entries  */

      if ( mysql_errno(&QueryDB) != 1062 ) {
        fprintf(stderr,"Command %s failed: %s\n", DBCommand,
                mysql_error(&QueryDB));
        printf("---->  mysql_errno:  %d\n",mysql_errno(&QueryDB));
        exit(-1);
      }  /*  if not ER_DUP_ENTRY  */

    }  /*  if error on DB Command  */

#ifdef DBDEBUG
    printf("consuming DB result and freeing QueryResult\n");
#endif

    QueryResult=mysql_use_result(&QueryDB);
    mysql_free_result(QueryResult);


    Cat = Cat->pc_Next;
  }  /*  while each Category in memory  */

  /*
    load Keywords into database
  */

  printf("\n");
  Key = Key_Top;
  while ( Key != NULL ) {
    printf("    Keyword %d\n",Key->pk_PID);

#ifdef DBDEBUG
    printf("      generating insert\n");
#endif

    sprintf(DBCommand,"insert into keywords (p_id, kw_keyword) values (%d, \"%s\")",
	    Key->pk_PID, Key->pk_Keyword);
    DBCommandLength = strlen(DBCommand);

#ifdef DBDEBUG
    printf("==>  Command (%d):  %s\n",DBCommandLength, DBCommand);
#endif

    /*  send the command to the DB  */

    if (mysql_real_query(&QueryDB, DBCommand, DBCommandLength)) {

      /*  we did lazy (no) list management - ignore duplicate entries  */

      if ( mysql_errno(&QueryDB) != 1062 ) {
        fprintf(stderr,"Command %s failed: %s\n", DBCommand,
                mysql_error(&QueryDB));
        printf("---->  mysql_errno:  %d\n",mysql_errno(&QueryDB));
        exit(-1);
      }  /*  if not ER_DUP_ENTRY  */

    }  /*  if error on DB Command  */

#ifdef DBDEBUG
    printf("consuming DB result and freeing QueryResult\n");
#endif

    QueryResult=mysql_use_result(&QueryDB);
    mysql_free_result(QueryResult);


    Key = Key->pk_Next;
  }  /*  while each Category in memory  */


  /*
    load ExifTags into database
  */

  printf("\n");
  ETags = ETags_Top;
  while ( ETags != NULL ) {
    printf("    ExifTag %d (%d)\n",ETags->pt_PID, ETags->pt_ID);

#ifdef DBDEBUG
    printf("      generating insert\n");
#endif

    sprintf(DBCommand,"insert into exiftag (p_id, ex_id, ex_value, ex_displayvalue) values (%d, %d, \"%s\", \"%s\")",
	    ETags->pt_PID, ETags->pt_ID, ETags->pt_Value, ETags->pt_DisplayValue);
    DBCommandLength = strlen(DBCommand);

#ifdef DBDEBUG
    printf("==>  Command (%d):  %s\n",DBCommandLength, DBCommand);
#endif

    /*  send the command to the DB  */

    if (mysql_real_query(&QueryDB, DBCommand, DBCommandLength)) {

      /*  we did lazy (no) list management - ignore duplicate entries  */

      if ( mysql_errno(&QueryDB) != 1062 ) {
        fprintf(stderr,"Command %s failed: %s\n", DBCommand,
                mysql_error(&QueryDB));
        printf("---->  mysql_errno:  %d\n",mysql_errno(&QueryDB));
        exit(-1);
      }  /*  if not ER_DUP_ENTRY  */

    }  /*  if error on DB Command  */

#ifdef DBDEBUG
    printf("consuming DB result and freeing QueryResult\n");
#endif

    QueryResult=mysql_use_result(&QueryDB);
    mysql_free_result(QueryResult);


    ETags = ETags->pt_Next;
  }  /*  while each Category in memory  */


  /*******************************************************************/
  /*  close the database connection  */
  /*******************************************************************/

  mysql_close(&QueryDB);


  /*  free data structures  */

  printf("\n  Freeing data structures\n");

  printf("    PhotoIds...\n");
  Photo = PhotoId_Top;
  while ( Photo != NULL ) {
    Photo_Current = Photo;
    Photo = Photo->ph_Next;
#ifdef MEMDEBUG
    printf("      freeing a  PhotoId\n");
#endif
    free(Photo_Current);
  };

  printf("    Photos...\n");
  Photo = Photo_Top;
  while ( Photo != NULL ) {
    Photo_Current = Photo;
    Photo = Photo->ph_Next;
#ifdef MEMDEBUG
    printf("      freeing a  Photo\n");
#endif
    free(Photo_Current);
  };

  printf("    PhotoInDB...\n");
  PhotoInDB = PhotoInDB_Top;
  while ( PhotoInDB != NULL ) {
    PhotoInDB_Current = PhotoInDB;
    PhotoInDB = PhotoInDB->ph_Next;
#ifdef MEMDEBUG
    printf("      freeing a  PhotoInDB\n");
#endif
    free(PhotoInDB_Current);
  }

  printf("    PFInDB...\n");
  PFInDB = PFInDB_Top;
  while ( PFInDB != NULL ) {
    PFInDB_Current = PFInDB;
    PFInDB = PFInDB->pf_Next;
#ifdef MEMDEBUG
    printf("      freeing a  PFInDB\n");
#endif
    free(PFInDB_Current);
  }

  printf("    PhotoFlags...\n");
  PF = PF_Top;
  while ( PF != NULL) {
    PF_Current = PF;
    PF = PF->pf_Next;
#ifdef MEMDEBUG
    printf("      freeing a  PhotoFlag\n");
#endif
    free(PF_Current);
  };

  printf("    Categories...\n");
  Cat = Cat_Top;
  while ( Cat != NULL ) {
    Cat_Current = Cat;
    Cat = Cat->pc_Next;
#ifdef MEMDEBUG
    printf("      freeing a  Category\n");
#endif
    free(Cat_Current);
  }

  printf("    Keywords...\n");
  Key = Key_Top;
  while ( Key != NULL ) {
    Key_Current = Key;
    Key = Key->pk_Next;
#ifdef MEMDEBUG
    printf("      freeing a  Keyword\n");
#endif
    free(Key_Current);
  }

  printf("    ExifTags...\n");
  ETags = ETags_Top;
  while ( ETags != NULL ) {
    ETags_Current = ETags;
    ETags = ETags->pt_Next;
#ifdef MEMDEBUG
    printf("      freeing an ExifTag\n");
#endif
    free(ETags_Current);
  }

}  /*  main  */

#endif
