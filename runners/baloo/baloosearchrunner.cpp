/*
 * This file is part of the KDE Baloo Project
 * Copyright (C) 2014  Vishesh Handa <me@vhanda.in>
 * Copyright (C) 2017 David Edmundson <davidedmundson@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "baloosearchrunner.h"

#include <QAction>
#include <QIcon>
#include <QDir>
#include <KRun>
#include <KRunner/QueryMatch>
#include <KLocalizedString>
#include <QMimeDatabase>
#include <QTimer>
#include <QMimeData>
#include <QApplication>
#include <QDBusConnection>
#include <QTimer>

#include <Baloo/Query>
#include <Baloo/IndexerConfig>

#include <KIO/OpenFileManagerWindowJob>

#include "dbusutils_p.h"
#include "krunner1adaptor.h"

static const QString s_openParentDirId = QStringLiteral("openParentDir");

int main (int argc, char **argv)
{
    Baloo::IndexerConfig config;
    if (!config.fileIndexingEnabled()) {
        return -1;
    }
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication app(argc, argv); //KRun needs widgets for error message boxes
    SearchRunner r;
    return app.exec();
}

SearchRunner::SearchRunner(QObject* parent)
    : QObject(parent),
    m_timer(new QTimer(this))
{

    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &SearchRunner::performMatch);

    new Krunner1Adaptor(this);
    qDBusRegisterMetaType<RemoteMatch>();
    qDBusRegisterMetaType<RemoteMatches>();
    qDBusRegisterMetaType<RemoteAction>();
    qDBusRegisterMetaType<RemoteActions>();
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/runner"), this);
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.runners.baloo"));
}

SearchRunner::~SearchRunner()
{
}

RemoteActions SearchRunner::Actions()
{
    return RemoteActions({RemoteAction{
        s_openParentDirId,
        i18n("Open Containing Folder"),
        QStringLiteral("document-open-folder")
    }});
}

RemoteMatches SearchRunner::Match(const QString& searchTerm)
{
    setDelayedReply(true);

    if (m_lastRequest.type() != QDBusMessage::InvalidMessage) {
         QDBusConnection::sessionBus().send(m_lastRequest.createReply(QVariantList()));
    }

    m_lastRequest = message();
    m_searchTerm = searchTerm;

    // Baloo (as of 2014-11-20) is single threaded. It has an internal mutex which results in
    // queries being queued one after another. Also, Baloo is really really slow for small queries
    // For example - on my SSD, it takes about 1.4 seconds for 'f' with an SSD.
    // When searching for "fire", it results in "f", "fi", "fir" and then "fire" being searched
    // We're therefore hacking around this by having a small delay for very short queries so that
    // they do not get queued internally in Baloo
    //
    int waitTimeMs = 0;

    if (searchTerm.length() <= 3) {
        waitTimeMs = 100;
    }
    //we're still using the event delayed call even when the length is < 3 so that if we have multiple Match() calls in our DBus queue, we only process the last one
    m_timer->start(waitTimeMs);

    return RemoteMatches();
}

void SearchRunner::performMatch()
{
    RemoteMatches matches;
    matches << matchInternal(m_searchTerm, QStringLiteral("Audio"), i18n("Audio"));
    matches << matchInternal(m_searchTerm, QStringLiteral("Image"), i18n("Image"));
    matches << matchInternal(m_searchTerm, QStringLiteral("Document"), i18n("Document"));
    matches << matchInternal(m_searchTerm, QStringLiteral("Video"), i18n("Video"));
    matches << matchInternal(m_searchTerm, QStringLiteral("Folder"), i18n("Folder"));
    matches << matchInternal(m_searchTerm, QStringLiteral("Archive"), i18n("Archive"));
    matches << matchInternal(m_searchTerm, QStringLiteral("Spreadsheet"), i18n("Spreadsheet"));
    matches << matchInternal(m_searchTerm, QStringLiteral("Presentation"), i18n("Presentation"));

    QDBusConnection::sessionBus().send(m_lastRequest.createReply(QVariant::fromValue(matches)));
    m_lastRequest = QDBusMessage();
}

RemoteMatches SearchRunner::matchInternal(const QString& searchTerm, const QString &type, const QString &category)
{
    Baloo::Query query;
    query.setSearchString(searchTerm);
    query.setType(type);
    query.setLimit(10);

    Baloo::ResultIterator it = query.exec();

    RemoteMatches matches;

    QMimeDatabase mimeDb;

    // KRunner is absolutely daft and allows plugins to set the global
    // relevance levels. so Baloo should not set the relevance of results too
    // high because then Applications will often appear after if the application
    // runner has not a higher relevance. So stupid.
    // Each runner plugin should not have to know about the others.
    // Anyway, that's why we're starting with .75
    float relevance = .75;
    while (it.next()) {
        RemoteMatch match;
        QString localUrl = it.filePath();
        const QUrl url = QUrl::fromLocalFile(localUrl);
        match.id = it.filePath();
        match.text = url.fileName();
        match.iconName = mimeDb.mimeTypeForFile(localUrl).iconName();
        match.relevance = relevance;
        match.type = Plasma::QueryMatch::PossibleMatch;
        QVariantMap properties;

        QString folderPath = url.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).toLocalFile();
        if (folderPath.startsWith(QDir::homePath())) {
            folderPath.replace(0, QDir::homePath().length(), QStringLiteral("~"));
        }

        properties[QStringLiteral("urls")] = QStringList({QString::fromLocal8Bit(url.toEncoded())});
        properties[QStringLiteral("subtext")] = folderPath;
        properties[QStringLiteral("category")] = category;

        match.properties = properties;
        relevance -= 0.05;

         matches << match;
    }

    return matches;
}

void SearchRunner::Run(const QString& id, const QString& actionId)
{
    const QUrl url = QUrl::fromLocalFile(id);
    if (actionId == s_openParentDirId) {
        KIO::highlightInFileManager({url});
        return;
    }

    new KRun(url, nullptr);
}
