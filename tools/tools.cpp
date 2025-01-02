#include "tools.h"
#include <QHostAddress>
#include <QRegularExpression>

namespace IPConstRegExp {
    const QRegularExpression IPv6Port = QRegularExpression(R"(^\[(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|fe80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::(ffff(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))]:([1-9]\d{0,3}|[1-5]\d{4}|6[0-4]\d{3}|65[0-4]\d{2}|655[0-2]\d|6553[0-5])$)");
    const QRegularExpression IPv4Port = QRegularExpression(R"(^(((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})(\.((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})){3}):([1-9]\d{0,3}|[1-5]\d{4}|6[0-4]\d{3}|65[0-4]\d{2}|655[0-2]\d|6553[0-5])$)");
    const QRegularExpression domainNamePort = QRegularExpression(R"(^((?!-)[A-Za-z0-9-]{1,63}(?<!-)\.)+[A-Za-z]{2,63}:([1-9]\d{0,3}|[1-5]\d{4}|6[0-4]\d{3}|65[0-4]\d{2}|655[0-2]\d|6553[0-5])$)");
    const QRegularExpression IPv4 = QRegularExpression(R"(^(((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})(\.((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})){3})$)");
    const QRegularExpression IPv6 = QRegularExpression(R"(^(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|fe80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::(ffff(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))$)");
    const QRegularExpression domainName = QRegularExpression(R"(^((?!-)[A-Za-z0-9-]{1,63}(?<!-)\.)+[A-Za-z]{2,63}$)");
    const QRegularExpression toplevelDomain = QRegularExpression(R"(^[a-zA-Z0-9-]+\.[A-Za-z]{2,6}$)");
    const QRegularExpression email = QRegularExpression(R"(^\w+([-+.]\w+)*@\w+([-.]\w+)*\.\w+([-.]\w+)*$)");
    const QRegularExpression alphabetAndNumber = QRegularExpression(R"(^[a-zA-Z0-9]+$)");
    const QRegularExpression url = QRegularExpression(R"(^(((ht|f)tps?):\/\/)?([^!@#$%^&*?.\s-]([^!@#$%^&*?.\s]{0,63}[^!@#$%^&*?.\s])?\.)+[a-z]{2,6}\/?)");
}

bool isIPv4(const QString &data) {
    return IPConstRegExp::IPv4.match(data).hasMatch();
}

bool isIPv6(const QString &data) {
    return IPConstRegExp::IPv6.match(data).hasMatch();
}

bool isIPv4Port(const QString &data) {
    return IPConstRegExp::IPv4Port.match(data).hasMatch();
}

bool isIPv6Port(const QString &data) {
    return IPConstRegExp::IPv6Port.match(data).hasMatch();
}

bool isDomainName(const QString &data) {
    return IPConstRegExp::domainName.match(data).hasMatch();
}

bool isDomainNamePort(const QString &data) {
    return IPConstRegExp::domainNamePort.match(data).hasMatch();
}

bool isToplevelDomain(const QString &data) {
    return IPConstRegExp::toplevelDomain.match(data).hasMatch();
}

bool isEmail(const QString &data) {
    return IPConstRegExp::email.match(data).hasMatch();
}

bool isAlphabetAndNumber(const QString &data) {
    return IPConstRegExp::alphabetAndNumber.match(data).hasMatch();
}

bool isUrl(const QString &data) {
    return IPConstRegExp::url.match(data).hasMatch();
}
