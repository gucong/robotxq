EXTRA_DIST = cchess46.mf cchess46.tmf cchess.sty ccpieces.mf manual.tex

manual.pdf : cchess46.mf cchess46.tmf cchess.sty ccpieces.mf manual.tex
	xelatex manual.tex
	xelatex manual.tex

dist_pdf_DATA = manual.pdf