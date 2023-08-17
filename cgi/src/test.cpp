#include "cgi/cgi.h"

using VUint = std::vector<uint32_t>;

VUint utf8ToVector(std::string const &s, bool sort) {
  VUint v;
  uint32_t b;
  int i;
  const uint8_t *p = (const uint8_t*) s.c_str();
  while (*p) {
    if ((*p & 0x80) == 0) {
      i = 0;
    } else if ((*p & 0xE0) == 0xC0) {
      i = 1;
    } else if ((*p & 0xF0) == 0xE0) {
      i = 2;
    } else if ((*p & 0xF8) == 0xF0) {
      i = 3;
    } else {
      std::cerr << "Unrecognized lead byte (" << std::hex << *p << ")"
          << std::endl;
      break;
    }
    for (b = 0; i >= 0; i--) {
      b |= *p++ << (i * 8);
    }
    v.push_back(b);
  }
  if (sort) {
    std::sort(v.begin(), v.end());
  }
  return v;
}

int main() {
  VUint t[2];
  int i;
  Cgi c;

  if (!c.ok()) {
    return 1;
  }

  std::cout << c.m_method << "{" << std::endl;
  for (auto a : c) {
    std::cout << a.first << "=" << a.second << std::endl;
  }
  std::cout << "}";

  if (c.size() < 2) {
    std::cout << "too few parameters";
  } else {
    for (i = 0; i < 2; i++) {
      t[i] = utf8ToVector(c.value(i), true);
    }
    std::cout << " " << (t[0] == t[1]);
    std::cout << c.m_contentType<<"\n";
    for (i = 0; i < 2; i++) {
      std::cout<<t[i].size() <<" "<<c.value(i).length()<<"\n";
    }
  }
}
