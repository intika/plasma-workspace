add_definitions(-DTRANSLATION_DOMAIN=\"plasma_package_splashscreen\")

set(splashscreen_SRCS
    splashscreen.cpp
)

add_library(plasma_packagestructure_splashscreen MODULE ${splashscreen_SRCS})
kcoreaddons_desktop_to_json(plasma_packagestructure_splashscreen plasma-packagestructure-splashscreen.desktop)

target_link_libraries(plasma_packagestructure_splashscreen
   KF5::I18n
   KF5::Package
)

install(TARGETS plasma_packagestructure_splashscreen DESTINATION ${KDE_INSTALL_PLUGINDIR}/kpackage/packagestructure)
