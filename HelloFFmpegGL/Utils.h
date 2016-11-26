#pragma once
#include <GL/glew.h>

class Utils
{
public:
	Utils();
	~Utils();

	static void *fileContents(const char *filename, GLint *length);
	static short leShort(unsigned char *bytes);
	static void *readTga(const char *filename, int *width, int *height);
};

