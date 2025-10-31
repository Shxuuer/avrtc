#ifndef BASE_UTILS_H_
#define BASE_UTILS_H_

#include <arpa/inet.h>
#include <glog/logging.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace avrtc {

enum ClientType { kSender = 0, kReceiver = 1 };

std::string GetUserName();
std::string GetDeviceName();
std::string GetClientName();

std::string GetCurrentIP();

using MessageMap = std::unordered_map<std::string, std::vector<std::string>>;
MessageMap ParseMessage(const std::string& msg);

}  // namespace avrtc
#endif  // BASE_UTILS_H_