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
import QtQuick.Controls 2.1 as QQC2
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

    // holds the image instance we're about to push onto the stack when it's finished loading
    property Item pendingImageItem: null

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
        Qt.callLater(loadImage);
    }

    Wallpaper.Image {
        id: imageWallpaper
        //the oneliner of difference between image and slideshow wallpapers
        renderingMode: (wallpaper.pluginName == "org.kde.image") ? Wallpaper.Image.SingleImage : Wallpaper.Image.SlideShow
        targetSize: Qt.size(root.width, root.height)
        slidePaths: wallpaper.configuration.SlidePaths
        slideTimer: wallpaper.configuration.SlideInterval
    }

    onFillModeChanged: Qt.callLater(loadImage);
    onModelImageChanged: Qt.callLater(loadImage);
    onConfigColorChanged: Qt.callLater(loadImage);
    onBlurChanged: Qt.callLater(loadImage);
    onWidthChanged: Qt.callLater(loadImage);
    onHeightChanged: Qt.callLater(loadImage);

    function loadImage() {
        console.log("About to load image", root.modelImage);
        if (pendingImageItem) {
            console.warn("Loading new image while already in the process of loading one");
            // Is this soon enough so it breaks our statusChanged binding before we w
            pendingImageItem.destroy();
            pendingImageItem = null;
        }

        var isStartup = !root.currentItem;

        var imageItem = baseImage.createObject(root, {
            //copy current config options, do not bind. Then we animate when we change anything
            source: root.modelImage,
            fillMode: root.fillMode,
            sourceSize: root.sourceSize,
            color: root.configColor,
            blur: root.blur,
            opacity: isStartup ? 1 : 0 // don't animate first show
        });

        imageItem.statusChanged.connect(function () {
            // animate only once image has been loaded or failed to
            if (imageItem.status !== Image.Loading) {
                console.log("Now transitioning to image", root.modelImage);
                root.replace(imageItem, isStartup ? QQC2.StackView.Immediate : QQC2.StackView.Transition);
                // need to unset it again or else it destroys the old image before the transition
                pendingImageItem = null;
            }
        });

        pendingImageItem = imageItem;
    }

    Component {
        id: baseImage

        Item {
            id: imageContainer

            property alias source: mainImage.source
            property alias fillMode: mainImage.fillMode
            property alias sourceSize: mainImage.sourceSize
            property alias color: backgroundColor.color
            property alias blur: blurEffect.visible

            property alias status: mainImage.status

            QQC2.StackView.onRemoved: destroy()

            Rectangle {
                id: backgroundColor
                anchors.fill: parent
            }

            Image {
                id: mainImage
                anchors.fill: parent
                asynchronous: true
                cache: false
            }

            GaussianBlur {
                id: blurEffect
                anchors.fill: parent
                source: mainImage
                radius: 32
                samples: 65
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
    // Keep the old image around till the new one is fully faded in
    // If we fade both at the same time you can see the background behind glimpse through
    replaceExit: Transition{
        PauseAnimation {
            duration: wallpaper.configuration.TransitionAnimationDuration
        }
    }
}
