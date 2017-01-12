/*

  function to build the header for a ZenFolio request

*/

/*
#undef DEBUG
#define DEBUG
*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

int BH_LoadGroupHier(char *AuthToken, int ContentLength, char* Header)
{

  char StringVal[1024];

#ifdef DEBUG
  printf("BH_LoadGroupHier:  entry\n");
#endif

  sprintf(StringVal,"%d",ContentLength);


  strcpy(Header,"POST /api/1.6/zfapi.asmx/LoadGroupHierarchy HTTP/1.1\n");
  /*  strcat(Header,"Host: www.zenfolio.com\n");*/
  strcat(Header,"Host: api.zenfolio.com\n");
  strcat(Header,"Content-Type: application/x-www-form-urlencoded\n");
  /*  strcat(Header,"Content-Type: text/xml; charset=utf-8\n");*/
  strcat(Header,"User-Agent:  Daemony Database Processor\n");
  strcat(Header,"X-Zenfolio-User-Agent: Daemony Database Processor\n");
  strcat(Header,"Content-Length: ");
  strcat(Header,StringVal);
  strcat(Header,"\nX-Zenfolio-Token: ");
  strcat(Header,AuthToken);
  strcat(Header,"\n\n");

#ifdef DEBUG
  printf("BH_LoadGroupHier:  return\n");
#endif

  return(strlen(Header));
}
