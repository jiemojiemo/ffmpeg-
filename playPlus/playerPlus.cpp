

extern "C"{
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}
#include "SDL.h"
#include "SDL_thread.h"

#include <iostream>
using namespace std;

#pragma warning(disable: 4996)

#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"swresample.lib")

#pragma comment(lib,"sdl2.lib")

#define SDL_AUDIO_BUFFER_SIZE 1152
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

static Uint8 *audio_chunk;
static Uint32 audio_len;
static Uint8 *audio_pos;

void fill_audio(void *udata, Uint8 *stream, int len){
	if (audio_len == 0)
		return;
	len = (len > audio_len ? audio_len : len);
	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;
}

int AudioResampling(AVCodecContext * audio_dec_ctx,
	AVFrame * pAudioDecodeFrame,
	int out_sample_fmt,
	int out_channels,
	int out_sample_rate,
	uint8_t* out_buf)
{
	SwrContext * swr_ctx = NULL;
	int data_size = 0;
	int ret = 0;
	int64_t src_ch_layout = audio_dec_ctx->channel_layout;
	int64_t dst_ch_layout = AV_CH_LAYOUT_STEREO;
	int dst_nb_channels = 0;
	int dst_linesize = 0;
	int src_nb_samples = 0;
	int dst_nb_samples = 0;
	int max_dst_nb_samples = 0;
	uint8_t **dst_data = NULL;
	int resampled_data_size = 0;

	swr_ctx = swr_alloc();
	if (!swr_ctx)
	{
		printf("swr_alloc error \n");
		return -1;
	}

	src_ch_layout = (audio_dec_ctx->channels ==
		av_get_channel_layout_nb_channels(audio_dec_ctx->channel_layout)) ?
		audio_dec_ctx->channel_layout :
		av_get_default_channel_layout(audio_dec_ctx->channels);

	if (out_channels == 1)
	{
		dst_ch_layout = AV_CH_LAYOUT_MONO;
		//printf("dst_ch_layout: AV_CH_LAYOUT_MONO\n");
	}
	else if (out_channels == 2)
	{
		dst_ch_layout = AV_CH_LAYOUT_STEREO;
		//printf("dst_ch_layout: AV_CH_LAYOUT_STEREO\n");
	}
	else
	{
		dst_ch_layout = AV_CH_LAYOUT_SURROUND;
		//printf("dst_ch_layout: AV_CH_LAYOUT_SURROUND\n");
	}

	if (src_ch_layout <= 0)
	{
		printf("src_ch_layout error \n");
		return -1;
	}

	src_nb_samples = pAudioDecodeFrame->nb_samples;
	if (src_nb_samples <= 0)
	{
		printf("src_nb_samples error \n");
		return -1;
	}

	av_opt_set_int(swr_ctx, "in_channel_layout", src_ch_layout, 0);
	av_opt_set_int(swr_ctx, "in_sample_rate", audio_dec_ctx->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", audio_dec_ctx->sample_fmt, 0);

	av_opt_set_int(swr_ctx, "out_channel_layout", dst_ch_layout, 0);
	av_opt_set_int(swr_ctx, "out_sample_rate", out_sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", (AVSampleFormat)out_sample_fmt, 0);

	if ((ret = swr_init(swr_ctx)) < 0) {
		printf("Failed to initialize the resampling context\n");
		return -1;
	}

	max_dst_nb_samples = dst_nb_samples = av_rescale_rnd(src_nb_samples,
		out_sample_rate, audio_dec_ctx->sample_rate, AV_ROUND_UP);
	if (max_dst_nb_samples <= 0)
	{
		printf("av_rescale_rnd error \n");
		return -1;
	}

	dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
	ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels,
		dst_nb_samples, (AVSampleFormat)out_sample_fmt, 0);
	if (ret < 0)
	{
		printf("av_samples_alloc_array_and_samples error \n");
		return -1;
	}


	dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, audio_dec_ctx->sample_rate) +
		src_nb_samples, out_sample_rate, audio_dec_ctx->sample_rate, AV_ROUND_UP);
	if (dst_nb_samples <= 0)
	{
		printf("av_rescale_rnd error \n");
		return -1;
	}
	if (dst_nb_samples > max_dst_nb_samples)
	{
		av_free(dst_data[0]);
		ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels,
			dst_nb_samples, (AVSampleFormat)out_sample_fmt, 1);
		max_dst_nb_samples = dst_nb_samples;
	}

	if (swr_ctx)
	{
		ret = swr_convert(swr_ctx, dst_data, dst_nb_samples,
			(const uint8_t **)pAudioDecodeFrame->data, pAudioDecodeFrame->nb_samples);
		if (ret < 0)
		{
			printf("swr_convert error \n");
			return -1;
		}

		resampled_data_size = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
			ret, (AVSampleFormat)out_sample_fmt, 1);
		if (resampled_data_size < 0)
		{
			printf("av_samples_get_buffer_size error \n");
			return -1;
		}
	}
	else
	{
		printf("swr_ctx null error \n");
		return -1;
	}

	memcpy(out_buf, dst_data[0], resampled_data_size);

	if (dst_data)
	{
		av_freep(&dst_data[0]);
	}
	av_freep(&dst_data);
	dst_data = NULL;

	if (swr_ctx)
	{
		swr_free(&swr_ctx);
	}
	return resampled_data_size;
}

//����һ��ȫ�ֵĽṹ������Ա������Ǵ��ļ��еõ����������еط���
//��ͬʱҲ��֤SDL�е������ص�����audio_callback �ܴ�����ط��õ���������
typedef struct PacketQueue{
	AVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	SDL_mutex *mutex;//��ΪSDL ����һ���������߳�����������Ƶ����ġ��������û����ȷ������������У������� ���ܰ����ݸ��ҡ�
	SDL_cond *cond;
}PacketQueue;

PacketQueue audioq;

void packet_queue_init(PacketQueue *pq){
	memset(pq, 0, sizeof(PacketQueue));
	pq->mutex = SDL_CreateMutex();
	pq->cond = SDL_CreateCond();
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt){
	AVPacketList *pkt1;
	if (av_dup_packet(pkt) < 0){
		printf("error");
		return -1;
	}

	pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
	if (!pkt1){
		printf("error");
		return -1;
	}

	pkt1->pkt = *pkt;
	pkt1->next = NULL;

	//����SDL_LockMutex()�������еĻ������Ա����������������Ӷ�����Ȼ��
	//��SDL_CondSignal()ͨ�����ǵ���������Ϊһ���� �պ�����������ڵȴ�����
	//��һ���ź��������������Ѿ��������ˣ����žͻ�������������ö��п�������
	//���ʡ�
	SDL_LockMutex(q->mutex);

	if (!q->last_pkt)//����Ϊ��
		q->first_pkt = pkt1;
	else//���в�Ϊ��
		q->last_pkt->next = pkt1;
	q->last_pkt = pkt1;
	q->nb_packets++;
	q->size += pkt1->pkt.size;
	SDL_CondSignal(q->cond);

	SDL_UnlockMutex(q->mutex);

	return 0;
}

int quit = 0;
int decode_interrupt_cb(void){
	return quit;
}

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block){
	AVPacketList *pkt1;
	int ret;

	SDL_LockMutex(q->mutex);

	for (;;){
		if (quit){
			ret = -1;
			break;
		}

		pkt1 = q->first_pkt;
		if (pkt1){
			q->first_pkt = pkt1->next;
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;
			q->size -= pkt1->pkt.size;
			*pkt = pkt1->pkt;
			av_free(pkt1);
			ret = 1;
			break;
		}
		else if (!block){
			ret = 0;
			break;
		}
		else{
			SDL_CondWait(q->cond, q->mutex);
		}
	}
	SDL_UnlockMutex(q->mutex);
	return ret;
}

int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size){
	static AVPacket pkt;
	static uint8_t *audio_pkt_data = NULL;
	static int audio_pkt_size = 0;

	int len1, data_size, ret = 0;

	static AVFrame *pFrame;
	pFrame = av_frame_alloc();

	/*if (packet_queue_get(&audioq, &pkt, 1) < 0){//�����￪ʼ��ȡ��main�̷߳�����еİ�
		printf("error, can't get packet from the queue");
		return -1;
	}

	len1 = avcodec_decode_audio4(aCodecCtx, pFrame, &ret, &pkt);
	if (len1 < 0)
		return -1;

	return AudioResampling(aCodecCtx, pFrame, AV_SAMPLE_FMT_S16, 2, 44100, audio_buf);*/
	for (;;){
		while (audio_pkt_size > 0){
			data_size = buf_size;
			len1 = avcodec_decode_audio4(aCodecCtx, pFrame, &ret, &pkt);

			//len1 = avcodec_decode_audio3(aCodecCtx, (int16_t *)audio_buf,
			//	&data_size, &pkt);
			if (len1 < 0){//if error, skip frame
				printf("error\n");
				audio_pkt_size = 0;
				break;
			}
			data_size = AudioResampling(aCodecCtx, pFrame, AV_SAMPLE_FMT_S16, 2, 44100, audio_buf);
			audio_pkt_data += len1;
			audio_pkt_size -= len1;
			if (data_size <= 0)//No data yet, get more frames
				continue;
			return data_size;
		}
		if (pkt.data)
			av_free_packet(&pkt);
		if (quit)
			return -1;
		if (packet_queue_get(&audioq, &pkt, 1) < 0){//�����￪ʼ��ȡ��main�̷߳�����еİ�
			printf("error, can't get packet from the queue");
			return -1;
		}
		audio_pkt_data = pkt.data;
		audio_pkt_size = pkt.size;
	}
}
//�����ص�����
//userdata�����룬stream�������len�����룬len��ֵһ��Ϊ4096�������з��ֵģ���
//audio_callback�����Ĺ����ǵ���audio_decode_frame�������ѽ�������ݿ�audio_buf׷����stream�ĺ��棬
//ͨ��SDL���audio_callback�Ĳ��ϵ��ã����Ͻ������ݣ�Ȼ��ŵ�stream��ĩβ��
//SDL����Ϊstream�����ݹ�����һ֡��Ƶ�ˣ��Ͳ�����, 
//����������len����stream��д���ݵ��ڴ����߶ȣ��Ƿ����audio_callback����д�뻺���С��
void audio_callback(void *userdata, Uint8 *stream, int len){
	//SDL_memset(stream, 0, len);
	AVCodecContext *aCodecCtx = (AVCodecContext*)userdata;
	int len1, audio_size;

	//audio_buf �Ĵ�СΪ 1.5 ��������֡�Ĵ�	С�Ա�����һ���ȽϺõĻ���
	static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	static unsigned int audio_buf_size = 0;
	static unsigned int audio_buf_index = 0;
	
	while (len > 0){
		if (audio_buf_index >= audio_buf_size){//already send all our data, get more
			audio_size = audio_decode_frame(aCodecCtx, audio_buf, sizeof(audio_buf));
			if (audio_size < 0){//error, output silence
				printf("error, output silence\n");
				audio_buf_size = SDL_AUDIO_BUFFER_SIZE;
				memset(audio_buf, 0, audio_buf_size);
			}
			else
				audio_buf_size = audio_size;
			audio_buf_index = 0;
		}
		len1 = audio_buf_size - audio_buf_index;
		if (len1>len){
			len1 = len;
		}
		memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
		len -= len1;
		stream += len1;
		audio_buf_index += len1;
	}
}

int main(int argc, char *agrv[]){
	av_register_all();	//ע�������е��ļ���ʽ�ͱ����Ŀ⣬���ǽ����Զ���ʹ���ڱ��򿪵ĺ��ʸ�ʽ���ļ���
	AVFormatContext *pFormatCtx;
	pFormatCtx = avformat_alloc_context();

	char filepath[] = "F:/video/6s_kapian.mp4";
	//Open an input stream and read the header
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0){
		printf("Can't open the file\n");
		return -1;
	}
	//Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0){
		printf("Couldn't find stream information.\n");
		return -1;
	}

	//output file information
	cout << "�ļ���Ϣ----------------------------------" << endl;
	av_dump_format(pFormatCtx, 0, filepath, 0);
	cout << "--------------------------------------------" << endl;

	int i, videoIndex, audioIndex;

	//Find the first video stream
	videoIndex = -1;
	audioIndex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++){//����Ƶ���ĸ���
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
			&& videoIndex < 0){
			videoIndex = i;
		}
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
			&& audioIndex < 0)
			audioIndex = i;
	}
	
	if (videoIndex == -1)
		return -1;
	if (audioIndex == -1)
		return -1;
	
	AVCodecContext *pCodecCtx, *paCodecCtx;
	AVCodec *pCodec, *paCodec;
	//Get a pointer to the codec context for the video stream
	//���й��ڱ����������Ϣ���Ǳ����ǽ���"codec context"��������������ģ�
	//�Ķ����������������������ʹ�õĹ��ڱ��������������
	pCodecCtx = pFormatCtx->streams[videoIndex]->codec;
	paCodecCtx = pFormatCtx->streams[audioIndex]->codec;
	//Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	paCodec = avcodec_find_decoder(paCodecCtx->codec_id);
	
	if (pCodec == NULL || paCodecCtx == NULL){
		printf("Unsupported codec!\n");
		return -1;
	}
	//Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
		printf("Could not open video codec.\n");
		return -1;
	}
	if (avcodec_open2(paCodecCtx, paCodec, NULL) < 0){
		printf("Could not open audio codec.\n");
		return -1;
	}

	//--------------------------------------------------------//

	printf("������ %3d\n", pFormatCtx->bit_rate);
	printf("���������� %s\n", paCodecCtx->codec->long_name);
	printf("time_base  %d \n", paCodecCtx->time_base);
	printf("������  %d \n", paCodecCtx->channels);
	printf("sample per second  %d \n", paCodecCtx->sample_rate);
	//--------------------------------------------------------//

	//allocate video frame and set its fileds to default value
	AVFrame *pFrame, *pFrameYUV;
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	
	//��ʹ����������һ֡���ڴ棬��ת����ʱ��������Ȼ��Ҫһ���ط�������ԭʼ
	//�����ݡ�����ʹ��avpicture_get_size �����������Ҫ�Ĵ�С�� Ȼ���ֹ�����
	//�ڴ�ռ䣺
	uint8_t *out_buffer;
	int numBytes;
	numBytes = avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	//av_malloc ��ffmpeg ��malloc������ʵ��һ���򵥵�malloc �İ�װ����������
	//֤�ڴ��ַ�Ƕ���ģ�4 �ֽڶ������2 �ֽڶ��룩���������ܱ� ���㲻����
	//��й©���ظ��ͷŻ�������malloc �����������š�
	out_buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
	//Assign appropriate parts of buffer to image planes in pFrameYUV
	//Note that pFrameYUV is an AVFrame, but AVFrame is a superset of AVPicture
	avpicture_fill((AVPicture*)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

	//----------------SDL--------------------------------------//
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)){
		printf("Could not initialize SDL -%s\n", SDL_GetError());
		exit(1);
	}
	//������������ѡ������ʣ�����ͨ�����������Ĳ� ����Ȼ������
	//����һ���ص�������һЩ�û�����userdata������ʼ������Ƶ��ʱ��SDL ����
	//�ϵص�������ص���������Ҫ��������������������һ���ض����������ֽڡ�
	//�����ǰ���Щ��Ϣ�ŵ�SDL_AudioSpec �ṹ���к����ǵ��ú���
	//SDL_OpenAudio()�ͻ�������豸���Ҹ������� ������һ��AudioSpec �ṹ
	//�塣����ṹ��������ʵ�����õ��ģ�����Ϊ���ǲ��ܱ�֤�õ�������Ҫ��ġ�
	SDL_AudioSpec wanted_spec;
	wanted_spec.freq = paCodecCtx->sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = paCodecCtx->channels;	//������ͨ����
	wanted_spec.silence = 0;	//������ʾ������ֵ
	wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;	//�����������Ĵ�С
	wanted_spec.callback = audio_callback;
	wanted_spec.userdata = paCodecCtx;

	if (SDL_OpenAudio(&wanted_spec, NULL) < 0){
		printf("SDL_OpenAudio error: %s\n", SDL_GetError());
		return -1;
	}

	packet_queue_init(&audioq);
	SDL_PauseAudio(0);

	SDL_Window *window = nullptr;
	window = SDL_CreateWindow("MyPlayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		pCodecCtx->width, pCodecCtx->height, SDL_WINDOW_SHOWN);
	if (!window){
		cout << SDL_GetError() << endl;
		return 1;
	}

	SDL_Renderer *ren = nullptr;
	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (ren == nullptr){
		cout << SDL_GetError() << endl;
		return -1;
	}

	SDL_Texture *texture = nullptr;
	texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_YV12,
		SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);
	SDL_Rect rect;
	rect.x = 0, rect.y = 0;
	rect.w = pCodecCtx->width;
	rect.h = pCodecCtx->height;

	//*************************************************************//
	//ͨ����ȡ������ȡ������Ƶ����Ȼ����������֡�����ת����ʽ���ұ���
	int frameFinished;
	//int psize = pCodecCtx->width * pCodecCtx->height;
	AVPacket packet;
	av_new_packet(&packet, numBytes);

	i = 0;
	int ret;
	static struct SwsContext *img_convert_ctx;
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
		pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, PIX_FMT_YUV420P,
		SWS_BICUBIC, NULL, NULL, NULL);

	//Read the next frame of a stream
	while (av_read_frame(pFormatCtx, &packet) >= 0){
		//Is this a packet from the video stream?
		if (packet.stream_index == videoIndex){
			//decode video frame of size packet.size from packet.data into picture
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
			//Did we get a video frame?
			if (ret >= 0){
				//Convert the image from its native format to YUV
				if (frameFinished){
					sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data,
						pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

					SDL_UpdateYUVTexture(texture, &rect, pFrameYUV->data[0], pFrameYUV->linesize[0],
						pFrameYUV->data[1], pFrameYUV->linesize[1], pFrameYUV->data[2], pFrameYUV->linesize[2]);
					
					SDL_RenderClear(ren);
					SDL_RenderCopy(ren, texture, &rect, &rect);
					SDL_RenderPresent(ren);
				}
				SDL_Delay(50);
			}
			else{
				av_free_packet(&packet);
				cout << "decode error" << endl;
				return -1;
			}
		}
		else if (packet.stream_index == audioIndex){
			//packet_queue_put(&audioq, &packet);
			/*ret = avcodec_decode_audio4(paCodecCtx, pFrame, &frameFinished, &packet);
			cout << pFrame->format << endl;

			if (ret < 0){
				printf("Error in decoding audio frame\n");
				exit(0);
			}
			if (frameFinished){
				printf("pts %5d\n", packet.pts);
				printf("dts %5d\n", packet.dts);
				printf("packet_size %5d\n", packet.size);
			}
			audio_chunk = (Uint8*)pFrame->data[0];
			audio_len = pFrame->linesize[0];
			
			audio_pos = audio_chunk;
			//SDL_PauseAudio(0);
			while (audio_len>0)
				SDL_Delay(1);*/
			packet_queue_put(&audioq, &packet);
		}
	}

	SDL_Event event;
	while (true){
		SDL_PollEvent(&event);
		switch (event.type){
		case SDL_QUIT:
			SDL_Quit();
			exit(0);
			break;
		case SDL_KEYDOWN:
		default:
			break;
		}
	}

	SDL_DestroyTexture(texture);

	av_frame_free(&pFrame);
	av_frame_free(&pFrameYUV);

	avcodec_close(pCodecCtx);

	avformat_close_input(&pFormatCtx);

	return 0;
}