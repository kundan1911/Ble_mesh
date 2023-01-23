#ifndef PICO_IR_DECODE_H
#define PICO_IR_DECODE_H

#include<stdint.h>

void ir_decode_huffman(int value[], int key[], int keyLength, unsigned char encoded[], int signed charLength, char* dV);

void ir_decode_double(char* in_data, char** out_data, uint16_t* out_length);

void ir_decode_single(char* in_data, char** out_data, uint16_t* out_length);

void dev_id_decode_hex(char* in_data, uint8_t* out_data);

#endif