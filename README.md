pkd: a sample public-key testing daemon using libssh

Building pkd:
* `make` (assumes Linux; libz, libcrypto, pthreads available; assumes cmake + libssh dependencies)

Building pkd against a specific libssh version:
* `LIBSSH_TEST_SHA=15bede0c0e03b4065fc8a156f11f1d1d87f0e7e8 make`

Running pkd together with tests (will use an assumed OpenSSH installation for `ssh`):
* start pkd: `./bin/pkd  ssh-dss ./keys/openssh-dsa`
* in another terminal, run tests: `./tests/1k-openssh.sh ./keys/openssh-rsa`

Running PuTTY tests (uses locally-built PuTTY `plink`):
* `make dl/putty/plink` to clone and build putty from source
* start pkd as usual: `./bin/pkd  ssh-rsa ./keys/openssh-rsa`
* in another terminal, run tests: `./tests/1k-putty.sh ./keys/putty-rsa`

Running Dropbear tests (uses locally-built dropbear `dbclient`):
* `make dl/dropbear/dbclient` to clone and build dbclient from source
* start pkd as usual: './bin/pkd ssh-rsa ./keys/openssh-rsa`
* in another terminal, run tests: `./tests/
