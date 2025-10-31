#include "base/socket.h"

namespace avrtc {

/**
 * 构造函数，创建socket并绑定地址
 * @param address 绑定的地址
 * @return void
 */
Socket::Socket(SocketAddress address) : address_(address) {
    socket_fd_ = socket(static_cast<int>(address.GetFamily()), SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        LOG(ERROR) << "Failed to create socket";
    }
    int opt = 1;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
        0) {
        perror("setsockopt failed");
        exit(1);
    }
}

Socket::~Socket() {
    Close();
}

/**
 * 关闭socket连接，对于派生类需要重写此方法时，
 * 必须调用基类的Close()，并调用相应的Stop()方法
 */
void Socket::Close() {
    if (socket_fd_ != -1) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

/**
 * 构造函数，创建服务器socket并绑定地址
 * @param address 绑定的地址
 * @return void
 */
ServerSocket::ServerSocket(SocketAddress address) : Socket(address) {
    int ret;
    ret = bind(socket_fd_, address.GetSockAddr(), address.GetSockLen());
    if (ret < 0) {
        LOG(ERROR) << "Failed to bind socket, " << strerror(errno);
    }
    ret = listen(socket_fd_, SOMAXCONN);
    if (ret < 0) {
        LOG(ERROR) << "Failed to listen on socket, " << strerror(errno);
    }
}

/**
 * 接受新的客户端连接，保存到clients_中，并触发OnAccept回调，
 * 接受新的连接后会将客户端socket加入到epoll监听中
 */
void ServerSocket::Accept() {
    // 接收新的客户端连接
    struct sockaddr_in client_address;
    socklen_t addr_len = sizeof(client_address);
    int client_fd =
        accept(socket_fd_, (struct sockaddr*)&(client_address), &addr_len);
    if (client_fd < 0) {
        LOG(ERROR) << "Failed to accept client connection";
    }

    // 保存客户端连接
    std::shared_ptr<SessionSocket> client_ptr =
        std::make_shared<SessionSocket>(client_address);
    client_ptr->SetFD(client_fd);

    clients_.insert({client_fd, client_ptr});

    // 触发回调
    if (OnAccept_) {
        OnAccept_(client_ptr);
    }

    // 加入到epoll监听
    SetNonBlocking(client_fd);
    epoll_event event = {.events = EPOLLIN, .data = {.fd = client_fd}};
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
        LOG(ERROR) << "Failed to add client socket to epoll";
        close(client_fd);
        return;
    }
}

/**
 * 设置文件描述符为非阻塞模式
 * @param fd 文件描述符
 */
void ServerSocket::SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        LOG(ERROR) << "Failed to get file descriptor flags";
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        LOG(ERROR) << "Failed to set non-blocking mode";
    }
}

/**
 * 启动服务器socket，进入事件循环
 * 使用epoll同时监听新连接和已有连接的数据
 * 在这里处理链接断开后的清理工作
 */
void ServerSocket::Start() {
    if (!OnAccept_) {
        LOG(INFO) << "OnAccept callback is not set, you'd better set it before "
                     "Start()";
    }
    epoll_fd = epoll_create1(0);

    SetNonBlocking(socket_fd_);
    epoll_event event = {.events = EPOLLIN, .data = {.fd = socket_fd_}};

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd_, &event) == -1) {
        LOG(ERROR) << "Failed to add server socket to epoll";
        close(epoll_fd);
        return;
    }

    epoll_event events[MAX_EVENTS];

    while (running_) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, TIMEOUT_MS);
        if (n == -1) {
            LOG(ERROR) << "Epoll wait error";
            break;
        } else if (n == 0) {
            LOG(INFO) << "size of clients_: "
                      << std::to_string(clients_.size());
            continue;
        }

        for (int i = 0; i < n; ++i) {
            // 处理新连接
            if (events[i].data.fd == socket_fd_) {
                Accept();
                continue;
            }
            // 处理已有连接的数据
            auto it = clients_.find(events[i].data.fd);
            if (it == clients_.end()) {
                LOG(ERROR) << "Unknown client socket";
                continue;
            }
            std::shared_ptr<SessionSocket> client = it->second;
            if (client->Recv()) {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->GetFD(), nullptr);
                clients_.erase(client->GetFD());
                continue;
            }
        }
    }

    close(epoll_fd);
    Close();
}

void ServerSocket::Stop() {
    running_ = false;
}

/**
 * 关闭所有客户端连接
 * @note 关闭服务器socket，必须先调用Stop()
 */
void ServerSocket::Close() {
    CHECK(running_ == false);  // 关闭socket之前必须先调用Stop()
    Stop();
    if (socket_fd_ == -1) {
        return;
    }
    Socket::Close();
    for (auto& client : clients_) {
        client.second->Close();
    }
    clients_.clear();
}

/**
 * 发送数据到socket
 * @param buffer 数据缓冲区
 * @param length 数据长度
 * @return 发送的字节数，失败返回-1
 */
int SessionSocket::Send(const char* buffer, size_t length) {
    CHECK(socket_fd_ != -1);
    int ret = send(socket_fd_, buffer, length, 0);
    if (ret < 0) {
        LOG(ERROR) << "Send failed: " << strerror(errno);
    }
    return ret;
}

/**
 * 发送字符串数据到socket
 * @param message 字符串数据
 * @return 发送的字节数，失败返回-1
 */
int SessionSocket::Send(std::string message) {
    return Send(message.c_str(), message.size());
}

/**
 * 从socket接收数据，触发OnReceive回调
 * 返回值：是否成功接收数据，true表示连接已关闭
 * 链接关闭时只会触发OnClose回调，不会触发OnReceive回调
 */
bool SessionSocket::Recv() {
    CHECK(socket_fd_ != -1);
    int buffer_size = 1024;
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(buffer_size);
    if ((buffer_size = recv(socket_fd_, buffer.get(), buffer_size, 0)) < 0) {
        LOG(ERROR) << "Recv failed: " << strerror(errno);
    }

    if (buffer_size == 0) {
        if (OnClose_)
            OnClose_(shared_from_this());
        return true;
    }

    if (OnReceive_)
        OnReceive_(shared_from_this(), buffer.get(), buffer_size);

    return false;
}

/**
 * 连接到服务器,进入接收数据循环
 */
void ClientSocket::Connect() {
    CHECK(socket_fd_ != -1);
    int ret =
        connect(socket_fd_, address_.GetSockAddr(), address_.GetSockLen());
    if (ret < 0) {
        LOG(ERROR) << "Failed to connect to server";
    }

    while (running_)
        Recv();
}

/**
 * 停止客户端socket的接收循环
 */
void ClientSocket::Stop() {
    running_ = false;
}

}  // namespace avrtc