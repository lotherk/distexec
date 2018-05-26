# Distexec

## build requirements

### linux

#### debian
```
$ apt-get install gcc make cmake gengetopt libssh2-dev libpcre-dev libjson-c-dev libcurl4-openssl-dev
```

### mac os

Install gengetopt and json-c via brew/macports

```
brew install gengetopt json-c pcre libssh2 
```
or
```
TODO
```

Install cmake from cmake.org (or via brew/macports)


### windows

Install gengetopt http://gnuwin32.sourceforge.net/packages/gengetopt.htm
(Add install path/bin to PATH environment variable)

Install chocolatey https://chocolatey.org

Run in cmd/powershell:

```
choco install cmake mingw curl

```

Check your PATH is setup correctly...

## build

```
git clone https://git.hiddenbox.org/lotherk/c-distexec
cd c-distexec
mkdir build
cd build
cmake ..
make
./distexec -h
```
