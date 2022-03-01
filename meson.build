project('editor', 'cpp',
  version : '0.1',
  default_options : [
	'cpp_std=c++20',
	'werror=true',
	'warning_level=3',
	'b_sanitize=address'
  ])

deps = [
  dependency('GL'),
  dependency('sdl2', default_options: ['werror=false', 'warning_level=0']),
]

srcs = []
test_srcs = []

# Platform
# POSIX-compliant platforms
if 'linux darwin freebsd netbsd openbsd'.contains(host_machine.system())
  if 'darwin' == host_machine.system()
    deps += [dependency('CoreText'), dependency('CoreFoundation')]
    srcs += ['src/Platform/LocateFontCoreText.cxx']
  else
    deps += dependency('fontconfig')
    srcs += ['src/Platform/LocateFontFontConfig.cxx']
  endif
  
  srcs += [
    'src/Platform/VirtualMemoryPosix.cxx'
  ]
elif 'windows' == host_machine.system()
  error('todo')
else
  error('unsupported platform')
endif

# Util
srcs += 'src/Util/Assert.cxx'

# TextBuffer
srcs += [
  'src/TextBuffer/iterator.cxx',
  'src/TextBuffer/TextBuffer.cxx'
]

test_srcs += [
  'src/TextBuffer/AppendBufferTest.cxx',
  'src/TextBuffer/TextBufferTest.cxx'
]

# Render
srcs += [
  'src/Render/Shader.cxx',
  'src/Render/RenderContext.cxx',
  'src/Render/GlyphAtlas.cxx',
  'src/Render/TileMap.cxx'
]

# UI
srcs += [
  'src/UI/ViewEditor.cxx'
]

exe = executable('editor',
srcs + [ 'src/EntryMain.cxx' ],
dependencies: deps)

# TODO: disable test build by default
test_exe = executable('editor_tests',
  srcs + test_srcs + [ 'src/EntryTest.cxx' ],
  # for some reason threads needs to be specified as a dependency TODO: find out why
  dependencies: deps + [dependency('catch2'), dependency('threads')],
  build_by_default: false)
 
run_target('run_tests', command: test_exe)

clang_tidy = find_program('clang-tidy', required: false)
if clang_tidy.found()
  run_target('lint', command: [
    clang_tidy,'-p', meson.build_root(),
    '-checks=performance-*,readability-*,portability-*,-readability-implicit-bool-conversion'] + files(srcs))
endif
# ,bugprone-*,cert-*,google-*