/*

  function to build the content for the LoadPhoto request

*/

/*
#undef DEBUG
#define DEBUG
*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

int BC_LoadGroupHier(char *LoginName, char *Level, char* Content)
{

  char StringVal[1024];

#ifdef DEBUG
  printf("BC_LoadGroupHier:  entry\n");
#endif

  strcpy(Content,"loginName=");
  strcat(Content,LoginName);
  strcat(Content,"&level=");
  strcat(Content,Level);

#ifdef DEBUG
  printf("BC_LoadGroupHier:  return\n");
#endif

  return(strlen(Content));
}
