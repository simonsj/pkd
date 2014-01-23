pkd: a sample public-key testing daemon using libssh

Building pkd:
* `make` (assumes Linux; libz, libcrypto, pthreads available)

Build pkd against a specific libssh version:
* `LIBSSH_TEST_SHA=15bede0c0e03b4065fc8a156f11f1d1d87f0e7e8 make`

Running pkd together with tests:
* start pkd: `./bin/pkd  ssh-dss ./keys/openssh-dsa`
* in another terminal, run tests: `./tests/1k-openssh.sh ./keys/openssh-rsa`
