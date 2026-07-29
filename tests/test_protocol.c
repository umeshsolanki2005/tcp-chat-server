#include "protocol.h"
#include <assert.h>

int main(void) {
    chat_message_t message;
    char buffer[sizeof(chat_message_t) + 16];
    size_t written = 0;

    prepare_message(&message, MSG_PUBLIC, "alice", "", "hello");
    assert(serialize_message(&message, buffer, sizeof(buffer), &written) == 0);
    assert(written == sizeof(uint32_t) + MAX_USERNAME + MAX_USERNAME + MAX_MESSAGE);
    assert(deserialize_message(buffer, written, &message) == 0);
    assert(message.type == MSG_PUBLIC);
    return 0;
}
