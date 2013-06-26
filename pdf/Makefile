

all: 
	make -C figures
	make pdf

t test:
	#pandoc test-sysvinit.md -o test-sysvinit.pdf --latex-engine=xelatex --toc --number-sections --columns=80
	pandoc test-sysvinit.md -o test-sysvinit.pdf --latex-engine=xelatex --toc --number-sections --columns=40
	gnome-open test-sysvinit.pdf

pdf:
	pandoc sysvinit.md -o sysvinit-new.tex --chapters --number-sections
	doconce subst '\\begin{verbatim}' '{\\begin{shaded}\\begin{verbatim}' sysvinit-new.tex
	doconce subst '\\end{verbatim}' '\\end{verbatim}\\end{shaded}}' sysvinit-new.tex
	doconce subst '\\begin{document}' '\\usepackage{framed,color}\n\\definecolor{shadecolor}{gray}{0.95}\n\\begin{document}' sysvinit-new.tex
	xelatex sysvinit-main.tex
	xelatex sysvinit-main.tex
	gnome-open sysvinit-main.pdf

c clean:
	rm *.html *.tex *.pdf *.aux *.log *.aux *.out *.old~
