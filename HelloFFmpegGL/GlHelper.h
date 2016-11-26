#pragma once
#include <GL/glew.h>
#include <opencv2/opencv.hpp>
#include "Resources.h"
#include "FFmpegEncode.h"

class GlHelper
{
public:
	
	static GlHelper* getInstance()
	{
		if (NULL == s_instance)
			return new GlHelper();
		else
			return s_instance;
	}

	static void releaseInstance()
	{
		if (NULL == s_instance)
			return;

		delete s_instance;
		s_instance = NULL;
	}
	
	void setUp(int width, int height)
	{
		setSize(width, height);

		initShaders();

		loadTexture("../images/beard.jpg", 0, "from", GL_TEXTURE0);
		loadTexture("../images/whale.png", 1, "to", GL_TEXTURE1);

		setupBuffers();
	}

	void drawScene(void);
	unsigned char* screenCapture();

private:
	GlHelper();
	~GlHelper();

	int initShaders(void);
	void loadTexture(const char* file_path, int index, const char* tex_name, const int tex_unit);

	void setupBuffers(void);
	void setSize(int width, int height);

	GLuint makeBuffer(
		GLenum target,
		const void *buffer_data,
		GLsizei buffer_size);
	GLuint makeTexture(const char *filename);
	GLuint makeShader(GLenum type, const char *filename);
	GLuint makeProgram(GLuint vertex_shader, GLuint fragment_shader);

	void ratioResize(const cv::Mat& src, cv::Mat& dst, int dst_w, int dst_h, float t=1.);
	void activeTexture(GLuint tex_unit, GLuint tex_id, GLuint tex_ul, GLuint value);
	
public:
	GLuint m_vertex_shader, m_fragment_shader, m_program;
	Resources m_res;

private:
	static GlHelper* s_instance;

	int m_width;
	int m_height;
};

