#ifndef BASE_RTP_H
#define BASE_RTP_H

#include <arpa/inet.h>
#include <glog/logging.h>
#include <time.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "base/codec_type.h"

namespace avrtc {

class RTPHandler {
  const static int kRtpCsrcMaxSize = 15;

 public:
  RTPHandler();
  RTPHandler(const std::vector<char>& rtp_packet);

  void Reset();
  void SetTimestamp();
  std::vector<char> GetRTPPacket();
  std::string ToHumanString();

  struct RTPFixedHeader {
    uint8_t extensions : 1;
    uint8_t cc : 4;
    uint8_t padding : 1;
    uint8_t version : 2;

    uint8_t marker : 1;
    uint8_t payload_type : 7;

    uint16_t sequence_number;
    uint32_t timestamp;
    uint32_t ssrc;
  };

  struct RTPHeader {
    RTPFixedHeader fixed;
    std::vector<uint32_t> csrc;
  };

  // rtp header extensions
  struct RTPHeaderExtension {
    uint16_t id;
    uint16_t length;  // in word
    std::vector<uint32_t> data;
  };

  // rtp packet
  struct RTPPacket {
    RTPHeader header;
    std::vector<RTPHeaderExtension> extensions;
    std::vector<char> payload;
  };

  void SetVersion(uint8_t version) { packet_.header.fixed.version = version; }
  uint8_t GetVersion() const { return packet_.header.fixed.version; }

  void SetPadding(uint8_t padding) { packet_.header.fixed.padding = padding; }
  uint8_t GetPadding() const { return packet_.header.fixed.padding; }

  void SetExtensions(uint8_t extensions) {
    packet_.header.fixed.extensions = extensions;
  }
  uint8_t GetExtensions() const { return packet_.header.fixed.extensions; }

  void SetMarker(uint8_t marker) { packet_.header.fixed.marker = marker; }
  uint8_t GetMarker() const { return packet_.header.fixed.marker; }

  void SetPayloadType(CodecType payload_type) {
    int type = static_cast<int>(payload_type);
    if (type > 127) {
      LOG(WARNING) << "Invalid payload type, must be in range 0-127";
      return;
    }
    packet_.header.fixed.payload_type = type;
  }
  CodecType GetPayloadType() const {
    return static_cast<CodecType>(packet_.header.fixed.payload_type);
  }

  void AddSequenceNumber() {
    uint16_t seq = GetSequenceNumber() + 1;
    packet_.header.fixed.sequence_number = htons(seq);
  }
  uint16_t GetSequenceNumber() const {
    return ntohs(packet_.header.fixed.sequence_number);
  }

  void SetTimestamp(uint32_t timestamp) {
    packet_.header.fixed.timestamp = htonl(timestamp);
  }
  uint32_t GetTimestamp() const {
    return ntohl(packet_.header.fixed.timestamp);
  }

  void SetSsrc(uint32_t ssrc) { packet_.header.fixed.ssrc = htonl(ssrc); }
  uint32_t GetSsrc() const { return ntohl(packet_.header.fixed.ssrc); }

  void AddCsrc(uint32_t csrc) {
    if (packet_.header.fixed.cc < kRtpCsrcMaxSize) {
      packet_.header.csrc.push_back(htonl(csrc));
      packet_.header.fixed.cc++;
    } else {
      LOG(WARNING) << "Cannot add more CSRCs, maximum size reached";
    }
  }
  uint8_t GetCsrcCount() const { return packet_.header.fixed.cc; }
  uint32_t GetCsrc(uint8_t index) const {
    if (index < packet_.header.fixed.cc) {
      return ntohl(packet_.header.csrc[index]);
    }
    return 0;
  }

  void AddExtension(RTPHeaderExtension ext) {
    ext.id = htons(ext.id);
    ext.length = htons(ext.length);
    for (auto& d : ext.data) {
      d = htonl(d);
    }
    packet_.extensions.push_back(ext);
    packet_.header.fixed.extensions = 1;
  }
  std::unique_ptr<RTPHeaderExtension> GetExtension(int index) const {
    if (index < static_cast<int>(packet_.extensions.size())) {
      auto ext = std::make_unique<RTPHeaderExtension>();
      ext->id = ntohs(packet_.extensions[index].id);
      ext->length = ntohs(packet_.extensions[index].length);
      for (const auto& d : packet_.extensions[index].data) {
        ext->data.push_back(ntohl(d));
      }
      return ext;
    }
    return nullptr;
  }

  void SetPayload(std::vector<char> payload) { packet_.payload = payload; }
  std::vector<char> GetPayload() const { return packet_.payload; }

 private:
  RTPPacket packet_;
};

}  // namespace avrtc

#endif  // BASE_RTP_H