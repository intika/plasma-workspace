/*
 *   Copyright (C) 2010 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "splashwindow.h"
#include "splashapp.h"

#include <QQmlContext>
#include <QQuickItem>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <KSharedConfig>
#include <KConfigGroup>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include <KWayland/Client/surface.h>
#include <KWayland/Client/plasmashell.h>
#include <KWindowSystem>

SplashWindow::SplashWindow(bool testing, bool window, const QString &theme)
    : KQuickAddons::QuickViewSharedEngine(),
      m_stage(0),
      m_testing(testing),
      m_window(window),
      m_theme(theme)
{
    setColor(Qt::transparent);
    setDefaultAlphaBuffer(true);
    setClearBeforeRendering(true);
    setResizeMode(KQuickAddons::QuickViewSharedEngine::SizeRootObjectToView);

    if (!m_window) {
        setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    }

    if (!m_testing && !m_window) {
        if (KWindowSystem::isPlatformX11()) {
            // X11 specific hint only on X11
            setFlags(Qt::BypassWindowManagerHint);
        } else if (!KWindowSystem::isPlatformWayland()) {
            // on other platforms go fullscreen
            // on Wayland we cannot go fullscreen due to QTBUG 54883
            setWindowState(Qt::WindowFullScreen);
        }
    }

    if (m_testing && !m_window && !KWindowSystem::isPlatformWayland()) {
        setWindowState(Qt::WindowFullScreen);
    }

    //be sure it will be eventually closed
    //FIXME: should never be stuck
    QTimer::singleShot(30000, this, &QWindow::close);
}

void SplashWindow::setStage(int stage)
{
    m_stage = stage;

    rootObject()->setProperty("stage", stage);
}

bool SplashWindow::event(QEvent *e)
{
    if (e->type() == QEvent::PlatformSurface) {
        if (auto pe = dynamic_cast<QPlatformSurfaceEvent*>(e)) {
            switch (pe->surfaceEventType()) {
            case QPlatformSurfaceEvent::SurfaceCreated:
                setupWaylandIntegration();
                break;
            case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
                delete m_shellSurface;
                m_shellSurface = nullptr;
                break;
            }
        }
    }
    return KQuickAddons::QuickViewSharedEngine::event(e);
}

void SplashWindow::keyPressEvent(QKeyEvent *event)
{
    KQuickAddons::QuickViewSharedEngine::keyPressEvent(event);
    if (m_testing && !event->isAccepted() && event->key() == Qt::Key_Escape) {
        close();
    }
}

void SplashWindow::mousePressEvent(QMouseEvent *event)
{
    KQuickAddons::QuickViewSharedEngine::mousePressEvent(event);
    if (m_testing && !event->isAccepted()) {
        close();
    }
}

void SplashWindow::setGeometry(const QRect& rect)
{
    bool oldGeometryEmpty = geometry().isNull();
    KQuickAddons::QuickViewSharedEngine::setGeometry(rect);

    if (oldGeometryEmpty) {
        bool loaded = false;
        if (!m_theme.isEmpty()) {
            KPackage::Package splashPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/SplashScreen"));
            splashPackage.setPath(m_theme);
            if (splashPackage.isValid()) {
                setSource(QUrl::fromLocalFile(splashPackage.filePath("splashmainscript")));
                loaded = splashPackage.isValid();
            }
        }
        
        if (!loaded) {
            KPackage::Package lnfPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
            KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), "KDE");
            const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
            if (!packageName.isEmpty()) {
                lnfPackage.setPath(packageName);
            }

            if (!m_theme.isEmpty()) {
                lnfPackage.setPath(m_theme);
            }

            Q_ASSERT(lnfPackage.isValid());
            setSource(QUrl::fromLocalFile(lnfPackage.filePath("splashmainscript")));
        }
    }

    if (m_shellSurface) {
        m_shellSurface->setPosition(geometry().topLeft());
    }
}

void SplashWindow::setupWaylandIntegration()
{
    if (m_shellSurface) {
        // already setup
        return;
    }
    if (SplashApp *a = qobject_cast<SplashApp*>(qApp)) {
        using namespace KWayland::Client;
        PlasmaShell *interface = a->waylandPlasmaShellInterface();
        if (!interface) {
            return;
        }
        Surface *s = Surface::fromWindow(this);
        if (!s) {
            return;
        }
        m_shellSurface = interface->createSurface(s, this);
        // Use OSD to make it go above all other windows
        // that's the closest we have to the X11 unmanged layer we have on Wayland
        m_shellSurface->setRole(PlasmaShellSurface::Role::OnScreenDisplay);
        m_shellSurface->setPosition(geometry().topLeft());
    }
}
