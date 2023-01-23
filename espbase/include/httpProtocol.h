#ifndef INCLUDE_HTTPCProtocol_H_
#define INCLUDE_HTTPCProtocol_H_

#include<stdbool.h>

struct HTTPprotocolData
{
	int bodyLength;
	char *body;
	char status[4];
};

extern int WD_HttpProtocol;
extern int WD_HttpProtocol_ex;

int writeParseKeys(bool withKey);
int writeHeader(char *header);
//int receiveFromServer(char *status,char **body,int *bodyLength);
int receiveFromServer(char *status,struct HTTPprotocolData *HTTPData);
int endHttpWrite(void);
//void startHttpParsing();

int loginIn(void);
//int getCommandPostAndDelete();
int startLongPolling(void);
int endLongPolling(void);
int checkInLongPoll(void);

int parseRun(void);
void initParseRun(void);





//void Define_parseRun(int(*first_time1)());
void Define_parseRun(int(*first_time1)(), int(*processHTTPCommands1)(), int(*needToReport1)(), int(*reportCommand1)());


#endif


