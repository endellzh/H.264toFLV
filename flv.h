#ifndef _FLV_H_
#define _FLV_H_

#define FLV_HAS_AUDIO (1<<2)
#define FLV_HAS_VIDEO (1<<0)

#define FLV_HEAD_SIZE       9
#define FLV_TAG_PRE_SIZE	4
#define FLV_TAG_HEAD_SIZE	11

typedef struct FlvTagHeader {
	unsigned char tagType;
	unsigned char size[3];
	unsigned char timeStamp[3];
	unsigned char extentTimeStamp;
	unsigned char streamID[3];
} T_FlvTagHeader, *PT_FlvTagHeader;

typedef struct FlvTag {
    unsigned char *pBuf;
    unsigned int bufLen;
    unsigned char *pBufHelp;    
    unsigned int bufHelpLen;
    unsigned int timeStamp; /* 时间戳 */
	unsigned int tagType; /* 数据类型 */
    unsigned long frameNum;
	unsigned long frameRatePos;
	unsigned long durationPos;
	unsigned long fileSizePos;
	unsigned long bitratePos;
} T_FlvTag, *PT_FlvTag;

typedef struct AudioParam
{
	unsigned int bitRate;			//音频码率
	unsigned int sampleRate;		//采样率
	unsigned int sampleDpth;		//采样深度
	unsigned int channel;			//通道数
	unsigned int samplesPerSec;	//样位率
	unsigned int codecType;        //音频编码类型
} T_AudioParam, *PT_AudioParam;

typedef struct VideoParam
{
    unsigned int frameType;		/* 视频帧类型( I帧，P帧 ) */
    unsigned int imageType;		/* 图像分辨率类型 */
    unsigned int form;				/* 制式（PAL或N制） */
    unsigned int width;			/* 宽度 */
    unsigned int height;			/* 高度 */
    unsigned int pitch;
    unsigned int fieldFlag;	    /* 分场标志 */
    unsigned int frameRate;        //帧率
    unsigned int codecType;		//视频编码类型
} T_VideoParam, *PT_VideoParam;

typedef struct AVParam
{
    T_AudioParam audioParam;
    T_VideoParam videoParam;
    int bHasAudio; /* 是否有音频数据 */
    int bHasVideo; /* 是否有视频数据 */
} T_AVParam, *PT_AVParam;

typedef struct FlvContext {
    T_FlvTag flvTag;
    T_AVParam avParam;
    unsigned int curTimeStamp;
    unsigned int aduioTimeInterval;
    unsigned int videoTimeInterval;
    FILE *pSrcFile;
    FILE *pDstFile;
} T_FlvContext, *PT_FlvContext;

typedef struct FlvHeader {
    unsigned char tagF;
    unsigned char tagL;
    unsigned char tagV;
    unsigned char version;
    unsigned char audioAndVideo;
    unsigned char dataOffset[4];
} T_FlvHeader, *PT_FlvHeader;

int conver2Flv(const char *srcFileName, const char *dstFileName);

#endif

