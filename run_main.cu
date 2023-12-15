__global__ static void run(struct Cell *pond, uintptr_t *buffer, int *in, uint64_t *prngState) 
{
    //const uintptr_t threadNo = (uintptr_t)targ;
    uintptr_t x,y,i;
    uintptr_t clock = 0;
    uintptr_t outputBuf[POND_DEPTH_SYSWORDS];
    uintptr_t currentWord,wordPtr,shiftPtr,inst,tmp;
    struct Cell *pptr,*tmpptr;
    uintptr_t ptr_wordPtr;
    uintptr_t ptr_shiftPtr;
    uintptr_t reg;
    uintptr_t facing;
    uintptr_t loopStack_wordPtr[POND_DEPTH];
    uintptr_t loopStack_shiftPtr[POND_DEPTH];
    uintptr_t loopStackPtr;
    uintptr_t falseLoopDepth;
    int stop;
    int skip;
    int access_neg_used;
    int access_pos_used;
    uintptr_t access_neg;
    uintptr_t access_pos;
    uintptr_t rand;
    if (!(clock % INFLOW_FREQUENCY)) {
        getRandomRollback(1, &x, buffer, in, prngState);
        x = x % POND_SIZE_X;
        getRandomRollback(1, &y, buffer, in, prngState);
        y = y % POND_SIZE_Y;
        pptr = &pond[y * POND_SIZE_X + x]; 
        pptr->ID = cellIdCounter;
        pptr->parentID = 0;
        pptr->lineage = cellIdCounter;
        pptr->generation = 0;
#ifdef INFLOW_RATE_VARIATION
        getRandomRollback(1, &rand, buffer, in, prngState);
        pptr->energy += INFLOW_RATE_BASE + (rand % INFLOW_RATE_VARIATION);
#else
        pptr->energy += INFLOW_RATE_BASE;
#endif /* INFLOW_RATE_VARIATION */
for(i=0;i<POND_DEPTH_SYSWORDS;++i) 
            getRandomRollback(1, &rand, buffer, in, prngState);
            pptr->genome[i] = rand;
        ++cellIdCounter;
    }
    /* Pick a random cell to execute */
    getRandomRollback(1, &rand, buffer, in, prngState);
    //
    x = rand % POND_SIZE_X;
    y = ((rand / POND_SIZE_X) >> 1) % POND_SIZE_Y;
    pptr = &pond[y * POND_SIZE_X + x];
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
    skip=0;
    access_neg_used = 0;
    access_pos_used = 0;
    access_neg = 0;
    access_pos = 0;
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
        getRandomRollback(1, &rand, buffer, in, prngState);
        if ((rand & 0xffffffff) < MUTATION_RATE) {
            getRandomRollback(1, &rand, buffer, in, prngState);
            tmp = rand; // Call getRandom() only once for speed 
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
            statCounters.instructionExecutions[inst] += 1.0;
        }
    }
    /* Copy outputBuf into neighbor if access is permitted and there
        * is energy there to make something happen. There is no need
        * to copy to a cell with no energy, since anything copied there
        * would never be executed and then would be replaced with random
        * junk eventually. See the seeding code in the main loop above. */ 
 if ((outputBuf[0] & 0xff) != 0xff) {
        getNeighbor(pond,x,y,facing, tmpptr);
        //printf("%lu\n", tmpptr->energy);
        if ((tmpptr->energy)) {
            accessAllowed(tmpptr,reg,0,1, &rand, buffer, in, prngState);
            if(rand) {
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
}

