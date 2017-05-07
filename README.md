# MyOS
A Linux Kernal written in C and x86 from scratch. 

##createfs
    This program takes a flat source directory (i.e. no subdirectories
    in the source directory) and creates a filesystem image in the
    format.  Run it with no parameters to see
    usage.

##elfconvert
    This program takes a 32-bit ELF (Executable and Linking Format) file
    - the standard executable type on Linux - and converts it to the
    executable format.  The output filename is
    <exename>.converted.

##fsdir/
	This is the directory from which your filesystem image was created.
	It contains versions of cat, fish, grep, hello, ls, and shell, as
	well as the frame0.txt and frame1.txt files that fish needs to run.
	If you want to change files in your OS's filesystem, modify this
	directory and then run the "createfs" utility on it to create a new
	filesystem image.

##README
    This file.

##kernel/
    This is the directory that contains the source code for the 
    operating system. Read the INSTALL file in that directory for
    instructions on how to set up the bootloader to boot this OS.