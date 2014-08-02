MemoryTracer
============

MemoryTracer is a PIN-based tool to analyze memory reads and writes by a program at run time.

[PIN](http://www.pintool.org) is a binary instrumentation framework that allows to embed given program codes into a running process. MemoryTracer uses PIN to instrument every memory access to record its address and size. The instrumentation is done at run time with the binary code of a specified process, allowing analysis of almost arbitrary programs.

## Installation

To install MemoryTracer, first install PIN, which can be downloaded at http://www.pintool.org. PIN has been developed and maintained by Intel for x86 and x86-64 processors. To install PIN, just unpack the binary distribution.

Suppose the installation directory of PIN is set at environment variable `$PIN_ROOT`. To compile the tool,

```
$ make PIN_ROOT=$PIN_ROOT obj-intel64/MemoryTracer.so
```

This will create a loadable library at `obj-intel64/MemoryTracer.so`. Replace `intel64` with `intel32` when using this tool on an Intel 32-bit processor. The library can then be used with the `pin` command as described below.


## Usage

MemoryTracer is used as a plugin tool for the `pin` command. To use `pin`, run:

```
$PIN_ROOT/pin -t TOOL-LIBRARY-PATH [TOOL-OPTIONS] -- <TARGET COMMAND LINE>
```

Here, `TOOL-LIBRARY-PATH` is the file path of the loadable shared library. The two dashes designates the following command line arguments represent the command line of an instrumented application. So, to apply MemoryTracer to command `ls`, just run:

```
$PIN_ROOT/pin -t MemoryTracer.so -- ls
```

### Help

To get the help message of MemoryTracer,

```
$PIN_ROOT/pin -t MemoryTracer.so -help -- ls
```

Note the target command line is required, though it's not used in this usage.

### Specify the function to analyze

By default, MemoryTracer analyzes only a function specified with option `-f`:

```
$PIN_ROOT/pin -t MemoryTracer.so -f FUNC_NAME -- TARGET
```

Note that `FUNC_NAME` needs to match function names in the binary program, which may not be necessarily the same as those in its source code. Use tools like `nm` to find the name of the interested function.

## Acknowledgment

This code is based on the sample PIN tools distributed as part of the PIN package.

## License

LGPL









