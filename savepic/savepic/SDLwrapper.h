#pragma once
#include<libavcodec\avcodec.h>
#include<SDL.h>
//YUV only
class SDL2Wrapper {
public:
	SDL2Wrapper(int w, int h);
	~SDL2Wrapper() { SDL_Quit(); }
	void PlayOneFrame(AVFrame* pFrameYUV);
	bool checkstate()const { return init_success; };
private:
	bool init_success;
	int screen_w;
	int screen_h;
	SDL_Renderer* sdlRenderer;
	SDL_Window *screen;
	SDL_Texture* sdlTexture;
	SDL_Rect sdlRect;
};

SDL2Wrapper::SDL2Wrapper(int w, int h) :screen_w(w), screen_h(h),init_success(true) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		init_success = false;
		return;
	}
	screen = SDL_CreateWindow("Simplest Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h, SDL_WINDOW_OPENGL);
	if (!screen) {
		printf("SDL: could not create window - exiting:%s\n", SDL_GetError());\
		init_success = false;
		return;
	}
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);


	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = screen_w;
	sdlRect.h = screen_h;

};


void SDL2Wrapper::PlayOneFrame(AVFrame* pFrameYUV) {
	SDL_UpdateYUVTexture(sdlTexture, &sdlRect,
		pFrameYUV->data[0], pFrameYUV->linesize[0],
		pFrameYUV->data[1], pFrameYUV->linesize[1],
		pFrameYUV->data[2], pFrameYUV->linesize[2]);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
	SDL_RenderPresent(sdlRenderer);
	//Delay 40ms  
	SDL_Delay(40);
}
