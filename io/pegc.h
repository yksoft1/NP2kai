#ifndef NP2_PEGC_H
#define NP2_PEGC_H

void gv256WriteF0(UINT32 address, UINT32 data, int size);
UINT32 gv256ReadF0(UINT32 address, int size);

UINT32 gv256ReadIO(UINT32 address, int size);
void gv256WriteIO(UINT32 address, UINT32 data, int size);

void gv256ResetRop();
void gv256_initPEGC();
void gv256_uninitPEGC();

#endif	/* NP2_PEGC_H */

