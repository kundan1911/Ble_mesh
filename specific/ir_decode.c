#include "ir_decode.h"
#include "commJSON.h"

#include <stdlib.h>
#include <string.h>

int getIndex(int token, int *list, int len){
	int i =0;
	for(i=0;i<len;i++){
		if(list[i] == token){
			return i;
		}
	}
	return -1;
}
int getBit(int count , int data){
	int a = 1;
	a = a << (7-count);
	return (data & a)>>(7-count);
}

void ir_decode_huffman(int value[], int key[], int keyLength, unsigned char encoded[], int signed charLength, char* dV){
	int code =0;
	unsigned char * running = encoded-1;
	int token = 0;
	int y= -1;
	for(int i = 0;i<charLength;i++){
		if(i%8==0)running++;
		signed char b = getBit(i%8,*running);
		token = (token<<1 ) | b;
		y = getIndex(token,key,keyLength);
		if(y != -1){
			token=0;
			dV[code++] = value[y];
		}
	}
	return;
}

void ir_decode_single(char* in_data, char** out_data, uint16_t* out_length){
	char *temp = in_data;
	(*out_length) = 1;
	while (temp[0] != '\0') {
		if (temp[0] == ',') {
			(*out_length)++;
		}
		temp++;
	}
	(*out_length)++;

	int i = 0, j = 0;
	(*out_data) = (char *)calloc((*out_length), sizeof(char));
	char *tempnam = strtok(in_data, ",");
	while (tempnam != NULL) {
		for (i = 0; i < strlen(tempnam); i++) {
			(*out_data)[j] = ((*out_data)[j] * 16) + getHexToNum(tempnam[i]);
		}
		j++;
		tempnam = strtok(NULL, ",");
	}
	return;
}

void ir_decode_double(char* in_data, char** out_data, uint16_t* out_length){
	char *temp = in_data;
	(*out_length) = 1;
	while (temp[0] != '\0')
	{
		if (temp[0] == ',')
			(*out_length)++;
		temp++;
	}
	(*out_length) = (*out_length) * 2;

	int j = 0;
	(*out_data) = (char *)calloc((*out_length), sizeof(char));
	char *tempnam = strtok(in_data, ",");
	((uint16_t *)(*out_data))[0]=atoi(tempnam);
	 while (tempnam != NULL)
	{
		((uint16_t *)(*out_data))[j] = atoi(tempnam);
		tempnam = strtok(NULL, ",");
		j++;
	}
	return;
}

void dev_id_decode_hex(char* in_data, uint8_t* out_data){
	char *temp = in_data;
	int arr_ptr=0;
	int hex_str_ptr=0;
	while(temp[hex_str_ptr]!=NULL){
		out_data[arr_ptr++]=(16*getHexToNum(temp[hex_str_ptr]))+(getHexToNum(temp[hex_str_ptr+1]));
		hex_str_ptr+=2;
	}
	return;
}