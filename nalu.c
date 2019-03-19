#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nalu.h"

static unsigned char gBuf[NALU_MAX_SIZE];

static int startCode3(unsigned char *pData)
{
    if(pData[0] != 0 || pData[1] != 0 || pData[2] != 1)
        return 0;
    else 
        return 1;
}

static int startCode4(unsigned char *pData)
{
    if(pData[0] != 0 || pData[1] != 0 || pData[2] != 0 || pData[3] != 1)
        return 0;
    else 
        return 1;    
}

int getNALU(PT_NALU pNalu, FILE *pFile)
{
    int ret;
    int startCodeLen;
    int pos;

    ret = fread(gBuf, 1, 3, pFile);

    if(startCode3(gBuf))
    {
        startCodeLen = 3;
    }
    else
    {
        ret = fread(gBuf+3, 1, 1, pFile);
        if(startCode4(gBuf))
            startCodeLen = 4;
        else
            return -1;
    }

    pos = startCodeLen;
    while(1)
    {
        if(feof(pFile))
            break;
        
        gBuf[pos++] = fgetc(pFile);

        if(startCode4(&gBuf[pos-4]))
        {
            fseek(pFile, -4, SEEK_CUR);
            pos -= 4;
            break;
        }
        else
        {
            if(startCode3(&gBuf[pos-3]))
            {
                fseek(pFile, -3, SEEK_CUR);
                pos -= 3;
                break;
            }
        }
    }

    pNalu->naluLen = pos-startCodeLen;
    pNalu->pData = realloc(pNalu->pData, pNalu->naluLen);
    memcpy(pNalu->pData, gBuf+startCodeLen, pNalu->naluLen);
    pNalu->naluType = (pNalu->pData[0] & 0x1F);

    return 0;
}


