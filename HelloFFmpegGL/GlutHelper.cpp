#include "stdafx.h"
#include "GlutHelper.h"
#include <GL/freeglut.h>

#define IF_ENCODE 10

GlHelper* GlutHelper::s_glh = NULL;
int GlutHelper::s_width = 512;
int GlutHelper::s_height = 512;
FFmpegEncode* GlutHelper::s_encode = NULL;

GlutHelper::GlutHelper()
{
	s_glh = GlHelper::getInstance();
	s_encode = FFmpegEncode::getInstance("..//result.mp4", s_width, s_height);
}

GlutHelper::~GlutHelper()
{
	GlHelper::releaseInstance();
	FFmpegEncode::release();
}

void GlutHelper::setUp(int width, int height, int argc, char** argv)
{
	if (NULL == s_glh)
		return;

	setUpBoard(width, height, argc, argv);

	s_glh->setUp(width, height);
	// 一启动就录制视频
	//s_glh->startScreenCapture();

	s_encode->setup();
	s_encode->startEncodeVideo();

}

int GlutHelper::setUpBoard(int width, int height, int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(width, height);
	glutCreateWindow("HelloFFmpegGL! ");

	s_width = width;
	s_height = height;

	glewInit();
	if (!GLEW_VERSION_2_0)
	{
		fprintf(stderr, "OpenGL 2.0 not available\n");
		return -1;
	}

	return 1;
}

void GlutHelper::drawScene()
{
	if (NULL == s_glh)
		return;

	s_glh->drawScene();
	glutSwapBuffers();

#ifdef IF_ENCODE
	unsigned char* data = s_glh->screenCapture();
	// 编码一帧
	s_encode->processFrame(data);
	free(data);
#endif
}

void GlutHelper::processNormalKeys(unsigned char key, int x, int y)
{
	// Esc键退出录制
	if(key==27)
	{
#ifdef IF_ENCODE
		//s_glh->stopScreenCapture();
		s_encode->endEncodeVideo();
#endif
		exit(0);
	}
} 

void GlutHelper::replay()
{
	if (NULL == s_glh)
		return;

	s_glh->drawScene();
	glutSwapBuffers();

#ifdef IF_ENCODE
	unsigned char* data = s_glh->screenCapture();
	// 编码一帧
	s_encode->processFrame(data);
	free(data);
#endif
}

void GlutHelper::callBack()
{
	glutDisplayFunc(&drawScene);
	glutIdleFunc(&replay);
	glutKeyboardFunc(&processNormalKeys);
}

void GlutHelper::loop()
{
	glutMainLoop();
}
