# Homework 1 - CSE 320 - Spring 2020
#### Professor Eugene Stark

### **Due Date: Friday 02/21/2020 @ 11:59pm**

**Read the entire doc before you start**

## Introduction

In this assignment, you will write a command line utility to perform
decompression of data that has been compressed into a set of rules using
a data compression algorithm called *Sequitur*, and you will also implement
some data structures to support the core Sequitur algorithm (for which I
will provide an implementation), so that in the end you will have constructed
a utility that can perform both compression and decompression.
The goal of this homework is to familiarize yourself with C programming,
with a focus on input/output, bitwise manipulations, and the use of pointers.

For all assignments in this course, you **MUST NOT** put any of the functions
that you write into the `main.c` file.  The file `main.c` **MUST ONLY** contain
`#include`s, local `#define`s and the `main` function (you may of course modify
the `main` function body).  The reason for this restriction has to do with our
use of the Criterion library to test your code.
Beyond this, you may have as many or as few additional `.c` files in the `src`
directory as you wish.  Also, you may declare as many or as few headers as you wish.
Note, however, that header and `.c` files distributed with the assignment base code
often contain a comment at the beginning which states that they are not to be
modified.  **PLEASE** take note of these comments and do not modify any such files,
as they will be replaced by the original versions during grading.

> :scream: Array indexing (**'A[]'**) is not allowed in this assignment. You
> **MUST USE** pointer arithmetic instead. All necessary arrays are declared in
> the `const.h` header file. You **MUST USE** these arrays. **DO NOT** create
> your own arrays. We **WILL** check for this.

> :nerd: Reference for pointers: [https://beej.us/guide/bgc/html/multi/pointers.html](https://beej.us/guide/bgc/html/multi/pointers.html).

# Getting Started

Fetch base code for `hw1` as described in `hw0`. You can find it at this link:
[https://gitlab02.cs.stonybrook.edu/cse320/hw1](https://gitlab02.cs.stonybrook.edu/cse320/hw1).

Both repos will probably have a file named `.gitlab-ci.yml` with different contents.
Simply merging these files will cause a merge conflict. To avoid this, we will
merge the repos using a flag so that the `.gitlab-ci.yml` found in the `hw1`
repo will replace the `hw0` version.  To merge, use this command:

```
git merge -m "Merging HW1_CODE" HW1_CODE/master --strategy-option=theirs
```

> :scream: Based on past experience, many students will either ignore the above command or forget
> to use it.  The result will be a **merge conflict**, which will be reported by git.
> Once a merge conflict has been reported, it is essential to correct it before committing
> (or to abort the merge without committing -- use `git merge --abort` and go back and try again),
> because git will have inserted markers into the files involved indicating the locations of the
> conflicts, and if you ignore this and commit anyway, you will end up with corrupted files.
> You should consider it important to read up at an early stage on merge conflicts with git and
> how to resolve them properly.

Here is the structure of the base code:

<pre>
.
├── .gitlab-ci.yml
└── hw1
    ├── hw1.sublime-project
    ├── include
    │   ├── const.h
    │   ├── debug.h
    │   └── sequitur.h
    ├── Makefile
    ├── rsrc
    │   ├── sequitur.c.seq
    │   ├── sheet.txt
    │   ├── sheet.txt.seq
    │   ├── twelve_days.txt
    │   └── twelve_days.txt.seq
    ├── src
    │   ├── comdec.c
    │   ├── digram_hash.c
    │   ├── main.c
    │   ├── rules.c
    │   ├── sequitur.c
    │   └── symbol.c
    └── tests
        └── hw1_tests.c
</pre>

- The `.gitlab-ci.yml` file is a file that specifies "continuous integration" testing
to be performed by the GitLab server each time you push a commit.  Usually it will
be configured to check that your code builds and runs, and that any provided unit tests
are passed.  You are free to change this file if you like.

> :scream:  The CI testing is for your own information; it does not directly have
> anything to do with assignment grading or whether your commit has been properly
> pushed to the server.  If some part of the testing fails, you will see the somewhat
> misleading message "commit failed" on the GitLab web interface.
> This does not mean that "your attempt to commit has failed" or that "your commit
> didn't get pushed to the server"; the very fact that the testing was triggered at
> all means that you successfully pushed a commit.  Rather, it means that the CI tests
> performed on a commit that you pushed did not succeed".  The purpose of the tests are
> to alert you to possible problems with your code; if you see that testing has failed
> it is worth investigating why that has happened.  However, the tests can sometimes
> fail for reasons that are not your fault; for example, the entire CI "runner" system
> may fail if someone submits code that fills up the system disk.  You should definitely
> try to understand why the tests have failed if they do, but it is not necessary to be
> overly obsessive about them.

- The `hw1.sublime-project` file is a "project file" for use by the Sublime Text editor.
It is included to try to help Sublime understand the organization of the project so that
it can properly identify errors as you edit your code.

- The `Makefile` is a configuration file for the `make` build utility, which is what
you should use to compile your code.  In brief, `make` or `make all` will compile
anything that needs to be, `make debug` does the same except that it compiles the code
with options suitable for debugging, and `make clean` removes files that resulted from
a previous compilation.  These "targets" can be combined; for example, you would use
`make clean debug` to ensure a complete clean and rebuild of everything for debugging.

- The `include` directory contains C header files (with extension `.h`) that are used
by the code.  Note that often (but not always) these files contain `DO NOT MODIFY`
instructions at the beginning.  You should observe these notices carefully where
they appear.

- The `src` directory contains C source files (with extension `.c`).

- The `tests` directory contains C source code (and sometimes headers and other files)
that are used by the Criterion tests.

- The `rsrc` directory contains some samples of compressed and uncompressed files that
you can use for testing purposes.

## A Note about Program Output

What a program does and does not print is VERY important.
In the UNIX world stringing together programs with piping and scripting is
commonplace. Although combining programs in this way is extremely powerful, it
means that each program must not print extraneous output. For example, you would
expect `ls` to output a list of files in a directory and nothing else.
Similarly, your program must follow the specifications for normal operation.
One part of our grading of this assignment will be to check whether your program
produces EXACTLY the specified output.  If your program produces output that deviates
from the specifications, even in a minor way, or if it produces extraneous output
that was not part of the specifications, it will adversely impact your grade
in a significant way, so pay close attention.

**Use the debug macro `debug` (described in the 320 reference document in the
Piazza resources section) for any other program output or messages you many need
while coding (e.g. debugging output).**

# Part 1: Program Operation and Argument Validation

In this part, you will write a function to validate the arguments passed to your
program via the command line. Your program will treat arguments as follows:

- If no flags are provided, you will display the usage and return with an
`EXIT_FAILURE` return code

- If the `-h` flag is provided, you will display the usage for the program and
  exit with an `EXIT_SUCCESS` return code.

- If the `-c` flag is provided, you will perform data compression; reading
  uncompressed data from `stdin` and writing compressed data to `stdout`,
  exiting with `EXIT_SUCCESS` on success and `EXIT_FAILURE` on any error.

- If the `-d` flag is provided, you will perform decompression; reading
  compressed data from `stdin` and writing uncompressed data to `stdout`,
  exiting with `EXIT_SUCCESS` on success and `EXIT_FAILURE` on any error.

> :nerd: `EXIT_SUCCESS` and `EXIT_FAILURE` are macros defined in `<stdlib.h>` which
> represent success and failure return codes respectively.

> :nerd: `stdin`, `stdout`, and `stderr` are special I/O "streams", defined
> in `<stdio.h>`, which are automatically opened at the start of execution
> for all programs and do not need to be reopened.

Some of these operations will also need other command line arguments which are
described in each part of the assignment.  The usage scenarios for this program are
described by the following message, which is printed by the program when it is invoked
without any arguments:

<pre>
Usage: bin/sequitur -h [any other number or type of arguments]
    -h       Help: displays this help menu
    -c       Compress: read raw data from standard input, output compressed data to standard output.
    -d       Decompress: read compressed data from standard input, output raw data to standard output.
             Optional additional parameter for -c (not permitted with -d):
                -b           BLOCKSIZE is the blocksize (in Kbytes, range [1, 1024])
                             to be used in compression.
</pre>

The `-c|-d` means that one or the other of `-c` or `-d` may be specified.
The `[-b BLOCKSIZE]` means that `-b` may be optionally specified, in which
case it is immediately followed by a parameter `BLOCKSIZE`.

A valid invocation of the program implies that the following hold about
the command-line arguments:

- All "positional arguments" (`-h|-d|-c`) come before any optional arguments (`-b`).
The optional arguments (in this case `-b` is the only optional argument) may come
in any order after the positional ones.

- If the `-h` flag is provided, it is the first positional argument after
the program name and any other arguments that follow are ignored.

- If the `-h` flag is *not* specified, then exactly one of `-c` or `-d`
must be specified.

- If an option requires a parameter, the corresponding parameter must be provided
(*e.g.* `-b` must always be followed by a BLOCKSIZE specification).

    - If `-b` is given, the BLOCKSIZE argument will be given as a decimal integer in
    the range [1, 1024].

For example, the following are a subset of the possible valid argument
combinations:

- `$ bin/sequitur -h ...`
- `$ bin/sequitur -c -b 10`
- `$ bin/sequitur -d`

> :scream: The `...` means that all arguments, if any, are to be ignored; e.g.
> the usage `bin/sequitur -h -x -y BLAHBLAHBLAH -z` is equivalent to `bin/sequitur -h`.

Some examples of invalid combinations would be:

- `$ bin/sequitur -c -d -b 1024`
- `$ bin/sequitur -b 1024 -c`
- `$ bin/sequitur -d -b 1024`
- `$ bin/sequitur -c -b 1k`

> :scream: You may use only "raw" `argc` and `argv` for argument parsing and
> validation. Using any libraries that parse command line arguments (e.g.
> `getopt`) is prohibited.

> :scream: Any libraries that help you parse strings are prohibited as well
> (`string.h`, `ctype.h`, etc). *This is intentional and will help you
> practice parsing strings and manipulating pointers.*

> :scream: You **MAY NOT** use dynamic memory allocation in this assignment
> (i.e. `malloc`, `realloc`, `calloc`, `mmap`, etc.).

> :nerd: Reference for command line arguments: [https://beej.us/guide/bgc/html/multi/morestuff.html#clargs](https://beej.us/guide/bgc/html/multi/morestuff.html#clargs).

**NOTE:** The `make` command compiles the `sequitur` executable into the `bin` folder.
All commands from here on are assumed to be run from the `hw1` directory.

### **Required** Validate Arguments Function

In `const.h`, you will find the following function prototype (function
declaration) already declared for you. You **MUST** implement this function
as part of the assignment.

```c
int validargs(int argc, char **argv);
```

The file `comdec.c` contains the following specification of the required behavior
of this function:

```c
/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv);
```

> :scream: This function must be implemented as specified as it will be tested
> and graded independently. **It should always return -- the USAGE macro should
> never be called from validargs.**

The `validargs` function should return -1 if there is any form of failure.
This includes, but is not limited to:

- Invalid number of arguments (too few or too many)

- Invalid ordering of arguments

- A missing parameter to an option that requires one (e.g. `-b` with no
  `BLOCKSIZE` specification).

- Invalid `BLOCKSIZE` (if one is specified).  A `BLOCKSIZE` is invalid if it
contains characters other than the digits ('0'-'9'), or if it denotes a
value not in the range [1, 1024].

The `global_options` variable of type `int` is used to record the mode
of operation (i.e. help/compress/decompress) of the program, as well as
the blocksize.  This is done as follows:

- If the `-h` flag is specified, the least significant bit (bit 0) is 1.

- The second-least-significant bit (bit 1) is 1 if `-c` is passed
(i.e. the user wants compression mode).

- The third-least-significant bit (bit 2) is 1 if `-d` is passed
(i.e. the user wants decompression mode).

- If the `-b` option was specified, then the given blocksize,
is recorded in the 16 most-significant bits of `global_options`,
otherwise the 16 most-significant bits of `global_options` are 0x0400
(representing a block size of 1024KB, which the program uses as a default
when none is specified).

If `validargs` returns -1 indicating failure, your program must call
`USAGE(program_name, return_code)` and return `EXIT_FAILURE`.
**Once again, `validargs` must always return, and therefore it must not
call the `USAGE(program_name, return_code)` macro itself.
That should be done in `main`.**

If `validargs` sets the least-significant bit of `global_options` to 1
(i.e. the `-h` flag was passed), your program must call `USAGE(program_name, return_code)`
and return `EXIT_SUCCESS`.

> :nerd: The `USAGE(program_name, return_code)` macro is already defined for you
> in `const.h`.

If validargs returns 0, then your program must read data from `stdin`,
either compressing it or decompressing it as specified by the values of
`global_options` and `block_size`, and writing the result to `stdout`.
Upon successful completion, your program should exit with exit status `EXIT_SUCCESS`;
otherwise, in case of an error it should exit with exit status `EXIT_FAILURE`.

> :nerd: Remember `EXIT_SUCCESS` and `EXIT_FAILURE` are defined in `<stdlib.h>`.
> Also note, `EXIT_SUCCESS` is 0 and `EXIT_FAILURE` is 1.

### Example validargs Executions

The following are examples of `global_options` settings for given inputs.
Each input is a bash command that can be used to run the program.

- **Input:** `bin/sequitur -h`.  **Setting:** 0x1 (`help` bit is set, other bits clear).

- **Input:** `bin/sequitur -c -b 1024`.  **Setting:** 0x4000002 (`compress` bit is set;
`blocksize` bits are 0x400, representing a block size of 1024KB).

- **Input:** `bin/sequitur -d`.  **Setting:** 0x00000004 (`decompress` bit is set;
`blocksize` bits are 0x0000).

- **Input:** `bin/sequitur -b 1024 -c`.  **Setting:** 0x0. This is an error
case because the specified argument ordering is invalid (`-b` is before `-c`).
In this case `validargs` returns -1, leaving `global_options` unset.

# Part 2: Grammar-Based Data Compression

This section gives some basic information on context-free grammars (CFGs) and how they
can be used as a compressed representation for strings.  Normally, you would study
CFGs in a Theory of Computation or Compilers course, but don't worry, we are not
interested here in CFGs in their full glory and it should not be difficult to
understand the way we are making use of them.

In general, a context-free grammar consists of a set of **rules**, which have the
following form:

```
H ==> B1 B2 ... Bn
```

Here H and B1, B2, ..., Bn are **symbols**, which can be either **terminal symbols**
("terminals", for short) or **nonterminal symbols** ("nonterminals", for short).
The symbol H which comprises the **head** of the rule is always a nonterminal,
but the symbols B1, B2, ..., Bn that comprise the **body** of the rule can be either
terminals or nonterminals.
A rule specifies a kind of expansion or rewriting that can be performed on a string:
if the nonterminal H occurs in a string, then the string can be expanded by replacing
individual occurrences of H by the string B1 B2 ... Bn.

We are only interested here in a very specific class of CFGs.  One restriction is that
we do not allow **recursive** rules, in which the nonterminal in the head also appears in the
body of the same rule.  In fact, we require strictly hierarchical CFGs in which there is an
ordering > on the symbols that is respected by the rules, so that if symbol H appears as the
head of a rule, then we must have H > B1, H > B2, ..., and H > Bn.
We also assume that there is a rule whose head S is greater in the ordering than all the
other symbols.  We call S the **start symbol**, and the rule having that symbol as its head
the **main rule**.  We assume in addition that every nonterminal is the head of **some** rule.
Taken together, what these conditions mean is that we can take the start symbol S,
expand it by applying the unique rule having S as its head, and then continue to expand
the string we obtain by replacing other nonterminals by the bodies of their associated rules,
until finally we obtain a string that consists only of terminal symbols.
That string is the string **generated by** the CFG.

As a concrete example, consider the following CFG:

```
S ==> DEcE
D ==> ab
E ==> Da
```

Here we are using upper-case letters for nonterminals and lower-case letters for terminals.
Beginning with the start symbol S, we can perform the following sequence of rewritings,
in which at each step we expand the **leftmost** nonterminal using its associated rule:

```
S ==> DEcE ==> abEcE ==> abDacE ==> ababacE ==> ababacDa ==> ababacaba
```

If you think about it a little bit, you can convince yourself that the leftmost choice
was just a convenience and that no matter in what order we chose to apply the rules,
we would still arrive at the same final string of terminals.  So we can think of this
grammar as a representation or encoding of the string `ababacaba`.
The data compression aspect comes from the fact that very long strings can sometimes
be encoded by very short grammars.  For example, consider the string:
`aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa`, which consists of 32 `a`s in a row.
This string can be represented by the following CFG:

```
S ==> BB
B ==> CC
C ==> DD
D ==> EE
E ==> aa
```

If you follow the pattern, you will see that you can encode the string that consists
of 2^n `a`s by a CFG that contains only `n` rules, each with a body that consists of
just two symbols.  So we can encode long repeating sequences of `a`s with CFGs
that are exponentially shorter in terms of how many symbols it takes to write them down.

OK, so now that we've seen how CFGs can be used to encode strings there are two
algorithmic problems to consider, one of which is easier than the other:

1. Given a CFG satisfying the restrictions set out above, how can we compute
   the string that it represents?
	   
2. Given a string, how can we find a CFG (hopefully "shorter") that represents
   that string?

The first problem, which is the easier one, is what you will solve in the first
part of this assignment.  You will write C code to read in a CFG specified in a
convenient format, and then you will use the rules to expand the start symbol to
obtain the string encoded by the CFG.

The second problem is harder, at least if we want to take the "shorter" thing
seriously.  Note that there is a trivial solution that is not very satisfying:
if we want to encode the string `aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa` by a CFG,
we can do it using a single rule:

```
S ==> aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
```

but that is hardly very economical, since it takes more symbols to write down
the CFG that it does to write down the string it encodes!
The *Sequitur* algorithm was devised to try to do a better job of solving
this second problem.  Basically, Sequitur tries to read a string and discover
repetitive structure in it that allows us to represent the string as a CFG
that consists of set of rules that is smaller than the length of the original string.
For example, Sequitur will discover the CFG given above given the string of 32 `a`s.

We will come back to Sequitur a bit later, but for right now, this is what
you need to know.  If you want to read more about Sequitur, you can visit
[this website](http://www.sequitur.info/).

# Part 3: Serialized Representation of CFGs

For the next part of this assignment, you will implement an application that
can read a CFG and expand it to obtain the string it encodes.  For this, we need
a definite format for encoding a CFG so that your application can read it.
To define such a format is the object of this section.

When all is said and done, a CFG is a sequence of **rules**, and each rule is a
sequence of **symbols**.  We will encode a CFG as a **block**, which begins
with a "start of block" (SOB) marker and ends with an "end of block" (EOB)
marker.  Between these two markers will be the rules, separated by
**rule delimiter** (RD) markers.  Each rule will be a sequence of symbols,
starting with the nonterminal symbol at the head of the rule and followed by
the sequence of symbols that comprise the body of the rule.
For us, there will always be at least two symbols in the body of a rule,
which means that each rule will have at least three symbols.

All that remains now is to settle on a format for representing the SOB, EOB,
and RD markers, as well as the sequence of symbols in a rule.
For this, we will take advantage of the properties of the already-devised **UTF-8**
encoding for Unicode characters (*e.g.* see [here](https://en.wikipedia.org/wiki/UTF-8).
We will represent terminal and nonterminal symbols as points in the Unicode
code space, which consists of 2^21 values ranging from `U+0000` to `U+10FFFF`.
This gives us the ability to represent 2^21 or somewhat more than two million symbols.
We reserve the first 256 points in this space (i.e. form `U+0000` to `U+00FF`)
for terminal symbols.  This will allow us to use CFGs to encode any sequence of
8-bit bytes.  The remaining values will be used for nonterminal symbols.

The UTF-8 encoding for Unicode has the convenient property of being self-synchronizing:
we can scan through a sequence of bytes and readily identify the beginning of each
successive code point.  So we will represent each rule simply as a UTF-8 encoded
sequence consisting of the symbols in the rule, beginning with the head of the rule
and continuing with the body.  Each symbol will be encoded as either one, two, three,
or four bytes, depending on the value of the symbol.
However, since each rule will have an unknown number of symbols in its body,
we need a way to identify the end of each rule.  That is where the rule delimiter (RD)
marker comes in.  A rule delimiter will be represented by single byte with hexadecimal
value `0x85`.  We will immediately be able to identify this marker, because no
valid UTF-8 encoding can start with a "continuation byte" having 10 as its two most
significant bits as 0x85 does.  So we won't be able to mistake a rule delimiter,
which terminates a rule, for another symbol in the same rule.
The same property holds for the "start of block" marker, which we will represent
as a single byte with value `0x83`, and the "end of block" marker, which we will
represent as a single byte with value `0x84`.

To summarize, a CFG using up to roughly 2 million different symbols will be represented
as a sequence of bytes that starts with `0x83` (i.e. SOB) continues with a sequence
of rules, each of which is a sequence of UTF-8 encoded symbols, with the rules being
separated by single bytes with value `0x85`, and finally terminated by a single byte
with the value `0x84`.

To be absolutely explicit about it, the CFG discussed above that represents the string
`ababacaba` can be represented as the following sequence of bytes (values in hexadecimal):

```
83 c4 80 c4 81 c4 82 63 c4 82 85 c4 81 61 62 85 c4 82 c4 81 61 84
```

Between the `83` at the beginning and `84` at the end, we have the following
three UTF-8 encoded sequences, separated by `85`:

```
c4 80 c4 81 c4 82 63 c4 82
c4 81 61 62 
c4 82 c4 81 61
```

If we replace the UTF-8 encodings by the sequence of code points that they encode,
we obtain:

```
U+0100 U+0101 U+0102 U+0063 U+0102
U+0101 U+0061 U+0062
U+0102 U+0101 U+0061
```

Recall that Unicode assigns the code points U+0061, U+0062, and U+0063
as the encodings of the characters 'a', 'b', and 'c', respectively.
These "small" values (code points from U+0000 to U+00FF) for us represent terminal symbols.
The remaining values U+0100, U+0101, and U+0102 are "large" values that represent
nonterminals.  If we arbitrarily assign letters 'S', 'D' and 'E' to these
(adopting the convention that the head of the first rule is to be taken as the
start symbol 'S'), then the resulting CFG can be written as:

```
S ==> DEcE
D ==> ab
E ==> Da
```

which is the CFG that we already discussed above.

> :scream:  Students are often confused at this point about the relationship
> between a sequence of hexadecimal numerals like `c4 82 c4 81 61` and the
> input data that their program will have to read.  It is important to understand
> that the input data is simply a sequence of bytes, which will **not** be text,
> in general.  Each byte is an uninterpreted 8-bit value, which can be read
> from the input stream using, *e.g.* the function `fgetc()`.  The input bytes
> are **not** encoded in hexadecimal, and your program will **not** be performing
> any interpretation of hexadecimal numerals.  A textual representation such as
> `c4 82 c4 81 61` is simply a way of representing a sequence of six bytes in a
> form that a human being can understand.

A final issue is that we have a limited amount of memory that we can use to store
grammar rules, but we don't want to limit the maximum size of a sequence of bytes
that we can compress.  For this reason, we permit a compressed data transmission
to consist of a sequence of arbitrarily many blocks, each of which represents up to
a specified maximum **block size** number of data bytes from the original sequence.
So that we can readily identify the beginning and end of a compressed data
transmission (*e.g.* to verify that transmission has not been truncated),
we will use an additional "start of transmission" (SOT) marker at the beginning
of the transmission and an "end of transmission" (EOT) marker at the end.
The SOT marker will be a single byte having value `0x81`.  The EOT marker will be
a single byte having value `0x82`.

**To summarize:** a compressed data transmission will consist of an initial SOT
marker byte having the value `0x81`, followed by zero or more compressed blocks,
each of which starts with an SOB marker and ends with an EOB marker and contains
a nonempty set of rules separated by RD markers as described above, and finally
ending with an EOT marker byte having the value `0x82`.

The following is an example of a complete compressed data transmission,
which has been formatted to be human-readable using the command `od` with arguments
`-t x1`.  The command `od`, which stands for "octal dump" is a traditional Unix program
to display binary data in a human-readable form.  The flags `-t x1` instruct `od`
to display this data as a sequence of hexadecimal values, each of which represents
one byte of data.  The longer sequence of digits at the beginning of each line
is not part of the data, but rather gives the **offset** in bytes from the beginning
of the sequence (given in octal) for the data shown on that particular line.
Thus, the first line represents the first 16 data bytes (starting at offset `00000000`),
the second line represents the second 16 data bytes (starting at offset `00000020`),
and so on.

> :nerd: To find out more about `od`, you should read the manual page using
> the command `man 1 od`.

```
0000000 81 83 c4 80 49 c4 81 6c 69 74 20 61 c4 81 68 65
0000020 65 74 2c 0a 85 c4 81 20 73 84 83 c4 80 41 c4 81
0000040 68 65 65 74 20 49 c4 81 6c 69 74 2e 0a 85 c4 81
0000060 20 73 84 83 c4 80 55 70 6f 6e 20 61 c4 81 6c 69
0000100 74 74 65 64 c4 81 85 c4 81 20 73 84 83 c4 80 68
0000120 65 65 74 0a 49 20 73 69 74 2e 0a 84 82
0000135
```

This compressed data transmission was made with a very small block size of 16
and there are four blocks in it.  The first three blocks will each expand to
16 bytes of data, and the last block will expand to 12 bytes, for a total of 60 bytes.
The original data was the following text file, where each line was terminated by
a single newline byte with value 0x10:

```
I slit a sheet,
A sheet I slit.
Upon a slitted sheet
I sit.
```

> :nerd:  You might notice in this case that the size of the "compressed" version
> of the file, at 93 bytes (135 octal), is actually substantially larger than the
> size of the original file, which was 60 bytes.  This is due to the small block
> size that was used, as well as the shortness of the original file; the Sequitur
> algorithm can only find patterns in a sequence if the sequence is long enough to
> contain such patterns in the first place.  If the file is compressed as a single
> block, the does a little bit better (88 bytes), but still longer than the original
> file.

# Part 4: Representing Rules

For our application, we will need a way to represent a set of rules in a form
that supports the kind of manipulations that we are going to need to be able to
perform in order to do compression and decompression.
For right now, just think about the problem of expanding the rules to obtain the
string that they represent (*i.e.* decompression).  Later, we will consider
manipulations used to build a set of rules incrementally while reading an
input string (*i.e.* compression).

For this assignment, we are going to represent everything in terms of linked lists
of **symbols**.  Each symbol will be represented by an instance of the following
C structure, which is defined in `sequitur.h`:

```
typedef struct symbol {
    unsigned int value;        // The value that uniquely identifies the symbol.
    unsigned int refcnt;       // Reference count if symbol is head of a rule, otherwise 0
    struct symbol *rule;       // NULL for terminal, non-NULL for nonterminal or sentinel
    struct symbol *next;       // Next symbol in rule body (or the sentinel, in case of last symbol)
    struct symbol *prev;       // Previous symbol in rule body (or the sentinel, in case of first symbol)
    struct symbol *nextr;      // If sentinel, next rule in list of all rules.
    struct symbol *prevr;      // If sentinel, previous rule in list of all rules.
} SYMBOL;
```

Each symbol has a "value" field, which determines the type
of the symbol (either terminal or nonterminal) and which uniquely identifies
that symbol.  "Small" values (i.e. `< FIRST_NONTERMINAL`) are used for terminal
symbols and "large" values (i.e. `>= FIRST_NONTERMINAL`) are used for nonterminal
symbols.  Terminal symbols correspond directly to individual bytes of data in
a string.  The value of a terminal symbol is simply the value of the corresponding
data byte, regarded as an unsigned integer in the range `[0, FIRST_NONTERMINAL)`.
For example, the three-letter sequence `abc` would be encoded in ASCII or UTF-8
as a sequence of three bytes with values 61, 62, 63 (expressed in hexadecimal).
We would represent this sequence by a list of three SYMBOL structures having values
61, 62, and, 63.  Nonterminal symbols, on the other hand, do not directly correspond
to data bytes in a sequence being compressed or decompressed, and the value of a
nonterminal symbol has no particular significance other than to distinguish one
particular nonterminal symbol from another.  Values of nonterminal symbols will
be assigned arbitrarily by the compression algorithm.

Each symbol also has some pointer fields, which we will use to chain symbols together
into lists.  The `next` and `prev` fields will be used to chain symbols together
as a doubly linked list to form the body of a rule.  The `nextr` and `prevr` fields
will be used to link the heads of rules (which will be represented by nonterminal
symbols) into a doubly linked list.  There is also a `refcnt` field, which is used
by the compression algorithm to maintain a count of the number of times a rule has
been used.  (We will discuss compression-related issues later on.)

We will represent a grammar rule by a **circular, doubly linked list** of the symbols
in the body of the rule, together with an additional **sentinel** symbol,
which is a nonterminal symbol that represents the head of the rule.
The sentinel is linked between the first and last symbol in the body of the
rule, resulting in the following (doubly linked) configuration:

```
H <-> B1 <-> B2 <-> ... <-> Bn
^                           ^
|                           |
+---------------------------+
```

The `next` and `prev` fields of the SYMBOL structure are used to chain symbols
together into a rule.  We refer to a rule using a pointer to the sentinel node `H`.
Sentinel nodes are distinguishable from other nodes by virtue of their having
a non-`NULL` "rule" field that points back to the sentinel node itself.

We also link rule sentinels together into a circular, doubly linked list of all rules,
using the `nextr` and `prevr` fields of the `SYMBOL` structure.  This circular
list does not itself use any additional sentinel node, but the global variable `main_rule`
is used to point to a distinguished "main rule" in the list, from which the other rules
can be accessed:

```
main_rule -> H1 <-> H2 <-> H3 <-> ... <-> Hn
             ^                            ^
             |                            |
             +----------------------------+
```

Circular, doubly linked lists are a useful data structure that is especially
convenient for adding and removing entries at arbitrary positions in the list.
The circular structure simplifies insertion and deletion by eliminates some edge
cases that otherwise arise in dealing with `NULL` pointers at the beginning and
end of a list.  You can read more about this kind of list
[here](https://en.wikipedia.org/wiki/Doubly_linked_list).

## Implementation

At this point, you should implement the following functions for which stubs and
specifications are given in `symbol.c` and `rules.c`:

**Needed for decompression:**
```c
void init_symbols(void);
SYMBOL *new_symbol(int value, SYMBOL *rule);

void init_rules(void);
SYMBOL *new_rule(int v);
void add_rule(SYMBOL *rule);
```

**Additionally needed for compression:**
```c
void recycle_symbol(SYMBOL *s);

void init_rules(void);
void delete_rule(SYMBOL *rule);
SYMBOL *ref_rule(SYMBOL *rule);
void unref_rule(SYMBOL *rule);
```

The implementations of these functions should be straightforward from their specifications,
given the discussion above and what you should already know about doubly linked lists
from previous courses.

# Part 5: Decompression

For this part of the assignment, your objective is to implement the function
`decompress()`, which has the following specification and stub in `comdec.c`:

```c
/**
 * Main decompression function.
 * Reads a compressed data transmission from an input stream, expands it,
 * and and writes the resulting decompressed data to an output stream.
 * The output stream is flushed once writing is complete.
 *
 * @param in  The stream from which the compressed block is to be read.
 * @param out  The stream to which the uncompressed data is to be written.
 * @return  The number of bytes written, in case of success, otherwise EOF.
 */
int decompress(FILE *in, FILE *out) {
    // To be implemented.
    return EOF;
}
```

We have already discussed in the previous section the format of the compressed input stream.
Your `decompress` function should work by using the C Standard I/O Library function
`fgetc()` to read the input stream byte-by-byte, parse the stream into blocks,
decode the rules contained in each block, and then expand the rules starting
with the main rule, outputting the uncompressed original data bytes as you go along.
You will probably want to implement a helper function that can read a UTF-8 encoded
character from an input stream and identify the special marker bytes (which are
illegal for UTF-8, but important for parsing the compressed data transmission
into blocks and rules).
For each block, you will need to construct a linked-list representation of each
of the rules in the block, and you will need to link all these rules in a block into
a single list, pointed at by the `main_rule` global variable.
You should use the `new_symbol` function that you implemented as part of the previous
section to obtain fresh instances of the `SYMBOL` structure as you require them.
In addition, you should use the `new_rule` function you implemented to construct
new rules having a specified nonterminal at the head (but with empty bodies),
and you should use the `add_rule` function to link rules into the list of all rules
headed by the `main_rule` variable.
You are on your own as far as adding symbols to the body of a rule is concerned;
there is no function that has been specified for this.  You might choose to
add a suitable function or functions to `symbol.c`, or you could just code these
operations in-line where they are required.

Once a block has been read and the corresponding set of rules constructed and linked
from the `main_rule` variable, you will need to use the rules to perform expansion,
starting from the main rule, and to output the decompressed data bytes to the output stream.
As the expansion of each block is completed, you will need to reinitialize and go on
to the next block, until finally the EOT marker is read, indicating the end of the
compressed transmission.  At that point you should call ``fflush()` on the output
stream, to ensure that no output remains buffered in memory.

The actual expansion of a set of rules can be carried out recursively as follows.
Beginning with the main_rule, traverse the body of the rule symbol-by-symbol.
Each time a terminal symbol is encountered, use `fputc()` to write to the output stream
a single data byte corresponding to the value of that symbol.
When a nonterminal symbol is encountered, recurse and expand using the rule having
that nonterminal symbol at its head.  With this algorithm, you do not need to
modify any rules or lists once the set of rules has been constructed -- it is just
a simple recursive traversal.  There is one issue, and that is you will need to be able
to find, given a nonterminal symbol, the rule that has that symbol as its head.
Although you could find the rule by scanning the list of all rules, it is much more
efficient to have a map from nonterminal symbols to rules.  For that purpose, you have
been provided with the `rule_map` global variable.  While reading a block, each time you
construct a rule, put a pointer to the head of the rule in the entry of `rule_map`
corresponding to the nonterminal at the rule head.
During expansion, when you encounter a nonterminal, you can then use the rule map
to go directly to the associated rule.

Be sure to modify `main.c` so that you will be able to perform decompression
by running your program from the command line using the `-d` flag; *viz.*:

```
$ bin/sequitur -d < COMPRESSED_FILE
```

> :nerd: The `<` symbol tells the shell to perform **input redirection**;
> *i.e.* the shell arranges for the contents of `COMPRESSED_FILE` to be made available
> to `bin/sequitur` on the standard input.  This way, your program can be used
> to decompress various files without having to modify it or to handle command-line
> arguments that specify the names of input files.
> It is also possible to perform **output redirection** by using `> OUTPUT_FILE`:
> the file `OUTPUT_FILE` will be created (or truncated if it already existed -- be careful!)
> and the output produced by the program on `stdout` is sent to that file instead
> of to the terminal.  This will allow you to save the results of decompression
> for closer inspection.

You should also check that your program produces the proper **exit status**,
as specified in `main.c`.  For example:

```
$ bin/sequitur -d < COMPRESSED_FILE
$ echo $?
0
```

The shell collects the exit status of each command that it runs and makes it available
through the shell variable `$?`.  The `echo` command above causes the value of that
variable to be printed.  In the above example, the value was 0, which corresponds
to `EXIT_SUCCESS`.

> :nerd:  Once you have successfully implemented decompression, you will be able
> to use your decompression program to "unlock" the code for the Sequitur algorithm
> that I have provided to help you implement the compression portion of the
> application.  Use your decompression program on the file `rsrc/sequitur.c.seq`
> in the basecode handout, and replace the stub file `src/sequitur.c` with the
> decompressed output (assuming of course, that your decompression program worked
> correctly and actually produced a valid C source file):
>
> ```
> $ bin/sequitur -d < rsrc/sequitur.c.seq > src/sequitur.c
> ```

# Part 6: Compression

In this part of the assignment, you will implement the function `compress()`,
which has the following stub in `comdec.c`:

```c
/**
 * Main compression function.
 * Reads a sequence of bytes from a specified input stream, segments the
 * input data into blocks of a specified maximum number of bytes,
 * uses the Sequitur algorithm to compress each block of input to a list
 * of rules, and outputs the resulting compressed data transmission to a
 * specified output stream in the format detailed in the header files and
 * assignment handout.  The output stream is flushed once the transmission
 * is complete.
 *
 * The maximum number of bytes of uncompressed data represented by each
 * block of the compressed transmission is limited to the specified value
 * "bsize".  Each compressed block except for the last one represents exactly
 * "bsize" bytes of uncompressed data and the last compressed block represents
 * at most "bsize" bytes.
 *
 * @param in  The stream from which input is to be read.
 * @param out  The stream to which the block is to be written.
 * @param bsize  The maximum number of bytes read per block.
 * @return  The number of bytes written, in case of success,
 * otherwise EOF.
 */
int compress(FILE *in, FILE *out, int bsize) {
```

The compression algorithm we will use is called **Sequitur**, and you can read
about it [here](http://www.sequitur.info/).  That website also has reference implementations
in several languages, which you are welcome to read and even use if you like, though
I actually don't expect that the code itself will be of much use to you other than possibly
for understanding purposes.

> :scream: Note that in general you are not welcome to refer to code on websites in order
> to do the homework assignments, but I am making an exception, in this particular case,
> for this particular website.

I decided that to ask you to implement the complete Sequitur algorithm yourself was probably
too difficult, so I have provided code for the core of the algorithm.
As a result, you don't have to understand it in full detail, but it will be helpful to have
some idea as to how it works, in order to identify and correct bugs in the supporting code
that you do have to implement.  So I will briefly discuss the algorithm here.

The basic idea of Sequitur is to read through a "string" (here by "string" we mean any
sequence of bytes) in a byte-by-byte fashion, while constructing incrementally a CFG that
somewhat efficiently encodes the portion of the string read so far.
In order to try to achieve efficiency, the CFGs produced by Sequitur satisfy two constraints:

1. *Digram uniqueness*:  Each **digram** (*i.e.* two-symbol sequence) can be found
   in at most one place in the CFG.

2. *Rule utility*:  Each rule in the CFG has at least two references to it
   (a reference is an occurrence of the rule head in the body of some other rule
   in the grammar).

Sequitur starts out initially with a single main rule having an empty body.
Each iteration of the "main loop" (you do have to implement this) reads a byte of
data from the input stream, creates a terminal symbol having as its value the value
of the byte read, and appends this symbol to the end of the main rule by calling
the function `insert_after()`, the implementation of which is part of the code that I supplied.
The newly added symbol, together with the symbol (if any) immediately to its left
forms a digram, which might or might not already be present somewhere else in
the current set of rules.  If it is not present elsewhere, then that is fine,
but if it is present, then the "Digram uniqueness" condition has been violated.
In order to check for and handle such violations, each time that `insert_after()`
has been called, the algorithm "main loop" will immediately follow it with a call
to function `check_digram()` (also in the code that I supplied), passing it a pointer
 to the second-to-last symbol in the main rule (*i.e.* the one just before the newly added symbol).
If `check_digram()` finds that the digram uniqueness condition has been violated,
then it will create a new rule, using a freshly generated nonterminal symbol for its
head and the problematic digram as its body, and it will eliminate the two
existing occurrences of that digram by replacing them by the fresh nonterminal,
leaving as the only occurrence the one in the newly generated rule.  So in pseudocode,
the main loop looks as follows:

```
   while(input data remains) {
       read a byte of input
	   create terminal symbol from input byte
	   use insert_after() to append symbol to main rule
	   call check_digram() on second-to-last symbol in main rule
   }
```

For example, consider the behavior of the algorithm on the input string "abbabbab".
I have used my implementation of the program to show how it works.
The first few steps just append symbols from the input and do not create any
repeated digram:

```
[0:0] $256 => a 

[0:0] $256 => a b 

[0:0] $256 => a b b 

[0:0] $256 => a b b a 
```

To explain the notation: each line here shows the main rule as the symbols are getting
appended to the end.  The first number inside the square brackets is the rule number,
which will be zero for the main rule.  The second number inside the square brackets
is the "reference count" for that rule, which tells how many times the head of that rule
is referred to in the body of other rules.  As the head of the main rule is never
referred to by any other rules, the reference count for it will always be zero.
The `$256` represent a nonterminal symbol with value 256, which is `FIRST_NONTERMINAL`.
The head of the main rule is always the first nonterminal, which is generated in the
initialization phase of the algorithm.  To the right of the `=>` is the rule body,
which in each case above consists of terminal symbols.  To make things easy to follow,
terminal symbols corresponding to printable characters have just been rendered as
the characters themselves (separated by spaces for readability).

Next, `b` is read and inserted:

```
[0:0] $256 => a b b a b 
```

This has created two instances of digram `ab`: one at the beginning of the rule
and one at the end.  The `check_digram()` function is called to check for and
handle this situation.  The result is as follows:

```
[0:0] $256 => $257 b $257 
[6:2] $257 => a b 
```

Note that an additional rule (rule number 6, having nonterminal `$257` as its head)
has been created with `a b` as its body, and the two original occurrences of the digram
`a b` have been replaced by instances of the nonterminal `$257`.  This eliminates
the repeated diagram from the rules, and leaves two references to the newly generated
rule, which is reflected in the reference count shown in the square brackets.

Next, another `b` is read and inserted.  This again creates repeated digrams;
in this case there are two occurrences of `$257 b`.  A new rule (rule number 1)
is created, and it is used to eliminate the repeated diagram:

```
[0:0] $256 => $258 $258
[6:1] $257 => a b 
[1:2] $258 => $257 b
```

However, notice what has happened: rule 6 previously had two references, but they
have both been eliminated and there is left only one reference to it, now in rule 1.
This is reflected in the reference count of rule 6, which is now 1.
This violates the "rule utility" condition (a rule that is applied only once is
just adding baggage without yielding any compression), so the algorithm now
expands the one remaning use of rule 6 and deletes that rule, leaving just two rules:

```
[0:0] $256 => $258 $258 
[1:2] $258 => a b b 
```

Next, an `a` is read and inserted without the creation of any repeated digram:

```
[0:0] $256 => $258 $258 a 
[1:2] $258 => a b b 
```

Then, a `b` is read and inserted, which results in two occurrences of digram `a b`:
one in rule 0 and one in rule 1:

```
[0:0] $256 => $258 $258 a b 
[1:2] $258 => a b b 
```

To correct this situation, a new rule (rule 5) is created and used to eliminate
the two occurrences of `a b`, leaving the following as the final set of rules:

```
[0:0] $256 => $258 $258 $259 
[1:2] $258 => $259 b 
[5:2] $259 => a b 
```

You may verify that expanding starting with rule 0 yields the original string `abbabbab`.

> :nerd:  You might have noticed that the rule numbers seem to be getting assigned randomly.
> In fact, the rule numbers are the indices in the `symbol_storage` array of the symbols
> used for the rule heads, and the order in which these get used depends on internal
> details like what symbols are recycled and re-used during each of the steps.
> The actual values are of no particular significance.

That's basically the gist of what the algorithm does.  If you study the implementation
and think about it more deeply, you will find that there is some more subtlety to it,
but this is pretty much what you need to understand in order to make use of the code
that I have supplied.

## Implementation:  Digram Hash Table

At this point, you will need to implement the **digram hash table** (in `digram_hash.c`),
which is used to keep track of the digrams that exist in the current set of rules.
The functions to implement are:

```
void init_digram_hash(void);
SYMBOL *digram_get(int v1, int v2);
int digram_delete(SYMBOL *first);
int digram_put(SYMBOL *first);
```

Specifications for these functions are given with the stubs in `digram_hash.c`.
As previously discussed, the basic scheme used for the hash table can be summarized
as "open addressed hashing using linear probing with an increment of 1".
What this means is: to look up a digram in the table you first apply the
`DIGRAM_HASH` function to the values of the two symbols in the digram.
This gives you an index into the `digram_table` that will be the starting point
for your search.  Starting from this index, you step from one entry to the next,
checking the symbol values of each digram you find against the values you are
looking for.  If you get to the end of the array, you continue at the beginning.
If you find a matching entry before you hit the first `NULL` entry, your lookup
was successful.  If you hit `NULL` before finding a match, your lookup failed.

For insertion, you perform a search in the same way as for lookup;
however this time you are looking to find a `vacant` entry *without* first
seeing a matching entry.  If you first find a matching entry, then the digram
already exists in the table and your return value should reflect that.
If you find a vacant entry first, then a matching digram did not previously
exist in the table and you install the new entry in the vacant position.

Deletion in an open-addressed hash table is not quite so straightforward.
You can't just replace the entry to be deleted by `NULL`, because this `NULL`
could end up between some other entry in the table and the starting index
given by the hash function.  In that case, a subsequent lookup of this entry
will (incorrectly) fail because the `NULL` will be encountered first,
terminating the search.  A solution to the problem is not to replace a deleted
entry by `NULL`, but rather by some other value `TOMBSTONE`, which is
distinguishable from `NULL`.  On lookup, you should treat `TOMBSTONE` values
as if they are occupied positions, but on insertion, you should treat them
as vacant.

I'm not going to include much more on the hash table implementation, because
everybody is supposed to have taken Data Structures prior to this course and
even if you don't remember anything about hash tables, you ought to be able
to review that material without too much difficulty.

> :scream:  You are **strongly cautioned** not to search for C implementations
> of hash tables or to use in your own implementation any such code you might
> inadvertently encounter.  You are expected to write this code yourself!

## Implementation:  Compression

Finally, once the digram hash table is done, you are ready to implement the
`compress()` function.
This function will begin by outputting a "start of transmission" (SOT) mark
to the output stream.
Then it will read blocks of data from the input stream; each block consisting
of as many bytes of data as possible, but no more than the maximum block
size passed as the parameter to `compress()`.
At the beginning of each block, the symbols, rules, and digram hash table are
re-initialized, and the main rule is created and installed in the `main_rule`
variable.
As each byte of data is read, it is appended to the end of the main rule
using `insert_after()` and then `check_digram()` is called to correct violations
of the two constraints that the set of rules is supposed to satisfy according
to the Sequitur algorithm.
When the end of a block is reached, the rules in the block are output as a
single block of compressed data according to the specification already discussed
for decompression.
Each block will start with a "start of block" (SOB) mark and end with an
"end of block" (EOB) mark, with intervening rules separated by "rule delimiter"
(RD) marks.  There must be at least one rule in each block, so if EOF is reached
before any data is read for a block, you will not output any SOB or EOB
but instead will go directly to outputting EOT.
A special case of this is where there is no data at all in the input.
In that case, the compressed output will consist simply of SOT followed immediately
by EOT, without any intervening blocks.

In order to output the rules within a block you will need to implement functions
that are capable of emitting UTF-8 encoded Unicode code points.
These are essentially the inverses of what you had to implement in order to read
the rules from a block.

# Part 7: Running the Completed Program

In any of its operating modes, the `sequitur` program reads from `stdin` and writes
to `stdout`.  As the input and output of the program is binary data, it will not
be useful to enter input directly from the terminal or display output directly to
the terminal.  Instead, the program can be run using **input and output redirection**
as has already been discussed; *e.g.*:

```
$ bin/sequitur -c -b 100 < testfile > outfile
```

This will cause the input to the program to be redirected from the file `testfile`
and the output from the program to be redirected to the file `outfile`.
For debugging purposes, the contents of `outfile` can be viewed using the
`od` ("octal dump") command:

```
$ od -t x1 outfile
0000000 81 83 c4 80 49 c4 81 6c 69 74 20 61 c4 81 68 65
0000020 65 74 2c 0a 85 c4 81 20 73 84 83 c4 80 41 c4 81
0000040 68 65 65 74 20 49 c4 81 6c 69 74 2e 0a 85 c4 81
0000060 20 73 84 83 c4 80 55 70 6f 6e 20 61 c4 81 6c 69
0000100 74 74 65 64 c4 81 85 c4 81 20 73 84 83 c4 80 68
0000120 65 65 74 0a 49 20 73 69 74 2e 0a 84 82
0000135
```

If you use the above command with an `outfile` that is much longer, there would
be so much output that the first few lines would be lost off of the top of the screen.
To avoid this, you can **pipe** the output to a program called `less`:

```
$ od -t x1 outfile | less
```

This will display only the first screenful of the output and give you the
ability to scan forward and backward to see different parts of the output.
Type `h` at the `less` prompt to get help information on what you can do
with it.  Type `q` at the prompt to exit `less`.

Alternatively, the output of the program can be redirected via a pipe to
the `od` command, without using any output file:

```
$ bin/sequitur -c -b 100 < testfile | od -t x1 | less
```

## Testing Your Program

Pipes can be used to test your program by compressing a file and immediately
decompressing it:

```
$ cat testfile | bin/sequitur -c -b 100 | bin/sequitur -d > outfile
```

If the program is working properly, the contents of `outfile` should
be identical to those of `testfile`.
It is useful to be able to compare two files to see if they have the same content.
 The `diff` command (use `man diff` to read the manual page) is useful for comparison
of text files, but not particularly useful for comparing binary files such as
compressed data files.  However the `cmp` command can be used to perform a
byte-by-byte comparison of two files, regardless of their content:

```
$ cmp file1 file2
```

If the files have identical content, `cmp` exits silently.
If one file is shorter than the other, but the content is otherwise identical,
`cmp` will report that it has reached `EOF` on the shorter file.
Finally, if the files disagree at some point, `cmp` will report the
offset of the first byte at which the files disagree.
If the `-l` flag is given, `cmp` will report all disagreements between the
two files.

We can take this a step further and run an entire test without using any files:

```
$ cmp -l <(cat testfile) <(cat testfile | bin/sequitur -c -b 100 | bin/sequitur -d)
```

This compares the original file `testfile` with the result of taking that
file and first compressing it using a block size of 100KB and then decompressing it.
Because both files are identical, `cmp` outputs nothing.

> :nerd: `<(...)` is known as **process substitution**. It is allows the output of the
> program(s) inside the parentheses to appear as a file for the outer program.

> :nerd: `cat` is a command that outputs a file to `stdout`.

## Unit Testing

Unit testing is a part of the development process in which small testable
sections of a program (units) are tested individually to ensure that they are
all functioning properly. This is a very common practice in industry and is
often a requested skill by companies hiring graduates.

> :nerd: Some developers consider testing to be so important that they use a
> work flow called **test driven development**. In TDD, requirements are turned into
> failing unit tests. The goal is then to write code to make these tests pass.

This semester, we will be using a C unit testing framework called
[Criterion](https://github.com/Snaipe/Criterion), which will give you some
exposure to unit testing. We have provided a basic set of test cases for this
assignment.

The provided tests are in the `tests/hw1_tests.c` file. These tests do the
following:

- `validargs_help_test` ensures that `validargs` sets the help bit
correctly when the `-h` flag is passed in.

- `validargs_serialize_test` ensures that `validargs` sets the compression-mode bit
correctly when the `-c` flag is passed in.

- `validargs_error_test` ensures that `validargs` returns an error when the
`-b` (blocksize) flag is specified together with the `-d` (decompress) flag.

- `help_system_test` uses the `system` syscall to execute your program through
Bash and checks to see that your program returns with `EXIT_SUCCESS`.

### Compiling and Running Tests

When you compile your program with `make`, a `sequitur_tests` executable will be
created in your `bin` directory alongside the `sequitur` executable. Running this
executable from the `hw1` directory with the command `bin/sequitur_tests` will run
the unit tests described above and print the test outputs to `stdout`. To obtain
more information about each test run, you can use the verbose print option:
`bin/sequitur_tests --verbose=0`.

The tests we have provided are very minimal and are meant as a starting point
for you to learn about Criterion, not to fully test your homework. You may write
your own additional tests in `tests/sequitur_tests.c`. However, this is not required
for this assignment. Criterion documentation for writing your own tests can be
found [here](http://criterion.readthedocs.io/en/master/).

# Hand-in instructions

**TEST YOUR PROGRAM VIGOROUSLY BEFORE SUBMISSION!**

Make sure that you have implemented all the required functions specifed in `const.h`.

Make sure that you have adhered to the restrictions (no array brackets, no prohibited
header files, no modifications to files that say "DO NOT MODIFY" at the beginning,
no functions other than `main()` in `main.c`) set out in this assignment document.

Make sure your directory tree looks basically like it did when you started
(there could possibly beadditional files that you added, but the original organization
should be maintained) and that your homework compiles (you should be sure to try compiling
with both `make clean all` and `make clean debug` because there are certain errors that can
occur one way but not the other).

This homework's tag is: `hw1`

`$ git submit hw1`

> :nerd: When writing your program try to comment as much as possible. Try to
> stay consistent with your formatting. It is much easier for your TA and the
> professor to help you if we can figure out what your code does quickly!

