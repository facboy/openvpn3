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

#ifndef OPENVPN_BUFFER_ZLIB_H
#define OPENVPN_BUFFER_ZLIB_H

#ifdef HAVE_ZLIB

#include <cstring> // for std::memset

#include <zlib.h>

#include <openvpn/common/clamp_typerange.hpp>
#include <openvpn/common/exception.hpp>
#include <openvpn/common/numeric_cast.hpp>
#include <openvpn/buffer/buffer.hpp>
#include <openvpn/buffer/buflist.hpp>


namespace openvpn {

using namespace numeric_util;

namespace ZLib {
OPENVPN_EXCEPTION(zlib_error);

class ZStreamBase // used internally by compress_gzip/decompress_gzip
{
  public:
    z_stream s;

  protected:
    ZStreamBase()
    {
        std::memset(&s, 0, sizeof(s));
    }

  private:
    ZStreamBase(const ZStreamBase &) = delete;
    ZStreamBase &operator=(const ZStreamBase &) = delete;
};

inline BufferPtr compress_gzip(BufferPtr src,
                               const size_t headroom,
                               const size_t tailroom,
                               const int level,
                               const int window_bits = 15,
                               const int mem_level = 8)
{
    constexpr int GZIP_ENCODING = 16;

    struct ZStream : public ZStreamBase
    {
        ~ZStream()
        {
            ::deflateEnd(&s);
        }
    };

    if (src)
    {
        int status;
        ZStream zs;
        zs.s.next_in = src->data();
        zs.s.avail_in = numeric_cast<decltype(zs.s.avail_in)>(src->size());
        status = ::deflateInit2(&zs.s,
                                level,
                                Z_DEFLATED,
                                GZIP_ENCODING + window_bits,
                                mem_level,
                                Z_DEFAULT_STRATEGY);
        if (status != Z_OK)
            OPENVPN_THROW(zlib_error, "zlib deflateinit2 failed, error=" << status);
        const uLong outcap = ::deflateBound(&zs.s, src->size());
        auto b = BufferAllocatedRc::Create(outcap + headroom + tailroom, BufAllocFlags::NO_FLAGS);
        b->init_headroom(headroom);
        zs.s.next_out = b->data();
        zs.s.avail_out = numeric_cast<decltype(zs.s.avail_out)>(outcap);
        status = ::deflate(&zs.s, Z_FINISH);
        if (status != Z_STREAM_END)
            OPENVPN_THROW(zlib_error, "zlib deflate failed, error=" << status);
        b->set_size(zs.s.total_out);
        return b;
    }
    else
        return BufferPtr();
}

inline BufferPtr decompress_gzip(BufferPtr src,
                                 const size_t headroom,
                                 const size_t tailroom,
                                 const size_t max_size,
                                 const size_t block_size = 4096,
                                 const int window_bits = 15)
{
    constexpr int GZIP_ENCODING = 16;

    struct ZStream : public ZStreamBase
    {
        ~ZStream()
        {
            ::inflateEnd(&s);
        }
    };

    if (src)
    {
        int status;
        ZStream zs;
        zs.s.next_in = src->data();
        zs.s.avail_in = numeric_cast<decltype(zs.s.avail_in)>(src->size());
        status = ::inflateInit2(&zs.s, GZIP_ENCODING + window_bits);
        if (status != Z_OK)
            OPENVPN_THROW(zlib_error, "zlib inflateinit2 failed, error=" << status);

        BufferList blist;
        size_t hr = headroom;
        size_t tr = tailroom;
        do
        {
            // use headroom/tailroom on first block to take advantage
            // of BufferList::join() optimization for one-block lists
            auto b = BufferAllocatedRc::Create(block_size + hr + tr, BufAllocFlags::NO_FLAGS);
            b->init_headroom(hr);
            const size_t avail = b->remaining(tr);
            zs.s.next_out = b->data();
            zs.s.avail_out = clamp_to_typerange<decltype(zs.s.avail_out)>(avail);
            status = ::inflate(&zs.s, Z_SYNC_FLUSH);
            if (status != Z_OK && status != Z_STREAM_END)
                OPENVPN_THROW(zlib_error, "zlib inflate failed, error=" << status);
            b->set_size(avail - zs.s.avail_out);
            blist.push_back(std::move(b));
            if (max_size && zs.s.total_out > max_size)
                OPENVPN_THROW(zlib_error, "zlib inflate max_size " << max_size << " exceeded");
            hr = tr = 0;
        } while (status == Z_OK);
        return blist.join(headroom, tailroom, true);
    }
    else
        return BufferPtr();
}

} // namespace ZLib
} // namespace openvpn

#endif
#endif
