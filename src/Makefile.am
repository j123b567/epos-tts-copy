##	Process this file with automake run in the top directory to yield Makefile.in

include $(top_srcdir)/Makefile.common

SUBDIRS = nnet

bin_PROGRAMS = epos say eposm vrfy gauge

SRC_BASE = parser.cc unit.cc rule.cc \
text.cc encoding.cc interf.cc function.cc options.cc \
synth.cc waveform.cc voice.cc client.cc \
ktdsyn.cc ptdsyn.cc lpcsyn.cc tdpsyn.cc mbrsyn.cc tcpsyn.cc \
common.h defaults.h unit.h parser.h rule.h text.h encoding.h interf.h \
function.h options.h hash.h synth.h voice.h exc.h \
waveform.h ktdsyn.h ptdsyn.h lpcsyn.h tdpsyn.h mbrsyn.h tcpsyn.h \
slab.h rx.h client.h agent.h 

epos_SOURCES = daemon.cc agent.cc ttscp.cc nonblock.cc hashd.cc rx.c $(SRC_BASE)
eposm_SOURCES = monolith.cc hash.cc rx.c $(SRC_BASE)
say_SOURCES = say.cc
gauge_SOURCES = gauge.cc
vrfy_SOURCES = vrfy.cc

epos_LDADD = nnet/libeposnnet.la
eposm_LDADD = nnet/libeposnnet.la

EXTRA_DIST = options.lst hashtmpl.cc marker.cc block.cc exc.cc \
nnet.cc qnxipc.cc

