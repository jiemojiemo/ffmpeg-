#ifndef DECODER_H
#define DECODER_H

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "SDL.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <SDL2/SDL.h>
#ifdef __cplusplus
};
#endif
#endif

class CDecoder{
public:
	CDecoder();
	virtual ~CDecoder();
	int OpenInputFile( const char *url, AVMediaType type );
	AVCodecContext* getCodecContext()const;
	bool isFinished();
	double getTime();

protected:
	AVFormatContext* m_pFormatCtx;
	AVCodecContext*	 m_pCodecCtx;
	AVCodec*		 m_pCodec;
	AVFrame*		 m_pFrame;
	AVPacket*		 m_packet;
	int				 m_mediaTypeIndex;
	bool			 m_isFinished;
private:
	CDecoder( const CDecoder& code ){}
	CDecoder& operator=( const CDecoder& code ){}
};

class CVideoDecoder : public CDecoder{
public:
	CVideoDecoder();
	virtual ~CVideoDecoder();

	int getHeight()const;
	int getWidth()const;
	AVPixelFormat getFormat()const;


	AVFrame* Process();
private:
	void Init();
private:
//	struct SwsContext *img_convert_ctx;
};

class CAudioDecoder : public CDecoder{
private:

public:
	CAudioDecoder();
	AVFrame* Process();
};

#endif