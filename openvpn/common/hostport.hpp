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

#ifndef OPENVPN_COMMON_HOSTPORT_H
#define OPENVPN_COMMON_HOSTPORT_H

#include <string>

#include <openvpn/common/exception.hpp>
#include <openvpn/common/number.hpp>
#include <openvpn/common/unicode.hpp>

namespace openvpn::HostPort {
OPENVPN_EXCEPTION(host_port_error);

inline bool is_valid_port(const unsigned int port)
{
    return port < 65536;
}

inline bool is_valid_port(const std::string &port, unsigned int *value = nullptr)
{
    return parse_number_validate<unsigned int>(port, 5, 1, 65535, value);
}

inline void validate_port(const std::string &port, const std::string &title, unsigned int *value = nullptr)
{
    if (!is_valid_port(port, value))
        OPENVPN_THROW(host_port_error, "bad " << title << " port number: " << Unicode::utf8_printable(port, 16));
}

inline void validate_port(const unsigned int port, const std::string &title)
{
    if (!is_valid_port(port))
        OPENVPN_THROW(host_port_error, "bad " << title << " port number: " << port);
}

inline unsigned short parse_port(const std::string &port, const std::string &title)
{
    unsigned int ret = 0;
    validate_port(port, title, &ret);
    return static_cast<unsigned short>(ret);
}

// An IP address is also considered to be a valid host
inline bool is_valid_host_char(const char c)
{
    return (c >= 'a' && c <= 'z')
           || (c >= 'A' && c <= 'Z')
           || (c >= '0' && c <= '9')
           || c == '.'
           || c == '-'
           || c == ':'; // for IPv6
}

inline bool is_valid_host(const std::string &host)
{
    if (!host.length() || host.length() > 256)
        return false;
    for (const auto &c : host)
    {
        if (!is_valid_host_char(c))
            return false;
    }
    return true;
}

inline bool is_valid_unix_sock_char(const unsigned char c)
{
    return c >= 0x21 && c <= 0x7E;
}

inline bool is_valid_unix_sock(const std::string &host)
{
    if (!host.length() || host.length() > 256)
        return false;
    for (const auto &c : host)
    {
        if (!is_valid_unix_sock_char(c))
            return false;
    }
    return true;
}

inline void validate_host(const std::string &host, const std::string &title)
{
    if (!is_valid_host(host))
        OPENVPN_THROW(host_port_error, "bad " << title << " host: " << Unicode::utf8_printable(host, 64));
}

inline bool split_host_port(const std::string &str,
                            std::string &host,
                            std::string &port,
                            const std::string &default_port,
                            const bool allow_unix,
                            unsigned int *port_save = nullptr)
{
    if (port_save)
        *port_save = 0;
    const size_t fpos = str.find_first_of(':');
    const size_t lpos = str.find_last_of(':');
    const size_t cb = str.find_last_of(']');
    if (lpos != std::string::npos                      // has one or more colons (':')
        && (cb == std::string::npos || cb + 1 == lpos) // either has no closing bracket (']') or closing bracket followed by a colon ("]:")
        && (cb != std::string::npos || fpos == lpos))  // either has a closing bracket (']') or a single colon (':') to avoid fake-out by IPv6 addresses without a port
    {
        // host:port or [host]:port specified
        host = str.substr(0, lpos);
        port = str.substr(lpos + 1);
    }
    else if (!default_port.empty())
    {
        // only host specified
        host = str;
        port = default_port;
    }
    else
        return false;

    // unbracket host
    if (host.length() >= 2 && host[0] == '[' && host[host.length() - 1] == ']')
        host = host.substr(1, host.length() - 2);

    if (allow_unix && port == "unix")
        return is_valid_unix_sock(host);
    else
        return is_valid_host(host) && is_valid_port(port, port_save);
}

} // namespace openvpn::HostPort

#endif
