# eBPF example

## Dependencies

To use eBPF loadbalancing `Python3` is needed for eBPF compilation.

On Ubuntu:
```shell
$ sudo apt install linux-headers-$(uname -r) clang libbpf-dev linux-tools-$(uname -r)
```

On Centos (tested on `Centos 8`):
```shell
$ sudo yum install kernel-headers clang
$ sudo dnf --enablerepo=powertools install libbpf-devel
$ sudo dnf install bpftool
```

## Install
To build this directory, use the following options on `configure` script:
- `--with-ebpf-example` and `--with-linux=<linux_headers_path>`
```shell
$ ./configure --with-ebpf-example --with-linux=/usr/src/linux
```
