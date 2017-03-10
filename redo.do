./redo-ifchange $2.o
c++ -std=c++14 $2.o -L /usr/local/opt/openssl/lib -lssl -lcrypto -o $3
