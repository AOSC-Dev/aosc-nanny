set(nanny_SRCS main.cpp)

add_executable(nanny ${nanny_SRCS})
target_link_libraries(nanny
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
    PRIVATE
        KF${KF_MAJOR_VERSION}::CoreAddons
        KF${KF_MAJOR_VERSION}::I18n
        KF${KF_MAJOR_VERSION}::IconThemes
)
install(TARGETS nanny ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
