if exists('b:current_syntax')
	finish
endif

syntax clear
syntax sync fromstart

syntax match blarbNumber /\d/
syntax match blarbComment /;.*/
syntax match blarbCommand /[\^$\~!?]/
syntax match blarbFunctionDefinition /^#[a-zA-Z]*/

hi default link blarbComment Comment
hi default link blarbCommand Statement
hi default link blarbFunctionDefinition Function
hi default link blarbNumber Number

let b:current_syntax = 'blarb'

