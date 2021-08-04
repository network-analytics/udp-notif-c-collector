# Compiling and installing UDP-notif collector

## Dependencies
This project uses autotools and gcc to compile and package the library.

On Ubuntu:
```shell
$ sudo apt-get install autoconf libtool make automake gcc pkg-config
```

On Centos (tested on `Centos 8`):
```shell
$ sudo yum install autoconf libtool make automake pkgconf
```

### Optional Dependencies
#### Using tcmalloc (Optional)
This project can use tcmalloc for memory management allowing better performance.

On Ubuntu:
```shell
$ sudo apt-get install libgoogle-perftools-dev
```

On Centos (tested on `Centos 8`):
```shell
$ sudo yum install gperftools gperftools-devel
```

## Installing
To install the library on a linux machine.
```shell
$ ./bootstrap
$ ./configure       # See ./configure --help for options
$ make
$ make install      # Usually needs sudo permissions
$ ./export.sh       # Optional: export LD_LIBRARY_PATH with /usr/local/lib in global variable to allow linking process
```

### Configure options
There are some custom `./configure` options : 
- `--with-examples`: compile examples directory. Not compiled by default.
- `--with-ebpf-example`: compile eBPF example. Not compiled by default. It check eBPF dependencies too.
- `--with-test`: compile testdirectory. Not compiled by default.
- `--with-pkgconfigdir=[/own_path/pkgconfig]`: overwrite pkgconfig directory to install .pc file [default: ${PREFIX}/lib/pkgconfig]
- `--enable-tcmalloc`: enable compilation with tcmalloc instead of native malloc. tcmalloc should be installed first.
- `--with-linux=[/own_path/linux/src]`: linux source code necesary for eBPF compilation [default: /usr/src/linux]. (On Ubuntu use /usr/src/<linux>-generic version)

## Uninstalling
```shell
$ make uninstall    # Usually need sudo permissions
```
You should remove the export of the lib in your bashrc manually yourself to fully remove the lib.
