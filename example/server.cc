#include <base/socket.h>
#include <base/thread.h>
#include <base/utils.h>
#include <example/ui.h>
#include <glog/logging.h>
#include <gtkmm/application.h>
#include <signal.h>

#include <algorithm>
#include <iostream>
#include <vector>

#include "base/sdp.h"

class AvrtcServer : public avrtc::ServerSocket {
   public:
    AvrtcServer(avrtc::SocketAddress address, std::shared_ptr<ServerUI> ui)
        : ServerSocket(address), server_ui_(ui) {
        SetOnAccept([this](std::shared_ptr<avrtc::SessionSocket> socket) {
            this->OnAccept(socket);
        });
    }

    void OnAccept(std::shared_ptr<avrtc::SessionSocket> socket) {
        socket->SetOnReceive(
            [this](std::shared_ptr<avrtc::SessionSocket> socket, char* buffer,
                   size_t length) { this->OnReceive(socket, buffer, length); });

        socket->SetOnClose(
            [this](std::shared_ptr<avrtc::SessionSocket> socket) {
                this->OnClose(socket);
            });
    }

    void AddMessageHandler(avrtc::MessageMap msg,
                           std::shared_ptr<avrtc::SessionSocket> socket) {
        std::string name = msg["Name"][0];
        ++this->current_sender_id_;
        if (msg["Type"][0] == "Sender") {
            server_ui_->PushTask([this, name]() {
                this->server_ui_->AddSender(name, this->current_sender_id_);
            });
            senders_.insert({current_sender_id_, socket});
        } else if (msg["Type"][0] == "Receiver") {
            server_ui_->PushTask([this, name]() {
                this->server_ui_->AddReceiver(name, this->current_receiver_id_);
            });
            receivers_.insert({current_receiver_id_, socket});
        } else
            return;
        server_ui_->Emit();
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "Response:Add\nID:%d\n",
                 current_sender_id_);
        socket->Send(buffer, strlen(buffer));
    }

    void CloseMessageHandler(avrtc::MessageMap msg,
                             std::shared_ptr<avrtc::SessionSocket> socket) {
        int id = std::stoi(msg["ID"][0]);

        if (msg["Type"][0] == "Sender") {
            LOG(INFO) << "Removing sender with ID: " << id;
            server_ui_->PushTask(
                [this, id]() { this->server_ui_->RemoveSender(id); });
            senders_.erase(id);
        } else if (msg["Type"][0] == "Receiver") {
            server_ui_->PushTask(
                [this, id]() { this->server_ui_->RemoveReceiver(id); });
            receivers_.erase(id);
        } else {
            return;
        }
        server_ui_->Emit();
    }

    void OnReceive(std::shared_ptr<avrtc::SessionSocket> socket,
                   char* buffer,
                   size_t length) {
        std::string msg(buffer, length);
        LOG(INFO) << "Received data:\n" << msg;
        auto parsed_msgs = avrtc::ParseMessage(msg);

        std::string request_type = parsed_msgs["Request"][0];
        if (request_type == "Add")
            AddMessageHandler(parsed_msgs, socket);
        else if (request_type == "Close")
            CloseMessageHandler(parsed_msgs, socket);
    }

    void OnClose(std::shared_ptr<avrtc::SessionSocket> socket) {
        // 在列表中找出socket对应的ID
        int id;
        std::string type;
        auto it = std::find_if(
            senders_.begin(), senders_.end(),
            [socket](const auto& pair) { return pair.second == socket; });
        if (it != senders_.end()) {
            id = it->first;
            type = "Sender";
            goto close;
        }
        it = std::find_if(
            receivers_.begin(), receivers_.end(),
            [socket](const auto& pair) { return pair.second == socket; });
        if (it != receivers_.end()) {
            id = it->first;
            type = "Receiver";
        } else {
            LOG(ERROR) << "Closed socket not found in senders or receivers.";
            return;
        }
    close:
        CloseMessageHandler(
            avrtc::ParseMessage("Request:Close\nID:" + std::to_string(id) +
                                "\nType:" + type + "\n"),
            socket);
    }

   private:
    std::shared_ptr<ServerUI> server_ui_;
    std::unordered_map<int, std::shared_ptr<avrtc::SessionSocket>> senders_;
    std::unordered_map<int, std::shared_ptr<avrtc::SessionSocket>> receivers_;
    int current_sender_id_ = 0;
    int current_receiver_id_ = 0;
};

int main(int argc, char** argv) {
    FLAGS_alsologtostderr = 1;
    google::InitGoogleLogging(argv[0]);
    auto app = Gtk::Application::create();
    std::shared_ptr<ServerUI> server_ui = std::make_shared<ServerUI>();

    std::unique_ptr<avrtc::Thread> thread = std::make_unique<avrtc::Thread>();
    std::shared_ptr<AvrtcServer> server = std::make_shared<AvrtcServer>(
        avrtc::SocketAddress(14562, AF_INET), server_ui);
    thread->AddTask([server]() {
        server->Start();
        avrtc::ThreadManager::Instance()->CurrentThread()->Stop();
    });

    app->run(*server_ui);
    return 0;
}