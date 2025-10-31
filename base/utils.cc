#include "base/utils.h"

namespace avrtc {

std::string GetUserName() {
    uid_t userid = getuid();
    struct passwd* pwd = getpwuid(userid);
    if (pwd) {
        return std::string(pwd->pw_name);
    }
    return "no_name";
}

std::string GetDeviceName() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "no_device";
}

std::string GetClientName() {
    return GetUserName() + "@" + GetDeviceName();
}

std::string GetCurrentIP() {
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
    std::string ip_address;

    if (getifaddrs(&ifaddr) == -1) {
        LOG(ERROR) << "getifaddrs failed";
        return "";
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET ||
            ifa->ifa_flags & IFF_LOOPBACK)
            continue;

        int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host,
                            NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if (s != 0) {
            LOG(ERROR) << "getnameinfo() failed for " +
                              std::string(ifa->ifa_name) + ": " +
                              gai_strerror(s);
            continue;
        }
        ip_address = host;
        break;
    }
    freeifaddrs(ifaddr);
    return ip_address;
}

MessageMap ParseMessage(const std::string& msg) {
    MessageMap result;
    std::istringstream stream(msg);
    std::string line;
    while (std::getline(stream, line)) {
        size_t delimiter_pos = line.find(':');
        if (delimiter_pos != std::string::npos) {
            std::string key = line.substr(0, delimiter_pos);
            std::string values_str = line.substr(delimiter_pos + 1);
            std::vector<std::string> values;
            std::istringstream values_stream(values_str);
            std::string value;
            while (std::getline(values_stream, value, ',')) {
                values.push_back(value);
            }
            result[key] = values;
        }
    }
    return result;
}

}  // namespace avrtc
