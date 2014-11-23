extern "C"{

#ifdef __cplusplus
 #define __STDC_CONSTANT_MACROS
 #ifdef _STDINT_H
  #undef _STDINT_H
 #endif
 # include <stdint.h>
#endif

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <chrono>

#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>

#include <sys/types.h>
#include <sys/syscall.h>

#include "capture.h"
#include "vcompress.h"

static UsageEnvironment *_env = 0;

#define SINK_PORT 3030
#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 720
#define FRAME_PER_SEC 25

using namespace std;
using namespace std::chrono;

pid_t gettid()
{
	return syscall(SYS_gettid);
}

#if 0 // for decode test
struct DecodeContext {
    AVCodec *codec;
    AVCodecContext *av_codec_context_;
    AVFrame *picture;
} dc;

static void decode_test(unsigned char *buffer, int len)
{
    static bool isInit = false;

    if (!isInit) {
        avcodec_register_all();

        memset(&dc, 0, sizeof(dc));
        // init context
        dc.codec = avcodec_find_decoder(CODEC_ID_H264);
        if (NULL == dc.codec) {
            cout << "Cannot create codec" << endl;
            return;
        }
        dc.av_codec_context_ = avcodec_alloc_context3(dc.codec);
        if (NULL == dc.av_codec_context_) {
            cout << "Cannot create codec context" << endl;
            return;
        }
        dc.av_codec_context_->width = VIDEO_WIDTH;
        dc.av_codec_context_->height = VIDEO_HEIGHT;
        dc.av_codec_context_->extradata = NULL;
        dc.av_codec_context_->pix_fmt = PIX_FMT_YUV420P;
        int res = avcodec_open2(dc.av_codec_context_, dc.codec, NULL);
        if (0 != res) {
            cout << "Cannot open codec: " << res << endl;
            return;
        }

        // init picture
        dc.picture = av_frame_alloc();
        if (NULL == dc.picture) {
            cout << "Cannot alloc frame" << endl;
            return;
        }
        int size = avpicture_get_size(PIX_FMT_YUV420P, VIDEO_WIDTH, VIDEO_HEIGHT);
        uint8_t *picbuf = (uint8_t *)av_malloc(size);
        avpicture_fill((AVPicture *)dc.picture, picbuf, PIX_FMT_YUV420P,
                VIDEO_WIDTH, VIDEO_HEIGHT);

        isInit = true;
    }

    AVPacket pkt;
    av_new_packet(&pkt, len);
    pkt.data = buffer;
    pkt.size = len;

    int frameFinished = 0;
    int res = avcodec_decode_video2(dc.av_codec_context_,
            dc.picture, &frameFinished, &pkt);
    if (res < 0) {
        cout << "cannot decode video2: " << res << endl;
        return;
    }

    cout << "Frame finished: " << frameFinished << endl;
}
#endif

// 使用 webcam + x264
class WebcamFrameSource : public FramedSource
{
	void *mp_capture, *mp_compress;	// v4l2 + x264 encoder
	int m_started;
	void *mp_token;
    system_clock::time_point mtime;
    system_clock::time_point mtimeGetting;

public:
	WebcamFrameSource(UsageEnvironment &env, int width,
            int height, PixelFormat format, double frame)
		: FramedSource(env)
	{
		fprintf(stderr, "[%d] %s .... calling\n", gettid(), __func__);
		mp_capture = capture_open("/dev/video0", width, height, format);
		if (!mp_capture) {
			fprintf(stderr, "%s: open /dev/video0 err\n", __func__);
			exit(-1);
		}

		mp_compress = vc_open(width, height, frame);
		if (!mp_compress) {
			fprintf(stderr, "%s: open x264 err\n", __func__);
			exit(-1);
		}

		m_started = 0;
		mp_token = 0;
        mtime = system_clock::now();
        mtimeGetting = system_clock::now();
	}

	~WebcamFrameSource ()
	{
		fprintf(stderr, "[%d] %s .... calling\n", gettid(), __func__);
		
		if (m_started) {
			envir().taskScheduler().unscheduleDelayedTask(mp_token);
		}

		if (mp_compress)
			vc_close(mp_compress);
		if (mp_capture)
			capture_close(mp_capture);
	}

protected:
    virtual unsigned maxFrameSize() const { return 100 * 1024; }
	virtual void doGetNextFrame ()
	{
        cout << "Function " << __FUNCTION__ << " at dur " <<
            (duration_cast<milliseconds>(system_clock::now() - mtime)).count()
            << endl;
		if (m_started) return;
		m_started = 1;

		// 根据 fps, 计算等待时间
		double delay = 1000.0 / FRAME_PER_SEC;
		int to_delay = delay * 1000;	// us

		mp_token = envir().taskScheduler().scheduleDelayedTask(to_delay,
				getNextFrame, this);
	}

private:
	static void getNextFrame (void *ptr)
	{
		((WebcamFrameSource*)ptr)->getNextFrame1();
	}

	void getNextFrame1 ()
	{
		// capture:
		Picture pic;
		if (capture_get_picture(mp_capture, &pic) < 0) {
			fprintf(stderr, "==== %s: capture_get_picture err\n", __func__);
			m_started = 0;
			return;
		}

		// compress
		const void *outbuf;
		int outlen;
		if (vc_compress(mp_compress, pic.data, pic.stride, &outbuf, &outlen) < 0) {
			fprintf(stderr, "==== %s: vc_compress err\n", __func__);
			m_started = 0;
			return;
		}

#if 0 // for decode test
        decode_test((unsigned char *)outbuf, outlen);
#endif

		int64_t pts, dts;
		int key;
		vc_get_last_frame_info(mp_compress, &key, &pts, &dts);

		// save outbuf
		gettimeofday(&fPresentationTime, 0);
		fFrameSize = outlen;
		if (fFrameSize > fMaxSize) {
			fNumTruncatedBytes = fFrameSize - fMaxSize;
			fFrameSize = fMaxSize;
		}
		else {
			fNumTruncatedBytes = 0;
		}

		memmove(fTo, outbuf, outlen);

		// notify
		afterGetting(this);
        cout << "Function " << __FUNCTION__ << " getting at dur " << (
                duration_cast<milliseconds>(
                    system_clock::now() - mtimeGetting)).count() << endl;

		m_started = 0;
	}
};

class WebcamOndemandMediaSubsession : public OnDemandServerMediaSubsession
{
private:
    int mwidth;
    int mheight;
    PixelFormat mformat;
    double mfps;


protected:
	WebcamOndemandMediaSubsession(UsageEnvironment &env, int width,
            int height, PixelFormat format, double fps)
		: OnDemandServerMediaSubsession(env, True) // reuse the first source
	{
		fprintf(stderr, "[%d] %s .... calling\n", gettid(), __func__);

        mwidth = width;
        mheight = height;
        mformat = format;
        mfps = fps;
	}

	~WebcamOndemandMediaSubsession ()
	{
		fprintf(stderr, "[%d] %s .... calling\n", gettid(), __func__);
	}

	virtual RTPSink *createNewRTPSink(Groupsock *rtpsock, unsigned char type, FramedSource *source)
	{
		fprintf(stderr, "[%d] %s .... calling\n", gettid(), __func__);
		return H264VideoRTPSink::createNew(envir(), rtpsock, type);
	}

	virtual FramedSource *createNewStreamSource(unsigned sid, unsigned &bitrate)
	{
		fprintf(stderr, "[%d] %s .... calling\n", gettid(), __func__);
		bitrate = 500;
		return H264VideoStreamFramer::createNew(envir(),
                new WebcamFrameSource(envir(), mwidth, mheight, mformat, mfps));
	}

    public: static WebcamOndemandMediaSubsession *createNew(UsageEnvironment &env,
                    int width, int height, PixelFormat format, double fps)
	{
		return new WebcamOndemandMediaSubsession(env, width, height, format, fps);
	}
};

int main (int argc, char **argv)
{
	// env
	TaskScheduler *scheduler = BasicTaskScheduler::createNew();
	_env = BasicUsageEnvironment::createNew(*scheduler);

	// rtsp server
	RTSPServer *rtspServer = RTSPServer::createNew(*_env, SINK_PORT);
	if (!rtspServer) {
		fprintf(stderr, "ERR: create RTSPServer err\n");
		exit(-1);
	}

	// add live stream
	do {
        // low resolution
		ServerMediaSession *sms = ServerMediaSession::createNew(*_env,
                "live", 0, "Session from /dev/video0"); 
		sms->addSubsession(WebcamOndemandMediaSubsession::createNew(*_env,
                    640, 360, PIX_FMT_YUV420P, FRAME_PER_SEC));
		rtspServer->addServerMediaSession(sms);

		char *url = rtspServer->rtspURL(sms);
		*_env << "using url \"" << url << "\"\n";
		delete [] url;

        // high resolution
		sms = ServerMediaSession::createNew(*_env,
                "live-high", 0, "Session from /dev/video0 with high resolution"); 
		sms->addSubsession(WebcamOndemandMediaSubsession::createNew(*_env,
                    1280, 720, PIX_FMT_YUV420P, FRAME_PER_SEC));
		rtspServer->addServerMediaSession(sms);

		url = rtspServer->rtspURL(sms);
		*_env << "using url \"" << url << "\"\n";
		delete [] url;
	} while (0);

	// run loop
	_env->taskScheduler().doEventLoop();

	return 1;
}

