vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO your-org/cryptopix-clwe
    REF v${VERSION}
    SHA512 0  # TODO: Update with actual SHA512 hash when releasing
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DCMAKE_BUILD_TYPE=Release
        -DBUILD_PYTHON_BINDINGS=OFF
        -DENABLE_TESTS=OFF
        -DENABLE_BENCHMARKS=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME CLWE
    CONFIG_PATH lib/cmake/CLWE
)

vcpkg_fixup_pkgconfig()

# Remove debug files
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

# Install license
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

# Copy usage file
file(COPY "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")