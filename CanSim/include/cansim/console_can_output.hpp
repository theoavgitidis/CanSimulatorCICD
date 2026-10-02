#pragma once

#include "cansim/can_output.hpp"

#include <ostream>

namespace cansim {

class ConsoleCanOutput final : public CanOutput {
  public:
    explicit ConsoleCanOutput(std::ostream& output);

    void send(const CanFrame& frame) override;

  private:
    std::ostream& output_;
};

} // namespace cansim
