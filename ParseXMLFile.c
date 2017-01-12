/*

  returns the name and contents from an XML file

*/

#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#ifdef LIBXML_TREE_ENABLED


static void
print_element_names(xmlDoc *doc, xmlNode * a_node)
{
  xmlNode *cur_node = NULL;
  xmlNodePtr cur_ptr = NULL;
  xmlDocPtr doc_ptr = NULL;
  xmlChar *key = NULL;

  doc_ptr = doc;

  for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      cur_ptr = cur_node;
      key = xmlNodeListGetString(doc_ptr, cur_ptr->xmlChildrenNode, 1);
      if ( key[0] != '\n')
	printf("%s %s\n",cur_node->name, key);
      else
	printf("%s\n",cur_node->name);

      xmlFree(key);
    }  /*  if element node */

    print_element_names(doc, cur_node->children);
  }
}


int main(int argc, char *argv[]) {
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;

  if ( argc != 2 ) {
    printf("Need a file to read...\n");
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

  /*  get the root element node */

  root_element = xmlDocGetRootElement(doc);

  print_element_names(doc, root_element);


  /*  clean up */

  xmlFreeDoc(doc);
  xmlCleanupParser();

}

#endif
