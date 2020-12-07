link_libraries(KlepticServer)
add_executable(EchoServer echo_server.cxx)
target_include_directories(EchoServer PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_executable(HTTPServer http_server.cxx)
target_include_directories(HTTPServer PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_executable(httpd httpd.cxx)
target_include_directories(httpd PRIVATE ${CMAKE_SOURCE_DIR}/src)

set_target_properties(httpd 
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

add_executable(parse_mime parse_mime.cxx)
target_include_directories(parse_mime PRIVATE ${CMAKE_SOURCE_DIR}/src)
