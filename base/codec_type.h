#ifndef BASE_CODEC_TYPE_H
#define BASE_CODEC_TYPE_H

#include <glog/logging.h>

#include <string>

namespace avrtc {

enum class CodecType { VP8 = 96, VP9 = 98, H264 = 99, H265 = 106, Opus = 111 };
std::string CodecTypeToString(CodecType type);

}  // namespace avrtc
#endif  // BASE_CODEC_TYPE_H