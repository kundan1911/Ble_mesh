#ifndef RMT_TX_H_
#define RMT_TX_H_

#include<stdint.h>

void rmt_tx_int();
int send_ir_data(char *node_id, char *pipe_addr, char *data, uint16_t size, int *timerNow, char *res);
int send_ir_data_new(char *node_id, uint16_t *data, uint16_t SAMPLE_CNT,int multiplier);

#endif
