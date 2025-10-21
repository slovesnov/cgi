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

//also supports old gcc 4.8.5
//application/x-www-form-urlencoded and multipart/form-data
class CgiFile {
public:
	std::string name, type, content;
	//size=content.length
};

template<class T>
class CgiParametersTemplate: public std::vector<std::pair<std::string, T>> {
public:
	using VSS=std::vector<std::pair<std::string,T>>;

	typename VSS::const_iterator find(std::string const &s) const {
		return std::find_if(this->begin(), this->end(),
				[&s](std::pair<std::string, T> const &p) {
					return p.first == s;
				});
	}

	typename VSS::iterator find(std::string const &s) {
		return std::find_if(this->begin(), this->end(),
				[&s](std::pair<std::string, T> const &p) {
					return p.first == s;
				});
	}

	//has or not key
	bool has(std::string const &s) const {
		return find(s) != this->end();
	}

	std::string const& key(size_t i) const {
		assert(i < this->size());
		VSS const &v = *this;
		return v[i].first;
	}

	T const& value(size_t i) const {
		assert(i < this->size());
		VSS const &v = *this;
		return v[i].second;
	}

	T const& value(std::string const &s) const {
		typename VSS::const_iterator it = find(s);
		assert(it!=this->end());
		return it->second;
	}

	T const& operator[](size_t i) const {
		return value(i);
	}

	T const& operator[](std::string const &s) const {
		return value(s);
	}

	CgiParametersTemplate const& operator=(VSS const &v) {
		VSS::operator=(v);
		return *this;
	}
};

using CgiParameters = CgiParametersTemplate<std::string>;
using CgiFiles = CgiParametersTemplate<std::vector<CgiFile>>;

class Cgi: public CgiParameters {

public:
	std::string m_status, m_boundary, m_contentType, m_method;
	CgiParameters m_cookie;
	CgiFiles m_files;
	Cgi();
	bool ok() const;

	bool emptyCookie() const {
		return m_cookie.empty();
	}

	size_t sizeCookie() const {
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

	bool parseBoundary(std::string const &s);
	static VSS parse(std::string const &s, int type = 0);
	using VString=std::vector<std::string>;
	static VString split(const std::string &subject,
			const std::string &separator);
	static VString split(const std::string &subject,
			const char separator = ' ');
	static std::string encode(const std::string &s, bool toUtf8);
};

#endif /* CGI_H_ */
