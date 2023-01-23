#ifndef INCLUDE_HTTPUTILS_H_
#define INCLUDE_HTTPUTILS_H_

int extractDataArray(char *data , char *name , char **retdata,int * len, int maxRetDataLength);
int extractData(char *data , char *name , char *retdata,int * len);
int extractNumber(char *data , char *name ,int * num);
void extractStatus(char *data,int len,char *statusCheck,char *status, int *State);
void extractContentLength(char *data, int len, char **body,int *bodyLength, int *State);
void gotBody(char *data,int len, int *bodyPosition, int *State);
void extractBody(char *data,int len,int *bodyLength,int *bodyCount, int *bodyPosition,char **body, int *State);
#endif
