CC = gcc
JSONSRCPATH = /home/dsb/nvdimm_test/
CFLAGS = -I/home/dsb/nvdimm_test/
LDFLAGS = -L/usr/local/lib -lcjson -lpmem

lat_test:
	$(CC) -march=native -o latency_test $(CFLAGS) $(JSONSRCPATH)json_parse.c fsdax_access_latency_test.c  $(LDFLAGS)

clean:
	rm latency_test

.PHONY: lat_test clean