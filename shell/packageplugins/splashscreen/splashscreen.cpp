/******************************************************************************
*   Copyright 2007-2009 by Aaron Seigo <aseigo@kde.org>                       *
*   Copyright 2013 by Sebastian Kügler <sebas@kde.org>                        *
*                                                                             *
*   This library is free software; you can redistribute it and/or             *
*   modify it under the terms of the GNU Library General Public               *
*   License as published by the Free Software Foundation; either              *
*   version 2 of the License, or (at your option) any later version.          *
*                                                                             *
*   This library is distributed in the hope that it will be useful,           *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU          *
*   Library General Public License for more details.                          *
*                                                                             *
*   You should have received a copy of the GNU Library General Public License *
*   along with this library; see the file COPYING.LIB.  If not, write to      *
*   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
*   Boston, MA 02110-1301, USA.                                               *
*******************************************************************************/

#include "splashscreen.h"

#include <KLocalizedString>
#include <KPackage/PackageLoader>

void SplashScreenPackage::initPackage(KPackage::Package *package)
{
    // http://community.kde.org/Plasma/SplashScreenPackage#
    package->setDefaultPackageRoot(QStringLiteral("plasma/splashscreen/"));

    //Directories
    package->addDirectoryDefinition("previews", QStringLiteral("previews"), i18n("Preview Images"));
    package->addFileDefinition("splashpreview", QStringLiteral("previews/splash.png"), i18n("Preview for Splash Screen"));

    package->addDirectoryDefinition("splash", QStringLiteral("splash"), i18n("Splash Screen"));
    package->addFileDefinition("splashmainscript", QStringLiteral("splash/Splash.qml"), i18n("Main Script for Splash Screen"));
}


K_EXPORT_KPACKAGE_PACKAGE_WITH_JSON(SplashScreenPackage, "plasma-packagestructure-splashscreen.json")

#include "splashscreen.moc"
