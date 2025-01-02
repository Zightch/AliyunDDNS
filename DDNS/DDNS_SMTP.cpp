#include "DDNS.h"
#include <QTimer>

void DDNS::smtpError_(int pNum, const QString &msg) {
    QString tmp = "位于第" + QString::number(theNTHRecipient) + "个接收者 邮件发送失败: " + QString::number(pNum) + ", " + msg;
    qWarning() << tmp.toUtf8().data();
    smtpFinish_();
}

void DDNS::smtpFinish_() {
    theNTHRecipient++;
    if (theNTHRecipient >= receiverEmails.size()) {
        qInfo() << ("\"" + currMailGroup + "\"邮件发送完成").toUtf8().data();
        theNTHRecipient = 0;
        if (isStart)QTimer::singleShot(0, this, &DDNS::getIP_);
        return;
    }
    sendMail_();
}

void DDNS::sendMail_() {
    if (!receiverEmails.isEmpty()) {
        if (smtpSSL)
            smtp.sendMailEncrypted(
                    smtpServerIP,
                    smtpServerPort,
                    smtpUser,
                    smtpPasswd,
                    smtpSender,
                    name,
                    receiverEmails[theNTHRecipient],
                    currTitle,
                    currMsg
            );
        else
            smtp.sendMail(
                    smtpServerIP,
                    smtpServerPort,
                    smtpUser,
                    smtpPasswd,
                    smtpSender,
                    name,
                    receiverEmails[theNTHRecipient],
                    currTitle,
                    currMsg
            );
    } else smtpFinish_();
}
