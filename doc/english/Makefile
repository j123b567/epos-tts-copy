
CONTENTS = intro.sgml rules.sgml options.sgml ttscp.sgml devel.sgml 

.PHONY: clean realclean dist

all: epos.txt

epos.sgml: header.sgml $(CONTENTS) footer.sgml
	@echo Updating epos.sgml
	@cat $^ | sed							\
		-e "s/CURRENT_EPOS_VERSION/`../../arch/unix/makever`/g"	\
		-e "s/CURRENT_EPOS_DATE/`../../arch/unix/makenicedate`/g"	\
		-f ../../arch/unix/doctwk.sed   > $@

epos.tex: epos.sgml
	@sgml2latex $^ --output=tex

epostwk.sgml: epos.sgml
	@sed '/^<sect/s:</\?tt>::g' <$^ >$@

epos.txt: epostwk.sgml
	@sgml2txt $^
	@mv epostwk.txt epos.txt

epos.html: epos.sgml
	@echo Converting to HTML
	@sgml2html $^

epos.dvi: epos.sgml
	@sgml2latex --output=dvi --papersize=a4 --language=en $^

epos.ps: epos.sgml
	@sgml2latex --output=ps --papersize=a4 --language=en $^

epos.info: epos.sgml
	@sgml2info $^

#/usr/lib/texmf/source/latex/misc/linuxdoc-sgml.sty: /usr/lib/sgml-tools/linuxdoc-sgml.sty
#	@ln -s /usr/lib/sgml-tools/linuxdoc-sgml.sty $@

clean:
	@rm -rf *~ *.bak *.log epostwk.* epos.sgml epos.tex

realclean:
	@rm -rf *~ *.bak epos* *.log /tmp/sgml2* /tmp/sgmltmp*

dist: realclean epos.html clean

header.sgml: ../../src/common.h
	@ echo Source code version may have changed, documentation adjusted.
	@ touch header.sgml
