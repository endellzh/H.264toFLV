#ifndef AMF_BYTE_STREAM_H_
#define AMF_BYTE_STREAM_H_

#include <string.h>
#include <stdint.h>

#define H264_STARTCODEPREFIX_LENGTH			4
enum
{
	H264_NON_IDR	= 1,
	H264_IDR		= 5,
	H264_SEI		= 6,
	H264_SPS		= 7,
	H264_PPS		= 8,
	H264_ELSE,
};

/* offsets for packed values */
#define FLV_AUDIO_SAMPLESSIZE_OFFSET	1
#define FLV_AUDIO_SAMPLERATE_OFFSET		2
#define FLV_AUDIO_CODECID_OFFSET		4
#define FLV_VIDEO_FRAMETYPE_OFFSET		4

#define FLV_HEAD_SIZE					9
#define FLV_TAG_PRE_SIZE				4
#define FLV_TAG_HEAD_SIZE				11

/* bitmasks to isolate specific values */
#define FLV_AUDIO_CHANNEL_MASK			0x01
#define FLV_AUDIO_SAMPLESIZE_MASK		0x02
#define FLV_AUDIO_SAMPLERATE_MASK		0x0c
#define FLV_AUDIO_CODECID_MASK			0xf0
#define FLV_VIDEO_CODECID_MASK			0x0f
#define FLV_VIDEO_FRAMETYPE_MASK		0xf0

//AMF 
#define AMF_AUDIO_ECMA_ARRAY_LENGTH		8
#define AMF_VIDEO_ECMA_ARRAY_LENGTH		7
#define AMF_END_OF_OBJECT				0x09

enum
{
	FLV_TAG_TYPE_NONE,
	FLV_TAG_TYPE_AUDIO = 0x08,
	FLV_TAG_TYPE_VIDEO = 0x09,
	FLV_TAG_TYPE_META = 0x12,
};

enum
{
	FLV_MONO = 0,
	FLV_STEREO = 1,
};

enum
{
	FLV_SAMPLESSIZE_8BIT = 0,
	FLV_SAMPLESSIZE_16BIT = 1,
	FLV_SAMPLESSIZE_24BIT = 2,
};

enum
{
	FLV_SAMPLERATE_SPECIAL = 0, /**< signifies 5512Hz and 8000Hz in the case of NELLYMOSER */
	FLV_SAMPLERATE_11025HZ = 1,
	FLV_SAMPLERATE_22050HZ = 2,
	FLV_SAMPLERATE_44100HZ = 3,
};

enum AUDIO_CODECID
{
	FLV_CODECID_MP3 = 2,
	FLV_CODECID_AAC = 10,
	FLV_CODECID_SPEEX = 11,
};

enum VIDEO_CODECID
{
	FLV_CODECID_H264 = 7,
};

enum
{
	FLV_FRAME_KEY = 1 << FLV_VIDEO_FRAMETYPE_OFFSET | 7,		//frametype "1 == keyframe", codecid "7 == AVC"
	FLV_FRAME_INTER = 2 << FLV_VIDEO_FRAMETYPE_OFFSET | 7,
};

typedef enum 
{
    AMF_DATA_TYPE_NUMBER = 0x00,
    AMF_DATA_TYPE_BOOL = 0x01,
    AMF_DATA_TYPE_STRING = 0x02,
    AMF_DATA_TYPE_OBJECT = 0x03,
    AMF_DATA_TYPE_NULL = 0x05,
    AMF_DATA_TYPE_UNDEFINED = 0x06,
    AMF_DATA_TYPE_REFERENCE = 0x07,
    AMF_DATA_TYPE_MIXEDARRAY = 0x08,
    AMF_DATA_TYPE_OBJECT_END = 0x09,
    AMF_DATA_TYPE_ARRAY = 0x0a,
    AMF_DATA_TYPE_DATE = 0x0b,
    AMF_DATA_TYPE_LONG_STRING = 0x0c,
    AMF_DATA_TYPE_UNSUPPORTED = 0x0d,
} AMFDataType;

static uint8_t *ui08_to_bytes(uint8_t *buf, uint8_t val) {
	buf[0] = (val)& 0xff;

	return buf + 1;
}

static uint8_t *ui16_to_bytes(uint8_t *buf, uint16_t val) {
    buf[0] = (val >> 8) & 0xff;
    buf[1] = (val) & 0xff;

    return buf + 2;
}

static uint8_t *ui24_to_bytes(uint8_t *buf, uint32_t val) {
    buf[0] = (val >> 16) & 0xff;
    buf[1] = (val >> 8) & 0xff;
    buf[2] = (val) & 0xff;

    return buf + 3;
}

static uint8_t *ui32_to_bytes(uint8_t *buf, uint32_t val) {
    buf[0] = (val >> 24) & 0xff;
    buf[1] = (val >> 16) & 0xff;
    buf[2] = (val >> 8) & 0xff;
    buf[3] = (val) & 0xff;

    return buf + 4;
}

static uint8_t *ui64_to_bytes(uint8_t *buf, uint64_t val) {
    buf[0] = (val >> 56) & 0xff;
    buf[1] = (val >> 48) & 0xff;
    buf[2] = (val >> 40) & 0xff;
    buf[3] = (val >> 32) & 0xff;
    buf[4] = (val >> 24) & 0xff;
    buf[5] = (val >> 16) & 0xff;
    buf[6] = (val >> 8) & 0xff;
    buf[7] = (val) & 0xff;

    return buf + 8;
}

static uint8_t *double_to_bytes(uint8_t *buf, double val) {
    union {
        uint8_t dc[8];
        double dd;
    } d;

    uint8_t b[8];

    d.dd = val;

    b[0] = d.dc[7];
    b[1] = d.dc[6];
    b[2] = d.dc[5];
    b[3] = d.dc[4];
    b[4] = d.dc[3];
    b[5] = d.dc[2];
    b[6] = d.dc[1];
    b[7] = d.dc[0];

    memcpy(buf, b, 8);

    return buf + 8;
}

static uint8_t bytes_to_ui32(const uint8_t *buf) {
    return (((buf[0]) << 24) & 0xff000000)
            | (((buf[1]) << 16) & 0xff0000)
            | (((buf[2]) << 8) & 0xff00)
            | (((buf[3])) & 0xff);
}

static uint8_t *amf_string_to_bytes(uint8_t *buf, const char *str) {
    uint8_t *pbuf = buf;
    size_t len = strlen(str);
    pbuf = ui16_to_bytes(pbuf, len);
    memcpy(pbuf, str, len);
    pbuf += len;

    return pbuf;
}

static uint8_t *amf_double_to_bytes(uint8_t *buf, double d) {
    uint8_t *pbuf = buf;
    pbuf = ui08_to_bytes(pbuf, AMF_DATA_TYPE_NUMBER);
    pbuf = double_to_bytes(pbuf, d);

    return pbuf;
}

static uint8_t *amf_bool_to_bytes(uint8_t *buf, int b) {
    uint8_t *pbuf = buf;
    pbuf = ui08_to_bytes(pbuf, AMF_DATA_TYPE_BOOL);
    pbuf = ui08_to_bytes(pbuf, !!b);

    return pbuf;
}

static uint64_t dbl2int(double value)
{
	union
	{
		double f;
		uint64_t i;
	}_val;

	_val.f = value;

	return _val.i;
}

#endif // _AMF_BYTE_STREAM_H_