#include <QCoreApplication>
#include "DDNS/DDNS.h"

int main(int argc, char **argv) {
    QCoreApplication a(argc, argv);
    DDNS::getObject(&a);
    return QCoreApplication::exec();
}
