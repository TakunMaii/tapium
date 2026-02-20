# Tapium - A tape-based programming language inspired by brainf\*ck

## Quick Start

```bash
$ make
$ ./tapium test.tap
```

## Tutorials

Tapium runs on an infinite (not really) tape with a pointer at position 0 initially. Every slot on the tape stores a signed 64-bit integer. The position of the pointer can not be negative.

### basic instructions

`#`: comment till the end of the line

`+<num>`: plus the number at the pointer by `num`

`-<num>`: minus the number at the pointer by `num`

`><num>`: move the pointer right by `num`

`<<num>`: move the pointer left by `num`

Note that there should be no spaces between the instruction and the `<num>`

The `<num>` above can be a non-negative integer like `37` or a single char like `'M'`, which is optional, if which is not declared, it will be one as default. This is also true for any other `<num>` below.

`,`: read a number and store it at the pointer

`.`: write the number at the pointer as a char

`{`: jump to the next instruction of the pair `}` if the number at the pointer is not zero

`}`: jump to the next instruction of the pair `{` if the number at the pointer is zero

### include

`%<include-path>`: read and interprete the `<include-path>` file

### macros

`@<name>`: start to define a macro named `<name>`, noting that there should be no spaces between `@` and the first letter of the macro name

`;`: end to define the macro, if it's not defining a macro, end the program instead

`<name>`: call the macro named `<name>`

The `<name>` should be a string of a-z, A-Z or _.

Macro definitions will be repalced if any new macro with the same name is defined.

Calling macros can be nested.

### tuples

`(`: start to define a tuple

`)<num>`: end to define a tuple

The tuple will be run for `<num>` times.

The tuples can be nested.
