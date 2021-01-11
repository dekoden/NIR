#ifndef MAIN_H
#define MAIN_H

struct Color32
{
	uchar r;
	uchar g;
	uchar b;
	uchar a;
};

extern "C"
{
	__declspec(dllexport) int getImages(Color32** raw, int width, int height, int numOfImg, int numOfCam); // , bool isShow);
	__declspec(dllexport) void processImage(unsigned char* data, int width, int height);
}

#endif
