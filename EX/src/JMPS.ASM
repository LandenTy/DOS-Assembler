start:
    MOV AL, 5
    MOV BL, 5
    CMP AL, BL
    JE equal
    MOV AL, 0
    JMP end
equal:
    MOV AL, 1
end:
    ; exit
    EXIT
