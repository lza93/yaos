OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)

MEMORY
{
	ccm  (rwx)	: ORIGIN = 0x10000000, LENGTH = 64K
	ram  (rwx)	: ORIGIN = 0x20000000, LENGTH = 128K
	rom  (rx)	: ORIGIN = 0x08000000, LENGTH = 512K
}

PROVIDE(_rom_start   = ORIGIN(rom));
PROVIDE(_rom_size    = LENGTH(rom));
PROVIDE(_ram_start   = ORIGIN(ram));
PROVIDE(_ram_size    = LENGTH(ram));
PROVIDE(_ram_end     = ORIGIN(ram) + LENGTH(ram));
PROVIDE(_vector_size = 0x200); /* The minimum alignment is 128 words. */

resched = sys_schedule;
systick = systick64;
SECTIONS
{
	.text :
	{
		*(.vector)

		*(.text.init)

		. = (. + 3) & ~ 3;
		_init_func_list = .;
		*(.text.init.0)
		*(.text.init.1)
		*(.text.init.2)
		*(.text.init.*)
		LONG(0); /* null sentinel */

		*(.text)
		*(.text.*)

		. = ALIGN(4);
		*(.rodata)
		*(.rodata.*)

		. = ALIGN(4);
		_driver_list = .;
		*(.driver_list)
		LONG(0);
		. = ALIGN(4);
		_device_list = .;
		*(.device_list)
		LONG(0);

		_shell_cmdlist = .;
		*(.shell_cmdlist)
		LONG(0);

		. = ALIGN(4);
		_etext = .;
	} > rom

	.data _ram_start + _vector_size :
	{
		. = ALIGN(4);
		_data = .;

		*(.data)
		*(.iap)

		. = ALIGN(4);
		_user_task_list = .;
		*(.user_task_list)
		LONG(0);

		. = ALIGN(4);
		_edata = .;
	} > ram AT > rom

	.bss :
	{
		. = ALIGN(4);
		_bss = .;

		*(.bss)
		*(COMMON)

		. = ALIGN(4);
		_ebss = .;
	} > ram AT > rom
}
