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
#include "SDLwrapper.h"
#include "pureencoder.h"

//Output YUV420P   
#define OUTPUT_YUV420P 0  
//'1' Use Dshow   
//'0' Use VFW  
#define USE_DSHOW 0  


//Refresh Event  
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)  

#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)  



int man(int argc, char* argv[])
{

	AVFormatContext *pFormatCtx;
	int             i, videoindex;
	AVCodecContext  *pCodecCtx;
	AVCodec         *pCodec;

	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	//Open File  
	//char filepath[]="src01_480x272_22.h265";  
	//avformat_open_input(&pFormatCtx,filepath,NULL,NULL)  

	//Register Device  
	avdevice_register_all();

	//Windows  
#ifdef _WIN32  

	//Show Device Options  
	//show_dshow_device_option();
	//Show VFW Options  
	//show_vfw_device();

#if USE_DSHOW  
	AVInputFormat *ifmt = av_find_input_format("dshow");
	//Set own video device's name  
	if (avformat_open_input(&pFormatCtx, "video=Integrated Camera", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#else  
	AVInputFormat *ifmt = av_find_input_format("vfwcap");
	if (avformat_open_input(&pFormatCtx, "0", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#endif  
#elif defined linux  
	//Linux  
	AVInputFormat *ifmt = av_find_input_format("video4linux2");
	if (avformat_open_input(&pFormatCtx, "/dev/video0", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#else  
	show_avfoundation_device();
	//Mac  
	AVInputFormat *ifmt = av_find_input_format("avfoundation");
	//Avfoundation  
	//[video]:[audio]  
	if (avformat_open_input(&pFormatCtx, "0", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#endif  


	if (avformat_find_stream_info(pFormatCtx, NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i<pFormatCtx->nb_streams; i++)
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
	AVFrame *pFrame, *pFrameYUV;
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	uint8_t* out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
		AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);//初始化frame跟out_buffer内容无关

																	//
	SDL2Wrapper sdl2(pCodecCtx->width, pCodecCtx->height);

	//encode prepare
	YUVEncoder yuvEn(pCodecCtx->width, pCodecCtx->height);
	if (!yuvEn.is_init()) {
		printf("yueencode failed.\n");
		return -1;
	}

	uint8_t* dstbuf = new uint8_t[MAX_BUFSIZE];
	//

	int ret, got_picture;

	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));

#if OUTPUT_YUV420P   
	FILE *fp_yuv = fopen("output.yuv", "wb+");
#endif    
	FILE *fp_h264 = fopen("ds.264", "wb+");
	struct SwsContext *img_convert_ctx;
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	//set width and heigh
	pFrameYUV->width = pCodecCtx->width;
	pFrameYUV->height = pCodecCtx->height;
	pFrameYUV->format = pCodecCtx->pix_fmt;

	for (int i = 0; i<300; i++) {
		//------------------------------  
		if (av_read_frame(pFormatCtx, packet) >= 0) {
			if (packet->stream_index == videoindex) {
				if (avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet) < 0) {
					printf("Decode Error.\n");
					return -1;
				}
				if (got_picture) {

					sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

#if OUTPUT_YUV420P    
					int y_size = pCodecCtx->width*pCodecCtx->height;
					fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y     
					fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U    
					fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V    
#endif    
					pFrameYUV->pts = i;

					int bsize;
					if ((bsize = yuvEn.Encode1Frm(pFrameYUV, dstbuf)) < 0) {
						printf("Encode Error.\n");
						return -1;
					}
					/*----------------------------------------------------------------------------------*/
					fwrite(dstbuf, 1, bsize, fp_h264);
					/*----------------------------------------------------------------------------------*/
					sdl2.PlayOneFrame(pFrameYUV);


				}
			}
			av_free_packet(packet);
		}

	}
	int bsize = 0;
	do {
		if ((bsize = yuvEn.Flush_Encoder(dstbuf)) < 0) {
			printf("Encode Error.\n");
			return -1;
		}
		fwrite(dstbuf, 1, bsize, fp_h264);
	} while (bsize > 0);
	sws_freeContext(img_convert_ctx);

#if OUTPUT_YUV420P   
	fclose(fp_yuv);
#endif   
	fclose(fp_h264);
	SDL_Quit();

	//av_free(out_buffer);  
	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}