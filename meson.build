project('editor', 'cpp',
  version : '0.1',
  default_options : [
	'cpp_std=c++20',
	'werror=true',
	'warning_level=3',
	#'b_sanitize=address'
  ])


catch2_dep = dependency('catch2')

srcs = []
test_srcs = []

# Platform
# POSIX-compliant platforms
if 'linux darwin freebsd netbsd openbsd'.contains(host_machine.system())
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


exe = executable('editor', srcs + [ 'src/EntryMain.cxx' ])

test_exe = executable('editor_tests',
  srcs + test_srcs + [ 'src/EntryTest.cxx' ],
  dependencies: catch2_dep)


test('catch2', test_exe, args: ['-r', 'tap'], protocol: 'tap')
