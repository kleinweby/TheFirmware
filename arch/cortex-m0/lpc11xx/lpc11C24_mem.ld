MEMORY
{
  /* Define each memory region */
  MFlash16 (rx) : ORIGIN = 0x0, LENGTH = 0x8000 /* 32k */
  RamLoc8 (rwx) : ORIGIN = 0x100000b8, LENGTH = 0x1F48 /* ORIGIN = 0x10000000, LENGTH = 0x2000 /* 8k */

}
  /* Define a symbol for the top of each memory region */
  __top_MFlash16 = 0x0 + 0x8000;
  __top_RamLoc8 = 0x10000000 + 0x2000;
