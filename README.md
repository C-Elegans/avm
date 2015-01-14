# AVM

AVM is a personal project created with the goal of learning C. It includes a
parser for the simple language it runs, a pretty-printer, and easy debugging
facilities.

```
push F
push F
mul
call
quit

e1:
  push 1
  push 2
  jmpez FF
  ret
```

[Tup][tupsite] is used to build the project because Make is too weird. Building
is easy: just run `tup` in the main directory. The only dependencies are a
recent gcc or clang. It probably works best with clang, I can't be bothered to
setup `ifdef`s for every warning suppression.

[tupsite]: http://gittup.org/tup/

## Documentation
AVM is a stack machine, and it provides two ways of placing stuff on the stack:
the `push` and `load` instruction.

`push` is a two word instruction; the first word is the actual instruction and
the next word is the value. At this point it can only push one word at a time.

`load` grabs the data from the given memory address and pushes it onto the
stack in reverse order. This can probably be used cleverly with some `error`
instructions and jumps to place more than one word on the stack.

    ┌─┬─┬─┬─┐
    │1│2│3│4│
    └─┴─┴─┴─┘
        ↓
       ┌─┐
       │1│
       ├─┤
       │2│
       ├─┤
       │3│
       ├─┤
       │4│
       ├─┤
       │⋮│
       └─┘

`add`, `sub`, `mul`, `and`, `or`, `xor`, `shr`, and `shl` are nearly typical
stack machine instructions. They are something like `a = pop(); b = pop(); b OP
a`, where the second pop is the lhs. This tends to be conceptually simpler and
is more useful.

`div` is a bit different, it divides by `1` if the rhs is `0`.

The machine has a second stack for function calls so that stack traces are
easy; `call` and `calli` place addresses on this stack. `call` unconditionally
goes to the address at the top of the stack, while `calli` takes its target
address as an immediate value. `ret` pops an element off the stack and goes
back to the point at which the function was called. The stack isn't cleared,
this can be used for result passing.

`jmpez` jumps to the immediate address if the element `pop`'d off the stack
*e*quals *z*ero.

`quit` pops an element off the stack and returns it to the outside calling
program.

`dup` duplicates the top element of the stack.

The layout of an operation is stable and can be relied upon. It is as follows:

    AVM_Opcode kind : 8;
    uint size : 24;
    avm_size_t address : 32;

The C standard decrees that the bit layout is undefined, but most compilers
don't try to create needless problems, so it should be fine.

Since any memory address can be executed, it's possible to perform runtime code
generation with bitshifts and the like.

## Bugs

- The parser may be buggy, I dunno.
- The VM relies on virtual memory, malicious programs might be able to send the
  OOM killer after you.
- I've run some fuzz testing, but there still may be bugs in the VM.
