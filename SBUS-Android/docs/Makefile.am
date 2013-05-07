doc_DATA = overview.pdf datarepr.pdf datasample.pdf prebuilt_cpts.pdf \
	      extensions.pdf java.pdf python.pdf protocol.pdf api.pdf \
	      metadata.pdf dynamics.pdf userguide.pdf hack.pdf c.pdf

.tex.pdf:
	latex $<
	dvips -t a4 -o $*.ps $*.dvi
	ps2pdf $*.ps
