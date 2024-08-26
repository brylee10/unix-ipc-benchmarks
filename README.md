# C++ IPC Benchmarks
This repository provides code which benchmarks some common POSIX IPC primitives using C++. This benchmarks:

```
- Message Queue
- Named Pipe (FIFOs)
- (Unnamed) Pipe
- Shared Memory
- Unix Domain Socket
```

Future work could involve benchmarking other primitives, such as Unix domain sockets or specific SPSC IPC implementations such as [Boost SPSC](https://www.boost.org/doc/libs/1_60_0/boost/lockfree/spsc_queue.hpp). In particular, it's possible that for larger message sizes (10K bytes), the Unix sockets may be more performant (in messages/sec) than, say, named pipes. 

This is inspired by prior work done by Goldsborough in [IPC bench](https://github.com/goldsborough/ipc-bench).

# Running
To run the code, clone the repository locally and then run the following from the root of the repo:

```shell
cmake .
make
```

After which, you can run individual tests via:

```shell
bin/launcher -m <message_size> -i <iterations> -n <benchmark name>
```

where `benchmark name` is any of: `message_queue`, `named_pipe`, `shm`, `unix_socket`. These benchmarks are all designed with a client/server architecture, which the launcher script is a wrapper for.

Benchmarks for the (unnamed) pipe (which does not have the client/server architecture) should be run via 

```shell
bin/pipe/pipe -m <message_size> -i <iterations>
```

# Results
The following displays the benchmark results of the different IPC methods where message size (bytes) is varied. Benchmarks involve a ping pong of a message between the client and server where one iteration is one complete ping pong. 

The results below display the number of messages sent per second across 10k iterations. The columns are different message sizes in bytes.

| IPC Method            | 64B Messages Sent/s | 128B Messages Sent/s | 1024B Messages Sent/s |
|-----------------------|--------------------:|---------------------:|----------------------:|
| Unix Domain Sockets   | 157,151             | 153,562              | 94,059                |
| Message Queue         | 207,172             | 173,801              | 49,418                |
| Named Pipe (FIFOs)    | 212,938             | 208,485              | 200,204               |
| Pipe                  | 304,432             | 287,397              | 259,470               |
| Shared Memory         | 5,359,056           | 4,672,897            | 2,452,182             |

The primary conclusion is shared memory significantly outperforms other IPC methods, especially for larger message sizes. Shared memory is fast because it allows multiple processes to access the same memory region directly without copying data. For other IPC methods, there is a copy from the writer message into the shared structure, and then from the IPC structure to the reader buffer. Additionally, since shared memory is not managed by the kernel and is directly `mmap`ed into the user address space, there is no system call and context switch. 

Regarding the other IPC methods, message queues have additional overhead due to kernel involvement for managing message structures, priority queues, and linked list traversal. However, the steep drop off in performance for larger message sizes was unexpected. This requires additional investigation. This could be related to the overhead of copying data between processes.

Between named pipes and unnamed pipes, prior work from [Goldsborough](https://github.com/goldsborough/ipc-bench) and by [Dato](https://www.baeldung.com/linux/ipc-performance-comparison) show that the named pipe slightly outperforms the pipe, but the above results show the reverse. Additional investigation would be needed to understand the discrepancy. 

# Code Structure

## Common Utilities
Common utilities and scaffolding code is located in `src/common` and IPC benchmark code is located in `src/(message_queue|named_pipe|pipe|shm)`.

`common/launcher.cc`: Benchmarks besides `pipe` have a client/server architecture (`pipe` instead uses `fork()` internally to create a child process as is canonical to copy the relevant file descriptors). These benchmarks are run via the `src/common/launcher.cc` which provides a common wrapper around the client/server API. The name of the IPC test is provided with the relevant message size and number of iterations and the correct client/server executable is selected. All executables are compiled in the `bin` folder, while static libraries for common utilities are compiled into `lib`. The individual client/server executables can be run via `bin/[ipc_type]/(client|server)` but it is easier to coordinate the two processes via the launcher (e.g. ensure the message size and iteration count is the same, and in particular the tests assume the server starts before the client for timing). 

`common/args.cc`: This contains helpers for argument parsing, used by the launcher and the individual client/server scripts.

`common/bench.cc`: Utilities for timing individual iterations and presenting summary statistics.

`common/signals.cc`: Instead of busy looping, the client/server tests use the two user defined signals (`SIGUSR1`/`SIGUSR2`) to indicate that a client is ready to receive messages. This is analogous to a condition variable with only one thread waiting.

`common/utils.cc`: Runtime assertions can be toggled via the `COMPILE_ASSERTS` macro.

## IPC Implementations
Implementations for each of the IPC methods are located in `src/[ipc_type]`. These generally follow a client/server architecture, except for the unnamed pipe. The below describes any notable files and functions. A `SignalManager` is only used to trigger the server to start the ping pong cycle after the client sends a signal indicating it is ready to receive messages. 

### Message Queue
`message_queue/queue_ops.cc`: Provides a wrapper around `msgctl` to delete, expand, or get overview info on a client or server message queue. 

### Shared Memory
`shm/shm.cc`: A `ShmManager` is used to provide wrapper functions which manage the shared memory segment, such as initialization, write, read, and clean up.

# IPC Notes
The below presents brief notes on the different IPC methods. For Unix sockets, pipes, named pipes, and message queues, these are all methods for IPC where the kernel abstracts the underlying the mechanism and data structures. All besides message queues are via file descriptors (message queues are identified via a System V IPC key created via `ftok`).

## Pipe
Pipes are a simple and widely used form of IPC that allow for unidirectional communication between processes using file descriptors. In this implementation, two unnamed pipes are used: one for server-to-client (s2c) communication and another for client-to-server (c2s) responses. The program forks into a parent (server) and child (client) process, using these pipes to "ping-pong" messages back and forth. The pipe has an internal buffer which is managed by the kernel and is typically 16KB to 64KB on MacOS.

The pipe interface relies on:
- **[`pipe`](https://man7.org/linux/man-pages/man2/pipe.2.html)**: creates a pair of file descriptors, one for reading and one for writing.
- **[`fork`](https://man7.org/linux/man-pages/man2/fork.2.html)**: creates the child process. Both processes inherit the pipe file descriptors.
- **[`read`, `write`](https://man7.org/linux/man-pages/man2/read.2.html)**: used for blocking reads and writes to transfer data.

## Named Pipes (FIFOs)
Unlike unnamed pipes created via the `pipe` system call, a named pipe is backed by a file so it can last as long as the system is up, beyond the life of the process. A named pipe is [bidirectional but half duplex](https://stackoverflow.com/a/9475519/16427614). With named pipes, you can also have multiple readers and writers which do not need to be `fork` from the same parent process.

The implementation follows these key steps:
- **[`mkfifo`](https://man7.org/linux/man-pages/man3/mkfifo.3.html)**: Creates a FIFO at a specified path. If the FIFO already exists, it is reused.
- **[`open`](https://man7.org/linux/man-pages/man2/open.2.html)**: Opens the FIFO for both reading and writing (`O_RDWR`), simplifying use in bidirectional communication.
- **[`read`, `write`](https://man7.org/linux/man-pages/man2/read.2.html)**: Used to read from and write to the FIFO. These operations block until the other end of the pipe is ready.

The FIFO is cleaned up with:
- **[`unlink`](https://man7.org/linux/man-pages/man2/unlink.2.html)**: Removes the FIFO from the filesystem when it is no longer needed. Since both client and server processes may attempt to unlink the FIFO, the implementation handles `ENOENT` errors gracefully.

This implementation demonstrates how to use named pipes for persistent and flexible IPC, suitable for scenarios where processes need to communicate across different execution contexts while remaining simple and lightweight.

## Message Queue
The System V message queue is tested, given that the newer POSIX message queue is [not implemented on MacOS](https://softwareengineeringexperiences.quora.com/Is-it-true-that-MacOS-does-not-fully-support-POSIX-message-queues-that-is-all-of-the-functionality-available-in-mqueue). The System V set of IPC primitives is alternatively called the XSI IPC, for which the relevant IPC functions are documented by the [Open Group Base Specifications](https://pubs.opengroup.org/onlinepubs/007904975/functions/xsh_chap02_07.html). This means newer POSIX functions like `mq_notify` for notification are not supported. Message queues are implemented as a linked list of messages held by the kernel. The linked list implementation allows the messages to be variable length. The queue sets limits on the message size, the number of messages, and the total size of all messages. The commands required to initialize a System V message queue are listed in the following Oracle docs under ["System V Messages"](https://docs.oracle.com/cd/E19455-01/806-4750/6jdqdfltg/index.html).

Message queues use the following API:
- [`ftok`](https://man7.org/linux/man-pages/man3/ftok.3.html): generate a unique System V IPC key based on a pathname and project ID
- [`msgctl`](https://pubs.opengroup.org/onlinepubs/007904975/functions/msgctl.html): change certain properties of a given message queue, such as to expand the capacity. Functions to resize and delete message queues are exposed via the `queue_ops` utility function in `bin/message_queue/queue_ops`
- [`msgget`, `msgrcv`, `msgsnd`](https://pubs.opengroup.org/onlinepubs/007904975/functions/xsh_chap02_07.html): the APIs to get a queue identifier, receive/send messages

## Shared Memory
The implementation uses POSIX shared memory functions which, unlike System V shared memory, allows the use of more modern APIs like `shm_open` and `mmap`. Shared memory minimizes kernel involvement once the memory is mapped, allowing processes to directly read and write to the shared region. In Linux, shared memory objects would be located in `/dev/shm` but in MacOS this path does not exist and the objects are not easily inspectable.

In this implementation, shared memory is managed through a `ShmManager` class, which handles memory setup, reading, writing, and cleanup operations. The shared memory is treated as a circular buffer to efficiently handle data streams.

The shared memory object is initialized using:
- **[`shm_open`](https://man7.org/linux/man-pages/man3/shm_open.3.html)**: Opens or creates a shared memory object identified by a name. The object is created with read and write permissions (`O_CREAT | O_RDWR`) and mode `0666`.
- **[`ftruncate`](https://man7.org/linux/man-pages/man2/ftruncate.2.html)**: Sets the size of the shared memory object. This call ensures that the memory region is large enough for the application’s needs.
- **[`mmap`](https://man7.org/linux/man-pages/man2/mmap.2.html)**: Maps the shared memory object into the process’s address space, allowing direct access to the memory region.

When the `ShmManager` is destroyed, it cleans up resources using:
- **[`munmap`](https://man7.org/linux/man-pages/man2/munmap.2.html)**: Unmaps the shared memory region.
- **[`shm_unlink`](https://man7.org/linux/man-pages/man3/shm_unlink.3.html)**: Unlinks the shared memory object, removing it from the system. 

## Unix Socket
A Unix domain socket allows bidirectional data exchange between two or more processes, analogous to an internet domain socket used for data exchange across machines (the Unix domain socket binds to a file path while the internet domain socket binds to an `IP Address:Port`).

The communication is handled via a client-server model:
- The server sets up a listening socket and waits for client connections.
- The client connects to the server using the specified Unix socket path.

Key operations involve:
- **[`socket`](https://man7.org/linux/man-pages/man2/socket.2.html)**: Creates the socket using the `AF_UNIX` address family.
- **[`bind`](https://man7.org/linux/man-pages/man2/bind.2.html)**: Binds the socket to a file path, allowing clients to locate the server.
- **[`listen`](https://man7.org/linux/man-pages/man2/listen.2.html)**: Puts the server socket into listening mode for incoming connections.
- **[`accept`](https://man7.org/linux/man-pages/man2/accept.2.html)**: Accepts client connections, creating a new socket for communication.
- **[`connect`](https://man7.org/linux/man-pages/man2/connect.2.html)**: The client connects to the server using the socket path.

Both client and server use **[`read`](https://man7.org/linux/man-pages/man2/read.2.html)** and **[`write`](https://man7.org/linux/man-pages/man2/write.2.html)** for data exchange. Once the connection is closed, the server removes the socket file using **[`unlink`](https://man7.org/linux/man-pages/man2/unlink.2.html)** to clean up.
