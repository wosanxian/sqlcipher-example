SQLCIPHER_DIR:=/home/mancode/apps/sqlcipher/4.10.0
SQLCIPHER_INC:=-I${SQLCIPHER_DIR}/include
SQLCIPHER_LIB:=${SQLCIPHER_DIR}/lib/libsqlite3.a

OPENSSL_DIR:=/lib/x86_64-linux-gnu
OPENSSL_INC:=-I/usr/incude
OPENSSL_LIB:=${OPENSSL_DIR}/libcrypto.a ${OPENSSL_DIR}/libssl.a

all:atest

atest:atest.cpp
	g++ -DSQLITE_HAS_CODEC -o atest atest.cpp ${SQLCIPHER_INC} ${OPENSSL_INC} ${SQLCIPHER_LIB} ${OPENSSL_LIB}

clean:
	rm -rf atest