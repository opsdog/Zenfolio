/*
  function to return the next space-seperated token from a string of tokens
*/

/*
#undef DEBUG
#define DEBUG
*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

char *NextTokenTrim(char *srcString)
{
  int srcStringLength,
    targetIndex,
    srcIndex;

  char *Token;

#ifdef DEBUG
  printf("==>  NextTokenTrim:  entry\n");
  printf("==>  NextTokenTrim:  entry %d\n",(int)srcString);
  printf("==>  NextTokenTrim:  entry %s\n",srcString);
#endif

  /*  set stuff up  */

  srcStringLength = strlen(srcString);

  Token = (char *)malloc(srcStringLength);

  Token[0]='\0';
  srcIndex = 0;

  /*  find the first token and remove it from the beginning of srcString  */

  while ( ( srcString[srcIndex] != ' ' ) & ( srcString[srcIndex] != '\0' ) ) {
#ifdef DEBUG
    printf("==>  NextTokenTrim:    top of while %c\n",srcString[srcIndex]);
#endif
    Token[srcIndex] = srcString[srcIndex];

    srcIndex++;
  }  /*  while scanning for first token in srcString  */
  Token[srcIndex]='\0';
#ifdef DEBUG
  printf("==>  NextTokenTrim:  out of while %d\n",srcIndex);
#endif

  if (srcString[srcIndex] == '\0' )
    srcIndex--;
  srcIndex++;
  targetIndex = 0;

#ifdef DEBUG
  printf("==>  NextTokenTrim:  eating first token %d %d\n",srcIndex, targetIndex);
#endif

  while ( srcString[srcIndex] != '\0' ) {
    srcString[targetIndex] = srcString[srcIndex];

    srcIndex++;
    targetIndex++;
  }  /*  while removing first token  */
  srcString[targetIndex]='\0';


  /*  clean up and go home  */

#ifdef DEBUG
  printf("==>  NextTokenTrim:  exit %s ->%s<-\n",Token,srcString);
#endif
  return(Token);

}
