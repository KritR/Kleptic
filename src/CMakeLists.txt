add_library(KlepticServer server.cxx mime_types.cxx tcp_sock.cxx socket.cxx concurrency.cxx http.cxx handler.cxx base64.cxx logger.cxx template.cxx router.cxx tls_sock.cxx sysv_ipc.cxx)


find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

target_include_directories(KlepticServer PRIVATE ${OPENSSL_INCLUDE_DIR})
#list(APPEND LIB_LIST ${OPENSSL_LIBRARIES})

target_link_libraries(KlepticServer ${OPENSSL_LIBRARIES})

target_link_libraries(KlepticServer ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(KlepticServer stdc++fs dl)
