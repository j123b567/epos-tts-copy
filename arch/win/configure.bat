# Run this script before opening the Visual C++ project file

call convert.bat

cd ..\..\src

move agent.cc agent.cpp
move client.cc client.cpp
move daemon.cc daemon.cpp
move unit.cc unit.cpp
move hash.cc hash.cpp
move interf.cc interf.cpp
move options.cc options.cpp
move ktdsyn.cc ktdsyn.cpp
move ptdsyn.cc ptdsyn.cpp
move lpcsyn.cc lpcsyn.cpp
move monolith.cc monolith.cpp
move parser.cc parser.cpp
move rule.cc rule.cpp
move say.cc say.cpp
move synth.cc synth.cpp
move tcpsyn.cc tcpsyn.cpp
move text.cc text.cpp
move ttscp.cc ttscp.cpp
move voice.cc voice.cpp
move waveform.cc waveform.cpp

del config.h
copy ..\arch\win\config.in .\config.h
copy ..\arch\win\epos.dsp .
copy ..\arch\win\eposm.dsp .
copy ..\arch\win\say.dsp .
copy ..\arch\win\epos.dsw .

copy ..\arch\win\service\*.* .

if not exist client.cc echo #include "client.cpp" > client.cc

cd ..\cfg\cfg

if not exist ..\..\arch\unix\epos.ini move .\epos.ini ..\..\arch\unix\epos.ini
copy ..\..\arch\win\epos.ini .\epos.ini

cd ..\..\arch\win

