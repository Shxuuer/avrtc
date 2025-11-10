#ifndef BASE_CODEC_H
#define BASE_CODEC_H

#include <arpa/inet.h>
#include <glog/logging.h>
#include <unistd.h>

#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "base/codec_type.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

namespace avrtc {

void CheckFfmpeg(int ret);
void CheckFfmpeg(const void* ret);

class FormatContext {
   public:
    FormatContext(const std::string& fileName);
    ~FormatContext();

    enum class MediaType { VIDEO, AUDIO };
    AVStream* GetStream(MediaType);
    AVPacket* GetNextPacket();

   protected:
    void InitInCtx(const std::string& fileName);

   private:
    AVFormatContext* format_ctx_ = NULL;
    std::vector<AVStream*> streams_;
    AVPacket* packet = av_packet_alloc();
};

class Codec {
   public:
    Codec(AVCodecID id);
    ~Codec();

    using OnDecodeNewFrameCallback = std::function<void(AVFrame*)>;
    using OnEncodeNewPacketCallback = std::function<void(AVPacket*)>;

    void CopyParamsFromStream(AVStream* stream);
    void SetOnDecodeCallback(OnDecodeNewFrameCallback cb);
    void SetOnEncodeCallback(OnEncodeNewPacketCallback cb);
    void DecodeFrame(AVPacket* packet);
    void FlushDecoder();
    void EncodeFrame(AVFrame* frame);
    void FlushEncoder();

   protected:
    void InitCodecCtx(AVCodecID id);

   private:
    const AVCodec* codec;
    AVCodecContext* codec_ctx;
    AVPacket* packet;
    AVFrame* frame;

    OnDecodeNewFrameCallback onDeFrameCb_;
    OnEncodeNewPacketCallback onEnPacketCb_;
};

}  // namespace avrtc

#endif  // BASE_CODEC_H