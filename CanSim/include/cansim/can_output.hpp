#pragma once

#include "cansim/can_frame.hpp"

namespace cansim {

class CanOutput {
  public:
    virtual ~CanOutput() = default;

    virtual void send(const CanFrame& frame) = 0;
};

} // namespace cansim
