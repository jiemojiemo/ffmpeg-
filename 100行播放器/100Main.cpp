#include <stdio.h>
#include <iostream>
using namespace std;

#define __STDC_CONSTANT_MACROS
#define _DEBUG 1

#ifdef _WIN32
//Window
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL.h"
};

#endif

#define OUTPUT_YUV420P 1

int main( int argc, char* argv[] )
{
	AVFormatContext *pFormatCtx;
	int				i,videoindex;
	AVCodecContext  *pCodecCtx;
	AVCodec			*pCodec;
	AVFrame	*pFrame, *pFrameYUV;
	uint8_t *out_buffer;
	AVPacket *packet;
	int y_size;
	int ret, got_picture;
	struct SwsContext *img_convert_ctx;

	char filepath[]="F:/video/6s_kapian.mp4";

	//SDL------------------------
	int screen_w = 0;
	int screen_h = 0;

	SDL_Window *screen;
	SDL_Renderer *sdlRenderer;
	SDL_Texture *sdlTexture;
	SDL_Rect sdlRect;

	FILE *fp_yuv;

	//注册了所有的文件格式和编解码器的库
	//这些库将自动的使用在被打开的合适格式的文件
	//只需要调用av_register_all()一次。
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	//获取输入文件的头信息
	//打开了一个文件
	if( avformat_open_input( &pFormatCtx, filepath, NULL, NULL ) != 0 )
	{
		printf( "Couldn't open input stream.\n" );
		return -1;
	}

	//get stream information
	//该函数主要用于给每个媒体流（音频/视频）的AVStream结构体赋值
	if( avformat_find_stream_info( pFormatCtx, NULL ) < 0 )
	{
		printf( "Couldn't find stream information.\n" );
		return -1;
	}

	videoindex = -1;
	cout <<__LINE__ << "nb_streams:" << pFormatCtx->nb_streams << endl;
	//在音频流、视频流、字幕流中找到你想要的流
	for( i = 0; i < pFormatCtx->nb_streams; ++i )
	{
		//如果是视频
		//这里的值，在avformat_find_stream_info时已经被设置好了
		if( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
		{
			videoindex = i;
			break;
		}//end if
	}//end for
	
	if( -1 == videoindex )
	{
		printf( "Didn't find a video stream.\n" );
		return -1;
	}


	cout << __LINE__ << "  yyyyyy" << videoindex << endl;


	pCodecCtx = pFormatCtx->streams[videoindex]->codec;


	// 通过code ID查找一个已经注册的音视频解码器
	// 引入 #include "libavcodec/avcodec.h"
	// 实现在: \ffmpeg\libavcodec\utils.c
	// 查找解码器之前,必须先调用av_register_all注册所有支持的解码器
	// 查找成功返回解码器指针,否则返回NULL
	// 音视频解码器保存在一个链表中,查找过程中,函数从头到尾遍历链表,通过比较解码器的ID来查找
	pCodec = avcodec_find_decoder( pCodecCtx->codec_id );

	if( pCodec == NULL )
	{
		printf( "Codec not found.\n" );
		return -1;
	}

	//avcodec_open2();作用
	/*（1）为各种结构体分配内存（通过各种av_malloc()实现）。
		（2）将输入的AVDictionary形式的选项设置到AVCodecContext。
		（3）其他一些零零碎碎的检查，比如说检查编解码器是否处于“实验”阶段。
		（4）如果是编码器，检查输入参数是否符合编码器的要求
		（5）调用AVCodec的init()初始化具体的解码器。*/
	//该函数用于初始化一个视音频编解码器的AVCodecContext
	if( avcodec_open2( pCodecCtx, pCodec, NULL ) <0 )
	{
		printf( "Could not open codec.\n" );
		return -1;
	}

	/*av_frame_alloc该函数仅仅分配AVFrame实例本身，而没有分配其内部的缓存*/
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	//根据宽高格式，来计算图片的字节数
	out_buffer = (uint8_t *)malloc( avpicture_get_size( PIX_FMT_YUV420P,pCodecCtx->width, pCodecCtx->height) );

	/*
	这个函数的使用本质上是为已经分配的空间的结构体AVPicture挂上一段用于保存数据的空间，
		这个结构体中有一个指针数组data[4]，挂在这个数组里。一般我们这么使用：
	*/
	avpicture_fill( (AVPicture *)pFrameYUV,out_buffer,PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height );
	packet = (AVPacket *)av_malloc( sizeof(AVPacket) );

	//Output Info-----------
	printf( "---------------------File Information---------------------\n" );
	av_dump_format( pFormatCtx,0,filepath,0 );
	printf( "----------------------------------------------------------\n" );
	//这个函数干嘛用的
	//建立一个上下文用于编码
	img_convert_ctx = sws_getContext( pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height,PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

#if OUTPUT_YUV420P
	fp_yuv = fopen( "output.yuv","wb+" );
#endif

	//进行SDL初始化
	//使用SDL进行播放视频
	if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER ) )
	{
		printf( "Could not initialize SDL - %s\n", SDL_GetError() );
		return -1;
	}

	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;

	////SDL 2.0 Support for multiple windows
	screen = SDL_CreateWindow( "Simplest ffmpeg player's Window",SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h, SDL_WINDOW_ALLOW_HIGHDPI );

	if( !screen )
	{
		printf("SDL: could not create window - exiting:%s\n",SDL_GetError());  
		return -1;
	}

	sdlRenderer = SDL_CreateRenderer( screen, -1, 0 );
	//IYUV: Y + U + V  (3 planes)
	//YV12: Y + V + U  (3 planes)
	sdlTexture = SDL_CreateTexture( sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width,pCodecCtx->height );

	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = screen_w;
	sdlRect.h = screen_h;

	//SDL End-------------------
	while( av_read_frame( pFormatCtx, packet ) >= 0 )
	{
		if( packet->stream_index == videoindex )
		{
			//ffmpeg中的avcodec_decode_video2()的作用是解码一帧视频数据。
			//输入一个压缩编码的结构体AVPacket，输出一个解码后的结构体AVFrame。
			//将packet中的数据解码到pFrame中去
			ret = avcodec_decode_video2( pCodecCtx, pFrame, &got_picture, packet );
			//printf(  "packet %d\n", ret);
			if( ret < 0 )
			{
				printf( "DEcode Errot.\n" );
				return -1;
			}//end if
			if( got_picture)
			{
				//从img_convert_ctx中获取参数
				sws_scale(img_convert_ctx, (const uint8_t * const *)pFrame->data,pFrame->linesize,0,pCodecCtx->height,
					pFrameYUV->data, pFrameYUV->linesize);
#if OUTPUT_YUV420P
				y_size=pCodecCtx->width*pCodecCtx->height;  
				fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y 
				fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
				fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
#endif
				//SDL---------------------------
#if 0
				SDL_UpdateTexture( sdlTexture, NULL, pFrameYUV->data[0], pFrameYUV->linesize[0] );  
#else
				SDL_UpdateYUVTexture(sdlTexture, &sdlRect,
					pFrameYUV->data[0], pFrameYUV->linesize[0],
					pFrameYUV->data[1], pFrameYUV->linesize[1],
					pFrameYUV->data[2], pFrameYUV->linesize[2]);
#endif	
				SDL_RenderClear( sdlRenderer );
				SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, &sdlRect );
				SDL_RenderPresent( sdlRenderer );

				//SDL End----------
				//Delay 40ms
				SDL_Delay( 40 );
			}//end if
		}//end if
		av_free_packet( packet );
	}//end while

	//读取一些残留数据
	while( 1 )
	{
		ret = avcodec_decode_video2( pCodecCtx, pFrame, &got_picture, packet );
		if( ret < 0 )
			break;
		if( !got_picture )
			break;
		sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, 
			pFrameYUV->data, pFrameYUV->linesize);
#if OUTPUT_YUV420P
		int y_size=pCodecCtx->width*pCodecCtx->height;  
		fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y 
		fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
		fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
#endif
		//SDL---------------------------
		SDL_UpdateTexture( sdlTexture, &sdlRect, pFrameYUV->data[0], pFrameYUV->linesize[0] );  
		SDL_RenderClear( sdlRenderer );  
		SDL_RenderCopy( sdlRenderer, sdlTexture,  NULL, &sdlRect);  
		SDL_RenderPresent( sdlRenderer );  
		//SDL End-----------------------
		//Delay 40ms
		SDL_Delay(40);

	}//end while
	
	sws_freeContext(img_convert_ctx);

#if OUTPUT_YUV420P 
	fclose(fp_yuv);
#endif 

	SDL_Quit();

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}