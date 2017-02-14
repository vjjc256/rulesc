# rulesc
A Go package that can run rules unit tests written in Go against the C implementation of those rules

## rulesc is a way to apply Go unit tests to the C implementation of rules
**rulesc** is a hybrid package, containing both Go and C sources, which
serves as a bridge between the Go and C implementations of the rules
controlling a Currant outlet's behavior. Rules are initially prototyped in
Go because of, among other reasons, Go's integrated support for unit
tests. Once the Go implementation of a rule has been finalized, the rule
needs to be ported to C for use on the hardware. Porting a rule's
implementation to C is relatively straightforward, but porting the unit
tests is not. **rulesc** solves this problem by using cgo to apply the unit
tests written in Go to the underlying C implementation.

## rulesc is a git submodule used in both the [rules](https://github.com/currantlabs/rules) and [new-day](https://github.com/currantlabs/new-day) repos
**rulesc** contains the C implementation of the rules which are compiled into the
**new-day** executable. Within the **rulesc**  directory, these shared C source files are used to build a
library, **librules.a**, and cgo is used to call into the C implementation
of rules from within wrapper functions that **rulesc** then exports to our
**rules** Go package. For ease of sharing the C sources with the
**new-day** git repo on the one hand, and sharing the Go functionality with
**rules** on the other, the **rulesc** repo is designed to be used as a git
submodule that gets included in both the **new-day** and **rules** repos.

As new rules are introduced and ported to our hardware, the C
implementations of those rules will be added here, to **rulesc**. The
**new-day** implementation will pick up the new rules functionality by
adding new C source files to its master Makefile.





