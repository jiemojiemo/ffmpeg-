#ifndef PLAYER_H
#define PLAYER_H

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


class CPlayer
{

private:
	static int m_screen_w,m_screen_h;
	static SDL_Window * m_pscreen; 
	static SDL_Renderer* m_psdlRenderer;
	static SDL_Texture* m_psdlTexture;
	static SDL_Rect m_sdlRect;
	static SDL_Thread *m_pmvideo_tid;
	static SDL_Event m_event; 
	static SDL_AudioSpec	m_wanted_spec;
	static uint8_t*		m_audio_out_buffer;
	static  Uint8  *m_audio_chunk; 
	static  Uint32  m_audio_len; 
	static  Uint8  *m_audio_pos; 
	static  int m_thread_exit;
	static  int m_buffer_size;
private:
	CPlayer( const CPlayer& play ){}
	CPlayer& operator=( const CPlayer& play ){}
	static void AudioCallBack( void *udata,Uint8 *stream,int len );
	static int sfp_refresh_thread(void *opaque);
public:
	CPlayer();
	static int AudioInit(int samplet_rate, int channels,int nb_samples,AVSampleFormat sample_fmt);
    static int VideoInit(int width, int height);
	int refresh(AVFrame* pframe);
	void SetThreadID( int i ){ m_thread_exit = i; }
	SDL_Event GetEvent(){ return m_event; }
	int UpdateNbsamples(AVFrame* frame);
	int Playsound(uint8_t* buffer);
};

#endif