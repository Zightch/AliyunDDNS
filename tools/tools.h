#pragma once

#include <QString>

bool isIPv4(const QString &);

bool isIPv6(const QString &);

bool isIPv4Port(const QString &);

bool isIPv6Port(const QString &);

bool isEmail(const QString &);

bool isToplevelDomain(const QString &);

bool isDomainName(const QString &);

bool isDomainNamePort(const QString &);

bool isAlphabetAndNumber(const QString &);

bool isUrl(const QString &);
