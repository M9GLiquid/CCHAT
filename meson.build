project(
  'cchat',
  'c',
  version: '0.5.0',
  default_options: ['c_std=c17', 'warning_level=2'],
)

thread_dep = dependency('threads')
czmq_dep = dependency('libczmq')
project_root = meson.project_source_root()
source_root = 'src'
client_version = get_option('client_version')
server_version = get_option('server_version')
configure_file(
  input: 'src/common/version.h.in',
  output: 'version.h',
  configuration: {
    'CLIENT_VERSION': client_version,
    'SERVER_VERSION': server_version,
  },
)

# src is the active build target for runtime binaries.

inc = include_directories(
  source_root / 'common',
  source_root / 'server',
  source_root / 'client',
)
build_inc = include_directories('.')

src_groups = {}

foreach group : ['server', 'client', 'common']
  src_lines = run_command(
    'bash',
    '-lc', 'cd '
    + project_root
    + ' && find @0@/@1@ -name "*.c" -print | sort'.format(source_root, group),
    check: true,
  ).stdout().strip().split('\n')

  src_paths = []
  foreach line : src_lines
    if line != ''
      src_paths += line
    endif
  endforeach
  src_groups += {group: src_paths}
endforeach

src_server_source_paths = src_groups['server']
src_client_source_paths = src_groups['client']
src_common_source_paths = src_groups['common']

server_sources = files(src_server_source_paths)
client_sources = files(src_client_source_paths)
common_sources = files(src_common_source_paths)
all_src_sources = files(src_server_source_paths + src_client_source_paths + src_common_source_paths)

static_library(
  'cchat_src_index',
  all_src_sources,
  include_directories: [inc, build_inc],
  dependencies: [thread_dep, czmq_dep],
  build_by_default: false,
  install: false,
)

if get_option('build_server')
  executable(
    'cchat_server',
    server_sources + common_sources,
    include_directories: [inc, build_inc],
    dependencies: [thread_dep, czmq_dep],
    install: false,
  )
endif

if get_option('build_client')
  executable(
    'cchat_client',
    client_sources + common_sources,
    include_directories: [inc, build_inc],
    dependencies: [thread_dep, czmq_dep],
    install: false,
  )
endif
