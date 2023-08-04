/*
 * cgi.h
 *
 *  Created on: 28.07.2023
 *      Author: alexey slovesnov
 * copyright(c/c++): 2014-doomsday
 *           E-mail: slovesnov@yandex.ru
 *         homepage: slovesnov.users.sourceforge.net
 */

#ifndef CGI_H_
#define CGI_H_

#include <iostream>
#include <string>
#include <vector>
#include <iconv.h>
#include <cstring>
#include <cassert>
#include <algorithm>

class CgiParameters: public std::vector<std::pair<std::string, std::string>> {
public:
	using VString=std::vector<std::string>;
	using VSS=std::vector<std::pair<std::string, std::string>>;

	VSS::const_iterator index(std::string const &s) const;

	//has or not key
	bool has(std::string const &s) const;
	std::string const& key(size_t i) const;
	std::string const& value(size_t i) const;
	std::string const& value(std::string const &s) const;
	std::string const& operator[](std::string const &s) const {
		return value(s);
	}

	//since operator [] is redefined need redefine normal [] operator
	std::pair<std::string, std::string>const& operator[](size_t i) const {
		return VSS::operator[](i);
	}

	CgiParameters const& operator=(VSS const &v) {
		VSS::operator=(v);
		return *this;
	}
};

class Cgi: public CgiParameters {
public:
	std::string m_status;
	std::string m_method;
	CgiParameters m_cookie;
	Cgi();
	bool ok() const;

	bool emptyCookie()const{
		return m_cookie.empty();
	}

	size_t sizeCookie()const{
		return m_cookie.size();
	}

	bool hasCookie(std::string const &s) const {
		return m_cookie.has(s);
	}

	std::string const& keyCookie(size_t i) const {
		return m_cookie.key(i);
	}

	std::string const& valueCookie(size_t i) const {
		return m_cookie.value(i);
	}

	std::string const& valueCookie(std::string const &s) const {
		return m_cookie.value(s);
	}

	std::string const& getErrorMessage() const {
		return m_status;
	}

	static VSS parse(std::string const &s, int type = 0);
	static VString split(const std::string &subject,
			const std::string &separator);
	static VString split(const std::string &subject,
			const char separator = ' ');
	static std::string encode(const std::string &s, bool toUtf8);
};

#endif /* CGI_H_ */
