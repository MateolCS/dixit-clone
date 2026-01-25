#pragma once

#include <vector>
#include <optional>

#include "message.h"

std::vector<uint8_t> serializeMessage(const Message& msg);
std::optional<Message> deserializeMesssage(const std::vector<uint8_t>& data);
