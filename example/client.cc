#include "example/client.h"

#include "example/ui.h"

AvrtcClient::AvrtcClient(avrtc::SocketAddress address,
                         std::shared_ptr<ClientUI> ui)
    : ClientSocket(address), client_ui_(ui) {
    SetOnConnected([this]() { this->OnConnected(); });
    SetOnReceive([this](std::shared_ptr<SessionSocket>, char* buffer,
                        size_t length) { this->OnReceive(buffer, length); });
}

void AvrtcClient::OnConnected() {
    LOG(INFO) << "Connected to server";

    // 发送初始SDP消息
    sdp_handler_ = std::make_shared<avrtc::SDPHandler>();
    sdp_handler_->z.push_back("Add");
    sdp_handler_->z.push_back(
        avrtc::ClientType::kSender == avrtc::ClientType::kSender ? "Sender"
                                                                 : "Receiver");
    Send(sdp_handler_->ToString());
    sdp_handler_->z.clear();
}

void AvrtcClient::OnReceive(char* buffer, size_t length) {
    std::string msg(buffer, length);
    LOG(INFO) << "Received data: " << msg;
    auto sdp_msg = std::make_shared<avrtc::SDPHandler>(msg);
    std::string response_type = sdp_msg->z[0];
    if (response_type == "Add") {
        id_ = std::stoi(sdp_msg->z[1]);
        LOG(INFO) << "Assigned ID: " << id_;
    }

    // sdp_handler_ = std::make_shared<avrtc::SDPHandler>();
    // sdp_handler_->z.push_back("Close");
    // sdp_handler_->z.push_back("Sender");
    // sdp_handler_->z.push_back(std::to_string(id_));
    // Send(sdp_handler_->ToString());
}

int main(int argc, char** argv) {
    FLAGS_alsologtostderr = 1;
    google::InitGoogleLogging(argv[0]);

    auto app = Gtk::Application::create();

    auto client_ui = std::make_shared<ClientUI>();
    std::shared_ptr<AvrtcClient> client = std::make_shared<AvrtcClient>(
        avrtc::SocketAddress("127.0.0.1", 14562), client_ui);
    client_ui->SetClient(client);

    app->run(*client_ui);

    return 0;
}