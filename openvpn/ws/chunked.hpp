//    OpenVPN -- An application to securely tunnel IP networks
//               over a single port, with support for SSL/TLS-based
//               session authentication and key exchange,
//               packet encryption, packet authentication, and
//               packet compression.
//
//    Copyright (C) 2012- OpenVPN Inc.
//
//    SPDX-License-Identifier: MPL-2.0 OR AGPL-3.0-only WITH openvpn3-openssl-exception
//

#pragma once

#include <openvpn/common/size.hpp>
#include <openvpn/common/hexstr.hpp>
#include <openvpn/buffer/buffer.hpp>
#include <openvpn/frame/frame.hpp>

namespace openvpn::WS {
class ChunkedHelper
{
    enum State
    {
        hex,
        post_hex,
        post_hex_lf,
        post_chunk_cr,
        post_chunk_lf,
        post_content_cr,
        post_content_lf,
        done,
        chunk,
    };

  public:
    ChunkedHelper()
        : state(hex),
          size(0)
    {
    }

    template <typename PARENT>
    bool receive(PARENT &callback, BufferAllocated &buf)
    {
        while (buf.defined())
        {
            if (state == chunk)
            {
                if (size)
                {
                    if (buf.size() <= size)
                    {
                        size -= buf.size();
                        callback.chunked_content_in(buf);
                        break;
                    }
                    else
                    {
                        BufferAllocated content(buf.read_alloc(size), size, BufAllocFlags::NO_FLAGS);
                        size = 0;
                        callback.chunked_content_in(content);
                    }
                }
                else
                    state = post_chunk_cr;
            }
            else if (state == done)
                break;
            else
            {
                const char c = char(buf.pop_front());
            reprocess:
                switch (state)
                {
                case hex:
                    {
                        const int v = parse_hex_char(c);
                        if (v >= 0)
                            size = (size << 4) + v;
                        else
                        {
                            state = post_hex;
                            goto reprocess;
                        }
                    }
                    break;
                case post_hex:
                    if (c == '\r')
                        state = post_hex_lf;
                    break;
                case post_hex_lf:
                    if (c == '\n')
                    {
                        if (size)
                            state = chunk;
                        else
                            state = post_content_cr;
                    }
                    else
                    {
                        state = post_hex;
                        goto reprocess;
                    }
                    break;
                case post_chunk_cr:
                    if (c == '\r')
                        state = post_chunk_lf;
                    break;
                case post_chunk_lf:
                    if (c == '\n')
                        state = hex;
                    else
                    {
                        state = post_chunk_cr;
                        goto reprocess;
                    }
                    break;
                case post_content_cr:
                    if (c == '\r')
                        state = post_content_lf;
                    break;
                case post_content_lf:
                    if (c == '\n')
                        state = done;
                    else
                    {
                        state = post_content_cr;
                        goto reprocess;
                    }
                    break;
                default: // should never be reached
                    break;
                }
            }
        }
        return state == done;
    }

    static BufferPtr transmit(BufferPtr buf)
    {
        const size_t headroom = 24;
        const size_t tailroom = 16;
        static const char crlf[] = "\r\n";

        if (!buf || buf->offset() < headroom || buf->remaining() < tailroom)
        {
            // insufficient headroom/tailroom, must realloc
            Frame::Context fc(headroom, 0, tailroom, 0, sizeof(size_t), BufAllocFlags::NO_FLAGS);
            buf = fc.copy(buf);
        }

        size_t size = buf->size();
        buf->prepend((unsigned char *)crlf, 2);
        if (size)
        {
            while (size)
            {
                char *pc = (char *)buf->prepend_alloc(1);
                *pc = render_hex_char(size & 0xF);
                size >>= 4;
            }
        }
        else
        {
            char *pc = (char *)buf->prepend_alloc(1);
            *pc = '0';
        }
        buf->write((unsigned char *)crlf, 2);
        return buf;
    }

  private:
    State state;
    size_t size;
};
} // namespace openvpn::WS
