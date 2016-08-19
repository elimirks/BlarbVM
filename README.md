BlarbVM
=======

Currently supported operations:
- a b ~ Store the value of a in register b
- a $ Push register a onto the stack
- b a ! NANDS index a with index b and stores the result in index b
- a ^ pop 'a' elements
- a ? if index a is true (non-zero), execute the rest of the line

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

The stack stores a collection of words (32 bits each)

