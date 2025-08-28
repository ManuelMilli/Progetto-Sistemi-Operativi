project('sha256server', 'c')

gcc = meson.get_compiler('c')
if get_option('debug')
  add_project_arguments('-DDEBUG', language: 'c')
endif

include_directories('inc')

ssl_lib = gcc.find_library('ssl')
crypto_lib = gcc.find_library('crypto')

commons = [ 'src/common.c', 'src/semaphores.c' ]
executable('client', 'src/client.c', commons, dependencies: [ssl_lib, crypto_lib])
executable('server', 'src/server.c', commons, dependencies: [ssl_lib, crypto_lib])
