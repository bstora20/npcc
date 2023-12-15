static void *run(void *targ)
{
    const uintptr_t threadNo = (uintptr_t)targ;
    uintptr_t x,y,i;
    uintptr_t clock = 0;

    /* Buffer used for execution output of candidate offspring */
    uintptr_t outputBuf[POND_DEPTH_SYSWORDS];

    /* Miscellaneous variables used in the loop */
    uintptr_t currentWord,wordPtr,shiftPtr,inst,tmp;
    struct Cell *pptr,*tmpptr;
    
    /* Virtual machine memory pointer register (which
     * exists in two parts... read the code below...) */
    uintptr_t ptr_wordPtr;
    uintptr_t ptr_shiftPtr;
    
    /* The main "register" */
    uintptr_t reg;
    
    /* Which way is the cell facing? */
    uintptr_t facing;

    /* Virtual machine loop/rep stack */
    uintptr_t loopStack_wordPtr[POND_DEPTH];
    uintptr_t loopStack_shiftPtr[POND_DEPTH];
    uintptr_t loopStackPtr;

    /* If this is nonzero, we're skipping to matching REP */
    /* It is incremented to track the depth of a nested set
     * of LOOP/REP pairs in false state. */
    uintptr_t falseLoopDepth;


    /* If this is nonzero, cell execution stops. This allows us
     * to avoid the ugly use of a goto to exit the loop. :) */
    int stop;
 /* Main loop */
    while (!exitNow) {
        /* Increment clock and run reports periodically */
        /* Clock is incremented at the start, so it starts at 1 */
        ++clock;
        exitNow = (clock == 1000000);

        if ((threadNo == 0)&&(!(clock % REPORT_FREQUENCY))) {
            doReport(clock);
        }

        /* Introduce a random cell somewhere with a given energy level */
        /* This is called seeding, and introduces both energy and
         * entropy into the substrate. This happens every INFLOW_FREQUENCY
         * clock ticks. */
        if (!(clock % INFLOW_FREQUENCY)) {
            x = getRandomRollback(1) % POND_SIZE_X;
            y = getRandomRollback(1) % POND_SIZE_Y;
            pptr = &pond[x][y];
            pptr->ID = cellIdCounter;
            pptr->parentID = 0;
            pptr->lineage = cellIdCounter;
            pptr->generation = 0;
#ifdef INFLOW_RATE_VARIATION
            pptr->energy += INFLOW_RATE_BASE + (getRandomRollback(1) % INFLOW_RATE_VARIATION);
#else
            pptr->energy += INFLOW_RATE_BASE;
#endif /* INFLOW_RATE_VARIATION */
            for(i=0;i<POND_DEPTH_SYSWORDS;++i)
                pptr->genome[i] = getRandomRollback(1);
            ++cellIdCounter;

        }

 /* Pick a random cell to execute */
        i = getRandomRollback(1);
        x = i % POND_SIZE_X;
        y = ((i / POND_SIZE_X) >> 1) % POND_SIZE_Y;
        pptr = &pond[x][y];

        /* Reset the state of the VM prior to execution */
        for(i=0;i<POND_DEPTH_SYSWORDS;++i)
            outputBuf[i] = ~((uintptr_t)0); /* ~0 == 0xfffff... */
        ptr_wordPtr = 0;
        ptr_shiftPtr = 0;
        reg = 0;
        loopStackPtr = 0;
        wordPtr = EXEC_START_WORD;
        shiftPtr = EXEC_START_BIT;
        facing = 0;
        falseLoopDepth = 0;
        stop = 0;
        int skip=0;
        int access_neg_used = 0;
        int access_pos_used = 0;

        /* We use a currentWord buffer to hold the word we're
         * currently working on.  This speeds things up a bit
         * since it eliminates a pointer dereference in the
         * inner loop. We have to be careful to refresh this
         * whenever it might have changed... take a look at
         * the code. :) */
        currentWord = pptr->genome[0];

        /* Keep track of how many cells have been executed */
        statCounters.cellExecutions += 1.0;

        /* Core execution loop */
        while ((pptr->energy)&&(!stop)) {
            /* Get the next instruction */
            inst = (currentWord >> shiftPtr) & 0xf;
            skip=0;
/* Randomly frob either the instruction or the register with a
             * probability defined by MUTATION_RATE. This introduces variation,
             * and since the variation is introduced into the state of the VM
             * it can have all manner of different effects on the end result of
             * replication: insertions, deletions, duplications of entire
             * ranges of the genome, etc. */


            if ((getRandomRollback(1) & 0xffffffff) < MUTATION_RATE) {
                tmp = getRandomRollback(1); // Call getRandom() only once for speed 
                if (tmp & 0x80) // Check for the 8th bit to get random boolean 
                    inst = tmp & 0xf; // Only the first four bits are used here 
                else reg = tmp & 0xf;
            }

            /* Each instruction processed costs one unit of energy */
            --pptr->energy;
            /* Execute the instruction */
            if (falseLoopDepth) {
                /* Skip forward to matching REP if we're in a false loop. */
                if (inst == 0x9) /* Increment false LOOP depth */
                    ++falseLoopDepth;
                else if (inst == 0xa) /* Decrement on REP */
                    --falseLoopDepth;
            } else {

                /* Keep track of execution frequencies for each instruction */
                statCounters.instructionExecutions[inst] += 1.0;
            }

            wordPtr=(wordPtr*((shiftPtr+4<SYSWORD_BITS)||(wordPtr+1<POND_DEPTH_SYSWORDS))
                +
                ((shiftPtr+4>=SYSWORD_BITS)&&(wordPtr+1<POND_DEPTH_SYSWORDS))
                +
                EXEC_START_WORD*((wordPtr+1>=POND_DEPTH_SYSWORDS)&&(shiftPtr+4>=SYSWORD_BITS)))*!skip + wordPtr*skip;

            currentWord=(currentWord*(shiftPtr+4<SYSWORD_BITS)
                +
                (pptr->genome[wordPtr])*(shiftPtr+4>=SYSWORD_BITS))*!skip
                + currentWord*skip;

            shiftPtr=((shiftPtr+4)
                    +
                    (shiftPtr+4>=SYSWORD_BITS)*(-shiftPtr-4))*!skip+shiftPtr*skip;
        }

        /* Copy outputBuf into neighbor if access is permitted and there
         * is energy there to make something happen. There is no need
         * to copy to a cell with no energy, since anything copied there
         * would never be executed and then would be replaced with random
         * junk eventually. See the seeding code in the main loop above. */
        if ((outputBuf[0] & 0xff) != 0xff) {
            tmpptr = getNeighbor(x,y,facing);

            //printf("%lu\n", tmpptr->energy);
            if ((tmpptr->energy)&&accessAllowed(tmpptr,reg,0,1)) {
                /* Log it if we're replacing a viable cell */
                if (tmpptr->generation > 2)
                    ++statCounters.viableCellsReplaced;

                tmpptr->ID = ++cellIdCounter;
                tmpptr->parentID = pptr->ID;
                tmpptr->lineage = pptr->lineage; /* Lineage is copied in offspring */
                tmpptr->generation = pptr->generation + 1;

                for(i=0;i<POND_DEPTH_SYSWORDS;++i)
                    tmpptr->genome[i] = outputBuf[i];
            }
        }
    }
    return (void *)0;
}
