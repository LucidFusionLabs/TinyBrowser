# $Id: CMakeLists.txt 1320 2014-10-26 08:12:52Z justin $

set(LFL_SINGLE_THREADED_BROWSER_SUPPORT 1)
set(LFL_V8JS 1)
lfl_set_os_toolkit(BROWSER)

if(NOT LFL_MOBILE)
  lfl_add_package(layout_tests SOURCES layout_tests.cpp
                  LINK_LIBRARIES ${LFL_APP_LIB} ${BROWSER_FRAMEWORK} ${BROWSER_GRAPHICS}
                  app_null_audio app_null_camera app_null_matrix app_null_fft
                  app_simple_resampler app_simple_loader ${LFL_APP_CONVERT} app_libpng_png
                  app_libjpeg_jpeg app_giflib_gif app_null_ogg app_libcss_css app_null_ttf
                  app_null_ssl app_null_js app_null_tu ${BROWSER_TOOLKIT} ${LFL_APP_OS}) 
endif()

lfl_project(lbrowser-render-sandbox)
lfl_add_package(lbrowser-render-sandbox SOURCES render.cpp
                LINK_LIBRARIES ${LFL_APP_LIB} app_null_framework
                app_null_graphics app_null_audio app_null_camera app_null_matrix app_null_fft
                app_simple_resampler app_simple_loader ${LFL_APP_CONVERT} app_libpng_png
                app_libjpeg_jpeg app_giflib_gif app_null_ogg app_libcss_css app_null_ttf
                app_null_ssl app_v8_js app_null_tu ${LFL_APP_OS})
lfl_post_build_copy_bin(lbrowser-render-sandbox lbrowser-render-sandbox LBrowser)

lfl_project(lbrowser)
if(LFL_SINGLE_THREADED_BROWSER_SUPPORT)
  set(libs ${LFL_APP_LIB} ${BROWSER_FRAMEWORK} ${BROWSER_GRAPHICS}
      ${LFL_APP_AUDIO} ${LFL_APP_CAMERA} app_null_matrix app_null_fft
      app_simple_resampler app_simple_loader ${LFL_APP_CONVERT} app_libpng_png
      app_libjpeg_jpeg app_giflib_gif app_null_ogg app_libcss_css app_null_ssl
      app_null_ttf app_v8_js app_null_tu ${BROWSER_TOOLKIT} ${LFL_APP_OS})
else()
  set(libs ${LFL_APP_LIB} ${BROWSER_FRAMEWORK} ${BROWSER_GRAPHICS}
      ${LFL_APP_AUDIO} ${LFL_APP_CAMERA} app_null_matrix app_null_fft
      app_simple_resampler app_simple_loader ${LFL_APP_CONVERT} app_libpng_png
      app_null_jpeg app_null_gif app_null_ogg app_null_css app_null_ttf
      app_openssl_ssl app_null_js app_null_tu ${BROWSER_TOOLKIT} ${LFL_APP_OS})
endif()
lfl_add_package(lbrowser SOURCES browser.cpp LINK_LIBRARIES ${libs})
if(NOT LFL_MOBILE)
  add_dependencies(lbrowser ffmpeg)
endif()

set(LFL_APP_ASSET_FILES ${LFL_APP_ASSET_FILES} ${LFL_SOURCE_DIR}/core/app/assets/FreeSans.ttf
    ${LFL_SOURCE_DIR}/core/app/assets/DejaVuSans*.ttf ${LFL_SOURCE_DIR}/core/app/assets/VeraMoBd.ttf,32,*)
set(LFL_APP_LIB_FILES ${LFL_APP_LIB_FILES} ${PORTAUDIO_LIB_FILES} ${FFMPEG_LIB_FILES})
lfl_post_build_start(lbrowser lbrowser LBrowser)
