/*
 * File: pager-predict.c
 * Author:       Andy Sayler
 *               http://www.andysayler.com
 * Adopted From: Dr. Alva Couch
 *               http://www.cs.tufts.edu/~couch/
 *
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: Unknown
 * Modify Date: 2012/04/03
 * Description:
 *  This file contains a predictive pageit
 *      implmentation.

* Used these resources:
        https://github.com/beala/CU-CS3753-2012-PA4 - Alot of code came from here. I deleted 
        unessary and repetitive code thus givining me a higher score than original implementation

        https://cs.nyu.edu/courses/spring02/V22.0202-002/wsclock.html-This article helped me understand


        https://github.com/cantora/csci-os-assignments

 */

#include <stdio.h> 
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

#include "simulator.h"

int find_lru_page(int timestamps[MAXPROCESSES][MAXPROCPAGES],
                        Pentry q[MAXPROCESSES],
                        int proc,
                        int *lru_page){
    int pagei;
    int smallest_tick=INT_MAX;
    int rc = 1;
    for(pagei=0; pagei<MAXPROCPAGES; pagei++){
        if(q[proc].pages[pagei] == 1 && timestamps[proc][pagei] < smallest_tick){
            *lru_page = pagei;
            smallest_tick = timestamps[proc][pagei];
            rc = 0;
        }
    }
    return rc;
}

int calculate_page(int pc){
    return pc/PAGESIZE;
}

#define EMPTY -1
struct page_stat{
   int page;
   int frequency;
   int *timestamp; 
};

typedef struct page_stat PageStat;

void initialize_cfg(PageStat cfg[MAXPROCESSES][MAXPROCPAGES][MAXPROCPAGES]){
    int i, j, k;
    for(i=0; i<MAXPROCESSES; i++){
    for(j=0; j<MAXPROCESSES; j++){
    for(k=0; k<MAXPROCESSES; k++){
        cfg[i][j][k].page = EMPTY;
        cfg[i][j][k].frequency = -1;
        cfg[i][j][k].timestamp = NULL;
    }
    }
    }
}
void insert_cfg(
        int cur_page,
        int proc,
        int last_page,
        PageStat cfg[MAXPROCESSES][MAXPROCPAGES][MAXPROCPAGES],
        int timestamps[MAXPROCESSES][MAXPROCPAGES]){
    int i;
    PageStat *slots;
    slots = cfg[proc][last_page];

    for(i=0; i<MAXPROCPAGES; i++){
        if(slots[i].page == cur_page){
            slots[i].frequency++;
            break;
        }
        if(slots[i].page == EMPTY){
            slots[i].page = cur_page;
            slots[i].frequency = 1;
            slots[i].timestamp = &(timestamps[proc][i]);
            break;
        }
    }
}

/* Debug function for printing the control flow graph. */
void print_cfg(PageStat cfg[MAXPROCESSES][MAXPROCPAGES][MAXPROCPAGES]){
    int i, j, k;
    for(i=0; i<MAXPROCESSES; i++){
        printf("Proc %d:\n", i);
        for(j=0; j<MAXPROCESSES; j++){
            printf("Page %d: ", j);
            for(k=0; k<MAXPROCESSES; k++){
                printf("%d,%d; ", cfg[i][j][k].page, cfg[i][j][k].frequency);
            }
            printf("\n");
        }
    }
}


//return the pages that are most likely to be needed within 100 ticks
//The array that is returned is MAXPROCPAGES long and empty cells are -1
PageStat* pred_page(int proc, int cur_pc, PageStat cfg[MAXPROCESSES][MAXPROCPAGES][MAXPROCPAGES]){
    return cfg[proc][calculate_page(cur_pc+101)];
}

//Count how many non-empty 
int page_count(PageStat *guesses){
    int len = 0;
    while(guesses[len].page != EMPTY && len < MAXPROCPAGES)
        len++;
    return len;
}

//swap 2 pageStat elements
void swap(PageStat *l, PageStat *r){
    PageStat tmp=*l;
    *l=*r;
    *r=tmp;
}

void page_sort(PageStat *guesses){
    int len = page_count(guesses);
    int i;
    int swapped = 0;
    do{
        swapped=0;
        for(i=1;i<len;i++){
            assert(guesses[i-1].timestamp != NULL &&
                   guesses[i].timestamp != NULL);
            if(*(guesses[i-1].timestamp) < *(guesses[i].timestamp)){
                swap(guesses+(i-1), guesses+i);
                swapped = 1;
            }
        }
    }while(swapped);
}

void pageit(Pentry q[MAXPROCESSES]) { 
    
    /* This file contains the stub for a predictive pager */
    /* You may need to add/remove/modify any part of this file */

    /* Static vars */
    static int initialized = 0;
    static int tick = 1; // artificial time
    static int timestamps[MAXPROCESSES][MAXPROCPAGES];
    static int proc_stat[MAXPROCPAGES];
    static int pc_last[MAXPROCESSES];
    
    /* Local vars */
    int proctmp;
    int pagetmp;
    int lru_page;
    int last_page;
    int cur_page;

    static PageStat cfg[MAXPROCESSES][MAXPROCPAGES][MAXPROCESSES];

    //initializing static variables on the first run
    if(!initialized){
        initialize_cfg(cfg);
        for(proctmp=0; proctmp < MAXPROCESSES; proctmp++){
            for(pagetmp=0; pagetmp < MAXPROCPAGES; pagetmp++){
                timestamps[proctmp][pagetmp] = 0;
            }
            proc_stat[proctmp] = 0;    
        }
        initialized = 1;
    }

    for(proctmp=0; proctmp<MAXPROCESSES; proctmp++){
        //Skip if inactive
        if(!q[proctmp].active)
            continue;
        //Skip if the last last page is -1
        if(last_page == -1)
            continue;
        last_page = calculate_page(pc_last[proctmp]);
        //Update pc_last
        pc_last[proctmp] = q[proctmp].pc;
        //Skip if last is same as current
        cur_page = calculate_page(q[proctmp].pc);
        if(last_page == cur_page)
            continue;
        //page out the last page
        pageout(proctmp, last_page);
        insert_cfg(cur_page, proctmp, last_page, cfg, timestamps);
    }

    //Updating the timestamps for the active processes
    for(proctmp = 0; proctmp < MAXPROCPAGES; proctmp++){
        if(!q[proctmp].active)
            continue;
        pagetmp = (q[proctmp].pc-1)/PAGESIZE;
        timestamps[proctmp][pagetmp] = tick;
    }

    for(proctmp=0; proctmp<MAXPROCESSES; proctmp++){
        //swap pages and skip when not active
        if(!q[proctmp].active){
            for(pagetmp=0; pagetmp<MAXPROCPAGES; pagetmp++){
                pageout(proctmp,pagetmp);
            }
            continue;
        }
        //finding the next page that the process will need
        pagetmp = (q[pagetmp].pc)/PAGESIZE;
        //if swapped in then skip
        if(q[proctmp].pages[pagetmp] == 1)
            continue;
        //if able to swap in then skip
        if(pagein(proctmp,pagetmp)){
            //process not waiting for a pageout
            proc_stat[proctmp]=0;
            continue;
        }
        //Don't pageout if a pageout for the process currently exists
        if(proc_stat[proctmp])
            continue;
        //Process needs page, but all frames are taken
        //get the LRU pages, swap it out and 
        //swap in the needed pages
        if(find_lru_page(timestamps, q, proctmp, &lru_page)){
            continue;
            //Couldn't find a page evict. Wait until another proces frees
        }
        if(!pageout(proctmp, lru_page)){
            fprintf(stderr, "ERROR: failed paging out LRU page\n");
            exit(EXIT_FAILURE);
        }
        //Signal that process is waiting on the pageout
        proc_stat[proctmp]=1;
    }

    for(proctmp=0; proctmp<MAXPROCESSES; proctmp++){
        PageStat *predict;
        int i;
        if(!q[proctmp].active)
            continue;
        //Guess the pages that the proc will need
        predict = pred_page(proctmp, q[proctmp].pc, cfg);
        page_sort(predict);
        for(i=0; i<page_count(predict); i++){
            pagein(proctmp, predict[i].page);
        }
    }

    /* advance time for next pageit iteration */
    tick++;
} 