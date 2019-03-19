#ifndef _NALU_H_
#define _NALU_H_

#define NALU_MAX_SIZE (1024 * 100)

typedef struct NALU {
    int naluType;
    int naluLen;
    unsigned char *pData;
} T_NALU, *PT_NALU;

int getNALU(PT_NALU pNalu, FILE *pFile);

#endif
