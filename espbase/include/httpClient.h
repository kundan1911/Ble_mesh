#ifndef INCLUDE_HTTPCLIENT_H_
#define INCLUDE_HTTPCLIENT_H_

#include<stdbool.h>

#define WEB_SERVER "aws.sagaservers.com" //PicoStone Main Server
//#define WEB_SERVER "192.168.1.122"
//#define WEB_SERVER "13.126.210.17" //PicoStone BackUp Server


#define WEB_PORT "443"
//#define WEB_PORT "8002"


//****** HTTP Error code*********************
#define HTTP_INIT_FAILED 1
#define INTERNET_CONNECT_FAILED 2
#define HTTP_SERVERCONNECT_FAILED 3
#define HTTP_SERVER_LOGIN_FAILED 4
#define INTERNET_CONNECTED 5
#define HTTP_SERVER_CONNECTED 6

//******************************************


int writeData(char *data,int len);



int httpInit();
int serverConnect();
int read_ssl_Data(unsigned char **buf,int len);
void superclose();
void Httpinitclose();
int connectionCheack();
bool isOkToWrite(int timeout_ms);
bool isThereSomethingToRead(int timeout_ms);


void setSocketTimeout(int timeout);
//int writeParseKeys(bool withKey);




#endif


