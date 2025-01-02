#include "DDNS.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

void DDNS::getIP_() {
    recTimer.stop();
    ret4.clear();
    ret6.clear();
    fail.clear();
    if (getIPServers.isEmpty()) {
        qWarning() << "没有GetIP服务器, 无法正常进行DNS解析更新";
        return;
    }
    for (const auto &i: getIPServers) {
        QNetworkRequest request;
        request.setUrl(QUrl(i));
        request.setTransferTimeout(10000);

        auto *reply = naManager.get(request);
        connect(reply, &QNetworkReply::finished, this, &DDNS::getIPFinish_);
    }
}

void DDNS::getIPFinish_() {
    auto reply = (QNetworkReply *) sender();
    reply->deleteLater();
    auto url = reply->url().toString();
    if (reply->error() != QNetworkReply::NoError) {
        auto data = reply->errorString();
        qWarning() << (url + " 请求失败: " + data).toUtf8().data();
        fail[url] = data;
    } else getIPAnalyse_(url, reply->readAll());

    QDateTime baseDateTime = QDateTime::currentDateTime(); // 创建基准时间点
    int msec; // 距离基准时间点的毫秒数
    {
        auto currDateTime = QDateTime::currentDateTime();
        // 提取当前时间和日期
        QTime currentTime = baseDateTime.time();
        int currentMinutes = currentTime.minute();

        // 计算距离下一个10分钟的分钟差
        int minutesToNext10 = (10 - currentMinutes % 10) % 10;
        if (minutesToNext10 == 0) {
            minutesToNext10 = 10; // 如果正好在10分钟的边界上，则跳到下一个10分钟
        }

        // 构建下一个10分钟的时间点，使用addSecs()自动生成正确的时间点
        baseDateTime = baseDateTime.addSecs(minutesToNext10 * 60 - currentTime.second() - currentTime.msec() / 1000);
        msec = int(baseDateTime.toMSecsSinceEpoch() - currDateTime.toMSecsSinceEpoch());
    }

    bool isRec = false;
    if (fail.size() == getIPServers.size()) {
        qInfo() << ("IP API全部请求失败, 将在" + baseDateTime.toString("yyyy-MM-dd hh:mm:ss") + "进行重试, 或者终止程序重新配置").toUtf8().data();
        if (alarmsLastSendDateTime.secsTo(QDateTime::currentDateTime()) >= 3600)
        {
            // 邮件 alarms2
            currMailGroup = "alarms2";
            currTitle = "Aliyun DDNS Alarms";
            QString tmp = alarms2Html;
            tmp.replace("${date}", currentDateTime_());
            tmp.replace("${name}", name);
            QString table;
            auto tmp2 = fail.keys();
            for (const auto &j: tmp2) {
                table += ("<div>" + j + "</div>\n");
                table += ("<div>" + fail[j] + "</div>\n");
            }
            tmp.replace("${table}", table);
            currMsg = tmp.toUtf8();
            sendMail_();
            alarmsLastSendDateTime = QDateTime::currentDateTime();
        }

        if (isStart)isStart = false;
        isRec = true;
    } else if (ret4.size() + ret6.size() + fail.size() == getIPServers.size()) {
        QTimer::singleShot(0, this, &DDNS::updateDNS_);
        qInfo() << ("IP API全部请求完成, 将在" + baseDateTime.toString("yyyy-MM-dd hh:mm:ss") + "重新请求并检查更新解析").toUtf8().data();
        isRec = true;
    }
    if (isRec)recTimer.start(msec); // 启动定时
}

void DDNS::getIPAnalyse_(const QString &url, const QByteArray &data) {
    auto content = data.trimmed();
    if (isIPv4(content))
    {
        ret4[url] = content;
        qInfo() << (url + " 返回的ip地址: " + content).toUtf8().data();
    }
    else if (isIPv6(content))
    {
        ret6[url] = content;
        qInfo() << (url + " 返回的ip地址: " + content).toUtf8().data();
    }
    else
    {
        QJsonParseError error;
        auto doc = QJsonDocument::fromJson(content, &error);
        if (error.error != QJsonParseError::NoError) {
            auto str = error.errorString();
            qWarning() << ("IP API " + url + " 返回的数据不是json: " + str).toUtf8().data();
            fail[url] = str;
            return;
        }
        if (!doc.isObject()) {
            qWarning() << ("IP API " + url + " 返回的数据不是json对象").toUtf8().data();
            fail[url] = "返回的数据不是json对象";
            return;
        }
        auto obj = doc.object();
        if (!obj.contains("ip")) {
            qWarning() << ("IP API " + url + " 返回的json数据不包含ip字段").toUtf8().data();
            fail[url] = "返回的json数据不包含ip字段";
            return;
        }
        auto ip = obj["ip"].toString();
        if (!isIPv4(ip) && !isIPv6(ip)) {
            qWarning() << ("IP API " + url + " 返回的ip内容不是有效的IPv4或IPv6").toUtf8().data();
            fail[url] = "返回的ip内容不是有效的IPv4或IPv6";
            return;
        }
        if (isIPv4(ip))
            ret4[url] = ip;
        if (isIPv6(ip))
            ret6[url] = ip;
        qInfo() << (url + " 返回的ip地址: " + ip).toUtf8().data();
    }
}
