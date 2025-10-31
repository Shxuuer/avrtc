#include <base/sdp.h>

namespace avrtc {

/**
 * 初始化 SDP Origin 对象，生成随机的 session id 和获取当前 IP 地址
 * 默认值:
 *  username: 当前用户名称
 *  session id: 随机生成的 32 位整数
 *  session version: 0
 *  net type: IN
 *  addr type: IP4
 *  unicast address: 当前机器的 IP 地址
 */
void SDPOrigin::Init() {
    username_ = GetUserName();
    session_id_ = std::random_device{}();
    session_version_ = 0;
    net_type_ = "IN";
    addr_type_ = "IP4";
    unicast_address_ = GetCurrentIP();
}

/**
 * 将 SDP Origin 字符串解析为 SDPOrigin 对象
 * 格式: o=<username> <session id> <session version> <net type> <addr type>
 * <unicast address>
 * 例如: o=alice 2890844526 2890842807 IN IP4 host.example.com
 * @param originString SDP Origin 字符串
 */
SDPOrigin::SDPOrigin(const std::string& originString) {
    std::stringstream ss(originString.substr(2));
    std::string segment;
    std::getline(ss, segment, ' ');
    username_ = segment;
    std::getline(ss, segment, ' ');
    session_id_ = std::stoul(segment);
    std::getline(ss, segment, ' ');
    session_version_ = static_cast<uint16_t>(std::stoul(segment));
    std::getline(ss, segment, ' ');
    net_type_ = segment;
    std::getline(ss, segment, ' ');
    addr_type_ = segment;
    std::getline(ss, segment, ' ');
    unicast_address_ = segment;
}

/**
 * 将 SDP Origin 对象转换为字符串表示形式
 * @return SDP Origin 字符串
 */
std::string SDPOrigin::ToString() const {
    std::string ret;
    ret.append("o=");
    ret.append(username_ + " ");
    ret.append(std::to_string(session_id_) + " ");
    ret.append(std::to_string(session_version_) + " ");
    ret.append(net_type_ + " ");
    ret.append(addr_type_ + " ");
    ret.append(unicast_address_ + "\n");
    return ret;
}

/**
 * 将 SDPMediaAttribute 对象转换为字符串表示形式
 * @return SDPMediaAttribute 字符串
 */
std::string SDPMediaAttribute::ToString() const {
    std::string ret = "a=";
    ret += AttributeTypeToStringList[static_cast<int>(type_)];
    ret += value_;
    ret += "\n";
    return ret;
}

/**
 * 解析媒体描述的类型、端口和协议
 * 格式: <media type> <port> <protocol> [<format> ...]
 * 例如: audio 49170 RTP/AVP
 * @param str 媒体描述字符串
 */
void SDPMediaDescription::SetTypePortProtocol(const std::string& str) {
    std::stringstream ss(str);
    std::string segment;

    std::getline(ss, segment, ' ');
    if (segment == "audio")
        media_type_ = MediaType::AUDIO;
    else if (segment == "video")
        media_type_ = MediaType::VIDEO;
    else if (segment == "application")
        media_type_ = MediaType::APPLICATION;
    else
        LOG(WARNING) << "Unknown media type: " << segment;

    std::getline(ss, segment, ' ');
    port_ = std::stoi(segment);

    std::getline(ss, segment, ' ');
    if (segment == "RTP/AVP")
        media_protocol_ = MediaProtocol::RTP_AVP;
    else if (segment == "UDP/TLS/RTP/SAVPF")
        media_protocol_ = MediaProtocol::UDP_TLS_RTP_SAVPF;
    else
        LOG(WARNING) << "Unknown media protocol: " << segment;

    while (getline(ss, segment, ' ')) {
        fmt_.push_back(static_cast<avrtc::CodecType>(std::stoi(segment)));
    }
}

/**
 * 解析媒体属性并添加到媒体描述中
 * 支持的属性类型包括: mid, direction, rtpmap, fmtp
 * 例如:
 * a=mid:1
 * a=sendrecv
 * a=rtpmap:96 VP8/90000
 * a=fmtp:96 profile-level-id=42e01f;level-asymmetry-allowed=1
 * @param str 媒体属性字符串
 */
void SDPMediaDescription::SetMediaAttributes(const std::string& str) {
    std::stringstream ss(str);
    std::string segment;

    std::getline(ss, segment, ':');
    if (segment == "mid") {
        mid_ = str.substr(4);
        media_attribute_.push_back(SDPMediaAttribute(AttributeType::MID, mid_));
    } else if (segment == "sendrecv" || segment == "sendonly" ||
               segment == "recvonly" || segment == "inactive") {
        MediaDirection direction;
        if (segment == "sendrecv")
            direction = MediaDirection::SENDRECV;
        else if (segment == "sendonly")
            direction = MediaDirection::SENDONLY;
        else if (segment == "recvonly")
            direction = MediaDirection::RECVONLY;
        else
            direction = MediaDirection::INACTIVE;
        media_attribute_.push_back(
            SDPMediaAttribute(AttributeType::DIRECTION, direction));
    } else if (segment == "rtpmap") {
        std::string value = str.substr(7);
        media_attribute_.push_back(
            SDPMediaAttribute(AttributeType::RTPMAP, value));
    } else if (segment == "fmtp") {
        std::string value = str.substr(5);
        media_attribute_.push_back(
            SDPMediaAttribute(AttributeType::FMTP, value));
    } else {
        LOG(WARNING) << "Unknown media attribute: " << str;
    }
}

/**
 * 使用媒体描述字符串初始化 SDPMediaDescription 对象
 * @param mediaBlock 媒体描述字符串
 */
SDPMediaDescription::SDPMediaDescription(const std::string& mediaBlock) {
    std::stringstream ss(mediaBlock);
    std::string segment;

    while (std::getline(ss, segment, '\n')) {
        if (segment[0] == 'm') {
            SetTypePortProtocol(segment.substr(2));
        } else {
            SetMediaAttributes(segment.substr(2));
        }
    }
}

/**
 * 使用指定的媒体类型、端口、协议和方向初始化 SDPMediaDescription 对象
 * @param mediaType 媒体类型
 * @param port 端口号
 * @param protocol 媒体协议
 * @param direction 媒体方向
 * @return SDPMediaDescription 对象
 */
SDPMediaDescription::SDPMediaDescription(MediaType mediaType,
                                         int port,
                                         MediaProtocol protocol,
                                         MediaDirection direction)
    : media_type_(mediaType), port_(port), media_protocol_(protocol) {
    media_attribute_.push_back(
        SDPMediaAttribute(AttributeType::DIRECTION, direction));
    media_attribute_.push_back(SDPMediaAttribute(AttributeType::MID, mid_));
}

/**
 * 添加编码格式到媒体描述中
 * @param codecType 编码类型
 * @param sampleRate 采样率
 */
void SDPMediaDescription::AddFormat(avrtc::CodecType codecType,
                                    int sampleRate) {
    fmt_.push_back(codecType);
    std::string codecAndSample = std::to_string(static_cast<int>(codecType)) +
                                 " " + CodecTypeToString(codecType) + "/" +
                                 std::to_string(sampleRate);
    media_attribute_.push_back(
        SDPMediaAttribute(AttributeType::RTPMAP, codecAndSample));
}

/**
 * 添加编码格式及其参数到媒体描述中
 * @param codecType 编码类型
 * @param sampleRate 采样率
 * @param args 编码参数
 */
void SDPMediaDescription::AddFormat(avrtc::CodecType codecType,
                                    int sampleRate,
                                    std::string args) {
    AddFormat(codecType, sampleRate);
    media_attribute_.push_back(SDPMediaAttribute(
        AttributeType::FMTP,
        std::to_string(static_cast<int>(codecType)) + " " + args));
}

/**
 * 从媒体描述中移除指定的编码格式
 * @param codecType 编码类型
 */
void SDPMediaDescription::RemoveFormat(avrtc::CodecType codecType) {
    fmt_.erase(std::remove(fmt_.begin(), fmt_.end(), codecType), fmt_.end());
    std::string prefix = std::to_string(static_cast<int>(codecType)) + " ";
    auto prefix_len = prefix.size();

    auto it = std::remove_if(media_attribute_.begin(), media_attribute_.end(),
                             [codecType, prefix, prefix_len](const auto& attr) {
                                 if (attr.type_ == AttributeType::RTPMAP ||
                                     attr.type_ == AttributeType::FMTP) {
                                     return attr.value_.substr(0, prefix_len) ==
                                            prefix;
                                 }
                                 return false;
                             });
    if (it == media_attribute_.end()) {
        LOG(WARNING) << "CodecType " << static_cast<int>(codecType)
                     << " not found in media attributes."
                     << "prefix: " << prefix;
    }
    media_attribute_.erase(it, media_attribute_.end());
}

/**
 * 获取指定编码类型的格式信息
 * @param codecType 编码类型
 * @return 包含格式编号和描述的对
 * @return std::pair<int, std::string> <sampleRate, codec args>
 */
std::pair<int, std::string> SDPMediaDescription::GetFormat(
    avrtc::CodecType& codecType) const {
    std::string prefix = std::to_string(static_cast<int>(codecType)) + " ";
    std::string rtpmap_prefix = "rtpmap:" + prefix;
    std::string fmtp_prefix = "fmtp:" + prefix;
    int sampleRate = 0;
    std::string args = "";

    for (const auto& attr : media_attribute_) {
        if (attr.value_.substr(0, prefix.size()) != prefix)
            continue;
        if (attr.type_ == AttributeType::RTPMAP) {
            std::string rtpmap_value = attr.value_.substr(prefix.size());
            size_t slash_pos = rtpmap_value.find('/');
            if (slash_pos != std::string::npos) {
                sampleRate = std::stoi(rtpmap_value.substr(slash_pos + 1));
            }
        } else if (attr.type_ == AttributeType::FMTP) {
            args = attr.value_.substr(prefix.size());
        }
    }
    return {sampleRate, args};
}

/**
 * 设置编码格式及其参数，若已存在则更新
 * @param codecType 编码类型
 * @param sampleRate 采样率
 * @param args 编码参数
 * @return void
 */
void SDPMediaDescription::SetFormat(avrtc::CodecType codecType,
                                    int sampleRate,
                                    const std::string& args) {
    RemoveFormat(codecType);
    if (!args.empty())
        AddFormat(codecType, sampleRate, args);
    else
        AddFormat(codecType, sampleRate);
}

/**
 * 执行媒体描述的协商，保留双方都支持的编码格式
 * @param remote_media_description 远程媒体描述对象
 * @return 协商结果，true 表示协商完成，false
 * 表示媒体类型不匹配或无共同编码格式
 */
bool SDPMediaDescription::Negotiation(
    const SDPMediaDescription& remote_media_description) {
    // 媒体类型不匹配，直接返回 false
    if (media_type_ != remote_media_description.media_type_)
        return false;
    // 协议不匹配，直接返回 false
    if (media_protocol_ != remote_media_description.media_protocol_)
        return false;
    // 任一方无编码格式，直接返回 false
    if (fmt_.empty() || remote_media_description.fmt_.empty())
        return false;
    // 找到双方都支持的编码格式
    std::vector<avrtc::CodecType> common_codecs;
    for (auto& codec : remote_media_description.fmt_) {
        if (std::find(fmt_.begin(), fmt_.end(), codec) != fmt_.end()) {
            common_codecs.push_back(codec);
        }
    }
    // 如果没有共同的编码格式，直接返回 false
    if (common_codecs.empty())
        return false;
    // mid_直接更新为远程 mid_
    mid_ = remote_media_description.mid_;
    // 删除不在 common_codecs 中的编码格式
    for (auto& codec : fmt_) {
        if (std::find(common_codecs.begin(), common_codecs.end(), codec) ==
            common_codecs.end()) {
            RemoveFormat(codec);
        }
    }
    // 更新本地 SDPHandler 对象的媒体描述
    for (auto& codec : common_codecs) {
        auto remote = remote_media_description.GetFormat(codec);
        SetFormat(codec, remote.first, remote.second);
    }
    return true;
}

/**
 * 将 SDPMediaDescription 对象转换为字符串表示形式
 * @return SDPMediaDescription 字符串
 */
std::string SDPMediaDescription::ToString() const {
    std::string ret;
    ret.append("m=" + MediaTypeToStringList[static_cast<int>(media_type_)] +
               " " + std::to_string(port_) + " " +
               MediaProtocolToStringList[static_cast<int>(media_protocol_)]);
    for (auto& fmt : fmt_) {
        ret.append(" " + std::to_string(static_cast<int>(fmt)));
    }
    ret.append("\n");

    for (auto& attribute : media_attribute_) {
        ret.append(attribute.ToString());
    }

    return ret;
}

/**
 * 默认构造函数，初始化 SDPHandler 对象
 */
SDPHandler::SDPHandler() {
    o->Init();
}

/**
 * 使用 SDP 字符串初始化 SDPHandler 对象
 * @param sdp_string SDP 字符串
 */
SDPHandler::SDPHandler(const std::string& sdp_string) {
    std::stringstream ss(sdp_string);
    std::string segment;

    while (std::getline(ss, segment, '\n')) {
        if (segment[0] == 'v') {
            v = std::stoi(segment.substr(2));
        } else if (segment[0] == 'o') {
            o = std::make_unique<SDPOrigin>(segment);
        } else if (segment[0] == 's') {
            s = segment.substr(2);
        } else if (segment[0] == 't') {
            t = segment.substr(2);
        } else if (segment[0] == 'm') {
            std::string media_block = segment + '\n';
            std::getline(ss, segment, '\n');
            while (segment[0] == 'a') {
                media_block += segment + "\n";
                if (!std::getline(ss, segment, '\n'))
                    break;
            }
            ss.seekg(-static_cast<int>(segment.size()) - 1, std::ios_base::cur);
            m.push_back(SDPMediaDescription(media_block));
        } else if (segment[0] == 'z') {
            z += segment + "\n";
        } else {
            break;
        }
    }
}

/**
 * 将 SDPHandler 对象转换为字符串表示形式
 * @return SDP 字符串
 */
std::string SDPHandler::ToString() const {
    std::string ret;
    ret.append("v=" + std::to_string(v) + "\n");
    ret.append(o->ToString());
    ret.append("s=" + s + "\n");
    ret.append("t=" + t + "\n");
    for (auto& desc : m) {
        ret.append(desc.ToString());
    }
    ret.append(z);
    return ret;
}

/**
 * 执行 SDP 协商，更新本地 SDPHandler 对象的相关字段
 * @param remote_sdp 远程 SDPHandler 对象
 * @return 协商结果，true 表示协商完成，false 表示版本不匹配或媒体描述不兼容
 */
bool SDPHandler::SDPNegotiation(SDPHandler& remote_sdp) {
    SDPHandler sdp_copy = SDPHandler(*this);
    if (v != remote_sdp.v)
        goto negotitation_fail;
    t = remote_sdp.t;

    for (auto& remote_media : remote_sdp.m) {
        bool found = false;
        for (auto& local_media : m) {
            if (local_media.Negotiation(remote_media)) {
                found = true;
                break;
            }
        }
        if (!found)
            goto negotitation_fail;
    }
    return true;

negotitation_fail:
    *this = sdp_copy;
    return false;
}

}  // namespace avrtc