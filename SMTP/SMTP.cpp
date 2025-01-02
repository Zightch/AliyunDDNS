#include <QRegularExpression>
#include <sstream>
#include <QSslSocket>
#include "SMTP.h"

#define TOTAL_STEPS 11

//期望服务器返回的代码
unsigned short expRetCode[] = {0, 220, 250, 334, 334, 235, 250, 250, 354, 250, 221};

SMTP::SMTP(QObject *parent) : QObject(parent) {
    connect(&waitConnect, &QTimer::timeout, this, &SMTP::disconnected_);
    connect(this, &SMTP::progress, this, &SMTP::nextStep_);
}

[[maybe_unused]]
bool SMTP::isSending() const {
    return tcpSocket != nullptr || sslSocket != nullptr;
}

void SMTP::sendMail(
        const QString &serverE,//服务器
        unsigned short portE,//端口号
        const QString &accountE,//账户
        const QString &passwdE,//密码
        const QString &senderE,//发件人邮箱
        const QString &senderNameE,//发件人姓名
        const QString &receiverE,//收件人邮箱
        const QString &titleE,//邮件标题
        const QString &dataE//邮件内容
) {
    if (isSending()) {
        emit error(progressNum, "当前SMTP对象正在发送邮件, 请稍后重试");
        return;
    }
    if (!checkParam_(serverE, portE, accountE, passwdE, senderE, receiverE))return;
    server = serverE;
    port = portE;
    account = accountE;
    passwd = passwdE;
    senderMail = senderE;
    senderName = senderNameE;
    receiver = receiverE;
    title = titleE;
    auto dt = dataE;
    dt.replace("\n", "\r\n");
    dt.replace("\r\r\n", "\r\n");
    dt.replace("\r", "\r\n");
    dt.replace("\r\n\n", "\r\n");
    dt.replace("\r\n.\r\n", "\r\n..\r\n");
    data = dt;

    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::connected, this, &SMTP::connected_);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &SMTP::read_);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &SMTP::disconnected_);
    waitConnect.start(waitConnectTime);
    tcpSocket->connectToHost(server, port);
}

void SMTP::sendMailEncrypted(
        const QString &serverE,//服务器
        unsigned short portE,//端口号
        const QString &accountE,//账户
        const QString &passwdE,//密码
        const QString &senderE,//发件人邮箱
        const QString &senderNameE,//发件人姓名
        const QString &receiverE,//收件人邮箱
        const QString &titleE,//邮件标题
        const QString &dataE//邮件内容
) {
    if (isSending()) {
        emit error(progressNum, "当前SMTP对象正在发送邮件, 请稍后重试");
        return;
    }
    if (!checkParam_(serverE, portE, accountE, passwdE, senderE, receiverE))return;
    server = serverE;
    port = portE;
    account = accountE;
    passwd = passwdE;
    senderMail = senderE;
    senderName = senderNameE;
    receiver = receiverE;
    title = titleE;
    auto dt = dataE;
    dt.replace("\n", "\r\n");
    dt.replace("\r\r\n", "\r\n");
    dt.replace("\r", "\r\n");
    dt.replace("\r\n\n", "\r\n");
    dt.replace("\r\n.\r\n", "\r\n..\r\n");
    data = dt;

    sslSocket = new QSslSocket(this);
    connect(sslSocket, &QSslSocket::encrypted, this, &SMTP::connected_);
    connect(sslSocket, &QSslSocket::readyRead, this, &SMTP::read_);
    connect(sslSocket, &QSslSocket::disconnected, this, &SMTP::disconnected_);
    waitConnect.start(waitConnectTime);
    sslSocket->connectToHostEncrypted(server, port);
}

bool SMTP::checkParam_(
        const QString &serverE,//服务器
        unsigned short portE,//端口号
        const QString &accountE,//账户
        const QString &passwdE,//密码
        const QString &senderE,//发件人邮箱
        const QString &receiverE//收件人邮箱
) {
    if (serverE.isEmpty()) {
        emit error(progressNum, "server不能为空");
        return false;
    }
    if (portE == 0) {
        emit error(progressNum, "port不能为0");
        return false;
    }
    if (accountE.isEmpty()) {
        emit error(progressNum, "account不能为空");
        return false;
    }
    if (passwdE.isEmpty()) {
        emit error(progressNum, "password不能为空");
        return false;
    }
    if (senderE.isEmpty()) {
        emit error(progressNum, "sender不能为空");
        return false;
    }
    if (receiverE.isEmpty()) {
        emit error(progressNum, "receiver不能为空");
        return false;
    }
    static const QRegularExpression rx = QRegularExpression(R"(^\w+([-+.]\w+)*@\w+([-.]\w+)*\.\w+([-.]\w+)*$)");
    if (!rx.match(senderE).hasMatch()) {
        emit error(progressNum, "sender不是合法的邮箱");
        return false;
    }
    if (!rx.match(receiverE).hasMatch()) {
        emit error(progressNum, "receiver不是合法的邮箱");
        return false;
    }

    return true;
}

void SMTP::connected_() {
    progressNum++;
}

int SMTP::getTotalSteps() {
    return TOTAL_STEPS;
}

void SMTP::read_() {
    auto socket = (QTcpSocket *) sender();
    readBuf += socket->readAll();
    while (readBuf.size() >= 2) {
        int lineSize = 1;
        while (lineSize < readBuf.size()) {
            if (readBuf[lineSize - 1] == '\r' && readBuf[lineSize] == '\n')
                break;
            lineSize++;
        }
        if (lineSize == readBuf.size())return;
        QByteArray msg;
        msg.append(readBuf, lineSize - 1);
        readBuf.remove(0, lineSize + 1);

        waitConnect.stop();
        waitConnect.start(waitConnectTime);

        static const QRegularExpression rx = QRegularExpression(R"(^[1-5]\d{2}\s.+$)");
        if (!rx.match(msg).hasMatch()) {
            int tmp = progressNum;
            reset_();
            emit error(tmp, "服务器返回的数据不符合SMTP协议规范, 请检查服务器的合法性");
            return;
        }

        std::stringstream ss;
        ss << msg.data();
        unsigned short code;
        ss >> code;

        if (code == expRetCode[progressNum])emit progress(progressNum++);
        else {
            int tmp = progressNum;
            reset_();
            emit error(tmp, msg);
            return;
        }
    }
}

void SMTP::disconnected_() {
    if (progressNum != TOTAL_STEPS) {
        int tmp = progressNum;
        reset_();
        emit error(tmp, "邮件正在发送中连接丢失, 邮件发送未完成");
    }
    else {
        reset_();
        emit finish();
    }
}

bool SMTP::setWaitConnectTime(int time) {
    if (100 <= time && time <= 300000) {
        waitConnectTime = time;
        return true;
    }
    return false;
}

SMTP::~SMTP() {
    reset_();
}

void SMTP::reset_() {
    server.clear();
    port = 0;
    account.clear();
    passwd.clear();
    senderMail.clear();
    senderName.clear();
    receiver.clear();
    title.clear();
    data.clear();
    waitConnect.stop();
    if (sslSocket != nullptr) {
        disconnect(sslSocket, &QSslSocket::disconnected, this, &SMTP::disconnected_);
        sslSocket->deleteLater();
        sslSocket = nullptr;
    }
    if (tcpSocket != nullptr) {
        disconnect(tcpSocket, &QTcpSocket::disconnected, this, &SMTP::disconnected_);
        tcpSocket->deleteLater();
        tcpSocket = nullptr;
    }
    progressNum = 0;
}

void SMTP::nextStep_(int step) {
    QByteArray sendMsg;
    if (step == 1)sendMsg = "HELO " + server.toUtf8() + "\r\n";
    else if (step == 2)sendMsg = "AUTH LOGIN\r\n";
    else if (step == 3)sendMsg = account.toUtf8().toBase64() + "\r\n";
    else if (step == 4)sendMsg = passwd.toUtf8().toBase64() + "\r\n";
    else if (step == 5)sendMsg = "MAIL FROM: <" + senderMail.toUtf8() + ">\r\n";
    else if (step == 6)sendMsg = "RCPT TO: <" + receiver.toUtf8() + ">\r\n";
    else if (step == 7)sendMsg = "DATA\r\n";
    else if (step == 8) {
        sendMsg = "Subject: " + title.toUtf8() + "\r\n";
        sendMsg += "From: " + senderName.toUtf8() + " <" + senderMail.toUtf8() + ">\r\n";
        sendMsg += "To:  <" + receiver.toUtf8() + ">\r\n";
        sendMsg += data.toUtf8() + "\r\n.\r\n";
    } else if (step == 9)sendMsg = "QUIT\r\n";
    else if (step == 10) {
        QTimer::singleShot(0, this, &SMTP::disconnected_);
        return;
    }

    if (sendMsg.isEmpty()) {
        int tmp = progressNum;
        reset_();
        emit error(tmp, "该步骤不存在");
        return;
    }
    QTcpSocket *socket = (tcpSocket == nullptr) ? sslSocket : tcpSocket;
    if (socket == nullptr) {
        int tmp = progressNum;
        reset_();
        emit error(tmp, "连接被异常删除");
        return;
    }
    socket->write(sendMsg);
}
