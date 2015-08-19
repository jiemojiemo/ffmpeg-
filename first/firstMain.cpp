#ifndef INT64_C 
#define INT64_C(c) (c ## LL) 
#define UINT64_C(c) (c ## ULL) 
#endif 

#ifdef __cplusplus 
extern "C" {
#endif 
	/*Include ffmpeg header file*/
#include <libavformat/avformat.h> 
#include <libavcodec/avcodec.h> 
#include <libswscale/swscale.h> 

#include <libavutil/imgutils.h>  
#include <libavutil/opt.h>     
#include <libavutil/mathematics.h>   
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus 
}
#endif 
#include <iostream>
using namespace std;


void SaveFrame( AVFrame* pFrame, int width, int height, int iFrame );


int main()
{
	//
	av_register_all();
	
	AVFormatContext* pFormatCtx = NULL;
	char *fileName = "6s_kapian.mp4";

	//Open the file
	//只是看一看文件头
	if( avformat_open_input( &pFormatCtx, fileName, NULL, NULL ) != 0 )
	{
		cerr << "Open file Error\n";
		return -1;

	}
	//接下去检查文件信息,存放在pFormatCtx->stream中
	if( avformat_find_stream_info( pFormatCtx,NULL ) < 0 )
	{
		cerr << "Find Stream Error\n";
		return -1;
	}
	//我们使用一个便利的函数来查看里面的信息
	//自动输出
	av_dump_format( pFormatCtx, 0, fileName, 0 );
	
	AVCodecContext *pCodecCtxOrig = NULL;
	AVCodecContext *pCodecCtx = NULL;


	//接下来找到第一个视频数据
	//因为在第一个视频数据之前有很多头，也有可能有音频
	int videoStream = -1;
	int i;
	for( i = 0; i < pFormatCtx->nb_streams; ++i )
	{
		//如果是视频
		if( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
		{
			videoStream = i;
			break;
		}
	}
	if( -1 == videoStream )
		return -1;

	pCodecCtx = pFormatCtx->streams[videoStream]->codec;
	
	AVCodec* pCodec = NULL;

	pCodec = avcodec_find_decoder( pCodecCtx->codec_id );
	if( pCodec == NULL )
	{
		cerr << "Unsupported codec!\n";
		return -1;
	}


	//Copy context
	pCodecCtx = avcodec_alloc_context3( pCodec );
	if( avcodec_copy_context( pCodecCtx, pCodecCtxOrig ) != 0 )
	{
		cerr << "Couldn't copy codec context";
		return -1;
	}

	//Open codec
	if( avcodec_open2( pCodecCtx, pCodec,NULL ) < 0 )
		return -1;

	//Storing the Data
	AVFrame *pFrame = NULL;

	//Allocate video frame
	pFrame = av_frame_alloc();

	//Allocate an AVFrame structure
	AVFrame* pFrameRGB = NULL;
	pFrameRGB = av_frame_alloc();
	if( pFrameRGB == NULL )
	{
		return -1;
	}

	uint8_t* buffer = NULL;
	int numBytes;
	numBytes = avpicture_get_size( PIX_FMT_RGB24,pCodecCtx->width, pCodecCtx->height );
	buffer = (uint8_t*)malloc( numBytes * sizeof( uint8_t ) );

	//Assign appropriate parts of buffer to image planes in pFrameRGB
	//Note that pFrameRGB is an AVFrame, but AVFrame is superset of AVPicture
	avpicture_fill( (AVPicture*)pFrameRGB, buffer, PIX_FMT_RGB24,
		pCodecCtx->width, pCodecCtx->height);

	struct SwsContext* sws_ctx = NULL;
	int frameFinished;
	AVPacket packet;
	//initialize SWS contex for software scaling
	sws_ctx = sws_getContext(
		pCodecCtx->width,
		pCodecCtx->height,
		pCodecCtx->pix_fmt,
		pCodecCtx->width,
		pCodecCtx->height,
		PIX_FMT_RGB24,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
		);
	i = 0;
	while( av_read_frame( pFormatCtx, &packet ) >= 0 )
	{
		if( packet.stream_index == videoStream )
			//Decode video frame
			avcodec_decode_video2( pCodecCtx,pFrame, &frameFinished, &packet );

		//Did we get a video frame?
		if( frameFinished )
		{
			//Convert the image from its native format to RGB
			sws_scale( sws_ctx, ( const uint8_t *const *)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
				pFrameRGB->data, pFrameRGB->linesize);

			if( ++i <= 5 )
			 SaveFrame( pFrameRGB, pCodecCtx->width, pCodecCtx->height,i );
		}
	}
}

void SaveFrame( AVFrame* pFrame, int width, int height, int iFrame )
{

}