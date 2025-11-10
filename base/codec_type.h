#ifndef BASE_CODEC_TYPE_H
#define BASE_CODEC_TYPE_H

#include <glog/logging.h>

#include <string>
#include <unordered_map>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace avrtc {

#define CODEC_TYPE_LIST                   \
  CODEC_ITEM(MUTE, 48, AV_CODEC_ID_NONE)  \
  CODEC_ITEM(VP8, 96, AV_CODEC_ID_VP8)    \
  CODEC_ITEM(VP9, 98, AV_CODEC_ID_VP9)    \
  CODEC_ITEM(H264, 99, AV_CODEC_ID_H264)  \
  CODEC_ITEM(H265, 106, AV_CODEC_ID_H265) \
  CODEC_ITEM(OPUS, 111, AV_CODEC_ID_OPUS)

enum class CodecType {
#define CODEC_ITEM(name, value, ffmpeg_id) name = value,
  CODEC_TYPE_LIST
#undef CODEC_ITEM
};

static const std::unordered_map<CodecType, std::string> codec_type_to_string = {
#define CODEC_ITEM(name, value, ffmpeg_id) {CodecType::name, #name},
    CODEC_TYPE_LIST
#undef CODEC_ITEM
};

static const std::unordered_map<std::string, CodecType> string_to_codec_type = {
#define CODEC_ITEM(name, value, ffmpeg_id) {#name, CodecType::name},
    CODEC_TYPE_LIST
#undef CODEC_ITEM
};

static const std::unordered_map<CodecType, AVCodecID> codec_type_to_ffmpeg_id =
    {
#define CODEC_ITEM(name, value, ffmpeg_id) {CodecType::name, ffmpeg_id},
        CODEC_TYPE_LIST
#undef CODEC_ITEM
};

std::string CodecTypeToString(CodecType type);
CodecType StringToCodecType(const std::string& str);
AVCodecID CodecTypeToFFmpegCodecID(CodecType type);

}  // namespace avrtc
#endif  // BASE_CODEC_TYPE_H