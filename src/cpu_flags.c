#include "../include/common.h"
#include "../include/cpu_flags.h"

static byte result_Z;  /* Last result that affected the Z flag. */
static byte result_N;  /* Last result that affected the N flag. */
static bool I, D, B;   /* Interrupt disable, decimal mode flag, break command. */
static bool C, V;      /* Carry and Overflow Flags. */

inline bool flg_is_C(void) { return C; }
inline bool flg_is_Z(void) { return result_Z == 0; }
inline bool flg_is_I(void) { return I; }
inline bool flg_is_D(void) { return D; }
inline bool flg_is_B(void) { return B; }
inline bool flg_is_V(void) { return V; }
inline bool flg_is_N(void) { return result_N & 0x80; }

inline void flg_set_C(void) { C = true; }
inline void flg_set_Z(void) { result_Z = 0; }
inline void flg_set_I(void) { I = true; }
inline void flg_set_D(void) { D = true; }
inline void flg_set_B(void) { B = true; }
inline void flg_set_V(void) { V = true; }
inline void flg_set_N(void) { result_N = 0x80; }

inline void flg_clear_C(void) { C = false; }
inline void flg_clear_Z(void) { result_Z = 1; }
inline void flg_clear_I(void) { I = false; }
inline void flg_clear_D(void) { D = false; }
inline void flg_clear_B(void) { B = false; }
inline void flg_clear_V(void) { V = false; }
inline void flg_clear_N(void) { result_N = 0; }

inline void flg_update_Z (byte value) { result_Z = value; }
inline void flg_update_N (byte value) { result_N = value; }
inline void flg_update_ZN(byte value) {
    flg_update_Z(value);
    flg_update_N(value);
}
