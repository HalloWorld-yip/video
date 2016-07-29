#pragma once
#include <stdio.h>  

#define __STDC_CONSTANT_MACROS  

//Windows  
extern "C"
{
#include "libavcodec/avcodec.h"  
#include "libavformat/avformat.h"  
#include "libswscale/swscale.h"  
#include "libavutil/imgutils.h"  
#include "libavdevice/avdevice.h"  
#include "SDL.h"  
};
#include"pureencoder.h"

void show_dshow_device_option() {
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_options", "true", 0);
	AVInputFormat *iformat = av_find_input_format("dshow");
	printf("========Device Option Info======\n");
	avformat_open_input(&pFormatCtx, "video=Integrated Camera", iformat, &options);
	printf("================================\n");
}
class Recorder {
private:
	AVFormatContext *pFormatCtx;
	int             videoindex,framepts;
	AVCodecContext  *pCodecCtx;
	AVCodec         *pCodec;
	AVInputFormat	*ifmt;
	AVFrame *pFrame, *pFrameYUV;
	AVPacket *packet;
	struct SwsContext *img_convert_ctx;
	void prepare_struct();
public:
	Recorder();
	~Recorder();
	int start_record();
	int get_width()const { return pCodecCtx->width; }
	int get_height()const { return pCodecCtx->height; }
	bool is_samestream()const { return (packet)?packet->stream_index == videoindex:0; }
	int output_h264package(YUVEncoder* yuvEn, uint8_t* dstbuf);
};

Recorder::Recorder() :framepts(0){
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	//Register Device  
	avdevice_register_all();
	show_dshow_device_option();
	ifmt = av_find_input_format("vfwcap");

}
Recorder::~Recorder() {
	sws_freeContext(img_convert_ctx);
	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
}
int Recorder::start_record() {
	//摄像头输入
	if (avformat_open_input(&pFormatCtx, "0", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
	//获取信息
	if (avformat_find_stream_info(pFormatCtx, NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (int i = 0; i<pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	if (videoindex == -1)
	{
		printf("Couldn't find a video stream.\n");
		return -1;
	}
	//初始化解码器
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL)<0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	//需要打开流获取视频信息才能初始化frame等结构。
	prepare_struct();

	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	return 0;
}

void Recorder::prepare_struct() {
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	uint8_t* out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
		AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);//初始化frame跟out_buffer内容无关
	packet = (AVPacket *)av_malloc(sizeof(AVPacket));	
	//set width and heigh
	pFrameYUV->width = pCodecCtx->width;
	pFrameYUV->height = pCodecCtx->height;
	pFrameYUV->format = pCodecCtx->pix_fmt;
}


int Recorder::output_h264package(YUVEncoder* yuvEn,uint8_t* dstbuf) {
	int got_picture;
	int bsize=0;
	if (av_read_frame(pFormatCtx, packet) >= 0) {
		if (packet->stream_index == videoindex) {
			if (avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet) < 0) {
				printf("Decode Error.\n");
				return -1;
			}
			if (got_picture) {

				sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);


				pFrameYUV->pts = framepts++;

				if ((bsize = yuvEn->Encode1Frm(pFrameYUV, dstbuf)) < 0) {
					printf("Encode Error.\n");
					return -1;
				}


			}
		}
		av_free_packet(packet);
	}
	return bsize;
}
