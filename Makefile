
all:
	@echo make s to ./s.sh sysvinit-2.88dsf
	@echo make b to make -C sysvinit-2.88dsf
	@echo make p to make -C pdf kmod
	@echo make f to make -C pdf/figures 
	@echo make d to execute ../doxygen
	@echo make c to git commit

.PHONY: pdf
p pdf: 
	make -C pdf 

t:
	make -C pdf t

o open:
	gedit pdf/sysvinit.md &
	gedit pdf/test-sysvinit.md &
	gnome-open  ../ds/html/index.html

b build:
	make -C sysvinit-2.88dsf

s subs:
	./s.sh sysvinit-2.88dsf
	cp sysvinit-2.88dsf-subs ../doxygen/ -R

f figure:
	make -C pdf/figures

d doxy:
	cd ../doxygen && doxygen

c commit: 
	git add .
	git commit -a -m "M sysvinit.md"
	git push

