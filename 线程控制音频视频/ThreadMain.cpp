#include "decoder.h"
#include "transcoder.h"
#include "player.h"
#include "SDL_thread.h"
#include <list>
using std::list;
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
CVideoDecoder vDecoder;
CAudioDecoder cDecoder;
CTransCoder transcoder;
CPlayer player;
char url[] = "F:/video/6xCrash_1.avi";
list<AVFrame*> frameList;
AVFrame* myFrame;
bool getFrame = true;
static int i = 0;

int AudioThread( void *data )
{
	cDecoder.OpenInputFile( url, AVMEDIA_TYPE_AUDIO );
	AVCodecContext* audioCtx = cDecoder.getCodecContext();
	transcoder.SwrInit(audioCtx);
	player.AudioInit(44100, 2, 1024, AV_SAMPLE_FMT_S16);
	for(;;)
	{
		AVFrame* audioFrame = cDecoder.Process();
		if( audioFrame == NULL )
			break;
		uint8_t* audioBuffer = transcoder.TransAudioFrame( audioFrame );
		player.UpdateNbsamples( audioFrame );
		player.Playsound( audioBuffer );
		//printf( "%d\n", i++);
	}
	return 10;
}
int VideoThread( void * data )
{
	vDecoder.OpenInputFile( url, AVMEDIA_TYPE_VIDEO );
	
	AVCodecContext* codecCtx = vDecoder.getCodecContext();
	player.VideoInit( codecCtx->width, codecCtx->height );
	transcoder.SwsInit( codecCtx );
	
	SDL_Event event;
	for(;;)
	{
		SDL_WaitEvent(&event);
		if( event.type == SFM_REFRESH_EVENT )
		{
			AVFrame *frame = vDecoder.Process();
			if( frame == NULL )
			{
				player.SetThreadID(1);
				break;
			}
			AVFrame *outFrame = transcoder.TransVideoFrame(frame);
			player.refresh( outFrame );
		}
		else if( event.type == SDL_QUIT )
		{
			player.SetThreadID( 1 );
			break;
		}//end else
		
	}
	return 100;
}
int CreateFrame( void *data )
{
	//vDecoder.OpenInputFile( url, AVMEDIA_TYPE_VIDEO );
	AVCodecContext* codecCtx = vDecoder.getCodecContext();

	transcoder.SwsInit( codecCtx );

	for(;;)
	{

		if(getFrame)
		{
			AVFrame *frame = vDecoder.Process();
			if( frame == NULL )
			{
				return -1;
				break;
			}
			AVFrame *outFrame = transcoder.TransVideoFrame(frame);
			myFrame = outFrame;
			getFrame = false;
		}

		//SDL_mutexV( lock );
	}
	return 100;
}

int main( int argc, char* argv[] )
{
	

	vDecoder.OpenInputFile( url, AVMEDIA_TYPE_VIDEO );
	AVCodecContext* codecCtx = vDecoder.getCodecContext();
	player.VideoInit( codecCtx->width, codecCtx->height );

	SDL_Thread* audiothread = SDL_CreateThread( AudioThread,"audio", NULL );
//	SDL_Thread* videothread = SDL_CreateThread( VideoThread,"video", NULL );
	SDL_Thread* framethread = SDL_CreateThread( CreateFrame,"list", NULL );
	SDL_Event event;
	for(;;)
	{
		SDL_WaitEvent(&event);
		if( event.type == SFM_REFRESH_EVENT )
		{
			if( !getFrame )
			{
				player.refresh( myFrame );
				getFrame = true;
			}
		}
		else if( event.type == SDL_QUIT )
		{
			player.SetThreadID( 1 );
			break;
		}//end else

	}

	if( audiothread == NULL )
	{
		printf( "´´½¨Ê§°Ü\n" );

	}

	int lots;
	SDL_WaitThread(audiothread, &lots);
	printf( "%d\n", lots );
	
	return 0;
}