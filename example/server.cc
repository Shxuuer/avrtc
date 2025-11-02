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
    struct SocketSdpPair {
        std::shared_ptr<avrtc::SessionSocket> socket;
        std::shared_ptr<avrtc::SDPHandler> sdp;
    };

    AvrtcServer(avrtc::SocketAddress address, std::shared_ptr<ServerUI> ui)
        : ServerSocket(address), server_ui_(ui) {
        // 创建服务器时设置接收连接回调
        SetOnAccept([this](std::shared_ptr<avrtc::SessionSocket> socket) {
            this->OnAccept(socket);
        });
    }

    void OnAccept(std::shared_ptr<avrtc::SessionSocket> socket) {
        // 链接后设置每个链接的接收数据和关闭回调
        socket->SetOnReceive(
            [this](std::shared_ptr<avrtc::SessionSocket> socket, char* buffer,
                   size_t length) { this->OnReceive(socket, buffer, length); });

        socket->SetOnClose(
            [this](std::shared_ptr<avrtc::SessionSocket> socket) {
                this->OnClose(socket);
            });
    }

    /**
     * 处理添加消息，将对端名字添加到UI列表中，并回复分配的ID，存储id map
     * @param sdp_msg SDP消息
     * @param socket 关联的socket连接
     */
    void AddMessageHandler(std::shared_ptr<avrtc::SDPHandler> sdp_msg,
                           std::shared_ptr<avrtc::SessionSocket> socket) {
        std::string name =
            sdp_msg->o->username_ + "@" + sdp_msg->o->unicast_address_;
        current_sender_id_++;

        std::shared_ptr<SocketSdpPair> pair = std::make_shared<SocketSdpPair>();
        pair->socket = socket;
        pair->sdp = sdp_msg;

        if (sdp_msg->z[1] == "Sender") {
            server_ui_->AddSender(name, current_sender_id_);
            senders_.insert({current_sender_id_, pair});
        } else if (sdp_msg->z[1] == "Receiver") {
            server_ui_->AddReceiver(name, current_receiver_id_);
            receivers_.insert({current_receiver_id_, pair});
        } else
            return;
        server_ui_->Emit();

        std::shared_ptr<avrtc::SDPHandler> response_sdp =
            std::make_shared<avrtc::SDPHandler>();
        response_sdp->z.push_back("Add");
        response_sdp->z.push_back(std::to_string(current_sender_id_));
        socket->Send(response_sdp->ToString());
    }

    /**
     * 处理关闭消息，从UI列表中移除对应ID，并从id map中删除
     * @param sdp_msg SDP消息
     * @param socket 关联的socket连接
     */
    void CloseMessageHandler(std::shared_ptr<avrtc::SDPHandler> sdp_msg,
                             std::shared_ptr<avrtc::SessionSocket> socket) {
        int id = std::stoi(sdp_msg->z[2]);

        if (sdp_msg->z[1] == "Sender") {
            LOG(INFO) << "Removing sender with ID: " << id;
            server_ui_->RemoveSender(id);
            senders_.erase(id);
        } else if (sdp_msg->z[1] == "Receiver") {
            server_ui_->RemoveReceiver(id);
            receivers_.erase(id);
        } else {
            return;
        }
        server_ui_->Emit();
    }

    /**
     * 接收数据回调，解析SDP消息并根据请求类型调用相应的处理函数
     * @param socket 关联的socket连接
     * @param buffer 接收到的数据缓冲区
     * @param length 接收到的数据长度
     */
    void OnReceive(std::shared_ptr<avrtc::SessionSocket> socket,
                   char* buffer,
                   size_t length) {
        std::string msg(buffer, length);
        LOG(INFO) << "Received data:\n" << msg;
        std::shared_ptr<avrtc::SDPHandler> sdp_msg =
            std::make_shared<avrtc::SDPHandler>(msg);

        auto request_type = sdp_msg->z[0];
        if (request_type == "Add")
            AddMessageHandler(sdp_msg, socket);
        else if (request_type == "Close")
            CloseMessageHandler(sdp_msg, socket);
        else
            LOG(WARNING) << "Unknown request type: " << request_type;
    }

    /**
     * 通过socket查找对应的ID
     * @param socket 关联的socket连接
     * @param map 存储socket和ID映射的map
     * @return 找到则返回ID，未找到返回-1
     */
    int FindIDBySocket(
        std::shared_ptr<avrtc::SessionSocket> socket,
        const std::unordered_map<int, std::shared_ptr<SocketSdpPair>>& map) {
        for (const auto& pair : map) {
            if (pair.second->socket == socket) {
                return pair.first;
            }
        }
        return -1;  // Not found
    }

    /**
     * 关闭连接回调，从UI列表和id map中移除对应的客户端
     * @param socket 关联的socket连接
     */
    void OnClose(std::shared_ptr<avrtc::SessionSocket> socket) {
        // 在列表中找出socket对应的ID
        int id;
        std::string type;
        if ((id = FindIDBySocket(socket, senders_)) != -1) {
            type = "Sender";
            goto close;
        } else if ((id = FindIDBySocket(socket, receivers_)) != -1) {
            type = "Receiver";
            goto close;
        } else {
            LOG(ERROR) << "Closed socket not found in senders or receivers.";
            return;
        }
    close:
        std::shared_ptr<avrtc::SDPHandler> sdp_msg =
            std::make_shared<avrtc::SDPHandler>();
        sdp_msg->z.push_back("Close");
        sdp_msg->z.push_back(type);
        sdp_msg->z.push_back(std::to_string(id));
        CloseMessageHandler(sdp_msg, socket);
    }

   private:
    std::shared_ptr<ServerUI> server_ui_;
    std::unordered_map<int, std::shared_ptr<SocketSdpPair>> senders_;
    std::unordered_map<int, std::shared_ptr<SocketSdpPair>> receivers_;
    int current_sender_id_ = 0;
    int current_receiver_id_ = 0;
};

int main(int argc, char** argv) {
    FLAGS_alsologtostderr = 1;
    google::InitGoogleLogging(argv[0]);
    auto app = Gtk::Application::create();

    auto server_ui = std::make_shared<ServerUI>();

    auto thread = std::make_unique<avrtc::Thread>();
    auto server = std::make_shared<AvrtcServer>(
        avrtc::SocketAddress(14562, AF_INET), server_ui);

    thread->AddTask([server]() {
        server->Start();
        avrtc::ThreadManager::Instance()->CurrentThread()->Stop();
    });

    app->run(*server_ui);
    server->Stop();
    server->Close();
    return 0;
}