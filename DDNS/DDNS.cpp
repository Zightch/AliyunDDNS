#include "DDNS.h"
#include <QDebug>
#include "Logger/Logger.h"

const QByteArray mailStartTitle = "Aliyun DDNS Start!";

DDNS *DDNS::once = nullptr;

DDNS *DDNS::getObject(QObject *parent) {
    if (once == nullptr)
        once = new DDNS(parent);
    return once;
}

DDNS::DDNS(QObject *parent) : QObject(parent) {
    config_();
    QTimer::singleShot(0, this, &DDNS::start_);
}

DDNS::~DDNS() = default;

void DDNS::start_() {
    qInfo() << "启动日志中...";
    Logger::instance();
    qInstallMessageHandler(Logger::messageHandler);
    qInfo() << "日志启动成功";

    connect(&recTimer, &QTimer::timeout, this, &DDNS::getIP_);
    connect(&smtp, &SMTP::error, this, &DDNS::smtpError_);
    connect(&smtp, &SMTP::finish, this, &DDNS::smtpFinish_);

    // 开始
    if (!receiverEmails.isEmpty()) {
        currMailGroup = mailStartTitle;
        currTitle = mailStartTitle;
        QString msg = startHtml;
        msg.replace("${name}", name);
        currMsg = msg.toUtf8();
        sendMail_();
    } else {
        qWarning() << "没有收件人";
        getIP_();
    }
}

QString DDNS::currentDateTime_() {
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
}

QString DDNS::updateLog_(const QString &data) {
    QString tmp0 = "[%1] %2";
    auto tmp1 = tmp0.arg(currentDateTime_());
    auto tmp2 = tmp1.arg(data);
    return tmp2;
}
