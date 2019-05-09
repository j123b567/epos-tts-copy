cmake_minimum_required(VERSION 2.8)
project(epos)
add_executable(say-epos src/say.cc)
add_executable(eposd src/agent.cc src/client.cc src/daemon.cc src/encoding.cc src/function.cc src/hashd.cc src/interf.cc src/ktdsyn.cc src/lpcsyn.cc src/nonblock.cc src/options.cc src/parser.cc src/ptdsyn.cc src/rule.cc src/synth.cc src/tcpsyn.cc  src/tdpsyn.cc src/text.cc src/ttscp.cc src/unit.cc src/voice.cc src/waveform.cc)

install(TARGETS say-epos eposd DESTINATION bin)
install(DIRECTORY cfg/ DESTINATION share/epos)

set(CMAKE_CXX_FLAGS "-fno-delete-null-pointer-checks -funsigned-char -Wno-format" )

add_definitions(-DBASE_DIR=${CMAKE_INSTALL_PREFIX}/share/epos)
