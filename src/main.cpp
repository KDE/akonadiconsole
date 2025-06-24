/*
    This file is part of Akonadi.

    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "akonadiconsole-version.h"

#include "instanceselector.h"

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDBusMetaType>
using namespace Qt::Literals::StringLiterals;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("akonadiconsole"));
    KAboutData aboutData(u"akonadiconsole"_s,
                         i18n("Akonadi Console"),
                         QStringLiteral(KDEPIM_VERSION),
                         i18n("The Management and Debugging Console for Akonadi"),
                         KAboutLicense::GPL,
                         i18n("(c) 2006-2025 the Akonadi developer"),
                         QString(),
                         u"https://community.kde.org/KDE_PIM/akonadi"_s);
    QApplication::setWindowIcon(QIcon::fromTheme(u"akonadi"_s));
    aboutData.addAuthor(u"Tobias König"_s, i18n("Author"), u"tokoe@kde.org"_s);
    aboutData.addAuthor(u"Volker Krause"_s, i18n("Author"), u"vkrause@kde.org"_s);
    aboutData.setProductName(QByteArrayLiteral("Akonadi/akonadiconsole"));
    KAboutData::setApplicationData(aboutData);

    KCrash::initialize();
    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    const QCommandLineOption remoteOption = QCommandLineOption(QStringList() << u"remote"_s, u"Connect to an Akonadi remote debugging server"_s, u"server"_s);
    parser.addOption(remoteOption);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDBusService service;

    qRegisterMetaType<QList<QByteArray>>();
    qDBusRegisterMetaType<QList<qint64>>();
    qDBusRegisterMetaType<QList<QByteArray>>();

    if (parser.isSet(remoteOption)) {
        const QString akonadiAddr = u"tcp:host=%1,port=31415"_s.arg(parser.value(u"remote"_s));
        const QString dbusAddr = u"tcp:host=%1,port=31416"_s.arg(parser.value(u"remote"_s));
        qputenv("AKONADI_SERVER_ADDRESS", akonadiAddr.toLatin1());
        qputenv("DBUS_SESSION_BUS_ADDRESS", dbusAddr.toLatin1());
    }

    InstanceSelector instanceSelector(parser.isSet(remoteOption) ? parser.value(remoteOption) : QString());

    return app.exec();
}
