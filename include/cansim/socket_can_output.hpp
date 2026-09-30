#pragma once

#include "cansim/can_output.hpp"

#include <string>

namespace cansim {

class SocketCanOutput final : public CanOutput {
  public:
    explicit SocketCanOutput(std::string interface_name);
    ~SocketCanOutput() override;

    SocketCanOutput(const SocketCanOutput&) = delete;
    SocketCanOutput& operator=(const SocketCanOutput&) = delete;

    void send(const CanFrame& frame) override;

  private:
    std::string interface_name_;
    int socket_fd_{-1};
};

} // namespace cansim
