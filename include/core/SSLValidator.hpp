#pragma once

#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <cstring>

namespace chimera {

class SSLValidator {
public:

    static bool verify(X509* cert, const char* hostname) {

        if (!cert || !hostname)
            return false;

        // --- Check Subject Alternative Names (SAN) ---
        GENERAL_NAMES* names = static_cast<GENERAL_NAMES*>(
            X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr));

        if (names) {

            int name_count = sk_GENERAL_NAME_num(names);

            for (int i = 0; i < name_count; ++i) {

                const GENERAL_NAME* name = sk_GENERAL_NAME_value(names, i);

                if (name->type == GEN_DNS) {

                    const char* dns_name =
                        reinterpret_cast<const char*>(
                            ASN1_STRING_get0_data(name->d.dNSName));

                    if (!dns_name)
                        continue;

                    // --- Exact match ---
                    if (_stricmp(dns_name, hostname) == 0) {
                        GENERAL_NAMES_free(names);
                        return true;
                    }

                    // --- Wildcard match: *.domain.com ---
                    if (dns_name[0] == '*' && dns_name[1] == '.') {

                        const char* suffix = dns_name + 1;

                        size_t host_len = strlen(hostname);
                        size_t suffix_len = strlen(suffix);

                        if (host_len > suffix_len) {

                            const char* host_suffix =
                                hostname + (host_len - suffix_len);

                            if (_stricmp(host_suffix, suffix) == 0) {
                                GENERAL_NAMES_free(names);
                                return true;
                            }
                        }
                    }
                }
            }

            GENERAL_NAMES_free(names);
        }

        // --- Fallback to Common Name (CN) ---
        X509_NAME* subject = X509_get_subject_name(cert);

        if (subject) {

            char cn[256] = {0};

            if (X509_NAME_get_text_by_NID(
                    subject,
                    NID_commonName,
                    cn,
                    sizeof(cn)) > 0) {

                if (_stricmp(cn, hostname) == 0)
                    return true;
            }
        }

        return false;
    }
};

} // namespace chimera
