#ifndef TRANSCODER_H
#define TRANSCODER_H
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
class CTransCoder
{
private:
	AVFrame* m_pOutFrame;
	AVCodecContext* m_CodecContext;
	AVPixelFormat m_pixFormat;
	uint8_t*	  m_buffer;
	uint8_t*	  m_audioBuffer;
	uint64_t	m_out_channel_layout;
	int			m_out_nb_samples;
	AVSampleFormat m_out_sample_fmt;
	int m_out_sample_rate ;
	int m_out_channels;

	struct SwsContext *m_img_convert_ctx;
	struct SwrContext *m_au_convert_ctx;
public:
	CTransCoder();
	~CTransCoder();
	void SwsInit( AVCodecContext* ctx );
	void SwrInit( AVCodecContext* ctx );
	AVFrame* TransVideoFrame( AVFrame* frame );
	uint8_t * TransAudioFrame( AVFrame* frame );

private:
	CTransCoder( const CTransCoder& t ){}
	CTransCoder& operator=( CTransCoder ){}
};


#endif

