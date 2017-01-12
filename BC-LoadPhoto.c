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

int BC_LoadPhoto(int PhotoId, char *Level, char* Content)
{

  char StringVal[1024];

#ifdef DEBUG
  printf("BC_LoadPhoto:  entry\n");
#endif

  strcpy(Content,"photoId=");
  sprintf(StringVal,"%d",PhotoId);
  strcat(Content,StringVal);
  strcat(Content,"&level=");
  strcat(Content,Level);

#ifdef DEBUG
  printf("BC_LoadPhoto:  return\n");
#endif

  return(strlen(Content));
}
