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

// Wrap the mbed TLS 2.3 SSL API as defined in <mbedtls/ssl.h>
// so that it can be used as the SSL layer by the OpenVPN core.

#ifndef OPENVPN_MBEDTLS_SSL_SSLCTX_H
#define OPENVPN_MBEDTLS_SSL_SSLCTX_H

#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <memory>

#include <mbedtls/ssl.h>
#include <mbedtls/oid.h>
#include <mbedtls/sha1.h>
#include <mbedtls/debug.h>
#include <mbedtls/asn1.h>
#include <mbedtls/version.h>

#include <openvpn/common/size.hpp>
#include <openvpn/common/exception.hpp>
#include <openvpn/common/base64.hpp>
#include <openvpn/common/binprefix.hpp>
#include <openvpn/common/string.hpp>
#include <openvpn/frame/memq_stream.hpp>
#include <openvpn/pki/cclist.hpp>
#include <openvpn/pki/pkcs1.hpp>
#include <openvpn/ssl/sslconsts.hpp>
#include <openvpn/ssl/sslapi.hpp>
#include <openvpn/ssl/verify_x509_name.hpp>
#include <openvpn/ssl/iana_ciphers.hpp>

#include <openvpn/mbedtls/pki/x509cert.hpp>
#include <openvpn/mbedtls/pki/x509certinfo.hpp>
#include <openvpn/mbedtls/pki/x509crl.hpp>
#include <openvpn/mbedtls/pki/dh.hpp>
#include <openvpn/mbedtls/pki/pkctx.hpp>
#include <openvpn/mbedtls/util/error.hpp>

// An SSL Context is essentially a configuration that can be used
// to generate an arbitrary number of actual SSL connections objects.

// MbedTLSContext is an SSL Context implementation that uses the
// mbed TLS library as a backend.

namespace openvpn {

namespace mbedtls_ctx_private {
namespace {

#if MBEDTLS_VERSION_NUMBER < 0x03000000
/*
 * This is a modified list from mbed TLS ssl_ciphersuites.c.
 * We removed some SHA1 methods near the top of the list to
 * avoid Chrome warnings about "obsolete cryptography".
 * We also removed ECDSA, CCM, PSK, and CAMELLIA algs.
 *
 * With mbed TLS 3 or newer we trust the default list of
 * algorithms in mbed TLS
 */


const int ciphersuites[] = // CONST GLOBAL
    {
        /* Selected AES-256 ephemeral suites */
        MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
        MBEDTLS_TLS_DHE_RSA_WITH_AES_256_GCM_SHA384,
        MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384,
        MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CBC_SHA256,

        MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
        MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384,

        /* Selected AES-128 ephemeral suites */
        MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
        MBEDTLS_TLS_DHE_RSA_WITH_AES_128_GCM_SHA256,
        MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256,
        MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CBC_SHA256,

        MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
        MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256,

        /* Selected remaining >= 128-bit ephemeral suites */
        MBEDTLS_TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA,
        MBEDTLS_TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA,

        MBEDTLS_TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA,

        /* Selected AES-256 suites */
        MBEDTLS_TLS_RSA_WITH_AES_256_GCM_SHA384,
        MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA256,
        MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA,
        MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384,
        MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384,
        MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA,

        MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384,
        MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384,
        MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA,

        /* Selected AES-128 suites */
        MBEDTLS_TLS_RSA_WITH_AES_128_GCM_SHA256,
        MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA256,
        MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA,
        MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256,
        MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256,
        MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA,

        MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256,
        MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256,
        MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA,

        /* Selected remaining >= 128-bit suites */
        MBEDTLS_TLS_RSA_WITH_3DES_EDE_CBC_SHA,
        MBEDTLS_TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA,

        MBEDTLS_TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA,

        0};
#endif

/*
 * X509 cert profiles.
 */

#ifdef OPENVPN_ALLOW_INSECURE_CERTPROFILE
// This profile includes the broken MD5 alrogithm.
// We are going to ship support for this algorithm for a limited
// amount of time to allow our users to switch to something else
const mbedtls_x509_crt_profile crt_profile_insecure = // CONST GLOBAL
    {
        MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_MD5)
            | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA1)
            | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_RIPEMD160)
            | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA224)
            | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA256)
            | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA384)
            | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA512),
        0xFFFFFFF, /* Any PK alg    */
        0xFFFFFFF, /* Any curve     */
        1024,      /* Minimum size for RSA keys */
};
#endif

const mbedtls_x509_crt_profile crt_profile_legacy = // CONST GLOBAL
    {
        MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA1)
            | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_RIPEMD160)
            | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA224)
            | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA256)
            | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA384)
            | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA512),
        0xFFFFFFF, /* Any PK alg    */
        0xFFFFFFF, /* Any curve     */
        1024,      /* Minimum size for RSA keys */
};

const mbedtls_x509_crt_profile crt_profile_preferred = // CONST GLOBAL
    {
        MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA256)
            | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA384)
            | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA512),
        0xFFFFFFF, /* Any PK alg    */
        0xFFFFFFF, /* Any curve     */
        2048,      /* Minimum size for RSA keys */
};
} // namespace
} // namespace mbedtls_ctx_private

// Handle different APIs regarding curves
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
using mbedtls_compat_group_id = uint16_t;
#else
using mbedtls_compat_group_id = mbedtls_ecp_group_id;
#endif

static inline mbedtls_compat_group_id
mbedtls_compat_get_group_id(const mbedtls_ecp_curve_info *curve_info)
{
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
    return curve_info->tls_id;
#else
    return curve_info->grp_id;
#endif
}

#if MBEDTLS_VERSION_NUMBER < 0x03000000
static inline void
mbedtls_ssl_conf_groups(mbedtls_ssl_config *conf, mbedtls_compat_group_id *groups)
{
    mbedtls_ssl_conf_curves(conf, groups);
}
#endif

// Represents an SSL configuration that can be used
// to instantiate actual SSL sessions.
class MbedTLSContext : public SSLFactoryAPI
{
  public:
    typedef RCPtr<MbedTLSContext> Ptr;

    enum
    {
        MAX_CIPHERTEXT_IN = 64 // maximum number of queued input ciphertext packets
    };

    // The data needed to construct a MbedTLSContext.
    class Config : public SSLConfigAPI
    {
        friend class MbedTLSContext;

      public:
        typedef RCPtr<Config> Ptr;

        Config()
            : external_pki(nullptr),
              ssl_debug_level(0),
              flags(0),
              ns_cert_type(NSCert::NONE),
              tls_version_min(TLSVersion::Type::V1_2),
              tls_cert_profile(TLSCertProfile::UNDEF),
              local_cert_enabled(true)
        {
        }

        SSLFactoryAPI::Ptr new_factory() override
        {
            return SSLFactoryAPI::Ptr(new MbedTLSContext(this));
        }

        void set_mode(const Mode &mode_arg) override
        {
            mode = mode_arg;
        }

        const Mode &get_mode() const override
        {
            return mode;
        }

        // if this callback is defined, no private key needs to be loaded
        void set_external_pki_callback(ExternalPKIBase *external_pki_arg, const std::string &alias) override
        {
            external_pki = external_pki_arg;
            external_pki_alias = alias;
        }

        void set_session_ticket_handler(TLSSessionTicketBase *session_ticket_handler_arg) override
        {
            // fixme -- this method should be implemented for server-side TLS session resumption tickets
            throw MbedTLSException("set_session_ticket_handler not implemented");
        }

        void set_client_session_tickets(const bool v) override
        {
            // fixme -- this method should be implemented for client-side TLS session resumption tickets
            throw MbedTLSException("set_client_session_tickets not implemented");
        }

        void enable_legacy_algorithms(const bool v) override
        {
            // We ignore the request to enable legacy as we do not have a runtime
            // configuration for this
        }

        void set_sni_handler(SNI::HandlerBase *sni_handler) override
        {
            // fixme -- this method should be implemented on the server-side for SNI
            throw MbedTLSException("set_sni_handler not implemented");
        }

        void set_sni_name(const std::string &sni_name_arg) override
        {
            // fixme -- this method should be implemented on the client-side for SNI
            throw MbedTLSException("set_sni_name not implemented");
        }

        void set_cn_reject_handler(CommonNameReject *cn_reject_handler_arg) override
        {
            throw MbedTLSException("set_cn_reject_handler not implemented");
        }

        void set_private_key_password(const std::string &pwd) override
        {
            priv_key_pwd = pwd;
        }

        void load_ca(const std::string &ca_txt, bool strict) override
        {
            MbedTLSPKI::X509Cert::Ptr c = new MbedTLSPKI::X509Cert();
            c->parse(ca_txt, "ca", strict);
            ca_chain = c;
        }

        void load_crl(const std::string &crl_txt) override
        {
            MbedTLSPKI::X509CRL::Ptr c = new MbedTLSPKI::X509CRL();
            c->parse(crl_txt);
            crl_chain = c;
        }

        void load_cert(const std::string &cert_txt) override
        {
            MbedTLSPKI::X509Cert::Ptr c = new MbedTLSPKI::X509Cert();
            c->parse(cert_txt, "cert", true);
            crt_chain = c;
        }

        void load_cert(const std::string &cert_txt, const std::string &extra_certs_txt) override
        {
            MbedTLSPKI::X509Cert::Ptr c = new MbedTLSPKI::X509Cert();
            c->parse(cert_txt, "cert", true);
            if (!extra_certs_txt.empty())
                c->parse(extra_certs_txt, "extra-certs", true);
            crt_chain = c;
        }

        void load_private_key(const std::string &key_txt) override
        {
            MbedTLSPKI::PKContext::Ptr p = new MbedTLSPKI::PKContext();
            auto *mbedrng = get_mbed_random_class();
            p->parse(key_txt, "config", priv_key_pwd, *mbedrng);
            priv_key = p;
        }

        void load_dh(const std::string &dh_txt) override
        {
            MbedTLSPKI::DH::Ptr mydh = new MbedTLSPKI::DH();
            mydh->parse(dh_txt, "server-config");
            dh = mydh;
        }

        std::string extract_ca() const override
        {
            if (!ca_chain)
                return std::string();
            return ca_chain->extract();
        }

        std::string extract_crl() const override
        {
            if (!crl_chain)
                return std::string();
            return crl_chain->extract();
        }

        std::string extract_cert() const override
        {
            if (!crt_chain)
                return std::string();
            return crt_chain->extract();
        }

        std::vector<std::string> extract_extra_certs() const override
        {
            if (!crt_chain)
                return std::vector<std::string>();
            return crt_chain->extract_extra_certs();
        }

        std::string extract_private_key() const override
        {
            if (!priv_key)
                return std::string();
            return priv_key->extract();
        }

        std::string extract_dh() const override
        {
            if (!dh)
                return std::string();
            return dh->extract();
        }

        PKType::Type private_key_type() const override
        {
            if (!priv_key)
                return PKType::PK_NONE;
            return priv_key->key_type();
        }

        size_t private_key_length() const override
        {
            if (!priv_key)
                return 0;
            return priv_key->key_length();
        }

        void set_frame(const Frame::Ptr &frame_arg) override
        {
            frame = frame_arg;
        }

        void set_debug_level(const int debug_level) override
        {
            ssl_debug_level = debug_level;
        }

        void set_flags(const unsigned int flags_arg) override
        {
            flags = flags_arg;
        }

        void set_ns_cert_type(const NSCert::Type ns_cert_type_arg) override
        {
            ns_cert_type = ns_cert_type_arg;
        }

        void set_remote_cert_tls(const KUParse::TLSWebType wt) override
        {
            KUParse::remote_cert_tls(wt, ku, eku);
        }

        void set_tls_remote(const std::string &tls_remote_arg) override
        {
            tls_remote = tls_remote_arg;
        }

        void set_tls_version_min(const TLSVersion::Type tvm) override
        {
            tls_version_min = tvm;
        }

        void set_tls_version_max(const TLSVersion::Type tvm) override
        {
        }

        void set_tls_version_min_override(const std::string &override) override
        {
            TLSVersion::apply_override(tls_version_min, override);
        }

        void set_tls_cert_profile(const TLSCertProfile::Type type) override
        {
            tls_cert_profile = type;
        }

        virtual void set_tls_cipher_list(const std::string &override)
        {
            if (!override.empty())
                tls_cipher_list = override;
        }

        virtual void set_tls_ciphersuite_list(const std::string &override)
        {
            // mbed TLS does not have TLS 1.3 support
        }

        virtual void set_tls_groups(const std::string &groups)
        {
            if (!groups.empty())
                tls_groups = groups;
        }

        void set_tls_cert_profile_override(const std::string &override) override
        {
            TLSCertProfile::apply_override(tls_cert_profile, override);
        }

        void set_local_cert_enabled(const bool v) override
        {
            local_cert_enabled = v;
        }

        void set_x509_track(X509Track::ConfigSet x509_track_config_arg) override
        {
            x509_track_config = std::move(x509_track_config_arg);
        }

        void set_rng(const StrongRandomAPI::Ptr &rng_arg) override
        {
            rng = rng_arg;
        }

        std::string validate_cert(const std::string &cert_txt) const override
        {
            MbedTLSPKI::X509Cert::Ptr cert = new MbedTLSPKI::X509Cert(cert_txt, "validation cert", true);
            return cert_txt; // fixme -- implement parse/re-render semantics
        }

        std::string validate_cert_list(const std::string &certs_txt) const override
        {
            MbedTLSPKI::X509Cert::Ptr cert = new MbedTLSPKI::X509Cert(certs_txt, "validation cert list", true);
            return certs_txt; // fixme -- implement parse/re-render semantics
        }

        std::string validate_private_key(const std::string &key_txt) const override
        {
            auto *mbedrng = get_mbed_random_class();
            MbedTLSPKI::PKContext::Ptr pkey = new MbedTLSPKI::PKContext(key_txt, "validation", "", *mbedrng);
            return key_txt; // fixme -- implement parse/re-render semantics
        }

        std::string validate_dh(const std::string &dh_txt) const override
        {
            MbedTLSPKI::DH::Ptr dh = new MbedTLSPKI::DH(dh_txt, "validation");
            return dh_txt; // fixme -- implement parse/re-render semantics
        }

        std::string validate_crl(const std::string &crl_txt) const override
        {
            MbedTLSPKI::X509CRL::Ptr crl = new MbedTLSPKI::X509CRL(crl_txt);
            return crl_txt; // fixme -- implement parse/re-render semantics
        }

        void load(const OptionList &opt, const unsigned int lflags) override
        {
            // client/server
            if (lflags & LF_PARSE_MODE)
                mode = opt.exists("client") ? Mode(Mode::CLIENT) : Mode(Mode::SERVER);

            // possibly disable peer cert verification
            if ((lflags & LF_ALLOW_CLIENT_CERT_NOT_REQUIRED)
                && opt.exists("client-cert-not-required"))
                flags |= SSLConst::NO_VERIFY_PEER;

            // sni
            {
                const std::string name = opt.get_optional("sni", 1, 256);
                if (!name.empty())
                    set_sni_name(name);
            }

            // ca
            {
                std::string ca_txt = opt.cat("ca");
                if (lflags & LF_RELAY_MODE)
                    ca_txt += opt.cat("relay-extra-ca");
                load_ca(ca_txt, true);
            }

            // CRL
            {
                const std::string crl_txt = opt.cat("crl-verify");
                if (!crl_txt.empty())
                    load_crl(crl_txt);
            }

            // local cert/key
            if (local_cert_enabled)
            {
                // cert/extra-certs
                {
                    const std::string &cert_txt = opt.get("cert", 1, Option::MULTILINE);
                    const std::string ec_txt = opt.cat("extra-certs");
                    load_cert(cert_txt, ec_txt);
                }

                // private key
                if (!external_pki)
                {
                    const std::string &key_txt = opt.get("key", 1, Option::MULTILINE);
                    load_private_key(key_txt);
                }
            }

            // DH
            if (mode.is_server() && opt.exists("dh"))
            {
                const std::string &dh_txt = opt.get("dh", 1, Option::MULTILINE);
                load_dh(dh_txt);
            }

            // relay mode
            std::string relay_prefix;
            if (lflags & LF_RELAY_MODE)
                relay_prefix = "relay-";

            // parse ns-cert-type
            ns_cert_type = NSCert::ns_cert_type(opt, relay_prefix);

            // parse remote-cert-x options
            KUParse::remote_cert_tls(opt, relay_prefix, ku, eku);
            KUParse::remote_cert_ku(opt, relay_prefix, ku);
            KUParse::remote_cert_eku(opt, relay_prefix, eku);

            // parse tls-remote
            tls_remote = opt.get_optional(relay_prefix + "tls-remote", 1, 256);

            // parse verify-x509-name
            verify_x509_name.init(opt, relay_prefix);

            // parse tls-version-min option
            {
#if defined(MBEDTLS_SSL_MAJOR_VERSION_3) && defined(MBEDTLS_SSL_MINOR_VERSION_3)
                const TLSVersion::Type maxver = TLSVersion::Type::V1_2;
#elif defined(MBEDTLS_SSL_MAJOR_VERSION_3) && defined(MBEDTLS_SSL_MINOR_VERSION_2)
                const TLSVersion::Type maxver = TLSVersion::Type::V1_1;
#else
                const TLSVersion::Type maxver = TLSVersion::Type::V1_0;
#endif
                tls_version_min = TLSVersion::parse_tls_version_min(opt, relay_prefix, maxver);
            }

            // parse tls-cert-profile
            tls_cert_profile = TLSCertProfile::parse_tls_cert_profile(opt, relay_prefix);

            // Overrides for tls cipher suites
            if (opt.exists("tls-cipher"))
                tls_cipher_list = opt.get_optional("tls-cipher", 1, 256);

            if (opt.exists("tls-groups"))
                tls_groups = opt.get_optional("tls-groups", 1, 256);

            // unsupported cert verification options
            {
            }
        }

#ifdef OPENVPN_JSON_INTERNAL
        SSLConfigAPI::Ptr json_override(const Json::Value &root, const bool load_cert_key) const override
        {
            throw MbedTLSException("json_override not implemented");
        }
#endif

        bool is_server() const
        {
            return mode.is_server();
        }

      private:
        MbedTLSRandom *get_mbed_random_class() const
        {
            if (!rng)
            {
                throw MbedTLSException("RNG not initialised yet");
            }
            auto *mbedrng = dynamic_cast<MbedTLSRandom *>(rng.get());
            if (!mbedrng)
            {
                throw MbedTLSException("RNG needs to be MbedTLSRandom");
            }
            return mbedrng;
        }


        const mbedtls_x509_crt_profile *select_crt_profile() const
        {
            switch (TLSCertProfile::default_if_undef(tls_cert_profile))
            {
#ifdef OPENVPN_ALLOW_INSECURE_CERTPROFILE
            case TLSCertProfile::INSECURE:
                return &mbedtls_ctx_private::crt_profile_insecure;
#endif
            case TLSCertProfile::LEGACY:
                return &mbedtls_ctx_private::crt_profile_legacy;
            case TLSCertProfile::PREFERRED:
                return &mbedtls_ctx_private::crt_profile_preferred;
            case TLSCertProfile::SUITEB:
                return &mbedtls_x509_crt_profile_suiteb;
            default:
                throw MbedTLSException("select_crt_profile: unknown cert profile");
            }
        }

        Mode mode;

      protected:
        MbedTLSPKI::X509Cert::Ptr crt_chain; // local cert chain (including client cert + extra certs)
        MbedTLSPKI::X509Cert::Ptr ca_chain;  // CA chain for remote verification

      private:
        MbedTLSPKI::X509CRL::Ptr crl_chain;  // CRL chain for remote verification
        MbedTLSPKI::PKContext::Ptr priv_key; // private key
        std::string priv_key_pwd;            // private key password
        MbedTLSPKI::DH::Ptr dh;              // diffie-hellman parameters (only needed in server mode)
        ExternalPKIBase *external_pki;
        std::string external_pki_alias;
        Frame::Ptr frame;
        int ssl_debug_level;
        unsigned int flags; // defined in sslconsts.hpp
        NSCert::Type ns_cert_type;
        std::vector<unsigned int> ku; // if defined, peer cert X509 key usage must match one of these values
        std::string eku;              // if defined, peer cert X509 extended key usage must match this OID/string
        std::string tls_remote;
        VerifyX509Name verify_x509_name;  // --verify-x509-name feature
        TLSVersion::Type tls_version_min; // minimum TLS version that we will negotiate
        TLSCertProfile::Type tls_cert_profile;
        std::string tls_cipher_list;
        std::string tls_groups;
        X509Track::ConfigSet x509_track_config;
        bool local_cert_enabled;
        StrongRandomAPI::Ptr rng; // random data source
    };

    // Represents an actual SSL session.
    // Normally instantiated by MbedTLSContext::ssl().
    class SSL : public SSLAPI
    {
        // read/write callback errors
        enum
        {
            // assumes that mbed TLS user-defined errors may start at -0x8000
            CT_WOULD_BLOCK = -0x8000,
            CT_INTERNAL_ERROR = -0x8001
        };

        friend class MbedTLSContext;

      public:
        typedef RCPtr<SSL> Ptr;

        void start_handshake() override
        {
            mbedtls_ssl_handshake(ssl);
        }

        ssize_t write_cleartext_unbuffered(const void *data, const size_t size) override
        {
            const int status = mbedtls_ssl_write(ssl, (const unsigned char *)data, size);
            if (status < 0)
            {
                if (status == CT_WOULD_BLOCK)
                    return SSLConst::SHOULD_RETRY;
                else if (status == CT_INTERNAL_ERROR)
                    throw MbedTLSException("SSL write: internal error");
                else
                    throw MbedTLSException("SSL write error", status);
            }
            else
                return status;
        }

        ssize_t read_cleartext(void *data, const size_t capacity) override
        {
            if (!overflow)
            {
                const int status = mbedtls_ssl_read(ssl, (unsigned char *)data, capacity);
                if (status < 0)
                {
                    if (status == CT_WOULD_BLOCK)
                        return SSLConst::SHOULD_RETRY;
                    else if (status == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
                        return SSLConst::PEER_CLOSE_NOTIFY;
                    else if (status == CT_INTERNAL_ERROR)
                        throw MbedTLSException("SSL read: internal error");
                    else
                        throw MbedTLSException("SSL read error", status);
                }
                else
                    return status;
            }
            else
                throw ssl_ciphertext_in_overflow();
        }

        bool read_cleartext_ready() const override
        {
            return !ct_in.empty() || mbedtls_ssl_get_bytes_avail(ssl);
        }

        void write_ciphertext(const BufferPtr &buf) override
        {
            if (ct_in.size() < MAX_CIPHERTEXT_IN)
                ct_in.write_buf(buf);
            else
                overflow = true;
        }

        void write_ciphertext_unbuffered(const unsigned char *data, const size_t size) override
        {
            if (ct_in.size() < MAX_CIPHERTEXT_IN)
                ct_in.write(data, size);
            else
                overflow = true;
        }

        bool read_ciphertext_ready() const override
        {
            return !ct_out.empty();
        }

        BufferPtr read_ciphertext() override
        {
            return ct_out.read_buf();
        }

        std::string ssl_handshake_details() const override
        {
            if (ssl)
            {
                const char *ver = mbedtls_ssl_get_version(ssl);
                const char *cs = mbedtls_ssl_get_ciphersuite(ssl);
                if (ver && cs)
                    return ver + std::string("/") + cs;
            }
            return "";
        }

        bool export_keying_material(const std::string &label, unsigned char *, size_t size) override
        {
            return false; // not implemented in our mbed TLS implementation
        }

        bool did_full_handshake() override
        {
            return false; // fixme -- not implemented
        }

        const AuthCert::Ptr &auth_cert() const override
        {
            return authcert;
        }

        void mark_no_cache() override
        {
            // fixme -- this method should be implemented for client-side TLS session resumption tickets
        }

        virtual ~SSL()
        {
            erase();
        }

      protected:
        SSL(MbedTLSContext *ctx, const char *hostname)
        {
            clear();
            try
            {
                const Config &c = *ctx->config;
                int endpoint, status;

                // set pointer back to parent
                parent = ctx;

                // set client/server mode
                if (c.mode.is_server())
                {
                    endpoint = MBEDTLS_SSL_IS_SERVER;
                    authcert.reset(new AuthCert());
                }
                else if (c.mode.is_client())
                    endpoint = MBEDTLS_SSL_IS_CLIENT;
                else
                    throw MbedTLSException("unknown client/server mode");

                // init SSL configuration object
                sslconf = new mbedtls_ssl_config;
                mbedtls_ssl_config_init(sslconf);
                mbedtls_ssl_config_defaults(sslconf,
                                            endpoint,
                                            MBEDTLS_SSL_TRANSPORT_STREAM,
                                            MBEDTLS_SSL_PRESET_DEFAULT);

                // init X509 cert profile
                mbedtls_ssl_conf_cert_profile(sslconf, c.select_crt_profile());

                // init SSL object
                ssl = new mbedtls_ssl_context;
                mbedtls_ssl_init(ssl);

                // set minimum TLS version
#if MBEDTLS_VERSION_NUMBER > 0x03000000
                mbedtls_ssl_protocol_version version;
                switch (c.tls_version_min)
                {
                default:
                case TLSVersion::Type::V1_2:
                    version = MBEDTLS_SSL_VERSION_TLS1_2;
                    break;

                case TLSVersion::Type::V1_3:
                    version = MBEDTLS_SSL_VERSION_TLS1_3;
                    break;
                }
                mbedtls_ssl_conf_min_tls_version(sslconf, version);
#else
                int major;
                int minor;
                switch (c.tls_version_min)
                {
#if defined(MBEDTLS_SSL_MAJOR_VERSION_3) && defined(MBEDTLS_SSL_MINOR_VERSION_1)
                case TLSVersion::Type::V1_0:
                    major = MBEDTLS_SSL_MAJOR_VERSION_3;
                    minor = MBEDTLS_SSL_MINOR_VERSION_1;
                    break;
#endif
#if defined(MBEDTLS_SSL_MAJOR_VERSION_3) && defined(MBEDTLS_SSL_MINOR_VERSION_2)
                case TLSVersion::Type::V1_1:
                    major = MBEDTLS_SSL_MAJOR_VERSION_3;
                    minor = MBEDTLS_SSL_MINOR_VERSION_2;
                    break;
#endif
#if defined(MBEDTLS_SSL_MAJOR_VERSION_3) && defined(MBEDTLS_SSL_MINOR_VERSION_3)
                default:
                case TLSVersion::Type::V1_2:
                    major = MBEDTLS_SSL_MAJOR_VERSION_3;
                    minor = MBEDTLS_SSL_MINOR_VERSION_3;
                    break;
#endif
                }
                mbedtls_ssl_conf_min_version(sslconf, major, minor);
#if 0
    /* force TLS 1.2 as maximum version (debugging only, disable in production) */
    /* This is basically is the same as tls-version-max that OpenVPN 2.x has but hardcoded */
	    mbedtls_ssl_conf_max_version(sslconf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
#endif
#endif


                {
                    // peer must present a valid certificate unless SSLConst::NO_VERIFY_PEER.
                    // Presenting a valid certificate can be made optional by specifying
                    // SSL:Const::PEER_CERT_OPTIONAL

                    int authmode;

                    if (c.flags & SSLConst::NO_VERIFY_PEER)
                        authmode = MBEDTLS_SSL_VERIFY_NONE;
                    else if (c.flags & SSLConst::PEER_CERT_OPTIONAL)
                        throw MbedTLSException("Optional peer verification not supported");
                    else
                        authmode = MBEDTLS_SSL_VERIFY_REQUIRED;

                    mbedtls_ssl_conf_authmode(sslconf, authmode);
                }

                // set verify callback
                mbedtls_ssl_conf_verify(sslconf, c.mode.is_server() ? verify_callback_server : verify_callback_client, this);

                // Notes on SSL resume/renegotiation:
                // SSL resume on server side is controlled by ssl_set_session_cache.
                // SSL renegotiation is disabled here via MBEDTLS_SSL_RENEGOTIATION_DISABLED
                // and ssl_legacy_renegotiation defaults to
                // MBEDTLS_SSL_LEGACY_NO_RENEGOTIATION.  To enable session tickets,
                // MBEDTLS_SSL_SESSION_TICKETS (compile flag) must be defined
                // in mbed TLS config.h.
                mbedtls_ssl_conf_renegotiation(sslconf, MBEDTLS_SSL_RENEGOTIATION_DISABLED);

                if (!c.tls_cipher_list.empty())
                {
                    set_mbedtls_cipherlist(c.tls_cipher_list);
                }
                else
                {
#if MBEDTLS_VERSION_NUMBER < 0x03000000
                    /* With newer versions we trust the default */
                    mbedtls_ssl_conf_ciphersuites(sslconf, mbedtls_ctx_private::ciphersuites);
#endif
                }

                if (!c.tls_groups.empty())
                {
                    set_mbedtls_groups(c.tls_groups);
                }
                // set CA chain
                if (c.ca_chain)
                    mbedtls_ssl_conf_ca_chain(sslconf,
                                              c.ca_chain->get(),
                                              c.crl_chain ? c.crl_chain->get() : nullptr);
                else if (!(c.flags & SSLConst::NO_VERIFY_PEER))
                    throw MbedTLSException("CA chain not defined");

                // Set hostname for SNI or if a CA chain is configured
                // Otherwise set the hostname explicitly to null to avoid
                // MBEDTLS_ERR_SSL_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME
                bool use_hostname = hostname && ((c.flags & SSLConst::ENABLE_CLIENT_SNI) || c.ca_chain);
                if (mbedtls_ssl_set_hostname(ssl, use_hostname ? hostname : nullptr))
                    throw MbedTLSException("mbedtls_ssl_set_hostname failed");

                // client cert+key
                if (c.local_cert_enabled)
                {
                    if (c.external_pki)
                    {
                        // set our own certificate, supporting chain (i.e. extra-certs), and external private key
                        if (c.crt_chain)
                        {
                            if (mbedtls_pk_get_type(&c.crt_chain.get()->get()->pk) == MBEDTLS_PK_RSA)
                            {
                                epki_ctx.epki_enable(ctx, epki_decrypt, epki_sign, epki_key_len);
                                mbedtls_ssl_conf_own_cert(sslconf, c.crt_chain->get(), epki_ctx.get());
                            }
                            else
                            {
                                throw MbedTLSException("cert has unsupported type for external pki support");
                            }
                        }
                        else
                            throw MbedTLSException("cert is undefined");
                    }
                    else
                    {
                        // set our own certificate, supporting chain (i.e. extra-certs), and private key
                        if (c.crt_chain && c.priv_key)
                            mbedtls_ssl_conf_own_cert(sslconf, c.crt_chain->get(), c.priv_key->get());
                        else
                            throw MbedTLSException("cert and/or private key is undefined");
                    }
                }

                // set DH
                if (c.dh)
                {
                    status = mbedtls_ssl_conf_dh_param_ctx(sslconf, c.dh->get());
                    if (status < 0)
                        throw MbedTLSException("error in ssl_set_dh_param_ctx", status);
                }

                // configure ciphertext buffers
                ct_in.set_frame(c.frame);
                ct_out.set_frame(c.frame);

                // set BIO
                mbedtls_ssl_set_bio(ssl, this, ct_write_func, ct_read_func, NULL);

                // set RNG
                if (c.rng)
                {
                    rng = c.rng;
                    mbedtls_ssl_conf_rng(sslconf, rng_callback, this);
                }
                else
                    throw MbedTLSException("RNG not defined");

                // set debug callback
                if (c.ssl_debug_level)
                    mbedtls_ssl_conf_dbg(sslconf, dbg_callback, ctx);

                /* OpenVPN 2.x disables cbc_record_splitting by default, therefore
                 * we have to do the same here to keep compatibility.
                 * If not disabled, this setting will trigger bad behaviours on
                 * TLS1.0 and possibly on other setups */
#if defined(MBEDTLS_SSL_CBC_RECORD_SPLITTING)
                mbedtls_ssl_conf_cbc_record_splitting(sslconf,
                                                      MBEDTLS_SSL_CBC_RECORD_SPLITTING_DISABLED);
#endif /* MBEDTLS_SSL_CBC_RECORD_SPLITTING */

                // Apply the configuration to the SSL connection object
                if (mbedtls_ssl_setup(ssl, sslconf) < 0)
                    throw MbedTLSException("mbedtls_ssl_setup failed");
            }
            catch (...)
            {
                erase();
                throw;
            }
        }

        mbedtls_ssl_config *sslconf;                       // SSL configuration parameters for SSL connection object
        std::unique_ptr<int[]> allowed_ciphers;            //! Hold the array that is used for setting the allowed ciphers
                                                           // must have the same lifetime as sslconf
        std::unique_ptr<mbedtls_compat_group_id[]> groups; //! Hold the array that is used for setting the curves


        MbedTLSContext *parent;

      private:
        void set_mbedtls_cipherlist(const std::string &cipher_list)
        {
            auto num_ciphers = std::count(cipher_list.begin(), cipher_list.end(), ':') + 1;

            allowed_ciphers.reset(new int[num_ciphers + 1]);

            std::stringstream cipher_list_ss(cipher_list);
            std::string ciphersuite;

            int i = 0;
            while (std::getline(cipher_list_ss, ciphersuite, ':'))
            {
                const tls_cipher_name_pair *pair = tls_get_cipher_name_pair(ciphersuite);

                if (pair && pair->iana_name != ciphersuite)
                {
                    OVPN_LOG_INFO("mbed TLS -- Deprecated cipher suite name '"
                                  << pair->openssl_name << "' please use IANA name ' "
                                  << pair->iana_name << "'");
                }

                auto cipher_id = mbedtls_ssl_get_ciphersuite_id(ciphersuite.c_str());
                if (cipher_id != 0)
                {
                    allowed_ciphers[i] = cipher_id;
                    i++;
                }
                else
                {
                    /* OpenVPN 2.x ignores silently ignores unknown cipher suites with
                     * mbed TLS. We warn about them in OpenVPN 3.x */
                    OVPN_LOG_INFO("mbed TLS -- warning ignoring unknown cipher suite '"
                                  << ciphersuite << "' in tls-cipher");
                }
            }

            // Last element needs to be null
            allowed_ciphers[i] = 0;
            mbedtls_ssl_conf_ciphersuites(sslconf, allowed_ciphers.get());
        }

        void set_mbedtls_groups(const std::string &tls_groups)
        {
            auto num_groups = std::count(tls_groups.begin(), tls_groups.end(), ':') + 1;

            /* add extra space for sentinel at the end */
            groups.reset(new mbedtls_compat_group_id[num_groups + 1]);

            std::stringstream groups_ss(tls_groups);
            std::string group;

            int i = 0;
            while (std::getline(groups_ss, group, ':'))
            {
                const mbedtls_ecp_curve_info *ci = mbedtls_ecp_curve_info_from_name(group.c_str());

                if (ci)
                {
                    groups[i] = mbedtls_compat_get_group_id(ci);
                    i++;
                }
                else
                {
                    OVPN_LOG_INFO("mbed TLS -- warning ignoring unknown group '"
                                  << group << "' in tls-groups");
                }
            }

            groups[i] = mbedtls_compat_group_id(0);
            mbedtls_ssl_conf_groups(sslconf, groups.get());
        }

        // cleartext read callback
        static int ct_read_func(void *arg, unsigned char *data, size_t length)
        {
            try
            {
                SSL *self = (SSL *)arg;
                const size_t actual = self->ct_in.read(data, length);
                return actual > 0 ? (int)actual : CT_WOULD_BLOCK;
            }
            catch (...)
            {
                return CT_INTERNAL_ERROR;
            }
        }

        // cleartext write callback
        static int ct_write_func(void *arg, const unsigned char *data, size_t length)
        {
            try
            {
                SSL *self = (SSL *)arg;
                self->ct_out.write(data, length);
                return (int)length;
            }
            catch (...)
            {
                return CT_INTERNAL_ERROR;
            }
        }

        // RNG callback -- return random data to mbed TLS
        static int rng_callback(void *arg, unsigned char *data, size_t len)
        {
            SSL *self = (SSL *)arg;
            return self->rng->rand_bytes_noexcept(data, len)
                       ? 0
                       : -1; // using -1 as a general-purpose mbed TLS error code
        }

        static void dbg_callback(void *arg, int level, const char *filename, int linenum, const char *text)
        {
            MbedTLSContext *self = (MbedTLSContext *)arg;
            if (level <= self->config->ssl_debug_level)
                OPENVPN_LOG_NTNL("mbed TLS[" << filename << ":" << linenum << " " << level << "]: "
                                             << text);
        }

        void clear()
        {
            parent = nullptr;
            ssl = nullptr;
            sslconf = nullptr;
            overflow = false;
            allowed_ciphers = nullptr;
        }

        void erase()
        {
            if (ssl)
            {
                mbedtls_ssl_free(ssl);
                mbedtls_ssl_config_free(sslconf);
                delete ssl;
                delete sslconf;
            }
            clear();
        }

        mbedtls_ssl_context *ssl;       // underlying SSL connection object
        MbedTLSPKI::PKContext epki_ctx; // external PKI context
        RandomAPI::Ptr rng;             // random data source
        MemQStream ct_in;               // write ciphertext to here
        MemQStream ct_out;              // read ciphertext from here
        AuthCert::Ptr authcert;
        bool overflow;
    };

    /////// start of main class implementation

    // create a new SSL instance
    SSLAPI::Ptr ssl() override
    {
        return SSL::Ptr(new SSL(this, nullptr));
    }

    // Get the library context. This currently does not exist for mbed TLS
    SSLLib::Ctx libctx() override
    {
        return nullptr;
    }

    // like ssl() above but verify hostname against cert CommonName and/or SubjectAltName
    SSLAPI::Ptr ssl(const std::string *hostname, const std::string *cache_key) override
    {
        return SSL::Ptr(new SSL(this, hostname ? hostname->c_str() : nullptr));
    }

    const Mode &mode() const override
    {
        return config->mode;
    }

    virtual ~MbedTLSContext()
    {
        erase();
    }

    constexpr static bool support_key_material_export()
    {
        /* mbed TLS 2.18+ can support RFC5705 but the API is painful to use */
        return false;
    }

#ifdef UNIT_TEST
    static void load_cert_info_into_authcert(AuthCert &authcert, const std::string &cert_txt)
    {
        const MbedTLSPKI::X509Cert cert(cert_txt, "MbedTLSContext::load_cert_info_into_authcert", true);

        // save the Common Name
        authcert.cn = MbedTLSPKI::x509_get_common_name(cert.get());

        // save the issuer fingerprint
        load_issuer_fingerprint_into_authcert(authcert, cert.get());

        // save the leaf cert serial number
        load_serial_number_into_authcert(authcert, cert.get());

        authcert.defined_ = true;
    }
#endif

  protected:
    MbedTLSContext(Config *config_arg)
        : config(config_arg)
    {
        if (config->local_cert_enabled)
        {
            // Verify that cert is defined
            if (!config->crt_chain)
                throw MbedTLSException("cert is undefined");
        }
    }

  private:
    size_t key_len() const
    {
        return mbedtls_pk_get_bitlen(&config->crt_chain->get()->pk) / 8;
    }

    // ns-cert-type verification

    bool ns_cert_type_defined() const
    {
        return config->ns_cert_type != NSCert::NONE;
    }

    bool verify_ns_cert_type(const mbedtls_x509_crt *cert) const
    {
#if MBEDTLS_VERSION_NUMBER < 0x03000000
        if (config->ns_cert_type == NSCert::SERVER)
            return bool(cert->ns_cert_type & MBEDTLS_X509_NS_CERT_TYPE_SSL_SERVER);
        else if (config->ns_cert_type == NSCert::CLIENT)
            return bool(cert->ns_cert_type & MBEDTLS_X509_NS_CERT_TYPE_SSL_CLIENT);
        else
#endif
            return false;
    }

    // remote-cert-ku verification

    bool x509_cert_ku_defined() const
    {
        return config->ku.size() > 0;
    }

    bool verify_x509_cert_ku(const mbedtls_x509_crt *cert)
    {
        if (mbedtls_x509_crt_has_ext_type(cert, MBEDTLS_OID_X509_EXT_EXTENDED_KEY_USAGE))
        {
            for (std::vector<unsigned int>::const_iterator i = config->ku.begin(); i != config->ku.end(); ++i)
            {
                if (mbedtls_x509_crt_check_key_usage(cert, *i))
                    return true;
            }
        }
        return false;
    }

    // remote-cert-eku verification

    bool x509_cert_eku_defined() const
    {
        return !config->eku.empty();
    }

    bool verify_x509_cert_eku(mbedtls_x509_crt *cert)
    {
        if (mbedtls_x509_crt_has_ext_type(cert, MBEDTLS_OID_X509_EXT_EXTENDED_KEY_USAGE))
        {
            mbedtls_x509_sequence *oid_seq = &cert->ext_key_usage;
            while (oid_seq != nullptr)
            {
                mbedtls_x509_buf *oid = &oid_seq->buf;

                // first compare against description
                {
                    const char *oid_str;
                    const int status = mbedtls_oid_get_extended_key_usage(oid, &oid_str);
                    if (status >= 0 && config->eku == oid_str)
                        return true;
                }

                // next compare against OID numeric string
                {
                    char oid_num_str[256];
                    const int status = mbedtls_oid_get_numeric_string(oid_num_str, sizeof(oid_num_str), oid);
                    if (status >= 0 && config->eku == oid_num_str)
                        return true;
                }
                oid_seq = oid_seq->next;
            }
        }
        return false;
    }

    static std::string status_string(const mbedtls_x509_crt *cert, const int depth, const uint32_t *flags)
    {
        std::ostringstream os;
        std::string status_str = "OK";
        if (*flags)
            status_str = "FAIL -- " + MbedTLSException::mbedtls_verify_flags_errtext(*flags);
        os << "VERIFY "
           << status_str
           << " : depth=" << depth
           << std::endl
           << cert_info(cert);
        return os.str();
    }

  protected:
    static int verify_callback_client(void *arg, mbedtls_x509_crt *cert, int depth, uint32_t *flags)
    {
        MbedTLSContext::SSL *ssl = (MbedTLSContext::SSL *)arg;
        MbedTLSContext *self = ssl->parent;
        bool fail = false;

        // log status
        if (self->config->flags & SSLConst::LOG_VERIFY_STATUS)
            OVPN_LOG_INFO(status_string(cert, depth, flags));

        // notify if connection is happening with an insecurely signed cert.

        // mbed TLS 3.0 does not allow the weaker signatures by default and also does not give a
        // proper accessor to these fields anymore
#if MBEDTLS_VERSION_NUMBER < 0x03000000
        if (cert->sig_md == MBEDTLS_MD_MD5)
        {
            ssl->tls_warnings |= SSLAPI::TLS_WARN_SIG_MD5;
        }

        if (cert->sig_md == MBEDTLS_MD_SHA1)
        {
            ssl->tls_warnings |= SSLAPI::TLS_WARN_SIG_SHA1;
        }
#endif

        // leaf-cert verification
        if (depth == 0)
        {
            // verify ns-cert-type
            if (self->ns_cert_type_defined() && !self->verify_ns_cert_type(cert))
            {
                OVPN_LOG_INFO("VERIFY FAIL -- bad ns-cert-type in leaf certificate");
                fail = true;
            }

            // verify X509 key usage
            if (self->x509_cert_ku_defined() && !self->verify_x509_cert_ku(cert))
            {
                OVPN_LOG_INFO("VERIFY FAIL -- bad X509 key usage in leaf certificate");
                fail = true;
            }

            // verify X509 extended key usage
            if (self->x509_cert_eku_defined() && !self->verify_x509_cert_eku(cert))
            {
                OVPN_LOG_INFO("VERIFY FAIL -- bad X509 extended key usage in leaf certificate");
                fail = true;
            }

            // verify tls-remote
            if (!self->config->tls_remote.empty())
            {
                const std::string subject = TLSRemote::sanitize_x509_name(MbedTLSPKI::x509_get_subject(cert));
                const std::string common_name = TLSRemote::sanitize_common_name(MbedTLSPKI::x509_get_common_name(cert));
                TLSRemote::log(self->config->tls_remote, subject, common_name);
                if (!TLSRemote::test(self->config->tls_remote, subject, common_name))
                {
                    OVPN_LOG_INFO("VERIFY FAIL -- tls-remote match failed");
                    fail = true;
                }
            }

            // verify-x509-name
            const VerifyX509Name &verify_x509 = self->config->verify_x509_name;
            if (verify_x509.get_mode() != VerifyX509Name::VERIFY_X509_NONE)
            {
                bool res = false;
                switch (verify_x509.get_mode())
                {
                case VerifyX509Name::VERIFY_X509_SUBJECT_DN:
                    res = verify_x509.verify(MbedTLSPKI::x509_get_subject(cert, true));
                    break;

                case VerifyX509Name::VERIFY_X509_SUBJECT_RDN:
                case VerifyX509Name::VERIFY_X509_SUBJECT_RDN_PREFIX:
                    res = verify_x509.verify(MbedTLSPKI::x509_get_common_name(cert));
                    break;

                default:
                    break;
                }
                if (!res)
                {
                    OVPN_LOG_INFO("VERIFY FAIL -- verify-x509-name failed");
                    fail = true;
                }
            }
        }

        if (fail)
            *flags |= MBEDTLS_X509_BADCERT_OTHER;
        return 0;
    }

    static int verify_callback_server(void *arg, mbedtls_x509_crt *cert, int depth, uint32_t *flags)
    {
        MbedTLSContext::SSL *ssl = (MbedTLSContext::SSL *)arg;
        MbedTLSContext *self = ssl->parent;
        bool fail = false;

        if (depth == 1) // issuer cert
        {
            // save the issuer cert fingerprint
            if (ssl->authcert)
            {
                if (!load_issuer_fingerprint_into_authcert(*ssl->authcert, cert))
                {
                    OVPN_LOG_INFO("VERIFY FAIL -- SHA1 calculation failed.");
                    fail = true;
                }
            }
        }
        else if (depth == 0) // leaf-cert
        {
            // verify ns-cert-type
            if (self->ns_cert_type_defined() && !self->verify_ns_cert_type(cert))
            {
                OVPN_LOG_INFO("VERIFY FAIL -- bad ns-cert-type in leaf certificate");
                fail = true;
            }

            // verify X509 key usage
            if (self->x509_cert_ku_defined() && !self->verify_x509_cert_ku(cert))
            {
                OVPN_LOG_INFO("VERIFY FAIL -- bad X509 key usage in leaf certificate");
                fail = true;
            }

            // verify X509 extended key usage
            if (self->x509_cert_eku_defined() && !self->verify_x509_cert_eku(cert))
            {
                OVPN_LOG_INFO("VERIFY FAIL -- bad X509 extended key usage in leaf certificate");
                fail = true;
            }

            if (ssl->authcert)
            {
                // save the Common Name
                ssl->authcert->cn = MbedTLSPKI::x509_get_common_name(cert);

                // save the leaf cert serial number
                load_serial_number_into_authcert(*ssl->authcert, cert);

                ssl->authcert->defined_ = true;
            }
        }

        if (fail)
            *flags |= MBEDTLS_X509_BADCERT_OTHER;
        return 0;
    }

    Config::Ptr config;

  private:
    static std::string cert_info(const mbedtls_x509_crt *cert, const char *prefix = nullptr)
    {
        const size_t buf_size = 4096;
        std::unique_ptr<char[]> buf(new char[buf_size]);
        const int size = mbedtls_x509_crt_info(buf.get(), buf_size, prefix ? prefix : "", cert);
        if (size >= 0)
            return std::string(buf.get());
        else
            return "error rendering cert";
    }

    void erase()
    {
    }

    static int epki_decrypt(void *arg,
#if MBEDTLS_VERSION_NUMBER < 0x03000000
                            int mode,
#endif
                            size_t *olen,
                            const unsigned char *input,
                            unsigned char *output,
                            size_t output_max_len)
    {
        OVPN_LOG_INFO("MbedTLSContext::epki_decrypt is unimplemented"
                      << " output_max_len=" << output_max_len);

        return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;
    }

    static int epki_sign(void *arg,
                         int (*f_rng)(void *, unsigned char *, size_t),
                         void *p_rng,
#if MBEDTLS_VERSION_NUMBER < 0x03000000
                         int mode,
#endif
                         mbedtls_md_type_t md_alg,
                         unsigned int hashlen,
                         const unsigned char *hash,
                         unsigned char *sig)
    {
        MbedTLSContext *self = (MbedTLSContext *)arg;
        try
        {
#if MBEDTLS_VERSION_NUMBER < 0x03000000
            if (mode == MBEDTLS_RSA_PRIVATE)
#else
            if (true)
#endif
            {
                size_t digest_prefix_len = 0;
                const unsigned char *digest_prefix = nullptr;

                /* get signature type */
                switch (md_alg)
                {
                case MBEDTLS_MD_NONE:
                    break;
                case MBEDTLS_MD_MD5:
                    digest_prefix = PKCS1::DigestPrefix::MD5;
                    digest_prefix_len = sizeof(PKCS1::DigestPrefix::MD5);
                    break;
                case MBEDTLS_MD_SHA1:
                    digest_prefix = PKCS1::DigestPrefix::SHA1;
                    digest_prefix_len = sizeof(PKCS1::DigestPrefix::SHA1);
                    break;
                case MBEDTLS_MD_SHA256:
                    digest_prefix = PKCS1::DigestPrefix::SHA256;
                    digest_prefix_len = sizeof(PKCS1::DigestPrefix::SHA256);
                    break;
                case MBEDTLS_MD_SHA384:
                    digest_prefix = PKCS1::DigestPrefix::SHA384;
                    digest_prefix_len = sizeof(PKCS1::DigestPrefix::SHA384);
                    break;
                case MBEDTLS_MD_SHA512:
                    digest_prefix = PKCS1::DigestPrefix::SHA512;
                    digest_prefix_len = sizeof(PKCS1::DigestPrefix::SHA512);
                    break;
                default:
                    OVPN_LOG_INFO("MbedTLSContext::epki_sign unrecognized hash_id"
#if MBEDTLS_VERSION_NUMBER < 0x03000000
                                  << "mode=" << mode
#endif
                                  << " md_alg=" << md_alg << " hashlen=" << hashlen);
                    return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;
                }

                /* concatenate digest prefix with hash */
                BufferAllocated from_buf(digest_prefix_len + hashlen);
                if (digest_prefix_len)
                    from_buf.write(digest_prefix, digest_prefix_len);
                from_buf.write(hash, hashlen);

                /* convert from_buf to base64 */
                const std::string from_b64 = base64->encode(from_buf);

                /* get signature */
                std::string sig_b64;
                const bool status = self->config->external_pki->sign(self->config->external_pki_alias, from_b64, sig_b64, "RSA_PKCS1_PADDING", "", "");
                if (!status)
                    throw ssl_external_pki("MbedTLS: could not obtain signature");

                /* decode base64 signature to binary */
                const size_t len = self->key_len();
                Buffer sigbuf(sig, len, false);
                base64->decode(sigbuf, sig_b64);

                /* verify length */
                if (sigbuf.size() != len)
                    throw ssl_external_pki("mbed TLS: incorrect signature length");

                /* success */
                return 0;
            }
            else
            {
                OVPN_LOG_INFO("MbedTLSContext::epki_sign unrecognized parameters"
#if MBEDTLS_VERSION_NUMBER < 0x03000000
                              << "mode=" << mode
#endif
                              << " md_alg=" << md_alg << " hashlen=" << hashlen);
                return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;
            }
        }
        catch (const std::exception &e)
        {
            OPENVPN_LOG("MbedTLSContext::epki_sign exception: " << e.what());
            return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;
        }
    }

    static size_t epki_key_len(void *arg)
    {
        MbedTLSContext *self = (MbedTLSContext *)arg;
        return self->key_len();
    }

    static void load_serial_number_into_authcert(AuthCert &authcert, const mbedtls_x509_crt *cert)
    {
        const mbedtls_x509_buf *s = &cert->serial;
        if (s->len > 0 && s->len <= authcert.serial.size())
        {
            const size_t offset = authcert.serial.size() - s->len;
            std::memset(authcert.serial.number(), 0, offset);
            std::memcpy(authcert.serial.number() + offset, s->p, s->len);
        }
    }

    static bool load_issuer_fingerprint_into_authcert(AuthCert &authcert, const mbedtls_x509_crt *cert)
    {
        const int SHA_DIGEST_LEN = 20;
        static_assert(sizeof(AuthCert::issuer_fp) == SHA_DIGEST_LEN, "size inconsistency");
#if MBEDTLS_VERSION_NUMBER < 0x02070000
        // mbed TLS 2.7.0 and newer deprecates mbedtls_sha1()
        // in favour of mbedtls_sha1_ret().

        // We support for older mbed TLS versions
        // to be able to build on Debian 9 and Ubuntu 16.
        mbedtls_sha1(cert->raw.p, cert->raw.len, authcert->issuer_fp);
#elif MBEDTLS_VERSION_NUMBER < 0x03000000
        if (mbedtls_sha1_ret(cert->raw.p, cert->raw.len, authcert.issuer_fp))
            return false;
#else
        // mbedtls_sha1_ret is renamed to mbedtls_sha1 in 3.0
        if (mbedtls_sha1(cert->raw.p, cert->raw.len, authcert.issuer_fp))
            return false;
#endif
        return true;
    }
};

inline const std::string get_ssl_library_version()
{
    unsigned int ver = mbedtls_version_get_number();
    std::string version = "mbed TLS "
                          + std::to_string((ver >> 24) & 0xff)
                          + "." + std::to_string((ver >> 16) & 0xff)
                          + "." + std::to_string((ver >> 8) & 0xff);

    return version;
}
} // namespace openvpn

#endif
