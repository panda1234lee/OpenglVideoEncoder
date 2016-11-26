#include "stdafx.h"
#include "GlHelper.h"
#include "Utils.h"
#include <GL/freeglut.h>
#include <time.h>

#define RGBA 1001

GlHelper* GlHelper::s_instance = NULL;

GlHelper::GlHelper():m_width(512), m_height(512)
{
}

GlHelper::~GlHelper()
{
	glDeleteTextures(1, &(m_res.texture_id[0]));
	glDeleteTextures(1, &(m_res.texture_id[1]));

	// Delete program object
	glDeleteProgram(m_program);
}

GLuint GlHelper::makeBuffer(
    GLenum target,
    const void *buffer_data,
    GLsizei buffer_size
)
{
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(target, buffer);
    glBufferData(target, buffer_size, buffer_data, GL_STATIC_DRAW);
    return buffer;
}

void GlHelper::ratioResize(const cv::Mat& src, cv::Mat& dst, int dst_w, int dst_h, float t)
{
	int w = src.cols;
	int h = src.rows;
	float m = float(w) / h;

	//float t = 1.;											// 必须大于等于1，缩放系数
	if (t < 1.)
	{
		std::cout << "Bad t value !" << std::endl;
		return;
	}

	float mm = float(dst_w) / dst_h;

	dst = cv::Mat(cv::Size(dst_w, dst_h), CV_8UC3, cv::Scalar(0));

	// 如果原图小于指定尺寸
	if (dst_w > w && dst_h > h)
	{
		//std::cout << " dst_w > w and dst_h > h !" << std::endl;
		// 直接扩展黑边即可
		int delta_x = .5 * (dst_w - w);
		int delta_y = .5 * (dst_h - h);
		// 防止 dst_w - w、dst_h - h 为奇数
		copyMakeBorder(src, dst, dst_h - h - delta_y, delta_y, dst_w - w - delta_x, delta_x,
			cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

		//imshow("Result", extend);
		//cv::waitKey(0);

		return;
	}

	float len = t * std::max(w, h);					// 缩放前的最大边
	float len_ = std::min(dst_w, dst_h);		// 缩放后的最小边

	if (m > 1.f && mm > 1.f)						// 缩放前和缩放后皆为横图
	{
		float n = float(dst_w) / w;					// 确保缩放后的大小能够容纳所有像素
		if (dst_h >= h * n)
		{
			len_ = std::max(dst_w, dst_h);		// 缩放后的最大边
		}
	}
	else 	if (m < 1.f && mm < 1.f)				// 缩放前和缩放后皆为竖图
	{
		float n = float(dst_w) / w;					// 确保缩放后的大小能够容纳所有像素
		if (dst_h >= h * n)
		{
			len = t * std::min(w, h);					// 缩放前的最小边
		}
	}

	float ratio = len_ / len;							// 得到缩放比例
														//std::cout << "Scale ratio: " << ratio << std::endl;

	int w_ = ratio * w;									// 缩放后的实际宽高
	int h_ = ratio * h;
	int delta_x = (dst_w - w_) / (2 * ratio);	// 计算缩放后的尺寸与目标尺寸的差值，用该值的一半换算为缩放前的值
	int delta_y = (dst_h - h_) / (2 * ratio);	// 该值用于平移图像至中心

	for (int i = 0; i < w; i++)
	{
		for (int j = 0; j < h; j++)
		{
			float u = (i + delta_x) / len;
			float v = (j + delta_y) / len;

			int x = u * len_;
			int y = v * len_;

			dst.at<cv::Vec3b>(y, x)[0] = src.at<cv::Vec3b>(j, i)[0];
			dst.at<cv::Vec3b>(y, x)[1] = src.at<cv::Vec3b>(j, i)[1];
			dst.at<cv::Vec3b>(y, x)[2] = src.at<cv::Vec3b>(j, i)[2];
		}
	}

	//imshow("Result", dst);
	//cv::waitKey(0);
}

GLuint GlHelper::makeTexture(const char *filename)
{
	int width, height;
	//unsigned char *pixels = read_jpeg(filename, &width, &height);
	cv::Mat src = cv::imread(filename, 1);
	//unsigned char* pixels = src.data;
	//width = src.cols;
	//height = src.rows;

	cv::Mat dst;

	// TODO: 等比例缩放
	width = m_width;
	height = m_height;
	ratioResize(src, dst, width, height);
	unsigned char* pixels = dst.data;

	GLuint texture;

	if (!pixels)
		return 0;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(
		GL_TEXTURE_2D, 0,							/* target, level */
		GL_RGB,											/* internal format */
		width, height, 0,								/* width, height, border */
		GL_BGR, GL_UNSIGNED_BYTE,			/* external format, type */
		pixels												/* pixels */
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//free(pixels); // for read_jpeg
	return texture;
}

GLuint GlHelper::makeShader(GLenum type, const char *filename)
{
	GLint length;
	GLchar *source = (GLchar *)Utils::fileContents(filename, &length);
	GLuint shader;
	GLint shader_ok;

	if (!source)
		return 0;

	shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar **)&source, &length);
	free(source);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
	if (!shader_ok)
	{
		fprintf(stderr, "Failed to compile %s:\n", filename);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

GLuint GlHelper::makeProgram(GLuint vertex_shader, GLuint fragment_shader)
{
	GLint program_ok;

	GLuint program = glCreateProgram();

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
	if (!program_ok)
	{
		fprintf(stderr, "Failed to link shader program:\n");
		glDeleteProgram(program);
		return 0;
	}
	return program;
}

int GlHelper::initShaders(void)
{
	m_vertex_shader = makeShader(GL_VERTEX_SHADER, "../shaders/noop.v.glsl");
	if (m_vertex_shader == 0)
		return -1;

	m_fragment_shader = makeShader(GL_FRAGMENT_SHADER, "../shaders/noop.f.glsl");
	if (m_fragment_shader == 0)
		return -1;

	m_program = makeProgram(m_vertex_shader, m_fragment_shader);
	if (m_program == 0)
		return -1;

	glUseProgram(m_program);

	return 1;
}

void GlHelper::loadTexture(const char* file_path, int index, const char* tex_name, const int tex_unit)
{
	m_res.texture_id[index] = makeTexture(file_path);
	m_res.unif_tex_loc[index] = glGetUniformLocation(m_program, tex_name);
}

void GlHelper::activeTexture(GLuint tex_unit, GLuint tex_id, GLuint tex_ul, GLuint value)
{
	glActiveTexture(tex_unit);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glUniform1i(tex_ul, value);	 // ☆
}

void GlHelper::drawScene(void)
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glViewport(0, 0, m_width, m_height);

	int numOfVertices = 6;

	// 仅需要初始化一次
	if (m_res.attr_pos == -1)
	{
		m_res.attr_pos
			= glGetAttribLocation(m_program, "position");

		glEnableVertexAttribArray(m_res.attr_pos);

		glBindBuffer(GL_ARRAY_BUFFER, m_res.vertex_buffer);

		glVertexAttribPointer(
			m_res.attr_pos,								 /* attribute */
			3,													 /* size */
			GL_FLOAT,									 /* type */
			GL_FALSE,									 /* normalized? */
			0,//sizeof(GLfloat)*3,					 /* stride */
			(void *)0										 /* array buffer offset */
		);
	}

	if (m_res.unif_time_loc == -1)
	{
		m_res.unif_time_loc = glGetUniformLocation(m_program, "iGlobalTime");
	}

#if 1
	// 用时钟有时会受到卡顿的影响！
	double time = clock() * .5 / CLOCKS_PER_SEC;
	glUniform1f(m_res.unif_time_loc, time);
#else
	static double time = 0;
	const int STEP = 100;	// 经验值
	glUniform1f(m_res.unif_time_loc, time / STEP);
	time ++;
#endif

	activeTexture(GL_TEXTURE0, m_res.texture_id[0], m_res.unif_tex_loc[0], 0);
	activeTexture(GL_TEXTURE1, m_res.texture_id[1], m_res.unif_tex_loc[1], 1);

	glDrawArrays(GL_TRIANGLES, 0, numOfVertices);

	glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned char* GlHelper::screenCapture()
{
	// 设置像素的对齐格式——Word对齐(4字节)
	glPixelStorei(GL_PACK_ALIGNMENT, 4);

#ifdef RGBA
	// 计算对齐后的真实宽度
	int nAlignWidth = (m_width * 32 + 31) / 32;
	// 分配缓冲区，calloc 比 malloc 多一个初始化为 0 的过程
	unsigned char *raw_data = (unsigned char *)calloc(nAlignWidth * m_height * 4, sizeof(char));

	//glReadBuffer(GL_FRONT);
	//glBindBuffer(GL_PIXEL_PACK_BUFFER, yourbuffer);
	clock_t start = clock();
	glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, raw_data);
	clock_t end = clock();
	double time = 1000. * (end - start) / CLOCKS_PER_SEC;
	std::cout << "glReadlPixels = " << time  << " ms" << std::endl;

#else
	// 计算对齐后的真实宽度
	int nAlignWidth = (m_width * 24 + 31) / 32;
	// 分配缓冲区，calloc 比 malloc 多一个初始化为 0 的过程
	unsigned char *raw_data = (unsigned char *)calloc(nAlignWidth * m_height * 4, sizeof(char));
	glReadPixels(0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, raw_data);

	//cv::Mat src = cv::Mat(m_height, m_width, CV_8UC3, raw_data);
	//cv::Mat yuv;
	//cvtColor(src, yuv, CV_RGB2YUV);

	//flip(bgr, dst, 0);	// 0:x; 1:y, -u:x,y;
	//imwrite("../images/test.bmp", dst);
	//imshow("test", dst);
	//cv::waitKey(0);
#endif

	return raw_data;
}

void GlHelper::setupBuffers(void)
{
	GLfloat vertex_buffer_data[] =
	{
		-1.f, -1.f, 0.f,
		1.f, -1.f, 0.f,
		1.f, 1.f, 0.f,

		1.f, 1.f, 0.f,
		-1.f, 1.f, 0.f,
		-1.f, -1.f, 0.f
	};

	m_res.vertex_buffer = makeBuffer(
		GL_ARRAY_BUFFER,
		vertex_buffer_data,
		sizeof(vertex_buffer_data)
		);
}

void GlHelper::setSize(int width, int height)
{
	m_width = width;
	m_height = height;
}
