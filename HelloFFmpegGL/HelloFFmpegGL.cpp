// HelloFFmpegGL.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "GlHelper.h"
#include "GlutHelper.h"

int main(int argc, char **argv)
{
	int width = 512, height = 512;

	GlutHelper gluth;

	gluth.setUp(width, height, argc, argv);
	gluth.callBack();
	gluth.loop();
	
    return 0;
}

