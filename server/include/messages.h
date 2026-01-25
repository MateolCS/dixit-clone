#pragma once

#include "message.h"

struct NetworkEvent {
    size_t senderId;
    Message message;
};
