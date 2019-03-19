#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flv.h"
#include "nalu.h"
#include "flv_format_define.h"
#include "osdep.h"

static void paramInit(PT_FlvContext pFlvCtx)
{
    pFlvCtx->avParam.bHasVideo = 1;
    pFlvCtx->avParam.videoParam.codecType = FLV_CODECID_H264;
    pFlvCtx->avParam.videoParam.width = 320;
    pFlvCtx->avParam.videoParam.width = 240;
    pFlvCtx->avParam.videoParam.frameRate = 15;
    pFlvCtx->curTimeStamp = 0;
    pFlvCtx->aduioTimeInterval = 20;
    pFlvCtx->videoTimeInterval = 66; /* 一秒15帧，帧间隔15ms */
}

static int getFlvHeader(PT_FlvContext pFlvCtx)
{
    T_FlvHeader flvHeaer;

    flvHeaer.tagF = 'F';
    flvHeaer.tagL = 'L';
    flvHeaer.tagV = 'V';
    flvHeaer.version = 0x01;
    flvHeaer.audioAndVideo = FLV_HAS_VIDEO;
    memset(flvHeaer.dataOffset, 0, 4);
    flvHeaer.dataOffset[3] = 0x09;

    pFlvCtx->flvTag.pBuf = realloc(pFlvCtx->flvTag.pBuf, FLV_HEAD_SIZE+FLV_TAG_PRE_SIZE);
    pFlvCtx->flvTag.bufLen = FLV_HEAD_SIZE+FLV_TAG_PRE_SIZE;

    unsigned char *pBuf = pFlvCtx->flvTag.pBuf;
    memcpy(pBuf, &flvHeaer, sizeof(T_FlvHeader));
    pBuf += sizeof(T_FlvHeader);
    pBuf = ui32_to_bytes(pBuf, 0);

    return 0;
}

static int flvFlushData(PT_FlvContext pFlvCtx)
{
    if(!pFlvCtx || !pFlvCtx->flvTag.pBuf || !pFlvCtx->flvTag.bufLen)
        return -1;

    if(fwrite(pFlvCtx->flvTag.pBuf, pFlvCtx->flvTag.bufLen, 1, pFlvCtx->pDstFile) < 0)
        return -1;

    return 0;
}

static int getVideoScriptParam(PT_FlvContext pFlvCtx)
{
    int startPos = FLV_HEAD_SIZE + FLV_TAG_PRE_SIZE + FLV_TAG_HEAD_SIZE; /* 第一个Tag data开始的位置 */

    /* 组装Video AMF数据 */
    
    pFlvCtx->flvTag.tagType = FLV_TAG_TYPE_META;
    pFlvCtx->flvTag.pBufHelp = realloc(pFlvCtx->flvTag.pBufHelp, 4096);
    pFlvCtx->flvTag.bufHelpLen = 4096;
    unsigned char *pBuf = pFlvCtx->flvTag.pBufHelp;

	//----------------------------------AFM包1--------------------------------------
	pBuf = ui08_to_bytes(pBuf, AMF_DATA_TYPE_STRING); /* 第一个AFM包，类型为0x02，表示字符串 */
	pBuf = amf_string_to_bytes(pBuf, "onMetaData"); /* 写入字符串的长度（UI16），然后再写入字符串 */

	//----------------------------------AFM包2--------------------------------------
	pBuf = ui08_to_bytes(pBuf, AMF_DATA_TYPE_MIXEDARRAY); /* 第二个AFM包，类型为0x08，表示数组 */
	pBuf = ui32_to_bytes(pBuf, AMF_VIDEO_ECMA_ARRAY_LENGTH); /* 写入数组长度，7个数组项 */
	int offset = 0;
	//-----duration，视频的时长
	pBuf = amf_string_to_bytes(pBuf, "duration"); /* 第一个数组元素 */
	offset = (int)(pBuf - pFlvCtx->flvTag.pBufHelp + 1); /* 偏移量 */
    pFlvCtx->flvTag.durationPos = startPos + offset; /* durationPos：记录下了duration变量在flv中的偏移量 */
	pBuf = amf_double_to_bytes(pBuf, 0); // written at end of encoding，写0只是为了占位

	//-----width
	pBuf = amf_string_to_bytes(pBuf, "width");
	pBuf = amf_double_to_bytes(pBuf, pFlvCtx->avParam.videoParam.width);

	//-----height
	pBuf = amf_string_to_bytes(pBuf, "height");
	pBuf = amf_double_to_bytes(pBuf, pFlvCtx->avParam.videoParam.height);

	//-----videodatarate
	pBuf = amf_string_to_bytes(pBuf, "videodatarate");
	offset = (int)(pBuf - pFlvCtx->flvTag.pBufHelp + 1);
	pFlvCtx->flvTag.bitratePos = startPos + offset; /* 记录下位置 */
	pBuf = amf_double_to_bytes(pBuf, 0); // written at end of encoding，占位

	//-----framerate
	pBuf = amf_string_to_bytes(pBuf, "framerate");
	offset = (int)(pBuf - pFlvCtx->flvTag.pBufHelp + 1);
	pFlvCtx->flvTag.frameRatePos = startPos + offset;
	pBuf = amf_double_to_bytes(pBuf, pFlvCtx->avParam.videoParam.frameRate); // written at end of encoding

	//-----videocodecid
	pBuf = amf_string_to_bytes(pBuf, "videocodecid");
	pBuf = amf_double_to_bytes(pBuf, pFlvCtx->avParam.videoParam.codecType);

	//-----filesize
	pBuf = amf_string_to_bytes(pBuf, "filesize");
	offset = (int)(pBuf - pFlvCtx->flvTag.pBufHelp + 1);
	pFlvCtx->flvTag.fileSizePos = startPos + offset;
	pBuf = amf_double_to_bytes(pBuf, 0); // written at end of encoding

	//-----END_OF_OBJECT，009
	pBuf = amf_string_to_bytes(pBuf, "");
	pBuf = ui08_to_bytes(pBuf, AMF_END_OF_OBJECT);

    pFlvCtx->flvTag.bufHelpLen = (int)(pBuf - pFlvCtx->flvTag.pBufHelp);

    return pFlvCtx->flvTag.bufHelpLen;
}

static int buildTag(PT_FlvContext pFlvCtx)
{
    if(!pFlvCtx || !pFlvCtx->flvTag.pBufHelp)
        return -1;

    T_FlvTagHeader flvTagHeader;

	ui08_to_bytes(&flvTagHeader.tagType, pFlvCtx->flvTag.tagType); //type
	ui24_to_bytes(flvTagHeader.size, pFlvCtx->flvTag.bufHelpLen); //size
	ui24_to_bytes(flvTagHeader.timeStamp, pFlvCtx->curTimeStamp); //timestamp，m_curr_timestamp：时间戳，毫秒
	ui08_to_bytes(&flvTagHeader.extentTimeStamp, pFlvCtx->curTimeStamp >> 24); //timestamp_ex
    memset(flvTagHeader.streamID, 0, 3);
    
    pFlvCtx->flvTag.bufLen = pFlvCtx->flvTag.bufHelpLen + FLV_TAG_HEAD_SIZE + FLV_TAG_PRE_SIZE;
    pFlvCtx->flvTag.pBuf = realloc(pFlvCtx->flvTag.pBuf, pFlvCtx->flvTag.bufLen);
    unsigned char *pBuf = pFlvCtx->flvTag.pBuf;
    memcpy(pBuf, &flvTagHeader, sizeof(T_FlvTagHeader));
    pBuf += sizeof(T_FlvTagHeader);

    memcpy(pBuf, pFlvCtx->flvTag.pBufHelp, pFlvCtx->flvTag.bufHelpLen);
    pBuf += pFlvCtx->flvTag.bufHelpLen;
    pBuf = ui32_to_bytes(pBuf, pFlvCtx->flvTag.bufLen - FLV_TAG_PRE_SIZE);

    pFlvCtx->flvTag.frameNum++;

    if(FLV_TAG_TYPE_AUDIO == pFlvCtx->flvTag.tagType)
	{
		pFlvCtx->curTimeStamp += pFlvCtx->aduioTimeInterval;
	}
	else if(FLV_TAG_TYPE_VIDEO == pFlvCtx->flvTag.tagType)
	{
		pFlvCtx->curTimeStamp += pFlvCtx->videoTimeInterval;
	}

    return 0;
}

static int getAVCSequenceHeader(T_NALU nalu[], PT_FlvContext pFlvCtx)
{
	unsigned char *sps = nalu[0].pData;
	unsigned int spsLen = nalu[0].naluLen;
	unsigned char *pps = nalu[1].pData;
	unsigned int ppsLen = nalu[1].naluLen;
    unsigned char flag = FLV_FRAME_KEY;

    pFlvCtx->flvTag.tagType = FLV_TAG_TYPE_VIDEO;
    pFlvCtx->flvTag.bufHelpLen = spsLen + ppsLen + 64;
    pFlvCtx->flvTag.pBufHelp = realloc(pFlvCtx->flvTag.pBufHelp, pFlvCtx->flvTag.bufHelpLen);
    unsigned char *pBuf = pFlvCtx->flvTag.pBufHelp;

	pBuf = ui08_to_bytes(pBuf, flag);

	pBuf = ui08_to_bytes(pBuf, 0); // AVCPacketType: 0x00 - AVC sequence header
	pBuf = ui24_to_bytes(pBuf, 0); // composition time

	// generate AVCC with sps and pps, AVCDecoderConfigurationRecord
	pBuf = ui08_to_bytes(pBuf, 1); // configurationVersion
	pBuf = ui08_to_bytes(pBuf, sps[1]); // AVCProfileIndication
	pBuf = ui08_to_bytes(pBuf, sps[2]); // profile_compatibility
	pBuf = ui08_to_bytes(pBuf, sps[3]); // AVCLevelIndication
	
	// 6 bits reserved (111111) + 2 bits nal size length - 1
	// (Reserved << 2) | Nal_Size_length = (0x3F << 2) | 0x03 = 0xFF
	pBuf = ui08_to_bytes(pBuf, 0xff); /* 表示NALU的size用几个字节表示 */
    
	// 3 bits reserved (111) + 5 bits number of sps (00001)
	// (Reserved << 5) | Number_of_SPS = (0x07 << 5) | 0x01 = 0xe1
	pBuf = ui08_to_bytes(pBuf, 0xe1);

	// sps
	pBuf = ui16_to_bytes(pBuf, spsLen);
	memcpy(pBuf, sps, spsLen);
	pBuf += spsLen;

	// pps
	pBuf = ui08_to_bytes(pBuf, 1); // number of pps
	pBuf = ui16_to_bytes(pBuf, ppsLen);
	memcpy(pBuf, pps, ppsLen);
	pBuf += ppsLen;

	pFlvCtx->flvTag.bufHelpLen = (unsigned int)(pBuf - pFlvCtx->flvTag.pBufHelp);

    return pFlvCtx->flvTag.bufHelpLen;
}

static int getvideoTagData(PT_NALU pNalu, PT_FlvContext pFlvCtx)
{
    pFlvCtx->flvTag.bufHelpLen = pNalu->naluLen + 9;
    pFlvCtx->flvTag.pBufHelp = realloc(pFlvCtx->flvTag.pBufHelp, pFlvCtx->flvTag.bufHelpLen);
    unsigned char *pBuf = pFlvCtx->flvTag.pBufHelp;

    pFlvCtx->flvTag.tagType = FLV_TAG_TYPE_VIDEO;

    if(pNalu->naluType == H264_IDR)
    {
        pBuf = ui08_to_bytes(pBuf, FLV_FRAME_KEY);
    }
    else
    {
		pBuf = ui08_to_bytes(pBuf, FLV_FRAME_INTER);
    }

	pBuf = ui08_to_bytes(pBuf, 1); // AVC NALU
	pBuf = ui24_to_bytes(pBuf, pFlvCtx->videoTimeInterval); // set interval time 
	pBuf = ui32_to_bytes(pBuf, pNalu->naluLen);
	memcpy(pBuf, pNalu->pData, pNalu->naluLen);

    return pFlvCtx->flvTag.bufHelpLen;
}

static void rewrite_amf_double(FILE *fp, uint64_t position, double value)
{
	uint64_t x = endian_fix64(dbl2int(value));
	fseek(fp, position, SEEK_SET);
	fwrite(&x, 8, 1, fp);
}


static int finishFlushData(PT_FlvContext pFlvCtx)
{
    double totalDuration = (double)pFlvCtx->curTimeStamp / 1000;
    unsigned long fileSize = ftell(pFlvCtx->pDstFile);
    
    rewrite_amf_double(pFlvCtx->pDstFile, pFlvCtx->flvTag.durationPos, totalDuration);
    rewrite_amf_double(pFlvCtx->pDstFile, pFlvCtx->flvTag.fileSizePos, fileSize);
    if (0 != totalDuration)
    {
        rewrite_amf_double(pFlvCtx->pDstFile, pFlvCtx->flvTag.bitratePos, fileSize * 8 / (totalDuration * 1000));
    }

    return 0;
}

static void freeFlvCtx(PT_FlvContext pFlvCtx)
{
    if(pFlvCtx->flvTag.pBuf)
        free(pFlvCtx->flvTag.pBuf);

    if(pFlvCtx->flvTag.pBufHelp)
        free(pFlvCtx->flvTag.pBufHelp);
}

static void freeNalu(PT_NALU pNalu)
{
    if(!pNalu)
        return ;

    if(pNalu->pData)
    {
        free(pNalu->pData);
        pNalu->pData = NULL;
        pNalu->naluLen = 0;
    }
}

int conver2Flv(const char *srcFileName, const char *dstFileName)
{    
    T_FlvContext flvCtx;
    T_NALU nalu[2];
    FILE *pSrcFile, *pDstFile;
    int ret;

    memset(nalu, 0, sizeof(nalu));
    memset(&flvCtx, 0, sizeof(flvCtx));

    pSrcFile = fopen(srcFileName, "rb");
    if(!pSrcFile)
    {
        printf("fopen %s fail!\n", srcFileName);
        return -1;
    }

    pDstFile = fopen(dstFileName, "wb");
    if(!pDstFile)
    {
        printf("fopen %s fail!\n", dstFileName);
        return -1;
    }

    flvCtx.pSrcFile = pDstFile;
    flvCtx.pDstFile = pDstFile;
    paramInit(&flvCtx);

    getFlvHeader(&flvCtx);
    flvFlushData(&flvCtx);

    getVideoScriptParam(&flvCtx);
    buildTag(&flvCtx);
    flvFlushData(&flvCtx);

    /* 此处忽略SEI */

    /* 得到 SPS */
    while(nalu[0].naluType != H264_SPS)
    {
        ret = getNALU(&nalu[0], pSrcFile);
        if(ret < 0)
        {
            printf("can't get SPS!\n");
            return -1;
        }
    }
    /* 得到PPS */
    getNALU(&nalu[1], pSrcFile);

    /* SPS与PPS */
    getAVCSequenceHeader(nalu, &flvCtx);
    buildTag(&flvCtx);
    flvFlushData(&flvCtx);

    while(!feof(pSrcFile))
    {
        getNALU(&nalu[0], pSrcFile);

        if(nalu[0].naluType == H264_SPS) /* SPS */
        {
            getNALU(&nalu[1], pSrcFile); /* PPS */
            if(nalu[1].naluType != H264_PPS)
                continue;
            
            getAVCSequenceHeader(nalu, &flvCtx);
            buildTag(&flvCtx);
            flvFlushData(&flvCtx);
        }
        else
        {
            getvideoTagData(&nalu[0], &flvCtx);
            buildTag(&flvCtx);
            flvFlushData(&flvCtx);
        }
    }

    finishFlushData(&flvCtx);

    freeFlvCtx(&flvCtx);

    freeNalu(&nalu[0]);
    freeNalu(&nalu[1]);

    return 0;
}
