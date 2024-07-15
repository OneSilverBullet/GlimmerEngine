#pragma once
#include <iostream>
#include <sstream>
#include <iomanip>
#include <array>
#include <vector>
#include <cstring>
#include <cstdint>



class GUUID {
public:
    GUUID(const std::string& name) {
        generateUUID(name);
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (size_t i = 0; i < 16; ++i) {
            oss << std::setw(2) << static_cast<int>(uuid[i]);
            if (i == 3 || i == 5 || i == 7 || i == 9) {
                oss << '-';
            }
        }
        return oss.str();
    }

private:
    void generateUUID(const std::string& name) {
        static const std::array<uint8_t, 16> namespaceUUID = {
            0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1,
            0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8
        };

        std::array<uint8_t, 20> hash = sha1(namespaceUUID, name);

        std::memcpy(uuid.data(), hash.data(), 16);

        uuid[6] = (uuid[6] & 0x0F) | 0x50; // Version 5
        uuid[8] = (uuid[8] & 0x3F) | 0x80; // Variant
    }

    std::array<uint8_t, 20> sha1(const std::array<uint8_t, 16>& namespaceUUID, const std::string& name) {
        static constexpr size_t BLOCK_SIZE = 64;
        static constexpr size_t HASH_SIZE = 20;

        std::array<uint8_t, BLOCK_SIZE> block = {};
        std::array<uint8_t, HASH_SIZE> digest = {};

        // Implement SHA-1 (minimal implementation for example)
        // This should be replaced with a proper SHA-1 library for production use

        // Initial hash values
        uint32_t h0 = 0x67452301;
        uint32_t h1 = 0xEFCDAB89;
        uint32_t h2 = 0x98BADCFE;
        uint32_t h3 = 0x10325476;
        uint32_t h4 = 0xC3D2E1F0;

        // Prepare message
        std::vector<uint8_t> message(namespaceUUID.begin(), namespaceUUID.end());
        message.insert(message.end(), name.begin(), name.end());
        uint64_t message_len = message.size() * 8;

        message.push_back(0x80);
        while ((message.size() % BLOCK_SIZE) != (BLOCK_SIZE - 8)) {
            message.push_back(0x00);
        }

        for (int i = 7; i >= 0; --i) {
            message.push_back((message_len >> (i * 8)) & 0xFF);
        }

        // Process each 512-bit block
        for (size_t i = 0; i < message.size(); i += BLOCK_SIZE) {
            std::array<uint32_t, 80> w = {};
            for (size_t j = 0; j < 16; ++j) {
                w[j] = (message[i + 4 * j] << 24) | (message[i + 4 * j + 1] << 16) | (message[i + 4 * j + 2] << 8) | message[i + 4 * j + 3];
            }

            for (size_t j = 16; j < 80; ++j) {
                w[j] = leftRotate(w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16], 1);
            }

            uint32_t a = h0;
            uint32_t b = h1;
            uint32_t c = h2;
            uint32_t d = h3;
            uint32_t e = h4;

            for (size_t j = 0; j < 80; ++j) {
                uint32_t f, k;
                if (j < 20) {
                    f = (b & c) | ((~b) & d);
                    k = 0x5A827999;
                }
                else if (j < 40) {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                }
                else if (j < 60) {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                }
                else {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }

                uint32_t temp = leftRotate(a, 5) + f + e + k + w[j];
                e = d;
                d = c;
                c = leftRotate(b, 30);
                b = a;
                a = temp;
            }

            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }

        for (int i = 0; i < 4; ++i) {
            digest[i] = (h0 >> (24 - i * 8)) & 0xFF;
            digest[i + 4] = (h1 >> (24 - i * 8)) & 0xFF;
            digest[i + 8] = (h2 >> (24 - i * 8)) & 0xFF;
            digest[i + 12] = (h3 >> (24 - i * 8)) & 0xFF;
            digest[i + 16] = (h4 >> (24 - i * 8)) & 0xFF;
        }

        return digest;
    }

    uint32_t leftRotate(uint32_t value, uint32_t count) const {
        return (value << count) | (value >> (32 - count));
    }

    std::array<uint8_t, 16> uuid = {};
};
