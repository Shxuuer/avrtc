#include "base/sdp.h"

#include "glog/logging.h"
#include "gtest/gtest.h"

TEST(SDPHandlerTest, CreateDefaultHandler) {
    auto sdp_handler = std::make_unique<avrtc::SDPHandler>();
    EXPECT_NE(sdp_handler, nullptr);
    auto sdp_media_description = avrtc::SDPMediaDescription(
        avrtc::MediaType::VIDEO, 5004, avrtc::MediaProtocol::RTP_AVP,
        avrtc::MediaDirection::SENDRECV);
    sdp_media_description.AddFormat(avrtc::CodecType::H264, 8000,
                                    "profile-level-id=42e01f;level-asymmetry-"
                                    "allowed=1;packetization-mode=1");
    sdp_media_description.AddFormat(
        avrtc::CodecType::H265, 8000,
        "profile-level-id=1;level-asymmetry-allowed=1");

    sdp_handler->m.push_back(sdp_media_description);
    sdp_handler->m.push_back(sdp_media_description);

    std::string expected_sdp =
        "v=0\n"
        "o=" +
        sdp_handler->o->username_ + " " +
        std::to_string(sdp_handler->o->session_id_) + " " +
        std::to_string(sdp_handler->o->session_version_) + " IN IP4 " +
        sdp_handler->o->unicast_address_ +
        "\n"
        "s=-\n"
        "t=0 0\n"
        "m=video 5004 RTP/AVP 99 106\n"
        "a=sendrecv\n"
        "a=mid:" +
        sdp_media_description.mid_ +
        "\n"
        "a=rtpmap:99 H264/8000\n"
        "a=fmtp:99 profile-level-id=42e01f;level-asymmetry-allowed=1;"
        "packetization-mode=1\n"
        "a=rtpmap:106 H265/8000\n"
        "a=fmtp:106 profile-level-id=1;level-asymmetry-allowed=1\n"
        "m=video 5004 RTP/AVP 99 106\n"
        "a=sendrecv\n"
        "a=mid:" +
        sdp_media_description.mid_ +
        "\n"
        "a=rtpmap:99 H264/8000\n"
        "a=fmtp:99 profile-level-id=42e01f;level-asymmetry-allowed=1;"
        "packetization-mode=1\n"
        "a=rtpmap:106 H265/8000\n"
        "a=fmtp:106 profile-level-id=1;level-asymmetry-allowed=1\n";
    EXPECT_EQ(sdp_handler->ToString(), expected_sdp);

    sdp_handler->m[0].RemoveFormat(avrtc::CodecType::H264);
    expected_sdp =
        "v=0\n"
        "o=" +
        sdp_handler->o->username_ + " " +
        std::to_string(sdp_handler->o->session_id_) + " " +
        std::to_string(sdp_handler->o->session_version_) + " IN IP4 " +
        sdp_handler->o->unicast_address_ +
        "\n"
        "s=-\n"
        "t=0 0\n"
        "m=video 5004 RTP/AVP 106\n"
        "a=sendrecv\n"
        "a=mid:" +
        sdp_media_description.mid_ +
        "\n"
        "a=rtpmap:106 H265/8000\n"
        "a=fmtp:106 profile-level-id=1;level-asymmetry-allowed=1\n"
        "m=video 5004 RTP/AVP 99 106\n"
        "a=sendrecv\n"
        "a=mid:" +
        sdp_media_description.mid_ +
        "\n"
        "a=rtpmap:99 H264/8000\n"
        "a=fmtp:99 profile-level-id=42e01f;level-asymmetry-allowed=1;"
        "packetization-mode=1\n"
        "a=rtpmap:106 H265/8000\n"
        "a=fmtp:106 profile-level-id=1;level-asymmetry-allowed=1\n";

    EXPECT_EQ(sdp_handler->ToString(), expected_sdp);
}

TEST(SDPHandlerTest, CreateHandlerFromString) {
    auto sdp_handler = std::make_unique<avrtc::SDPHandler>();
    EXPECT_NE(sdp_handler, nullptr);
    auto sdp_media_description = avrtc::SDPMediaDescription(
        avrtc::MediaType::VIDEO, 5004, avrtc::MediaProtocol::RTP_AVP,
        avrtc::MediaDirection::SENDRECV);
    sdp_media_description.AddFormat(avrtc::CodecType::H264, 8000,
                                    "profile-level-id=42e01f;level-asymmetry-"
                                    "allowed=1;packetization-mode=1");
    sdp_media_description.AddFormat(
        avrtc::CodecType::H265, 8000,
        "profile-level-id=1;level-asymmetry-allowed=1");

    sdp_handler->m.push_back(sdp_media_description);
    sdp_handler->m.push_back(sdp_media_description);

    std::string sdp_string = sdp_handler->ToString();
    auto parsed_sdp_handler = std::make_unique<avrtc::SDPHandler>(sdp_string);
    EXPECT_EQ(parsed_sdp_handler->ToString(), sdp_string);
}

TEST(SDPHandlerTest, Negotiation) {
    auto local_sdp_handler = std::make_unique<avrtc::SDPHandler>();
    EXPECT_NE(local_sdp_handler, nullptr);
    auto local_media_description = avrtc::SDPMediaDescription(
        avrtc::MediaType::VIDEO, 5004, avrtc::MediaProtocol::RTP_AVP,
        avrtc::MediaDirection::SENDRECV);
    local_media_description.AddFormat(avrtc::CodecType::H264, 8000);
    local_media_description.AddFormat(avrtc::CodecType::H265, 8000);
    local_sdp_handler->m.push_back(local_media_description);
    LOG(INFO) << "Local SDP before negotiation:\n"
              << local_sdp_handler->ToString();

    auto remote_sdp_handler = std::make_unique<avrtc::SDPHandler>();
    EXPECT_NE(remote_sdp_handler, nullptr);
    auto remote_media_description = avrtc::SDPMediaDescription(
        avrtc::MediaType::VIDEO, 6004, avrtc::MediaProtocol::RTP_AVP,
        avrtc::MediaDirection::SENDRECV);
    remote_media_description.AddFormat(
        avrtc::CodecType::H264, 8000,
        "profile-level-id=42e01f;level-asymmetry-"
        "allowed=1;packetization-mode=2");
    remote_media_description.AddFormat(avrtc::CodecType::VP9, 90000);
    remote_sdp_handler->m.push_back(remote_media_description);
    LOG(INFO) << "Remote SDP before negotiation:\n"
              << remote_sdp_handler->ToString();

    bool negotiation_result =
        local_sdp_handler->SDPNegotiation(*remote_sdp_handler);
    EXPECT_TRUE(negotiation_result);
    LOG(INFO) << "Local SDP after negotiation:\n"
              << local_sdp_handler->ToString();

    auto remote_media_description_audio = avrtc::SDPMediaDescription(
        avrtc::MediaType::AUDIO, 5003, avrtc::MediaProtocol::RTP_AVP,
        avrtc::MediaDirection::SENDRECV);
    remote_media_description_audio.AddFormat(avrtc::CodecType::Opus, 48000);
    remote_sdp_handler->m.push_back(remote_media_description_audio);
    EXPECT_FALSE(local_sdp_handler->SDPNegotiation(*remote_sdp_handler));

    local_sdp_handler->m.push_back(remote_media_description_audio);
    EXPECT_TRUE(local_sdp_handler->SDPNegotiation(*remote_sdp_handler));
    LOG(INFO) << "Local SDP after negotiation with audio:\n"
              << local_sdp_handler->ToString();
}