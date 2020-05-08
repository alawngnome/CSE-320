# Homework 5 - CSE 320 - Spring 2020
#### Professor Eugene Stark

### **Due Date: Friday 5/8/2020 @ 11:59pm**

## Introduction

The goal of this assignment is to become familiar with low-level POSIX
threads, multi-threading safety, concurrency guarantees, and networking.
The overall objective is to implement a server that simulates the behavior
of a Private Branch Exchange (PBX) telephone system.
As you will probably find this somewhat difficult, to grease the way
I have provided you with a design for the server, as well as binary object
files for almost all the modules.  This means that you can build a
functioning server without initially facing too much complexity.
In each step of the assignment, you will replace one of my
binary modules with one built from your own source code.  If you succeed
in replacing all of my modules, you will have completed your own
version of the server.

For this assignment, there are three modules to work on:
  * Server initialization (`main.c`)
  * Server module (`server.c`)
  * PBX module (`pbx.c`)

It is probably best if you work on the modules in the order listed.
You should turn in whatever modules you have worked on.
Though the exact details are not set at this time, I expect that your
code will be compiled and tested in the following four configurations:
  1. Your `main.c` with my `server.o` and `pbx.o`.
  2. Your `main.c` and `server.c` with my `pbx.o`.
  3. Your `main.c`, `server.c` and `pbx.c`.
  4. Unit tests on your `pbx.c` in isolation.

If your code fails to compile in one or more of these configurations,
you will receive zero points for that configuration.
If your code compiles in a particular configuration, then you will receive
a point for your code having compiled and you will receive points for whatever
test cases for that configuration your program passes.

### Takeaways

After completing this homework, you should:

* Have a basic understanding of socket programming
* Understand thread execution, mutexes, and semaphores
* Have an understanding of POSIX threads
* Have some insight into the design of concurrent data structures
* Have enhanced your C programming abilities

## Hints and Tips

* We strongly recommend you check the return codes of all system
  calls. This will help you catch errors.

* **BEAT UP YOUR OWN CODE!** Throw lots of concurrent calls at your
  data structure libraries to ensure safety.

* Your code should **NEVER** crash. We will be deducting points for
  each time your program crashes during grading. Make sure your code
  handles invalid usage gracefully.

* You should make use of the macros in `debug.h`. You would never
  expect a library to print arbitrary statements as it could interfere
  with the program using the library. **FOLLOW THIS CONVENTION!**
  `make debug` is your friend.

> :scream: **DO NOT** modify any of the header files provided to you in the base code.
> These have to remain unmodified so that the modules can interoperate correctly.
> We will replace these header files with the original versions during grading.
> You are of course welcome to create your own header files that contain anything
> you wish.

## Helpful Resources

### Textbook Readings

You should make sure that you understand the material covered in
chapters **11.4** and **12** of **Computer Systems: A Programmer's
Perspective 3rd Edition** before starting this assignment.  These
chapters cover networking and concurrency in great detail and will be
an invaluable resource for this assignment.

### pthread Man Pages

The pthread man pages can be easily accessed through your terminal.
However, [this opengroup.org site](http://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html)
provides a list of all the available functions.  The same list is also
available for [semaphores](http://pubs.opengroup.org/onlinepubs/7908799/xsh/semaphore.h.html).

## Getting Started

Fetch and merge the base code for `hw5` as described in `hw0`. You can
find it at this link: https://gitlab02.cs.stonybrook.edu/cse320/hw5.
Remember to use the `--stategy-option=theirs` flag for the `git merge`
command to avoid merge conflicts in the Gitlab CI file.

### The Base Code

Here is the structure of the base code:

```
.
├── .gitlab-ci.yml
└── hw5
    ├── demo
    │   └── pbx
    ├── hw5.sublime-project
    ├── include
    │   ├── debug.h
    │   ├── pbx.h
    │   └── server.h
    ├── lib
    │   └── pbx.a
    ├── Makefile
    ├── src
    │   ├── globals.c
    │   └── main.c
    ├── tests
    │   └── .git_keep
    └── util
        └── tester.c
```

The base code consists of header files that define module interfaces,
a library `pbx.a` containing binary object code for my
implementations of the modules, a source code file `globals.c` that
contains definitions of some global variables, and a source code file `main.c`
that contains a stub for function `main()`.  The `Makefile` is
designed to compile any existing source code files and then link them
against the provided library.  The result is that any modules for
which you provide source code will be included in the final
executable, but modules for which no source code is provided will be
pulled in from the library.  The `pbx.a` library was compiled
without `-DDEBUG`, so it does not produce any debugging printout.
Also provided is a demonstration binary `demo/pbx`.  This executable
is a complete implementation of the PBX server, which is intended to help
you understand the specifications from a behavioral point of view.
It also does not produce any debugging printout, however.
The `util` directory contains the source code for an automated test client,
`tester`.  This is described in more detail below.

Most of the detailed specifications for the various modules and functions
that you are to implement are provided in the comments in the header
files in the `include` directory.  In the interests of brevity and avoiding
redundancy, those specifications are not reproduced in this document.
Nevertheless, the information they contain is very important, and constitutes
the authoritative specification of what you are to implement.

> :scream: The various functions and variables defined in the header files
> constitute the **entirety** of the interfaces between the modules in this program.
> Use these functions and variables as described and **do not** introduce any
> additional functions or global variables as "back door" communication paths
> between the modules.  If you do, the modules you implement will not interoperate
> properly with my implementations, and it will also likely negatively impact
> our ability to test your code.

## The PBX Server: Overview

"PBX" is a simple implementation of a server that simulates a PBX
telephone system.  A PBX is a private telephone exchange that is used
within a business or other organization to allow calls to be placed
between telephone units (TUs) attached to the system, without having to
route those calls over the public telephone network.  We will use the
familiar term "extension" to refer to one of the TUs attached to a PBX.

The PBX system provides the following basic capabilities:

  * *Register* a TU as an extension in the system.
  * *Unregister* a previously registered extension.

Once a TU has been registered, the following operations are available
to perform on it:

  * *Pick up* the handset of a registered TU.  If the TU was ringing,
then a connection is established with a calling TU.  If the TU was not
ringing, then the user hears a dial tone over the receiver.
  * *Hang up* the handset of a registered TU.  Any call in progress is
disconnected.
  * *Dial* an extension on a registered TU.  If the dialed extension is
currently "on hook" (*i.e.* the telephone handset is on the switchhook),
then the dialed extension starts to ring, indicating the presence of an
incoming call, and a "ring back" notification is played over the receiver
of the calling extension.  Otherwise, if the dialed extension is "off hook",
then a "busy signal" notification is played over the receiver of the
calling extension.
  * *Chat* over the connection made when one TU has dialed an extension
and the called extension has picked up.

The basic idea of these operations should be simple and familiar,
since I am sure that everyone has used a telephone system :wink:.
However, we will need a rather more detailed and complete specification
than just this simple overview.

## The PBX Server: Details

Our PBX system will be implemented as a multi-threaded network server.
When the server is started, a **master server** thread sets up a socket on
which to listen for connections from clients (*i.e.* the TUs).  When a network
connection is accepted, a **client service thread** is started to handle requests
sent by the client over that connection.  The client service thread registers
the client with the PBX system and is assigned an extension number.
The client service thread then executes a service loop in which it repeatedly
receives a **message** sent by the client, performs some operation determined
by the message, and sends one or more messages in response.
The server will also send messages to a client asynchronously as a result of actions
performed on behalf of other clients.
For example, if one client sends a "dial" message to dial another extension,
then if that extension is currently on-hook it will receive an asynchronous
"ring" message, indicating that the ringer is to be activated.

Messages from a client to a server represent commands to be performed.
Except for messages containing "chat" sent from a connected TU, every message
from the server to a client will consist of a notification of the current state
of that client, as it is currently understood by the server.
Usually these messages will inform the client of a state change that has occurred
as a result of a command the client sent, or of an asynchronous state change
that has occurred as a result of a command sent by some other client.
If a command sent by a client does not result in any state change, then the
response sent by the server will simply report the unchanged state.

> :nerd: One of the basic tenets of network programming is that a
> network connection can be broken at any time and the parties using
> such a connection must be able to handle this situation.  In the
> present context, the client's connection to the PBX server may
> be broken at any time, either as a result of explicit action by the
> client or for other reasons.  When disconnection of the client is
> noticed by the client service thread, the server acts as though the client
> had sent an explicit **hangup** command, the client is then unregistered from
> the PBX, and the client service thread terminates.

The PBX system maintains the set of registered clients in the form of a mapping
from assigned extension numbers to clients.
It also maintains, for each registered client, information about the current
state of the TU for that client.  The following are the possible states of a TU:

  * **On hook**: The TU handset is on the switchhook and the TU is idle.
  * **Ringing**: The TU handset is on the switchhook and the TU ringer is
active, indicating the presence of an incoming call.
  * **Dial tone**:  The TU handset is off the switchhook and a dial tone is
being played over the TU receiver.
  * **Ring back**:  The TU handset is off the switchhook and a "ring back"
signal is being played over the TU receiver.
  * **Busy signal**:  The TU handset is off the switchhook and a "busy"
signal is being played over the TU receiver.
  * **Connected**:  The TU handset is off the switchhook and a connection has been
established between this TU and the TU at another extension.  In this state
it is possible for users at the two TUs to "chat" with each other over the
connection.
  * **Error**:  The TU handset is off the switchhook and an "error" signal
is being played over the TU receiver.

Transitions between TU states occur in response to messages received from the
associated client, and sometimes also asynchronously in conjunction with
state transitions of other TUs.
The list below specifies all the possible transitions between states that a TU
can perform.  The arrival of any message other than those explicitly listed for
each particular state does not cause any transition between states to take place.

  * When in the **on hook** state:
    * A **pickup** message from the client will cause a transition to the **dial tone** state.
    * An asynchronous transition to the **ringing** state is also possible from this state.

  * When in the **ringing** state:
    * A **pickup** message from the client will cause a transition to the **connected** state.
      Simultaneously, the called TU will also make an asynchronous transition from the
      **ringing** state to the **connected** state.  The PBX will establish a connection
      between these two TUs, which we will refer to as *peers* as long as the connection
      remains established.
    * An asynchronous transition to the **on hook** state is also possible from this state.

  * When in the **dial tone** state:
    * A **hangup** message from the client will cause a transition to the **on hook** state.
    * A **dial** message from the client will cause a transition to either the **error**,
      **busy signal**, or **ring back** states, depending firstly on whether or not a TU
      is currently registered at the dialed extension, and secondly, whether the TU at
      the dialed extension is currently in the **on hook** state or in some other state.
      If there is no TU registered at the dialed extension, the transition will be to the
      **error** state.
      If there is a TU registered at the dialed extension, then if that extension is
      currently not in the **on hook** state, then the transition will be to the
      **busy signal** state, otherwise the transition will be to the **ring back** state.
      In the latter case, the TU at the dialed extension makes an asynchronous transition
      to the **ringing** state, simultaneously with the transition of the calling TU to the
      **ring back** state.

  * When in the **ring back** state:
    * A **hangup** message from the client will cause a transition to the **on hook** state.
      Simultaneously, the called TU will make an asynchronous transition from the **ringing**
	  state to the **on hook** state.
    * An asynchronous transition to the **connected** state (if the called TU picks up)
      or to the **dial tone** state (if the called TU unregisters) is also possible from this state.

  * When in the **busy signal** state:
    * A **hangup** message from the client will cause a transition to the **on hook** state.

  * When in the **connected** state:
    * A **hangup** message from the client will cause a transition to the **on hook** state.
      Simultaneously, the peer TU will make a transition from the **connected** state to
      the **dial tone** state.
    * An asynchronous transition to the **dial tone** state is possible from this state.

  * When in the **error** state:
    * A **hangup** message from the client will cause a transition to the **on hook** state.

Messages are sent between the client and server in a text-based format,
in which each message consists of a single line of text, the end of which is
indicated by the two-byte line termination sequence `"\r\n"`.
There is no *a priori* limitation on the length of the line of text that is sent
in a message.
The possible messages that can be sent are listed below.
The initial keywords in each message are case-sensitive.

  * Commands from Client to Server
    * **pickup**
    * **hangup**
    * **dial** #, where # is the number of the extension to be dialed.
    *  **chat** ...arbitrary text...

  * Responses from Server to Client
    * **ON HOOK** #, where # reports the extension number of the client.
    * **RINGING**
    * **DIAL TONE**
    * **RING BACK**
    * **CONNECTED** #, where # is the number of the extension to which the
      connection exists.
    * **ERROR**
    * **CHAT** ...arbitrary text...

### Demonstration Server

To help you understand what the PBX server is supposed to do, I have provided
a complete implementation for demonstration purposes.
Run it by typing the following command:

```
$ demo/pbx -p 3333
```

You may replace `3333` by any port number 1024 or above (port numbers below 1024
are generally reserved for use as "well-known" ports for particular services,
and require "root" privilege to be used).
The server should report that it has been initialized and is listening
on the specified port.
From another terminal window, use `telnet` to connect to the server as follows:

```
$ telnet localhost 3333
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
ON HOOK 4
```

You can now issue commands to the server:

```
pickup
DIAL TONE
dial 5
ERROR
hangup
ON HOOK 4
```

If you make a second connection to the server from yet another terminal window,
you can make calls.

## Task I: Server Initialization

When the base code is compiled and run, it will print out a message
saying that the server will not function until `main()` is
implemented.  This is your first task.  The `main()` function will
need to do the following things:

- Obtain the port number to be used by the server from the command-line
  arguments.  The port number is to be supplied by the required option
  `-p <port>`.
  
- Install a `SIGHUP` handler so that clean termination of the server can
  be achieved by sending it a `SIGHUP`.  Note that you need to use
  `sigaction()` rather than `signal()`, as the behavior of the latter is
  not well-defined in a multithreaded context.

- Set up the server socket and enter a loop to accept connections
  on this socket.  For each connection, a thread should be started to
  run function `pbx_client_service()`.

These things should be relatively straightforward to accomplish, given the
information presented in class and in the textbook.  If you do them properly,
the server should function and accept connections on the specified port,
and you should be able to connect to the server using the test client.

## Task II: Server Module

In this part of the assignment, you are to implement the server module,
which provides the function `pbx_client_service()` that is invoked
when a client connects to the server.  You should implement this function
in the `src/server.c` file.

The `pbx_client_service` function is invoked as the **thread function**
for a thread that is created (using ``pthread_create()``) to service a
client connection.
The argument is a pointer to the integer file descriptor to be used
to communicate with the client.  Once this file descriptor has been
retrieved, the storage it occupied needs to be freed.
The thread must then become detached, so that it does not have to be
explicitly reaped, and it must register the client file descriptor with
the PBX module.
Finally, the thread should enter a service loop in which it repeatedly
receives a message sent by the client, parses the message, and carries
out the specified command.
The actual work involved in carrying out the command is performed by calling
the functions provided by the PBX module.
These functions will also send the required response back to the client,
so the server module need not be directly concerned with that.

## Task III: PBX Module

The PBX module is the central module in the implementation of the server.
It provides the functions listed below, for which more detailed specifications
are given in the header file `pbx.h`.  You should implement these functions
in the `src/pbx.c` file.

  * `PBX *pbx_init()`:  Initialize a new PBX.
  * `pbx_shutdown(PBX *pbx)`:  Shut down a PBX.
  * `TU *pbx_register(PBX *pbx, int fd)`:  Register a TU client with a PBX.
  * `int pbx_unregister(PBX *pbx, TU *tu)`:  Unregister a TU from a PBX.
  * `int tu_fileno(TU *tu)`: Get the file descriptor for the network connection underlying a TU.
  * `int tu_extension(TU *tu)`: Get the extension number for a TU.
  * `int tu_pickup(TU *tu)`: Take a TU receiver off-hook (i.e. pick up the handset).
  * `int tu_hangup(TU *tu)`: Hang up a TU (i.e. replace the handset on the switchhook).
  * `int tu_dial(TU *tu, int ext)`: Dial an extension on a TU.
  * `int tu_chat(TU *tu, char *msg)`: "Chat" over a connection.

The PBX module will need to maintain a registry of connected clients and
manage the TU objects associated with these clients.
The PBX will need to be able to map each extension number to the associated TU object.
It will need to maintain, for each TU, the file descriptor of the underlying network
connection and the current state of the TU.
It will need to use the file descriptor to issue a response to the client,
as well as any required asynchronous notifications, whenever one of the `tu_xxx` functions
is called.
Finally, as the PBX and TU objects will be accessed concurrently by multiple threads,
the PBX module will need to provide appropriate synchronization (for example, using
mutexes and/or semaphores) to ensure correct and reliable operation.
Changes to the state of a TU will require exclusive access to the TU.
Some operations require simultaneous changes of state of two TUs; these will require
exclusive access to both TUs at the same time in order to ensure the simultaneity of
the state changes.
Sending responses to the client underlying a TU will require exclusive access to the
client file descriptor, in order to ensure that messages sent by separate threads
are serialized over the network connection, rather than intermingled.
Finally, the `pbx_shutdown()` function is required to shut down the network connections
to all registered clients (the `shutdown(2)` function can be used to shut down a socket
for reading, writing, or both, without closing the associated file descriptor)
and it is then required to wait for all the client service threads to unregister
the associated TUs before returning.  Consider using a semaphore, possibly in conjunction
with additional bookkeeping variables, for this purpose.

## Test Exerciser

I have provided code for a test exerciser that you can use to help debug and test
your implementation.  The code for it is in `util/tester.c`.
You can build it using `make util/tester` and then run it as `util/tester`.
It accepts the following command-line arguments:

  * `-h <hostname>`
    Specify the hostname to use when connecting to the server.  The default is `localhost`.
  * `-p <port #>`
    Specify the port number to use when connecting to the server.  The default is `3333`.
  * `-l <length of test>`
    Specify the length of the test, in terms of the number of commands to be sent to the server.
    The default is `0`, which means no limit.
  * `-x <min extension #>`
    Specify the minimum extension number that will be dialed during a test.  The default is `4`.
  * `-y <max extension #>`
    Specify the maximum extension number that will be dialed during a test.  The default is `5`.
  * `-d <microseconds>`
    Specify the basic delay time in microseconds.  The default is `100000` (0.1 seconds).
    Setting a smaller value will result in a more rapidly executing test.
    Setting a larger value will slow down the test.

The tester contains a table that determines the probabilities of the various actions to
be taken in each possible state, as well as a table used to check whether a particular
state transition is valid for the current state.  You can read the code to find out more
and you may make changes if you like.

## Submission Instructions

Make sure your hw5 directory looks similarly to the way it did
initially and that your homework compiles (be sure to try compiling
both with and without "debug").
Note that you should omit any source files for modules that you did not
complete, and that you might have some source and header files in addition
to those shown.  You are also, of course, encouraged to create Criterion
tests for your code.

It would definitely be a good idea to use `valgrind` to check your program
for memory and file descriptor leaks.  Keeping track of allocated objects
and making sure to free them is potentially one of the more challenging aspects
of this assignment.

To submit, run `git submit hw5`.
