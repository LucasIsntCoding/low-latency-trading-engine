#pragma once

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "llt/core/types.hpp"

namespace llt {

inline std::string serialize_md_event(const MarketDataEvent& ev) {
    return std::to_string(ev.seq) + "," + std::to_string(ev.exchange_ts_ns) + "," + ev.symbol + "," +
           std::string(side_to_string(ev.side)) + "," + std::to_string(ev.price_ticks) + "," +
           std::to_string(ev.qty) + "," + std::string(action_to_string(ev.action)) + "\n";
}

class UdpPublisher {
public:
    UdpPublisher(const std::string& host, int port) {
        fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (fd_ < 0) throw std::runtime_error("socket() failed");
        std::memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(static_cast<uint16_t>(port));
        if (::inet_pton(AF_INET, host.c_str(), &addr_.sin_addr) != 1) {
            throw std::runtime_error("inet_pton() failed for host " + host);
        }
    }

    ~UdpPublisher() { if (fd_ >= 0) ::close(fd_); }

    void publish(const MarketDataEvent& ev) {
        const auto msg = serialize_md_event(ev);
        ::sendto(fd_, msg.data(), msg.size(), 0, reinterpret_cast<sockaddr*>(&addr_), sizeof(addr_));
    }

private:
    int fd_{-1};
    sockaddr_in addr_{};
};

} // namespace llt
