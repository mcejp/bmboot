//! @file
//! @brief  Message queue
//! @author Martin Cejp

#include "bmboot/message_queue.hpp"

#include <algorithm>
#include <string.h>

using namespace bmboot;
using std::nullopt;

// ************************************************************

std::optional<size_t> MessageQueueBase::getPendingMessageSize(size_t header_size)
{
    size_t body_size {};

    if (spaceUsed() < sizeof(body_size))
    {
        return nullopt;
    }

    peekRaw(&body_size, sizeof(body_size), 0);

    if (spaceUsed() < sizeof(body_size) + header_size + body_size)
    {
        // the message is incomplete, don't return it just yet
        return nullopt;
    }

    return body_size;
}

// ************************************************************

void MessageQueueBase::peekRaw(void* data_out, size_t length, size_t skip)
{
    auto rd_offset = maskIndex(m_control_block.rdpos + skip);

    auto run = std::min(length, m_buffer_size - rd_offset);
    auto rem = length - run;

    memcpy(data_out, m_buffer + rd_offset, run);
    memcpy((uint8_t*) data_out + run, m_buffer, rem);
}

// ************************************************************

std::optional<size_t> MessageQueueBase::readMessage(void* header, size_t header_size, std::span<uint8_t> buffer)
{
    auto maybe_body_size = getPendingMessageSize(header_size);

    if (!maybe_body_size.has_value())
    {
        return nullopt;
    }

    size_t body_size = *maybe_body_size;
    auto return_length = std::min(body_size, buffer.size());

    peekRaw(header, header_size, sizeof(body_size));
    peekRaw(buffer.data(), return_length, sizeof(body_size) + header_size);

    m_control_block.rdpos = wrapIndex(m_control_block.rdpos + sizeof(body_size) + header_size + body_size);

    return return_length;
}

// ************************************************************

bool MessageQueueBase::writeMessage(void const* header, size_t header_size, std::span<uint8_t const> body)
{
    // In the ring buffer, the message is laid out like this:
    //   +-----------+--------+------+
    //   | body_size | header | body |
    //   +-----------+--------+------+
    //
    // The size of the header is NOT included; it is assumed that both the sender and receiver agree on its exact structure
    // (if they don't, knowing the size will not help you...)
    // The header can also be void (0 bytes), as can the body.

    const size_t body_size = body.size();

    if (spaceAvailable() < sizeof(body_size) + header_size + body.size())
    {
        return false;
    }

    writeRaw(&body_size, sizeof(body_size));
    writeRaw(header, header_size);
    writeRaw(body.data(), body.size());

    return true;
}

// ************************************************************

void MessageQueueBase::writeRaw(void const* data, size_t length)
{
    auto run = std::min(length, m_buffer_size - maskIndex(m_control_block.wrpos));
    auto rem = length - run;

    memcpy(m_buffer + maskIndex(m_control_block.wrpos), data, run);
    memcpy(m_buffer, (uint8_t const*) data + run, rem);
    m_control_block.wrpos = wrapIndex(m_control_block.wrpos + length);
}

// ************************************************************
