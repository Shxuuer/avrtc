#ifndef BASE_SDP_H
#define BASE_SDP_H

#include <glog/logging.h>

#include <algorithm>
#include <deque>
#include <random>
#include <string>
#include <vector>

#include "base/codec_type.h"
#include "base/utils.h"

namespace avrtc {

enum class MediaDirection { SENDRECV, SENDONLY, RECVONLY, INACTIVE };
static const std::vector<std::string> MediaDirectionToStringList = {
    "sendrecv", "sendonly", "recvonly", "inactive"};

enum class AttributeType { RTPMAP, FMTP, DIRECTION, MID };
static const std::vector<std::string> AttributeTypeToStringList = {
    "rtpmap:", "fmtp:", "", "mid:"};

enum class MediaType { AUDIO, VIDEO, APPLICATION };
const std::vector<std::string> MediaTypeToStringList = {
    "audio", "video", "application"};

enum class MediaProtocol { RTP_AVP, UDP_TLS_RTP_SAVPF };
const std::vector<std::string> MediaProtocolToStringList = {
    "RTP/AVP", "UDP/TLS/RTP/SAVPF"};

class SDPOrigin {
 public:
  SDPOrigin() = default;
  SDPOrigin(const std::string& originString);
  void Init();

  std::string ToString() const;
  void SessionIdIncrement() { ++session_version_; }

 public:
  std::string username_;
  uint32_t session_id_;
  uint16_t session_version_;
  std::string net_type_;
  std::string addr_type_;
  std::string unicast_address_;
};

class SDPMediaAttribute {
 public:
  SDPMediaAttribute() = delete;

  SDPMediaAttribute(AttributeType type, const std::string& value)
      : type_(type), value_(value) {}

  SDPMediaAttribute(AttributeType type, MediaDirection direction)
      : type_(type),
        value_(MediaDirectionToStringList[static_cast<int>(direction)]) {
    CHECK(type == AttributeType::DIRECTION)
        << "use MediaDirection to make SDPMediaAttribute class must "
           "have type DIRECTION";
  }

  std::string ToString() const;

 public:
  AttributeType type_;
  std::string value_;
};  // namespace avrtc

class SDPMediaDescription {
 public:
  SDPMediaDescription() = delete;

  SDPMediaDescription(const std::string& mediaBlock);
  void SetTypePortProtocol(const std::string& str);
  void SetMediaAttributes(const std::string& str);

  SDPMediaDescription(MediaType mediaType,
                      int port,
                      MediaProtocol protocol,
                      MediaDirection direction);

  void AddFormat(avrtc::CodecType codecType, int sampleRate);
  void AddFormat(avrtc::CodecType codecType, int sampleRate, std::string args);
  void SetFormat(avrtc::CodecType codecType,
                 int sampleRate,
                 const std::string& args);
  std::pair<int, std::string> GetFormat(avrtc::CodecType& codecType) const;
  void RemoveFormat(avrtc::CodecType codecType);

  bool Negotiation(const SDPMediaDescription& remote_media_description);

  std::string ToString() const;

 public:
  MediaType media_type_;
  int port_;
  MediaProtocol media_protocol_;
  std::string mid_ = std::to_string(std::random_device{}());
  std::vector<avrtc::CodecType> fmt_;
  std::deque<SDPMediaAttribute> media_attribute_;
};

class SDPHandler {
 public:
  SDPHandler();
  SDPHandler(const std::string& sdp_string);

  std::string ToString() const;
  bool SDPNegotiation(SDPHandler& remote_sdp);

 public:
  int v = 0;                                                     // version
  std::shared_ptr<SDPOrigin> o = std::make_shared<SDPOrigin>();  // origin
  std::string s = "-";                                           // session name
  std::string t = "0 0";                                         // timing
  std::vector<SDPMediaDescription> m;  // media descriptions
  std::string z;                       // other attributes can be added here
};

}  // namespace avrtc

#endif  // BASE_SDP_H