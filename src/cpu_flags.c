#include "../include/common.h"
#include "../include/cpu_flags.h"

/* Carry flag (C). */

static int C;            /* Last result that affected the C flag. */
static Operation C_type; /* Type of the last operation that affected the C flag. */

inline bool flg_is_C   (void) {
    switch (C_type) {
        case ADC:   return C & 0xF00;
        case CMP:   return C >= 0;
        case ROL:   return C & 0x80;
        case ROR:   return C & 0x01;
        default:    return false;
    }
}

inline void flg_set_C  (void) { C = 0x80; C_type = ROL; }
inline void flg_clear_C(void) { C = 0x00; C_type = ROL; }
inline void flg_update_C(int result, Operation type) { C = result; C_type = type; }

/* Zero flag (Z) and negative flag (N). */

static byte Z;  /* Last result that affected the Z flag. */
static byte N;  /* Last result that affected the N flag. */

inline bool flg_is_Z   (void) { return !Z;       }
inline bool flg_is_N   (void) { return N & 0x80; }
inline void flg_set_Z  (void) { Z = 0x00;        }
inline void flg_set_N  (void) { N = 0x80;        }
inline void flg_clear_Z(void) { Z = 0x01;        }
inline void flg_clear_N(void) { N = 0x00;        }

inline void flg_update_Z (byte value) { Z = value; }
inline void flg_update_N (byte value) { N = value; }
inline void flg_update_ZN(byte value) {
    flg_update_Z(value);
    flg_update_N(value);
}

/* Interrupt disable (I) and decimal mode flag (D). */

static bool I;  /* Interrupt disable. */
static bool D;  /* Decimal mode flag. */

inline bool flg_is_I   (void) { return I;  }
inline bool flg_is_D   (void) { return D;  }
inline void flg_set_I  (void) { I = true;  }
inline void flg_set_D  (void) { D = true;  }
inline void flg_clear_I(void) { I = false; }
inline void flg_clear_D(void) { D = false; }

/* Overflow flag (V). */

static byte V1, V2, V;   /* Last result and operands that affected the V flag. */
static Operation V_type; /* Type of the last operation that affected the V flag. */

inline bool flg_is_V(void) {
    return V_type == BIT ? V & 0x40 : ((V ^ V1) & (V ^ V2)) & 0x80;
}

inline void flg_set_V  (void) { V = 0x40; V_type = BIT; }
inline void flg_clear_V(void) { V = 0x00; V_type = BIT; }

inline void flg_update_V_bit(byte s) { V_type = BIT; V = s; }

inline void flg_update_V(byte s, byte a, byte b) {
    V_type = ADC; V = s; V1 = a; V2 = b;
}

/* Get / set processor flag status (P). */

inline byte flg_get_status(bool B) {
    byte P = 0x00;
    P |= flg_is_C();
    P |= (flg_is_Z() << 1);
    P |= (flg_is_I() << 2);
    P |= (flg_is_D() << 3);
    P |= (B          << 4);
    P |= (0x01       << 5);
    P |= (flg_is_V() << 6);
    P |= (flg_is_N() << 7);
    return P;
}

inline void flg_set_status(byte P) {
    if (P & 0x01) flg_set_C(); else flg_clear_C();
    if (P & 0x02) flg_set_Z(); else flg_clear_Z();
    if (P & 0x04) flg_set_I(); else flg_clear_I();
    if (P & 0x08) flg_set_D(); else flg_clear_D();
    if (P & 0x40) flg_set_V(); else flg_clear_V();
    if (P & 0x80) flg_set_N(); else flg_clear_N();
}

/* Reset flags. */

inline void flg_reset(void) {
    flg_clear_C();
    flg_clear_Z();
    flg_clear_I();
    flg_clear_D();
    flg_clear_V();
    flg_clear_N();
}
