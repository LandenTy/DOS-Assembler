start:
    MOV AL, 0x0F      ; AL = 00001111b
    MOV BL, 1         ; BL = 1 (for DIV test later maybe)

    ; Shift left AL by 1 (AL <<= 1)
    SHL AL, 1

    PRINT "After SHL AL, 1: "
    MOV AH, 0
    CALL PRINT_HEX_AL

    ; Shift right AL by 2 (AL >>= 2)
    SHR AL, 2

    PRINT " After SHR AL, 2: "
    CALL PRINT_HEX_AL

    ; Rotate left AL by 1 (ROL AL, 1)
    ROL AL, 1

    PRINT " After ROL AL, 1: "
    CALL PRINT_HEX_AL

    ; Rotate right AL by 1 (ROR AL, 1)
    ROR AL, 1

    PRINT " After ROR AL, 1: "
    CALL PRINT_HEX_AL

    ; Exit program
    EXIT

; ------------------------------
; PRINT_HEX_AL: Print AL in hex
; ------------------------------
PRINT_HEX_AL:
    PUSH AX
    MOV AH, 2

    MOV DL, AL
    SHR DL, 4
    CALL PRINT_HEX_DIGIT

    MOV DL, AL
    AND DL, 0Fh
    CALL PRINT_HEX_DIGIT

    POP AX
    RET

; ------------------------------
; PRINT_HEX_DIGIT: Print hex digit in DL
; ------------------------------
PRINT_HEX_DIGIT:
    CMP DL, 9
    JG  .letter
    ADD DL, '0'
    JMP .print
.letter:
    ADD DL, 'A' - 10
.print:
    INT 21h

    RET
