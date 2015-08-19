#include "decoder.h"

#include <iostream>
using std::cerr;

AVRational TIME_BASE_Q = {1,1000000};

CDecoder::CDecoder():m_isFinished( false )
{
	av_register_all();
	this->m_pFormatCtx = avformat_alloc_context();
}

int CDecoder::OpenInputFile( const char *url, AVMediaType type )
{
	if( avformat_open_input( &m_pFormatCtx, url, NULL, NULL ) != 0 )
	{
		cerr << "Could not open the file\n";
		return -1;
	}

	if( avformat_find_stream_info( m_pFormatCtx, NULL ) < 0 )
	{
		cerr << "Could not find stream information.\n";
		return -1;
	}

	m_mediaTypeIndex = av_find_best_stream( m_pFormatCtx, type, -1, -1, &m_pCodec, 0 );

	if( m_mediaTypeIndex < 0 )
	{
		av_log( NULL, AV_LOG_ERROR, "Can not find a Index\n" );
		return -1;
	}

	m_pCodecCtx = m_pFormatCtx->streams[m_mediaTypeIndex]->codec;
	m_pCodec = avcodec_find_decoder( m_pCodecCtx->codec_id );

	if( m_pCodec == NULL )
	{
		av_log( NULL, AV_LOG_ERROR, "Can not find a Codec\n" );
		return -1;
	}

	if( avcodec_open2( m_pCodecCtx, m_pCodec, NULL ) < 0 )
	{
		av_log( NULL, AV_LOG_ERROR, "Can not open a Codec\n" );
		return -1;
	}

	m_pFrame = av_frame_alloc();
	m_packet = (AVPacket *)av_malloc( sizeof(AVPacket) );
	

	return 0;

}
AVCodecContext* CDecoder::getCodecContext()const
{
	return m_pCodecCtx;
}

bool CDecoder::isFinished()
{
	return m_isFinished;
}

double CDecoder::getTime()
{
	AVRational time_base = m_pFormatCtx->streams[m_mediaTypeIndex]->time_base;
	m_pFrame->pts=av_frame_get_best_effort_timestamp( m_pFrame );
	double pts_time = av_rescale_q( m_pFrame->pts, time_base, TIME_BASE_Q )/1000000.0;
	return pts_time;
}

int CVideoDecoder::getHeight()const
{
	return m_pCodecCtx->height;
}

int CVideoDecoder::getWidth()const
{
	return m_pCodecCtx->width;
}

AVPixelFormat CVideoDecoder::getFormat()const
{
	return m_pCodecCtx->pix_fmt;
}



CDecoder::~CDecoder()
{
	av_free( m_pFrame );
	avcodec_close( m_pCodecCtx );
	avformat_close_input( &m_pFormatCtx );
}
CVideoDecoder::~CVideoDecoder()
{

}


CVideoDecoder::CVideoDecoder()
{

}

void CVideoDecoder::Init()
{

}

AVFrame* CVideoDecoder::Process()
{
	int ret = 0;
	int got_picture;
	while( 1 )
	{
		if( av_read_frame( m_pFormatCtx, m_packet ) >= 0  )
		{
			if( m_packet->stream_index == m_mediaTypeIndex )
			{
				ret = avcodec_decode_video2( m_pCodecCtx, m_pFrame, &got_picture, m_packet );
				if( ret < 0 )
				{
					av_log( NULL, AV_LOG_ERROR, "Decode Error\n" );
					return NULL;
				}

				if( got_picture )
				{
					printf( "vedio time:%lf\n", this->getTime() );
					return m_pFrame;
					
				}//end if(  )
			}
			else
			{
				//printf( "aaaaa\n" );
			}
			av_free_packet(m_packet);
		}//end if
		else
		{
			m_isFinished = true;
			return NULL;
		}
	}//end while
}

CAudioDecoder::CAudioDecoder()
{

}

AVFrame* CAudioDecoder::Process()
{
	int ret = 0;
	int got_picture;
	while( 1 )
	{
		if( av_read_frame( m_pFormatCtx, m_packet ) >= 0  )
		{
			if( m_packet->stream_index == m_mediaTypeIndex )
			{
				ret = avcodec_decode_audio4( m_pCodecCtx, m_pFrame, &got_picture, m_packet );
				if( ret < 0 )
				{
					av_log( NULL, AV_LOG_ERROR, " Audio Decode Error\n" );
					return NULL;
				}

				if( got_picture )
				{
					printf( "audio time:%lf\n", this->getTime() );
					return m_pFrame;

				}//end if(  )
			}
			else
			{
				//printf( "aaaaa\n" );
			}
			av_free_packet(m_packet);
		}//end if
		else
		{
			m_isFinished = true;
			return NULL;
		}
	}//end while
}