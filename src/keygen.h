/*
 * keygen.h - Keil license key generation (STUB)
 *
 * NOTICE: The original keygen algorithm has been intentionally removed.
 * Only function signatures are preserved to keep the UI workflow intact.
 * See keygen.c for details.
 */
#ifndef KEYGEN_H
#define KEYGEN_H

#include <stdint.h>

/* MT19937 random number generator - general purpose utility */
void mt_seed(uint32_t seed);
uint32_t mt_rand(void);

/*
 * Serial and license key generation (STUB)
 *
 * These functions produce random output in the correct format
 * for UI demonstration purposes only. The original algorithms
 * have been removed.
 */
void generate_serial(char* out, int use_cid, int target, int license);
void generate_license_key(char* out, char* cid, int use_cid,
    int target, int license);

#endif /* KEYGEN_H */
