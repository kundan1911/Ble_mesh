
#ifndef __COMMJSON_H_
#define __COMMJSON_H_

#include <stddef.h>
#ifndef NULL
#define NULL (void *)0
#endif


char *getJSONvalue_helper(char *jsonpair,char *token);
char *getJSONvalue(char *jsonstr, char *key,char *token);
 int getHexToNum(char ch);
#endif // __COMMJSON_H_