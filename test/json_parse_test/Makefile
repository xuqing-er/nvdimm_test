CC = gcc
JSONSRCPATH = /home/dsb/nvdimm_test/
CFLAGS = -I/home/dsb/nvdimm_test/
LDFLAGS = -L/usr/local/lib -lcjson 

json_test:
	$(CC) -o test $(CFLAGS) $(JSONSRCPATH)json_parse.c json_parse_test.c  $(LDFLAGS)

clean:
	rm test

.PHONY: json_test clean