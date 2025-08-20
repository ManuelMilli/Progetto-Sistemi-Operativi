project('sha256server', 'c')

gcc = meson.get_compiler('c')

ssl_lib = gcc.find_library('ssl')
crypto_lib = gcc.find_library('crypto')

executable('client', 'client.c', 'common.c', dependencies: [ssl_lib, crypto_lib])
executable('server', 'server.c', 'common.c', dependencies: [ssl_lib, crypto_lib])