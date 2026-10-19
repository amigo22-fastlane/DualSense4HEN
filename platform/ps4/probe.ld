/* SPDX-License-Identifier: GPL-3.0-only
 * Project linker layout for GoldHEN-style ET_DYN ELF, 16 KiB pages. */
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)
PHDRS {
  text PT_LOAD FLAGS(5);
  rodata PT_LOAD FLAGS(4);
  data PT_LOAD FLAGS(6);
  dynamic PT_DYNAMIC FLAGS(6);
}
SECTIONS {
  .text : ALIGN(0x4000) { PROVIDE_HIDDEN(__image_start = .); *(.text .text.*) } :text
  .rodata : ALIGN(0x4000) { *(.rodata .rodata.*) } :rodata
  .dynsym : { *(.dynsym) } :rodata
  .dynstr : { *(.dynstr) } :rodata
  .gnu.hash : { *(.gnu.hash) } :rodata
  .hash : { *(.hash) } :rodata
  .rela.dyn : { *(.rela.dyn) } :rodata
  .data : ALIGN(0x4000) { PROVIDE_HIDDEN(__image_rw_start = .); *(.data .data.*) } :data
  .data.rel.ro : { *(.data.rel.ro .data.rel.ro.*) } :data
  .got : { *(.got .got.*) } :data
  .dynamic : { *(.dynamic) } :data :dynamic
  .bss : { *(.bss .bss.*) *(COMMON) PROVIDE_HIDDEN(__image_end = .); } :data
  /DISCARD/ : { *(.comment) *(.eh_frame*) *(.note*) }
}
