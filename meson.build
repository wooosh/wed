project('editor', 'cpp',
  version : '0.1',
  default_options : [
	'cpp_std=c++20',
	'werror=true',
	'warning_level=3',
	#'b_sanitize=address'
  ])

deps = [
  dependency('sdl2'),
  dependency('SDL2_ttf')
]

srcs = []
test_srcs = []

# Platform
# POSIX-compliant platforms
if 'linux darwin freebsd netbsd openbsd'.contains(host_machine.system())
  if 'darwin' == host_machine.system()
    error('todo')
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

# UI
srcs += [
  'src/UI/Backend.cxx',
  'src/UI/Font.cxx',
  'src/UI/ViewEditor.cxx'
]

exe = executable('editor',
srcs + [ 'src/EntryMain.cxx' ],
dependencies: deps)

test_exe = executable('editor_tests',
  srcs + test_srcs + [ 'src/EntryTest.cxx' ],
  # for some reason threads needs to be specified as a dependency TODO: find out why
  dependencies: deps + [dependency('catch2'), dependency('threads')])


test('catch2', test_exe, args: ['-r', 'tap'], protocol: 'tap')
