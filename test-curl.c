#include <stdio.h>
#include <strings.h>

#include <curl/curl.h>

int main (int argc, char *argv[])
{
  char url[50];

  CURLcode code;
  curl_version_info_data *version_info;

  strcpy(url, "http://www.daemony.com/curltest.html");

  code = curl_global_init(CURL_GLOBAL_ALL);

  printf("code %d\n",(int)code);

  version_info = curl_version_info(CURLVERSION_NOW);

  printf("  age %d\n",(int)version_info->age);
  printf("  version %s\n",version_info->version);
  printf("  host %s\n",version_info->host);
  printf("  ssl %s\n",version_info->ssl_version);
  printf("  libssh %s\n",version_info->libssh_version);

  printf("\n%s\n",curl_version());

  curl_global_cleanup();

}
