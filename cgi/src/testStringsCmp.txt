#include "cgi.h"

int main() {
	std::string s, q[2], t[2];
	int i;

	Cgi c;

	if (!c.ok()) {
		std::cout << c.getErrorMessage();
		return 1;
	}

	std::cout << c.m_method << "{" << std::endl;
	for (auto a : c) {
		std::cout << a.first << "=" << a.second << std::endl;
	}
	std::cout << "}";

	if (c.size() < 2) {
		std::cout << Cgi::encode("слишком мало параметров", true);
	} else {
		for (i = 0; i < 2; i++) {
			s = c.value(i);
			q[i] = s;
			std::sort(q[i].begin(), q[i].end());
			t[i] = Cgi::encode(s, false);
			std::sort(t[i].begin(), t[i].end());
		}
		std::cout << (q[0] == q[1]) << " " << (t[0] == t[1]);
	}
}
