// #include <string.h>
// #include <stdlib.h>
// #include <stdio.h>

// #include "httpProtocol.h"
// #include "httpClient.h"
// #include "httpUtils.h"

// #include "pico_main.h"
// #include "error_manager.h"
// #include "wifi.h"
// #include "nvs_common.h"

// #include "esp_log.h"
// #include "esp_wifi.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "lwip/netif.h"
// #include "lwip/tcpip.h"


// static const char *TAG = "httpProtocol";

// extern const char *server_root_cert;
// int WD_HttpProtocol = 0;
// int WD_HttpProtocol_ex = 0;
// //extern const char *CURRENT_VERSION;

// char sessionToken[50];
// int sessionTokenSize;
// const int buf_len=700;

// int (*first_time_LoggedIn)() = NULL;
// int (*process_HTTP_Commands)(char *body) = NULL;
// int (*need_To_Report)() = NULL;
// int (*report_Command)() = NULL;

// char SERVER_PASSWORD[33];

// int repeatedRestart =0 ;


// void restart() {
// 	if(repeatedRestart++>5) {
// 		ESP_LOGE(TAG, "Restarting Due To Repeated HTTP Restarts");
// 		esp_restart();
// 	}
// 	superclose();
// 	vTaskDelay(150);
// }


// void workedOnce() {
// 	repeatedRestart = 0;
// }

// unsigned char *buf=NULL;
// int receiveFromServer2(char *statusCheck,char **body,int *bodyLength,char *status,bool showData) {
// 	if(buf == NULL) {
// 		buf = malloc(buf_len);
// 		if(buf==NULL) {
// 			return -4;
// 		}
// 	}
// 	int len,ret;
// 	int bodyPosition=0;
// 	int bodyCount = 0;
// 	int extractState =0;
// 	do {
// 		len = buf_len - 1;
// 		//printf("\nbuf = %s\n",buf);
// 		ret = read_ssl_Data(&buf,len);

// 		if(ret <= 0 ) {
// 			return ret;
// 		}

// 		len = ret;
// 		buf[len]='\0';

// 		if(extractState==0) {
// 			extractStatus((char *)buf,len,statusCheck,status,&extractState);
// 		}
// 		if(extractState==1) {
// 			extractContentLength((char *)buf,len,body,bodyLength,&extractState);
// 		}
// 		if(extractState==2) {
// 			gotBody((char *)buf,len,&bodyPosition,&extractState);
// 		}
// 		if(extractState==3) {
// 			extractBody((char *)buf,len,bodyLength,&bodyCount,&bodyPosition,body,&extractState);
// 		}
// 		if(extractState==4) {
// 			if(showData && bodyLength==0) {
// 				free(*body);
// 				*body=NULL;
// 				continue;
// 			}
// 			return 1;
// 		}

// 	} while(1);
// 	return -2;
// }

// int receiveFromServer(char *status,struct HTTPprotocolData *HTTPData) {
// 	if(HTTPData==NULL) {
// 		struct HTTPprotocolData tempHTTPData;
// 		tempHTTPData.body = NULL;
// 		tempHTTPData.bodyLength = 0;
// 		memset(tempHTTPData.status,0,4);

// 		int ret = receiveFromServer2(status,&tempHTTPData.body,&tempHTTPData.bodyLength,tempHTTPData.status,false);
// 		free(tempHTTPData.body);
// 		return ret;
// 	}
// 	else {
// 		int ret = receiveFromServer2(status,&HTTPData->body,&HTTPData->bodyLength,HTTPData->status,false);
// 		return ret;
// 	}
// }




// int writeHeader(char *header) {
// 	return writeData(header,(int)strlen(header));
// }

// int writeParseKeys(bool withKey) {
// 	if(writeHeader("Host: api.picostone.com\r\n")==-1) {
// 		return -1;
// 	}
// 	if(writeHeader("Connection: Keep-Alive\r\n")==-1) {
// 		return -1;
// 	}
// 	if(writeHeader("Keep-Alive: timeout=15, max=1000\r\n")==-1) {
// 		return -1;
// 	}
// 	return 0;
// }

// int endHttpWrite(void) {
// 	if(writeHeader("\r\n")==-1) {
// 		return -1;
// 	}
// 	return 0;
// }

// int send_POST(char *jsonData,char *path) {
// 	char header[60];
// 	char host[64];

// 	sprintf(host,"POST %s HTTP/1.1\r\n",path);
// 	sprintf(header,"Content-Length: %d\r\n",(int)strlen(jsonData));

// 	//startHttpParsing();
// 	if(writeHeader(host)==-1) {
// 		return -1;
// 	}
// 	if(writeParseKeys(true)==-1) {
// 		return -1;
// 	}
// 	if(writeHeader("Content-Type: application/x-www-form-urlencoded\r\n")==-1) {
// 		return -1;
// 	}
// 	if(writeHeader(header)==-1) {
// 		return -1;
// 	}
// 	if(writeHeader("\r\n")==-1) {
// 		return -1;
// 	}
// 	if(writeHeader(jsonData)==-1) {
// 		return -1;
// 	}
// 	if(endHttpWrite()==-1) {
// 		return -1;
// 	}

// 	if(receiveFromServer("200",NULL)<0) {
// 		return -1;
// 	}
// 	//startHttpParsing();
// 	return 1;
// }


// //===============================================================SUB>FILE===========



// void saveSessionToken(char *body)
// {
// 	extractData(body,"token",sessionToken,&sessionTokenSize);
// 	printf("session token = %s",sessionToken);
// }

// int loginIn(void) {
// 	char stats[150];
// 	char stats1[150];
// 	memset(stats,0,150);
// 	memset(stats1,0,150);
// 	char header[60];
// 	memset(header,0,60);

// 	setSocketTimeout(10000);

// 	char ipAddress[16];
// 	wifi_sta_get_ip(ipAddress);

// 	sprintf(stats,"username=%s&password=%s&version=%s&ipAddress=%s&wifi_ssid=%s",
// 		DEVICE_ID, SERVER_PASSWORD, ESP_FIRMWARE_VERSION, ipAddress, wifi_connected_ap_info.ssid);
// 	sprintf(stats1,"&wifi_bssid="MACSTR"&RSSI=%02d&channel=%d&partition=%d",MAC2STR(wifi_connected_ap_info.bssid),wifi_connected_ap_info.rssi,wifi_connected_ap_info.primary, (RUNNING_PARTITION[4]-'0'));
// 	printf("%s\n",stats);
// 	printf("%s\n",stats1);
// 	int hLenth = (int)strlen(stats) + (int)strlen(stats1);
// 	sprintf(header,"Content-Length: %d\r\n",hLenth);

// 	if(!isOkToWrite(1000)) {
// 		return -1;
// 	}

// 	if(writeHeader("POST /device/authenticate HTTP/1.1\r\n")==-1) {
// 		return -1;
// 	}
// 	if(writeParseKeys(true)==-1) {
// 		return -1;
// 	}
// 	if(writeHeader("Content-Type: application/x-www-form-urlencoded\r\n")==-1) {
// 		return -1;
// 	}
// 	if(writeHeader(header)==-1) {
// 		return -1;
// 	}
// 	if(writeHeader("\r\n")==-1) {
// 		return -1;
// 	}
// 	if(writeHeader(stats)==-1) {
// 		return -1;
// 	}
// 	if(writeHeader(stats1)==-1) {
// 		return -1;
// 	}
// 	if(endHttpWrite()==-1) {
// 		return -1;
// 	}
// 	struct HTTPprotocolData tempHTTPData;
// 	tempHTTPData.body = NULL;
// 	tempHTTPData.bodyLength = 0;
// 	memset(tempHTTPData.status,0,4);
// 	if(receiveFromServer("200",&tempHTTPData)<0) {
// 		return -1;
// 		free(tempHTTPData.body);
// 	}
// 	saveSessionToken(tempHTTPData.body);
// 	free(tempHTTPData.body);
// 	return 1;
// }

// //POST /commanddone
// //command objectid and token
// int deleteCommand(char *objId,int res) {
// 	char stats[100];
// 	memset(stats,0,100);
// 	char header[60];
// 	memset(header,0,60);
// 	sprintf(stats,"objectid=%s&token=%s&res=%d",objId,sessionToken,res);
// 	sprintf(header,"Content-Length: %d\r\n",(int)strlen(stats));

// 	if(writeHeader("POST /commanddone HTTP/1.1\r\n")==-1) {
// 		return -1;
// 	}
// 	if(writeParseKeys(true)==-1) {
// 		return -1;
// 	}
// 	if(writeHeader("Content-Type: application/x-www-form-urlencoded\r\n")==-1) {
// 		return -1;
// 	}
// 	if(writeHeader(header)==-1) {
// 		return -1;
// 	}
// 	if(writeHeader("\r\n")==-1) {
// 		return -1;
// 	}

// 	if(writeHeader(stats)==-1) {
// 		return -1;
// 	}
// 	if(endHttpWrite()==-1) {
// 		return -1;
// 	}
// 	if(receiveFromServer("200",NULL)<0) {
// 		return -1;
// 	}
// 	return 0;
// }



// /*********
// Long Polling Section
// *************/


// int startLongPolling(void) {
// 	setSocketTimeout(120000);
// 	char stats[75];
// 	memset(stats,0,75);
// 	char header[60];
// 	memset(header,0,60);

// 	sprintf(stats,"token=%s",sessionToken);
// 	sprintf(header,"Content-Length: %d\r\n",(int)strlen(stats));

// 	if(writeHeader("POST /command/startLongPoll HTTP/1.1\r\n")==-1) {
// 		return -1;
// 	}
// 	if(writeParseKeys(true)==-1) {
// 		return -1;
// 	}
// 	if(writeHeader("Content-Type: application/x-www-form-urlencoded\r\n")==-1) {
// 		return -1;
// 	}
// 	if(writeHeader(header)==-1) {
// 		return -1;
// 	}
// 	if(writeHeader("\r\n")==-1) {
// 		return -1;
// 	}
// 	if(writeHeader(stats)==-1) {
// 		return -1;
// 	}
// 	if(endHttpWrite()==-1) {
// 		return -1;
// 	}
// 	return 0;
// }

// int endLongPolling(void) {
// 	setSocketTimeout(10000);
// 	char stats[75];
// 	memset(stats,0,75);
// 	char header[60];
// 	memset(header,0,60);

// 	sprintf(stats,"token=%s",sessionToken);
// 	sprintf(header,"Content-Length: %d\r\n",(int)strlen(stats));

// 	if(!isOkToWrite(1000)) {
// 		return -1;
// 	}

// 	if(writeHeader("POST /command/endLongPoll HTTP/1.1\r\n")==-1) {
// 		return -1;
// 	}
// 	if(writeParseKeys(true)==-1) {
// 		return -1;
// 	}
// 	if(writeHeader("Content-Type: application/x-www-form-urlencoded\r\n")==-1) {
// 		return -1;
// 	}
// 	if(writeHeader(header)==-1) {
// 		return -1;
// 	}
// 	if(writeHeader("\r\n")==-1) {
// 		return -1;
// 	}

// 	if(writeHeader(stats)==-1) {
// 		return -1;
// 	}
// 	if(endHttpWrite()==-1) {
// 		return -1;
// 	}

// 	int i=0;
// 	for (i = 0; i<2; i++) {
// 		if(!isThereSomethingToRead(3000)) {
// 			return -1;
// 		}
// 		struct HTTPprotocolData tempHTTPData;
// 		tempHTTPData.body = NULL;
// 		tempHTTPData.bodyLength = 0;
// 		memset(tempHTTPData.status,'\0',4);

// 		int ret = receiveFromServer("200",&tempHTTPData);
// 		if(ret<0){
// 			free(tempHTTPData.body);
// 			return ret;
// 		}
// 		int result =strcmp(tempHTTPData.body, "\"Removed\"");
// 		free(tempHTTPData.body);
// 		if(result == 0) {
// 			return 0;
// 		}
// 	}
// 	return -1;
// }

// int HelpCheckInLongPoll(struct HTTPprotocolData *tempHTTPData) {
// 	int pc = process_HTTP_Commands(tempHTTPData->body);
// 	char objid[60];
// 	memset(objid,0,60);
// 	int length;
// 	extractData(tempHTTPData->body,"_id",objid,&length);
// 	if(objid[0]!='\0'){
// 		int ret = -1;
// 		ret = deleteCommand(objid,pc);
// 		if(ret<0) {
// 			return -1;
// 		}
// 	}
// 	return 1;
// }

// int checkInLongPoll(void) {
// 	for(int i=0;i<=60;i++) {
// 		WD_HttpProtocol = 0;
// 		WD_HttpProtocol_ex = 0;

// 		if (wifi_sta_get_connected_status()!=true) {
// 			return -1;
// 		}

// 		if(!isThereSomethingToRead(200)) {
// 			workedOnce();
// 			if(need_To_Report()) {
// 				printf("Need to report something\n");
// 				return 1;
// 			}
// 		}
// 		else {
// 			struct HTTPprotocolData tempHTTPData;
// 			tempHTTPData.body = NULL;
// 			tempHTTPData.bodyLength = 0;
// 			memset(tempHTTPData.status,0,4);
// 			//buf[len]='\0';
// 			int ret = 0;
// 			if(receiveFromServer("200",&tempHTTPData)<0) {
// 				//printf("asdssdd10");
// 				free(tempHTTPData.body);
// 				return -1;
// 			}
// 			else {
// 				ret = HelpCheckInLongPoll(&tempHTTPData);
// 			}
// 			free(tempHTTPData.body);
// 			return ret;
// 		}
// 	}
// 	return 0;
// }

// int afterLogin()
// {
// 	if (startLongPolling() < 0) {
// 		return -1;
// 	}
// 	if (checkInLongPoll() < 0) {
// 		return -1;
// 	}
// 	if (endLongPolling() < 0) {
// 		return -1;
// 	}
// 	if (report_Command() < 0) {
// 		return -1;
// 	}
// 	return 1;
// }

// int afterLoginCall() {
// 	int ret = 0;
// 	do{
// 		workedOnce();
// 		WD_HttpProtocol = 0;
// 		WD_HttpProtocol_ex = 0;
// 		ret = afterLogin();
// 	}while(ret >= 0);
// 	return ret;
// }

// int doWithRetry(int f() , int retries , int Delay) {
// 	int retryCounter = retries;
// 	int ret = 0;
// 	while(retryCounter-->0) {
// 		WD_HttpProtocol = 0;
// 		ret = f();
// 		if(ret==1) {
// 			return 1;
// 		}
// 		if(Delay > 0) {
// 			vTaskDelay(Delay);
// 		}
// 	}
// 	return ret;
// }

// void HTTPerrorDetection(int errorData) {
// 	if (errorData < 0) {
// 		return;
// 	}
// 	if(errorData == error_code_http) {
// 		return;
// 	}
// 	error_code_http = errorData;

// 	if(errorData == HTTP_SERVER_CONNECTED) {
// 		setErrorCode(ERROR_INTERNET_CONNECTION,false);
// 	}
// 	else {
// 		setErrorCode(ERROR_INTERNET_CONNECTION,true);
// 	}

// 	printf("HTTPerrorDetection error = %d",errorData);
// }

// int parseRun() {
// 	if (httpInit() != 1) {
// 		return -1;
// 	}
// 	WD_HttpProtocol = 0;
// 	vTaskDelay(150);
// 	if (doWithRetry(serverConnect,2,4) != 1) {
// 		return HTTP_SERVERCONNECT_FAILED;
// 	}
// 	WD_HttpProtocol = 0;
// 	if (loginIn() != 1) {
// 		return HTTP_SERVER_LOGIN_FAILED;
// 	}
// 	WD_HttpProtocol = 0;

// 	vTaskDelay(10);
// 	HTTPerrorDetection(HTTP_SERVER_CONNECTED);
// 	if (first_time_LoggedIn() < 0) {
// 		return -1;
// 	}
// 	return afterLoginCall();
// }

// void Define_parseRun(int(*first_time1)(), int(*processHTTPCommands1)(char *body), int(*needToReport1)(), int(*reportCommand1)()){
// 	first_time_LoggedIn = first_time1;
// 	process_HTTP_Commands = processHTTPCommands1;
// 	need_To_Report = needToReport1;
// 	report_Command = reportCommand1;
// }

// bool changedNetifDefault = false;

// void parseRunCall() {
// 	memset(SERVER_PASSWORD,0,sizeof(SERVER_PASSWORD));
// 	if (nvs_get_srv_password(SERVER_PASSWORD)) {
// 		esp_restart();
// 	}
// 	while(1) {
// 		WD_HttpProtocol = 0;
// 		//printf("Free Heap : %d", heap_caps_get_free_size(MALLOC_CAP_8BIT));
// 		int ret = connectionCheack();
// 		if (ret == INTERNET_CONNECTED) {
// 			if(!changedNetifDefault) {
// 				LOCK_TCPIP_CORE();
// 				netif_set_default(netif_list->next);
// 				changedNetifDefault = true;
// 				UNLOCK_TCPIP_CORE();
// 			}

// 			ret = parseRun();
// 			restart();
// 			}
// 		HTTPerrorDetection(ret);
// 		vTaskDelay(50);
// 	}
// }


// void initParseRun() {
// 	xTaskCreate(parseRunCall, "tsk10", 8192, NULL, 5, NULL);
// }