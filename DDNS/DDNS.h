#pragma once

#include <QNetworkAccessManager>
#include "SMTP/SMTP.h"

class DDNS : public QObject {
Q_OBJECT

public:
    static DDNS *getObject(QObject * = nullptr);

private:

    explicit DDNS(QObject *);

    ~DDNS() override;

    void start_();

    void config_();

    void smtpError_(int, const QString &);

    void smtpFinish_();

    void sendMail_();

    void getIP_();

    void getIPFinish_();

    void getIPAnalyse_(const QString &, const QByteArray &);

    void updateDNS_();

    //查询解析记录
    void describeDomainRecords_();

    void findFinish_();

    //更新解析记录
    void updateDomainRecord_(const QString &);

    void updateFinish_();

    void sendAlarms1_();

    static QString currentDateTime_();

    static QString updateLog_(const QString &);

    QString aliyunGenerateSignature_(const QUrlQuery &);

    bool isStart = true;
    QList<QString> getIPServers;//获取IP的服务器列表
    QString name;//名称

    QString smtpServerIP;//SMTP服务器IP
    unsigned short smtpServerPort{};//SMTP服务器端口号
    bool smtpSSL = true;//SMTP是否开启SSL
    QString smtpUser;//SMTP用户
    QString smtpPasswd;//SMTP用户密码
    QString smtpSender;//SMTP发件人
    QList<QString> receiverEmails;//邮件接收者邮箱列表

    QString accessKeyID;//阿里云AccessKey ID
    QString accessKeySecret;//阿里云AccessKey Secret
    QString domainName;//一级域名
    QString rr;//子域名
    QString type; // 记录类型

    QString notify0Html;
    QString notify1Html;
    QString alarms0Html;
    QString alarms1Html;
    QString alarms2Html;
    QString alarms3Html;

    QDateTime alarmsLastSendDateTime = QDateTime::fromSecsSinceEpoch(0); // 报警邮件最后一次发送时间

    SMTP smtp;
    int theNTHRecipient = 0;
    QString currTitle;
    QString currMsg;
    QString currMailGroup;

    QHash<QString, QString> ret4;
    QHash<QString, QString> ret6;
    QHash<QString, QString> fail;
    QTimer recTimer;

    QNetworkAccessManager naManager;
    QList<QString> updateLogs;
    QString oldIP;

    static DDNS *once;
};
