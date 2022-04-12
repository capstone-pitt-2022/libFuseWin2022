# libFuseWin2022

# ntapfuse

ntapfuse is a FUSE filesystem that mirrors a directory.

## Dependencies

The following packages need to be installed using

```console
  $ sudo apt-get install <pkg-name>

* fuse
* libfuse-dev
* sqlite3
* libsqlite3-dev
* python3
* python3-pip

Please ensure that the version of python installed is >= 3.6.
Further please ensure that you have installed the pytest package:

```console
  $ pip install pytest


ntapfuse requires a base directory to be mirroed and a mount point:

```console
  $ ntapfuse mount <base> <mount>

ntapfuse should be unmounted using the fusermount command:

```console
  $ fusermount -u <mount>

