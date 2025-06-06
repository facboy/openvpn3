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

// test certificate subject and common name against tls_remote parameter

#ifndef OPENVPN_SSL_TLS_REMOTE_H
#define OPENVPN_SSL_TLS_REMOTE_H

#include <cstring>
#include <string>

namespace openvpn::TLSRemote {
inline bool test(const std::string &tls_remote, const std::string &subject, const std::string &common_name)
{
    return tls_remote == subject || common_name.starts_with(tls_remote);
}

inline void log(const std::string &tls_remote, const std::string &subject, const std::string &common_name)
{
    OPENVPN_LOG("tls-remote validation"
                << std::endl
                << "  tls-remote: '" << tls_remote << '\'' << std::endl
                << "  Subj: '" << subject << '\'' << std::endl
                << "  CN: '" << common_name << '\'');
}

// modifies x509 name in a way that is compatible with
// name remapping behavior on OpenVPN 2.x
inline std::string sanitize_x509_name(const std::string &str)
{
    std::string ret;
    bool leading_dash = true;
    ret.reserve(str.length());
    for (size_t i = 0; i < str.length(); ++i)
    {
        const char c = str[i];
        if (c == '-' && leading_dash)
        {
            ret += '_';
            continue;
        }
        leading_dash = false;
        if ((c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9')
            || c == '_' || c == '-' || c == '.'
            || c == '@' || c == ':' || c == '/'
            || c == '=')
            ret += c;
        else
            ret += '_';
    }
    return ret;
}

// modifies common name in a way that is compatible with
// name remapping behavior on OpenVPN 2.x
inline std::string sanitize_common_name(const std::string &str)
{
    std::string ret;
    ret.reserve(str.length());
    for (size_t i = 0; i < str.length(); ++i)
    {
        const char c = str[i];
        if ((c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9')
            || c == '_' || c == '-' || c == '.'
            || c == '@' || c == '/')
            ret += c;
        else
            ret += '_';
    }
    return ret;
}
} // namespace openvpn::TLSRemote

#endif
