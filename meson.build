project(
  'cchat',
  'c',
  version: '0.5.0',
  default_options: [
    'c_std=c17',
    'warning_level=2'
  ]
)

thread_dep = dependency('threads')
czmq_dep = dependency('libczmq')
project_root = meson.project_source_root()

# src is the active build target for runtime binaries.
inc = include_directories('src/common', 'src/server', 'src/client')

# Auto-discover groups so adding .c files needs no meson.build edits.
src_groups = {}

foreach group : ['server', 'client', 'common']
  src_lines = run_command(
    'bash',
    '-lc',
    'cd ' + project_root + ' && find src/@0@ -maxdepth 1 -name "*.c" -print | sort'.format(group),
    check: true
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

# Non-default index target keeps src files in compile_commands.json.
static_library(
  'cchat_src_index',
  all_src_sources,
  include_directories: inc,
  dependencies: [thread_dep, czmq_dep],
  build_by_default: false,
  install: false
)

if get_option('build_server')
  executable(
    'cchat_server',
    server_sources + common_sources,
    include_directories: inc,
    dependencies: [thread_dep, czmq_dep],
    install: false
  )
endif

if get_option('build_client')
  executable(
    'cchat_client',
    client_sources + common_sources,
    include_directories: inc,
    dependencies: [thread_dep, czmq_dep],
    install: false
  )
endif