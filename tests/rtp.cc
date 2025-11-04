#include "base/rtp.h"

#include <sstream>

#include "glog/logging.h"
#include "gtest/gtest.h"

TEST(RTPHandlerTest, CreateDefaultHandler) {
    auto rtp_handler = std::make_unique<avrtc::RTPHandler>();
    rtp_handler->SetVersion(2);
    rtp_handler->SetPayloadType(avrtc::CodecType::MUTE);
    EXPECT_EQ(rtp_handler->ToHumanString(),
              "80 60 00 00 00 00 00 00 00 00 00 00");
}

TEST(RTPHandlerTest, CreateCsrcHandler) {
    auto rtp_handler = std::make_unique<avrtc::RTPHandler>();
    rtp_handler->SetVersion(2);
    rtp_handler->SetPayloadType(avrtc::CodecType::MUTE);
    rtp_handler->AddCsrc(1234);
    rtp_handler->AddCsrc(5678);
    EXPECT_EQ(rtp_handler->GetCsrcCount(), 2);
    EXPECT_EQ(rtp_handler->GetCsrc(0), 1234);
    EXPECT_EQ(rtp_handler->GetCsrc(1), 5678);
    EXPECT_EQ(rtp_handler->ToHumanString(),
              "84 60 00 00 00 00 00 00 00 00 00 00 00 00 04 D2 00 00 16 2E");
}

TEST(RTPHandlerTest, Extension) {
    auto rtp_handler = std::make_unique<avrtc::RTPHandler>();
    rtp_handler->SetVersion(2);
    rtp_handler->SetPayloadType(avrtc::CodecType::MUTE);
    rtp_handler->SetExtensions(1);

    avrtc::RTPHandler::RTPHeaderExtension ext;
    ext.id = 1;
    ext.length = 4;
    ext.data = {0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF00};
    rtp_handler->AddExtension(ext);

    EXPECT_EQ(rtp_handler->ToHumanString(),
              "81 60 00 00 00 00 00 00 00 00 00 00 BE DE 00 06 00 01 00 04 "
              "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00");
}

TEST(RTPHandlerTest, PayloadExtension) {
    auto rtp_handler = std::make_unique<avrtc::RTPHandler>();
    rtp_handler->SetVersion(2);
    rtp_handler->SetPayloadType(avrtc::CodecType::MUTE);
    rtp_handler->SetExtensions(1);

    avrtc::RTPHandler::RTPHeaderExtension ext;
    ext.id = 1;
    ext.length = 4;
    ext.data = {0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF00};
    rtp_handler->AddExtension(ext);

    auto payload_int =
        std::vector<int>{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                         0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00};
    std::vector<char> payload;
    for (const auto& byte : payload_int) {
        payload.push_back(static_cast<char>(byte));
    }
    rtp_handler->SetPayload(payload);

    EXPECT_EQ(rtp_handler->ToHumanString(),
              "81 60 00 00 00 00 00 00 00 00 00 00 BE DE 00 06 00 01 00 04 "
              "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00 11 22 33 44 "
              "55 66 77 88 99 AA BB CC DD EE FF 00");
}

TEST(RTPHandlerTest, CreateHandlerFromPacket) {
    auto rtp_handler = std::make_unique<avrtc::RTPHandler>();
    rtp_handler->SetVersion(2);
    rtp_handler->SetPayloadType(avrtc::CodecType::MUTE);
    rtp_handler->SetExtensions(1);
    rtp_handler->AddCsrc(1234);
    rtp_handler->AddCsrc(5678);

    avrtc::RTPHandler::RTPHeaderExtension ext;
    ext.id = 1;
    ext.length = 4;
    ext.data = {0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF00};
    rtp_handler->AddExtension(ext);

    auto payload_int =
        std::vector<int>{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                         0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00};
    std::vector<char> payload;
    for (const auto& byte : payload_int) {
        payload.push_back(static_cast<char>(byte));
    }
    rtp_handler->SetPayload(payload);

    auto rtp_packet = rtp_handler->GetRTPPacket();
    auto new_rtp_handler = std::make_unique<avrtc::RTPHandler>(rtp_packet);
    EXPECT_EQ(new_rtp_handler->ToHumanString(), rtp_handler->ToHumanString());
}