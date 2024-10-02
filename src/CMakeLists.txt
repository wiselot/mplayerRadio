aux_source_directory(. src_list)

find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(ALSA REQUIRED)
pkg_check_modules(LIBMPG123 REQUIRED libmpg123)
pkg_check_modules(LIBAO REQUIRED ao)


include_directories(${SQLite3_INCLUDE_DIRS}
                    ${ALSA_INCLUDE_DIRS}
                    ${LIBMPG123_INCLUDE_DIRS}
                    ${LIBAO_INCLUDE_DIRS})

add_executable(mplayerRadioMain ${src_list})
target_link_libraries(mplayerRadioMain SQLite::SQLite3 ALSA::ALSA ${LIBMPG123_LIBRARIES} ${LIBAO_LIBRARIES})