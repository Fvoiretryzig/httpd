# Do *NOT* modify the existing build rules.
# You may add your own rules, e.g., "make run" or "make test".

LAB = httpd

include Makefile.git

.PHONY: build submit

build: $(LAB).c
	$(call git_commit, "compile")
	gcc httpd.c -g -w -lpthread -o httpd


submit:
	cd .. && tar cj $(LAB) > submission.tar.bz2
	curl -F "task=M7" -F "id=$(STUID)" -F "name=$(STUNAME)" -F "submission=@../submission.tar.bz2" 114.212.81.90:5000/upload
