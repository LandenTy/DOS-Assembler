start:
    MOV AL, 10  
    MOV BL, 3     

    SUB AL, BL     

    INC AL      
    DEC AL       

    CMP AL, BL    

    MOV AH, 0      
    MOV AL, 9      
    MOV BL, 3      
    DIV BL         

    ; Done
    EXIT