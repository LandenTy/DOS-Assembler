start:
    MOV AL, 42
    CALL myfunc
    MOV BL, AL
    JMP done

myfunc:
    INC AL
    RET

done:
    EXIT
