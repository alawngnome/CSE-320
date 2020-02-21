#include "const.h"
#include "sequitur.h"

/*
 * Digram hash table.
 *
 * Maps pairs of symbol values to first symbol of digram.
 * Uses open addressing with linear probing.
 * See, e.g. https://en.wikipedia.org/wiki/Open_addressing
 */

/**
 * Clear the digram hash table.
 */
void init_digram_hash(void) {
    for(int i = 0; i < MAX_DIGRAMS; i++){
        *(digram_table + i) = NULL;
    }
    // To be implemented.
}

/**
 * Look up a digram in the hash table.
 *
 * @param v1  The symbol value of the first symbol of the digram.
 * @param v2  The symbol value of the second symbol of the digram.
 * @return  A pointer to a matching digram (i.e. one having the same two
 * symbol values) in the hash table, if there is one, otherwise NULL.
 */
SYMBOL *digram_get(int v1, int v2) {

    int position = DIGRAM_HASH(v1, v2);

    int i = position; //copy to iterate and check against original
    SYMBOL *digram = *(digram_table+i);

    if(digram == NULL /*|| digram->next == NULL*/) {
        debug("Digram not found :(\n");
        return NULL;
    }


    if(digram != TOMBSTONE && digram->value == v1 && digram->next->value == v2) {
            debug("Digram found at index %d\n", i);
            return digram;
    }

    i++; //if not the first value, start loop from position + 1

    while(i <= MAX_DIGRAMS){ //check starting from position + 1
        if(i == MAX_DIGRAMS)
            i = 0;
        else if(i == position)
            return NULL;

        SYMBOL *digram = *(digram_table+i);

        if (digram == TOMBSTONE) {
            //continue;
        }

        else if(digram == NULL || digram->next == NULL) {
            debug("Digram not found\n");
            return NULL;
        }

        else if(digram->value == v1 && digram->next->value == v2) {
            debug("Digram found at index %d\n", i);
            return digram;
        }

        i++;
    }

    return NULL;
}

/**
 * Delete a specified digram from the hash table.
 *
 * @param digram  The digram to be deleted.
 * @return 0 if the digram was found and deleted, -1 if the digram did
 * not exist in the table.
 *
 * Note that deletion in an open-addressed hash table requires that a
 * special "tombstone" value be left as a replacement for the value being
 * deleted.  Tombstones are treated as vacant for the purposes of insertion,
 * but as filled for the purpose of lookups.
 *
 * Note also that this function will only delete the specific digram that is
 * passed as the argument, not some other matching digram that happens
 * to be in the table.  The reason for this is that if we were to delete
 * some other digram, then somebody would have to be responsible for
 * recycling the symbols contained in it, and we do not have the information
 * at this point that would allow us to be able to decide whether it makes
 * sense to do it here.
 */
int digram_delete(SYMBOL *digram) {

    int position = DIGRAM_HASH(digram->value, digram->next->value);

    int i = position; //copy to iterate and check against original

    SYMBOL *secondDigram = *(digram_table+i);

    if(secondDigram == NULL) {
        debug("Digram %lu not found while deleting", SYMBOL_INDEX(digram));
        return -1;
    }

    if(digram == secondDigram) {
        *(digram_table + i) = TOMBSTONE;
        return 0;
    }


    i++;


    while(i <= MAX_DIGRAMS){ //check starting from position + 1
        if(i == MAX_DIGRAMS)
            i = 0;
        else if(i == position)
            return -1;

        secondDigram = *(digram_table+i);

        if (secondDigram == TOMBSTONE)
        {
            //continue;
        }

        else if(secondDigram == NULL) {
            return -1;
        }

        else if(digram == secondDigram) {  //if matching digrams
            *(digram_table + i) = TOMBSTONE;
            debug("Digram deleted at index %d", i);
            return 0;
        }

        i++;
    }

    return -1;
}

/**
 * Attempt to insert a digram into the hash table.
 *
 * @param digram  The digram to be inserted.
 * @return  0 in case the digram did not previously exist in the table and
 * insertion was successful, 1 if a matching digram already existed in the
 * table and no change was made, and -1 in case of an error, such as the hash
 * table being full or the given digram not being well-formed.
 */
int digram_put(SYMBOL *digram) {

    int position = DIGRAM_HASH(digram->value, digram->next->value);

    int i = position; //copy to iterate and check against original

    SYMBOL *secondDigram = *(digram_table+i);

    if(digram == NULL)
        return -1;

    if (digram->next == NULL)
        return -1;

    if(digram == secondDigram) {
        debug("Digram already exists :(\n");
        return 1;
    }

    if(secondDigram == NULL || secondDigram == TOMBSTONE) {
        debug("Digram %lu (%d, %d) inserted at index %d\n", SYMBOL_INDEX(digram), digram->value, digram->next->value, i);
        *(digram_table + i) = digram;
        return 0;
    }

    i++;


    while(i <= MAX_DIGRAMS){ //check starting from position + 1
        if(i == MAX_DIGRAMS)
            i = 0;
        else if(i == position)
            return -1;

        secondDigram = *(digram_table+i);

        if(digram == secondDigram) //if matching digrams
            return 1;

        if(secondDigram == NULL || secondDigram == TOMBSTONE) {
            *(digram_table + i) = digram;
            return 0;
        }

        i++;
    }

    return -1;
}
