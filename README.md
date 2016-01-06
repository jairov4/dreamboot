UEFI Image Writer
=================

[![Build status](https://ci.appveyor.com/api/projects/status/9oarh2rflxfqdmf8?svg=true)](https://ci.appveyor.com/project/jairov4/dreamboot)

I wrote this application to enable easy image writing to a disk device.
I was in trouble because my Intel Atom based tablet (Teclast X98) was bricked. It was running to BIOS but I was unable to install any OS due to I loss my droidboot (android fastboot) partition, but I was still able to boot UEFI simple applications. I wrote this app to write a prepared image of first sectors of the disk containing the pre-created GPT partition table and partitions with contents including the droidboot image.

A simple UEFI application that can:
* be compiled on either on Windows or Linux (using Visual Studio 2015 or MinGW-w64).
* be compiled for either x86_32 or x86_64 UEFI targets
* be tested on the fly, through a [QEMU](http://www.qemu.org)+[OVMF](http://tianocore.github.io/ovmf/)
  UEFI virtual machine.

## Prerequisites

* [Visual Studio 2015](https://www.visualstudio.com/downloads/download-visual-studio-vs)
  or [MinGW-w64](http://mingw-w64.sourceforge.net/) (with msys, if using MinGW-w64 on Windows)
* [QEMU](http://www.qemu.org)
* git
* wget, unzip, if not using Visual Studio

## Sub-Module initialization

For convenience, the project relies on the gnu-efi library (but __not__ on
the gnu-efi compiler itself), so you need to initialize the git submodules:
```
git submodule init
git submodule update
```

## Compilation and testing

If using Visual Studio, just press `F5` to have the application compiled and
launched in the QEMU emulator.

If using MinGW-w64, issue the following from a command prompt:

`make qemu`

Note that in both cases, the debug process will download the current version of
the EDK2 UEFI firmware and run your application against it in the QEMU virtual
UEFI environment.  
In case the download fails, you can download the latest from:
http://tianocore.sourceforge.net/wiki/OVMF and extract the `OVMF.fd` as
`OVMF_x86_32.fd` or `OVMF_x86_64.fd`in the top directory.
