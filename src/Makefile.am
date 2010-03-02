bin_PROGRAMS=iii-extract-riff-chunk
sbin_PROGRAMS=iiid
noinst_HEADERS = \
	eyefi.h \
	eyekinfig.h eyetil.h \
	eyefiworker.h

AM_CPPFLAGS = ${CPPFLAGS_DEBUG} \
	      -DEYEKIN_CONF_DIR=\"${sysconfdir}/${PACKAGE}\"
DEFAULT_INCLUDES = -I${top_builddir} -I${builddir} -I${srcdir}
INCLUDES = ${MODULES_CFLAGS} ${UUID_CFLAGS}

iiid_SOURCES = iiid.cc \
	eyekinfig.cc eyetil.cc \
	eyefiservice.cc eyefiworker.cc
nodist_iiid_SOURCES = \
	${builddir}/soapC.cpp ${builddir}/soapeyefiService.cpp \
	COPYING.cc
iiid_LDADD = ${MODULES_LIBS} ${UUID_LIBS}

iii_extract_riff_chunk_SOURCES = iii-extract-riff-chunk.cc
nodist_iii_extract_riff_chunk_SOURCES = COPYING.cc

COPYING.cc: ${top_srcdir}/COPYING
	echo "const char * COPYING = " >$@ || (rm $@;exit 1)
	sed -e 's/"/\\"/g' -e 's/^/\"/' -e 's/$$/\\n\"/' $< >>$@ || (rm $@;exit 1)
	echo ';' >>$@ || (rm $@;exit 1)

${srcdir}/eyefiservice.cc: ${builddir}/soapeyefiService.h
${srcdir}/iiid.cc: ${builddir}/eyefi.nsmap

${builddir}soapC.cpp ${builddir}/soapeyefiService.cpp ${builddir}/eyefi.nsmap ${builddir}/soapeyefiService.h: ${srcdir}/eyefi.h
	${SOAPCPP2} -d${builddir} -S -L -a -i -w -x $<

clean-local:
	rm -f soap{{H,Stub,eyefiService}.h,{C,eyefiService}.cpp} eyefi.nsmap COPYING.cc
