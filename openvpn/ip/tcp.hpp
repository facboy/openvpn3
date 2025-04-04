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

#include <openvpn/common/endian.hpp>
#include <openvpn/ip/ipcommon.hpp>

namespace openvpn {

#pragma pack(push)
#pragma pack(1)

struct TCPHeader
{
    static unsigned int length(const std::uint8_t doff_res)
    {
        return ((doff_res) & 0xF0) >> 2;
    }

    std::uint16_t source;
    std::uint16_t dest;
    std::uint32_t seq;
    std::uint32_t ack_seq;
    std::uint8_t doff_res;
    std::uint8_t flags;
    std::uint16_t window;
    std::uint16_t check;
    std::uint16_t urgent_p;

    // helper enum to parse options in TCP header
    enum
    {
        OPT_EOL = 0,
        OPT_NOP = 1,
        OPT_MAXSEG = 2,
        OPTLEN_MAXSEG = 4
    };

    enum
    {
        FLAG_SYN = 1 << 1
    };
};

#pragma pack(pop)

/*
 * The following routine is used to update an
 * internet checksum.  "acc" is a 32-bit
 * accumulation of all the changes to the
 * checksum (adding in old 16-bit words and
 * subtracting out new words), and "cksum"
 * is the checksum value to be updated.
 */
inline void tcp_adjust_checksum(int acc, std::uint16_t &cksum)
{
    int _acc = acc;
    _acc += cksum;
    if (_acc < 0)
    {
        _acc = -_acc;
        _acc = (_acc >> 16) + (_acc & 0xffff);
        _acc += _acc >> 16;
        cksum = (uint16_t)~_acc;
    }
    else
    {
        _acc = (_acc >> 16) + (_acc & 0xffff);
        _acc += _acc >> 16;
        cksum = (uint16_t)_acc;
    }
}
} // namespace openvpn
