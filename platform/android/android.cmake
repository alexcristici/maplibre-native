if(NOT ANDROID_NDK_TOOLCHAIN_INCLUDED)
    message(FATAL_ERROR "-- Toolchain file not included, see https://developer.android.com/ndk/guides/cmake")
endif()

target_compile_definitions(
    mbgl-core
    PUBLIC
)

include(${PROJECT_SOURCE_DIR}/vendor/icu.cmake)
include(${PROJECT_SOURCE_DIR}/vendor/sqlite.cmake)

# cmake-format: off
target_compile_options(mbgl-vendor-csscolorparser PRIVATE $<$<CONFIG:Release>:-Oz> $<$<CONFIG:Release>:-Qunused-arguments> $<$<CONFIG:Release>:-flto>)
target_compile_options(mbgl-vendor-icu PRIVATE $<$<CONFIG:Release>:-Oz> $<$<CONFIG:Release>:-Qunused-arguments> $<$<CONFIG:Release>:-flto>)
target_compile_options(mbgl-vendor-parsedate PRIVATE $<$<CONFIG:Release>:-Oz> $<$<CONFIG:Release>:-Qunused-arguments> $<$<CONFIG:Release>:-flto>)
target_compile_options(mbgl-vendor-sqlite PRIVATE $<$<CONFIG:Release>:-Oz> $<$<CONFIG:Release>:-Qunused-arguments> $<$<CONFIG:Release>:-flto>)
target_compile_options(mbgl-compiler-options INTERFACE $<$<CONFIG:Release>:-Oz> $<$<CONFIG:Release>:-Qunused-arguments> $<$<CONFIG:Release>:-flto>)
# cmake-format: on

target_link_libraries(
    mbgl-compiler-options
    INTERFACE
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Release>:-Wl,--icf=all>
        $<$<CONFIG:Release>:-flto>
        $<$<CONFIG:Release>:-fuse-ld=gold>
)

target_sources(
    mbgl-core
    PRIVATE
        ${PROJECT_SOURCE_DIR}/platform/android/src/async_task.cpp
        ${PROJECT_SOURCE_DIR}/platform/android/src/attach_env.cpp
        ${PROJECT_SOURCE_DIR}/platform/android/src/attach_env.hpp
        ${PROJECT_SOURCE_DIR}/platform/android/src/bitmap.cpp
        ${PROJECT_SOURCE_DIR}/platform/android/src/bitmap.hpp
        ${PROJECT_SOURCE_DIR}/platform/android/src/bitmap_factory.cpp
        ${PROJECT_SOURCE_DIR}/platform/android/src/bitmap_factory.hpp
        ${PROJECT_SOURCE_DIR}/platform/android/src/image.cpp
        ${PROJECT_SOURCE_DIR}/platform/android/src/jni.cpp
        ${PROJECT_SOURCE_DIR}/platform/android/src/jni.hpp
        ${PROJECT_SOURCE_DIR}/platform/android/src/run_loop.cpp
        ${PROJECT_SOURCE_DIR}/platform/android/src/run_loop_impl.hpp
        ${PROJECT_SOURCE_DIR}/platform/android/src/string_util.cpp
        ${PROJECT_SOURCE_DIR}/platform/android/src/thread.cpp
        ${PROJECT_SOURCE_DIR}/platform/android/src/timer.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/gfx/headless_backend.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/gfx/headless_frontend.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/map/map_snapshotter.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/platform/time.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/asset_file_source.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/database_file_source.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/file_source_manager.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/file_source_request.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/local_file_request.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/local_file_source.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/mbtiles_file_source.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/main_resource_loader.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/offline.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/offline_database.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/offline_download.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/online_file_source.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/$<IF:$<BOOL:${MLN_WITH_PMTILES}>,pmtiles_file_source.cpp,pmtiles_file_source_stub.cpp>
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/sqlite3.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/text/bidi.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/util/compression.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/util/filesystem.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/util/monotonic_timer.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/util/png_writer.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/util/thread_local.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/util/utf.cpp
        ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/layermanager/layer_manager.cpp
)

if(MLN_WITH_OPENGL)
    target_sources(
        mbgl-core
        PRIVATE
            ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/gl/headless_backend.cpp
            ${PROJECT_SOURCE_DIR}/platform/linux/src/headless_backend_egl.cpp
            ${PROJECT_SOURCE_DIR}/platform/android/src/gl_functions.cpp
    )
endif()

if(MLN_WITH_VULKAN)
    target_sources(
        mbgl-core
        PRIVATE
            ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/vulkan/headless_backend.cpp
    )
endif()

target_include_directories(
    mbgl-core
    PRIVATE ${PROJECT_SOURCE_DIR}/platform/default/include
)

target_link_libraries(
    mbgl-core
    PRIVATE
        EGL
        GLESv3
        MapLibreNative::Base::jni.hpp
        android
        atomic
        jnigraphics
        log
        mbgl-vendor-icu
        mbgl-vendor-sqlite
        z
)

add_library(
    example-custom-layer MODULE
    ${PROJECT_SOURCE_DIR}/platform/android/src/example_custom_layer.cpp
)

target_include_directories(
    example-custom-layer
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
    example-custom-layer
    PRIVATE
        GLESv3
        MapLibreNative::Base
        log
        mbgl-compiler-options
)

add_library(
    mbgl-test-runner SHARED
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    ${PROJECT_SOURCE_DIR}/platform/android/src/test/test_runner.cpp
    ${PROJECT_SOURCE_DIR}/platform/android/src/test/test_runner_common.cpp
    ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/text/local_glyph_rasterizer.cpp
    ${PROJECT_SOURCE_DIR}/platform/android/src/test/collator_test_stub.cpp
    ${PROJECT_SOURCE_DIR}/platform/android/src/test/number_format_test_stub.cpp
)

target_include_directories(
    mbgl-test-runner
    PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue
    ${PROJECT_SOURCE_DIR}/platform/android/src
    ${PROJECT_SOURCE_DIR}/src
)

# this is needed because Android is not officially supported
# https://discourse.cmake.org/t/error-when-crosscompiling-with-whole-archive-target-link/9394
# https://cmake.org/cmake/help/latest/release/3.24.html#generator-expressions
set(CMAKE_LINK_LIBRARY_USING_WHOLE_ARCHIVE
"-Wl,--whole-archive <LIBRARY> -Wl,--no-whole-archive"
)
set(CMAKE_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED True)

find_package(curl CONFIG)

target_link_libraries(
    mbgl-test-runner
    PRIVATE
        MapLibreNative::Base::jni.hpp
        mbgl-compiler-options
        $<$<BOOL:${curl_FOUND}>:curl::curl_static>
        $<LINK_LIBRARY:WHOLE_ARCHIVE,mbgl-test>
)


target_sources(
    mbgl-test-runner
    PRIVATE ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/storage/http_file_source.cpp
)

add_custom_command(
    TARGET mbgl-test-runner PRE_BUILD
    COMMAND
        ${CMAKE_COMMAND}
        -E
        make_directory
        ${PROJECT_SOURCE_DIR}/test/results
    COMMAND
        ${CMAKE_COMMAND}
        -E
        tar
        "chf"
        "test/android/app/src/main/assets/data.zip"
        --format=zip
        --files-from=test/android/app/src/main/assets/to_zip.txt
    COMMAND
        ${CMAKE_COMMAND}
        -E
        remove_directory
        ${PROJECT_SOURCE_DIR}/test/results
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_library(
    mbgl-benchmark-runner SHARED
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    ${PROJECT_SOURCE_DIR}/platform/android/src/test/benchmark_runner.cpp
    ${PROJECT_SOURCE_DIR}/platform/android/src/test/test_runner_common.cpp
    ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/text/local_glyph_rasterizer.cpp
    ${PROJECT_SOURCE_DIR}/platform/android/src/test/collator_test_stub.cpp
    ${PROJECT_SOURCE_DIR}/platform/android/src/test/number_format_test_stub.cpp
    ${PROJECT_SOURCE_DIR}/platform/android/MapLibreAndroid/src/cpp/http_file_source.cpp
)

target_include_directories(
    mbgl-benchmark-runner
    PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue ${PROJECT_SOURCE_DIR}/platform/android/src ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(
    mbgl-benchmark-runner
    PRIVATE
        MapLibreNative::Base::jni.hpp
        mbgl-compiler-options
        $<LINK_LIBRARY:WHOLE_ARCHIVE,mbgl-benchmark>
)

add_custom_command(
    TARGET mbgl-benchmark-runner PRE_BUILD
    COMMAND
        ${CMAKE_COMMAND}
        -E
        make_directory
        ${PROJECT_SOURCE_DIR}/benchmark/results
    COMMAND
        ${CMAKE_COMMAND}
        -E
        tar
        "chf"
        "benchmark/android/app/src/main/assets/data.zip"
        --format=zip
        --files-from=benchmark/android/app/src/main/assets/to_zip.txt
    COMMAND
        ${CMAKE_COMMAND}
        -E
        remove_directory
        ${PROJECT_SOURCE_DIR}/benchmark/results
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_library(
    mbgl-render-test-runner SHARED
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    ${PROJECT_SOURCE_DIR}/platform/android/src/test/render_test_runner.cpp
    ${PROJECT_SOURCE_DIR}/platform/android/src/test/test_runner_common.cpp
    ${PROJECT_SOURCE_DIR}/platform/default/src/mbgl/text/local_glyph_rasterizer.cpp
    ${PROJECT_SOURCE_DIR}/platform/android/src/test/collator_test_stub.cpp
    ${PROJECT_SOURCE_DIR}/platform/android/src/test/number_format_test_stub.cpp
    ${PROJECT_SOURCE_DIR}/platform/android/MapLibreAndroid/src/cpp/http_file_source.cpp
)

if(MLN_WITH_VULKAN)
    target_compile_definitions(mbgl-render-test-runner PRIVATE "MLN_RENDER_BACKEND_VULKAN=1")
endif()

target_include_directories(
    mbgl-render-test-runner
    PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue ${PROJECT_SOURCE_DIR}/platform/android/src ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(
    mbgl-render-test-runner
    PRIVATE
        MapLibreNative::Base::jni.hpp
        android
        log
        mbgl-compiler-options
        mbgl-render-test
)

add_custom_command(
    TARGET mbgl-render-test-runner PRE_BUILD
    COMMAND
        ${CMAKE_COMMAND}
        -E
        tar
        "chf"
        "render-test/android/app/src/main/assets/data.zip"
        --format=zip
        --files-from=render-test/android/app/src/main/assets/to_zip.txt
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

install(TARGETS mbgl-render-test-runner LIBRARY DESTINATION lib)
