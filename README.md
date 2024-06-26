# Meteorite

This is a very simple assembler that targets the Supernova ISA

### Syntax

To single line comments, just add a `;` to comment until the end of the line:

```meteor
; like so
```

To create labels in a line, write their name inside square brackets and 
a semi colon afterwards, followed by a comment or a newline:

```meteor
[label_1]:
```

*In theory* spaces are allowed but better not try.

All instructions will fall in one of the following categories for the instruction """parser""":

* R instructions, which are in the form `instr r* r* r*`, where `*` represents any number of a register
* S instructions with hex immediate, in the form `instr r* r* #h` where `*` represents register numbers and `h` a valid hex number
* S instructions with decimal immediate, in the form `instr r* r* $d` where `*` represents register numbers and `d` a valid decimal number
* S instructions with label, in the form `instr r* r* [l]` where `*` represents register numbers and `l` a valid label name
* L instructions with hex immediate, in the form `instr r* #h` where `*` represents register numbers and `h` a valid hex number
* L instructions with decimal immediate, in the form `instr r* $d` where `*` represents register numbers and `d` a valid decimal number
* L instructions with label, in the form `instr r* [l]` where `*` represents register numbers and `l` a valid label name

That is basically it for now, data operations and multiple sections is a future for next releases
