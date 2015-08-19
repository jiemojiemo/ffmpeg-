/**
 * 最简单的SDL2播放音频的例子（SDL2播放PCM）
 * Simplest Audio Play SDL2 (SDL2 play PCM) 
 *
 * 雷霄骅 Lei Xiaohua
 * leixiaohua1020@126.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * 本程序使用SDL2播放PCM音频采样数据。SDL实际上是对底层绘图
 * API（Direct3D，OpenGL）的封装，使用起来明显简单于直接调用底层
 * API。
 *
 * 函数调用步骤如下: 
 *
 * [初始化]
 * SDL_Init(): 初始化SDL。
 * SDL_OpenAudio(): 根据参数（存储于SDL_AudioSpec）打开音频设备。
 *
 * [循环播放数据]
 * SDL_PauseAudio(): 播放音频数据。
 * SDL_Delay(): 延时等待播放完成。
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
//当音频设备需要更多数据的时候就会调用这个回调函数
//void (SDLCALL * SDL_AudioCallback) (void *userdata, Uint8 * stream,int len);
//userdata：SDL_AudioSpec结构中的用户自定义数据，一般情况下可以不用。
//stream：该指针指向需要填充的音频缓冲区。
//	len：音频缓冲区的大小（以字节为单位）。
//SDL2有一个地方和SDL1.x不一样：SDL2中必须首先使用SDL_memset()将stream中的数据设置为0。
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
	//音频规格结构体
	SDL_AudioSpec wanted_spec;
	wanted_spec.freq = 44100;			//音频采样率，常用的有48000，44100
	wanted_spec.format = AUDIO_F32;		//音频数据格式
	wanted_spec.channels = 1;			//声道数
	wanted_spec.silence = 0;			//设置静音的值
	wanted_spec.samples = 1024;			//音频缓冲区的采样个数，要求必须是2的n次方
	wanted_spec.callback = fill_audio;	//填充音频缓冲区的回调函数

	//    int SDLCALL SDL_OpenAudio(SDL_AudioSpec * desired, SDL_AudioSpec * obtained);  
	//desired：期望的参数。
	//obtained：实际音频设备的参数，一般情况下设置为NULL即可。
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
		//使用SDL_PauseAudio()可以播放音频数据
		//void SDLCALL SDL_PauseAudio(int pause_on)
		//当pause_on设置为0的时候即可开始播放音频数据。设置为1的时候，将会播放静音的值。
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