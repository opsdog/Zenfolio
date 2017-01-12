/*

  from the LoadPhotoSet XML deliver the specific photo information

*/

/*
#undef DEBUG
#define DEBUG
*/

#include <stdio.h>
#include <strings.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#ifdef LIBXML_TREE_ENABLED

#define MAXPHOTOS 2000

int main(int argc, char *argv[])
{
  int photoindex, numphotos;
  xmlChar *key = NULL;
  xmlDoc *doc;
  xmlDocPtr doc_ptr;
  xmlNode *a_node = NULL,
    *cur_node = NULL,
    *root_node = NULL,
    *photoset = NULL;
  xmlNodePtr cur_ptr = NULL;
  struct photo {
    char id[50];
    char width[50];
    char height[50];
    char title[256];
    char filename[256];
    char uploadedon[256];
    char takenon[256];
    char owner[50];
    char gallery[50];
    char views[50];
    char mime[50];
    char originalurl[256];
    char ulcore[256];
  };
  struct photo Photos[MAXPHOTOS];
  char galleryid[50];

  if ( argc != 2 ) {
    printf("Need a file to parse\n");
    return(1);
  }

#ifdef DEBUG
  printf("\nParsePhotoSet:  ENTRY\n");
#endif

  /*  initialize the library and check version compatability */

  LIBXML_TEST_VERSION ;

  /* parse the file */

  doc = xmlReadFile(argv[1], NULL, 0);

  if ( doc == NULL ) {
    printf("could not parse file %s\n",argv[1]);
    return(1);
  }

  doc_ptr = doc;

  /*  get the root element node */

  root_node = xmlDocGetRootElement(doc);
  cur_node = root_node;

  /*  first level element is PhotoSet */
  /*  second level trigger element is Elements */
  /*  we are looking for the third level elements called PhotoSet */

  /* cur_node is currently PhotoSet - dive in */

#ifdef DEBUG
  printf("Should be PhotoSet: %s\n",cur_node->name);
#endif

  cur_node = cur_node->children;

  /* 
    now at second level 
    need to grab the gallery Id while consuming until Photos node found
  */  

  while ( strcmp((char *)cur_node->name,"Photos") ) {
#ifdef DEBUG
    printf("  Checking: %s\n",cur_node->name);
#endif
    if ( strcmp((char *)cur_node->name,"Id") == 0 ) {
      cur_ptr = cur_node;
      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
      strcpy(galleryid, (char *)key);
    }
    cur_node=cur_node->next;
  }  /*  for each node */

#ifdef DEBUG
  printf("Out of second level while - should be Photos: %s\n",cur_node->name);
#endif

  /* dive in... */

  cur_node = cur_node->children;

  /* this loop will consume the rest of the nodes and parse out the Photos */

  photoindex = -1;

  while ( cur_node != NULL ) {

#ifdef DEBUG
    printf("  Checking for Photo: %s\n",cur_node->name);
#endif

    if ( strcmp((char *)cur_node->name,"Photo") == 0 ) {
#ifdef DEBUG
      printf("    Found Photo: %s\n",cur_node->name);
#endif
      photoset = cur_node->children;
      photoindex++;

      while (photoset != NULL) {
#ifdef DEBUG
	printf("      node: %s %d\n",photoset->name, photoindex);
#endif

	cur_ptr = photoset;
	key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);

	if ( strcmp((char *)photoset->name,"Id") == 0 ) {
	  strcpy(Photos[photoindex].id, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"Width") == 0 ) {
	  strcpy(Photos[photoindex].width, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"Height") == 0 ) {
	  strcpy(Photos[photoindex].height, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"Title") == 0 ) {
	  strcpy(Photos[photoindex].title, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"FileName") == 0 ) {
	  strcpy(Photos[photoindex].filename, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"UploadedOn") == 0 ) {
	  strcpy(Photos[photoindex].uploadedon, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"TakenOn") == 0 ) {
	  strcpy(Photos[photoindex].takenon, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"Owner") == 0 ) {
	  strcpy(Photos[photoindex].owner, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"Gallery") == 0 ) {
	  strcpy(Photos[photoindex].gallery, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"Views") == 0 ) {
	  strcpy(Photos[photoindex].views, (char *)key);
	}

	xmlFree(key);
	photoset=photoset->next;
      }  /*  while in the photoset */
    }  /*  if cur_node is PhotoSet */
    cur_node=cur_node->next;
  }  /* while reading the rest of the XML */

  numphotos = photoindex + 1;

  /*  we now have an array of photo information to print out */

  for (photoindex=0; photoindex < numphotos; photoindex++)
    printf("%s %s %s\n",
	   galleryid,
	   Photos[photoindex].id,
	   Photos[photoindex].filename
	   );


  /*  clean up */

  xmlFreeDoc(doc);
  xmlCleanupParser();

}


#endif
