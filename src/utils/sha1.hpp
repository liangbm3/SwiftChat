// sha1.hpp
#ifndef SHA1_HPP
#define SHA1_HPP
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class SHA1 {
 public:
  SHA1() { reset(); }
  void update(const std::string &s) {
    update(reinterpret_cast<const uint8_t *>(s.c_str()), s.length());
  }
  void update(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
      buffer_[buffer_size_++] = data[i];
      if (buffer_size_ == 64) {
        transform(buffer_);
        buffer_size_ = 0;
      }
    }
  }
  std::vector<uint8_t> final() {
    uint64_t total_bits = (transforms_ * 64 + buffer_size_) * 8;
    buffer_[buffer_size_++] = 0x80;
    if (buffer_size_ > 56) {
      while (buffer_size_ < 64) buffer_[buffer_size_++] = 0;
      transform(buffer_);
      buffer_size_ = 0;
    }
    while (buffer_size_ < 56) buffer_[buffer_size_++] = 0;
    for (int i = 0; i < 8; ++i)
      buffer_[56 + i] = (uint8_t)(total_bits >> (56 - i * 8));
    transform(buffer_);

    std::vector<uint8_t> hash;
    hash.resize(20);
    for (int i = 0; i < 5; ++i) {
      hash[i * 4 + 0] = (digest_[i] >> 24) & 0xFF;
      hash[i * 4 + 1] = (digest_[i] >> 16) & 0xFF;
      hash[i * 4 + 2] = (digest_[i] >> 8) & 0xFF;
      hash[i * 4 + 3] = (digest_[i] >> 0) & 0xFF;
    }
    return hash;
  }

 private:
  void reset() {
    digest_[0] = 0x67452301;
    digest_[1] = 0xEFCDAB89;
    digest_[2] = 0x98BADCFE;
    digest_[3] = 0x10325476;
    digest_[4] = 0xC3D2E1F0;
    buffer_size_ = 0;
    transforms_ = 0;
  }
  static uint32_t rol(uint32_t value, size_t bits) {
    return (value << bits) | (value >> (32 - bits));
  }
  void transform(const uint8_t *buffer) {
    uint32_t m[80];
    for (int i = 0; i < 16; ++i)
      m[i] = (buffer[i * 4] << 24) | (buffer[i * 4 + 1] << 16) |
             (buffer[i * 4 + 2] << 8) | buffer[i * 4 + 3];
    for (int i = 16; i < 80; ++i)
      m[i] = rol(m[i - 3] ^ m[i - 8] ^ m[i - 14] ^ m[i - 16], 1);

    uint32_t a = digest_[0], b = digest_[1], c = digest_[2], d = digest_[3],
             e = digest_[4];
    for (int i = 0; i < 80; ++i) {
      uint32_t f, k;
      if (i < 20) {
        f = (b & c) | (~b & d);
        k = 0x5A827999;
      } else if (i < 40) {
        f = b ^ c ^ d;
        k = 0x6ED9EBA1;
      } else if (i < 60) {
        f = (b & c) | (b & d) | (c & d);
        k = 0x8F1BBCDC;
      } else {
        f = b ^ c ^ d;
        k = 0xCA62C1D6;
      }
      uint32_t temp = rol(a, 5) + f + e + k + m[i];
      e = d;
      d = c;
      c = rol(b, 30);
      b = a;
      a = temp;
    }
    digest_[0] += a;
    digest_[1] += b;
    digest_[2] += c;
    digest_[3] += d;
    digest_[4] += e;
    transforms_++;
  }
  uint32_t digest_[5];
  uint8_t buffer_[64];
  size_t buffer_size_;
  uint64_t transforms_;
};
#endif