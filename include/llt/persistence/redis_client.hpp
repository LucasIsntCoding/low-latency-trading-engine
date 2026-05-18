#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace llt {

class RedisClient {
public:
    RedisClient() = default;
    RedisClient(const RedisClient&) = delete;
    RedisClient& operator=(const RedisClient&) = delete;
    ~RedisClient() { close(); }

    bool connect_to(const std::string& host, int port) {
        close();
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) return false;
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));
        if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
            close(); return false;
        }
        if (::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            close(); return false;
        }
        return true;
    }

    bool set(const std::string& key, const std::string& value) {
        return command({"SET", key, value});
    }

    bool hset(const std::string& key, const std::string& field, const std::string& value) {
        return command({"HSET", key, field, value});
    }

    bool connected() const noexcept { return fd_ >= 0; }

private:
    bool command(const std::vector<std::string>& parts) {
        if (fd_ < 0) return false;
        std::ostringstream oss;
        oss << '*' << parts.size() << "\r\n";
        for (const auto& p : parts) {
            oss << '$' << p.size() << "\r\n" << p << "\r\n";
        }
        const auto payload = oss.str();
        const auto n = ::send(fd_, payload.data(), payload.size(), MSG_NOSIGNAL);
        if (n < 0) { close(); return false; }
        char buf[256]{};
        const auto r = ::recv(fd_, buf, sizeof(buf) - 1, 0);
        if (r <= 0) { close(); return false; }
        return buf[0] != '-';
    }

    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    int fd_{-1};
};

} // namespace llt
