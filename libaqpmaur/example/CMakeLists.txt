set(aursearcher_test_SRCS
main_aursearcher.cpp
)

automoc4_add_executable(aursearcher
${aursearcher_test_SRCS}
)

target_link_libraries(aursearcher
${QT_QTCORE_LIBRARY}
aqpmaur
)
