MOVC R0,#4000
MOVC R1,#1
MOVC R2,#0
MOVC R3,#3
MOVC R4,#1
MOVC R5,#2
MOVC R6,#3
MOVC R7,#4
ADD R8,R1,R2
SUB R9,R1,R2
ADDL R10,R1,#10
SUBL R11,R1,#10
AND R12,R1,R2
OR R13,R1,R2
EX-OR R14,R1,R2
MUL R15,R1,R3
STORE R3,R1,#1
LOAD R8,R1,#1
STR R5,R1,R1
LDR R9,R1,R1
SUB R1,R5,R1
MOVC R5,#1
CMP R1,R2
BNZ  #-16
MOVC R2,#0
MOVC R3,#3
HALT
