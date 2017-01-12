/*

  Loads the Group hierarchy for a user

  Builds a list of PhotoSets to be loaded into the PhotoSets table

*/

/*
#undef DEBUG
#define DEBUG
*/

/*
#undef DBDEBUG
#define DBDEBUG
*/

/*
#undef SHOWFOUND
#define SHOWFOUND
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

#include "LoadGroups.h"


#ifdef LIBXML_TREE_ENABLED


int main(int argc, char *argv[]) {

  /*  general vars */

  int i;
  int BytesRead = 0;
  int NumBytes = 0;
  char ch;
  char subcatstr[10];

  /*  ZenFolio vars */

  int HeaderLength, ContentLength;
  char AuthToken[256];
  char buf[MAXRECEIVESIZE];
  char Header[MAXHEADERSIZE];
  char Content[MAXDATASIZE];
  char ContentLengthstr[5];
  /*  char RemoteHost[25] = "www.zenfolio.com";*/
  char RemoteHost[25] = "api.zenfolio.com";
  /*  char RemoteHost[25] = "ZFREMOTEHOST";*/

  /*  socket vars */

  int RemotePort = 443;
  int SocketFD, type;
  struct hostent *ZenHost;
  struct sockaddr_in RemoteAddr;
  SSL_CTX *ZenCTX;
  SSL *ZenSSL;

  /*  XML vars */

  int Tempfile, XMLfile;
  xmlDoc *doc;
  xmlDocPtr doc_ptr;
  xmlChar *key;
  xmlNodePtr cur_ptr = NULL;
  xmlNode 
    *cur_node = NULL,
    *root_node = NULL;


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


  /*  program specific */

  char TempfileName[50] = "/tmp/XMLish-Groups";
  FILE *tempfile;

  char tmpString[256],
    strAccessMask[2048];

  int CurrentGroup;
  int CurrentPhotoSet;
  int CurrentTitlePhoto;
  int CurrentAD;
  int RootGroup;
  int numGroups,
    numAccessDescriptors,
    numPhotoSets;

  char StringVal[50];

  xmlNode *ElementsNode = NULL,
    *GroupNode = NULL,
    *PSGroupNode = NULL,
    *PSElementsNode = NULL,
    *GEPhotoSetNode = NULL,
    *ADNode = NULL,
    *TitlePhotoNode = NULL,
    *PhotosNode = NULL,
    *KeywordsNode = NULL,
    *CategoriesNode = NULL,
    *PhotoSetNode = NULL;

  struct tmp_PS {
    int  tPS_Id;
    char tPS_Name[NAMESTRINGLENGTH];
  };

  struct tmp_PS tmp_PhotoSets[MAXPHOTOSETS];
  int PhotoSetCounter = -1,
    NumPhotoSets = -1,
    AMCounter = -1,
    NumAM = -1,
    ADCounter = -1,
    NumAD = -1;

  struct psgroupstruct {
    int  psgrp_ID;
    char psgrp_CreatedOn[DATESTRINGLENGTH];
    char psgrp_ModifiedOn[DATESTRINGLENGTH];
    char psgrp_PageURL[URLSTRINGLENGTH];
    int  psgrp_TitlePhone;  /*  FK to Photo  */
    char psgrp_MailboxId[EMAILSTRINGLENGTH];
    int  psgrp_ImmedChildCount;
    int  psgrp_TextCn;
    char psgrp_Caption[CAPTIONSTRINGLENGTH];
    int  psgrp_CollectionCount;
    int  psgrp_SubGroupCount;
    int  psgrp_GalleryCount;
    int  psgrp_PhotoCount;
    /*  how to handle ParentGroups  */
    /*  GroupElements will be its own structure  */

    struct psgroupstruct *psgrp_Next;
  };
  struct psgroupstruct *PSGroups = NULL;
  struct psgroupstruct *PSGroups_Top = NULL;
  struct psgroupstruct *PSGroups_Bot = NULL;
  struct psgroupstruct *PSGroups_Current = NULL;
  struct psgroupstruct *PSGroups_Last = NULL;
  struct psgroupstruct *PSGroups_Temp = NULL;

  PSGroups_Top = (struct psgroupstruct *)malloc(sizeof(struct psgroupstruct));
  PSGroups = PSGroups_Top;

  /*
    this the first level of LoadGroupHierarchy and Gallery (PS)Groups
  */

  struct groupstruct *Groups = NULL;
  struct groupstruct *Groups_Top = NULL;
  struct groupstruct *Groups_Bot = NULL;
  struct groupstruct *Groups_Current = NULL;
  struct groupstruct *Groups_Last = NULL;
  struct groupstruct *Groups_Temp = NULL;

  /*
    AccessDescriptor is used in many contexts
  */

  struct accdes *AD = NULL;
  struct accdes *AD_Top = NULL;
  struct accdes *AD_Bot = NULL;
  struct accdes *AD_Current = NULL;
  struct accdes *AD_Last = NULL;
  struct accdes *AD_Temp = NULL;

  /*
    AccessMask is used in many contexts 
  */

  struct accmask *AM = NULL;
  struct accmask *AM_Top = NULL;
  struct accmask *AM_Bot = NULL;
  struct accmask *AM_Current = NULL;
  struct accmask *AM_Last = NULL;
  struct accmask *AM_Temp = NULL;

  /*
    Galleries are PhotoSets
  */

  struct gallery *Gal = NULL;
  struct gallery *Gal_Top = NULL;
  struct gallery *Gal_Bot = NULL;
  struct gallery *Gal_Current = NULL;
  struct gallery *Gal_Last = NULL;
  struct gallery *Gal_Temp = NULL;

  /*
    TitlePhotos contain a subset of Photo information
    fill in what we get - load it into the db
    when we process the full photo info will have to read in the partial
      info from the db and build a partial list then fill it out fully
  */

  struct photo *Photo = NULL;
  struct photo *Photo_Top = NULL;
  struct photo *Photo_Bot = NULL;
  struct photo *Photo_Current = NULL;
  struct photo *Photo_Last = NULL;
  struct photo *Photo_Temp = NULL;

  /*
    PhotoFlags information will be presented for TitlePhoto elements
  */

  struct pflags *PF = NULL;
  struct pflags *PF_Top = NULL;
  struct pflags *PF_Bot = NULL;
  struct pflags *PF_Current = NULL;
  struct pflags *PF_Last = NULL;
  struct pflags *PF_Temp = NULL;

  /*
    bit string of 0's to set AccessMask elements to all false
  */

  char *strAMfalse;

  strAMfalse=(char *)malloc( (sizeof(struct accmask) + 1) );
  for ( i = 0 ; i < sizeof(struct accmask) ; i++ )
    strAMfalse[i] = 0;

#ifdef MEMDEBUG
  printf("DOUGEE:  %d %d\n",i,sizeof(struct accmask));
#endif

  strAMfalse[i]='\0';

  /*
    bit string of 0's to set PhotoFlags elements to all false
  */

  char *strPFfalse;

  strPFfalse=(char *)malloc( (sizeof(struct pflags) + 1) );
  for ( i = 0 ; i < sizeof(struct pflags) ; i++ )
    strPFfalse[i] = 0;

#ifdef MEMDEBUG
  printf("DOUGEE:  %d %d\n",i,sizeof(struct pflags));
#endif

  strPFfalse[i]='\0';

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

  ContentLength = BC_LoadGroupHier("opsdog", "Full", Content);

#ifdef DEBUG
  printf("++  Content (%d): %s\n",ContentLength, Content);
#endif

  HeaderLength = BH_LoadGroupHier(AuthToken, ContentLength, Header);

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
  XMLfile = open("XML-Groups.xml", O_WRONLY | O_CREAT);
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

    we now have the complete Group hierarchy in XML-Groups.xml

    parse it into memory structures
    validate pointers are referenced
    load groups into database
    dump PhotoSets to be read and put into database

  */


  /*  initialize the library and check version compatability */

  LIBXML_TEST_VERSION ;

  /* parse the file */

  doc = xmlReadFile("XML-Groups.xml", NULL, 0);
  if ( doc == NULL ) {
    printf("could not parse file %s\n",argv[1]);
    return(1);
  }

#ifdef DEBUG
  printf("\n\nXML file open - let's roll...\n");
#endif

  doc_ptr = doc;

  /*  get the root element node */

  root_node = xmlDocGetRootElement(doc);
  cur_node = root_node;

  /*
    first level element is Group and is the RootGroup
    Elements first level item holds the remaining groups
  */

  /*  cur_node better be Group - dive in... */

#ifdef DEBUG
  printf("Should be Group: %s\n",cur_node->name);
#endif

#ifdef DEBUG
  printf("\nprinting first level elements:\n");
  while ( cur_node != NULL ) {
    printf("  Found: %s\n",cur_node->name);
    cur_node = cur_node->next;
  }  /*  while have nodes  */
#endif

#ifdef DEBUG
  printf("\nprinting second level elements:\n");
  cur_node = root_node->children;
  while ( cur_node != NULL ) {
    printf("  Found: %s\n",cur_node->name);
    cur_node = cur_node->next;
  }  /*  while have nodes  */
#endif

#ifdef DEBUG
  printf("\nprinting third level Element elements:\n");
  cur_node = root_node->children;
  while ( cur_node != NULL ) {
    if ( strcmp((char *)cur_node->name,"Elements") == 0 ) {
      ElementsNode = cur_node->children;
      while ( ElementsNode != NULL ) {
	printf("  Found: %s\n",ElementsNode->name);
	ElementsNode = ElementsNode->next;
      }  /*  while parsing third level  */
    }  /*  if found second level Elements  */
    cur_node = cur_node->next;
  }  /*  while parsing second level  */
#endif

  /*
    parse the XML file
  */

  printf("  Parsing XML file...\n");

  cur_node = root_node;
  while ( cur_node != NULL ) {

    /*    printf("%s\n",cur_node->name);*/

    if ( strcmp((char *)cur_node->name,"Group") == 0 ) {
#ifdef DEBUG
      printf("Group\n");
#endif

      /*  capture this (root) group's info before diving into Elements  */

      Groups_Top = (struct groupstruct *)malloc(sizeof(struct groupstruct));
      Groups = Groups_Top;
      Groups_Top->grp_Next = NULL;

      GroupNode = cur_node->children;
      while ( GroupNode != NULL ) {

#ifdef SHOWFOUND
	printf("  Found:  %s\n",GroupNode->name);
#endif

	/*
	  parse the root group info:  
	    ID, GroupIndex, Title, AccessDescriptor, Owner
	    CreatedOn, ModifiedOn

	  dive into the Elements
	*/

	if ( strcmp((char *)GroupNode->name,"Id") == 0 ) {
	  cur_ptr = GroupNode;
	  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	  printf("  Id %s\n",(char *)key);
#endif
	  sscanf((char *)key,"%d",&Groups_Top->grp_ID);
	}  /*  found the root group Id  */

	if ( strcmp((char *)GroupNode->name,"GroupIndex") == 0 ) {
	  cur_ptr = GroupNode;
	  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	  printf("  GroupIndex %s\n",(char *)key);
#endif
	  sscanf((char *)key,"%d",&Groups_Top->grp_GroupIndex);
	}  /*  found the root group GroupIndex  */

	if ( strcmp((char *)GroupNode->name,"Title") == 0 ) {
	  cur_ptr = GroupNode;
	  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	  printf("  Title %s\n",(char *)key);
#endif
	  strcpy(Groups_Top->grp_Title,(char *)key);
	}  /*  found the root group Title  */

	if ( strcmp((char *)GroupNode->name,"AccessDescriptor") == 0 ) {
#ifdef DEBUG
	  printf("  AccessDescriptor\n");
#endif

	  ADCounter++;
	  AD_Temp=(struct accdes *)malloc(sizeof(struct accdes));
	  strcpy(AD_Temp->ad_PasswordHint,"");
	  strcpy(AD_Temp->ad_SrcPasswordHint,"");

	  /*  NEED TO DIVE IN HERE  */

	  ADNode = GroupNode->children;
	  while ( ADNode != NULL ) {
#ifdef SHOWFOUND
	    printf("    Found:  %s\n",ADNode->name);
#endif

	    if ( strcmp((char *)ADNode->name,"RealmId") == 0 ) {
	      cur_ptr = ADNode;
	      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	      printf("    RealmId %s\n",(char *)key);
#endif

	      sscanf((char *)key,"%d",&AD_Temp->ad_RealmId);
	      sscanf((char *)key,"%d",&Groups_Top->grp_AD);

	    }  /*  found an AccessDescriptor RealmId  */

	    if ( strcmp((char *)ADNode->name,"AccessType") == 0 ) {
	      cur_ptr = ADNode;
	      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	      printf("    AccessType  %s\n",(char *)key);
#endif
	      strcpy(AD_Temp->ad_AccessType,(char *)key);
	    }  /*  found an AccessDescriptor AccessType  */

	    if ( strcmp((char *)ADNode->name,"IsDerived") == 0 ) {
	      cur_ptr = ADNode;
	      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	      printf("    IsDerived  %s\n",(char *)key);
#endif
	      if ( strcmp((char *)key,"true") == 0 )
		AD_Temp->ad_IsDerived = TRUE;
	      else
		AD_Temp->ad_IsDerived = FALSE;
	    }  /*  found an AccessDescriptor IsDerived  */

	    if ( strcmp((char *)ADNode->name,"AccessMask") == 0 ) {
	      cur_ptr = ADNode;
	      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	      printf("    AccessMask  %s\n",(char *)key);
#endif
	      sscanf((char *)key,"%d",&AD_Temp->ad_AccessMask);
	      AM_Top = ProcAM(AM_Top, (char *)key, "AD", AD_Temp->ad_RealmId);

	    }  /*  found an AccessDescriptor AccessMask  */

	    if ( strcmp((char *)ADNode->name,"Viewers") == 0 ) {
	      cur_ptr = ADNode;
	      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	      printf("    Viewers  %s\n",(char *)key);
#endif
	      /*
		Viewers will only be valid if AccessType is UserList
		otherwise it will be a NULL string.
		Since I don't use UserLists, no processing takes place here
	      */
	    }  /*  found an AccessDescriptor Viewers  */


	    ADNode = ADNode->next;
	  }  /*  while parsing AccessDescriptor elements  */

	  AD_Top = AddAD(AD_Top,AD_Temp);
	  free(AD_Temp);

	}  /*  found the root group AccessDescriptor  */

	if ( strcmp((char *)GroupNode->name,"Owner") == 0 ) {
	  cur_ptr = GroupNode;
	  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	  printf("  Owner %s\n",(char *)key);
#endif
	  strcpy(Groups_Top->grp_Owner,(char *)key);
	}  /*  found the root group Owner  */

	if ( strcmp((char *)GroupNode->name,"HideBranding") == 0 ) {
	  cur_ptr = GroupNode;
	  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	  printf("  HideBranding %s\n",(char *)key);
#endif
	  if ( strcmp((char *)key,"true") == 0 )
	    Groups_Top->grp_HideBranding = TRUE;
	  else
	    Groups_Top->grp_HideBranding = FALSE;
	}  /*  found the root group HideBranding  */

	if ( strcmp((char *)GroupNode->name,"CreatedOn") == 0 ) {
	  cur_ptr = GroupNode;
	  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	  printf("  CreatedOn %s\n",(char *)key);
#endif
	  strcpy(Groups_Top->grp_CreatedOn,(char *)key);
	}  /*  found the root group CreatedOn  */

	if ( strcmp((char *)GroupNode->name,"ModifiedOn") == 0 ) {
	  cur_ptr = GroupNode;
	  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
	  printf("  ModifiedOn %s\n",(char *)key);
#endif
	  strcpy(Groups_Top->grp_ModifiedOn,(char *)key);
	}  /*  found the root group ModifiedOn  */

	if ( strcmp((char *)GroupNode->name,"Elements") == 0 ) {

	  /*
	    within Elements there are PhotoSet and Group elements
	  */

#ifdef DEBUG
	  printf("  Elements\n");
#endif
	  ElementsNode = GroupNode->children;
	  while ( ElementsNode != NULL ) {
#ifdef SHOWFOUND
	    printf("    Found:  %s\n",ElementsNode->name);
#endif

	    if ( strcmp((char *)ElementsNode->name,"Group") == 0 ) {
#ifdef DEBUG
	      printf("    Group\n");
#endif

	      Groups_Temp = (struct groupstruct *)malloc(sizeof(struct groupstruct));
	      Groups_Temp->grp_Next = NULL;

	      PSGroupNode=ElementsNode->children;
	      while ( PSGroupNode != NULL ) {
#ifdef SHOWFOUND
		printf("      Found:  %s\n",PSGroupNode->name);
#endif

		if ( strcmp((char *)PSGroupNode->name,"Id") == 0 ) {
		  cur_ptr = PSGroupNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      Id %s\n",(char *)key);
#endif
		  sscanf((char *)key,"%d",&Groups_Temp->grp_ID);
		}  /*  found a PSGroup Id element  */

		if ( strcmp((char *)PSGroupNode->name,"GroupIndex") == 0 ) {
		  cur_ptr = PSGroupNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      GroupIndex %s\n",(char *)key);
#endif
		  sscanf((char *)key,"%d",&Groups_Temp->grp_GroupIndex);
		}  /*  found a PSGroup GroupIndex element  */

		if ( strcmp((char *)PSGroupNode->name,"Title") == 0 ) {
		  cur_ptr = PSGroupNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      Title %s\n",(char *)key);
#endif
		  strcpy(Groups_Temp->grp_Title,(char *)key);
		}  /*  found a PSGroup Title element  */

		if ( strcmp((char *)PSGroupNode->name,"AccessDescriptor") == 0 ) {
#ifdef DEBUG
		  printf("      AccessDescriptor\n");
#endif

		  ADCounter++;
		  AD_Temp=(struct accdes *)malloc(sizeof(struct accdes));
		  strcpy(AD_Temp->ad_PasswordHint,"");
		  strcpy(AD_Temp->ad_SrcPasswordHint,"");

		  /*  NEED TO DIVE IN HERE  */

		  ADNode = PSGroupNode->children;
		  while ( ADNode != NULL ) {
#ifdef SHOWFOUND
		    printf("        Found:  %s\n",ADNode->name);
#endif

		    if ( strcmp((char *)ADNode->name,"RealmId") == 0 ) {
		      cur_ptr = ADNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        RealmId %s\n",(char *)key);
#endif
		      sscanf((char *)key,"%d",&AD_Temp->ad_RealmId);
		      sscanf((char *)key,"%d",&Groups_Temp->grp_AD);
		    }  /*  found an AccessDescriptor RealmId  */

		    if ( strcmp((char *)ADNode->name,"AccessType") == 0 ) {
		      cur_ptr = ADNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        AccessType  %s\n",(char *)key);
#endif
		      strcpy(AD_Temp->ad_AccessType,(char *)key);
		    }  /*  found an AccessDescriptor AccessType  */

		    if ( strcmp((char *)ADNode->name,"IsDerived") == 0 ) {
		      cur_ptr = ADNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        IsDerived  %s\n",(char *)key);
#endif
		      if ( strcmp((char *)key,"true") == 0 )
			AD_Temp->ad_IsDerived = TRUE;
		      else
			AD_Temp->ad_IsDerived = FALSE;
		    }  /*  found an AccessDescriptor IsDerived  */

		    if ( strcmp((char *)ADNode->name,"AccessMask") == 0 ) {
		      cur_ptr = ADNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        AccessMask  %s\n",(char *)key);
#endif
		      AM_Top = ProcAM(AM_Top, (char *)key, "AD", AD_Temp->ad_RealmId);
		    }  /*  found an AccessDescriptor AccessMask  */

		    if ( strcmp((char *)ADNode->name,"Viewers") == 0 ) {
		      cur_ptr = ADNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        Viewers  %s\n",(char *)key);
#endif
		      /*
			Viewers will only be valid if AccessType is UserList
			otherwise it will be a NULL string.
			Since I don't use UserLists, no processing takes place here
		      */
		    }  /*  found an AccessDescriptor Viewers  */


		    ADNode = ADNode->next;
		  }  /*  while parsing AccessDescriptor elements  */

		  AD_Top = AddAD(AD_Top,AD_Temp);
		  free(AD_Temp);

		}  /*  found a PSGroup AccessDescriptor element  */

		if ( strcmp((char *)PSGroupNode->name,"Owner") == 0 ) {
		  cur_ptr = PSGroupNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      Owner %s\n",(char *)key);
#endif
		  strcpy(Groups_Temp->grp_Owner,(char *)key);
		}  /*  found a PSGroup Owner element  */

		if ( strcmp((char *)PSGroupNode->name,"CreatedOn") == 0 ) {
		  cur_ptr = PSGroupNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      CreatedOn %s\n",(char *)key);
#endif
		  strcpy(Groups_Temp->grp_CreatedOn,(char *)key);
		}  /*  found a PSGroup CreatedOn element  */

		if ( strcmp((char *)PSGroupNode->name,"ModifiedOn") == 0 ) {
		  cur_ptr = PSGroupNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      ModifiedOn %s\n",(char *)key);
#endif
		  strcpy(Groups_Temp->grp_ModifiedOn,(char *)key);
		}  /*  found a PSGroup ModifedOn element  */

		if ( strcmp((char *)PSGroupNode->name,"Elements") == 0 ) {
#ifdef DEBUG
		  printf("      Elements\n");
#endif

		  /*  NEED TO DIVE IN HERE  */

		  PSElementsNode = PSGroupNode->children;
		  while ( PSElementsNode != NULL ) {
		    /*		    printf("        Found:  %s\n",PSElementsNode->name);*/

		    if ( strcmp((char *)PSElementsNode->name,"PhotoSet") == 0 ) {
#ifdef DEBUG
		      printf("        PhotoSet\n");
#endif

		      PhotoSetCounter++;

		      Gal_Temp = (struct gallery *)malloc(sizeof(struct gallery));
		      Gal_Temp->gal_TitlePhoto = -1;
		      Gal_Temp->gal_Title[0] = '\0';
		      Gal_Temp->gal_Caption[0] = '\0';
		      Gal_Temp->gal_Next = NULL;

		      /*  DIVE IN HERE  */

		      GEPhotoSetNode=PSElementsNode->children;
		      while ( GEPhotoSetNode != NULL ) {
#ifdef SHOWFOUND
			printf("          Found:  %s\n",GEPhotoSetNode->name);
#endif

			if ( strcmp((char *)GEPhotoSetNode->name,"Id") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          Id %s\n",(char *)key);
#endif

			  sscanf((char *)key,"%d",&tmp_PhotoSets[PhotoSetCounter].tPS_Id);
			  sscanf((char *)key,"%d",&Gal_Temp->gal_ID);

			}  /*  found a PhotoSet Id  */

			if ( strcmp((char *)GEPhotoSetNode->name,"GroupIndex") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          GroupIndex %s\n",(char *)key);
#endif
			  sscanf((char *)key,"%d",&Gal_Temp->gal_GroupIndex);
			}  /*  found a PhotoSet GroupIndex  */

			if ( strcmp((char *)GEPhotoSetNode->name,"Title") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          Title %s\n",(char *)key);
#endif

			  strcpy(tmp_PhotoSets[PhotoSetCounter].tPS_Name,(char *)key);
			  strcpy(Gal_Temp->gal_Title,(char *)key);

			}  /*  found a PhotoSet Title  */

			if ( strcmp((char *)GEPhotoSetNode->name,"AccessDescriptor") == 0 ) {
#ifdef DEBUG
			  printf("          AccessDescriptor\n");
#endif

			  ADCounter++;
			  AD_Temp=(struct accdes *)malloc(sizeof(struct accdes));
			  strcpy(AD_Temp->ad_PasswordHint,"");
			  strcpy(AD_Temp->ad_SrcPasswordHint,"");

			  /*  NEED TO DIVE IN HERE  */

			  ADNode = GEPhotoSetNode->children;
			  while ( ADNode != NULL ) {
#ifdef SHOWFOUND
			    printf("            Found:  %s\n",ADNode->name);
#endif

			    if ( strcmp((char *)ADNode->name,"RealmId") == 0 ) {
			      cur_ptr = ADNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			      printf("            RealmId %s\n",(char *)key);
#endif
			      sscanf((char *)key,"%d",&AD_Temp->ad_RealmId);
			      sscanf((char *)key,"%d",&Gal_Temp->gal_AD);
			    }  /*  found an AccessDescriptor RealmId  */

			    if ( strcmp((char *)ADNode->name,"AccessType") == 0 ) {
			      cur_ptr = ADNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			      printf("            AccessType  %s\n",(char *)key);
#endif
			      strcpy(AD_Temp->ad_AccessType,(char *)key);
			    }  /*  found an AccessDescriptor AccessType  */

			    if ( strcmp((char *)ADNode->name,"IsDerived") == 0 ) {
			      cur_ptr = ADNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			      printf("            IsDerived  %s\n",(char *)key);
#endif
			      if ( strcmp((char *)key,"true") == 0 )
				AD_Temp->ad_IsDerived = TRUE;
			      else
				AD_Temp->ad_IsDerived = FALSE;
			    }  /*  found an AccessDescriptor IsDerived  */

			    if ( strcmp((char *)ADNode->name,"AccessMask") == 0 ) {
			      cur_ptr = ADNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			      printf("            AccessMask  %s\n",(char *)key);
#endif
			      AM_Top = ProcAM(AM_Top, (char *)key, "AD", AD_Temp->ad_RealmId);
			    }  /*  found an AccessDescriptor AccessMask  */

			    if ( strcmp((char *)ADNode->name,"Viewers") == 0 ) {
			      cur_ptr = ADNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			      printf("            Viewers  %s\n",(char *)key);
#endif
			      /*
				Viewers will only be valid if AccessType is UserList
				otherwise it will be a NULL string.
				Since I don't use UserLists, no processing takes place here
			      */
			    }  /*  found an AccessDescriptor Viewers  */


			    ADNode = ADNode->next;
			  }  /*  while parsing AccessDescriptor elements  */

			  AD_Top = AddAD(AD_Top,AD_Temp);
			  free(AD_Temp);

			}  /*  found a PhotoSet AccessDescriptor  */

			if ( strcmp((char *)GEPhotoSetNode->name,"Owner") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          Owner %s\n",(char *)key);
#endif
			  strcpy(Gal_Temp->gal_Owner,(char *)key);
			}  /*  found a PhotoSet Owner  */

			if ( strcmp((char *)GEPhotoSetNode->name,"HideBranding") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          HideBranding %s\n",(char *)key);
#endif
			  if ( strcmp((char *)key,"true") == 0 )
			    Gal_Temp->gal_HideBranding = TRUE;
			  else
			    Gal_Temp->gal_HideBranding = FALSE;
			}  /*  found a PhotoSet HideBranding  */

			if ( strcmp((char *)GEPhotoSetNode->name,"Caption") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          Caption %s\n",(char *)key);
#endif
			  strcpy(Gal_Temp->gal_Caption,(char *)key);
			}  /*  found a PhotoSet Caption  */

			if ( strcmp((char *)GEPhotoSetNode->name,"CreatedOn") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          CreatedOn %s\n",(char *)key);
#endif
			  strcpy(Gal_Temp->gal_CreatedOn,(char *)key);
			}  /*  found a PhotoSet CreatedOn  */

			if ( strcmp((char *)GEPhotoSetNode->name,"ModifiedOn") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          ModifiedOn %s\n",(char *)key);
#endif
			  strcpy(Gal_Temp->gal_ModifiedOn,(char *)key);
			}  /*  found a PhotoSet ModifiedOn  */

			if ( strcmp((char *)GEPhotoSetNode->name,"PhotoCount") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          PhotoCount %s\n",(char *)key);
#endif
			  sscanf((char *)key,"%d",&Gal_Temp->gal_PhotoCount);
			}  /*  found a PhotoSet PhotoCount  */

			if ( strcmp((char *)GEPhotoSetNode->name,"PhotoBytes") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          PhotoBytes %s\n",(char *)key);
#endif
			  sscanf((char *)key,"%d",&Gal_Temp->gal_PhotoBytes);
			}  /*  found a PhotoSet PhotoBytes  */

			if ( strcmp((char *)GEPhotoSetNode->name,"Views") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          Views %s\n",(char *)key);
#endif
			  sscanf((char *)key,"%d",&Gal_Temp->gal_Views);
			}  /*  found a PhotoSet Views  */

			if ( strcmp((char *)GEPhotoSetNode->name,"Type") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          Type %s\n",(char *)key);
#endif
			  strcpy(Gal_Temp->gal_Type,(char *)key);
			}  /*  found a PhotoSet Type  */

			if ( strcmp((char *)GEPhotoSetNode->name,"TitlePhoto") == 0 ) {
#ifdef DEBUG
			  printf("          TitlePhoto\n");
#endif

			  Photo_Temp = (struct photo *)malloc(sizeof(struct photo));
			  Photo_Temp->ph_Next = NULL;
			  Photo_Temp->ph_Sequence[0]='\0';

			  /*  NEED TO DIVE IN HERE  */

			  TitlePhotoNode = GEPhotoSetNode->children;
			  while ( TitlePhotoNode != NULL ) {
#ifdef SHOWFOUND
			    printf("            Found:  %s\n",TitlePhotoNode->name);
#endif

			    if ( strcmp((char *)TitlePhotoNode->name,"Id") == 0 ) {
			      cur_ptr = TitlePhotoNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			      printf("            Id %s\n",(char *)key);
#endif
			      sscanf((char *)key,"%d",&Gal_Temp->gal_TitlePhoto);
			      sscanf((char *)key,"%d",&Photo_Temp->ph_ID);
			    }  /*  found a TitlePhoto Id  */

			    if ( strcmp((char *)TitlePhotoNode->name,"Width") == 0 ) {
			      cur_ptr = TitlePhotoNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			      printf("            Width %s\n",(char *)key);
#endif
			      sscanf((char *)key,"%d",&Photo_Temp->ph_Width);
			    }  /*  found a TitlePhoto Width  */

			    if ( strcmp((char *)TitlePhotoNode->name,"Height") == 0 ) {
			      cur_ptr = TitlePhotoNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			      printf("            Height %s\n",(char *)key);
#endif
			      sscanf((char *)key,"%d",&Photo_Temp->ph_Height);
			    }  /*  found a TitlePhoto Height  */

			    if ( strcmp((char *)TitlePhotoNode->name,"Sequence") == 0 ) {
			      cur_ptr = TitlePhotoNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
			      /*			      printf("            Sequence %s (%d)\n",(char *)key, strlen((char *)key));*/

			      /*  DIVE IN HERE ??  */

			    }  /*  found a TitlePhoto Sequence  */

			    if ( strcmp((char *)TitlePhotoNode->name,"AccessDescriptor") == 0 ) {
#ifdef DEBUG
			      printf("            AccessDescriptor\n");
#endif

			      ADCounter++;
			      AD_Temp=(struct accdes *)malloc(sizeof(struct accdes));
			      strcpy(AD_Temp->ad_PasswordHint,"");
			      strcpy(AD_Temp->ad_SrcPasswordHint,"");

			      /*  DIVE IN HERE  */

			      ADNode = TitlePhotoNode->children;
			      while ( ADNode != NULL ) {
#ifdef SHOWFOUND
				printf("              Found:  %s\n",ADNode->name);
#endif

				if ( strcmp((char *)ADNode->name,"RealmId") == 0 ) {
				  cur_ptr = ADNode;
				  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
				  printf("              RealmId %s\n",(char *)key);
#endif
				  sscanf((char *)key,"%d",&AD_Temp->ad_RealmId);
				  sscanf((char *)key,"%d",&Photo_Temp->ph_AD);
				}  /*  found an AccessDescriptor RealmId  */

				if ( strcmp((char *)ADNode->name,"AccessType") == 0 ) {
				  cur_ptr = ADNode;
				  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
				  printf("              AccessType  %s\n",(char *)key);
#endif
				  strcpy(AD_Temp->ad_AccessType,(char *)key);
				}  /*  found an AccessDescriptor AccessType  */

				if ( strcmp((char *)ADNode->name,"IsDerived") == 0 ) {
				  cur_ptr = ADNode;
				  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
				  printf("              IsDerived  %s\n",(char *)key);
#endif
				  if ( strcmp((char *)key,"true") == 0 )
				    AD_Temp->ad_IsDerived = TRUE;
				  else
				    AD_Temp->ad_IsDerived = FALSE;
				}  /*  found an AccessDescriptor IsDerived  */

				if ( strcmp((char *)ADNode->name,"AccessMask") == 0 ) {
				  cur_ptr = ADNode;
				  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
				  printf("              AccessMask  %s\n",(char *)key);
#endif
				  AM_Top = ProcAM(AM_Top, (char *)key, "AD", AD_Temp->ad_RealmId);
				}  /*  found an AccessDescriptor AccessMask  */

				if ( strcmp((char *)ADNode->name,"Viewers") == 0 ) {
				  cur_ptr = ADNode;
				  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
				  printf("              Viewers  %s\n",(char *)key);
#endif
				  /*
				    Viewers will only be valid if AccessType is UserList
				    otherwise it will be a NULL string.
				    Since I don't use UserLists, no processing takes place here
				  */
				}  /*  found an AccessDescriptor Viewers  */


				ADNode = ADNode->next;
			      }  /*  while parsing AccessDescriptor elements  */

			      AD_Top = AddAD(AD_Top,AD_Temp);
			      free(AD_Temp);

			    }  /*  found a TitlePhoto AccessDescriptor  */

			    if ( strcmp((char *)TitlePhotoNode->name,"Flags") == 0 ) {

			      /*  DIVE IN HERE  */

			      cur_ptr = TitlePhotoNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			      printf("            Flags %s\n",(char *)key);
#endif

			      PF_Top = ProcPF(PF_Top, (char *)key, Photo_Temp->ph_ID);

			    }  /*  found a TitlePhoto Flags  */

			    if ( strcmp((char *)TitlePhotoNode->name,"MimeType") == 0 ) {
			      cur_ptr = TitlePhotoNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			      printf("            MimeType %s\n",(char *)key);
#endif
			      strcpy(Photo_Temp->ph_MimeType,(char *)key);
			    }  /*  found a TitlePhoto MimeType  */

			    if ( strcmp((char *)TitlePhotoNode->name,"OriginalUrl") == 0 ) {
			      cur_ptr = TitlePhotoNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			      printf("            OriginalUrl %s\n",(char *)key);
#endif
			      strcpy(Photo_Temp->ph_OriginalUrl,(char *)key);
			    }  /*  found a TitlePhoto OriginalUrl  */

			    if ( strcmp((char *)TitlePhotoNode->name,"UrlCore") == 0 ) {
			      cur_ptr = TitlePhotoNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			      printf("            UrlCore %s\n",(char *)key);
#endif
			      strcpy(Photo_Temp->ph_UrlCore,(char *)key);
			    }  /*  found a TitlePhoto UrlCore  */

			    if ( strcmp((char *)TitlePhotoNode->name,"UrlHost") == 0 ) {
			      cur_ptr = TitlePhotoNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			      printf("            UrlHost %s\n",(char *)key);
#endif
			      strcpy(Photo_Temp->ph_UrlHost,(char *)key);
			    }  /*  found a TitlePhoto UrlHost  */

			    if ( strcmp((char *)TitlePhotoNode->name,"UrlToken") == 0 ) {
			      cur_ptr = TitlePhotoNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			      printf("            UrlToken %s\n",(char *)key);
#endif
			      strcpy(Photo_Temp->ph_UrlToken,(char *)key);
			    }  /*  found a TitlePhoto UrlToken  */


			    TitlePhotoNode = TitlePhotoNode->next;
			  }  /*  while parsing TitlePhotoNode elements  */


			  Photo_Top = AddTitlePhoto(Photo_Top, Photo_Temp);
			  free(Photo_Temp);

			}  /*  found a PhotoSet TitlePhoto  */

			if ( strcmp((char *)GEPhotoSetNode->name,"IsRandomTitlePhoto") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
			  printf("          IsRandomTitlePhoto %s\n",(char *)key);
			}  /*  found a PhotoSet IsRandomTitlePhoto  */

			if ( strcmp((char *)GEPhotoSetNode->name,"Photos") == 0 ) {
			  printf("          Photos\n");

			  /*  NEED TO DIVE IN HERE  */

			  PhotosNode = GEPhotoSetNode->children;
			  while ( PhotosNode != NULL ) {
			    printf("            Found:  %s\n",PhotosNode->name);

			    PhotosNode = PhotosNode->next;
			  }  /*  while parsing Photos elements  */

			}  /*  found a PhotoSet Photos  */

			if ( strcmp((char *)GEPhotoSetNode->name,"Keywords") == 0 ) {
			  printf("          Keywords\n");

			  /*  NEED TO DIVE IN HERE  */

			  KeywordsNode = GEPhotoSetNode->children;
			  while ( KeywordsNode != NULL ) {
			    printf("        Found:  %s\n",KeywordsNode->name);

			    if ( strcmp((char *)KeywordsNode->name,"Keyword") == 0 ) {
			      cur_ptr = KeywordsNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
			      printf("            Keyword %s\n",(char *)key);
			    }  /*  found a PhotoSet Keyword  */


			    KeywordsNode = KeywordsNode->next;
			  }  /*  while parsing Keywords elements  */

			}  /*  found a PhotoSet Keywords  */

			if ( strcmp((char *)GEPhotoSetNode->name,"Categories") == 0 ) {
			  printf("          Categories\n");

			  /*  NEED TO DIVE IN HERE  */

			  CategoriesNode = GEPhotoSetNode->children;
			  while ( CategoriesNode != NULL ) {
			    printf("        Found:  %s\n",CategoriesNode->name);

			    if ( strcmp((char *)CategoriesNode->name,"Category") == 0 ) {
			      cur_ptr = CategoriesNode;
			      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
			      printf("            Category %s\n",(char *)key);
			    }  /*  found a PhotoSet Category  */


			    CategoriesNode = CategoriesNode->next;
			  }  /*  while parsing Categories elements  */

			}  /*  found a PhotoSet Categories  */

			if ( strcmp((char *)GEPhotoSetNode->name,"UploadUrl") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          UploadUrl %s\n",(char *)key);
#endif
			  strcpy(Gal_Temp->gal_UploadUrl,(char *)key);
			}  /*  found a PhotoSet UploadUrl  */

			if ( strcmp((char *)GEPhotoSetNode->name,"PageUrl") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          PageUrl %s\n",(char *)key);
#endif
			  strcpy(Gal_Temp->gal_PageUrl,(char *)key);
			}  /*  found a PhotoSet PageUrl  */

			if ( strcmp((char *)GEPhotoSetNode->name,"MailboxId") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          MailboxId %s\n",(char *)key);
#endif
			  strcpy(Gal_Temp->gal_MailboxId,(char *)key);
			}  /*  found a PhotoSet MailboxId  */

			if ( strcmp((char *)GEPhotoSetNode->name,"TextCn") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          TextCn %s\n",(char *)key);
#endif
			  sscanf((char *)key,"%d",&Gal_Temp->gal_TextCn);
			}  /*  found a PhotoSet TextCn  */

			if ( strcmp((char *)GEPhotoSetNode->name,"PhotoListCn") == 0 ) {
			  cur_ptr = GEPhotoSetNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          PhotoListCn %s\n",(char *)key);
#endif
			  sscanf((char *)key,"%d",&Gal_Temp->gal_PhotoListCn);
			}  /*  found a PhotoSet PhotoListCn  */



			GEPhotoSetNode=GEPhotoSetNode->next;
		      }  /*  while parsing the group - elements - photoset  */

		      Gal_Top = AddGal(Gal_Top, Gal_Temp);
		      free(Gal_Temp);

		    }  /*  found a Group --> Elements --> PhotoSet element  */



		    PSElementsNode = PSElementsNode->next;
		  }  /*  while parsing Elements within a PSGroup  */
		}  /*  found a PSGroup Id element  */



		PSGroupNode = PSGroupNode->next;
	      }  /*  parsing a PSGroup element within Elements  */

	      Groups_Temp->grp_Next = Groups_Top;
	      Groups_Top = Groups_Temp;
	      /*	      Groups_Top = AddGroup(Groups_Top, Groups_Temp);*/
	      /*	      free(Groups_Temp);*/

	    }  /*  found a PSGroup element within Elements  */

	    if ( strcmp((char *)ElementsNode->name,"PhotoSet") == 0 ) {

	      /*
		within PhotoSet parse the specific data
	      */

#ifdef DEBUG
	      printf("    PhotoSet\n");
#endif

	      PhotoSetCounter++;

	      Gal_Temp = (struct gallery *)malloc(sizeof(struct gallery));
	      Gal_Temp->gal_TitlePhoto = -1;
	      Gal_Temp->gal_Title[0] = '\0';
	      Gal_Temp->gal_Caption[0] = '\0';
	      Gal_Temp->gal_Next = NULL;

	      PhotoSetNode = ElementsNode->children;
	      while ( PhotoSetNode != NULL ) {
#ifdef SHOWFOUND
		printf("      Found:  %s\n",PhotoSetNode->name);
#endif

		if ( strcmp((char *)PhotoSetNode->name,"Id") == 0 ) {
		  cur_ptr = PhotoSetNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      Id %s\n",(char *)key);
#endif

		  sscanf((char *)key,"%d",&tmp_PhotoSets[PhotoSetCounter].tPS_Id);
		  sscanf((char *)key,"%d",&Gal_Temp->gal_ID);

		}  /*  found a PhotoSet Id  */

		if ( strcmp((char *)PhotoSetNode->name,"GroupIndex") == 0 ) {
		  cur_ptr = PhotoSetNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      GroupIndex %s\n",(char *)key);
#endif
		  sscanf((char *)key,"%d",&Gal_Temp->gal_GroupIndex);
		}  /*  found a PhotoSet GroupIndex  */

		if ( strcmp((char *)PhotoSetNode->name,"Title") == 0 ) {
		  cur_ptr = PhotoSetNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      Title %s\n",(char *)key);
#endif

		  strcpy(tmp_PhotoSets[PhotoSetCounter].tPS_Name,(char *)key);
		  strcpy(Gal_Temp->gal_Title,(char *)key);

		}  /*  found a PhotoSet Title  */

		if ( strcmp((char *)PhotoSetNode->name,"AccessDescriptor") == 0 ) {
#ifdef DEBUG
		  printf("      AccessDescriptor\n");
#endif

		  ADCounter++;
		  AD_Temp=(struct accdes *)malloc(sizeof(struct accdes));
		  strcpy(AD_Temp->ad_PasswordHint,"");
		  strcpy(AD_Temp->ad_SrcPasswordHint,"");

		  /*  NEED TO DIVE IN HERE  */

		  ADNode = PhotoSetNode->children;
		  while ( ADNode != NULL ) {
#ifdef SHOWFOUND
		    printf("        Found:  %s\n",ADNode->name);
#endif

		    if ( strcmp((char *)ADNode->name,"RealmId") == 0 ) {
		      cur_ptr = ADNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        RealmId %s\n",(char *)key);
#endif
		      sscanf((char *)key,"%d",&AD_Temp->ad_RealmId);
		      sscanf((char *)key,"%d",&Gal_Temp->gal_AD);
		    }  /*  found an AccessDescriptor RealmId  */

		    if ( strcmp((char *)ADNode->name,"AccessType") == 0 ) {
		      cur_ptr = ADNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        AccessType  %s\n",(char *)key);
#endif
		      strcpy(AD_Temp->ad_AccessType,(char *)key);
		    }  /*  found an AccessDescriptor AccessType  */

		    if ( strcmp((char *)ADNode->name,"IsDerived") == 0 ) {
		      cur_ptr = ADNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        IsDerived  %s\n",(char *)key);
#endif
		      if ( strcmp((char *)key,"true") == 0 )
			AD_Temp->ad_IsDerived = TRUE;
		      else
			AD_Temp->ad_IsDerived = FALSE;
		    }  /*  found an AccessDescriptor IsDerived  */

		    if ( strcmp((char *)ADNode->name,"AccessMask") == 0 ) {
		      cur_ptr = ADNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        AccessMask  %s\n",(char *)key);
#endif
		      AM_Top = ProcAM(AM_Top, (char *)key, "AD", AD_Temp->ad_RealmId);
		    }  /*  found an AccessDescriptor AccessMask  */

		    if ( strcmp((char *)ADNode->name,"Viewers") == 0 ) {
		      cur_ptr = ADNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        Viewers  %s\n",(char *)key);
#endif
		      /*
			Viewers will only be valid if AccessType is UserList
			otherwise it will be a NULL string.
			Since I don't use UserLists, no processing takes place here
		      */
		    }  /*  found an AccessDescriptor Viewers  */


		    ADNode = ADNode->next;
		  }  /*  while parsing AccessDescriptor elements  */

		  AD_Top = AddAD(AD_Top,AD_Temp);
		  free(AD_Temp);

		}  /*  found a PhotoSet AccessDescriptor  */

		if ( strcmp((char *)PhotoSetNode->name,"Owner") == 0 ) {
		  cur_ptr = PhotoSetNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      Owner %s\n",(char *)key);
#endif
		  strcpy(Gal_Temp->gal_Owner,(char *)key);
		}  /*  found a PhotoSet Owner  */

		if ( strcmp((char *)PhotoSetNode->name,"HideBranding") == 0 ) {
		  cur_ptr = PhotoSetNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      HideBranding %s\n",(char *)key);
#endif
		  if ( strcmp((char *)key,"true") == 0 )
		    Gal_Temp->gal_HideBranding = TRUE;
		  else
		    Gal_Temp->gal_HideBranding = FALSE;
		}  /*  found a PhotoSet HideBranding  */

		if ( strcmp((char *)PhotoSetNode->name,"Caption") == 0 ) {
		  cur_ptr = PhotoSetNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      Caption %s\n",(char *)key);
#endif
		  strcpy(Gal_Temp->gal_Caption,(char *)key);
		}  /*  found a PhotoSet Caption  */

		if ( strcmp((char *)PhotoSetNode->name,"CreatedOn") == 0 ) {
		  cur_ptr = PhotoSetNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      CreatedOn %s\n",(char *)key);
#endif
		  strcpy(Gal_Temp->gal_CreatedOn,(char *)key);
		}  /*  found a PhotoSet CreatedOn  */

		if ( strcmp((char *)PhotoSetNode->name,"ModifiedOn") == 0 ) {
		  cur_ptr = PhotoSetNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      ModifiedOn %s\n",(char *)key);
#endif
		  strcpy(Gal_Temp->gal_ModifiedOn,(char *)key);
		}  /*  found a PhotoSet ModifiedOn  */

		if ( strcmp((char *)PhotoSetNode->name,"PhotoCount") == 0 ) {
		  cur_ptr = PhotoSetNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      PhotoCount %s\n",(char *)key);
#endif
		  sscanf((char *)key,"%d",&Gal_Temp->gal_PhotoCount);
		}  /*  found a PhotoSet PhotoCount  */

		if ( strcmp((char *)PhotoSetNode->name,"PhotoBytes") == 0 ) {
		  cur_ptr = PhotoSetNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      PhotoBytes %s\n",(char *)key);
#endif
		  sscanf((char *)key,"%d",&Gal_Temp->gal_PhotoBytes);
		}  /*  found a PhotoSet PhotoBytes  */

		if ( strcmp((char *)PhotoSetNode->name,"Views") == 0 ) {
		  cur_ptr = PhotoSetNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      Views %s\n",(char *)key);
#endif
		  sscanf((char *)key,"%d",&Gal_Temp->gal_Views);
		}  /*  found a PhotoSet Views  */

		if ( strcmp((char *)PhotoSetNode->name,"Type") == 0 ) {
		  cur_ptr = PhotoSetNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      Type %s\n",(char *)key);
#endif
		  strcpy(Gal_Temp->gal_Type,(char *)key);
		}  /*  found a PhotoSet Type  */

		if ( strcmp((char *)PhotoSetNode->name,"TitlePhoto") == 0 ) {
#ifdef DEBUG
		  printf("      TitlePhoto\n");
#endif

		  Photo_Temp = (struct photo *)malloc(sizeof(struct photo));
		  Photo_Temp->ph_Next = NULL;
		  Photo_Temp->ph_Sequence[0]='\0';

		  /*  NEED TO DIVE IN HERE  */

		  TitlePhotoNode = PhotoSetNode->children;
		  while ( TitlePhotoNode != NULL ) {
#ifdef SHOWFOUND
		    printf("        Found:  %s\n",TitlePhotoNode->name);
#endif

		    if ( strcmp((char *)TitlePhotoNode->name,"Id") == 0 ) {
		      cur_ptr = TitlePhotoNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        Id %s\n",(char *)key);
#endif
		      sscanf((char *)key,"%d",&Gal_Temp->gal_TitlePhoto);
		      sscanf((char *)key,"%d",&Photo_Temp->ph_ID);
		    }  /*  found a TitlePhoto Id  */

		    if ( strcmp((char *)TitlePhotoNode->name,"Width") == 0 ) {
		      cur_ptr = TitlePhotoNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        Width %s\n",(char *)key);
#endif
		      sscanf((char *)key,"%d",&Photo_Temp->ph_Width);
		    }  /*  found a TitlePhoto Width  */

		    if ( strcmp((char *)TitlePhotoNode->name,"Height") == 0 ) {
		      cur_ptr = TitlePhotoNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        Height %s\n",(char *)key);
#endif
		      sscanf((char *)key,"%d",&Photo_Temp->ph_Height);
		    }  /*  found a TitlePhoto Height  */

		    if ( strcmp((char *)TitlePhotoNode->name,"Sequence") == 0 ) {
		      cur_ptr = TitlePhotoNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        Sequence %s\n",(char *)key);
#endif
		      /*		      strcpy(Photo_Temp->ph_Sequence,(char *)key);*/

		      /*  DIVE IN HERE ??  */

		    }  /*  found a TitlePhoto Sequence  */

		    if ( strcmp((char *)TitlePhotoNode->name,"AccessDescriptor") == 0 ) {
#ifdef DEBUG
		      printf("        AccessDescriptor\n");
#endif

		      ADCounter++;
		      AD_Temp=(struct accdes *)malloc(sizeof(struct accdes));
		      strcpy(AD_Temp->ad_PasswordHint,"");
		      strcpy(AD_Temp->ad_SrcPasswordHint,"");

		      /*  DIVE IN HERE  */

		      ADNode = TitlePhotoNode->children;
		      while ( ADNode != NULL ) {
#ifdef SHOWFOUND
			printf("          Found:  %s\n",ADNode->name);
#endif

			if ( strcmp((char *)ADNode->name,"RealmId") == 0 ) {
			  cur_ptr = ADNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          RealmId %s\n",(char *)key);
#endif
			  sscanf((char *)key,"%d",&AD_Temp->ad_RealmId);
			}  /*  found an AccessDescriptor RealmId  */

			if ( strcmp((char *)ADNode->name,"AccessType") == 0 ) {
			  cur_ptr = ADNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          AccessType  %s\n",(char *)key);
#endif
			  strcpy(AD_Temp->ad_AccessType,(char *)key);
			}  /*  found an AccessDescriptor AccessType  */

			if ( strcmp((char *)ADNode->name,"IsDerived") == 0 ) {
			  cur_ptr = ADNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          IsDerived  %s\n",(char *)key);
#endif
			  if ( strcmp((char *)key,"true") == 0 )
			    AD_Temp->ad_IsDerived = TRUE;
			  else
			    AD_Temp->ad_IsDerived = FALSE;
			}  /*  found an AccessDescriptor IsDerived  */

			if ( strcmp((char *)ADNode->name,"AccessMask") == 0 ) {
			  cur_ptr = ADNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          AccessMask  %s\n",(char *)key);
#endif
			  AM_Top = ProcAM(AM_Top, (char *)key, "AD", AD_Temp->ad_RealmId);
			}  /*  found an AccessDescriptor AccessMask  */

			if ( strcmp((char *)ADNode->name,"Viewers") == 0 ) {
			  cur_ptr = ADNode;
			  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
			  printf("          Viewers  %s\n",(char *)key);
#endif
			  /*
			    Viewers will only be valid if AccessType is UserList
			    otherwise it will be a NULL string.
			    Since I don't use UserLists, no processing takes place here
			  */
			}  /*  found an AccessDescriptor Viewers  */


			ADNode = ADNode->next;
		      }  /*  while parsing AccessDescriptor elements  */

		      AD_Top = AddAD(AD_Top,AD_Temp);
		      free(AD_Temp);

		    }  /*  found a TitlePhoto AccessDescriptor  */


		    if ( strcmp((char *)TitlePhotoNode->name,"Flags") == 0 ) {
		      cur_ptr = TitlePhotoNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        Flags %s\n",(char *)key);
#endif
		      PF_Top = ProcPF(PF_Top, (char *)key, Photo_Temp->ph_ID);
		    }  /*  found a TitlePhoto Flags  */

		    if ( strcmp((char *)TitlePhotoNode->name,"MimeType") == 0 ) {
		      cur_ptr = TitlePhotoNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        MimeType %s\n",(char *)key);
#endif
		      strcpy(Photo_Temp->ph_MimeType,(char *)key);
		    }  /*  found a TitlePhoto MimeType  */

		    if ( strcmp((char *)TitlePhotoNode->name,"OriginalUrl") == 0 ) {
		      cur_ptr = TitlePhotoNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        OriginalUrl %s\n",(char *)key);
#endif
		      strcpy(Photo_Temp->ph_OriginalUrl,(char *)key);
		    }  /*  found a TitlePhoto OriginalUrl  */

		    if ( strcmp((char *)TitlePhotoNode->name,"UrlCore") == 0 ) {
		      cur_ptr = TitlePhotoNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        UrlCore %s\n",(char *)key);
#endif
		      strcpy(Photo_Temp->ph_UrlCore,(char *)key);
		    }  /*  found a TitlePhoto UrlCore  */

		    /*  dougee  */

		    if ( strcmp((char *)TitlePhotoNode->name,"UrlHost") == 0 ) {
		      cur_ptr = TitlePhotoNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        UrlHost %s\n",(char *)key);
#endif
		      strcpy(Photo_Temp->ph_UrlHost,(char *)key);
		    }  /*  found a TitlePhoto UrlHost  */

		    if ( strcmp((char *)TitlePhotoNode->name,"UrlToken") == 0 ) {
		      cur_ptr = TitlePhotoNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		      printf("        UrlToken %s\n",(char *)key);
#endif
		      strcpy(Photo_Temp->ph_UrlToken,(char *)key);
		    }  /*  found a TitlePhoto UrlToken  */


		    TitlePhotoNode = TitlePhotoNode->next;
		  }  /*  while parsing TitlePhotoNode elements  */

		  Photo_Top = AddTitlePhoto(Photo_Top, Photo_Temp);
		  free(Photo_Temp);

		}  /*  found a PhotoSet TitlePhoto  */

		if ( strcmp((char *)PhotoSetNode->name,"IsRandomTitlePhoto") == 0 ) {
		  cur_ptr = PhotoSetNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
		  printf("      IsRandomTitlePhoto %s\n",(char *)key);
		}  /*  found a PhotoSet IsRandomTitlePhoto  */

		if ( strcmp((char *)PhotoSetNode->name,"Photos") == 0 ) {
		  printf("      Photos\n");

		  /*  NEED TO DIVE IN HERE  */

		  PhotosNode = PhotoSetNode->children;
		  while ( PhotosNode != NULL ) {
		    printf("        Found:  %s\n",PhotosNode->name);

		    PhotosNode = PhotosNode->next;
		  }  /*  while parsing Photos elements  */

		}  /*  found a PhotoSet Photos  */

		if ( strcmp((char *)PhotoSetNode->name,"Keywords") == 0 ) {
		  printf("      Keywords\n");

		  /*  NEED TO DIVE IN HERE  */

		  KeywordsNode = PhotoSetNode->children;
		  while ( KeywordsNode != NULL ) {
		    printf("        Found:  %s\n",KeywordsNode->name);

		    if ( strcmp((char *)KeywordsNode->name,"Keyword") == 0 ) {
		      cur_ptr = KeywordsNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
		      printf("        Keyword %s\n",(char *)key);
		    }  /*  found a PhotoSet Keyword  */


		    KeywordsNode = KeywordsNode->next;
		  }  /*  while parsing Keywords elements  */

		}  /*  found a PhotoSet Keywords  */

		if ( strcmp((char *)PhotoSetNode->name,"Categories") == 0 ) {
		  printf("      Categories\n");

		  /*  NEED TO DIVE IN HERE  */

		  CategoriesNode = PhotoSetNode->children;
		  while ( CategoriesNode != NULL ) {
		    printf("        Found:  %s\n",CategoriesNode->name);

		    if ( strcmp((char *)CategoriesNode->name,"Category") == 0 ) {
		      cur_ptr = CategoriesNode;
		      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
		      printf("        Category %s\n",(char *)key);
		    }  /*  found a PhotoSet Category  */


		    CategoriesNode = CategoriesNode->next;
		  }  /*  while parsing Categories elements  */

		}  /*  found a PhotoSet Categories  */

		if ( strcmp((char *)PhotoSetNode->name,"UploadUrl") == 0 ) {
		  cur_ptr = PhotoSetNode;
		  key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
#ifdef DEBUG
		  printf("      UploadUrl %s\n",(char *)key);
#endif
		  strcpy(Gal_Temp->gal_UploadUrl,(char *)key);
		}  /*  found a PhotoSet UploadUrl  */




		PhotoSetNode = PhotoSetNode->next;
	      }  /*  while parsing fourth level PhotoSet elements  */

	      Gal_Top = AddGal(Gal_Top, Gal_Temp);
	      free(Gal_Temp);

	    }  /*  found a PhotoSet element  */

	    ElementsNode = ElementsNode->next;
	  }  /*  while parsing third level Elements elements  */
	}  /*  dive into Elements from second level  */



	GroupNode = GroupNode->next;
      }  /*  while parsing second level Group elements  */
    }  /*  dive into Group from first level  */

    cur_node = cur_node->next;
  }  /*  while parsing first level elements  */


  /*  clean up XML stuff  */

  xmlFreeDoc(doc);
  xmlCleanupParser();

#ifdef DEBUG
  printf("\n");
#endif

  /*  capture Counter values  */

  NumPhotoSets = PhotoSetCounter + 1;
  NumAD = ADCounter + 1;

  /*
    this block of code dumps the built data structures
  */

#ifdef DUMPDATA
  printf("\n\nCounters:\n  PhotoSet %d\n  AD %d\n",
	 PhotoSetCounter, ADCounter);

  printf("\n\nGroups:\n");
  Groups_Current = Groups_Top;
  while ( Groups_Current != NULL ) {
    printf("  %15d\n",Groups_Current->grp_ID);
    Groups_Current = Groups_Current->grp_Next;
  }  /*  while printing Groups  */

  printf("\n\nGalleries:\n");
  Gal_Current = Gal_Top;
  while ( Gal_Current != NULL ) {
    printf("  %15d %s\n",Gal_Current->gal_ID, Gal_Current->gal_Title);
    Gal_Current = Gal_Current->gal_Next;
  }  /*  while printing galleries  */

  printf("\n\nPhotoSets:\n");
  for (PhotoSetCounter = 0 ; PhotoSetCounter < NumPhotoSets ; PhotoSetCounter++ )
    printf("  (%3d) %15d %s\n",PhotoSetCounter,
	   tmp_PhotoSets[PhotoSetCounter].tPS_Id,
	   tmp_PhotoSets[PhotoSetCounter].tPS_Name);

  printf("\n\nAccessDescriptors:\n");
  AD_Current = AD_Top;
  while ( AD_Current != NULL ) {
    printf("  %15d\n",AD_Current->ad_RealmId);
    AD_Current = AD_Current->ad_Next;
  }  /*  while printing AccessDescriptors  */

  printf("\n\nAccessMasks:\n");
  AM_Current = AM_Top;
  while ( AM_Current != NULL ) {
    printf("  %15d %15d %15d\n",AM_Current->am_PID, 
	   AM_Current->am_PSID,
	   AM_Current->am_ADID);
    AM_Current = AM_Current->am_Next;
  }  /*  while printing AccessMasks  */

  printf("\n\nPhotos:\n");
  Photo_Current = Photo_Top;
  while (Photo_Current != NULL ) {
    /*
    printf("  %15d %dx%d\n",Photo_Current->ph_ID, 
	   Photo_Current->ph_Width, Photo_Current->ph_Height);
    */
    printf("  %15d %dx%d %s\n",Photo_Current->ph_ID, 
	   Photo_Current->ph_Width, Photo_Current->ph_Height,
	   Photo_Current->ph_UrlToken);
    Photo_Current = Photo_Current->ph_Next;
  }  /*  while printing Photos  */

  printf("\n\nPhotoFlags:\n");
  PF_Current = PF_Top;
  while ( PF_Current != NULL ) {
    printf("  %15d\n",PF_Current->pf_PID);
    PF_Current = PF_Current->pf_Next;
  }  /*  while printing PhotoFlags  */

#endif

  /*  load the database  */

  printf("  Loading the database...\n");

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


  /*******************************************************************/
  /*  load the PhotoSet Ids into the temporary table  */
  /*******************************************************************/

#ifdef DEBUG
  printf("\n\nLoading PhotoSet into database...\n");
#endif

  for ( PhotoSetCounter = 0 ; PhotoSetCounter < NumPhotoSets ; PhotoSetCounter++ ) {

#ifdef DEBUG
    printf("  Loading %d - %s\n",tmp_PhotoSets[PhotoSetCounter].tPS_Id,
	   tmp_PhotoSets[PhotoSetCounter].tPS_Name);
#endif

    strcpy(Command,"insert into tmp_PhotoSet (tps_Id) values ('");
    sprintf(StringVal,"%d",tmp_PhotoSets[PhotoSetCounter].tPS_Id);
    strcat(Command,StringVal);
    strcat(Command,"')");

    CommandLength = strlen(Command);

#ifdef DBDEBUG
    printf("    Command --> %s\n",Command);
#endif

    if (mysql_real_query(&QueryDB, Command, CommandLength)) {
      fprintf(stderr,"Command %s failed: %s\n",Command,
	      mysql_error(&QueryDB));
      exit(-1);
    }
    /*  QueryResult=mysql_use_result(&QueryDB);*/


  }  /*  for each PhotoSet - load into DB  */

  /*******************************************************************/
  /*  load the AccessDescriptor Ids into the AccessDescriptor table  */
  /*******************************************************************/

#ifdef DEBUG
  printf("\n\nLoading AccessDescriptor table...\n");
#endif

  AD_Current = AD_Top;
  while ( AD_Current != NULL ) {
#ifdef DEBUG
    printf("  Loading %d\n",AD_Current->ad_RealmId);
#endif
    strcpy(Command,"insert into AccessDescriptor (AD_RealmId, AD_IDControlled, AD_Type, AD_AccessMask, AD_IsDerived, AD_PasswordHint, AD_SrcPasswordHint) values ('");
    sprintf(StringVal,"%d",AD_Current->ad_RealmId);  /*  RealmId  */
    strcat(Command,StringVal);
    strcat(Command,"', '0', '");                     /*  ID_Controlled  */
    strcat(Command,AD_Current->ad_AccessType);       /*  AccessType  */
    strcat(Command,"', '0', ");                     /*  AccessMask - FIX THIS  */
    if ( AD_Current->ad_IsDerived )
      strcat(Command,"TRUE, '");
    else                                             /*  IsDerived  */
      strcat(Command,"FALSE, '");
    strcat(Command,AD_Current->ad_PasswordHint);     /*  PasswordHint  */
    strcat(Command,"', '");
    strcat(Command,AD_Current->ad_SrcPasswordHint);  /*  SrcPasswordHint  */

    strcat(Command,"')");

    CommandLength = strlen(Command);

#ifdef DBDEBUG
    printf("    Command --> %s\n",Command);
#endif

    if (mysql_real_query(&QueryDB, Command, CommandLength)) {
      fprintf(stderr,"Command %s failed: %s\n",Command,
	      mysql_error(&QueryDB));
      exit(-1);
    }
    /*  QueryResult=mysql_use_result(&QueryDB);*/


    AD_Current = AD_Current->ad_Next;
  }  /*  for each AD - load into DB  */

  /*******************************************************************/
  /*  load the AccessMask data into the AccessMask table  */
  /*******************************************************************/

#ifdef DEBUG
  printf("\n\nLoading AccessMask table...\n");
#endif

  AM_Current = AM_Top;
  while ( AM_Current != NULL ) {
#ifdef DEBUG
    printf("  Loading %s %d %d %d\n",AM_Current->am_Type,
	   AM_Current->am_PID,
	   AM_Current->am_PSID,
	   AM_Current->am_ADID);
#endif
    strcpy(Command,"insert into AccessMask (AM_Type, P_ID, PS_ID, AD_ID, AM_HideDateCreated, AM_HideDateModified, AM_HideDateTaken, AM_HideMetaData, AM_HideUserStats, AM_HideVisits, AM_NoCollections, AM_NoPrivateSearch, AM_NoPublicSearch, AM_NoRecentList, AM_ProtectExif, AM_ProtectExtraLarge, AM_ProtectLarge, AM_ProtectMedium, AM_ProtectOriginals, AM_ProtectGuestbook, AM_NoPublicGBPosts, AM_NoPrivateGBPosts, AM_NoAnonGBPosts, AM_ProtectComments, AM_NoPublicComments, AM_NoPrivateComments, AM_NoAnonComments, AM_PasswordProtOrig, AM_ProtectAll) values ('");
    strcat(Command,AM_Current->am_Type);
    strcat(Command,"', '");
    sprintf(StringVal,"%d",AM_Current->am_PID);
    strcat(Command,StringVal);
    strcat(Command,"', '");
    sprintf(StringVal,"%d",AM_Current->am_PSID);
    strcat(Command,StringVal);
    strcat(Command,"', '");
    sprintf(StringVal,"%d",AM_Current->am_ADID);
    strcat(Command,StringVal);
    strcat(Command,"', ");

    if ( AM_Current->am_HideDateCreated )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_HideDateModified )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_HideDateTaken )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_HideMetaData )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_HideUserStats )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_HideVisits )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_NoCollections )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_NoPrivateSearch )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_NoPublicSearch )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_NoRecentList )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_ProtectExif )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_ProtectExtraLarge )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_ProtectLarge )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_ProtectMedium )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_ProtectOriginals )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_ProtectGuestbook )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_NoPublicGBPosts )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_NoPrivateGBPosts )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_NoAnonGBPosts )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_ProtectComments )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_NoPublicComments )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_NoPrivateComments )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_NoAnonComments )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_PasswordProtOrig )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( AM_Current->am_ProtectAll )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");


    strcat(Command,")");
    CommandLength = strlen(Command);

#ifdef DBDEBUG
    printf("    Command (%d) --> %s\n",CommandLength,Command);
#endif

    if (mysql_real_query(&QueryDB, Command, CommandLength)) {

      /*  we did lazy (no) list management - ignore duplicate entries  */

      if ( mysql_errno(&QueryDB) != 1062 ) {
	fprintf(stderr,"Command %s failed: %s\n",Command,
		mysql_error(&QueryDB));
	printf("---->  mysql_errno:  %d\n",mysql_errno(&QueryDB));
	exit(-1);
      }  /*  if not ER_DUP_ENTRY  */

    }  /*  if error on DB Command  */
    /*  QueryResult=mysql_use_result(&QueryDB);*/



    AM_Current = AM_Current->am_Next;
  }  /*  while traversing AccessMask list  */

  /*******************************************************************/
  /*  load the PhotoFlags information into the PhotoFlags table  */
  /*******************************************************************/

#ifdef DEBUG
  printf("\n\nLoading PhotoFlags table...\n");
#endif

  PF_Current = PF_Top;
  while ( PF_Current != NULL ) {
#ifdef DEBUG
    printf("  Loading %d\n",PF_Current->pf_PID);
#endif

    strcpy(Command,"insert into PhotoFlags (P_ID, PF_HasTitle, PF_HasCaption, PF_HasKeywords, PF_HasCategories, PF_HasExif, PF_HasComments) values ('");

    sprintf(StringVal,"%d",PF_Current->pf_PID);
    strcat(Command,StringVal);
    strcat(Command,"', ");

    if ( PF_Current->pf_HasTitle )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( PF_Current->pf_HasCaption )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( PF_Current->pf_HasKeywords )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( PF_Current->pf_HasCategories )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( PF_Current->pf_HasExif )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");
    strcat(Command,", ");

    if ( PF_Current->pf_HasComments )
      strcat(Command,"TRUE");
    else
      strcat(Command,"FALSE");


    strcat(Command,")");
    CommandLength = strlen(Command);

#ifdef DBDEBUG
    printf("    Command (%d) --> %s\n",CommandLength,Command);
#endif

    if (mysql_real_query(&QueryDB, Command, CommandLength)) {
      fprintf(stderr,"Command %s failed: %s\n",Command,
	      mysql_error(&QueryDB));
      exit(-1);
    }
    /*  QueryResult=mysql_use_result(&QueryDB);*/



    PF_Current = PF_Current->pf_Next;
  }  /*  while loading PhotoFlags  */


  /*******************************************************************/
  /*  load the Groups information into the GroupElement table  */
  /*******************************************************************/

#ifdef DEBUG
  printf("\n\nLoading GroupElement table...\n");
#endif

  Groups_Current = Groups_Top;
  while ( Groups_Current != NULL ) {
#ifdef DEBUG
    printf("  Loading %d\n",Groups_Current->grp_ID);
#endif
    strcpy(Command,"insert into GroupElement (GE_ID, GE_GroupIndex, GE_Title, GE_AD, GE_Owner, GE_HideBranding, GE_CreatedOn, GE_ModifiedOn) values ('");
    sprintf(StringVal,"%d",Groups_Current->grp_ID);          /*  GroupId  */
    strcat(Command,StringVal);
    strcat(Command,"', '");
    sprintf(StringVal,"%d",Groups_Current->grp_GroupIndex);  /*  GroupIndex  */
    strcat(Command,StringVal);
    strcat(Command,"', '");
    strcat(Command,Groups_Current->grp_Title);               /*  Title  */
    strcat(Command,"', '");
    sprintf(StringVal,"%d",Groups_Current->grp_AD);          /*  AD  */
    strcat(Command,StringVal);
    strcat(Command,"', '");
    strcat(Command,Groups_Current->grp_Owner);               /*  Owner  */
    strcat(Command,"', ");
    if ( Groups_Current->grp_HideBranding )
      strcat(Command,"TRUE, '");
    else                                                 /*  HideBranding  */
      strcat(Command,"FALSE, '");
    strcat(Command,Groups_Current->grp_CreatedOn);           /*  CreatedOn  */
    strcat(Command,"', '");
    strcat(Command,Groups_Current->grp_ModifiedOn);           /*  ModifiedOn  */

    strcat(Command,"')");

    CommandLength = strlen(Command);

#ifdef DBDEBUG
    printf("    Command --> %s\n",Command);
#endif

    if (mysql_real_query(&QueryDB, Command, CommandLength)) {
      fprintf(stderr,"Command %s failed: %s\n",Command,
	      mysql_error(&QueryDB));
      exit(-1);
    }
    /*  QueryResult=mysql_use_result(&QueryDB);*/


    Groups_Current = Groups_Current->grp_Next;
  }  /*  for each Groups - load into DB  */


  /*******************************************************************/
  /*  load the Gallery information into the Gallery table  */
  /*******************************************************************/

#ifdef DEBUG
  printf("\n\nLoading Gallery table...\n");
#endif

  Gal_Current = Gal_Top;
  while ( Gal_Current != NULL ) {
#ifdef DEBUG
    printf("  Loading %d\n",Gal_Current->gal_ID);
#endif

    strcpy(Command,"insert into Gallery (GAL_ID, GAL_GroupIndex, GAL_Title, GAL_AD, GAL_Owner, GAL_HideBranding, GAL_Caption, GAL_CreatedOn, GAL_ModifiedOn, GAL_PhotoCount, GAL_PhotoBytes, GAL_Views, GAL_Type, GAL_TitlePhoto, GAL_UploadUrl) values ('");
    sprintf(StringVal,"%d",Gal_Current->gal_ID);           /*  Gallery Id  */
    strcat(Command,StringVal);
    strcat(Command,"', '");
    sprintf(StringVal,"%d",Gal_Current->gal_GroupIndex);   /*  GroupIndex  */
    strcat(Command,StringVal);
    strcat(Command,"', '");
    strcat(Command,Gal_Current->gal_Title);                /*  Title  */
    strcat(Command,"', '");
    sprintf(StringVal,"%d",Gal_Current->gal_AD);           /*  AD  */
    strcat(Command,StringVal);
    strcat(Command,"', '");
    strcat(Command,Gal_Current->gal_Owner);                /*  Owner  */
    strcat(Command,"', ");
    if ( Gal_Current->gal_HideBranding )
      strcat(Command,"TRUE, '");
    else                                                 /*  HideBranding  */
      strcat(Command,"FALSE, '");
    strcat(Command,Gal_Current->gal_Caption);              /*  Caption  */
    strcat(Command,"', '");
    strcat(Command,Gal_Current->gal_CreatedOn);            /*  CreatedOn  */
    strcat(Command,"', '");
    strcat(Command,Gal_Current->gal_ModifiedOn);           /*  ModifiedOn  */
    strcat(Command,"', '");
    sprintf(StringVal,"%d",Gal_Current->gal_PhotoCount);   /*  PhotoCount  */
    strcat(Command,StringVal);
    strcat(Command,"', '");
    sprintf(StringVal,"%d",Gal_Current->gal_PhotoBytes);   /*  PhotoBytes  */
    strcat(Command,"', '");
    sprintf(StringVal,"%d",Gal_Current->gal_Views);        /*  Views  */
    strcat(Command,StringVal);
    strcat(Command,"', '");
    strcat(Command,Gal_Current->gal_Type);                 /*  Type  */
    strcat(Command,"', '");
    sprintf(StringVal,"%d",Gal_Current->gal_TitlePhoto);   /*  TitlePhoto  */
    strcat(Command,StringVal);
    strcat(Command,"', '");
    strcat(Command,Gal_Current->gal_UploadUrl);            /*  UploadUrl  */

    strcat(Command,"')");

    CommandLength = strlen(Command);

#ifdef DBDEBUG
    printf("    Command --> %s\n",Command);
#endif

    if (mysql_real_query(&QueryDB, Command, CommandLength)) {
      fprintf(stderr,"Command %s failed: %s\n",Command,
	      mysql_error(&QueryDB));
      exit(-1);
    }
    /*  QueryResult=mysql_use_result(&QueryDB);*/


    Gal_Current = Gal_Current->gal_Next;
  }  /*  for each Gallery - load into DB  */


  /*******************************************************************/
  /*  load the Photo info gathered into the Photo table */
  /*******************************************************************/

  /*  the only Photo information we get in this pass is for the  */
  /*  TitlePhoto elements.  this is a subset and the rest of it  */
  /*  will have to be gathered in another pass.  */

  /*
    Id
    Width
    Height
    Sequence
    AccessDescriptor
    Flags
    MimeType
    OriginalUrl
    UrlCore
    Urlhost
    UrlToken
  */


#ifdef DEBUG
  printf("\n\nLoading Photo table...\n");
#endif

  Photo_Current = Photo_Top;
  while ( Photo_Current != NULL ) {
#ifdef DEBUG
    printf("  Loading %d\n",Photo_Current->ph_ID);
#endif
    strcpy(Command,"insert into Photo (P_ID, P_Width, P_Height, P_Sequence, AD_ID, P_MimeType, P_OriginalUrl, P_UrlCore, P_Urlhost, P_UrlToken) values ('");
    sprintf(StringVal,"%d",Photo_Current->ph_ID);    /*  Photo ID  */
    strcat(Command,StringVal);
    strcat(Command,"', '");

    sprintf(StringVal,"%d",Photo_Current->ph_Width);
    strcat(Command,StringVal);
    strcat(Command,"', '");
    sprintf(StringVal,"%d",Photo_Current->ph_Height);
    strcat(Command,StringVal);
    strcat(Command,"', '");
    /*    strcat(Command,Photo_Current->ph_Sequence);*/
    strcat(Command,"', '");
    sprintf(StringVal,"%d",Photo_Current->ph_AD);
    strcat(Command,StringVal);
    strcat(Command,"', '");
    strcat(Command,Photo_Current->ph_MimeType);
    strcat(Command,"', '");
    strcat(Command,Photo_Current->ph_OriginalUrl);
    strcat(Command,"', '");
    strcat(Command,Photo_Current->ph_UrlCore);
    strcat(Command,"', '");
    strcat(Command,Photo_Current->ph_UrlHost);
    strcat(Command,"', '");
    strcat(Command,Photo_Current->ph_UrlToken);


    strcat(Command,"')");

    CommandLength = strlen(Command);

#ifdef DBDEBUG
    printf("    Command --> %s\n",Command);
#endif

    if (mysql_real_query(&QueryDB, Command, CommandLength)) {
      fprintf(stderr,"Command %s failed: %s\n",Command,
	      mysql_error(&QueryDB));
      exit(-1);
    }
    /*  QueryResult=mysql_use_result(&QueryDB);*/


    Photo_Current = Photo_Current->ph_Next;
  }  /*  for each Photo - load into DB  */


  /*******************************************************************/
  /*  close the database connection  */
  /*******************************************************************/

  mysql_free_result(QueryResult);
  mysql_close(&QueryDB);

  /*
    clean up and go home
  */

  /*  free data structures  */

  printf("\nFreeing data structures\n");

  printf("  Galleries...\n");
  Gal = Gal_Top;
  while ( Gal != NULL ) {
    Gal_Current = Gal;
    Gal = Gal->gal_Next;
#ifdef MEMDEBUG
    printf("    freeing a  Gallery\n");
#endif
    free(Gal_Current);
  };

  printf("  PhotoSet Groups...\n");
  PSGroups = PSGroups_Top;
  while ( PSGroups != NULL ) {
    PSGroups_Current = PSGroups;
    PSGroups = PSGroups->psgrp_Next;
#ifdef MEMDEBUG
    printf("    freeing a  PSGroup\n");
#endif
    free(PSGroups_Current);
  };

  printf("  Groups...\n");
  Groups = Groups_Top;
  while ( Groups != NULL ) {
    Groups_Current = Groups;
    Groups = Groups->grp_Next;
#ifdef MEMDEBUG
    printf("    freeing a  Groups\n");
#endif
    free(Groups_Current);
  };

  printf("  AccessDescriptors...\n");
  AD = AD_Top;
  while ( AD != NULL ) {
    AD_Current = AD;
    AD = AD->ad_Next;
#ifdef MEMDEBUG
    printf("    freeing an AD\n");
#endif
    free(AD_Current);
  };

  printf("  AccessMasks...\n");
  AM = AM_Top;
  while ( AM != NULL ) {
    AM_Current = AM;
    AM = AM->am_Next;
#ifdef MEMDEBUG
    printf("    freeing an AM\n");
#endif
    free(AM_Current);
  };

  printf("  Photos...\n");
  Photo = Photo_Top;
  while ( Photo != NULL ) {
    Photo_Current = Photo;
    Photo = Photo->ph_Next;
#ifdef MEMDEBUG
    printf("    freeing a  Photo\n");
#endif
    free(Photo_Current);
  };

  printf("  PhotoFlags...\n");
  PF = PF_Top;
  while ( PF != NULL) {
    PF_Current = PF;
    PF = PF->pf_Next;
#ifdef MEMDEBUG
    printf("    freeing a  PhotoFlag\n");
#endif
    free(PF_Current);
  };


#ifdef SICK
  printf("\n\ncough, cough\n\n");
#endif

}

#endif
