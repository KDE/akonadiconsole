/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "akonadiconsole-version.h"
#include "instanceselector.h"
#include <kcoreaddons_version.h>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <Kdelibs4ConfigMigrator>
#endif

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDBusMetaType>

int main(int argc, char **argv)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif
    QApplication app(argc, argv);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Kdelibs4ConfigMigrator migrate(QStringLiteral("akonadiconsole"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("akonadiconsolerc"));
    migrate.setUiFiles(QStringList() << QStringLiteral("akonadiconsoleui.rc"));
    migrate.migrate();
#endif
    KAboutData aboutData(QStringLiteral("akonadiconsole"),
                         i18n("Akonadi Console"),
                         QStringLiteral(KDEPIM_VERSION),
                         i18n("The Management and Debugging Console for Akonadi"),
                         KAboutLicense::GPL,
                         i18n("(c) 2006-2023 the Akonadi developer"),
                         QString(),
                         QStringLiteral("https://community.kde.org/KDE_PIM/akonadi"));
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("akonadi")));
    aboutData.addAuthor(QStringLiteral("Tobias König"), i18n("Author"), QStringLiteral("tokoe@kde.org"));
    aboutData.addAuthor(QStringLiteral("Volker Krause"), i18n("Author"), QStringLiteral("vkrause@kde.org"));
    aboutData.setProductName(QByteArrayLiteral("Akonadi/akonadiconsole"));
    KAboutData::setApplicationData(aboutData);

    KCrash::initialize();
    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    const QCommandLineOption remoteOption = QCommandLineOption(QStringList() << QStringLiteral("remote"),
                                                               QStringLiteral("Connect to an Akonadi remote debugging server"),
                                                               QStringLiteral("server"));
    parser.addOption(remoteOption);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDBusService service;

    qRegisterMetaType<QVector<QByteArray>>();
    qDBusRegisterMetaType<QVector<qint64>>();
    qDBusRegisterMetaType<QVector<QByteArray>>();

    if (parser.isSet(remoteOption)) {
        const QString akonadiAddr = QStringLiteral("tcp:host=%1,port=31415").arg(parser.value(QStringLiteral("remote")));
        const QString dbusAddr = QStringLiteral("tcp:host=%1,port=31416").arg(parser.value(QStringLiteral("remote")));
        qputenv("AKONADI_SERVER_ADDRESS", akonadiAddr.toLatin1());
        qputenv("DBUS_SESSION_BUS_ADDRESS", dbusAddr.toLatin1());
    }

    InstanceSelector instanceSelector(parser.isSet(remoteOption) ? parser.value(remoteOption) : QString());

    return app.exec();
}
