#include "base/codec.h"

namespace avrtc {

/**
 * @brief Check the return value of an FFmpeg function call.
 * @param ret The return value to check.
 * @note If the return value indicates an error (negative value),
 *      an error message is logged using glog.
 */
void CheckFfmpeg(const int ret) {
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG(ERROR) << "FFmpeg error: " << errbuf;
    }
}

/**
 * @brief Check the return value of an FFmpeg function call that returns a
 * pointer.
 * @param ret The pointer to check.
 * @note If the pointer is null, an error message is logged using glog.
 */
void CheckFfmpeg(const void* ret) {
    if (ret == nullptr) {
        LOG(ERROR) << "FFmpeg error: returned null pointer.";
    }
}

/**
 * Constructor for FormatContext class.
 * @param fileName The name of the input file to open.
 * @note Initializes the format context by opening the input file
 *     and finding the stream information.
 */
FormatContext::FormatContext(const std::string& fileName) {
    InitInCtx(fileName);
}

/**
 * Initialize the format context by opening the input file
 *       and finding the stream information.
 */
void FormatContext::InitInCtx(const std::string& fileName) {
    int video_stream_index_, audio_stream_index_;
    CheckFfmpeg(
        avformat_open_input(&format_ctx_, "test_data/oceans.mp4", NULL, NULL));
    CheckFfmpeg(avformat_find_stream_info(format_ctx_, NULL));
    CheckFfmpeg(video_stream_index_ = av_find_best_stream(
                    format_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0));
    CheckFfmpeg(audio_stream_index_ = av_find_best_stream(
                    format_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0));
    streams_.push_back(format_ctx_->streams[video_stream_index_]);
    streams_.push_back(format_ctx_->streams[audio_stream_index_]);
}

AVStream* FormatContext::GetStream(MediaType) {
    return streams_[static_cast<int>(MediaType::VIDEO)];
}

AVPacket* FormatContext::GetNextPacket() {
    if (av_read_frame(format_ctx_, packet) < 0)
        return nullptr;
    return packet;
}

FormatContext::~FormatContext() {
    avformat_close_input(&format_ctx_);
    av_packet_free(&packet);
}

/**
 * Constructor for Codec class.
 * @param id The AVCodecID of the codec to initialize.
 * @note Initializes the codec context for decoding.
 */
Codec::Codec(AVCodecID id) {
    InitCodecCtx(id);
    packet = av_packet_alloc();
    frame = av_frame_alloc();
}

/**
 * Copy codec parameters from the given AVStream to the codec context.
 * @param stream The AVStream from which to copy parameters.
 */
void Codec::CopyParamsFromStream(AVStream* stream) {
    CheckFfmpeg(avcodec_parameters_to_context(codec_ctx, stream->codecpar));
    CheckFfmpeg(avcodec_open2(codec_ctx, codec, NULL));
}
void Codec::SetOnDecodeCallback(OnDecodeNewFrameCallback cb) {
    onDeFrameCb_ = cb;
}

void Codec::SetOnEncodeCallback(OnEncodeNewPacketCallback cb) {
    onEnPacketCb_ = cb;
}

void Codec::DecodeFrame(AVPacket* packet) {
    CheckFfmpeg(avcodec_send_packet(codec_ctx, packet));
    int ret = 0;
    while (ret >= 0) {
        ret = avcodec_receive_frame(codec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            CheckFfmpeg(ret);
        }
        if (onDeFrameCb_)
            onDeFrameCb_(frame);
        else
            LOG(WARNING) << "onDeFrameCb_ is not set.";
        av_frame_unref(frame);
    }
}

void Codec::FlushDecoder() {
    DecodeFrame(nullptr);
}

void Codec::EncodeFrame(AVFrame* frame) {
    CheckFfmpeg(avcodec_send_frame(codec_ctx, frame));
    int ret = 0;
    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_ctx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        CheckFfmpeg(ret);
        if (onEnPacketCb_)
            onEnPacketCb_(packet);
        else
            LOG(WARNING) << "onEnPacketCb_ is not set.";
        av_packet_unref(packet);
    }
}

void Codec::FlushEncoder() {
    EncodeFrame(nullptr);
}

/**
 * Initialize the decoder context for the given codec ID.
 * @param id The AVCodecID of the codec to initialize.
 * @note Finds the decoder for the given codec ID and allocates
 *    the codec context.
 */
void Codec::InitCodecCtx(AVCodecID id) {
    CheckFfmpeg(codec = avcodec_find_decoder(id));
    CheckFfmpeg(codec_ctx = avcodec_alloc_context3(codec));
}

Codec::~Codec() {
    avcodec_free_context(&codec_ctx);
    av_packet_free(&packet);
    av_frame_free(&frame);
}

}  // namespace avrtc