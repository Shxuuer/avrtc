#include "base/rtp.h"

namespace avrtc {

RTPHandler::RTPHandler() {
    Reset();
}

/**
 * 重置 RTP 包信息
 */
void RTPHandler::Reset() {
    RTPHeader& header = packet_.header;
    header.version = 2;
    header.padding = 0;
    header.extensions = 0;
    header.cc = 0;
    header.marker = 0;
    header.payload_type = 0;
    header.sequence_number = 0;
    header.timestamp = 0;
    header.ssrc = 0;
    for (int i = 0; i < kRtpCsrcSize; ++i) {
        header.csrc[i] = 0;
    }

    packet_.payload = nullptr;
    packet_.payload_size = 0;
}

}  // namespace avrtc