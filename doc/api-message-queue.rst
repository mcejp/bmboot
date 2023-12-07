*********************
API -- Message Queues
*********************

.. doxygenfile:: message_queue.hpp
   :sections: detaileddescription

.. doxygenclass:: bmboot::MessageQueueReader
   :members:

.. doxygenclass:: bmboot::MessageQueueReader< void >
   :members:

.. doxygenclass:: bmboot::MessageQueueWriter
   :members:

.. doxygenclass:: bmboot::MessageQueueWriter< void >
   :members:

.. doxygenfunction:: bmboot::createMessageQueue


Examples
========

Creating the writing end of a mesage queue on Linux:

.. code::

   struct MyHeader
   {
       int foo;
       float bar;
   };

   int fd = open("/dev/mem", O_RDWR);

   Mmap buffer(nullptr,
               QUEUE_BUFFER_SIZE,
               PROT_READ | PROT_WRITE,
               MAP_SHARED,
               fd,
               QUEUE_PHYSICAL_ADDRESS);

   auto wrQueue = createMessageQueue<MessageQueueWriter<MyHeader>>(buffer.data(), QUEUE_BUFFER_SIZE);

   char const message[] = "Hello world";
   wrQueue.write({123, 456.789f}, {(uint8_t const*) message, sizeof(message)});


Creating the reading end of a message queue on bare metal:

.. code::

    auto rdQueue = createMessageQueue<MessageQueueReader<MyHeader>>((void*) QUEUE_PHYSICAL_ADDRESS,
                                                                    QUEUE_BUFFER_SIZE);

    for (;;)
    {
        std::array<uint8_t, 200> buffer;
        auto message = rdQueue.read(buffer);

        if (message.has_value())
        {
            printf("received %d, %f, %s\n", message->first.foo,
                                            message->first.bar,
                                            message->second.data());
        }
   }
