BlarbVM
=======

Currently supported operations:
- a b ~ Store the value of a in register b
- a $ Push register a onto the stack
- b a ! NANDS index a with index b and stores the result in index b
- a ^ pop 'a' elements
- a ? if index a is true (non-zero), execute the rest of the line
- "filename.blarb" @ Include the given file 
- f e d c b a % Execute the system call with the given args
- a b = Set the memory address at index b to the byte (not word) at index a

Labels are created by:

```
#labelname (optional comments - after the space everything is ignored)
```
and called by simply writing their name.

e.g.
```
3 copyi

#copyi Copy immediate
	; ... do stuff
```

To test, run "./blarb test/lib.blarb"

The stack stores a collection of 64-bit words

To include a file, you must push a null terminated string onto the stack.
Or, you can use a string literal, which will automatically push a
"backwards", null-terminated sequence of characters onto the stack.
To escape a quotation, use "\"". To escape backslashes, use "\\"

Loop Example
------------

This code is using functions defined in lib.blarb.

```
"lib.blarb" @ ; Include the library, which has nice functions (as used below)

0            ; Start with the number 0 on the stack
1 addi       ; Increment it
1 copy       ; Copy it twice (one for comparison, one for the list)
1 copy       ;
5 iseqi      ; Check if the number is 5
1 ? 2 ^ exit ; If it is 5 (1 ? means check the 1st stack index), exit
1 ^ -6 jumpi ; Otherwise, jump 6 lines back and continue execution (a loop)

exit
```

The result of the above code is a list of numbers 1-5 on the stack.

