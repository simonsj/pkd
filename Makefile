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
LIBSSH_TEST_SHA ?= 9c86571e1eab42c9a556c675700448169c6be217

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

PUTTY_TEST_SHA ?= 357f09924e87cf646f9b7caca9afe1cda3202e2a

dl/putty/README:
	git clone http://github.com/simonsj/putty.git dl/putty

# builds both putty/plink and putty/puttygen
dl/putty/plink: dl/putty/README Makefile
	@set -e; \
	cd dl/putty; \
	if [ `git rev-parse HEAD` != "$(PUTTY_TEST_SHA)" ]; then \
		git fetch origin; \
		git reset --hard $(PUTTY_TEST_SHA); \
	fi; \
	./mkfiles.pl && ./mkauto.sh && ./configure && \
	make VERBOSE=1 CFLAGS="-Wno-sign-compare" -j8 puttygen plink && cd -


#
# dropbear
#

DROPBEAR_TEST_SHA ?= 162fcab34736d18074158926bf937962ff5e2f7d

dl/dropbear/README:
	git clone http://github.com/mkj/dropbear.git dl/dropbear

dl/dropbear/dbclient: dl/dropbear/README
	@set -e; \
	cd dl/dropbear; \
	if [ `git rev-parse HEAD` != "$(DROPBEAR_TEST_SHA)" ]; then \
		git fetch origin; \
		git reset --hard $(DROPBEAR_TEST_SHA); \
	fi; \
	autoconf && autoheader && ./configure && \
	make -j8 dbclient && cd -
