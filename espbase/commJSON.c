#ifndef __COMMJSON_C_
#define __COMMJSON_C_

#include <string.h>
#include <stdlib.h>
#include "commJSON.h"

char *getJSONvalue_helper(char *jsonpair,char *token)
{
	char *tempnam,*tempnam1 = strdup(jsonpair);
	tempnam = strtok (tempnam1,token);
	if (tempnam != NULL)
	{
	tempnam = strtok (NULL, token);
	if(tempnam ==NULL) {
		free(tempnam1);
		return NULL;}
	tempnam = strdup(tempnam);
	free(tempnam1);
	return tempnam;

}
else return NULL;

}

char *getJSONvalue(char *jsonstr, char *key,char *token)
{
	char *tempnam;

	tempnam = strstr(jsonstr,key);
	if(tempnam!=	NULL)
		return getJSONvalue_helper(tempnam,token);

	else return NULL;
}


 int getHexToNum(char ch){
	int num=0;
	if(ch>='0' && ch<='9')
	{
		num=((int)ch)-48;

	}
	else
	{
		switch(ch)
		{
			case 'A': case 'a': num=10; break;
			case 'B': case 'b': num=11; break;
			case 'C': case 'c': num=12; break;
			case 'D': case 'd': num=13; break;
			case 'E': case 'e': num=14; break;
			case 'F': case 'f': num=15; break;
			default: num=0;
			}
	}
	return num;
}
#endif // __COMMJSON_C_
