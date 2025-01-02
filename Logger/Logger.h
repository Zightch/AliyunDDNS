#pragma once

#include <QObject>
#include <QMutex>
#include <QQueue>
#include <QString>
#include <QThread>

class Logger : public QObject {
Q_OBJECT

public:
    static Logger *instance();

    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    void enqueueMessage(const QString &message);

signals:

    void newLogMessage();

private:
    explicit Logger();

    ~Logger() override;

    static Logger *once;
    QMutex mutex;
    QQueue<QString> logQueue;
    QThread *thread = nullptr;
private slots:

    void processMessages();
};
