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

// Wrap a mbed TLS pk_context object.

#ifndef OPENVPN_MBEDTLS_PKI_PKCTX_H
#define OPENVPN_MBEDTLS_PKI_PKCTX_H

#include <string>
#include <sstream>
#include <cstring>

#include <mbedtls/pk.h>

#include <openvpn/common/size.hpp>
#include <openvpn/common/exception.hpp>
#include <openvpn/common/rc.hpp>
#include <openvpn/mbedtls/util/error.hpp>
#include <openvpn/mbedtls/util/rand.hpp>

namespace openvpn::MbedTLSPKI {

class PKContext : public RC<thread_unsafe_refcount>
{
  public:
    typedef RCPtr<PKContext> Ptr;

    PKContext()
        : ctx(nullptr)
    {
    }

    PKContext(const std::string &key_txt, const std::string &title, const std::string &priv_key_pwd, MbedTLSRandom &rand)
        : ctx(nullptr)
    {
        try
        {
            parse(key_txt, title, priv_key_pwd, rand);
        }
        catch (...)
        {
            dealloc();
            throw;
        }
    }

    bool defined() const
    {
        return ctx != nullptr;
    }

    PKType::Type key_type() const
    {
        switch (mbedtls_pk_get_type(ctx))
        {
        case MBEDTLS_PK_RSA:
        case MBEDTLS_PK_RSA_ALT:
        case MBEDTLS_PK_RSASSA_PSS:
            return PKType::PK_RSA;
        case MBEDTLS_PK_ECKEY:
        case MBEDTLS_PK_ECKEY_DH:
            return PKType::PK_EC;
        case MBEDTLS_PK_ECDSA:
            return PKType::PK_ECDSA;
        case MBEDTLS_PK_NONE:
            return PKType::PK_NONE;
        default:
            return PKType::PK_UNKNOWN;
        }
    }

    size_t key_length() const
    {
        return mbedtls_pk_get_bitlen(ctx);
    }

    void parse(const std::string &key_txt, const std::string &title, const std::string &priv_key_pwd, MbedTLSRandom &rand)
    {
        alloc();
        // key_txt.length() is increased by 1 as it does not include the NULL-terminator
        // which mbedtls_pk_parse_key() expects to see.
        const int status = mbedtls_pk_parse_key(ctx,
                                                (const unsigned char *)key_txt.c_str(),
                                                key_txt.length() + 1,
                                                (const unsigned char *)priv_key_pwd.c_str(),
                                                priv_key_pwd.length()
#if MBEDTLS_VERSION_NUMBER > 0x03000000
                                                    ,
                                                mbedtls_ctr_drbg_random,
                                                rand.get_ctr_drbg_ctx()
#endif
        );
        if (status < 0)
            throw MbedTLSException("error parsing " + title + " private key", status);
    }

    std::string extract() const
    {
        // maximum size of the PEM data is not available at this point
        BufferAllocated buff(16000);

        int ret = mbedtls_pk_write_key_pem(ctx, buff.data(), buff.max_size());
        if (ret < 0)
            throw MbedTLSException("extract priv_key: can't write to buffer", ret);

        return std::string((const char *)buff.data());
    }

    std::string render_pem() const
    {
        return extract();
    }

    void epki_enable(void *arg,
                     mbedtls_pk_rsa_alt_decrypt_func epki_decrypt,
                     mbedtls_pk_rsa_alt_sign_func epki_sign,
                     mbedtls_pk_rsa_alt_key_len_func epki_key_len)
    {
        alloc();
        const int status = mbedtls_pk_setup_rsa_alt(ctx, arg, epki_decrypt, epki_sign, epki_key_len);
        if (status < 0)
            throw MbedTLSException("error in mbedtls_pk_setup_rsa_alt", status);
    }

    mbedtls_pk_context *get() const
    {
        return ctx;
    }

    ~PKContext()
    {
        dealloc();
    }

  private:
    void alloc()
    {
        if (!ctx)
        {
            ctx = new mbedtls_pk_context;
            mbedtls_pk_init(ctx);
        }
    }

    void dealloc()
    {
        if (ctx)
        {
            mbedtls_pk_free(ctx);
            delete ctx;
            ctx = nullptr;
        }
    }

    mbedtls_pk_context *ctx;
};

} // namespace openvpn::MbedTLSPKI
#endif
