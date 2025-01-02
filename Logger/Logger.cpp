#include "Logger.h"
#include <QDateTime>
#include <iostream>
#include <QCoreApplication>
#if defined(_WIN32) || defined(_MSC_VER)
#include <windows.h>
#endif

Logger *Logger::once = nullptr;

Logger *Logger::instance() {
    if (once == nullptr)
        once = new Logger();
    return once;
}

Logger::Logger() : QObject() {
    thread = new QThread();
    moveToThread(thread);
    connect(this, &Logger::newLogMessage, this, &Logger::processMessages, Qt::QueuedConnection);
    thread->start();
}

Logger::~Logger() {
    thread->quit();
    thread->wait();
    thread->deleteLater();
    thread = nullptr;
    once = nullptr;
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QString text;
    switch (type) {
        case QtDebugMsg:
            text = "调试";
            break;
        case QtInfoMsg:
            text = "信息";
            break;
        case QtWarningMsg:
            text = "警告";
            break;
        case QtCriticalMsg:
            text = "严重";
            break;
        case QtFatalMsg:
            text = "致命";
    }

    QString file;
    QString filePath(context.file);
    if (!filePath.isEmpty()) {
        auto i = filePath.end();
        for (; i != filePath.begin(); i--)
            if ((*i) == '/' || (*i) == '\\')break;
        for (i++; i != filePath.end(); i++)
            file.push_back(*i);
    }

    QString currentDateTime = QDateTime::currentDateTime().toString("MM-dd hh:mm:ss");
    QString threadId = QString::number((quintptr) QThread::currentThreadId(), 16);
    QString fileAndLine = QString("%1:%2").arg(file).arg(context.line);
#ifndef NDEBUG
    QString formattedMessage = QString("[%1][%2][%3][%4] %5")
            .arg(currentDateTime)
            .arg(threadId)
            .arg(text)
            .arg(fileAndLine)
            .arg(msg);
#else
    QString formattedMessage = QString("[%1][%2][%3] %4")
            .arg(currentDateTime)
            .arg(threadId)
            .arg(text)
            .arg(msg);
#endif
    instance()->enqueueMessage(formattedMessage);
}

void Logger::enqueueMessage(const QString &message) {
    QMutexLocker locker(&mutex);
    logQueue.enqueue(message);
    emit newLogMessage();
}

void Logger::processMessages() {
    QMutexLocker locker(&mutex);
    if (!logQueue.isEmpty()) {
        QByteArray message = logQueue.dequeue().toUtf8();
#if defined(_WIN32) || defined(_MSC_VER)
        static const auto utf8ToGBK = [](const QByteArray &utf8_str) {
            int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), -1, nullptr, 0);
            auto wstr = new wchar_t[len + 1];
            MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), -1, wstr, len);
            len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
            char *str = new char[len + 1];
            WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, nullptr, nullptr);
            QByteArray result(str);
            delete[] wstr;
            delete[] str;
            return result;
        };
        message = utf8ToGBK(message);
#endif
        std::cout << message.toStdString() << std::endl;
    }
}
