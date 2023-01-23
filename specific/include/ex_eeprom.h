#ifndef EX_EEPROM_H_
#define EX_EEPROM_H_

#include<stdint.h>
#include "scenes.h"

void ex_eeprom_store_scene_ir(scene_entry_ir_t scene_entry);
void ex_eeprom_int();
void eeprom_test(uint16_t a);
void read_scene_command(int scene_id,int command_id, scene_entry_ir_t *scene_data);
int write_command_to_eeprom(scene_entry_ir_t *scene_data);
void erase_scene(uint8_t scene_id);

#endif
