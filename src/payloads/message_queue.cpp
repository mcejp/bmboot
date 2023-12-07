#include <bmboot/message_queue.hpp>
#include <bmboot/payload_runtime.hpp>

#include "../tools/message_queue_demo.hpp"

using namespace bmboot;

int main(int argc, char** argv)
{
    bmboot::notifyPayloadStarted();

    printf("Hello from payload\n");

    auto rdQueue =  createMessageQueue<MessageQueueReader<void>>(    (uint8_t*) physical_address_of_queue,                  queue_size);
    auto rdQueue2 = createMessageQueue<MessageQueueReader<MyHeader>>((uint8_t*) physical_address_of_queue + queue_size,     queue_size);
    auto wrQueue =  createMessageQueue<MessageQueueWriter<void>>(    (uint8_t*) physical_address_of_queue + 2 * queue_size, queue_size);
    auto wrQueue2 = createMessageQueue<MessageQueueWriter<MyHeader>>((uint8_t*) physical_address_of_queue + 3 * queue_size, queue_size);

    printf("Writing to queue...\n");

    char const greeting[] = "Hello world from Bare-metal";
    wrQueue.write({(uint8_t const*) greeting, sizeof(greeting)});

    wrQueue2.write({2023, 12.06f}, {});

    printf("Done writing, now reading...\n");

    for (;;)
    {
        std::array<uint8_t, 200> buffer;
        auto message = rdQueue.read(buffer);

        if (message.has_value())
        {
            printf("received '%s'\n", message->data());
        }

        auto message2 = rdQueue2.read(buffer);

        if (message2.has_value())
        {
            printf("received %d, %f\n", message2->first.foo, message2->first.bar);
        }
    }
}
