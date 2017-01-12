/*

  Loads the ZenFolio categories into the CategoryInfo table

*/

/*#undef DEBUG*/
/*#define DEBUG*/

/*#undef DBDEBUG*/
/*#define DBDEBUG*/


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

#define MAXDATASIZE 4096
#define MAXHEADERSIZE 1024
/*#define MAXRECEIVESIZE 16384*/
#define MAXRECEIVESIZE 20000

#define MAXCATEGORIES 800

#include <libxml/parser.h>
#include <libxml/tree.h>

#ifdef LIBXML_TREE_ENABLED


#include <my_global.h>
#include <mysql.h>
#include <my_sys.h>



int main(int argc, char *argv[]) {

  int i;
  int SocketFD, type;
  int BytesRead = 0;
  int NumBytes = 0;
  int HeaderLength, ContentLength;
  int catNumericLength;
  int catindex = -1;
  int numcategories = 0;

  int Tempfile, XMLfile;
  int RemotePort = 443;

  char ch;
  char AuthToken[256];
  char buf[MAXRECEIVESIZE];
  char Header[MAXHEADERSIZE];
  char Content[MAXDATASIZE];
  char ContentLengthstr[5];
  /*  char RemoteHost[25] = "www.zenfolio.com";*/
  char RemoteHost[25] = "api.zenfolio.com";
  char TempfileName[50] = "/tmp/XMLish-Categories";
  char subcatstr[10];

  FILE *tempfile;

  struct hostent *ZenHost;
  struct sockaddr_in RemoteAddr;

  SSL_CTX *ZenCTX;
  SSL *ZenSSL;

  struct catstruct {
    char description[50];
    char numeric[15];
  };
  struct catstruct Categories[MAXCATEGORIES];

  xmlDoc *doc;
  xmlDocPtr doc_ptr;
  xmlChar *key;

  xmlNode *cur_node = NULL,
    *root_node = NULL,
    /*    *catarry = NULL,*/
    *category = NULL;

  xmlNodePtr cur_ptr = NULL;

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
  MYSQL_FIELD *FieldDB;


  /* arg checking */

  if ( argc != 2) {
    printf("Need an AuthToken\n");
    return(1);
  }

  /*  open the temp file to dump the reply into */

  /*  tempfile = fopen("/tmp/XMLish-Categories", "w");*/
  tempfile = fopen(TempfileName, "w");

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

  /*
  strcpy(Content,"photosetId=");
  strcat(Content,argv[2]);
  strcat(Content,"&level=Full");
  strcat(Content,"&includePhotos=false");
  */

  strcpy(Content,"");

  ContentLength = strlen(Content);
  sprintf(ContentLengthstr,"%d",ContentLength);
#ifdef DEBUG
  printf("++  Content (%d): %s\n",ContentLength, Content);
#endif

  /*  strcpy(Header,"POST /zf/api/zfapi.asmx/GetCategories HTTP/1.1\n");*/
  strcpy(Header,"POST /api/1.8/zfapi.asmx/GetCategories HTTP/1.1\n");
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
      fprintf(tempfile,"%c",buf[type]);
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

  fclose(tempfile);

  /*  remove the header from the temp file and create the final XML file */

  Tempfile = open(TempfileName, O_RDONLY | O_CREAT);
  if ( fchmod(Tempfile, 0000600) == -1 ) {
    printf("Tempfile chmod failed --> %d\n",errno);
  }
  XMLfile = open("XML-Categories.xml", O_WRONLY | O_CREAT);
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
  unlink(TempfileName);
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

    We now have the XML definition of the categories in XML-Categories.xml

    Parse it

  */

  /*  initialize the library and check version compatability */

  LIBXML_TEST_VERSION ;

  /* parse the file */

  doc = xmlReadFile("XML-Categories.xml", NULL, 0);
  if ( doc == NULL ) {
    printf("could not parse file %s\n",argv[1]);
    return(1);
  }

  doc_ptr = doc;

  /*  get the root element node */

  root_node = xmlDocGetRootElement(doc);
  cur_node = root_node;

  /* 
    first level element is ArrayOfCategory
    second level element of interest is Category
    third level elements of interest are Code and DisplayName
  */

  /*  cur_node better be ArrayOfCategory - dive in... */

#ifdef DEBUG
  printf("Should be ArrayOfCategory: %s\n",cur_node->name);
#endif

  cur_node = cur_node->children;

  /*
    now at second level
    consume until Category node found
    process Category nodes
  */


  while ( cur_node != NULL ) {
#ifdef DEBUG
    printf("  Checking: %s\n",cur_node->name);
#endif

    if ( strcmp((char *)cur_node->name,"Category") == 0 ) {
      catindex++;
#ifdef DEBUG
      printf("  Found a Category (%d)\n",catindex);
#endif

      /*  dive in */

      category=cur_node->children;
      while ( category != NULL ) {
#ifdef DEBUG
	printf("    Checking: %s\n",category->name);
#endif

	if ( strcmp((char *)category->name,"Code") == 0 ) {
	  cur_ptr = category;
	  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	  printf("      node: %s (%d)\n",category->name, catindex);
	  printf("      value:  %s\n",(char *)key);
#endif
	  strcpy(Categories[catindex].numeric, (char *)key);
	}  /*  if a Code node  */

	if (strcmp((char *)category->name,"DisplayName") == 0 ) {
	  cur_ptr = category;
	  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	  printf("      node: %s (%d)\n",category->name, catindex);
	  printf("      value:  %s\n",(char *)key);
#endif
	  strcpy(Categories[catindex].description, (char *)key);
	}  /*  if a DisplayName node  */

	category = category->next;
      }  /*  process keywords */
    }  /*  process a Category  */

    cur_node=cur_node->next;
  }  /*  while perusing second level nodes */


  /*  clean up XML stuff  */

  xmlFreeDoc(doc);
  xmlCleanupParser();

  /*
    we now have the categories in an array of structures
    do some intelligent organization
  */

  numcategories = catindex + 1;

#ifdef DEBUG
  printf("found %d Categories (%s to %s)\n",
	 numcategories, 
	 Categories[0].description,
	 Categories[catindex].description);
#endif

  /*
    open the database to load categories as we parse them
  */

  /*  where is the database?  */

  DBvip=fopen("/tmp/DBvip","r");
  fscanf(DBvip,"%s",DBLocation);

#ifdef DBDEBUG
  printf("Database is running on %s\n",DBLocation);
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

  /* open the database */

#ifdef DBDEBUG
  printf("Opening database:\n  %s\n  %s\n  %s\n  %s\n",
	 DBHost, DBUser, DBPassword, DBName
	 );
#endif

  if (mysql_init(&QueryDB) == NULL) {
    fprintf(stderr,"Database not initialized\n");
    exit(-1);
  }
  /*  if (!mysql_real_connect(&QueryDB,"localhost","root","",DBName,*/
  /*  if (!mysql_real_connect(&QueryDB,"big-mac","doug","ILikeSex",DBName,*/
  if (!mysql_real_connect(&QueryDB,DBHost,DBUser,DBPassword,DBName,
			  3306,NULL,0))
    {
      fprintf(stderr, "Connect failed: %s\n",mysql_error(&QueryDB));
      exit(-1);
    }


  /*
    find the top level categories
    these will have only the first 3 significat characters non-zero
  */

  /*
    it appears that there are 2 lengths of numerics
    7 character numerics only have a category and subcategory
    SOME 8 character numerics have category, subcategory, and 
      subcategory details coded as cccssddd
  */

  /*  test for checking for top level categories... */

#ifdef DEBUG
  printf("\n\n");
  printf("about to enter categories loop with %d categories...\n\n",
	 numcategories);
#endif
  for (catindex=0; catindex < numcategories ; catindex++) {
    catNumericLength=strlen(Categories[catindex].numeric);
#ifdef DEBUG
    printf("Top of loop\n");
    printf("%d --> %s %s - %d -- ",
	   catindex,
	   Categories[catindex].description,
	   Categories[catindex].numeric,
	   catNumericLength
	   );
#endif
    for (i=0; i < 10; i++) subcatstr[i] = '\0';
    for (i=3; i < catNumericLength; i++) {
#ifdef DEBUG
      printf("%c",Categories[catindex].numeric[i]);
#endif
      subcatstr[i-3]=Categories[catindex].numeric[i];
    }
#ifdef DEBUG
    printf(" (%s)\n",subcatstr);
#endif

    switch( catNumericLength ) {

    case 7:
      if ( strcmp(subcatstr,"0000") == 0 ) {
#ifdef DEBUG
	printf("  ---->  Top level category\n");
#endif

	/*  load into database as a top level category  */

	strcpy(Command,"insert into categoryinfo (CAT_Code, CAT_DisplayName, isTopLevelCategory, isSubCatDefinition) values ('");
	strcat(Command,Categories[catindex].numeric);
	strcat(Command,"', '");
	strcat(Command,Categories[catindex].description);
	strcat(Command,"', TRUE, FALSE)");

	CommandLength=strlen(Command);

#ifdef DBDEBUG
	printf("Command is %s\n",Command);
#endif

	if (mysql_real_query(&QueryDB, Command, CommandLength)) {
	  fprintf(stderr,"Command %s failed: %s\n",Command,
		  mysql_error(&QueryDB));
	  exit(-1);
	}
  /*  QueryResult=mysql_use_result(&QueryDB);*/


      } else {
          /*  load into the database as a leaf category */
#ifdef DEBUG
	  printf("  ---->  Leaf category\n");
#endif

	  strcpy(Command,"insert into categoryinfo (CAT_Code, CAT_DisplayName, isTopLevelCategory, isSubCatDefinition) values ('");
	  strcat(Command,Categories[catindex].numeric);
	  strcat(Command,"', '");
	  strcat(Command,Categories[catindex].description);
	  strcat(Command,"', FALSE, FALSE)");

	  CommandLength=strlen(Command);

#ifdef DBDEBUG
	  printf("Command is %s\n",Command);
#endif

	  if (mysql_real_query(&QueryDB, Command, CommandLength)) {
	    fprintf(stderr,"Command %s failed: %s\n",Command,
		    mysql_error(&QueryDB));
	    exit(-1);
	  }
	  /*  QueryResult=mysql_use_result(&QueryDB);*/

	}  /*  if top  level category */
      break;

    case 8:
      if ( strcmp(subcatstr,"00000") == 0 ) {
#ifdef DEBUG
	printf("  ---->  Top level category\n");
#endif

	/*  load into database as a top level category  */

	strcpy(Command,"insert into categoryinfo (CAT_Code, CAT_DisplayName, isTopLevelCategory, isSubCatDefinition) values ('");
	strcat(Command,Categories[catindex].numeric);
	strcat(Command,"', '");
	strcat(Command,Categories[catindex].description);
	strcat(Command,"', TRUE, FALSE)");

	CommandLength=strlen(Command);

#ifdef DBDEBUG
	printf("Command is %s\n",Command);
#endif

	if (mysql_real_query(&QueryDB, Command, CommandLength)) {
	  fprintf(stderr,"Command %s failed: %s\n",Command,
		  mysql_error(&QueryDB));
	  exit(-1);
	}
	/*  QueryResult=mysql_use_result(&QueryDB);*/


      }  /*  8 character top level category  */
      else {
	for (i=0 ; i < 10 ; i++) subcatstr[i] = '\0';
	for (i=5 ; i < catNumericLength ; i++)
	  subcatstr[i-5]=Categories[catindex].numeric[i];

	if ( strcmp(subcatstr,"000") == 0 ) {
#ifdef DEBUG
	  printf("  ----> Sub category definition\n");
#endif

	  /*  load into database as a subcategory header  */

	  strcpy(Command,"insert into categoryinfo (CAT_Code, CAT_DisplayName, isTopLevelCategory, isSubCatDefinition) values ('");
	  strcat(Command,Categories[catindex].numeric);
	  strcat(Command,"', '");
	  strcat(Command,Categories[catindex].description);
	  strcat(Command,"', FALSE, TRUE)");

	  CommandLength=strlen(Command);

#ifdef DBDEBUG
	  printf("Command is %s\n",Command);
#endif

	  if (mysql_real_query(&QueryDB, Command, CommandLength)) {
	    fprintf(stderr,"Command %s failed: %s\n",Command,
		    mysql_error(&QueryDB));
	    exit(-1);
	  }
	  /*  QueryResult=mysql_use_result(&QueryDB);*/

	}  
	else {
	  /* load into database as a leaf category */

#ifdef DEBUG
	  printf("  ---->  Leaf category\n");
#endif

	  strcpy(Command,"insert into categoryinfo (CAT_Code, CAT_DisplayName, isTopLevelCategory, isSubCatDefinition) values ('");
	  strcat(Command,Categories[catindex].numeric);
	  strcat(Command,"', '");
	  strcat(Command,Categories[catindex].description);
	  strcat(Command,"', FALSE, FALSE)");

	  CommandLength=strlen(Command);

#ifdef DBDEBUG
	  printf("Command is %s\n",Command);
#endif

	  if (mysql_real_query(&QueryDB, Command, CommandLength)) {
	    fprintf(stderr,"Command %s failed: %s\n",Command,
		    mysql_error(&QueryDB));
	    exit(-1);
	  }

	}  /*  found a subcategory definition  */
      }  /*  not 8 character top level category  */

      break;

    default:
      printf("\n\n!! UNDEFINED catNumericLength %d !!\n",catNumericLength);
      exit(-1);
      break;

    }  /*  switch on catNumericLength  */

  }  /*  for each category found  */

#ifdef DEBUG
  printf("Out of category loop\n");
#endif

  /*
    close the database
  */

  QueryResult=mysql_use_result(&QueryDB);
  mysql_free_result(QueryResult);
  mysql_close(&QueryDB);

}


#endif
