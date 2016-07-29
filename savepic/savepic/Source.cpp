#pragma warning(disable:4996)
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
#include "recoder.h"
#include"H264player.h"
//Output YUV420P   
#define OUTPUT_YUV420P 0  
//'1' Use Dshow   
//'0' Use VFW  
#define USE_DSHOW 0  


//Refresh Event  
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)  

#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)  



int main(int argc, char* argv[])
{

	Recorder rec;
	if (rec.start_record() < 0) {
		printf("record start fail.\n");
		return -1;
	}
 

	//
	SDL2Wrapper sdl2(rec.get_width(), rec.get_height());

	//encode prepare
	YUVEncoder yuvEn(rec.get_width(), rec.get_height());
	H264player hplayer;
	if (!yuvEn.is_init()) {
		printf("yueencode failed.\n");
		return -1;
	}

	uint8_t* dstbuf = new uint8_t[MAX_BUFSIZE];
	//



#if OUTPUT_YUV420P   
	FILE *fp_yuv = fopen("output.yuv", "wb+");
#endif    
	FILE *fp_h264 = fopen("ds.264", "wb+");
	int bsize = 0;
	for (int i=0;i<300;i++) {
			
		/*------------------------------ 编码------------------------------ */ 
		bsize = rec.output_h264package(&yuvEn, dstbuf);
		/*------------------------------ 解码------------------------------ */
		hplayer.transform_and_play(dstbuf,bsize,&sdl2);
		/*----------------------------------------------------------------- */
		//fwrite(dstbuf, 1, bsize, fp_h264);
	}
	bsize = 0;
	do {
		if ((bsize = yuvEn.Flush_Encoder(dstbuf)) < 0) {
			printf("Encode Error.\n");
			return -1;
		}
		//fwrite(dstbuf, 1, bsize, fp_h264);

		hplayer.transform_and_play(dstbuf, bsize, &sdl2);
	} while (bsize > 0);
	
	while (hplayer.flush(&sdl2) > 0);
#if OUTPUT_YUV420P   
	fclose(fp_yuv);
#endif   
	fclose(fp_h264);

	//av_free(out_buffer);  

	return 0;
}