SRC=$(wildcard *.dot)
PNG=$(SRC:%.dot=%.png) 
dotname=$(SRC:%.dot=%) 

all: $(PNG) 
	@echo all pngs is generated.

%.png: %.dot
	dot -Tpng $< -o $@

c clean:
	rm $(PNG)
