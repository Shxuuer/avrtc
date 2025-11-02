#ifndef EXAMPLE_CLIENT_H_
#define EXAMPLE_CLIENT_H_

#include <glog/logging.h>

#include <iostream>
#include <string>

#include "base/sdp.h"
#include "base/socket.h"
#include "base/thread.h"

class ClientUI;

class AvrtcClient : public avrtc::ClientSocket {
 public:
  AvrtcClient(avrtc::SocketAddress address, std::shared_ptr<ClientUI> ui);

  void OnConnected();
  void OnReceive(char* buffer, size_t length);

  std::shared_ptr<avrtc::SDPHandler> sdp_handler_;

 private:
  int id_ = -1;
  std::shared_ptr<ClientUI> client_ui_;
};

#endif  // EXAMPLE_CLIENT_H_