ENTRY(_entry)

SECTIONS {
	. = 0x00000500;

	.text : {
		KEEP(*(.entry))
		KEEP(*(.text))
	}

	.rodata : {
		KEEP(*(.rodata))
	}

	.data : {
        START_CONSTRUCTORS_INITARR = .;
        KEEP(*(.init_array));
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)));
        END_CONSTRUCTORS_INITARR = .;
        START_CONSTRUCTORS_CTORS = .;
        KEEP(*(.ctors));
        KEEP(*(SORT_BY_INIT_PRIORITY(.ctors.*)));
        END_CONSTRUCTORS_CTORS = .;
        KEEP(*(.data))
	}

	.bss : {
		_bss_start = ABSOLUTE(.);
		KEEP(*(.bss))
		_bss_end = ABSOLUTE(.);
	}
}
