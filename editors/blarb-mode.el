(require 'generic-x)

(define-generic-mode 'blarb-mode
  '(";")                            ;; comments start with ';'
  nil
  '(("#[a-zA-Z_][a-zA-Z0-9_]*" . 'font-lock-function-name-face)
    ("[0-9]+" . font-lock-constant-face))
  '("\\.blarb$")                      ;; files for which to activate this mode 
  nil                               ;; other functions to call
  "A mode for blarb files"          ;; doc string for this mode
  )

(provide 'blarb-mode)
