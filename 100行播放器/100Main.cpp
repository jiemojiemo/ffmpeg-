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

	//ע�������е��ļ���ʽ�ͱ�������Ŀ�
	//��Щ�⽫�Զ���ʹ���ڱ��򿪵ĺ��ʸ�ʽ���ļ�
	//ֻ��Ҫ����av_register_all()һ�Ρ�
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	//��ȡ�����ļ���ͷ��Ϣ
	//����һ���ļ�
	if( avformat_open_input( &pFormatCtx, filepath, NULL, NULL ) != 0 )
	{
		printf( "Couldn't open input stream.\n" );
		return -1;
	}

	//get stream information
	//�ú�����Ҫ���ڸ�ÿ��ý��������Ƶ/��Ƶ����AVStream�ṹ�帳ֵ
	if( avformat_find_stream_info( pFormatCtx, NULL ) < 0 )
	{
		printf( "Couldn't find stream information.\n" );
		return -1;
	}

	videoindex = -1;
	cout <<__LINE__ << "nb_streams:" << pFormatCtx->nb_streams << endl;
	//����Ƶ������Ƶ������Ļ�����ҵ�����Ҫ����
	for( i = 0; i < pFormatCtx->nb_streams; ++i )
	{
		//�������Ƶ
		//�����ֵ����avformat_find_stream_infoʱ�Ѿ������ú���
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


	// ͨ��code ID����һ���Ѿ�ע�������Ƶ������
	// ���� #include "libavcodec/avcodec.h"
	// ʵ����: \ffmpeg\libavcodec\utils.c
	// ���ҽ�����֮ǰ,�����ȵ���av_register_allע������֧�ֵĽ�����
	// ���ҳɹ����ؽ�����ָ��,���򷵻�NULL
	// ����Ƶ������������һ��������,���ҹ�����,������ͷ��β��������,ͨ���ȽϽ�������ID������
	pCodec = avcodec_find_decoder( pCodecCtx->codec_id );

	if( pCodec == NULL )
	{
		printf( "Codec not found.\n" );
		return -1;
	}

	//avcodec_open2();����
	/*��1��Ϊ���ֽṹ������ڴ棨ͨ������av_malloc()ʵ�֣���
		��2���������AVDictionary��ʽ��ѡ�����õ�AVCodecContext��
		��3������һЩ��������ļ�飬����˵����������Ƿ��ڡ�ʵ�顱�׶Ρ�
		��4������Ǳ������������������Ƿ���ϱ�������Ҫ��
		��5������AVCodec��init()��ʼ������Ľ�������*/
	//�ú������ڳ�ʼ��һ������Ƶ���������AVCodecContext
	if( avcodec_open2( pCodecCtx, pCodec, NULL ) <0 )
	{
		printf( "Could not open codec.\n" );
		return -1;
	}

	/*av_frame_alloc�ú�����������AVFrameʵ��������û�з������ڲ��Ļ���*/
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	//���ݿ�߸�ʽ��������ͼƬ���ֽ���
	out_buffer = (uint8_t *)malloc( avpicture_get_size( PIX_FMT_YUV420P,pCodecCtx->width, pCodecCtx->height) );

	/*
	���������ʹ�ñ�������Ϊ�Ѿ�����Ŀռ�Ľṹ��AVPicture����һ�����ڱ������ݵĿռ䣬
		����ṹ������һ��ָ������data[4]��������������һ��������ôʹ�ã�
	*/
	avpicture_fill( (AVPicture *)pFrameYUV,out_buffer,PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height );
	packet = (AVPacket *)av_malloc( sizeof(AVPacket) );

	//Output Info-----------
	printf( "---------------------File Information---------------------\n" );
	av_dump_format( pFormatCtx,0,filepath,0 );
	printf( "----------------------------------------------------------\n" );
	//������������õ�
	//����һ�����������ڱ���
	img_convert_ctx = sws_getContext( pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height,PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

#if OUTPUT_YUV420P
	fp_yuv = fopen( "output.yuv","wb+" );
#endif

	//����SDL��ʼ��
	//ʹ��SDL���в�����Ƶ
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
			//ffmpeg�е�avcodec_decode_video2()�������ǽ���һ֡��Ƶ���ݡ�
			//����һ��ѹ������Ľṹ��AVPacket�����һ�������Ľṹ��AVFrame��
			//��packet�е����ݽ��뵽pFrame��ȥ
			ret = avcodec_decode_video2( pCodecCtx, pFrame, &got_picture, packet );
			//printf(  "packet %d\n", ret);
			if( ret < 0 )
			{
				printf( "DEcode Errot.\n" );
				return -1;
			}//end if
			if( got_picture)
			{
				//��img_convert_ctx�л�ȡ����
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

	//��ȡһЩ��������
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