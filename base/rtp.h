#ifndef BASE_RTP_H
#define BASE_RTP_H

#include <glog/logging.h>

#include <cstdint>

#include "base/codec_type.h"

namespace avrtc {

class RTPHandler {
  const static int kRtpCsrcSize = 5;

 public:
  RTPHandler();
  void Reset();

  void SetVersion(uint8_t version) { header_.version = version; }
  uint8_t GetVersion() const { return header_.version; }

  void SetPadding(uint8_t padding) { header_.padding = padding; }
  uint8_t GetPadding() const { return header_.padding; }

  void SetExtensions(uint8_t extensions) { header_.extensions = extensions; }
  uint8_t GetExtensions() const { return header_.extensions; }

  void SetCsrcCount(uint8_t cc) { header_.cc = cc; }
  uint8_t GetCsrcCount() const { return header_.cc; }

  void SetMarker(uint8_t marker) { header_.marker = marker; }
  uint8_t GetMarker() const { return header_.marker; }

  void SetPayloadType(CodecType payload_type) {
    header_.payload_type = static_cast<uint8_t>(payload_type);
  }
  CodecType GetPayloadType() const {
    return static_cast<CodecType>(header_.payload_type);
  }

  void SetSequenceNumber(uint16_t sequence_number) {
    header_.sequence_number = sequence_number;
  }
  uint16_t GetSequenceNumber() const { return header_.sequence_number; }

  void SetTimestamp(uint32_t timestamp) { header_.timestamp = timestamp; }
  uint32_t GetTimestamp() const { return header_.timestamp; }

  void SetSsrc(uint32_t ssrc) { header_.ssrc = ssrc; }
  uint32_t GetSsrc() const { return header_.ssrc; }

  void SetPayload(std::shared_ptr<uint8_t[]> payload, size_t payload_size) {
    packet_.payload = payload;
    packet_.payload_size = payload_size;
  }
  std::shared_ptr<uint8_t[]> GetPayload() const { return packet_.payload; }

  struct RTPHeader {
    uint8_t version : 2;
    uint8_t padding : 1;
    uint8_t extensions : 1;
    uint8_t cc : 4;
    uint8_t marker : 1;
    uint8_t payload_type : 7;
    uint16_t sequence_number;
    uint32_t timestamp;
    uint32_t ssrc;
    uint32_t csrc[kRtpCsrcSize];
  };

  // rtp packet
  struct RTPPacket {
    RTPHeader header;
    std::shared_ptr<uint8_t[]> payload;
    size_t payload_size;
  };

  // rtp header extensions
  struct RTPHeaderExtension {
    uint16_t id;
    size_t length;
    std::shared_ptr<uint8_t[]> data;
  };

  using RTPHeader = struct RTPHeader;
  using RTPPacket = struct RTPPacket;
  using RTPHeaderExtension = struct RTPHeaderExtension;

  RTPPacket packet_;
  RTPHeader& header_ = packet_.header;
};

}  // namespace avrtc

#endif  // BASE_RTP_H