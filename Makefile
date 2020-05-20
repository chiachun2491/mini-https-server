all: Server

CC = gcc
CFLAG = -lcrypto -lssl 

Server: server.c 
	$(CC) -o Server server.c mySSLTool.c myFileTool.c $(CFLAG)

.PHONY: clean,run

clean:
	rm Server

cleancrt:
	rm ./key/*.key ./key/*.crt ./key/*.srl

cleanvol:
	rm -r storage

cleanall:
	make clean
	make cleancrt
	make cleanvol

runserver:
	./Server

generateStorage:
	mkdir storage
	cp *.c *.h *.md storage

generateCA:
	openssl genrsa -des3 -out ./key/ca.key 2048
	openssl req -x509 -new -nodes -key ./key/ca.key -sha256 -days 1024 -out ./key/ca.crt

generateHost:
	openssl req -new -sha256 -nodes -out ./key/host.csr -newkey rsa:2048 -keyout ./key/host.key -config ./key/host.conf
	openssl x509 -req -in ./key/host.csr -CA ./key/ca.crt -CAkey ./key/ca.key -CAcreateserial -out ./key/host.crt -days 500 -sha256 -extfile ./key/host.ext
	rm ./key/host.csr

init:
	make generateStorage
	make generateCA
	make generateHost
	make