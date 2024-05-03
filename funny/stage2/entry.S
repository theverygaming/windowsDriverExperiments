.section .entry
.global _entry
_entry:
    call _c_entry
    .a:
    jmp .a
