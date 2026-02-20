# Tapium - A tape-based programming language inspired by brainf\*ck

## Quick Start

```bash
$ make
$ ./tapium test.tap
```

## Tutorials

Tapium runs on an infinite (not really) tape with a pointer at position 0 initially. Every slot on the tape stores a signed 64-bit integer.

`#`: comment till the end of the line

`+`: plus the number at the pointer by one

`-`: minus the number at the pointer by one

`>`: move the pointer right by one

`<`: move the pointer left by one

`,`: read a number and store it at the pointer

`.`: write the number at the pointer

`[`: jump to the next instruction of the pair `]` if the number at the pointer is not zero

`]`: jump to the next instruction of the pair `[` if the number at the pointer is zero
