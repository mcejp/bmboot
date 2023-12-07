//! @file
//! @brief  Message queue
//!
//! As a utility to its users, Bmboot provides a set of classes implementing unidirectional message queues.
//! Every message can be prefixed with a header of user-defined type.
//!
//! The header type must match between the reader and the writer; undefined behavior ensues otherwise.
//!
//! On the manager (Linux) side, the message queue(s) should be instantiated after Bmboot has been started on the target core.
//! This is to ensure that the message queue control structure initialization (which is done on Linux) is cache-coherent.

#pragma once

#include <optional>
#include <span>
#include <stdint.h>
#include <string.h>
#include <utility>

namespace bmboot
{

// The message queue is implemented as a ring buffer using two indexes "mod 2*capacity"
// See https://gist.github.com/mcejp/719d3485b04cfcf82e8a8734957da06a

struct MessageQueueShmem
{
    size_t rdpos;
    size_t wrpos;
};

class MessageQueueBase
{
public:
    MessageQueueBase(MessageQueueShmem volatile& control_block, uint8_t* buffer, size_t buffer_size)
            : m_control_block(control_block), m_buffer(buffer), m_buffer_size(buffer_size) {}

    size_t maskIndex(size_t index) const { return index % m_buffer_size; }
    size_t spaceAvailable() const { return m_buffer_size - spaceUsed(); }
    size_t spaceUsed() const { return wrapIndex(m_control_block.wrpos - m_control_block.rdpos); }
    size_t wrapIndex(size_t index) const { return index % (2 * m_buffer_size); }

    std::optional<size_t> getPendingMessageSize(size_t header_size);
    std::optional<size_t> readMessage(void* header, size_t header_size, std::span<uint8_t> buffer);
    bool writeMessage(void const* header, size_t header_size, std::span<uint8_t const> body);

private:
    MessageQueueShmem volatile& m_control_block;
    uint8_t* m_buffer;
    size_t m_buffer_size;

    void peekRaw(void* data_out, size_t length, size_t skip);
    void writeRaw(void const* data, size_t length);
};

/**
 * Message queue writer. Do not instantiate this class directly; use @link bmboot::createMessageQueue createMessageQueue@endlink.
 *
 * @tparam MessageHeader Data structure used as message header. Must be POD.
 */
template <typename MessageHeader>
class MessageQueueWriter : MessageQueueBase
{
public:
    MessageQueueWriter(MessageQueueShmem volatile& control_block, uint8_t* buffer, size_t buffer_size)
            : MessageQueueBase(control_block, buffer, buffer_size) {}

    /**
     * Write a message into the queue
     * @param header message header
     * @param body message body
     * @return true on success, false if the message does not fit
     */
    bool write(MessageHeader const& header, std::span<uint8_t const> body)
    {
        return this->writeMessage(&header, sizeof(header), body);
    }
};

/**
 * Message queue writer, specialized for messages without a fixed header.
 *
 * Do not instantiate this class directly; use @link bmboot::createMessageQueue createMessageQueue@endlink.
 */
template<>
class MessageQueueWriter<void> : MessageQueueBase
{
public:
    MessageQueueWriter(MessageQueueShmem volatile& control_block, uint8_t* buffer, size_t buffer_size)
            : MessageQueueBase(control_block, buffer, buffer_size) {}

    /**
     * Write a message into the queue
     * @param body message body
     * @return true on success, false if the message does not fit
     */
    bool write(std::span<uint8_t const> body)
    {
        return this->writeMessage(nullptr, 0, body);
    }
};

/**
 * Message queue reader. Do not instantiate this class directly; use @link bmboot::createMessageQueue createMessageQueue@endlink.
 *
 * @tparam MessageHeader Data structure used as message header. Must be POD.
 */
template <typename MessageHeader>
class MessageQueueReader : MessageQueueBase
{
public:
    MessageQueueReader(MessageQueueShmem volatile& control_block, uint8_t* buffer, size_t buffer_size)
            : MessageQueueBase(control_block, buffer, buffer_size) {}

    /**
     * Check if there is a message in the queue, and return its length.
     * This can be used to appropriately size the buffer for a subsequent call to read().
     *
     * @return The length of a pending message. This excludes the header: a return value of 0 indicates a message
     *         with a header and an empty body. @b std::nullopt is returned if there is no message to read.
     */
    std::optional<size_t> getPendingMessageSize()
    {
        return MessageQueueBase::getPendingMessageSize(sizeof(header_buffer));
    }

    /**
     * Attempt to read a message from the queue
     * @param body_buffer Buffer to receive the message body. If the buffer is insufficient, the excess data is
     *        truncated. The appropriate size can be determined by calling getPendingMessageSize().
     * @return The message, returned as a tuple of header (by reference) and a sub-span of @p body_buffer.
     *         The header is guaranteed to be valid until the next method call on this reader.
     *         @b std::nullopt is returned if there is no message to read.
     */
    std::optional<std::pair<MessageHeader const&, std::span<uint8_t>>> read(std::span<uint8_t> body_buffer)
    {
        auto body_size = this->readMessage(&header_buffer, sizeof(header_buffer), body_buffer);

        if (!body_size.has_value())
        {
            return std::nullopt;
        }

        return {{header_buffer, {body_buffer.data(), *body_size}}};
    }

private:
    MessageHeader header_buffer;
};

/**
 * Message queue reader, specialized for messages without a fixed header.
 *
 * Do not instantiate this class directly; use @link bmboot::createMessageQueue createMessageQueue@endlink.
 */
template <>
class MessageQueueReader<void> : MessageQueueBase
{
public:
    MessageQueueReader(MessageQueueShmem volatile& control_block, uint8_t* buffer, size_t buffer_size)
            : MessageQueueBase(control_block, buffer, buffer_size) {}

    /**
     * Check if there is a message in the queue, and return its length.
     * This can be used to appropriately size the buffer for a subsequent call to read().
     *
     * @return The length of a pending message. A return value of 0 indicates an empty message.
     *         @b std::nullopt is returned if there is no message to read.
     */
    std::optional<size_t> getPendingMessageSize()
    {
        return MessageQueueBase::getPendingMessageSize(0);
    }

    /**
     * Attempt to read a message from the queue
     * @param body_buffer Buffer to receive the message body. If the buffer is insufficient, the excess data is
     *        truncated. The appropriate size can be determined by calling getPendingMessageSize().
     * @return The message, returned as a sub-span of @body_buffer.
     *         @b std::nullopt is returned if there is no message to read.
     */
    std::optional<std::span<uint8_t>> read(std::span<uint8_t> body_buffer)
    {
        auto body_size = this->readMessage(nullptr, 0, body_buffer);

        if (!body_size.has_value())
        {
            return std::nullopt;
        }

        return {{body_buffer.data(), *body_size}};
    }
};

/**
 * Instantiate a message queue reader or writer
 * @tparam T Reader or writer class, for example @c MessageQueueReader<MyHeader, 256>
 * @param virtual_address Virtual (on Linux) or physical (on bare metal) address of the memory area reserved for the message queue
 * @param size Size of said memory area
 * @return A message queue reader or writer
 */
template <typename T>
T createMessageQueue(void* virtual_address, size_t size)
{
    auto& control_block = *(MessageQueueShmem*) virtual_address;
    auto buffer = (uint8_t*) virtual_address + sizeof(MessageQueueShmem);
    auto buffer_size = size - sizeof(MessageQueueShmem);

    // The only asymmetry in this implementation is that the Linux side is tasked with initializing the message queue control structure
    // This is because the Linux side has the power to do this before that payload is started
#ifdef __linux__
    memset(&control_block, 0, sizeof(control_block));
#endif

    return T(control_block, buffer, buffer_size);
}

}
