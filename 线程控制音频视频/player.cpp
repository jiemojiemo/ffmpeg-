#include "player.h"
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

Uint8* CPlayer::m_audio_chunk = NULL;
Uint32 CPlayer::m_audio_len = 0;
Uint8* CPlayer::m_audio_pos = NULL;
uint8_t* CPlayer::m_audio_out_buffer = NULL;
SDL_AudioSpec CPlayer::m_wanted_spec;
int CPlayer:: m_thread_exit=0;
SDL_Window* CPlayer:: m_pscreen; 
SDL_Renderer* CPlayer::m_psdlRenderer;
SDL_Texture* CPlayer::m_psdlTexture;
SDL_Rect CPlayer::m_sdlRect;
SDL_Thread * CPlayer::m_pmvideo_tid;
int CPlayer::m_screen_w;
int CPlayer::m_screen_h;
SDL_Event CPlayer::m_event;
int CPlayer::m_buffer_size;
int thread_exit=0;
int thread_pause=0;
CPlayer::CPlayer()
{

}

int CPlayer::sfp_refresh_thread(void *opaque){

	//while (m_thread_exit==0) {
	//	SDL_Event event;
	//	event.type = SFM_REFRESH_EVENT;
	//	SDL_PushEvent(&event);
	//	SDL_Delay(40);
	//}	
	//return 0;
	thread_exit=0;
	thread_pause=0;
	while (!thread_exit) 
	{
		if (!thread_exit)
		{
			SDL_Event event;
			event.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&event);
		}

			SDL_Delay(20);

	}
	thread_exit=0;
	thread_pause=0;
	SDL_Event event;
	event.type=SFM_REFRESH_EVENT;
	SDL_PushEvent(&event);
	//	SDL_Delay(40);
	return 0;
}

void CPlayer::AudioCallBack( void *udata,Uint8 *stream,int len )
{
	//SDL 2.0
	SDL_memset(stream, 0, len);
	if(m_audio_len==0)		/*  Only  play  if  we  have  data  left  */ 
		return; 
	len=(len>m_audio_len?m_audio_len:len);	/*  Mix  as  much  data  as  possible  */ 

	SDL_MixAudio(stream,m_audio_pos,len,SDL_MIX_MAXVOLUME);
	m_audio_pos += len; 
	m_audio_len -= len; 
}

int CPlayer::VideoInit(int width, int height)
{
	//SDL 2.0 Support for multiple windows
	m_screen_w = width;
	m_screen_h = height;
	m_pscreen = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		m_screen_w, m_screen_h,SDL_WINDOW_OPENGL);

	if(!m_pscreen) {  
		printf("SDL: could not create window - exiting:%s\n",SDL_GetError());  
		return -1;
	}
	m_psdlRenderer = SDL_CreateRenderer(m_pscreen, -1, 0);  
	//IYUV: Y + U + V  (3 planes)
	//YV12: Y + V + U  (3 planes)
	m_psdlTexture = SDL_CreateTexture(m_psdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,width,height);  

	m_sdlRect.x=0;
	m_sdlRect.y=0;
	m_sdlRect.w=m_screen_w;
	m_sdlRect.h=m_screen_h;

	m_pmvideo_tid = SDL_CreateThread( sfp_refresh_thread,NULL,NULL );
}
int CPlayer::AudioInit(int samplet_rate, int channels,int nb_samples,AVSampleFormat sample_fmt)
{
	m_buffer_size = av_samples_get_buffer_size( NULL, channels, nb_samples, sample_fmt, 1 );

	m_wanted_spec.freq=samplet_rate;
	m_wanted_spec.format=AUDIO_S16SYS;
	m_wanted_spec.silence=0;
	m_wanted_spec.channels=channels;
	m_wanted_spec.samples=nb_samples;
	m_wanted_spec.callback=AudioCallBack;

	if (SDL_OpenAudio(&m_wanted_spec, NULL)<0){ 
		printf("can't open audio.\n"); 
		return -1; 
	} 
	
}

int CPlayer::Playsound(uint8_t* buffer)
{
	while(m_audio_len>0)
		SDL_Delay(1);
	m_audio_chunk=(Uint8*)buffer;
	m_audio_len=m_buffer_size;
	m_audio_pos=m_audio_chunk;
	SDL_PauseAudio(0);
	return 1;
}

int CPlayer::UpdateNbsamples(AVFrame* frame)
{
	if( m_wanted_spec.samples != frame->nb_samples )
	{
		//SDL_CloseAudio();
		//m_wanted_spec.samples= frame->nb_samples;
		m_buffer_size = av_samples_get_buffer_size( NULL, m_wanted_spec.channels, frame->nb_samples, AV_SAMPLE_FMT_S16, 1 );
		//if (SDL_OpenAudio(&m_wanted_spec, NULL)<0)
		//{ 
	//		printf("can't open audio.\n"); 
	//		return -1; 
	//	} 
	}
	return 0;
}
int CPlayer::refresh(AVFrame* pframe)
{
	SDL_UpdateTexture( m_psdlTexture, &m_sdlRect,pframe->data[0],pframe->linesize[0] );
	SDL_RenderClear( m_psdlRenderer );

	SDL_RenderCopy( m_psdlRenderer, m_psdlTexture, NULL, NULL);  
	//SDL_RenderCopy( sdlRenderer, sdlTexture,&sdlRect,&sdlRect );
	SDL_RenderPresent( m_psdlRenderer ); 

	return 1;
}