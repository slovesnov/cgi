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
#ifdef _WIN32
#include <fcntl.h>
#endif

static const std::string CGI_OK = "OK";
#ifdef _WIN32
const char SEP = '\\';
#else
const char SEP='/';
#endif
static const std::string MULTIPART = "multipart/form-data";

#define SHORT_FILENAME (strrchr(__FILE__, SEP) ? strrchr(__FILE__, SEP) + 1 : __FILE__)
#define CREATE_ERROR(s) m_status="error "+std::string(s)+" file:"+SHORT_FILENAME+" line:"+std::to_string(__LINE__);
#define CREATE_ERROR_RETURN_FALSE(s) CREATE_ERROR(s);return false;
#define CREATE_ERROR_PRINT_RETURN(s) CREATE_ERROR(s);std::cout<<m_status;return;

Cgi::Cgi() {
	std::string s;
	m_status = CGI_OK;
	const char *p, *p1;
	p = getenv("REQUEST_METHOD");
	if (!p) {
		//not output error, because possible user needs non cgi mode
		CREATE_ERROR("REQUEST_METHOD not found");
		return;
	}
	m_method = p;
	//now in cgi mode always output error
	std::cout << "Content-type: text/html;charset=utf-8\n\n";

	//already in cgi mode output header even in case of errors
	if (m_method == "POST") {
#ifdef _WIN32
		//otherwise stops on 0x1a
		_setmode(_fileno( stdin), _O_BINARY);
#endif
		p = getenv("CONTENT_TYPE");
		if (!p) {
			CREATE_ERROR_PRINT_RETURN("CONTENT_TYPE not found")
		}

		p1 = strchr(p, ';');
		if (p1) {
			m_contentType = std::string(p, p1 - p);
			if (m_contentType == MULTIPART) {
				const char BOUNDARY[] = "boundary=";
				p = strstr(p1, BOUNDARY);
				if (!p) {
					CREATE_ERROR_PRINT_RETURN("boundary not found")
				}
				m_boundary = p + strlen(BOUNDARY);
			}
		} else {
			m_contentType = std::string(p);
		}

		s = "";
		const int L = 4096;
		char b[L];
		int bytes;
		while ((bytes = fread(b, 1, L, stdin))) {
			s += std::string(b, bytes);
		}
	} else if (m_method == "GET") {
		p = getenv("QUERY_STRING");
		if (!p) {
			CREATE_ERROR_PRINT_RETURN("QUERY_STRING not found")
			return;
		}
		s = p;
	} else {
		CREATE_ERROR_PRINT_RETURN("no post or get method")
		return;
	}
	if (m_contentType == MULTIPART) {
		if (!parseBoundary(s)) {
			std::cout << m_status;
			return;
		}
	} else {
		VSS::operator =(parse(s, 0));
	}

	p = getenv("HTTP_COOKIE");
	if (!p) {
		CREATE_ERROR_PRINT_RETURN("HTTP_COOKIE not found")
		return;
	}
	s = p;
	m_cookie = parse(s, 1);
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

bool Cgi::parseBoundary(const std::string &s) {
	const std::string name = "name=\"";
	const std::string filename = "filename=\"";
	const std::string ct = "Content-Type: ";
	const std::string eol = "\r\n";

	const std::string sbegin = "--" + m_boundary + eol;
	const std::string send = eol + "--" + m_boundary + "--" + eol;
	const size_t start = sbegin.length();
	const size_t end = send.length();

	size_t p, p1, ps;
	std::string n, v, e1, type, fname;
	CgiFile file;
	if (s.substr(0, start) != sbegin) {
		CREATE_ERROR_RETURN_FALSE("invalid start");
	}
	if (s.substr(s.length() - end) != send) {
		CREATE_ERROR_RETURN_FALSE("invalid end");
	}

	auto ve = split(s.substr(start, s.length() - end - start), eol + sbegin);
	for (auto e : ve) {
		p = ps = e.find("\n");
		if (p == std::string::npos) {
			CREATE_ERROR_RETURN_FALSE("no new line in parameter header found");
		}

		e1 = e.substr(0, p);

		p = e1.find(name);
		if (p == std::string::npos) {
			CREATE_ERROR_RETURN_FALSE("no name=\" found");
		}
		p += name.length();
		p1 = e1.find('"', p);
		if (p1 == std::string::npos) {
			CREATE_ERROR_RETURN_FALSE("\" after name found");
		}
		n = e1.substr(p, p1 - p);

		p = e1.find(filename);
		if (p == std::string::npos) {
			p = e.find(eol + eol);
			if (p == std::string::npos) {
				CREATE_ERROR_RETURN_FALSE("no double eol found");
			}
			v = e.substr(p + 2 * eol.length());
			push_back( { n, v });
		} else {
			p += filename.length();
			p1 = e1.find('"', p);
			if (p1 == std::string::npos) {
				CREATE_ERROR_RETURN_FALSE("no file name close quote found");
			}
			if (p1 != p) { //if p1==p no files not need to add
				file.name = e1.substr(p, p1 - p); //filename
				ps++;
				p1 = e.find("\r", ps);
				if (p1 == std::string::npos) {
					CREATE_ERROR_RETURN_FALSE("no 2nd line found");
				}
				file.type = e.substr(ps + ct.length(), p1 - ps - ct.length());

				p = e.find(eol + eol);
				if (p == std::string::npos) {
					CREATE_ERROR_RETURN_FALSE("no double eol found");
				}
				file.content = e.substr(p + 2 * eol.length());
				if (n.substr(n.length() - 2) == "[]") { //remove last [] like in php
					n = n.substr(0, n.length() - 2);
				}
				auto it = m_files.find(n);
				if (it == m_files.end()) {
					m_files.push_back( { n, { file } });
				} else {
					it->second.push_back(file);
				}
			}
		}

//		auto l=split(e,eol);
//		for(auto a:l){
//			std::cout<<" ["<<a<<"]"<<std::endl;
//		}
//		std::cout<<"###"<<n<<"="<<v<<" "<<type<<"###"<<std::endl;
	}
	return true;
}
