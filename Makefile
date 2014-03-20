#
# pkd
#

TARGET = pkd
CFLAGS += -g -Wall -Werror
LDFLAGS +=
INCLUDES += -Idl/libssh/include
LIBS = dl/libssh/build/src/libssh.a \
       dl/libssh/build/src/threads/libssh_threads.a \
       -lcrypto \
       -lz \
       -lrt \
       -lpthread

bin/pkd: obj/pkd.o dl/libssh/build/src/libssh.a
	$(CC) $< $(LDFLAGS) $(LIBS) -o $@

obj/pkd.o: src/pkd.c dl/libssh/README dl/libssh/build/src/libssh.a
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


#
# libssh
#

# branch simonsj/v0-6-plus by default
LIBSSH_TEST_SHA ?= 1298e8751fd930f9fd551218817068427cb1c3f3

CMAKEFLAGS = \
  -DWITH_GSSAPI=OFF -DWITH_SFTP=OFF -DWITH_STATIC_LIB=ON \
  -DWITH_PCAP=OFF -DWITH_DEBUG_CALLTRACE=OFF -DWITH_EXAMPLES=OFF \
  -DCMAKE_BUILD_TYPE=Debug

dl/libssh/README:
	git clone http://github.com/simonsj/libssh.git dl/libssh

dl/libssh/build/src/libssh.a: dl/libssh/README Makefile
	@set -e; \
	cd dl/libssh; \
	if [ `git rev-parse HEAD` != "$(LIBSSH_TEST_SHA)" ]; then \
		git fetch origin; \
		git reset --hard $(LIBSSH_TEST_SHA); \
	fi; \
	mkdir -p build; \
	cd build && \
	cmake $(CMAKEFLAGS) .. && \
	make VERBOSE=1 -j8 && cd - && cd ..


#
# putty
#

dl/putty/README:
	git clone http://github.com/simonsj/putty.git dl/putty

# builds both putty/plink and putty/puttygen
dl/putty/plink: dl/putty/README Makefile
	@cd dl/putty; \
	if [ `git rev-parse HEAD` != "$(PUTTY_TEST_SHA)" ]; then \
		git fetch origin; \
		git reset --hard $(PUTTY_TEST_SHA); \
	fi; \
	./mkfiles.pl && ./mkauto.sh && ./configure && \
	make VERBOSE=1 CFLAGS="-Wno-sign-compare" -j8 puttygen plink && cd -
