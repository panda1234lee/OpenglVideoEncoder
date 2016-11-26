#pragma once

extern "C"
{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class FFmpegEncode
{
public:

	static FFmpegEncode* getInstance(const char* file = "../output.mp4", int w = 512, int h = 512)
	{
		if(NULL == s_instance)
		{
			return new FFmpegEncode(file, w, h);
		}
		else
			return s_instance;
	}

	static void release()
	{
		if (s_instance)
		{
			delete s_instance;
		}
		s_instance = NULL;
	}

	// 重载赋值操作符
	//FFmpegEncode& operator=(FFmpegEncode& encode)
	//{
	//	return encode;
	//}

	void setup();
    int processFrame(const unsigned char *raw_data);
    int startEncodeVideo();
    void endEncodeVideo();
	// ---
    double getVideoDuration();
    double getVideoPlayingTime();

private:
	// 禁止其他构造方法
	FFmpegEncode(const char *out_file, int w , int h);
	//FFmpegEncode(const FFmpegEncode&){};	// 拷贝构造函数

	~FFmpegEncode(void);
	
    int init();
    int prepareCodec();
	void prepareSwsContext();
	int makeFrame();
    void cvtRawData2Frame(const unsigned char *raw_data, AVFrame* av_f);
    int flushEncoder(AVFormatContext *fmt_ctx, unsigned int stream_index);

    AVOutputFormat *m_out_fmt;
    AVStream *m_out_stream;
    AVFrame *m_out_frm;

    uint8_t *m_picture_buf;

    // ---
    AVFormatContext	*m_fmt_ctx;
    AVCodecContext	*m_codec_ctx;
    SwsContext	*m_sws_ctx;
	AVDictionary *m_param;
	// ---
    int m_videoindex;
    const char *m_filepath;
    int m_width, m_height;
	clock_t m_start, m_end;

	static FFmpegEncode* s_instance;
};

