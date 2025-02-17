#include <QUrlQuery>
#include <QMessageAuthenticationCode>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "DDNS.h"

// 阿里云的API地址和版本
const QString aliyunApiUrl = "https://alidns.aliyuncs.com";
const QString aliyunApiVersion = "2015-01-09";

QString DDNS::aliyunGenerateSignature_(const QUrlQuery &query) {
    // 按照参数名的字典顺序排序
    auto items = query.queryItems();
    std::sort(items.begin(), items.end());

    // 使用URL编码拼接参数
    QString paramString;
    for (const auto &item: items) {
        paramString.append(QUrl::toPercentEncoding(item.first));
        paramString.append("=");
        paramString.append(QUrl::toPercentEncoding(item.second));
        paramString.append("&");
    }
    paramString.chop(1); // 去掉最后一个&

    // 在参数前后加上HTTP方法和URL编码的斜杠
    QString stringToSign = "GET&%2F&" + QUrl::toPercentEncoding(paramString);
    // 使用HMAC-SHA1算法计算签名，密钥为AccessKeySecret加上一个&
    QString key = accessKeySecret + "&";
    QString signature = QMessageAuthenticationCode::hash(stringToSign.toUtf8(), key.toUtf8(), QCryptographicHash::Sha1).toBase64();
    return QUrl::toPercentEncoding(signature);
}

void DDNS::updateDNS_()
{
    QList<QString> values;
    if (type == "A")
        values = ret4.values();
    else
        values = ret6.values();
    if (values.isEmpty())
    {
        if (alarmsLastSendDateTime.secsTo(QDateTime::currentDateTime()) >= 3600)
        {
            currMailGroup = "alarms3";
            currTitle = name + " Alarms";
            QString tmp = alarms3Html;
            tmp.replace("${date}", currentDateTime_());
            tmp.replace("${name}", name);
            tmp.replace("${type}", type);
            QString table;
            auto tmp1 = ret4.keys();
            for (const auto &j: tmp1) {
                table += ("<div>" + j + "</div>\n");
                table += ("<div>" + ret4[j] + "</div>\n");
            }
            auto tmp2 = ret6.keys();
            for (const auto &j: tmp2) {
                table += ("<div>" + j + "</div>\n");
                table += ("<div>" + ret6[j] + "</div>\n");
            }
            auto tmp3 = fail.keys();
            for (const auto &j: tmp3) {
                table += ("<div>" + j + "</div>\n");
                table += ("<div>" + fail[j] + "</div>\n");
            }
            tmp.replace("${table}", table);
            currMsg = tmp.toUtf8();
            sendMail_();
            alarmsLastSendDateTime = QDateTime::currentDateTime();
        }
        if (isStart)isStart = false;
        return;
    }
    int i = 1;
    while (i < values.size() && values[i - 1] == values[i])i++;
    if (i != values.size()) {
        if (alarmsLastSendDateTime.secsTo(QDateTime::currentDateTime()) >= 3600)
        {
            currMailGroup = "alarms0";
            currTitle = name + " Alarms";
            QString tmp = alarms0Html;
            tmp.replace("${date}", currentDateTime_());
            tmp.replace("${name}", name);
            QString table;
            auto tmp1 = ret4.keys();
            for (const auto &j: tmp1) {
                table += ("<div>" + j + "</div>\n");
                table += ("<div>" + ret4[j] + "</div>\n");
            }
            auto tmp2 = ret6.keys();
            for (const auto &j: tmp2) {
                table += ("<div>" + j + "</div>\n");
                table += ("<div>" + ret6[j] + "</div>\n");
            }
            auto tmp3 = fail.keys();
            for (const auto &j: tmp3) {
                table += ("<div>" + j + "</div>\n");
                table += ("<div>" + fail[j] + "</div>\n");
            }
            tmp.replace("${table}", table);
            currMsg = tmp.toUtf8();
            sendMail_();
            alarmsLastSendDateTime = QDateTime::currentDateTime();
        }
        if (isStart)isStart = false;
        return;
    }
    describeDomainRecords_();
}

void DDNS::describeDomainRecords_() {
    updateLogs.append(updateLog_("开始查询"));
    // 构造查询参数
    QUrlQuery query;
    query.addQueryItem("Action", "DescribeDomainRecords"); // 操作接口名
    query.addQueryItem("DomainName", domainName); // 域名
    query.addQueryItem("RRKeyWord", rr); // 主机记录的关键字
    query.addQueryItem("Format", "JSON"); // 返回数据的格式
    query.addQueryItem("Version", aliyunApiVersion); // API版本号
    query.addQueryItem("AccessKeyId", accessKeyID); // AccessKeyId
    query.addQueryItem("SignatureMethod", "HMAC-SHA1"); // 签名方式
    query.addQueryItem("Timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)); // 请求的时间戳
    query.addQueryItem("SignatureVersion", "1.0"); // 签名算法版本
    query.addQueryItem("SignatureNonce", QUuid::createUuid().toString()); // 唯一随机数
    updateLogs.append(updateLog_("查询参数: " + query.toString()));

    // 生成签名并添加到参数中
    QString signature = aliyunGenerateSignature_(query);
    updateLogs.append(updateLog_("查询签名: " + signature));
    query.addQueryItem("Signature", signature);

    // 构造请求对象
    QString requestUrl = aliyunApiUrl + "/?" + query.toString();
    updateLogs.append(updateLog_("查询URL: " + requestUrl));
    QNetworkRequest request;
    request.setUrl(QUrl(requestUrl));
    request.setTransferTimeout(10000);

    // 发送GET请求
    QNetworkReply *reply = naManager.get(request);
    connect(reply, &QNetworkReply::finished, this, &DDNS::findFinish_);
    updateLogs.append(updateLog_("已发送查询请求"));
}

void DDNS::updateDomainRecord_(const QString &recordId) {
    updateLogs.append(updateLog_("开始更新"));
    // 构造查询参数
    QUrlQuery query;
    query.addQueryItem("Action", "UpdateDomainRecord"); // 操作接口名
    query.addQueryItem("RecordId", recordId); // 解析记录的ID
    query.addQueryItem("RR", rr); // 主机记录
    query.addQueryItem("Type", type); // 记录类型
    query.addQueryItem("Value", type == "A" ? ret4.begin().value() : ret6.begin().value()); // 记录值
    query.addQueryItem("Format", "JSON"); // 返回数据的格式
    query.addQueryItem("Version", aliyunApiVersion); // API版本号
    query.addQueryItem("AccessKeyId", accessKeyID); // AccessKeyId
    query.addQueryItem("SignatureMethod", "HMAC-SHA1"); // 签名方式
    query.addQueryItem("Timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)); // 请求的时间戳
    query.addQueryItem("SignatureVersion", "1.0"); // 签名算法版本
    query.addQueryItem("SignatureNonce", QUuid::createUuid().toString()); // 唯一随机数
    updateLogs.append(updateLog_("更新参数: " + query.toString()));

    // 生成签名并添加到参数中
    QString signature = aliyunGenerateSignature_(query);
    updateLogs.append(updateLog_("更新签名: " + signature));
    query.addQueryItem("Signature", signature);

    // 构造请求对象
    QString requestUrl = aliyunApiUrl + "/?" + query.toString();
    updateLogs.append(updateLog_("更新URL: " + requestUrl));
    QNetworkRequest request;
    request.setUrl(QUrl(requestUrl));
    request.setTransferTimeout(10000);

    QNetworkReply *reply = naManager.get(request);
    connect(reply, &QNetworkReply::finished, this, &DDNS::updateFinish_);
    updateLogs.append(updateLog_("已发送更新请求"));
}

void DDNS::findFinish_() {
    auto reply = (QNetworkReply *) sender();
    reply->deleteLater();
    auto alarms = [&](const QString &data) {
        auto error = "DescribeDomainRecord请求失败" + data;
        qWarning() << error.toUtf8().data();
        updateLogs.append(updateLog_(error));
        sendAlarms1_();
    };
    if (reply->error() != QNetworkReply::NoError) {
        alarms(": " + reply->errorString());
        updateLogs.clear();
        return;
    }
    // 读取响应数据
    QByteArray data = reply->readAll();
    updateLogs.append(updateLog_("查询返回数据: " + data));
    // 解析JSON数据
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    // 获取响应的状态码
    auto code = obj["Code"].toString().toUtf8();
    if (!code.isEmpty()) {
        alarms(", code: " + code + ", msg: " + obj["Message"].toString());
        updateLogs.clear();
        return;
    }
    // 获取解析记录列表
    QJsonArray records = obj["DomainRecords"].toObject()["Record"].toArray();
    if (records.isEmpty()) {
        alarms(": 没有查询到记录");
        updateLogs.clear();
        return;
    }
    QJsonObject record = records.first().toObject();
    QString recordId = record["RecordId"].toString().toUtf8(); // 解析记录的ID
    QString value = record["Value"].toString().toUtf8(); // 解析记录的值
    updateLogs.append(updateLog_("查询到的解析: " + value));
    updateLogs.append(updateLog_("查询到RecordId: " + recordId));
    if (value == (type == "A" ? ret4.begin().value() : ret6.begin().value())) {
        qInfo() << (rr + "." + domainName + "的记录为: " + value + ", RecordId: " + recordId + ", 无需更新").toUtf8().data();

        auto currDate = QDateTime::currentDateTime();
        auto currHour = currDate.toString("hh").toInt();
        auto currMin = currDate.toString("mm").toInt();
        auto currDay = currDate.toString("dd").toInt();

        if (isStart || (currHour == 8 && currDay == 1 && currMin < 10)) {
            currMailGroup = "notify0";
            currTitle = name + " Notify";
            QString tmp = notify0Html;
            tmp.replace("${date}", currentDateTime_());
            tmp.replace("${name}", name);
            QString table;
            auto tmp1 = ret4.keys();
            for (const auto &i: tmp1) {
                table += ("<div>" + i + "</div>\n");
                table += ("<div>" + ret4[i] + "</div>\n");
            }
            auto tmp2 = ret6.keys();
            for (const auto &j: tmp2) {
                table += ("<div>" + j + "</div>\n");
                table += ("<div>" + ret6[j] + "</div>\n");
            }
            auto tmp3 = fail.keys();
            for (const auto &j: tmp3) {
                table += ("<div>" + j + "</div>\n");
                table += ("<div>" + fail[j] + "</div>\n");
            }
            tmp.replace("${table}", table);
            tmp.replace("${domainName}", rr + "." + domainName);
            tmp.replace("${ip}", type == "A" ? ret4.begin().value() : ret6.begin().value());
            currMsg = tmp.toUtf8();
            sendMail_();
            if (isStart)isStart = false;
        }

        updateLogs.clear();
        return;
    }
    oldIP = value;
    updateDomainRecord_(recordId);
}

void DDNS::updateFinish_() {
    auto reply = (QNetworkReply *) sender();
    reply->deleteLater();
    auto alarms = [&](const QString &data) {
        auto error = "UpdateDomainRecord请求失败" + data;
        qWarning() << error.toUtf8().data();
        updateLogs.append(updateLog_(error));
        sendAlarms1_();
    };
    if (reply->error() != QNetworkReply::NoError) {
        alarms(": " + reply->errorString());
        updateLogs.clear();
        return;
    }
    // 读取响应数据
    QByteArray data = reply->readAll();
    updateLogs.append(updateLog_("查询返回数据: " + data));
    // 解析JSON数据
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    // 获取响应的状态码
    auto code = obj["Code"].toString().toUtf8();
    if (!code.isEmpty()) {
        alarms(", code: " + code + ", msg: " + obj["Message"].toString());
        updateLogs.clear();
        return;
    }
    // 成功，获取更新后的记录ID
    QString newRecordId = obj["RecordId"].toString().toUtf8();
    qInfo() << (rr + "." + domainName + "更新成功: " + (type == "A" ? ret4.begin().value() : ret6.begin().value()) + ", 新的RecordId: " + newRecordId).toUtf8().data();

    currMailGroup = "notify1";
    currTitle = name + " Notify";
    QString tmp = notify1Html;
    tmp.replace("${date}", currentDateTime_());
    tmp.replace("${name}", name);
    QString table;
    auto tmp1 = ret4.keys();
    for (const auto &i: tmp1) {
        table += ("<div>" + i + "</div>\n");
        table += ("<div>" + ret4[i] + "</div>\n");
    }
    auto tmp2 = ret6.keys();
    for (const auto &j: tmp2) {
        table += ("<div>" + j + "</div>\n");
        table += ("<div>" + ret6[j] + "</div>\n");
    }
    auto tmp3 = fail.keys();
    for (const auto &j: tmp3) {
        table += ("<div>" + j + "</div>\n");
        table += ("<div>" + fail[j] + "</div>\n");
    }
    tmp.replace("${table}", table);
    tmp.replace("${domainName}", rr + "." + domainName);
    tmp.replace("${oldIp}", oldIP);
    tmp.replace("${newIp}", (type == "A" ? ret4.begin().value() : ret6.begin().value()));
    currMsg = tmp.toUtf8();
    sendMail_();
    if (isStart)isStart = false;

    updateLogs.clear();
}

void DDNS::sendAlarms1_() {
    if (alarmsLastSendDateTime.secsTo(QDateTime::currentDateTime()) >= 3600)
    {
        currMailGroup = "alarms1";
        currTitle = name + " Alarms";
        QString tmp = alarms1Html;
        tmp.replace("${date}", currentDateTime_());
        tmp.replace("${name}", name);
        QString log;
        for (const auto &i: updateLogs)
            log += (i + "\n");
        tmp.replace("${log}", log);
        currMsg = tmp.toUtf8();
        sendMail_();
        alarmsLastSendDateTime = QDateTime::currentDateTime();
    }
    if (isStart)isStart = false;
}
