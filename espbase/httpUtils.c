#include "httpUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int extractDataArray(char *data , char *name , char **retdata,int * len, int maxRetDataLength)
{
	char *retData1 = *retdata;
	char buf[50];
	snprintf(buf, sizeof buf, "%s%s%s", "\"", name, "\"");
	char *foo =strstr(data,buf);
	if(foo!=NULL)
	{
		char *foo1 = strchr(foo,':');

		if(foo1!=NULL)
		{
			char *tempnam,*tempnam1 = strdup(foo);
			if (tempnam1 != NULL)
			{
				tempnam = strtok (tempnam1,":,]}");
				if (tempnam != NULL)
				{
					tempnam = strtok (NULL, "}]");
					if(tempnam == NULL){
						len = 0;
						return -1;
					}
					tempnam = strchr(tempnam,'[');
					if (tempnam != NULL && !(tempnam[0] == '{') && tempnam[0] == '[')
					{
						tempnam = strtok (tempnam, "[");
						if(tempnam == NULL)
						{
							len = 0;
							return -1;
						}
						if(tempnam != NULL)
						{

							tempnam = strdup(tempnam);
							char *found;
							int count=0;
							while( (found = strsep(&tempnam,",")) != NULL && (count<maxRetDataLength)){
								found = strchr(found,'\"');
								found = strtok (found, "\"");
								strcpy(retData1,found);
								retData1 = retData1 + 10;
								count++;
							}
							*len = count;
							free(tempnam1);
							free(tempnam);
							return 1;
						}

					}
				}
				free(tempnam1);
			}
		}
	}
	return -1;

}

int extractData(char *data , char *name , char *retdata,int * len)
{
	//memset(retdata,0,50);
	char buf[50];
	snprintf(buf, sizeof buf, "%s%s%s", "\"", name, "\"");
	//printf("foo1 = %s\n",buf);
	char *foo =strstr(data,buf);
	//printf("foo1 = %s\n",foo);
	if(foo!=NULL)
	{
		char *foo1 = strchr(foo,':');
		//printf("foo2 = %s\n",foo1);
		if(foo1!=NULL)
		{
			char *tempnam,*tempnam1 = strdup(foo);
			if (tempnam1 != NULL)
			{
				//printf("tempnam = %s tempnam1 = %s\n",tempnam,tempnam1);
				tempnam = strtok (tempnam1,":,]}");
				//free(tempnam1);
				//printf("tempnam2 = %s\n",tempnam);
				if (tempnam != NULL)
				{
					tempnam = strtok (NULL, ",}]");
					//printf("tempnam3 = %s\n",tempnam);
					if (tempnam != NULL && !(tempnam[0] == '{') && !(tempnam[0] == '['))
					{
						tempnam = strtok (tempnam, "\"");

						if(tempnam != NULL)
						{
							tempnam = strdup(tempnam);
							//printf("length = %d\n",strlen(tempnam));
							//printf("tempnam4ddddddddddddddddddddd = %s\n",tempnam);
							*len = strlen(tempnam);
							strcpy (retdata, tempnam);
							free(tempnam1);
							free(tempnam);
							//free(foo);
							return 1;
						}

					}
				}
				free(tempnam1);
			}
		}
	}
	return -1;

}
int extractNumber(char *data , char *name ,int * num)
{
	char retdata[10];
	memset(retdata,0,10);
	char buf[50];
	snprintf(buf, sizeof buf, "%s%s%s", "\"", name, "\"");
	char *foo =strstr(data,buf);
	if(foo!=NULL)
	{
		//printf("foo1 = %s\n",foo);
		foo = strchr(foo,':');
		//printf("foo2 = %s\n",foo);
		if(foo!=NULL)
		{
			char*too1=strchr(foo,',');
			char*too2=strchr(foo,']');
			char*too3=strchr(foo,'}');
			int i=0;
			foo++;
			do {
				retdata[i++]=*foo;
				foo++;
				//printf("digit = %c\n",retdata[i-1]);
				//printf("is digit = %d\n",isdigit(retdata[i-1]));
				if(!(isdigit((unsigned char)retdata[i-1])>0))
				{
					return -1;
				}

			}while((too1!=foo) && (too2!=foo) && (too3!=foo));
			//printf("foo4 = %d\n",retdata[1]);
			//printf("foo5 = %s\n",isdigit(retdata));

			*num = atoi(retdata);
			return 1;

		}
	}
	return -1;
}


void extractStatus(char *data,int len,char *statusCheck1,char *status, int *State)
{
	char statusCheck[10];

	snprintf(statusCheck, sizeof statusCheck, "%s%s", statusCheck1, "\0");

	int lineend=0;
	int i=0;
	for(i=0;i<len-1;i++) {
		if(data[i]=='\r' && data[i+1]=='\n')
			{lineend = i;
				break;
			}
	}
	char line[50];

	if(lineend>49) {
		lineend = 49;
	}

	strncpy(line,data,lineend);

	line[lineend]='\0';


	if(lineend>0) {
		if(strstr(line,"HTTP/")!=NULL){
			char *firstspace = strchr(line,' ');
			firstspace = strtok (firstspace," ");
			if(firstspace != NULL) {
				strcpy (status, firstspace);
				if(strcmp(status,statusCheck) == 0) {
					*State = 1;
					return;
				}
			}
		}
	}
	*State = 0;
	return;
}

void extractContentLength(char *data, int len, char **body,int *bodyLength, int *State)
{
	char *temp = strstr(data,"Content-Length: ");
	//printf("temp = %s\n",data);
	if(temp!=NULL)
	{
		temp=temp+16;
		char *endding = strchr(temp,'\r');
		if(endding!=NULL) {
			char length[5];
			memset(length,0,5);
			int i=0;
			for(;temp!=endding;temp++) {
				length[i++]=*temp;
			}
			int d = atoi((const char*)length);
			#ifdef debug
			printf("length is %d",d);
			#endif

			*body=(char*)malloc(d+1);
			if(*body==NULL) {
				printf("eCl:malloced Failed");
				return;
			}
			else {
				*State=2;
			}
			memset(*body,0,d+1);
			*bodyLength=d;
		}
	}
}


void gotBody(char *data,int len, int *bodyPosition, int *State) {
	char *foo = strstr(data,"\r\n\r\n");
	if(foo!=NULL) {
		*bodyPosition = foo+4-data;
		*State=3;
	}
}

void extractBody(char *data,int len,int *bodyLength,int *bodyCount, int *bodyPosition,char **body, int *State) {
	char *foo = data + *bodyPosition;
	for(;*bodyCount<*bodyLength&&foo-data<len;foo++) {
		(*body)[(*bodyCount)++]=*foo;
	}
	if(*bodyCount==*bodyLength) {
		*State=4;
	}
	*bodyPosition=0;
}