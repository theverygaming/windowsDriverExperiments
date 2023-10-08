bits 32
mov ecx, 0x69420

mov al, 'O'
out 0xE9, al
mov al, 'K'
out 0xE9, al

jmp edi
