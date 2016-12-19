# FUSE
 FUSE is an in-memory filesystem.

## FUSE
Modern operating systems support multiple filesystems. The operating system directs each filesystem operation to the appropriate implementation of the routine. It multiplexes the filesystem calls across many distinct implementations. For example, on a read system call the operation system uses NFS code if it is an NFS file but uses ext3 code if it is an ext3 file.

FUSE (Filesystem in Userspace) is an interface that exports filesystem operations to user-space. Thus filesystem routines are executed in user-space. Continuing the example, if the file is a FUSE file then operating system upcalls into user space in order to invoke the code associated with read.
## RAMDISK

Created an in-memory filesystem. Instead of reading and writing disk blocks, the RAMDISK filesystem is using main memory for storage. (Note: When the main memory is over allocated, the operating system will page some memory out to disk. Here, we still consider that in-memory.)

The RAMDISK filesystem will support the basic POSIX/Unix commands listed below. Externally, the RAMDISK appears as a standard Unix FS. Notably, it is hierarchtical (has directories) and must support standard accesses, such as read, write, and append. However, the filesystem is not persistent. The data and metadata are lost when the process terminates, which is also when the process frees the memory it has allocated.

The internal design of the filesystem is using linked list.

RAMDISK should not write any data to disk.

## Invocation

Clone or Download the repositry, Run the Makefile:
```
Make
```
Makefile is designed in such a way that it will create the sufficient files required to make the program work. Now, Run the program using:
```
ramdisk /path/to/dir 512 
```
