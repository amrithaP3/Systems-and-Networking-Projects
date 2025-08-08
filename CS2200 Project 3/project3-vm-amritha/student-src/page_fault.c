#include "mmu.h"
#include "pagesim.h"
#include "swapops.h"
#include "stats.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/**
 * --------------------------------- PROBLEM 6 --------------------------------------
 * Checkout PDF section 7 for this problem
 *
 * Page fault handler.
 *
 * When the CPU encounters an invalid address mapping in a page table, it invokes the
 * OS via this handler. Your job is to put a mapping in place so that the translation
 * can succeed.
 *
 * @param addr virtual address in the page that needs to be mapped into main memory.
 *
 * HINTS:
 *      - You will need to use the global variable current_process when
 *      altering the frame table entry.
 *      - Use swap_exists() and swap_read() to update the data in the
 *      frame as it is mapped in.
 * ----------------------------------------------------------------------------------
 */
void page_fault(vaddr_t addr)
{
   // TODO: Get a new frame, then correctly update the page table and frame table

   // Getting VPN from faulting virtual address
   vpn_t vpn = vaddr_vpn(addr);

   // Getting page table
   pte_t *page_table = (pte_t *) (mem + PTBR * PAGE_SIZE);

   // Getting the page table entry
   pte_t *entry = &page_table[vpn];

   // Getting PFN of a new frame
   pfn_t new_frame = free_frame();

   if (swap_exists(entry)) {
      // Reading the data from saved frame to new frame
      swap_read(entry, (mem + new_frame * PAGE_SIZE));
   }
   else {
      // Clearing the new frame
      memset((mem + new_frame * PAGE_SIZE), 0, PAGE_SIZE);
   }

   // Updating the page table entry
   entry->pfn = new_frame;
   entry->valid = 1;
   entry->dirty = 0;

   // Updating the frame table entry
   frame_table[new_frame].protected = 0;
   frame_table[new_frame].mapped = 1;
   frame_table[new_frame].process = current_process;
   frame_table[new_frame].vpn = vpn;
   frame_table[new_frame].ref_count = 0;

   // Incrementing the page fault count
   stats.page_faults++;
}

#pragma GCC diagnostic pop
