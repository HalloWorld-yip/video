#pragma once
#include <stdio.h>  

#define __STDC_CONSTANT_MACROS  

//  todo
const int MAX_BUFSIZE = 4096<<4;

extern "C"
{
#include "libavutil/opt.h"  
#include "libavcodec/avcodec.h"  
#include "libavutil/imgutils.h"  
};



class YUVEncoder {
public:
	YUVEncoder(int w,int h);
	//return size of dstbuf if success,or -1;
	int Encode1Frm(AVFrame* psrc, uint8_t* dstbuf);
	//return size of dstbuf if success,or -1;
	int Flush_Encoder(uint8_t* dstbuf);
	bool is_init()const { return init_success; }
	~YUVEncoder();
private:
	AVCodec *pCodec;
	AVCodecContext *pCodecCtx;
	int framecnt;
	FILE *fp_in;
	FILE *fp_out;
	AVPacket pkt;
	int y_size;
	int in_w, in_h;
	bool init_success;
};

YUVEncoder::YUVEncoder(int w,int h):in_w(w),in_h(h),framecnt(0),init_success(true){
	avcodec_register_all();

	pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!pCodec) {
		printf("Codec not found\n");
		init_success = false;
		return ;
	}
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx) {
		printf("Could not allocate video codec context\n");
		init_success = false;
		return ;
	}
	pCodecCtx->bit_rate = 400000;
	pCodecCtx->width = in_w;
	pCodecCtx->height = in_h;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;
	pCodecCtx->gop_size = 10;
	pCodecCtx->max_b_frames = 1;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	
	av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		init_success = false;
		printf("Could not open codec\n");
		return ;
	}

	//pFrame = av_frame_alloc();
	//if (!pFrame) {
	//	printf("Could not allocate video frame\n");
	//	init_success = false;
	//	return ;
	//}
	//pFrame->format = pCodecCtx->pix_fmt;
	//pFrame->width = pCodecCtx->width;
	//pFrame->height = pCodecCtx->height;

	//if (av_image_alloc(pFrame->data, pFrame->linesize, pCodecCtx->width, pCodecCtx->height,
	//	pCodecCtx->pix_fmt, 16) < 0) {
	//	printf("Could not allocate raw picture buffer\n");
	//	init_success = false;
	//	return ;
	//}

	y_size = pCodecCtx->width * pCodecCtx->height;

}

int YUVEncoder::Encode1Frm(AVFrame* psrc,uint8_t* dstbuf) {
	int got_output,ret,tmp=0;

	av_init_packet(&pkt);
	pkt.data = NULL;    // packet data will be allocated by the encoder  
	pkt.size = 0;
	/*----------------------------------------------------------------------------------*/
	//Read raw YUV data  

	/*----------------------------------------------------------------------------------*/
	/* encode the image */
	ret = avcodec_encode_video2(pCodecCtx, &pkt, psrc, &got_output);
	if (ret < 0) {
		printf("Error encoding frame\n");
		return -1;
	}
	if (got_output) {
		printf("Succeed to encode frame: %5d\tsize:%5d\n", framecnt, pkt.size);
		framecnt++;
		/*----------------------------------------------------------------------------------*/
		//fwrite(pdst->data, 1, pdst->size, fp_out);
		if (pkt.size > MAX_BUFSIZE) {
			printf("buf.\n");
		}
		memcpy_s(dstbuf, MAX_BUFSIZE, pkt.data, pkt.size);
		/*----------------------------------------------------------------------------------*/
		tmp = pkt.size;
		av_free_packet(&pkt);
	}
	return tmp;
}
int YUVEncoder::Flush_Encoder(uint8_t* dstbuf) {
	int got_output, ret, tmp=0;
	ret = avcodec_encode_video2(pCodecCtx, &pkt, NULL, &got_output);
	if (ret < 0) {
		printf("Error encoding frame\n");
		return -1;
	}
	if (got_output) {
		printf("Succeed to encode frame: %5d\tsize:%5d\n", framecnt, pkt.size);
		framecnt++;
		/*----------------------------------------------------------------------------------*/
		//fwrite(pdst->data, 1, pdst->size, fp_out);
		if (pkt.size > MAX_BUFSIZE) {
			printf("buf.\n");
		}
		memcpy_s(dstbuf, MAX_BUFSIZE, pkt.data, pkt.size);
		/*----------------------------------------------------------------------------------*/
		tmp = pkt.size;
		av_free_packet(&pkt);
	}
	return tmp;
}


YUVEncoder::~YUVEncoder() {
	avcodec_close(pCodecCtx);
	av_free(pCodecCtx);
	//av_freep(&pFrame->data[0]);
	//av_frame_free(&pFrame);

}