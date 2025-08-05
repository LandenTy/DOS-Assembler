start:
    MOV AL, 5
print_loop:
    PRINT "Looping..."
    JMP print_loop

exit_label:
    PRINT "Done!"
    EXIT
