#include "base/codec.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <memory>

void onNewFrame(AVFrame* frame) {
    EXPECT_EQ(frame->width, 960);
    EXPECT_EQ(frame->height, 400);
}

TEST(CodecTest, CheckDecoder) {
    auto FormatCtx =
        std::make_unique<avrtc::FormatContext>("test_data/oceans.mp4");
    auto codec = std::make_unique<avrtc::Codec>(AV_CODEC_ID_H264);
    EXPECT_NE(codec, nullptr);
    AVStream* stream =
        FormatCtx->GetStream(avrtc::FormatContext::MediaType::VIDEO);
    codec->CopyParamsFromStream(stream);
    codec->SetOnDecodeCallback(onNewFrame);
    AVPacket* packet = nullptr;
    while ((packet = FormatCtx->GetNextPacket()) != nullptr) {
        if (packet->stream_index != stream->index) {
            av_packet_unref(packet);
            continue;
        }
        codec->DecodeFrame(packet);
        av_packet_unref(packet);
    }
    codec->FlushDecoder();
}