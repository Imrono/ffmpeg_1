#include <iostream>
using namespace std;
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include <jpeglib.h>
#include "VAP_Common.h"
}

int open_input_file(const char *filename, AVFormatContext **in_fmt_ctx,
		int &nVideoStream);
int SaveFrameToJPG(char *picFile, uint8_t *pRGBBuffer, int nWidth, int nHeight);
int SaveFrameToBMP(char *picFile, uint8_t *pRGBBuffer, int nWidth, int nHeight,
		int nBitCount);
int SaveFrameToPPM(char *picFile, AVFrame *pFrame, int nWidth, int nHeight);

int main(int argc, char **argv) {
	cout << "VRG_ROIInfo_Version:" << VRG_ROIInfo_Version() << endl;

	int inSecond = 0;
	cout << "start second" << endl;
	cin >> inSecond;

	int ret = 0;
	AVFormatContext *ifmt_ctx;
	AVPacket packet;
	AVFrame *frame = NULL;
	unsigned stream_index = 0;
	int nVideoStream = -1;
	unsigned type = 0;
	AVCodecContext *codecCtxDec = NULL;  // 编解码器上下文
	int got_frame = 0;

	char szPicFile[256] = "tetete.jpeg";
	uint8_t *pBuffer;
	int numBytes;
	AVFrame *pFrameRGB = NULL;
	struct SwsContext *img_convert_ctx;

	cout << "VRG_ROIInfo_Version:" << VRG_ROIInfo_Version() << endl;
//1. register all formats and codecs
	av_register_all();

//2. open_input_file - 打开输入文件（得到并打开各个流的解码器AVCodec）
	if ((ret = open_input_file(argv[1], &ifmt_ctx, nVideoStream)) < 0) {
		goto END;
	}

//3. av_seek_frame - 搜索timestamp外的关键帧KeyFrame
	if ((ret = av_seek_frame(ifmt_ctx, -1, inSecond * AV_TIME_BASE, AVSEEK_FLAG_ANY/*AVSEEK_FLAG_BACKWARD*/)) < 0) {
		goto END;
	}

	av_init_packet(&packet);
	while (true) {
//4. av_read_frame - 读取码流中的音频若干帧或者视频一帧。
//					 解码视频的时候，每解码一个视频帧，需要先调用av_read_frame()获得一帧视频的压缩数据，
//					 然后才能对该数据进行解码。
		if ((ret = av_read_frame(ifmt_ctx, &packet)) < 0)
			goto END;

		stream_index = packet.stream_index;
		type = ifmt_ctx->streams[packet.stream_index]->codec->codec_type;
		codecCtxDec = ifmt_ctx->streams[nVideoStream]->codec;
		av_log(NULL, AV_LOG_DEBUG, "Demuxer gave frame of stream_index %u\n", stream_index);
		if (type != AVMEDIA_TYPE_VIDEO)
			continue;

//5. av_frame_alloc - AVFrame的初始化函数1
		frame = av_frame_alloc();
		if (!frame) {
			ret = AVERROR(ENOMEM);
			goto END;
		}

//6. avcodec_decode_video2 - 输入一个压缩编码的结构体AVPacket，输出一个解码后的结构体AVFrame
		ret = avcodec_decode_video2(ifmt_ctx->streams[stream_index]->codec,
				frame, &got_frame, &packet);
		if (ret < 0) {
			av_frame_free(&frame);
			av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
			goto END;
		}
		if (got_frame) {
//7. 为RGB图片分配内存，并初始化
			numBytes = avpicture_get_size(AV_PIX_FMT_BGR24, codecCtxDec->width,
					codecCtxDec->height);
			pBuffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
			pFrameRGB = av_frame_alloc();
			avpicture_fill((AVPicture *) pFrameRGB, pBuffer,
					AV_PIX_FMT_BGR24/*AV_PIX_FMT_RGB24*/, codecCtxDec->width,
					codecCtxDec->height);

//8. 得到RGB24数据
			img_convert_ctx = sws_getContext(codecCtxDec->width,
					codecCtxDec->height, codecCtxDec->pix_fmt,
					codecCtxDec->width, codecCtxDec->height, AV_PIX_FMT_BGR24,
					SWS_BILINEAR, NULL, NULL, NULL);
//9. pFrameAV -> pFrameRGB
			sws_scale(img_convert_ctx, frame->data, frame->linesize, 0,
					codecCtxDec->height, pFrameRGB->data, pFrameRGB->linesize);
			SaveFrameToJPG(szPicFile, pFrameRGB->data[0], codecCtxDec->width,
					codecCtxDec->height);
			//SaveFrameToBMP(szPicFile, pFrameRGB->data[0],  codecCtxDec->width, codecCtxDec->height, 24);
			//SaveFrameToPPM(szPicFile, pFrameRGB, codecCtxDec->width, codecCtxDec->height);

			av_frame_free(&pFrameRGB);
			av_free(pBuffer);
			sws_freeContext(img_convert_ctx);
			av_frame_free(&frame);
			goto END;
		} else {
			av_frame_free(&frame);
		}
	}
END:
	av_packet_unref(&packet);
	av_frame_free(&frame);
	return 0;
}

int open_input_file(const char *filename, AVFormatContext **in_fmt_ctx,
		int &nVideoStream) {
	int ret;
	unsigned int i;
	AVFormatContext *ifmt_ctx = NULL;
//1. avformat_open_input - 打开多媒体数据并且获得一些相关的信息。
	if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
		return ret;
	}
//2. avformat_find_stream_info - 填充ifmt_ctx，确定流信息
	if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
		return ret;
	}
//3. 循环流，视频、音频等
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		AVStream *stream;
		AVCodecContext *codec_ctx;
		stream = ifmt_ctx->streams[i];
		codec_ctx = stream->codec;
		/* Reencode video & audio and remux subtitles etc. */
		if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
				|| codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
			/* Open decoder */
//4. avcodec_find_decoder - 从codec_ctx->codec_id查找FFmpeg的解码器
			AVCodec *codec = avcodec_find_decoder(codec_ctx->codec_id);
//5. avcodec_open2 - 解码，填充codec_ctx
			ret = avcodec_open2(codec_ctx, codec, NULL);

			if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
				nVideoStream = i;
			}
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR,
						"Failed to open decoder for stream #%u\n", i);
				return ret;
			}
		}
	}
	av_dump_format(ifmt_ctx, 0, filename, 0);

	*in_fmt_ctx = ifmt_ctx;
	return 0;
}

int SaveFrameToJPG(char *picFile, uint8_t *pRGBBuffer, int nWidth,
		int nHeight) {
	FILE *fp = fopen(picFile, "wb");
	if (fp == NULL) {
		printf("file open error %s\n", picFile);
		return -1;
	}

	struct jpeg_compress_struct jcs;
	// 声明错误处理器，并赋值给jcs.err域
	struct jpeg_error_mgr jem;
	jcs.err = jpeg_std_error(&jem);

	jpeg_create_compress(&jcs);

	jpeg_stdio_dest(&jcs, fp);

	// 为图的宽和高，单位为像素
	jcs.image_width = nWidth;
	jcs.image_height = nHeight;
	// 在此为1,表示灰度图， 如果是彩色位图，则为3
	jcs.input_components = 3;
	//JCS_GRAYSCALE表示灰度图，JCS_RGB表示彩色图像
	jcs.in_color_space = JCS_RGB;

	jpeg_set_defaults(&jcs);
	jpeg_set_quality(&jcs, 80, TRUE);
	jpeg_start_compress(&jcs, TRUE);

	// 一行位图
	JSAMPROW row_pointer[1];
	//每一行的字节数,如果不是索引图,此处需要乘以3
	int row_stride = jcs.image_width * 3;

	// 对每一行进行压缩
	while (jcs.next_scanline < jcs.image_height) {
		row_pointer[0] = &(pRGBBuffer[jcs.next_scanline * row_stride]);
		jpeg_write_scanlines(&jcs, row_pointer, 1);
	}
	jpeg_finish_compress(&jcs);
	jpeg_destroy_compress(&jcs);

	fclose(fp);
	return 0;
}

#pragma pack(1)
#pragma pack(push)
#pragma pack(1)
typedef struct tagBITMAPFILEHEADER {
	u_int16_t bfType;		//2 位图文件的类型，必须为“BM”
	u_int32_t bfSize;       //4 位图文件的大小，以字节为单位
	u_int16_t bfReserved1;	//2 位图文件保留字，必须为0
	u_int16_t bfReserved2;	//2 位图文件保留字，必须为0
	u_int32_t bfOffBits;    //4 位图数据的起始位置，以相对于位图文件头的偏移量表示，以字节为单位
} BITMAPFILEHEADER;			//该结构占据14个字节。
typedef struct tagBITMAPINFOHEADER {
	u_int32_t biSize;			//4 本结构所占用字节数
	u_int32_t biWidth;			//4 位图的宽度，以像素为单位
	u_int32_t biHeight;   		//4 位图的高度，以像素为单位
	u_int16_t biPlanes;   		//2 目标设备的平面数不清，必须为1
	u_int16_t biBitCount;   	//2 每个像素所需的位数，必须是1(双色), 4(16色)，8(256色)或24(真彩色)之一
	u_int32_t biCompression; //4 位图压缩类型，必须是 0(不压缩),1(BI_RLE8压缩类型)或2(BI_RLE4压缩类型)之一
	u_int32_t biSizeImage;   	//4 位图的大小，以字节为单位
	u_int32_t biXPelsPerMeter; 	//4 位图水平分辨率，每米像素数
	u_int32_t biYPelsPerMeter; 	//4 位图垂直分辨率，每米像素数
	u_int32_t biClrUsed; 		//4 位图实际使用的颜色表中的颜色数
	u_int32_t biClrImportant; 	//4 位图显示过程中重要的颜色数
} BITMAPINFOHEADER;          	//该结构占据40个字节。
#pragma pack(pop)
int SaveFrameToBMP(char *picFile, uint8_t *pRGBBuffer, int nWidth, int nHeight,
		int nBitCount) {
	BITMAPFILEHEADER bmpheader;
	BITMAPINFOHEADER bmpinfo;
	memset(&bmpheader, 0, sizeof(BITMAPFILEHEADER));
	memset(&bmpinfo, 0, sizeof(BITMAPINFOHEADER));

	FILE *fp = NULL;
	fp = fopen(picFile, "wb");
	if (NULL == fp) {
		printf("file open error %s\n", picFile);
		return -1;
	}
	// set BITMAPFILEHEADER value
	bmpheader.bfType = 0x4d42; //'BM';
	bmpheader.bfReserved1 = 0;
	bmpheader.bfReserved2 = 0;
	bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmpheader.bfSize = bmpheader.bfOffBits + nWidth * nHeight * nBitCount / 8;
	//bmpheader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)+ iBufLeng;
	// set BITMAPINFOHEADER value
	bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.biWidth = nWidth;
	bmpinfo.biHeight = 0 - nHeight;
	bmpinfo.biPlanes = 1;
	bmpinfo.biBitCount = nBitCount;
	bmpinfo.biCompression = 0/*BI_RGB*/;
	bmpinfo.biSizeImage = 0;
	bmpinfo.biXPelsPerMeter = 0;
	bmpinfo.biYPelsPerMeter = 0;
	bmpinfo.biClrUsed = 0;
	bmpinfo.biClrImportant = 0;
	// write pic file
	fwrite(&bmpheader, sizeof(BITMAPFILEHEADER), 1, fp);
	fwrite(&bmpinfo, sizeof(BITMAPINFOHEADER), 1, fp);
	fwrite(pRGBBuffer, nWidth * nHeight * nBitCount / 8, 1, fp);
	fclose(fp);
	return 0;
}

int SaveFrameToPPM(char *picFile, AVFrame *pFrame, int nWidth, int nHeight) {
	FILE *fp = fopen(picFile, "wb");
	if (NULL == fp) {
		printf("file open error %s\n", picFile);
		return -1;
	}
	// write header
	fprintf(fp, "P6\n%d %d\n255\n", nWidth, nHeight);

	// write pixel data
	for (int y = 0; y < nHeight; y++) {
		fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, nWidth * 3, fp);
	}
	fclose(fp);
	return 0;
}
