#include "base/codec_type.h"

namespace avrtc {

std::string CodecTypeToString(CodecType type) {
    auto it = codec_type_to_string.find(type);
    if (it != codec_type_to_string.end()) {
        return it->second;
    }
    return "";
}

CodecType StringToCodecType(const std::string& str) {
    auto it = string_to_codec_type.find(str);
    if (it != string_to_codec_type.end()) {
        return it->second;
    }
    LOG(ERROR) << "StringToCodecType: Invalid codec type string: " << str;
    return CodecType::MUTE;  // Default to MUTE on error
}

AVCodecID CodecTypeToFFmpegCodecID(CodecType type) {
    auto it = codec_type_to_ffmpeg_id.find(type);
    if (it != codec_type_to_ffmpeg_id.end()) {
        return it->second;
    }
    LOG(ERROR) << "CodecTypeToFFmpegCodecID: Invalid codec type.";
    return AV_CODEC_ID_NONE;  // Default to NONE on error
}

}  // namespace avrtc