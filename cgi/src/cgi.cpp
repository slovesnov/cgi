/*
 * cgi.cpp
 *
 *  Created on: 28.07.2023
 *      Author: alexey slovesnov
 * copyright(c/c++): 2014-doomsday
 *           E-mail: slovesnov@yandex.ru
 *         homepage: slovesnov.users.sourceforge.net
 */

#include "cgi.h"

static const std::string CGI_OK = "OK";

Cgi::VSS::const_iterator CgiParameters::index(std::string const &s) const {
	//need to point out type of p for old g++
	return std::find_if(begin(), end(),
			[&s](std::pair<std::string, std::string> const &p) {
				return p.first == s;
			});
}

bool CgiParameters::has(std::string const &s) const {
	return index(s) != end();
}

std::string const& CgiParameters::value(std::string const &s) const {
	VSS::const_iterator it = index(s);
	assert(it!=end());
	return it->second;
}

std::string const& CgiParameters::key(size_t i) const {
	assert(i < size());
	return (*this)[i].first;
}

std::string const& CgiParameters::value(size_t i) const {
	assert(i < size());
	return (*this)[i].second;
}

Cgi::Cgi() {
	std::string s;
	m_status = CGI_OK;
	const char *p = getenv("REQUEST_METHOD");
	if (!p) {
		m_status = "REQUEST_METHOD not found";
		return;
	}
	//already in cgi mode output header even in case of errors
	std::cout << "Content-type: text/html;charset=utf-8\n\n";
	m_method = p;
	if (m_method == "POST") {
		std::getline(std::cin, s);
	} else if (m_method == "GET") {
		p = getenv("QUERY_STRING");
		if (!p) {
			m_status = "QUERY_STRING not found";
			return;
		}
		s = p;
	} else {
		m_status = "no post or get method";
		return;
	}
	VSS::operator =(parse(s, 0));

//	if (parseCookie) {
	p = getenv("HTTP_COOKIE");
	if (!p) {
		m_status = "HTTP_COOKIE not found";
		return;
	}
	s = p;
	m_cookie = parse(s, 1);
//	}
}

bool Cgi::ok() const {
	return m_status == CGI_OK;
}

Cgi::VSS Cgi::parse(std::string const &s, int type) {
	VSS v;
	int i;
	std::string t, q[2];
	const char *p;
	if (s.empty()) {
		return v;
	}
	for (auto const &a : split(s, type == 1 ? "; " : "&")) {
		auto b = split(a, '=');
		assert(b.size() == 2);
		i = 0;
		for (auto c : b) {
			p = c.c_str();
			std::string &t = q[i++];
			t = "";
			while (*p) {
				if (*p == '%') {
					p++;
					//can be "%7B1%2C which means "{1," so cann't use stoi directly
					t += stoi(std::string(p, 2), nullptr, 16);
					p += 2;
				} else {
					//' ' converted to '+'
					t += *p == '+' ? ' ' : *p;
					p++;
				}
			}
		}
		v.push_back( { q[0], q[1] });
	}
	return v;
}

Cgi::VString Cgi::split(const std::string &subject,
		const std::string &separator) {
	VString r;
	size_t pos, prev;
	for (prev = 0; (pos = subject.find(separator, prev)) != std::string::npos;
			prev = pos + separator.length()) {
		r.push_back(subject.substr(prev, pos - prev));
	}
	r.push_back(subject.substr(prev, subject.length()));
	return r;
}

Cgi::VString Cgi::split(const std::string &subject, const char separator) {
	return split(subject, std::string(1, separator));
}

std::string Cgi::encode(const std::string &s, bool toUtf8) {
	std::string r;
	const char UTF8[] = "utf-8";
	const char CP1251[] = "cp1251";
	iconv_t cd = toUtf8 ? iconv_open(UTF8, CP1251) : iconv_open(CP1251, UTF8);
	if ((iconv_t) -1 == cd) {
		printf("error %d", __LINE__);
		perror("iconv_open");
		return r;
	}

	size_t inbytesleft = s.length();
	char *in = new char[inbytesleft];
	strncpy(in, s.c_str(), inbytesleft);
	size_t outbytesleft = inbytesleft * 2 + 1;
	char *out = new char[outbytesleft];
	char *outbuf = out;
	size_t ret = iconv(cd, &in, &inbytesleft, &outbuf, &outbytesleft);
	if ((size_t) -1 == ret) {
		printf("error %d", __LINE__);
		perror("iconv");
		return r;
	}
	*outbuf = 0;
	r = out;
	delete[] out;
	iconv_close(cd);
	return r;
}
