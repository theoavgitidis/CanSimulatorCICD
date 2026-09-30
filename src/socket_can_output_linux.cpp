#include "cansim/socket_can_output.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace cansim {
namespace {

[[nodiscard]] std::string systemError(const std::string& action,
                                      const std::string& interface_name) {
    return action + " for CAN interface '" + interface_name + "': " + std::strerror(errno);
}

} // namespace

SocketCanOutput::SocketCanOutput(std::string interface_name)
    : interface_name_{std::move(interface_name)} {
    socket_fd_ = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd_ < 0) {
        throw std::runtime_error(systemError("Failed to open SocketCAN socket", interface_name_));
    }

    ifreq request{};
    std::strncpy(request.ifr_name, interface_name_.c_str(), IFNAMSIZ - 1);

    if (::ioctl(socket_fd_, SIOCGIFINDEX, &request) < 0) {
        const std::string message =
            systemError("Failed to find SocketCAN interface", interface_name_);
        ::close(socket_fd_);
        socket_fd_ = -1;
        throw std::runtime_error(message);
    }

    sockaddr_can address{};
    address.can_family = AF_CAN;
    address.can_ifindex = request.ifr_ifindex;

    if (::bind(socket_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        const std::string message = systemError("Failed to bind SocketCAN socket", interface_name_);
        ::close(socket_fd_);
        socket_fd_ = -1;
        throw std::runtime_error(message);
    }
}

SocketCanOutput::~SocketCanOutput() {
    if (socket_fd_ >= 0) {
        ::close(socket_fd_);
    }
}

void SocketCanOutput::send(const CanFrame& frame) {
    can_frame socket_frame{};
    socket_frame.can_id = frame.id;
    socket_frame.can_dlc = frame.dlc;
    std::copy(frame.data.begin(), frame.data.end(), socket_frame.data);

    const ssize_t bytes_written = ::write(socket_fd_, &socket_frame, sizeof(socket_frame));
    if (bytes_written != static_cast<ssize_t>(sizeof(socket_frame))) {
        throw std::runtime_error(systemError("Failed to write CAN frame", interface_name_));
    }
}

} // namespace cansim
