/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *  Copyright 2014 Sebastian Kügler <sebas@kde.org>
 *  Copyright 2014 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.5
import QtQuick.Controls 2.3 as QQC2
import QtQuick.Window 2.2
import QtGraphicalEffects 1.0
import org.kde.plasma.wallpapers.image 2.0 as Wallpaper
import org.kde.plasma.core 2.0 as PlasmaCore

QQC2.StackView {
    id: root

    readonly property string modelImage: imageWallpaper.wallpaperPath
    readonly property string configuredImage: wallpaper.configuration.Image
    readonly property int fillMode: wallpaper.configuration.FillMode
    readonly property string configColor: wallpaper.configuration.Color
    readonly property bool blur: wallpaper.configuration.Blur
    readonly property size sourceSize: Qt.size(root.width * Screen.devicePixelRatio, root.height * Screen.devicePixelRatio)

    //public API, the C++ part will look for those
    function setUrl(url) {
        wallpaper.configuration.Image = url
        imageWallpaper.addUsersWallpaper(url);
    }

    function action_next() {
        imageWallpaper.nextSlide();
    }

    function action_open() {
        Qt.openUrlExternally(currentImage.source)
    }

    //private

    onConfiguredImageChanged: {
        imageWallpaper.addUrl(configuredImage)
    }
    Component.onCompleted: {
        if (wallpaper.pluginName == "org.kde.slideshow") {
            wallpaper.setAction("open", i18nd("plasma_applet_org.kde.image", "Open Wallpaper Image"), "document-open");
            wallpaper.setAction("next", i18nd("plasma_applet_org.kde.image","Next Wallpaper Image"),"user-desktop");
        }
        loadImage();
    }

    Wallpaper.Image {
        id: imageWallpaper
        //the oneliner of difference between image and slideshow wallpapers
        renderingMode: (wallpaper.pluginName == "org.kde.image") ? Wallpaper.Image.SingleImage : Wallpaper.Image.SlideShow
        targetSize: Qt.size(root.width, root.height)
        slidePaths: wallpaper.configuration.SlidePaths
        slideTimer: wallpaper.configuration.SlideInterval
    }

    Timer {
        id: loadTimer
        interval: 0 //FIXME DAVE - this is unmergable rubbish
        running: false
        onTriggered: loadImage()
    }
    onFillModeChanged: loadTimer.start();
    onModelImageChanged: loadTimer.start();
    onConfigColorChanged: loadTimer.start();
    onBlurChanged: loadTimer.start();
    onWidthChanged: loadTimer.start();
    onHeightChanged: loadTimer.start();

    function loadImage() {
        console.log("PUSHING", root.modelImage); //FIXME  we end up doing this twice on boot.
        root.replace(baseImage,
                    {//copy current config options, do not bind. Then we animate when we change anything
                        "source": root.modelImage,
                        "fillMode": root.fillMode,
                        "sourceSize": root.sourceSize,
                        "color": root.configColor,
                        "blur": root.blur,
                        "opacity": 0},
                    root.currentItem ? QQC2.StackView.Transition : QQC2.StackView.Immediate);//dont' animate first show
    }

    Component {
        id: baseImage

        Image {
            id: mainImage
            property alias color: backgroundColor.color
            property alias blur: blurEffect.visible

            asynchronous: true
            cache: false
            source: imageA.source
            z: -1

            Rectangle {
                id: backgroundColor
                anchors.fill: parent
                visible: mainImage.status === Image.Ready
                z: -2
            }

            GaussianBlur {
                id: blurEffect
                anchors.fill: parent
                source: mainImage
                radius: 32
                samples: 65
                z: mainImage.z
            }
        }
    }

    replaceEnter: Transition {
        OpacityAnimator {
            from: 0
            to: 1
            duration: wallpaper.configuration.TransitionAnimationDuration
        }
    }
    replaceExit: Transition {
        OpacityAnimator {
            from: 1
            to: 0
            duration: wallpaper.configuration.TransitionAnimationDuration
        }
    }
}
