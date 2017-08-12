#ifndef CPU_FLAGS_H
#define CPU_FLAGS_H

#include <stdbool.h>

bool flg_is_C(void);
bool flg_is_Z(void);
bool flg_is_I(void);
bool flg_is_D(void);
bool flg_is_B(void);
bool flg_is_V(void);
bool flg_is_N(void);

void flg_set_C(void);
void flg_set_Z(void);
void flg_set_I(void);
void flg_set_D(void);
void flg_set_B(void);
void flg_set_V(void);
void flg_set_N(void);

void flg_clear_C(void);
void flg_clear_Z(void);
void flg_clear_I(void);
void flg_clear_D(void);
void flg_clear_B(void);
void flg_clear_V(void);
void flg_clear_N(void);

void flg_update_Z (byte value);
void flg_update_N (byte value);
void flg_update_ZN(byte value);

#endif /* CPU_FLAGS_H */
