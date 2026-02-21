# Tapium - A tape-based programming language inspired by brainf\*ck

## Quick Start

```bash
$ make
$ ./tapium examples/hello_world.tap
```

## Tutorials

Tapium runs on an infinite (not really) tape with a pointer at position 0 initially. Every slot on the tape stores a signed 64-bit integer. The position of the pointer can not be negative.

### basic instructions

`#`: comment till the end of the line

`+<num>`: add the number at the pointer by `<num>`

`-<num>`: substract the number at the pointer by `<num>`

`><num>`: move the pointer right by `<num>`

`<<num>`: move the pointer left by `<num>`

Note that there should be no spaces between the instruction and the `<num>`

The `<num>` above can be a non-negative integer like `37` or a single char like `'M'`, which is optional, if which is not declared, it will be one as default. This is also true for any other `<num>` below.

`,<type>`: read a value and store it on the tape at the pointer

`.<type>`: output the value one the tape at the pointer

The `<type>` above can be `#` for integers, `%` for strings or not given for chars

`.%`: write the number at the pointer as a string

`{`: jump to the next instruction of the pair `}` if the number at the pointer is not zero

`}`: jump to the next instruction of the pair `{` if the number at the pointer is zero

### strings

Strings are stored in the memory whose pointer will be recoreded on the tape.

`"<string-content>"`: apply a new space in the memory, put the string content in it and then recored the pointer to that memory on the tape at the pointer

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

### embedded functions

Embedded functions are functions defined in C and can be used in Tapium. Here is the list of them:

+ `rand`: get a random integer and store it at the pointer
+ `add`: add the integer at this slot by the integer at the next right slot
+ `sub`: substract the integer at this slot by the integer at the next right slot
+ `mul`: multiply the integer at this slot by the integer at the next right slot
+ `div`: divide the integer at this slot by the integer at the next right slot
