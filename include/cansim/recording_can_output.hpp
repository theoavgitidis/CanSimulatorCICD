#pragma once

#include "cansim/can_output.hpp"

#include <vector>

namespace cansim {

class RecordingCanOutput final : public CanOutput {
  public:
    void send(const CanFrame& frame) override {
        frames_.push_back(frame);
    }

    [[nodiscard]] const std::vector<CanFrame>& frames() const {
        return frames_;
    }

  private:
    std::vector<CanFrame> frames_{};
};

} // namespace cansim
