//! @file
//! @brief  Message queue demo
//! @author Martin Cejp

#include "message_queue_demo.hpp"

#include "bmboot/domain.hpp"
#include "bmboot/domain_helpers.hpp"
#include "bmboot/message_queue.hpp"

#include "../utility/mmap.hpp"

#include <fcntl.h>
#include <unistd.h>

using namespace bmboot;

// ************************************************************

int main(int argc, char** argv)
{
    fprintf(stderr, "Start Bmboot\n");

    auto domain = throwOnError(IDomain::open(DomainIndex::cpu1), "IDomain::open");
    throwOnError(domain->ensureReadyToLoadPayload(), "ensureReadyToLoadPayload");

    fprintf(stderr, "Map memory\n");

    int fd = open("/dev/mem", O_RDWR);
    Mmap buffer(nullptr, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, physical_address_of_queue);

    fprintf(stderr, "Init message queues\n");

    auto wrQueue =  createMessageQueue<MessageQueueWriter<void>>(    buffer.data(),                  queue_size);
    auto wrQueue2 = createMessageQueue<MessageQueueWriter<MyHeader>>((uint8_t*) buffer.data() + queue_size,     queue_size);
    auto rdQueue =  createMessageQueue<MessageQueueReader<void>>(    (uint8_t*) buffer.data() + 2 * queue_size, queue_size);
    auto rdQueue2 = createMessageQueue<MessageQueueReader<MyHeader>>((uint8_t*) buffer.data() + 3 * queue_size, queue_size);

    fprintf(stderr, "Run payload\n");

    loadPayloadFromFileOrThrow(*domain, "payload_message_queue_cpu1.bin");

    char const message[] = "Hello world from Linux";
    wrQueue.write({(uint8_t const*) message, sizeof(message)});

    wrQueue2.write({123, 456.789f}, {});

    for (int i = 0; i < 10; i++) {
        std::array<uint8_t, 200> buffer;
        auto received = rdQueue.read(buffer);

        if (received.has_value())
        {
            auto s = std::string(received->begin(), received->end());
            printf("Received message (%zu bytes): '%s'\n", received->size(), s.c_str());
        }

        auto received2 = rdQueue2.read(buffer);

        if (received2.has_value())
        {
            auto header = received2->first;
            auto body = received2->second;
            printf("Received message: foo=%d bar=%f + %zu bytes of body (not shown)\n", header.foo, header.bar, body.size());
        }

        usleep(10'000);
    }

    fprintf(stderr, "Displaying payload stdout (Ctrl-C to quit)\n");

    runConsoleUntilInterrupted(*domain);
}
