#include "decoder.h"
#include "transcoder.h"
#include "player.h"
#include "SDL_thread.h"
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
int main( int argc, char *argv[] )
{
	CVideoDecoder vDecoder;
	CAudioDecoder cDecoder;
	char url[] = "F:/video/6xCrash.avi";



	CTransCoder transcoder;
	

	CPlayer player;
	vDecoder.OpenInputFile( url, AVMEDIA_TYPE_VIDEO );
	cDecoder.OpenInputFile( url, AVMEDIA_TYPE_AUDIO );
	AVCodecContext* codecCtx = vDecoder.getCodecContext();
	AVCodecContext* audioCtx = cDecoder.getCodecContext();
	transcoder.SwsInit( codecCtx );
	transcoder.SwrInit(audioCtx);
	player.VideoInit( codecCtx->width, codecCtx->height );
	player.AudioInit(44100, 2, 1024, AV_SAMPLE_FMT_S16);
	AVFrame *frame = vDecoder.Process();
	AVFrame* audioFrame = cDecoder.Process();
	double audioTime = cDecoder.getTime();
	double vedioTime = vDecoder.getTime();
	SDL_Event event;
	for( ;; )
	{
		SDL_WaitEvent(&event);
		if( event.type == SFM_REFRESH_EVENT )
		{
			if( audioTime < vedioTime )
			{
				uint8_t* audioBuffer = transcoder.TransAudioFrame( audioFrame );
				player.UpdateNbsamples( audioFrame );
				player.Playsound( audioBuffer );

				audioFrame = cDecoder.Process();
				audioTime =  cDecoder.getTime();

				if( audioFrame == NULL )
				{
					player.SetThreadID(1);
					break;
				}

			}
			else
			{
				AVFrame *outFrame = transcoder.TransVideoFrame(frame);
				player.refresh( outFrame );

				frame = vDecoder.Process();
				vedioTime = vDecoder.getTime();

				if( frame == NULL )
				{
					player.SetThreadID(1);
					break;
				}
			}
		}
		else if( event.type == SDL_QUIT )
		{
			player.SetThreadID( 1 );
			break;
		}//end else


		//while( 1 )
		//{
		//	
		//	uint8_t* audioBuffer = transcoder.TransAudioFrame( audioFrame );
		//	player.UpdateNbsamples( audioFrame );
		//	player.Playsound( audioBuffer );
		//	audioFrame = cDecoder.Process();
		//	//audioTime = cDecoder.getTime();
		//}
		//if( frame != NULL )
		//{
		//	AVFrame *outFrame = transcoder.TransVideoFrame(frame);
		//	player.refresh( outFrame );

		//}
		//else
		//{
		//	player.SetThreadID( 1 );
		//	break;
		//}
		
	}//end for














	//for( ;; )
	//{
	//	


	//	event = player.GetEvent();
	//	//Wait
	//	SDL_WaitEvent( &event );
	//	if( event.type == SFM_REFRESH_EVENT )
	//	{
	//		AVFrame *frame = vDecoder.Process();
	//		AVFrame* audioFrame = cDecoder.Process();
	//		//double audioTime = cDecoder.getTime();
	//		//double vedioTime = vDecoder.getTime();
	//		/*		while( audioTime < vedioTime )
	//		{
	//		uint8_t* audioBuffer = transcoder.TransAudioFrame( audioFrame );
	//		player.UpdateNbsamples( audioFrame );
	//		player.Playsound( audioBuffer );
	//		audioFrame = cDecoder.Process();
	//		audioTime = cDecoder.getTime();
	//		}
	//		*/
	//		uint8_t* audioBuffer = transcoder.TransAudioFrame( audioFrame );
	//		player.UpdateNbsamples( audioFrame );
	//		player.Playsound( audioBuffer );
	//		//if( frame != NULL )
	//		//{
	//		////	AVFrame *outFrame = transcoder.TransVideoFrame(frame);
	//		//	uint8_t* audioBuffer = transcoder.TransAudioFrame( audioFrame );
	//		//	player.UpdateNbsamples( audioFrame );
	//		//	player.Playsound( audioBuffer );
	//		////	player.refresh( outFrame );
	//		//	
	//		//}
	//		//else
	//		//{
	//		//	player.SetThreadID( 1 );
	//		//	break;
	//		//}
	//	}
	//	else if( event.type == SDL_QUIT )
	//	{
	//		player.SetThreadID( 1 );
	//		break;
	//	}
	//}//end for


	

	return 0;
}