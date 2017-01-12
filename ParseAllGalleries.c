/*

  from the LoadGalleries XML deliver the specific gallery information

*/

/*#undef DEBUG*/
/*#define DEBUG*/

#include <stdio.h>
#include <strings.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#ifdef LIBXML_TREE_ENABLED

#define MAXGALLERIES 256

int main(int argc, char *argv[])
{
  int galindex, numgals;
  xmlChar *key = NULL;
  xmlDoc *doc;
  xmlDocPtr doc_ptr;
  xmlNode *a_node = NULL,
    *cur_node = NULL,
    *root_node = NULL,
    *photoset = NULL,
    *group = NULL,
    *element = NULL;
  xmlNodePtr cur_ptr = NULL;
  struct gallery {
    char id[50];
    char groupindex[50];
    char title[100];
    char owner[50];
    char caption[2048];
    char createdon[100];
    char modifiedon[100];
    char photocount[50];
    char photobytes[60];
    char views[50];
    char type[50];
  };
  struct gallery Galleries[MAXGALLERIES];

  if ( argc != 2 ) {
    printf("Need a file to parse\n");
    return(1);
  }

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

  /*  first level element is Group */
  /*  second level trigger element is Elements */
  /*  we are looking for the third level elements called PhotoSet */

  /*  Groups in the third level now contain PhotoSet objects */

  /* cur_node is currently group - dive in */

#ifdef DEBUG
  printf("Should be Group: %s\n",cur_node->name);
#endif

  cur_node = cur_node->children;

  /* now at second level */
  /* consume until Elements node found */

  while ( strcmp((char *)cur_node->name,"Elements") ) {
#ifdef DEBUG
    printf("  Checking: %s\n",cur_node->name);
#endif
    cur_node=cur_node->next;
  }  /*  for each node */

#ifdef DEBUG
  printf("Out of second level while - should be Elements: %s\n",cur_node->name);
#endif

  /* dive in... */

  cur_node = cur_node->children;

  /* this loop will consume the rest of the nodes and parse out the PhotoSets */

  galindex = -1;

  while ( cur_node != NULL ) {

    if ( strcmp((char *)cur_node->name,"Group") == 0 ) {
#ifdef DEBUG
      printf("  Found a third level group\n");
#endif
      group = cur_node->children;
      while ( group != NULL ) {
#ifdef DEBUG
	printf("    node: %s %d\n",group->name, galindex);
#endif

	/*  PhotoSets within the group will be in Elements */

	if ( strcmp((char *)group->name,"Elements") == 0 ) {
#ifdef DEBUG
	  printf("    Found Elements\n");
#endif
	  element = group->children;

	  while ( element != NULL ) {
#ifdef DEBUG
	    printf("      node: %s %d\n",element->name, galindex);
#endif

	    if ( strcmp((char *)element->name,"PhotoSet") == 0 ) {
#ifdef DEBUG
	      printf("      Found PhotoSet\n");
#endif

	      photoset = element->children;

	      galindex++;

	      while (photoset != NULL) {
#ifdef DEBUG
		printf("        node: %s %d\n",photoset->name, galindex);
#endif

		cur_ptr = photoset;
		key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);

		if ( strcmp((char *)photoset->name,"Id") == 0 ) {
		  strcpy(Galleries[galindex].id, (char *)key);
		}
		if ( strcmp((char *)photoset->name,"GroupIndex") == 0 ) {
		  strcpy(Galleries[galindex].groupindex, (char *)key);
		}
		if ( strcmp((char *)photoset->name,"Title") == 0 ) {
		  strcpy(Galleries[galindex].title, (char *)key);
		}
		if ( strcmp((char *)photoset->name,"Owner") == 0 ) {
		  strcpy(Galleries[galindex].owner, (char *)key);
		}
		if ( strcmp((char *)photoset->name,"Caption") == 0 ) {
		  strcpy(Galleries[galindex].caption, (char *)key);
		}
		if ( strcmp((char *)photoset->name,"CreatedOn") == 0 ) {
		  strcpy(Galleries[galindex].createdon, (char *)key);
		}
		if ( strcmp((char *)photoset->name,"ModifiedOn") == 0 ) {
		  strcpy(Galleries[galindex].modifiedon, (char *)key);
		}
		if ( strcmp((char *)photoset->name,"PhotoCount") == 0 ) {
		  strcpy(Galleries[galindex].photocount, (char *)key);
		}
		if ( strcmp((char *)photoset->name,"PhotoBytes") == 0 ) {
		  strcpy(Galleries[galindex].photobytes, (char *)key);
		}
		if ( strcmp((char *)photoset->name,"Views") == 0 ) {
		  strcpy(Galleries[galindex].views, (char *)key);
		}
		if ( strcmp((char *)photoset->name,"Type") == 0 ) {
		  strcpy(Galleries[galindex].type, (char *)key);
		}

		xmlFree(key);

		photoset=photoset->next;
	      }  /*  while parsing PhotoSet */
	    }  /*  if in fourth level PhotoSet  */

	    element = element->next;
	  }  /*  while in fourth level Element */
	}  /*  if fourth level Element */

	group=group->next;
      }  /*  while in the group */
    }  /*  if third level group */

    if ( strcmp((char *)cur_node->name,"PhotoSet") == 0 ) {
#ifdef DEBUG
      printf("Found PhotoSet: %s\n",cur_node->name);
#endif
      photoset = cur_node->children;
      galindex++;

      while (photoset != NULL) {
#ifdef DEBUG
	printf("  node: %s %d\n",photoset->name, galindex);
#endif

	cur_ptr = photoset;
	key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);

	if ( strcmp((char *)photoset->name,"Id") == 0 ) {
	  strcpy(Galleries[galindex].id, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"GroupIndex") == 0 ) {
	  strcpy(Galleries[galindex].groupindex, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"Title") == 0 ) {
	  strcpy(Galleries[galindex].title, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"Owner") == 0 ) {
	  strcpy(Galleries[galindex].owner, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"Caption") == 0 ) {
	  strcpy(Galleries[galindex].caption, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"CreatedOn") == 0 ) {
	  strcpy(Galleries[galindex].createdon, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"ModifiedOn") == 0 ) {
	  strcpy(Galleries[galindex].modifiedon, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"PhotoCount") == 0 ) {
	  strcpy(Galleries[galindex].photocount, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"PhotoBytes") == 0 ) {
	  strcpy(Galleries[galindex].photobytes, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"Views") == 0 ) {
	  strcpy(Galleries[galindex].views, (char *)key);
	}
	if ( strcmp((char *)photoset->name,"Type") == 0 ) {
	  strcpy(Galleries[galindex].type, (char *)key);
	}

	xmlFree(key);
	photoset=photoset->next;
      }  /*  while in the photoset */
    }  /*  if cur_node is PhotoSet */
    cur_node=cur_node->next;
  }  /* while reading the rest of the XML */

  numgals = galindex + 1;

  /*  we now have an array of gallery information to print out */

  for (galindex=0; galindex < numgals; galindex++)
    printf("%s \"%s\"\n",
	   Galleries[galindex].id,
	   Galleries[galindex].title
	   );



  /*  clean up */

  xmlFreeDoc(doc);
  xmlCleanupParser();

}


#endif
