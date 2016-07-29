#pragma once
#pragma once
#include <stdio.h>
#include"SDLWrapper.h"
#define in_buffer_size  4096
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
};
class H264player {
private:
	AVCodec *pCodec;
	AVCodecContext *pCodecCtx;
	AVCodecParserContext *pCodecParserCtx;
	int frame_count;
	AVFrame	*pFrame, *pFrameYUV;
	uint8_t *out_buffer;
	//uint8_t in_buffer[in_buffer_size + FF_INPUT_BUFFER_PADDING_SIZE];
	uint8_t *cur_ptr;
	int cur_size;
	AVPacket packet;
	AVCodecID codec_id;
	int ret, got_picture;
	int y_size;
	int first_time;
	struct SwsContext *img_convert_ctx;
public:
	H264player();
	~H264player();
	int init();
	int transform_and_play(uint8_t* in_buffer, int cur_size, SDL2Wrapper* sdl2);
	int flush(SDL2Wrapper* sdl2);
};
H264player::H264player() {
	codec_id = AV_CODEC_ID_H264;
	first_time = 1;
	init();
}
int H264player::init() {
	avcodec_register_all();
	pCodec = avcodec_find_decoder(codec_id);
	if (!pCodec) {
		printf("Codec not found\n");
		return -1;
	}
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx) {
		printf("Could not allocate video codec context\n");
		return -1;
	}
	pCodecParserCtx = av_parser_init(codec_id);
	if (!pCodecParserCtx) {
		printf("Could not allocate video parser context\n");
		return -1;
	}
	if (pCodec->capabilities&CODEC_CAP_TRUNCATED)
		pCodecCtx->flags |= CODEC_FLAG_TRUNCATED; /* we do not send complete frames */
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Could not open codec\n");
		return -1;
	}
	pFrame = av_frame_alloc();
	av_init_packet(&packet);
	return 0;
}

int H264player::transform_and_play(uint8_t* in_buffer, int cur_size, SDL2Wrapper* sdl2) {
	if (cur_size == 0)
		return -1;
	cur_ptr = in_buffer;
	while (cur_size>0) {
		int len = av_parser_parse2(
			pCodecParserCtx, pCodecCtx,
			&packet.data, &packet.size,
			cur_ptr, cur_size,
			AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
		cur_ptr += len;
		cur_size -= len;
		if (packet.size == 0)
			continue;
		//Some Info from AVCodecParserContext
		printf("Packet Size:%6d\t", packet.size);
		switch (pCodecParserCtx->pict_type) {
		case AV_PICTURE_TYPE_I: printf("Type: I\t"); break;
		case AV_PICTURE_TYPE_P: printf("Type: P\t"); break;
		case AV_PICTURE_TYPE_B: printf("Type: B\t"); break;
		default: printf("Type: Other\t"); break;
		}
		printf("Output Number:%4d\t", pCodecParserCtx->output_picture_number);
		printf("Offset:%8ld\n", pCodecParserCtx->cur_offset);
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, &packet);
		if (ret < 0) {
			printf("Decode Error.(½âÂë´íÎó)\n");
			return ret;
		}
		if (got_picture) {
			if (first_time) {
				printf("\nCodec Full Name:%s\n", pCodecCtx->codec->long_name);
				printf("width:%d\nheight:%d\n\n", pCodecCtx->width, pCodecCtx->height);
				//SwsContext
				img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
					pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
				pFrameYUV = av_frame_alloc();
				out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
				avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
				y_size = pCodecCtx->width*pCodecCtx->height;
				first_time = 0;
			}
			printf("Succeed to decode 1 frame!\n");
			sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
				pFrameYUV->data, pFrameYUV->linesize);
			sdl2->PlayOneFrame(pFrameYUV);
		}
	}
	return 0;
}


int H264player::flush(SDL2Wrapper* sdl2) {
	packet.data = NULL;
	packet.size = 0;
	ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, &packet);
	if (ret < 0) {
		printf("Decode Error.(½âÂë´íÎó)\n");
		return ret;
	}
	if (!got_picture)
		return 0;
	if (got_picture) {
		printf("Flush Decoder: Succeed to decode 1 frame!\n");
		sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
			pFrameYUV->data, pFrameYUV->linesize);
		sdl2->PlayOneFrame(pFrameYUV);
	}
	return 1;
}

H264player::~H264player() {
	sws_freeContext(img_convert_ctx);
	av_parser_close(pCodecParserCtx);
	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	av_free(pCodecCtx);
}