OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

SECTIONS
{
	. = 0x100000;

	.text :
	{
		*(.text)
	}
	
	.data ALIGN (4096) :
	{
		*(.data)
		*(.rodata*)
	}
	
	.bss ALIGN (4096) :
	{
		*(COMMON*)
		*(.bss*)
	}
}
