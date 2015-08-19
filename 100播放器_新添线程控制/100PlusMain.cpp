////**
////* 最简单的基于FFmpeg的视频播放器2(SDL升级版)
////* Simplest FFmpeg Player 2(SDL Update)
////*
////* 雷霄骅 Lei Xiaohua
////* leixiaohua1020@126.com
////* 中国传媒大学/数字电视技术
////* Communication University of China / Digital TV Technology
////* http://blog.csdn.net/leixiaohua1020
////*
////* 第2版使用SDL2.0取代了第一版中的SDL1.2
////* Version 2 use SDL 2.0 instead of SDL 1.2 in version 1.
////*
////* 本程序实现了视频文件的解码和显示(支持HEVC，H.264，MPEG2等)。
////* 是最简单的FFmpeg视频解码方面的教程。
////* 通过学习本例子可以了解FFmpeg的解码流程。
////* 本版本中使用SDL消息机制刷新视频画面。
////* This software is a simplest video player based on FFmpeg.
////* Suitable for beginner of FFmpeg.
////*
////* 备注:
////* 标准版在播放视频的时候，画面显示使用延时40ms的方式。这么做有两个后果：
////* （1）SDL弹出的窗口无法移动，一直显示是忙碌状态
////* （2）画面显示并不是严格的40ms一帧，因为还没有考虑解码的时间。
////* SU（SDL Update）版在视频解码的过程中，不再使用延时40ms的方式，而是创建了
////* 一个线程，每隔40ms发送一个自定义的消息，告知主函数进行解码显示。这样做之后：
////* （1）SDL弹出的窗口可以移动了
////* （2）画面显示是严格的40ms一帧
////* Remark:
////* Standard Version use's SDL_Delay() to control video's frame rate, it has 2
////* disadvantages:
////* (1)SDL's Screen can't be moved and always "Busy".
////* (2)Frame rate can't be accurate because it doesn't consider the time consumed 
////* by avcodec_decode_video2()
////* SU（SDL Update）Version solved 2 problems above. It create a thread to send SDL 
////* Event every 40ms to tell the main loop to decode and show video frames.
////*/
//
//#include <stdio.h>
//#include <iostream>
//using namespace std;
//#define __STDC_CONSTANT_MACROS
//
//#ifdef _WIN32
////Windows
//extern "C"
//{
//#include "libavcodec/avcodec.h"
//#include "libavformat/avformat.h"
//#include "libswscale/swscale.h"
//#include "SDL.h"
//};
//#else
////Linux...
//#ifdef __cplusplus
//extern "C"
//{
//#endif
//#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>
//#include <libswscale/swscale.h>
//#include <SDL2/SDL.h>
//#ifdef __cplusplus
//};
//#endif
//#endif
//
////Refresh Event
//#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
//
//int thread_exit=0;
//
//int sfp_refresh_thread(void *opaque){
//	while (thread_exit==0) {
//		SDL_Event event;
//		event.type = SFM_REFRESH_EVENT;
//		SDL_PushEvent(&event);
//		
//	}
//	SDL_Delay(40);
//	return 0;
//}
//
//
//int main(int argc, char* argv[])
//{
//	AVFormatContext	*pFormatCtx;
//	int				i, videoindex;
//	AVCodecContext	*pCodecCtx;
//	AVCodec			*pCodec;
//	AVFrame	*pFrame,*pFrameYUV;
//	uint8_t *out_buffer;
//	AVPacket *packet;
//	int ret, got_picture;
//
//
//	//------------SDL----------------
//	int screen_w,screen_h;
//	SDL_Window *screen; 
//	SDL_Renderer* sdlRenderer;
//	SDL_Texture* sdlTexture;
//	SDL_Rect sdlRect;
//	SDL_Thread *video_tid;
//	SDL_Event event;
//
//	struct SwsContext *img_convert_ctx;
//
//	char filepath[]="F:/video/6s_kapian.mp4";
//	av_register_all();
//	pFormatCtx = avformat_alloc_context();
//
//	//打开文件
//	if( avformat_open_input(&pFormatCtx,filepath,NULL,NULL) != 0 )
//	{
//		cerr << "Could not open the file\n";
//		return -1;
//	}
//
//	//获取流信息
//	if( avformat_find_stream_info( pFormatCtx,NULL ) < 0 )
//	{
//		cerr << "Could not find stream information.\n";
//		return -1;
//	}
//
//	//找到视频位置
//	videoindex = -1;
//	for( i = 0; i < pFormatCtx->nb_streams; ++i )
//	{
//		if( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
//		{
//			videoindex = i;
//			break;
//		}//end if
//	}//end for
//
//	if( -1 == videoindex )
//	{
//		cerr << "Did not find a video stream.\n";
//		return -1;
//	}
//
//	//找到流之后，设置流的编码上下文
//	//以及设置编码器
//
//	//设置编码器上下文
//	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
//	//设置编码器类型
//	pCodec = avcodec_find_decoder( pCodecCtx->codec_id );
//
//	if( pCodec == NULL )
//	{
//		cerr << "Codec not found.\n";
//		return -1;
//	}
//	//该函数用于初始化一个视音频编解码器的AVCodecContext
//	if( avcodec_open2(pCodecCtx,pCodec,NULL) < 0 )
//	{
//		cerr << "Could not open codec.\n";
//		return -1;
//	}
//
//	//设置frame
//	pFrame = av_frame_alloc();
//	pFrameYUV = av_frame_alloc();
//	//绑定一个buffer到pFrameYUV中
//	out_buffer = (uint8_t *)malloc( avpicture_get_size( PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height ) );
//	avpicture_fill( (AVPicture*)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height );
//
//
//	//Output Info-----------------------------
//	printf("---------------- File Information ---------------\n");
//	av_dump_format(pFormatCtx,0,filepath,0);
//	printf("-------------------------------------------------\n");
//
//	//初始化图像转换上下文
//	img_convert_ctx = sws_getContext( pCodecCtx->width, pCodecCtx->height,pCodecCtx->pix_fmt,
//		pCodecCtx->width,pCodecCtx->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
//
//	//初始化SDL
//	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
//		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
//		return -1;
//	} 
//	//SDL 2.0 Support for multiple windows
//	screen_w = pCodecCtx->width;
//	screen_h = pCodecCtx->height;
//	screen = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
//		screen_w, screen_h,SDL_WINDOW_OPENGL);
//
//	if(!screen) {  
//		printf("SDL: could not create window - exiting:%s\n",SDL_GetError());  
//		return -1;
//	}
//	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);  
//	//IYUV: Y + U + V  (3 planes)
//	//YV12: Y + V + U  (3 planes)
//	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,pCodecCtx->width,pCodecCtx->height);  
//
//	sdlRect.x=0;
//	sdlRect.y=0;
//	sdlRect.w=screen_w;
//	sdlRect.h=screen_h;
//
//	packet = (AVPacket *)av_malloc( sizeof(AVPacket) );
//
//	video_tid = SDL_CreateThread( sfp_refresh_thread,NULL,NULL );
//
//	for(;;)
//	{
//		//Wait
//		SDL_WaitEvent(&event);
//		if( event.type == SFM_REFRESH_EVENT )
//		{
//			if( av_read_frame(pFormatCtx,packet) >= 0 )
//			{
//				if( packet->stream_index == videoindex )
//				{
//					ret = avcodec_decode_video2(pCodecCtx,pFrame,&got_picture,packet );
//					if( ret < 0 )
//					{
//						printf("Decode Error.\n");
//						return -1;
//					}//end if( ret < 0 )
//
//					if( got_picture )
//					{
//						sws_scale(img_convert_ctx,(const uint8_t* const*)pFrame->data, pFrame->linesize,
//							0,pCodecCtx->height,pFrameYUV->data, pFrameYUV->linesize);
//						//SDL--------
//						SDL_UpdateTexture( sdlTexture, &sdlRect,pFrameYUV->data[0],pFrameYUV->linesize[0] );
//						SDL_RenderClear( sdlRenderer );
//
//						//SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, NULL);  
//						SDL_RenderCopy( sdlRenderer, sdlTexture,&sdlRect,&sdlRect );
//						SDL_RenderPresent( sdlRenderer );  
//					}//end if( got_picture )
//				}
//				av_free_packet(packet);
//			}//end if(av_read_frame(pFormatCtx,packet) >= 0)
//			else {
//				thread_exit = 1;
//				break;
//			}//end else
//		}//end if
//		else if( event.type == SDL_QUIT )
//		{
//			thread_exit = 1;
//			break;
//		}//end else
//
//
//	}//end for(;;)
//
//
//	sws_freeContext(img_convert_ctx);
//	SDL_Quit();
//
//	//------
//	av_free( pFrameYUV );
//	av_free( pFrame );
//	avcodec_close( pCodecCtx );
//	avformat_close_input(&pFormatCtx);
//	return 0;
//}

//**
//* 最简单的基于FFmpeg的视频播放器2(SDL升级版)
//* Simplest FFmpeg Player 2(SDL Update)
//*
//* 雷霄骅 Lei Xiaohua
//* leixiaohua1020@126.com
//* 中国传媒大学/数字电视技术
//* Communication University of China / Digital TV Technology
//* http://blog.csdn.net/leixiaohua1020
//*
//* 第2版使用SDL2.0取代了第一版中的SDL1.2
//* Version 2 use SDL 2.0 instead of SDL 1.2 in version 1.
//*
//* 本程序实现了视频文件的解码和显示(支持HEVC，H.264，MPEG2等)。
//* 是最简单的FFmpeg视频解码方面的教程。
//* 通过学习本例子可以了解FFmpeg的解码流程。
//* 本版本中使用SDL消息机制刷新视频画面。
//* This software is a simplest video player based on FFmpeg.
//* Suitable for beginner of FFmpeg.
//*
//* 备注:
//* 标准版在播放视频的时候，画面显示使用延时40ms的方式。这么做有两个后果：
//* （1）SDL弹出的窗口无法移动，一直显示是忙碌状态
//* （2）画面显示并不是严格的40ms一帧，因为还没有考虑解码的时间。
//* SU（SDL Update）版在视频解码的过程中，不再使用延时40ms的方式，而是创建了
//* 一个线程，每隔40ms发送一个自定义的消息，告知主函数进行解码显示。这样做之后：
//* （1）SDL弹出的窗口可以移动了
//* （2）画面显示是严格的40ms一帧
//* Remark:
//* Standard Version use's SDL_Delay() to control video's frame rate, it has 2
//* disadvantages:
//* (1)SDL's Screen can't be moved and always "Busy".
//* (2)Frame rate can't be accurate because it doesn't consider the time consumed 
//* by avcodec_decode_video2()
//* SU（SDL Update）Version solved 2 problems above. It create a thread to send SDL 
//* Event every 40ms to tell the main loop to decode and show video frames.
//*/

#include <stdio.h>
#include <iostream>
using namespace std;
#define __STDC_CONSTANT_MACROS

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
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
AVRational TIME_BASE_Q = {1,1000000};
//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
static  Uint8  *audio_chunk; 
static  Uint32  audio_len; 
static  Uint8  *audio_pos; 

void  fill_audio(void *udata,Uint8 *stream,int len){ 
	//SDL 2.0
	SDL_memset(stream, 0, len);
	if(audio_len==0)		/*  Only  play  if  we  have  data  left  */ 
		return; 

	len=(len>audio_len?audio_len:len);	/*  Mix  as  much  data  as  possible  */ 

	SDL_MixAudio(stream,audio_pos,len,SDL_MIX_MAXVOLUME);
	audio_pos += len; 
	audio_len -= len; 
} 




int thread_exit=0;
int thread_pause=0;

int sfp_refresh_thread(void *opaque){
	thread_exit=0;
	thread_pause=0;
	while (!thread_exit) 
	{
		if (!thread_exit)
		{
			SDL_Event event;
			event.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&event);
			SDL_Delay(0);
		}

		

	}
	thread_exit=0;
	thread_pause=0;
	SDL_Event event;
	event.type=SFM_REFRESH_EVENT;
	SDL_PushEvent(&event);
	//	SDL_Delay(40);
	return 0;
}

void FormatOpen( AVFormatContext** p, const char *filePath )
{
	if( avformat_open_input(p,filePath,NULL,NULL) != 0 )
	{
		cerr << "Could not open the file\n";
		exit(-1);
	}
}
void FindStreamInformation( AVFormatContext* p )
{
	if( avformat_find_stream_info( p,NULL ) < 0 )
	{
		cerr << "Could not find stream information.\n";
		exit(-1);
	}
}
int GetStreamIndex( AVFormatContext* p, AVMediaType type )
{
	for( int i = 0; i < p->nb_streams; ++i )
	{
		if( p->streams[i]->codec->codec_type == type )
		{
			return i;
		}//end if
	}//end for
	return -1;
}
int main(int argc, char* argv[])
{
	AVFormatContext	*pFormatCtx;
	int				i, videoindex,audioStream;
	AVCodecContext	*pCodecCtx,*pCodecAudioCtx;
	AVCodec			*pCodec,*pCodecAudio;
	AVFrame	*pFrame,*pFrameYUV;
	uint8_t *out_buffer;
	uint8_t *audio_out_buffer;
	AVPacket *packet;
	int ret, got_picture;
	int64_t in_channel_layout;


	//------------SDL----------------
	int screen_w,screen_h;
	SDL_Window *screen; 
	SDL_Renderer* sdlRenderer;
	SDL_Texture* sdlTexture;
	SDL_Rect sdlRect;
	SDL_Thread *video_tid;
	SDL_Event event;
	SDL_AudioSpec	wanted_spec;

	struct SwsContext *img_convert_ctx;
	struct SwrContext* au_conert_ctx;

	char filepath[]="F:/video/6s_kapian.avi";

	av_register_all();
	pFormatCtx = avformat_alloc_context();

	//打开文件
	//if( avformat_open_input(&pFormatCtx,filepath,NULL,NULL) != 0 )
	//{
	//	cerr << "Could not open the file\n";
	//	return -1;
	//}
	FormatOpen( &pFormatCtx, filepath );

	//获取流信息
	FindStreamInformation( pFormatCtx );

	//找到视频位置
	videoindex = GetStreamIndex( pFormatCtx, AVMEDIA_TYPE_VIDEO );
	if( -1 == videoindex )
	{
		cerr << "Did not find a video stream.\n";
		return -1;
	}
	//找到音频位置
	audioStream = GetStreamIndex( pFormatCtx, AVMEDIA_TYPE_AUDIO );
	if( -1 == audioStream )
	{
		cerr << "Did not find a audio stream.\n";
		return -1;
	}

	//找到流之后，设置流的编码上下文
	//以及设置编码器

	//设置编码器上下文
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	//设置编码器类型
	pCodec = avcodec_find_decoder( pCodecCtx->codec_id );

	pCodecAudioCtx = pFormatCtx->streams[audioStream]->codec;
	pCodecAudio = avcodec_find_decoder( pCodecAudioCtx->codec_id );


	if( pCodec == NULL )
	{
		cerr << "Codec not found.\n";
		return -1;
	}

	if( pCodecAudio == NULL )
	{
		cerr << "Codec not found.\n";
		return -1;
	}
	//该函数用于初始化一个视音频编解码器的AVCodecContext
	if( avcodec_open2(pCodecCtx,pCodec,NULL) < 0 )
	{
		cerr << "Could not open codec.\n";
		return -1;
	}

	if( avcodec_open2(pCodecAudioCtx,pCodecAudio,NULL) < 0 )
	{
		cerr << "Could not open codec.\n";
		return -1;
	}

	//Out Audio Param
	uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
	//nb_samples:
	int out_nb_samples = 1024;
	AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	int out_sample_rate = 44100;
	int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);

	//Out buffer size
	int out_buffer_size = av_samples_get_buffer_size( NULL, out_channels, out_nb_samples, out_sample_fmt, 1 );

	audio_out_buffer = (uint8_t *)av_malloc( MAX_AUDIO_FRAME_SIZE );

	//设置frame
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	//绑定一个buffer到pFrameYUV中
	out_buffer = (uint8_t *)malloc( avpicture_get_size( PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height ) );
	avpicture_fill( (AVPicture*)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height );




	//Output Info-----------------------------
	printf("---------------- File Information ---------------\n");
	av_dump_format(pFormatCtx,0,filepath,0);
	printf("-------------------------------------------------\n");

	//初始化图像转换上下文
	img_convert_ctx = sws_getContext( pCodecCtx->width, pCodecCtx->height,pCodecCtx->pix_fmt,
		pCodecCtx->width,pCodecCtx->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	//初始化SDL
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	} 
	//SDL_AudioSpec
	wanted_spec.freq = out_sample_rate; 
	wanted_spec.format = AUDIO_S16SYS; 
	wanted_spec.channels = out_channels; 
	wanted_spec.silence = 0; 
	wanted_spec.samples = out_nb_samples; 
	wanted_spec.callback = fill_audio; 
	wanted_spec.userdata = pCodecAudioCtx; 

	if (SDL_OpenAudio(&wanted_spec, NULL)<0){ 
		printf("can't open audio.\n"); 
		return -1; 
	} 
	//SDL 2.0 Support for multiple windows
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;
	screen = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h,SDL_WINDOW_OPENGL);

	if(!screen) {  
		printf("SDL: could not create window - exiting:%s\n",SDL_GetError());  
		return -1;
	}
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);  
	//IYUV: Y + U + V  (3 planes)
	//YV12: Y + V + U  (3 planes)
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,pCodecCtx->width,pCodecCtx->height);  

	sdlRect.x=0;
	sdlRect.y=0;
	sdlRect.w=screen_w;
	sdlRect.h=screen_h;

	packet = (AVPacket *)av_malloc( sizeof(AVPacket) );

	video_tid = SDL_CreateThread( sfp_refresh_thread,NULL,NULL );
	in_channel_layout = av_get_default_channel_layout(pCodecAudioCtx->channels);
	//swr

	au_conert_ctx = swr_alloc();
	au_conert_ctx = swr_alloc_set_opts( au_conert_ctx, out_channel_layout, out_sample_fmt, out_sample_rate,
		in_channel_layout, pCodecAudioCtx->sample_fmt, pCodecAudioCtx->sample_rate, 0, NULL);
	swr_init(au_conert_ctx);

	for(;;)
	{
		//Wait
		SDL_WaitEvent(&event);
		if( event.type == SFM_REFRESH_EVENT )
		{
			if( av_read_frame(pFormatCtx,packet) >= 0 )
			{
				if( packet->stream_index == videoindex )
				{
					ret = avcodec_decode_video2(pCodecCtx,pFrame,&got_picture,packet );
					if( ret < 0 )
					{
						printf("Decode Error.\n");
						return -1;
					}//end if( ret < 0 )

					if( got_picture )
					{
						//			pFrame->pts=av_frame_get_best_effort_timestamp(pFrame);
						//			printf("pts:%d\n",pFrame->pts);
						AVRational time_base = pFormatCtx->streams[videoindex]->time_base;
						pFrame->pts=av_frame_get_best_effort_timestamp( pFrame );
						double pts_time = av_rescale_q( pFrame->pts, time_base, TIME_BASE_Q )/1000000.0;
						printf( "vedio time:%d\n",pFrame->pts );
						sws_scale(img_convert_ctx,(const uint8_t* const*)pFrame->data, pFrame->linesize,
							0,pCodecCtx->height,pFrameYUV->data, pFrameYUV->linesize);
						//SDL--------
						SDL_UpdateTexture( sdlTexture, &sdlRect,pFrameYUV->data[0],pFrameYUV->linesize[0] );
						SDL_RenderClear( sdlRenderer );

						SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, NULL);  
						//SDL_RenderCopy( sdlRenderer, sdlTexture,&sdlRect,&sdlRect );
						SDL_RenderPresent( sdlRenderer );  
					}//end if( got_picture )
				}
				else if( packet->stream_index == audioStream )
				{
					ret = avcodec_decode_audio4(pCodecAudioCtx, pFrame, &got_picture, packet);
					if( ret < 0 ){
						printf("Error in decoding audio frame.\n");
						return -1;
					}//end if
					if( got_picture > 0 ){
						if( out_nb_samples != pFrame->nb_samples )
						{
							SDL_CloseAudio();
							out_nb_samples = pFrame->nb_samples;
							out_buffer_size = av_samples_get_buffer_size( NULL, out_channels, out_nb_samples, out_sample_fmt, 1 );
							wanted_spec.samples = out_nb_samples;
							if (SDL_OpenAudio(&wanted_spec, NULL)<0){ 
								printf("can't open audio.\n"); 
								return -1; 
							} 
						}
						swr_convert( au_conert_ctx, &audio_out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)pFrame->data, pFrame->nb_samples );
						AVRational time_base = pFormatCtx->streams[audioStream]->time_base;
						pFrame->pts=av_frame_get_best_effort_timestamp( pFrame );
						double pts_time = pFrame->pts / 44100.0;
						printf( "audio time:%d\n",pFrame->pts );
					//	printf("index:%5d\t pts:%lld\t packet size:%d\n",index,packet->pts,packet->size);

					}//end if
					while(audio_len>0)//Wait until finish
						SDL_Delay(1); 

					//Set audio buffer (PCM data)
					audio_chunk = (Uint8 *) audio_out_buffer; 
					//Audio buffer length
					audio_len =out_buffer_size;
					audio_pos = audio_chunk;

					//Play
					SDL_PauseAudio(0);
				}//end if (packet->stream_index == audioStream)
				av_free_packet(packet);
			}//end if(av_read_frame(pFormatCtx,packet) >= 0)
			else {
				thread_exit = 1;
				break;
			}//end else
		}//end if

		else if( event.type == SDL_QUIT )
		{
			thread_exit = 1;
			break;
		}//end else


	}//end for(;;)
	SDL_CloseAudio();//Close SDL
	swr_free(&au_conert_ctx);
	sws_freeContext(img_convert_ctx);
	SDL_Quit();

	//------
	av_free( pFrameYUV );
	av_free( pFrame );
	avcodec_close( pCodecCtx );
	avformat_close_input(&pFormatCtx);



	return 0;
}

