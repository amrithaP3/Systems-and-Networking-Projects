#include "mmu.h"
#include "pagesim.h"
#include "va_splitting.h"
#include "swapops.h"
#include "stats.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/* The frame table pointer. You will set this up in system_init. */
fte_t *frame_table;

/**
 * --------------------------------- PROBLEM 2 --------------------------------------
 * Checkout PDF sections 4 for this problem
 *
 * In this problem, you will initialize the frame_table pointer. The frame table will
 * be located at physical address 0 in our simulated memory. You should zero out the
 * entries in the frame table, in case for any reason physical memory is not clean.
 *
 * HINTS:
 *      - mem: Simulated physical memory already allocated for you.
 *      - PAGE_SIZE: The size of one page
 * ----------------------------------------------------------------------------------
 */
void system_init(void)
{
    // TODO: initialize the frame_table pointer.
    frame_table = (fte_t *)mem;

    // Zero out the frame table (num frames in physical mem = frame table size = NUM_FRAMES * sizeof(fte_t))
    memset(frame_table, 0, NUM_FRAMES * sizeof(fte_t));

    // Marking the first frame as protected to never evict frame table
    frame_table->protected = 1;
}

/**
 * --------------------------------- PROBLEM 5 --------------------------------------
 * Checkout PDF section 6 for this problem
 *
 * Takes an input virtual address and performs a memory operation.
 *
 * @param addr virtual address to be translated
 * @param access 'r' if the access is a read, 'w' if a write
 * @param data If the access is a write, one byte of data to written to our memory.
 *             Otherwise NULL for read accesses.
 *
 * HINTS:
 *      - Remember that not all the entry in the process's page table are mapped in.
 *      Check what in the pte_t struct signals that the entry is mapped in memory.
 * ----------------------------------------------------------------------------------
 */
uint8_t mem_access(vaddr_t addr, char access, uint8_t data)
{
    // TODO: translate virtual address to physical, then perform the specified operation

    // Getting VPN from virtual address
    vpn_t vpn = vaddr_vpn(addr);

    // Getting page table
    pte_t *page_table = (pte_t *) (mem + PTBR * PAGE_SIZE);

    // Getting the page table entry
    pte_t *entry = &page_table[vpn];

    /* Either read or write the data to the physical address
       depending on 'rw' */
    if (entry->valid == 0) {
        page_fault(addr);
    }

    // Incrementing the number of memory accesses
    stats.accesses++;

    // Getting the pfn and offset
    pfn_t pfn = entry->pfn;
    uint16_t offset = vaddr_offset(addr);

    // Updating the referenced and mapped bits
    entry->referenced = 1;

    // Don't need this bc handled in page fault
    //frame_table[pfn].mapped = 1;

    if (access == 'r') {
        // Getting the data from the physical address
        uint8_t info = mem[pfn * PAGE_SIZE + offset];
        
        // Returning data from physical address
        return info;
    } else if (access == 'w') {
        // Writing the data to the physical address
        mem[pfn * PAGE_SIZE + offset] = data;

        // Setting the dirty bit since the page has been written to
        entry->dirty = 1;

        return data;
    }

    // Returning 0 if the access is invalid
    return 0;
}