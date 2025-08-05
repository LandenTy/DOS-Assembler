start:
    MOV AL, 0x0F
    AND AL, 0xF0
    OR AL, 0x01
    XOR AL, 0xFF
    NOT AL
    
    EXIT