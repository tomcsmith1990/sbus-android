# This is the Makefile for the demonstration dissertation
# written by Martin Richards
#
# Note that continuation lines require '\'
# and that TAB is used after ':' and before unix commands.

DISS = diss.tex refs.bib propbody.tex

"":
	make figures
	make diss.pdf
	make clean
	
help:
	@echo
	@echo "USAGE:"
	@echo
	@echo "make          display help information"
	@echo "make diss.ps  make a postscript version of the dissertation"
	@echo "make diss.pdf make a .pdf version of the dissertation"
	@echo "make all      construct proposal.dvi and diss.ps"
	@echo "make count    display an estimated word count"
	@echo "make clean    remove all remakeable files"
	@echo

prop:	proposal.dvi
	xdvi proposal.dvi

diss.ps:	$(DISS)
	latex diss
	bibtex diss
	latex diss
	bibtex diss
	latex diss
	bibtex diss
	dvips -Ppdf -G0 -t a4 -pp 0-200 -o diss.ps diss.dvi

diss.pdf:	diss.ps
	ps2pdf diss.ps
	
count:
	detex diss.tex | tr -cd '0-9A-Za-z \n' | wc -w

all:	proposal.dvi diss.ps

figures:
	pdfcrop -margins 10 figs/*.pdf
	pdftops -eps figs/*-crop.pdf
	rm figs/*-crop.pdf
	rename -f 's/\-crop.eps/.eps/' figs/*.eps

clean:
	rm -f diss.ps *.dvi *.aux *.log *.err
	rm -f core *~ *.lof *.toc *.blg *.bbl
	rm -f makefile.txt
