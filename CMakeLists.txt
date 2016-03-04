# $Id: CMakeLists.txt 1320 2014-10-26 08:12:52Z justin $

set(LFL_SINGLE_THREADED_BROWSER_SUPPORT 1)
set(LFL_V8JS 1)

lfl_project(lbrowser-render-sandbox)
lfl_add_target(lbrowser-render-sandbox render.cpp)
target_compile_definitions(lbrowser-render-sandbox PUBLIC ${LFL_APP_DEF})
target_include_directories(lbrowser-render-sandbox PUBLIC ${LFL_APP_INCLUDE})
target_link_libraries(lbrowser-render-sandbox ${LFL_APP_LIB} app_null_os app_null_framework app_null_graphics
                      app_null_audio app_null_camera app_null_matrix app_null_fft
                      app_simple_resampler app_simple_loader ${LFL_APP_CONVERT} app_libpng_png
                      app_libjpeg_jpeg app_giflib_gif app_libcss_css app_freetype_ttf app_null_ssl
                      app_v8_js app_null_tu)
lfl_post_build_copy_bin(lbrowser-render-sandbox lbrowser-render-sandbox LBrowser)

lfl_project(lbrowser)
lfl_add_target(lbrowser browser.cpp)
target_compile_definitions(lbrowser PUBLIC ${LFL_APP_DEF})
target_include_directories(lbrowser PUBLIC ${LFL_APP_INCLUDE})
if(LFL_SINGLE_THREADED_BROWSER_SUPPORT)
  target_link_libraries(lbrowser ${LFL_APP_LIB} ${LFL_APP_OS} ${LFL_APP_FRAMEWORK} ${LFL_APP_GRAPHICS}
                        ${LFL_APP_AUDIO} ${LFL_APP_CAMERA} app_null_matrix app_null_fft
                        app_simple_resampler app_simple_loader ${LFL_APP_CONVERT} app_libpng_png
                        app_libjpeg_jpeg app_giflib_gif app_libcss_css app_freetype_ttf app_openssl_ssl
                        app_v8_js app_null_tu)
else()
  target_link_libraries(lbrowser ${LFL_APP_LIB} ${LFL_APP_OS} ${LFL_APP_FRAMEWORK} ${LFL_APP_GRAPHICS}
                        ${LFL_APP_AUDIO} ${LFL_APP_CAMERA} app_null_matrix app_null_fft
                        app_simple_resampler app_simple_loader ${LFL_APP_CONVERT} app_libpng_png
                        app_null_jpeg app_null_gif app_null_css app_null_ttf app_openssl_ssl
                        app_null_js app_null_tu)
endif()
lfl_post_build_start(lbrowser lbrowser LBrowser)

if(NOT LFL_MOBILE)
  lfl_add_target(layout_tests layout_tests.cpp)
  target_compile_definitions(layout_tests PUBLIC ${LFL_APP_DEF})
  target_include_directories(layout_tests PUBLIC ${LFL_APP_INCLUDE})
  target_link_libraries(layout_tests ${LFL_APP_LIB} ${LFL_APP_OS} ${LFL_APP_FRAMEWORK} ${LFL_APP_GRAPHICS}
                        app_null_audio app_null_camera app_null_matrix app_null_fft
                        app_simple_resampler app_simple_loader ${LFL_APP_CONVERT} app_libpng_png
                        app_libjpeg_jpeg app_giflib_gif app_libcss_css app_freetype_ttf app_null_ssl
                        app_null_js app_null_tu)
endif()
