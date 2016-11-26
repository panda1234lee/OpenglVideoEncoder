#pragma once

#include "GlHelper.h"

class GlutHelper
{
public:
	GlutHelper();
	~GlutHelper();

	void setUp(int width, int height, int argc, char** argv);
	int setUpBoard(int width, int height, int argc, char **argv);
	void callBack();
	void loop();

private:
	static void processNormalKeys(unsigned char key, int x, int y);
	static void drawScene();
	static void replay();

private:
	static GlHelper* s_glh;
	static FFmpegEncode *s_encode;

	static int s_width;
	static int s_height;
};

