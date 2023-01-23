#ifndef PICO_ERROR_MANAGER_H
#define PICO_ERROR_MANAGER_H

#include<stdbool.h>
#include "cJSON.h"

#define ERROR_WIFI_CONNECTION 0
#define ERROR_INTERNET_CONNECTION 1
#define ERROR_ENCRYPTION_KEY 2

extern int error_code_http;

void setErrorCode(int errorType, bool state);
void creatErrorMessage(int erroeCode,cJSON* root);

extern int ERROR_CODE_NO;

#endif