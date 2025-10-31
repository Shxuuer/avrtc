#ifndef BASE_SOCKET_H
#define BASE_SOCKET_H

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace avrtc {

/**
 * 封装IP地址和端口
 */
class SocketAddress {
 public:
  // 服务端用于绑定任意地址的IP
  constexpr static int kAnyAddr = INADDR_ANY;

  // 构造函数
  SocketAddress(const char* ip_str,
                uint16_t port,
                sa_family_t family = AF_INET) {
    address_.sin_family = family;
    address_.sin_port = htons(port);
    inet_pton(family, ip_str, &address_.sin_addr);
  }

  SocketAddress(uint16_t port, sa_family_t family = AF_INET)
      : SocketAddress("0.0.0.0", port, family) {}

  SocketAddress(const struct sockaddr_in& address) : address_(address) {}

  // 获取sockaddr指针
  const struct sockaddr* GetSockAddr() const {
    return reinterpret_cast<const struct sockaddr*>(&address_);
  }
  // 获取地址长度
  socklen_t GetSockLen() const { return sizeof(address_); }

  // 将地址转换为字符串形式的IP地址
  std::string ToIpString() const {
    char ip_str[INET_ADDRSTRLEN] = {0};
    inet_ntop(
        address_.sin_family, &(address_.sin_addr), ip_str, INET_ADDRSTRLEN);
    return std::string(ip_str);
  }

  uint16_t GetPort() const { return ntohs(address_.sin_port); }
  sa_family_t GetFamily() const { return address_.sin_family; }
  in_addr_t GetIP() const { return address_.sin_addr.s_addr; }

 private:
  struct sockaddr_in address_{};
};

// 基类Socket，封装基本的socket操作
class Socket {
 public:
  Socket() = delete;
  Socket(SocketAddress address);
  virtual ~Socket();

  virtual void Close();

  int GetFD() const { return socket_fd_; }
  void SetFD(int fd) { socket_fd_ = fd; }

 protected:
  SocketAddress address_;
  int socket_fd_ = -1;
};

// 会话Socket，支持发送和接收数据
class SessionSocket : public Socket,
                      public std::enable_shared_from_this<SessionSocket> {
 public:
  using Socket::Socket;

  int Send(const char* buffer, size_t length);
  int Send(std::string message);
  bool Recv();

  using OnReceiveCallback = std::function<void(
      std::shared_ptr<SessionSocket>, char* buffer, size_t length)>;
  using OnCloseCallback = std::function<void(std::shared_ptr<SessionSocket>)>;

  /**
   * 设置接收数据回调,触发时机：每次接收到数据时
   */
  void SetOnReceive(OnReceiveCallback cb) { OnReceive_ = cb; }
  /**
   * 设置连接关闭回调,触发时机：连接断开时
   */
  void SetOnClose(OnCloseCallback cb) { OnClose_ = cb; }

 private:
  OnReceiveCallback OnReceive_;
  OnCloseCallback OnClose_;
};

// 客户端Socket，支持连接到服务器
class ClientSocket : public SessionSocket {
 public:
  using SessionSocket::SessionSocket;

  void Connect();
  void Stop();

 private:
  bool running_ = true;
};

// 服务器Socket，支持接受客户端连接
class ServerSocket : public Socket {
 public:
  const int MAX_EVENTS = 20;
  const int TIMEOUT_MS = 1000;

  ServerSocket(SocketAddress address);
  void Close() override;
  void Accept();
  void Start();
  void Stop();

  using OnAcceptCallback = std::function<void(std::shared_ptr<SessionSocket>)>;
  void SetOnAccept(OnAcceptCallback cb) { OnAccept_ = cb; }

 private:
  static void SetNonBlocking(int fd);
  int epoll_fd;
  bool running_ = true;
  std::unordered_map<int, std::shared_ptr<SessionSocket>> clients_;

  OnAcceptCallback OnAccept_;
};

}  // namespace avrtc

#endif  // BASE_SOCKET_H