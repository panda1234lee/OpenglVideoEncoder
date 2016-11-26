
#include "stdafx.h"
#include "FFmpegEncode.h"

// 音频的重复播放，先用SDL测试

#define TAG "FFmpegEncode"

//#define ERROR_LOG 2048
//#define DEBUG_LOG 1024
#define VIDEO_OUTPUT_BUF_SIZE 200000
#define TRACE_FUNC() LOGE("(%d)-<%s>\n", __LINE__, __FUNCTION__)
#define IN_VID_FMT AV_PIX_FMT_RGBA
#define OUT_VID_FMT AV_PIX_FMT_YUVJ420P
#define DST_W 512
#define DST_H 512

FFmpegEncode *FFmpegEncode::s_instance = NULL;

FFmpegEncode::FFmpegEncode(const char *out_file, int w, int h)
{
    m_out_fmt = NULL;
    m_out_stream = NULL;
    m_out_frm = NULL;
    m_sws_ctx = NULL;
    m_param = NULL;
    m_picture_buf = NULL;
    m_videoindex = 0;

    m_filepath = out_file;
    m_width = w;
    m_height = h;
}

FFmpegEncode::~FFmpegEncode(void)
{
    //clear
    if (m_picture_buf)
    {
        av_free(m_picture_buf);
        m_picture_buf = NULL;
    }

    av_frame_unref(m_out_frm);

    if(m_sws_ctx)
    {
        sws_freeContext(m_sws_ctx);
        m_sws_ctx = NULL;
    }
}

void FFmpegEncode::setup()
{
    av_register_all();	// ☆

    init();
    prepareCodec();
	prepareSwsContext();
    makeFrame();
}

int FFmpegEncode::init()
{
    // 根据输出文件名猜测格式
    m_out_fmt = av_guess_format(NULL, m_filepath, NULL);
    int res = avformat_alloc_output_context2(&m_fmt_ctx, NULL, NULL,
              m_filepath);

    if (res != 0)
    {
        return -1;
    }

    // 设置输出格式和文件名
    m_fmt_ctx->oformat = m_out_fmt;
    _snprintf(m_fmt_ctx->filename, sizeof(m_fmt_ctx->filename), "%s",
              m_filepath);

    return 0;
}

int FFmpegEncode::prepareCodec()
{
#ifdef DEBUG_LOG
    TRACE_FUNC();
#endif

    if (m_out_fmt->video_codec != CODEC_ID_NONE)
    {
        // 创建输出流
        m_out_stream = avformat_new_stream(m_fmt_ctx, 0);
    }

    const int frame_rate = 60;

    // 指向流的编解码器上下文指针
    m_codec_ctx = m_out_stream->codec;
    // 设置相应的参数
    m_codec_ctx->codec_id = CODEC_ID_H264;			// CODEC_ID_MPEG4, CODEC_ID_H264;
    m_codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    m_codec_ctx->bit_rate = 8000000;							// 400 Kbit/s
    m_codec_ctx->width = m_width;
    m_codec_ctx->height = m_height;
    m_codec_ctx->time_base.num = 1;
    m_codec_ctx->time_base.den = frame_rate;			    // 编码器的时间基
    m_codec_ctx->gop_size = 25;									// I帧间隔
    m_codec_ctx->pix_fmt = OUT_VID_FMT;
	
    if(m_codec_ctx->codec_id == CODEC_ID_H264)
    {
        m_codec_ctx->me_range = 16;
        m_codec_ctx->max_qdiff = 10;
        m_codec_ctx->qcompress = 0.75;
        m_codec_ctx->qmin = 10;
        m_codec_ctx->qmax = 51;

        av_dict_set(&m_param, "preset", "ultrafast", 0);
        av_dict_set(&m_param, "tune", "zerolatency", 0);
        //av_dict_set(&m_param, "profile", "main", 0);
    }

#if 0
    if (m_codec_ctx->codec_id == CODEC_ID_MPEG2VIDEO)
    {
        m_codec_ctx->max_b_frames = 2;              // 最大b帧数
    }

    if (m_codec_ctx->codec_id == CODEC_ID_MPEG1VIDEO)
    {
        m_codec_ctx->mb_decision = 2;				// 模式判断模式
    }
#endif

    if (!strcmp(m_fmt_ctx->oformat->name, "mp4")
            || !strcmp(m_fmt_ctx->oformat->name, "mov")
            || !strcmp(m_fmt_ctx->oformat->name, "3gp"))
    {
        m_codec_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    AVCodec *codec = NULL;
    codec = avcodec_find_encoder(m_codec_ctx->codec_id);
    if (!codec)
    {
        return -1;
    }

	if(codec->capabilities & CODEC_CAP_TRUNCATED)
		m_codec_ctx->flags |= CODEC_FLAG_TRUNCATED; /* we do not send complete frames */

    if (avcodec_open2(m_codec_ctx, codec, &m_param))
    {
        return -1;
    }

    // ------------------------------------------------------
    // 用有效的信息把 AVFormatContext 的流域（streams field）填满
    // 0代表AVOutputFormat，1代表AVInputFormat
    av_dump_format(m_fmt_ctx, 0, m_filepath, 1);	// ??

    return 0;
}

void FFmpegEncode::prepareSwsContext()
{
	// RGBA -> YUV420
	if (m_sws_ctx == NULL)
	{
		m_sws_ctx = sws_getContext(m_width, m_height, IN_VID_FMT,
			m_codec_ctx->width, m_codec_ctx->height, m_codec_ctx->pix_fmt,
			SWS_FAST_BILINEAR, NULL, NULL, NULL);	// SWS_ACCURATE_RND, SWS_LANCZOS, SWS_POINT, SWS_BICUBIC

		// --------------------------------------------
#if 0
		int colorspace = SWS_CS_ITU601;
		const int* pCoef = sws_getCoefficients(colorspace);
		const int* inv_table = sws_getCoefficients(SWS_CS_ITU601); 
		const int* table = sws_getCoefficients(AVCOL_SPC_RGB);
		int srcRange, dstRange;
		int brightness = 0, contrast = 1<<16, saturation = 1<<16;

		int result = sws_getColorspaceDetails(m_sws_ctx, &inv_table, &srcRange, &table, &dstRange, &brightness, &contrast, &saturation);
		if (result != -1)
		{
			result = sws_setColorspaceDetails(m_sws_ctx, pCoef, srcRange, pCoef, dstRange, brightness, contrast, saturation);
		}
		//sws_setColorspaceDetails(m_sws_ctx, inv_table, 0, table, 0, brightness, contrast, saturation);

		printf("srcRange = %d, dstRange = %d\n", srcRange, dstRange);
		printf("inv_table = %d, table = %d\n", *inv_table, *table);
		printf("brightness = %d, contrast = %d, saturation = %d\n", brightness, contrast, saturation);

		 //打印 configuration 信息
		const char* info = swscale_configuration();
		printf("info = %s\n", info);
		 --------------------------------------------

		//Colorspace
		//int ret = sws_setColorspaceDetails(m_sws_ctx, 
		//	sws_getCoefficients(SWS_CS_ITU709 ), 0,
		//	sws_getCoefficients(SWS_CS_ITU709), 0,
		//	0, 0, 0);
		//if (ret == -1)
		//{
		//	printf( "Colorspace not support.\n");
		//	return ;
		//}
#endif
	}
}

int FFmpegEncode::makeFrame()
{
    // ------------------------------------------------------
    // 初始化AVFrame
    m_out_frm = av_frame_alloc();
    m_out_frm->format = OUT_VID_FMT;
    m_out_frm->width = m_width;
    m_out_frm->height = m_height;
	//av_frame_set_color_range(m_out_frm, AVCOL_RANGE_MPEG);
	//av_frame_set_colorspace(m_out_frm, AVCOL_SPC_RGB);

    // 为AVFrame分配内存
    // AV_PIX_FMT_YUV420P
    int size = avpicture_get_size(m_codec_ctx->pix_fmt, m_codec_ctx->width,
                                  m_codec_ctx->height);

    m_picture_buf = (uint8_t *)av_malloc(size);
    if (!m_picture_buf)
    {
        av_frame_free(&m_out_frm);
        return -1;
    }
    // 将缓存与AVFrame进行关联
    avpicture_fill((AVPicture *)m_out_frm, m_picture_buf, m_codec_ctx->pix_fmt,
                   m_codec_ctx->width, m_codec_ctx->height);
    // ------------------------------------------------------


    return 0;
}

int FFmpegEncode::processFrame(const unsigned char *raw_data)
{
#ifdef DEBUG_LOG
    TRACE_FUNC();
#endif

    if (NULL == raw_data)
    {
        return -1;
    }
    // 数据格式转换！
    cvtRawData2Frame(raw_data, m_out_frm);

    // ------------------------------------------------------------------------------
    static long int index = 0;
    m_out_frm->pts = index++;

    AVPacket enc_pkt;
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);

    int got_out;
    // ret == 0 表示成功
    // got_out !=0 表示packet 不为空
    int ret = avcodec_encode_video2(m_codec_ctx, &enc_pkt, m_out_frm, &got_out);

    if (got_out > 0)
    {
        // ☆ 时间基换算 AVCodecContext(0.04) -> （AVStream(0.000011),(0.000078)☆
        // 编码后的帧的PTS
        enc_pkt.pts = av_rescale_q(m_out_frm->pts, m_codec_ctx->time_base, m_out_stream->time_base);

#ifdef DEBUG_LOG
        av_log(NULL, AV_LOG_WARNING, "dts = %ld, pts = %ld\n", enc_pkt.dts, enc_pkt.pts);
        av_log(NULL, AV_LOG_WARNING, "Encode - AVCodecContext:%lf, AVStream:%lf\n", av_q2d(m_codec_ctx->time_base), av_q2d(m_out_stream->time_base));
        av_log(NULL, AV_LOG_WARNING, "Encode - coded_frame_pts = %ld, rescale_pts = %lld\n", m_codec_ctx->coded_frame->pts, enc_pkt.pts);
#endif

        if (m_codec_ctx->coded_frame->key_frame)
        {
            enc_pkt.flags |= AV_PKT_FLAG_KEY;
        }

        enc_pkt.stream_index = m_videoindex;

        // 将包中数据交给Muxer(将packet写入文件当中)
        int ret = av_write_frame(m_fmt_ctx, &enc_pkt);
        //		int ret = av_interleaved_write_frame(m_fmt_ctx, &pkt);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "av_write_frame error! ");
            av_packet_unref(&enc_pkt);
            return -1;
        }
    }
    else
    {
        av_log(NULL, AV_LOG_ERROR, "avcodec_encode_video2 error! ");
        av_packet_unref(&enc_pkt);
        return -1;
    }

    av_packet_unref(&enc_pkt);
    return 0;
}

void FFmpegEncode::cvtRawData2Frame(const unsigned char *raw_data, AVFrame *frame)
{

	if (NULL == m_sws_ctx)
	{
		return;
	}

    // 给pFrameRGBA帧分配缓冲区
    AVFrame *pFrameRGBA = av_frame_alloc();
    pFrameRGBA->format = IN_VID_FMT;

    int size = avpicture_get_size(IN_VID_FMT, m_width, m_height);
    uint8_t *buf = (uint8_t *)av_malloc(size);
    // 拷贝 raw_data 到缓冲区
    memcpy(buf, raw_data, size);
    // frame <=> buf
    avpicture_fill((AVPicture *)pFrameRGBA, buf, IN_VID_FMT, m_width,
                   m_height);

    // RGBA->YUV420
    sws_scale(m_sws_ctx, pFrameRGBA->data, pFrameRGBA->linesize, 0, m_height,
              frame->data, frame->linesize);

    av_free(buf);
    av_frame_free(&pFrameRGBA);
}

int FFmpegEncode::startEncodeVideo()
{
#ifdef DEBUG_LOG
    TRACE_FUNC();
#endif
    if (!(m_out_fmt->flags & AVFMT_NOFILE))
    {
        // 打开IO, AVIOContext* pb
        if (avio_open(&m_fmt_ctx->pb, m_filepath, AVIO_FLAG_WRITE) < 0)
        {
            return -1;
        }
    }

    // 写入文件头
    avformat_write_header(m_fmt_ctx, NULL);

	m_start = clock();

    return 0;
}

void FFmpegEncode::endEncodeVideo()
{
#ifdef DEBUG_LOG
    TRACE_FUNC();
#endif

	m_end = clock();
	m_fmt_ctx->duration = 1.f*(m_end - m_start) / CLOCKS_PER_SEC;

    // 写尾部信息
    av_write_trailer(m_fmt_ctx);
}

int FFmpegEncode::flushEncoder(AVFormatContext *fmt_ctx,
                               unsigned int stream_index)
{
    int ret;
    int got_frame;
    AVPacket enc_pkt;

    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities
            & CODEC_CAP_DELAY))
        return 0;

    while (1)
    {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec,
                                    &enc_pkt, NULL, &got_frame);
        av_frame_free(NULL);

        if (ret < 0)
            break;

        if (!got_frame)
        {
            ret = 0;
            break;
        }

#if DEBUG_LOG
        av_log(NULL, AV_LOG_INFO, "Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
#endif
        /* mux encoded frame */
        ret = av_write_frame(fmt_ctx, &enc_pkt);

        if (ret < 0)
            break;
    }

    return ret;
}

double FFmpegEncode::getVideoDuration()
{
    int64_t duration = m_fmt_ctx->duration;
    double d = duration * 1.0f / AV_TIME_BASE;
#ifdef DEBUG_LOG
    av_log(NULL, AV_LOG_ERROR, "Encode - duration = %lf\n", d);
#endif
    return d;
}

double FFmpegEncode::getVideoPlayingTime()
{
    double time = av_q2d(m_codec_ctx->time_base)
                  * m_codec_ctx->coded_frame->pts;

    if (time < 0)
    {
#ifdef DEBUG_LOG
        av_log(NULL, AV_LOG_ERROR, "Encode - playing_time = %lf\n", time);
#endif
        time = -1.0;
    }
    else
    {
#ifdef DEBUG_LOG
        av_log(NULL, AV_LOG_WARNING, "Encode - playing_time = %lf\n", time);
#endif
    }

    return time;
}

