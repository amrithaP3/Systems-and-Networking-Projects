#include "types.h"
#include "pagesim.h"
#include "mmu.h"
#include "swapops.h"
#include "stats.h"
#include "util.h"

pfn_t select_victim_frame(void);

pfn_t last_evicted = 0;

/**
 * --------------------------------- PROBLEM 7 --------------------------------------
 * Checkout PDF section 7 for this problem
 *
 * Make a free frame for the system to use. You call the select_victim_frame() method
 * to identify an "available" frame in the system (already given). You will need to
 * check to see if this frame is already mapped in, and if it is, you need to evict it.
 *
 * @return victim_pfn: a phycial frame number to a free frame be used by other functions.
 *
 * HINTS:
 *      - When evicting pages, remember what you checked for to trigger page faults
 *      in mem_access
 *      - If the page table entry has been written to before, you will need to use
 *      swap_write() to save the contents to the swap queue.
 * ----------------------------------------------------------------------------------
 */
pfn_t free_frame(void)
{
    pfn_t victim_pfn;
    victim_pfn = select_victim_frame();

    // TODO: evict any mapped pages.
    if (frame_table[victim_pfn].mapped) {
        // Getting vpn
        vpn_t vpn = frame_table[victim_pfn].vpn;

        // Getting page table
        pte_t *page_table = (pte_t *) (mem + frame_table[victim_pfn].process->saved_ptbr * PAGE_SIZE);

        // Getting the page table entry
        pte_t *entry = &page_table[vpn];
        
        if (entry->dirty) {
            // Writing the data to swap space
            swap_write(entry, (mem + entry->pfn * PAGE_SIZE));

            // Incrementing the writebacks to disk
            stats.writebacks++;

            // Clearing the dirty bit
            entry->dirty = 0;
        }

        // Clearing bits of the page table entry
        entry->valid = 0;
        entry->referenced = 0;
    }
    // Clearing the frame table entry
    frame_table[victim_pfn].mapped = 0;
    frame_table[victim_pfn].process = NULL;

    // could delete these 2 lines
    frame_table[victim_pfn].vpn = 0;
    frame_table[victim_pfn].ref_count = 0;

    return victim_pfn;
}

/**
 * --------------------------------- PROBLEM 9 --------------------------------------
 * Checkout PDF section 7, 9, and 11 for this problem
 *
 * Finds a free physical frame. If none are available, uses either a
 * randomized, FCFS, or clocksweep algorithm to find a used frame for
 * eviction.
 *
 * @return The physical frame number of a victim frame.
 *
 * HINTS:
 *      - Use the global variables MEM_SIZE and PAGE_SIZE to calculate
 *      the number of entries in the frame table.
 *      - Use the global last_evicted to keep track of the pointer into the frame table
 * ----------------------------------------------------------------------------------
 */
pfn_t select_victim_frame()
{
    /* See if there are any free frames first */
    size_t num_entries = MEM_SIZE / PAGE_SIZE;
    for (size_t i = 0; i < num_entries; i++)
    {
        if (!frame_table[i].protected && !frame_table[i].mapped)
        {
            return i;
        }
    }

    // RANDOM implemented for you.
    if (replacement == RANDOM)
    {
        /* Play Russian Roulette to decide which frame to evict */
        pfn_t unprotected_found = NUM_FRAMES;
        for (pfn_t i = 0; i < num_entries; i++)
        {
            if (!frame_table[i].protected)
            {
                unprotected_found = i;
                if (prng_rand() % 2)
                {
                    return i;
                }
            }
        }
        /* If no victim found yet take the last unprotected frame
           seen */
        if (unprotected_found < NUM_FRAMES)
        {
            return unprotected_found;
        }
    }
    else if (replacement == APPROX_LRU) {
        // TODO: Implement the Approximate LRU algorithm here

        // Setting to max possible value of ref_count
        uint8_t smallest_ref_count = 255;
        pfn_t to_evict = -1;
        for (pfn_t i = 0; i < NUM_FRAMES; i++) {
            if (!frame_table[i].protected) {
                if (frame_table[i].ref_count < smallest_ref_count) {
                    smallest_ref_count = frame_table[i].ref_count;
                    to_evict = i;
                }
            }
        }
        return to_evict;
    }
    else if (replacement == CLOCKSWEEP) {
        // TODO: Implement the clocksweep page replacement algorithm here

        // Getting the frame number after the last evicted frame
        pfn_t curr = last_evicted;

        while(1) {
            // Getting the frame number after the last evicted frame (circular array)
            curr = (curr + 1) % NUM_FRAMES;

            // If the frame is not protected and its ref bit is not set, return it
            if (!frame_table[curr].protected) {
                // Getting page table
                pte_t *page_table = (pte_t *) (mem + frame_table[curr].process->saved_ptbr * PAGE_SIZE);

                // Getting the page table entry
                pte_t *entry =&page_table[frame_table[curr].vpn];

                if (!entry->referenced) {
                    last_evicted = curr;
                    return curr;
                }
                entry->referenced = 0;
            }
        }
    }

    /* If every frame is protected, give up. This should never happen
       on the traces we provide you. */
    panic("System ran out of memory\n");
    exit(1);
}

/**
 * --------------------------------- PROBLEM 10.2 --------------------------------------
 * Checkout PDF for this problem
 *
 * Updates the associated variables for the Approximate LRU,
 * called every time the simulator daemon wakes up.
 *
 * ----------------------------------------------------------------------------------
 */
void daemon_update(void) {
    pfn_t num_entries = MEM_SIZE / PAGE_SIZE;
    for (pfn_t i = 0; i < num_entries; i++) {
        if (frame_table[i].mapped && !frame_table[i].protected) {
            // Getting page table
            pte_t *page_table = (pte_t *) (mem + frame_table[i].process->saved_ptbr * PAGE_SIZE);

            // Getting the page table entry
            pte_t *entry = &page_table[frame_table[i].vpn];

            frame_table[i].ref_count = ((entry->referenced << 7) | (frame_table[i].ref_count >> 1));
            entry->referenced = 0;
        }
    }
}
