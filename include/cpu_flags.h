#ifndef CPU_FLAGS_H
#define CPU_FLAGS_H

#include <stdbool.h>
#include "common.h"

typedef enum { ADC, BIT, CMP, ROL, ROR, SBC } Operation;

bool flg_is_C(void);
bool flg_is_Z(void);
bool flg_is_I(void);
bool flg_is_D(void);
bool flg_is_V(void);
bool flg_is_N(void);

void flg_set_C(void);
void flg_set_Z(void);
void flg_set_I(void);
void flg_set_D(void);
void flg_set_V(void);
void flg_set_N(void);

void flg_clear_C(void);
void flg_clear_Z(void);
void flg_clear_I(void);
void flg_clear_D(void);
void flg_clear_V(void);
void flg_clear_N(void);

void flg_reset(void);

void flg_update_Z (byte value);
void flg_update_N (byte value);
void flg_update_ZN(byte value);
void flg_update_C (int result, Operation type);
void flg_update_V (byte s, byte a, byte b);
void flg_update_V_bit(byte s);

byte flg_get_status(bool B);
void flg_set_status(byte status);

#endif /* CPU_FLAGS_H */
