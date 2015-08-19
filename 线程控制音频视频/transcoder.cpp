#include "transcoder.h"
#define MAX_AUDIO_FRAME_SIZE 192000
CTransCoder::CTransCoder()
{

}



void CTransCoder::SwrInit( AVCodecContext* ctx )
{
	//Out Audio Param
	m_out_channel_layout = AV_CH_LAYOUT_STEREO;
	//nb_samples:
	m_out_nb_samples = 1024;
	m_out_sample_fmt = AV_SAMPLE_FMT_S16;
	m_out_sample_rate = 44100;
	m_out_channels = av_get_channel_layout_nb_channels(m_out_channel_layout);

	//Out buffer size
	int out_buffer_size = av_samples_get_buffer_size( NULL, m_out_channels, m_out_nb_samples, m_out_sample_fmt, 1 );

	m_audioBuffer = (uint8_t *)av_malloc( MAX_AUDIO_FRAME_SIZE );

	int64_t in_channel_layout = av_get_default_channel_layout(ctx->channels);


	m_au_convert_ctx = swr_alloc();
	m_au_convert_ctx = swr_alloc_set_opts( m_au_convert_ctx, m_out_channel_layout, m_out_sample_fmt, m_out_sample_rate,
		in_channel_layout, ctx->sample_fmt, ctx->sample_rate, 0, NULL);
	swr_init(m_au_convert_ctx);
}

void CTransCoder::SwsInit( AVCodecContext* ctx )
{
	m_pOutFrame = av_frame_alloc();

	m_CodecContext = ctx;

	m_buffer = (uint8_t*)malloc( avpicture_get_size( PIX_FMT_YUV420P, m_CodecContext->width, m_CodecContext->height ) );
	avpicture_fill( (AVPicture*)m_pOutFrame, m_buffer, PIX_FMT_YUV420P, m_CodecContext->width, m_CodecContext->height );

	m_img_convert_ctx = sws_getContext( m_CodecContext->width, m_CodecContext->height,
		m_CodecContext->pix_fmt,m_CodecContext->width, m_CodecContext->height,
		PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL );
	
}

AVFrame* CTransCoder::TransVideoFrame( AVFrame* frame )
{
	sws_scale(m_img_convert_ctx,(const uint8_t* const*)frame->data, frame->linesize,
		0,m_CodecContext->height,m_pOutFrame->data, m_pOutFrame->linesize);

	return m_pOutFrame;
}
uint8_t * CTransCoder::TransAudioFrame( AVFrame* frame )
{
	swr_convert( m_au_convert_ctx, &m_audioBuffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)frame->data, frame->nb_samples );

	return m_audioBuffer;
}

CTransCoder::~CTransCoder()
{
	if( m_buffer != NULL )
	{
		free( m_buffer );
		m_buffer = NULL;
	}
	av_free( m_pOutFrame );
	av_free( m_audioBuffer );
	swr_free( &m_au_convert_ctx );
	sws_freeContext( m_img_convert_ctx );

}
