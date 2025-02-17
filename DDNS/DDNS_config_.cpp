#include "DDNS.h"
#include <QDebug>
#include <QSettings>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>

const QString configFilePath = "./configs/AliyunDDNS.ini";

#define SMTP_PORT(SSL) SSL ? 465 : 25

void DDNS::config_() {
    QDir dir("./");
    if (!dir.exists("configs")) {
        dir.mkpath("configs");
    }
    if (!QFile::exists(configFilePath)) {
        QSettings conf(configFilePath, QSettings::IniFormat);

        conf.setValue("DDNS/get-IP-servers", "./configs/get-IP-servers.json");
        conf.setValue("DDNS/name", "");

        conf.setValue("SMTP/server", "");
        conf.setValue("SMTP/SSL", "true");
        conf.setValue("SMTP/user", "");
        conf.setValue("SMTP/passwd", "");
        conf.setValue("SMTP/sender", "");
        conf.setValue("SMTP/to", "./configs/receiver-emails.json");

        conf.setValue("Aliyun/AccessKey-ID", "");
        conf.setValue("Aliyun/AccessKey-Secret", "");
        conf.setValue("Aliyun/DomainName", "");
        conf.setValue("Aliyun/RR", "");
        conf.setValue("Aliyun/Type", "");

        conf.setValue("MailMsg/notify0", "./configs/notify0.html");
        conf.setValue("MailMsg/notify1", "./configs/notify1.html");
        conf.setValue("MailMsg/alarms0", "./configs/alarms0.html");
        conf.setValue("MailMsg/alarms1", "./configs/alarms1.html");
        conf.setValue("MailMsg/alarms2", "./configs/alarms2.html");
    }

    QSettings conf(configFilePath, QSettings::IniFormat);

    auto sSSL = conf.value("SMTP/SSL").toString();
    if (sSSL == "true")smtpSSL = true;
    else if (sSSL == "false")smtpSSL = false;
    else conf.setValue("SMTP/SSL", "true");

    auto sServer = conf.value("SMTP/server").toString();
    if (sServer.isEmpty())qWarning() << "请指定SMTP服务器";
    else {
        do {
            static const QRegularExpression IPv6Port(R"(^\[(.*)\]:(\d+)$)");
            auto match = IPv6Port.match(sServer);
            if (match.hasMatch()) {
                smtpServerIP = match.captured(1);
                smtpServerPort = match.captured(2).toUShort();
                break;
            }
            auto list = sServer.split(':');
            if (list.size() == 2) {
                smtpServerIP = list[0];
                smtpServerPort = list[1].toUShort();
                break;
            }
            smtpServerIP = sServer;
            smtpServerPort = SMTP_PORT(smtpSSL);
        } while (false);
    }

    smtpSender = conf.value("SMTP/sender").toString();
    if (smtpSender.isEmpty())qWarning() << "请指定SMTP的发件人";

    smtpUser = conf.value("SMTP/user").toString();
    if (smtpUser.isEmpty())qWarning() << "请填入你的SMTP用户";
    smtpPasswd = conf.value("SMTP/passwd").toString();
    if (smtpPasswd.isEmpty())qWarning() << "请填入你的SMTP用户密码";
    accessKeyID = conf.value("Aliyun/AccessKey-ID").toString();
    if (accessKeyID.isEmpty())qWarning() << "请填入你的阿里云AccessKey ID";
    accessKeySecret = conf.value("Aliyun/AccessKey-Secret").toString();
    if (accessKeySecret.isEmpty())qWarning() << "请填入你的阿里云AccessKey Secret";

    domainName = conf.value("Aliyun/DomainName").toString();
    if (domainName.isEmpty())qWarning() << "请填入你的一级域名";

    rr = conf.value("Aliyun/RR").toString();
    if (rr.isEmpty())qWarning() << "请填入你的子域名";
    auto ut = conf.value("Aliyun/Type").toString();
    if (ut != "A" && ut != "AAAA")
    {
        qWarning() << ("文件\"" + configFilePath + "\"中的配置\"Aliyun/Type\"不符合记录类型, 已重置为\"A\"").toUtf8().data();
        conf.setValue("Aliyun/Type", "A");
        ut = "A";
    }
    type = ut;

    QFile gisf(conf.value("DDNS/get-IP-servers").toString());
    QByteArray gisd;
    if (gisf.open(QFile::ReadOnly)) {
        gisd = gisf.readAll();
        gisf.close();
        QJsonDocument gisDoc = QJsonDocument::fromJson(gisd);
        if (gisDoc.isArray()) {
            QJsonArray array = gisDoc.array();
            for (const auto &i: array)
                getIPServers.append(i.toString());
        } else
            qWarning() << "get-IP-servers内容不是有效的json数组";
    } else
        qWarning() << "get-IP-servers文件打开失败, 请检查文件路径或权限";

    name = conf.value("DDNS/name").toString();
    if (name.isEmpty())qWarning() << "请填入你的DDNS名称";

    QFile ref(conf.value("SMTP/to").toString().toUtf8());
    QByteArray red;
    if (ref.open(QFile::ReadOnly)) {
        red = ref.readAll();
        ref.close();
        QJsonDocument reDoc = QJsonDocument::fromJson(red);
        if (reDoc.isArray()) {
            QJsonArray array = reDoc.array();
            for (auto && i : array)
                receiverEmails.append(i.toString());
        } else
            qWarning() << "receiver-emails内容不是有效的json数组";
    } else
        qWarning() << "receiver-emails文件打开失败, 请检查文件路径或权限";

    QFile nh0(conf.value("MailMsg/notify0").toString());
    if (nh0.open(QFile::ReadOnly)) notify0Html = nh0.readAll();
    else
        qWarning() << "notify0-html文件打开失败, 请检查文件路径或权限";

    QFile nh1(conf.value("MailMsg/notify1").toString());
    if (nh1.open(QFile::ReadOnly)) notify1Html = nh1.readAll();
    else
        qWarning() << "notify1-html文件打开失败, 请检查文件路径或权限";

    QFile ah0(conf.value("MailMsg/alarms0").toString());
    if (ah0.open(QFile::ReadOnly)) alarms0Html = ah0.readAll();
    else
        qWarning() << "alarms0-html文件打开失败, 请检查文件路径或权限";

    QFile ah1(conf.value("MailMsg/alarms1").toString());
    if (ah1.open(QFile::ReadOnly)) alarms1Html = ah1.readAll();
    else
        qWarning() << "alarms1-html文件打开失败, 请检查文件路径或权限";

    QFile ah2(conf.value("MailMsg/alarms2").toString());
    if (ah2.open(QFile::ReadOnly)) alarms2Html = ah2.readAll();
    else
        qWarning() << "alarms2-html文件打开失败, 请检查文件路径或权限";

    QFile ah3(conf.value("MailMsg/alarms3").toString());
    if (ah3.open(QFile::ReadOnly)) alarms3Html = ah3.readAll();
    else
        qWarning() << "alarms3-html文件打开失败, 请检查文件路径或权限";
}
