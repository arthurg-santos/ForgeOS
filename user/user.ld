ENTRY(_start)

SECTIONS
{
    /* Acima de 1 GiB: fora da região de huge pages do kernel,
       permitindo mapeamentos privados de 4KiB sem colisão. */
    . = 0x40000000;

    .text : ALIGN(4K)
    {
        *(.text*)
        *(.rodata*)
    }

    .data : ALIGN(4K)
    {
        *(.data*)
    }

    .bss : ALIGN(4K)
    {
        *(.bss*)
        *(COMMON)
    }
}
