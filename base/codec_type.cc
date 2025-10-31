#include "base/codec_type.h"

namespace avrtc {
std::string CodecTypeToString(CodecType type) {
    switch (type) {
        case CodecType::VP8:
            return "VP8";
        case CodecType::VP9:
            return "VP9";
        case CodecType::H264:
            return "H264";
        case CodecType::H265:
            return "H265";
        case CodecType::Opus:
            return "Opus";
        default:
            LOG(FATAL) << "Unkown Codec.";
    }
    return "";
}
}