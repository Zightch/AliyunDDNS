#pragma once

#include <QObject>
#include <QTimer>

class QTcpSocket;
class QSslSocket;

class SMTP : public QObject {
Q_OBJECT

public:
    explicit SMTP(QObject * = nullptr);

    ~SMTP() override;

    [[maybe_unused]] [[nodiscard]]
    bool isSending() const;

    //普通发送
    void sendMail(
            const QString &,//服务器
            unsigned short,//端口号
            const QString &,//账户
            const QString &,//密码
            const QString &,//发件人邮箱
            const QString &,//发件人姓名
            const QString &,//收件人邮箱
            const QString &,//邮件标题
            const QString &//邮件内容
    );

    //加密发送
    void sendMailEncrypted(
            const QString &,//服务器
            unsigned short,//端口号
            const QString &,//账户
            const QString &,//密码
            const QString &,//发件人邮箱
            const QString &,//发件人姓名
            const QString &,//收件人邮箱
            const QString &,//邮件标题
            const QString &//邮件内容
    );

    static int getTotalSteps();

    bool setWaitConnectTime(int);

signals:

    void progress(int);

    void error(int, const QString &);

    void finish();

private:
    void nextStep_(int);
    void reset_();
    void connected_();
    void read_();
    void disconnected_();

    bool checkParam_(
            const QString &,//服务器
            unsigned short,//端口号
            const QString &,//账户
            const QString &,//密码
            const QString &,//发件人邮箱
            const QString &//收件人邮箱
    );

    int progressNum = 0;
    int waitConnectTime = 3000;

    QString server;//服务器
    unsigned short port = 0;//端口号
    QString account;//账户
    QString passwd;//密码
    QString senderMail;//发件人邮箱
    QString senderName;//发件人姓名
    QString receiver;//收件人邮箱和收件人姓名
    QString title;//邮件标题
    QString data;//邮件内容

    QByteArray readBuf;

    QTimer waitConnect;
    QTcpSocket *tcpSocket = nullptr;
    QSslSocket *sslSocket = nullptr;
};
