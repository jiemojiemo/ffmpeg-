/**
 * ��򵥵�SDL2������Ƶ�����ӣ�SDL2����PCM��
 * Simplest Audio Play SDL2 (SDL2 play PCM) 
 *
 * ������ Lei Xiaohua
 * leixiaohua1020@126.com
 * �й���ý��ѧ/���ֵ��Ӽ���
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * ������ʹ��SDL2����PCM��Ƶ�������ݡ�SDLʵ�����ǶԵײ��ͼ
 * API��Direct3D��OpenGL���ķ�װ��ʹ���������Լ���ֱ�ӵ��õײ�
 * API��
 *
 * �������ò�������: 
 *
 * [��ʼ��]
 * SDL_Init(): ��ʼ��SDL��
 * SDL_OpenAudio(): ���ݲ������洢��SDL_AudioSpec������Ƶ�豸��
 *
 * [ѭ����������]
 * SDL_PauseAudio(): ������Ƶ���ݡ�
 * SDL_Delay(): ��ʱ�ȴ�������ɡ�
 *
 * This software plays PCM raw audio data using SDL2.
 * SDL is a wrapper of low-level API (DirectSound).
 * Use SDL is much easier than directly call these low-level API.
 *
 * The process is shown as follows:
 *
 * [Init]
 * SDL_Init(): Init SDL.
 * SDL_OpenAudio(): Opens the audio device with the desired 
 *					parameters (In SDL_AudioSpec).
 *
 * [Loop to play data]
 * SDL_PauseAudio(): Play Audio.
 * SDL_Delay(): Wait for completetion of playback.
 */

#include <stdio.h>
#include <tchar.h>

extern "C"
{
#include <SDL.h>
};

//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
static Uint8 *audio_chunk;
static Uint32 audio_len;
static Uint8 *audio_pos;

/* Audio Callback
 * The audio function callback takes the following parameters: 
 * stream: A pointer to the audio buffer to be filled 
 * len: The length (in bytes) of the audio buffer 
 * 
*/ 
//����Ƶ�豸��Ҫ�������ݵ�ʱ��ͻ��������ص�����
//void (SDLCALL * SDL_AudioCallback) (void *userdata, Uint8 * stream,int len);
//userdata��SDL_AudioSpec�ṹ�е��û��Զ������ݣ�һ������¿��Բ��á�
//stream����ָ��ָ����Ҫ������Ƶ��������
//	len����Ƶ�������Ĵ�С�����ֽ�Ϊ��λ����
//SDL2��һ���ط���SDL1.x��һ����SDL2�б�������ʹ��SDL_memset()��stream�е���������Ϊ0��
void fill_audio( void *udata, Uint8 *stream, int len ){
	SDL_memset( stream, 0, len );
	if(0 == audio_len )
		return;

	len = ( len > audio_len?audio_len:len );
	
	SDL_MixAudio( stream, audio_pos, len, SDL_MIX_MAXVOLUME );

	audio_pos +=len;
	audio_len -=len;
}

int main( int argc, char *argv[] )
{
	//Init
	if( SDL_Init( SDL_INIT_AUDIO | SDL_INIT_TIMER ) ){
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	}
	//SDL_AudioSpec -- Audio Specification Structure 
	//��Ƶ���ṹ��
	SDL_AudioSpec wanted_spec;
	wanted_spec.freq = 44100;			//��Ƶ�����ʣ����õ���48000��44100
	wanted_spec.format = AUDIO_F32;		//��Ƶ���ݸ�ʽ
	wanted_spec.channels = 1;			//������
	wanted_spec.silence = 0;			//���þ�����ֵ
	wanted_spec.samples = 1024;			//��Ƶ�������Ĳ���������Ҫ�������2��n�η�
	wanted_spec.callback = fill_audio;	//�����Ƶ�������Ļص�����

	//    int SDLCALL SDL_OpenAudio(SDL_AudioSpec * desired, SDL_AudioSpec * obtained);  
	//desired�������Ĳ�����
	//obtained��ʵ����Ƶ�豸�Ĳ�����һ�����������ΪNULL���ɡ�
	if( SDL_OpenAudio(&wanted_spec, NULL ) < 0 ){
		printf( "can't open audou.\n" );
		return -1;
	}
	char filePath[] = "1_32_slow";
	FILE *fp = fopen( filePath, "rb+" );
	if( fp == NULL )
	{
		printf( "cannot open this file\n" );
		return -1;
	}

	//For YUV420P
	int pcm_buffer_size = 4096;
	char *pcm_buffer = (char*)malloc( pcm_buffer_size );
	int data_count = 0;

	while( 1 ){
		if( fread(pcm_buffer, 1, pcm_buffer_size, fp) != pcm_buffer_size ){
			//Loop
			fseek( fp, 0, SEEK_SET );
			fread( pcm_buffer, 1, pcm_buffer_size, fp );
			data_count = 0;
		}//end if
		printf("Now Playing %10d Bytes data.\n",data_count);
		data_count += pcm_buffer_size;

		//Set audio buffer (PCM data)
		audio_chunk = (Uint8 *)pcm_buffer;
		//Audio buffer length
		audio_len = pcm_buffer_size;
		audio_pos = audio_chunk;
		//Play
		//ʹ��SDL_PauseAudio()���Բ�����Ƶ����
		//void SDLCALL SDL_PauseAudio(int pause_on)
		//��pause_on����Ϊ0��ʱ�򼴿ɿ�ʼ������Ƶ���ݡ�����Ϊ1��ʱ�򣬽��Ქ�ž�����ֵ��
		SDL_PauseAudio( 0 );
		while( audio_len > 0 )
		{
			SDL_Delay(1);
		}
	}//end while

	free( pcm_buffer );
	SDL_Quit();
	return 0 ;
}