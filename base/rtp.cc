#include "base/rtp.h"

namespace avrtc {

/**
 * 构造一个默认的 RTP 包
 */
RTPHandler::RTPHandler() {
    Reset();
}

/**
 * 从RTP包数据构造 RTPHandler 对象
 */
RTPHandler::RTPHandler(const std::vector<char>& rtp_packet) {
    size_t offset = 0;

    // header without csrcs
    size_t header_without_csrcs_size = sizeof(RTPFixedHeader);
    if (rtp_packet.size() - offset < header_without_csrcs_size) {
        LOG(ERROR) << "Invalid RTP packet: too small";
        return;
    }
    memcpy(&packet_.header.fixed, rtp_packet.data() + offset,
           header_without_csrcs_size);
    offset += header_without_csrcs_size;

    // csrcs
    size_t csrcs_size = packet_.header.fixed.cc * 4;
    if (rtp_packet.size() - offset < csrcs_size) {
        LOG(ERROR) << "Invalid RTP packet: insufficient CSRCs data";
        return;
    }
    packet_.header.csrc.resize(packet_.header.fixed.cc);
    memcpy(packet_.header.csrc.data(), rtp_packet.data() + offset, csrcs_size);
    offset += csrcs_size;

    // extensions
    if (packet_.header.fixed.extensions) {
        if (rtp_packet.size() - offset < 4) {
            LOG(ERROR)
                << "Invalid RTP packet: insufficient extension header data";
            return;
        }

        uint16_t extension_id = 0;
        memcpy(&extension_id, rtp_packet.data() + offset, sizeof(extension_id));
        extension_id = ntohs(extension_id);
        if (extension_id != 0xBEDE) {
            LOG(ERROR) << "RTP header extension ID is not 0xBEDE";
            return;
        }
        offset += sizeof(extension_id);

        size_t extension_length = 0;  // in 32-bit words
        memcpy(&extension_length, rtp_packet.data() + offset, sizeof(uint16_t));
        extension_length = ntohs(extension_length);
        if (extension_length < 0) {
            LOG(ERROR) << "Invalid RTP packet: negative extension length";
            return;
        }
        offset += sizeof(uint16_t);

        for (size_t i = 4; i < extension_length; ++i) {
            RTPHeaderExtension ext;
            if (rtp_packet.size() - offset < 4) {
                LOG(ERROR) << "Invalid RTP packet: insufficient extension data";
                return;
            }

            memcpy(&ext.id, rtp_packet.data() + offset, 2);
            offset += 2;

            memcpy(&ext.length, rtp_packet.data() + offset, 2);
            size_t length = ntohs(ext.length);
            offset += 2;

            if (rtp_packet.size() - offset < length) {
                LOG(ERROR) << "Invalid RTP packet: insufficient extension "
                              "content data";
                return;
            }
            ext.data.resize(length);
            memcpy(ext.data.data(), rtp_packet.data() + offset, length * 4);
            offset += length * 4;
            i += length / 4;
            packet_.extensions.push_back(ext);
        }
    }

    // payload
    if (rtp_packet.size() - offset <= 0) {
        LOG(ERROR) << "Invalid RTP packet: insufficient payload data";
        return;
    }
    packet_.payload.resize(rtp_packet.size() - offset);
    memcpy(packet_.payload.data(), rtp_packet.data() + offset,
           packet_.payload.size());
    offset += packet_.payload.size();

    // padding
    if (packet_.header.fixed.padding) {
        if (packet_.payload.empty()) {
            LOG(ERROR) << "RTP padding set but payload is empty";
            packet_.payload.clear();
            return;
        }
        size_t padding_size = static_cast<uint8_t>(packet_.payload.back());
        if (padding_size == 0 || padding_size > packet_.payload.size()) {
            LOG(ERROR) << "Invalid RTP padding size: " << padding_size;
            packet_.payload.clear();
            return;
        }
        packet_.payload.resize(packet_.payload.size() - padding_size);
    }
}

/**
 * 重置 RTP 包信息
 */
void RTPHandler::Reset() {
    // header
    packet_.header.fixed.version = 2;
    packet_.header.fixed.padding = 0;
    packet_.header.fixed.extensions = 0;
    packet_.header.fixed.cc = 0;
    packet_.header.fixed.marker = 0;
    packet_.header.fixed.payload_type = 0;
    packet_.header.fixed.sequence_number = 0;
    packet_.header.fixed.timestamp = 0;
    packet_.header.fixed.ssrc = 0;
    packet_.header.csrc.clear();

    // extension
    packet_.extensions.clear();

    // payload
    packet_.payload.clear();
}

void RTPHandler::SetTimestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint32_t timestamp =
        static_cast<uint32_t>(ts.tv_sec * 90000 + (ts.tv_nsec * 9) / 100000);
    SetTimestamp(timestamp);
}

/**
 * 获取 RTP 包的二进制数据
 */
std::vector<char> RTPHandler::GetRTPPacket() {
    size_t header_without_csrcs_size = sizeof(RTPFixedHeader);
    size_t csrcs_size = packet_.header.fixed.cc * 4;
    size_t extension_size = 0;
    if (packet_.header.fixed.extensions) {
        extension_size = 4;
        for (const auto& ext : packet_.extensions) {
            extension_size += 4 + ext.data.size() * 4;
        }
    }
    size_t payload_size = packet_.payload.size();
    int offset = 0;

    auto packet = std::vector<char>(header_without_csrcs_size + csrcs_size +
                                    extension_size + payload_size);

    memcpy(packet.data() + offset, &packet_.header.fixed,
           header_without_csrcs_size);
    offset += header_without_csrcs_size;

    memcpy(packet.data() + offset, packet_.header.csrc.data(), csrcs_size);
    offset += csrcs_size;

    if (extension_size != 0) {
        if (extension_size / 4 - 1 > 0xFFFF) {
            LOG(WARNING) << "RTP header extension size too large";
            return packet;
        }
        uint32_t extension_header = 0xBEDE << 16 | extension_size / 4;
        extension_header = htonl(extension_header);

        memcpy(packet.data() + offset, &extension_header, 4);
        offset += 4;

        for (const auto& ext : packet_.extensions) {
            memcpy(packet.data() + offset, &ext.id, 2);
            uint16_t ext_length = htons(static_cast<uint16_t>(ext.data.size()));
            memcpy(packet.data() + offset + 2, &ext_length, 2);
            memcpy(packet.data() + offset + 4, ext.data.data(),
                   ext.data.size() * 4);
            offset += 4 + ext.data.size() * 4;
        }
    }
    memcpy(packet.data() + offset, packet_.payload.data(),
           packet_.payload.size());
    return packet;
}

std::string RTPHandler::ToHumanString() {
    std::string info = "";
    std::vector<char> rtp_packet = GetRTPPacket();
    for (size_t i = 0; i < rtp_packet.size(); ++i) {
        char buffer[4];
        snprintf(buffer, sizeof(buffer), "%02X ",
                 static_cast<uint8_t>(rtp_packet[i]));
        info += buffer;
    }

    return info.substr(0, info.size() - 1);
}

}  // namespace avrtc