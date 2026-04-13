project(
  'cchat',
  'c',
  version: '0.4.0',
  default_options: [
    'c_std=c17',
    'warning_level=2'
  ]
)

thread_dep = dependency('threads')

inc = include_directories(
  'src/common',
  'src/server',
  'src/client'
)

server_sources = files(
  'src/server/main.c',
  'src/server/server.c',
  'src/server/state.c'
)

client_sources = files(
  'src/client/client.c'
)

if get_option('build_server')
  executable(
    'cchat_server',
    server_sources,
    include_directories: inc,
    dependencies: thread_dep,
    install: false
  )
endif

if get_option('build_client')
  executable(
    'cchat_client',
    client_sources,
    include_directories: inc,
    dependencies: thread_dep,
    install: false
  )
endif