
#include <glog/logging.h>

#include <iostream>
#include <string>

#include "base/socket.h"
#include "base/thread.h"
#include "example/ui.h"

class AvrtcClient : public avrtc::ClientSocket {
   public:
    AvrtcClient(avrtc::SocketAddress address) : ClientSocket(address) {
        SetOnReceive(
            [this](std::shared_ptr<SessionSocket>, char* buffer,
                   size_t length) { this->OnReceive(buffer, length); });
    }

    void OnReceive(char* buffer, size_t length) {
        std::string msg(buffer, length);
        LOG(INFO) << "Received data: " << msg;
        auto parsed_msgs = avrtc::ParseMessage(msg);
        std::string response_type = parsed_msgs["Response"][0];
        if (response_type == "Add") {
            id_ = std::stoi(parsed_msgs["ID"][0]);
            LOG(INFO) << "Assigned ID: " << id_;
        }

        // int i = 0;
        // while (i < 1000000000)
        //   i++;
        Stop();
        Close();
        // Send("Request:Close\nID:" + std::to_string(id_) + "\nType:Sender\n");
    }

   private:
    int id_ = -1;
};

int main(int argc, char** argv) {
    FLAGS_alsologtostderr = 1;
    google::InitGoogleLogging(argv[0]);
    // auto app = Gtk::Application::create();
    // ClientUI client_ui;
    // app->run(client_ui);

    std::unique_ptr<avrtc::Thread> socketThread =
        std::make_unique<avrtc::Thread>();
    std::shared_ptr<AvrtcClient> client =
        std::make_shared<AvrtcClient>(avrtc::SocketAddress("127.0.0.1", 14562));
    socketThread->AddTask([client]() { client->Connect(); });

    int i = 0;
    while (i < 100000000)
        i++;
    client->Send("Request:Add\nType:Sender\nName:" + avrtc::GetClientName() +
                 "\n");
    socketThread->Join();
    return 0;
}