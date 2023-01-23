#ifndef INCLUDE_PARSEMANAGER_H_
#define INCLUDE_PARSEMANAGER_H_

#include<stdbool.h>

#define CMD_IRDATA		5032

#define LOCAL_SOURCE_SWITCHBOARD	0
#define LOCAL_SOURCE_SMARTPHONE 	1
#define LOCAL_SOURCE_SCENE_SELF		2
#define LOCAL_SOURCE_SCENE_EXT		3

int parseRun();
void initParsemanager();
// int addToChangeLog(int port , int newstate ,int source);
int addToChangeLog(int subDeviceId ,int port , int newstate, int source, int field);
int addToReportCommand(int commandID, bool forceToReport);
int reportCommand();
int reportDeviceDetail();

#endif
