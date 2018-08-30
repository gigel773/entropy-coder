#include "ec.h"

/* ------ Internal functions implementation ------ */

/*
Description:
    Spreads symbols according to their frequencies
Parameters:
    dst             pointer to array where to store spreaded symbols
    frequencies     array with occurrences of every symbol in alphabet
    tableLog        power of 2 to get table size
*/
static void spreadSymbols(
        char *dst,
        const unsigned int *frequencies,
        int frequenciesLength,
        int tableLog) {

    /* Variables */
    int tableSize = 1 << tableLog;
    int tableMask = tableSize - 1;
    int position = 0;
    int step = EC_TABLE_STEP(tableSize);

    /* Main cycle */
    for (char symbol = 0; symbol < frequenciesLength; symbol++) {
        for (int occurence = 0; occurence < frequencies[symbol]; occurence++) {
            dst[position] = symbol;
            position = (position + step) & tableMask;
        }
    }
}

/*
Description:
    Writes given number of bits from state to stream
Parameters:
    context         pointer to context that contains information about already encoded part of array
    numberOfBits    number of bits to write from state
Note:
    This function is also shifting current state
*/
static void writeToStream(EC_encoderContext *context, int numberOfBits) {
    if (numberOfBits > context->availableBits) {
        /* Write as many as possible */
        *context->buffer <<= context->availableBits;
        *context->buffer |= (context->currentState & ((1 << context->availableBits) - 1));
        numberOfBits -= context->availableBits;
        context->availableBits = 8;

        /* Get next buffer */
        context->buffer++;
        context->numberOfUsedBytes += 1;

        /* Write rest */
        *context->buffer <<= numberOfBits;
        *context->buffer |= (context->currentState & ((1 << numberOfBits) - 1));
        context->availableBits -= numberOfBits;
    } else {
        *context->buffer <<= numberOfBits;
        *context->buffer |= (context->currentState & ((1 << numberOfBits) - 1));
        context->availableBits -= numberOfBits;
    }
    context->currentState >>= numberOfBits;
}

/* ------ Main external functions implementation ------ */

EC_status EC_buildEncoderTable(
        EC_tableEncoderRecord *dstTable,
        EC_stateTableRecord *nextStateTable,
        unsigned int *frequencies,
        int frequenciesLength,
        int tableLog) {

    /* Variables */
    int tableSize = 1 << tableLog;
    short *intervalBeginIndex = (short *) malloc(frequenciesLength * sizeof(short));
    char *spreadedSymbols = (char *) malloc(tableSize * sizeof(char));

    /* Calculating data for encoding */
    for (char symbol = 0, position = 0; symbol < frequenciesLength; symbol++) {
        intervalBeginIndex[symbol] = position;
        unsigned int highestSetBit;
        _BitScanReverse(&highestSetBit, frequencies[symbol]);

        /* Calculating numbers */
        const int bitsOut = tableLog - highestSetBit;
        const int threshold = frequencies[symbol] << bitsOut;
        const int offset = position - frequencies[symbol];
        position += frequencies[symbol];

        EC_tableEncoderRecord tmpRecord = {
                bitsOut,
                threshold,
                offset
        };

        /* Writing into table */
        dstTable[symbol] = tmpRecord;
    }

    /* Spreading symbols */
    spreadSymbols(spreadedSymbols, frequencies, frequenciesLength, tableLog);

    /* Calculating "nextStateTable" */
    for (int position = 0; position < tableSize; position++) {
        /* Getting symbol from spreaded table */
        const char symbol = spreadedSymbols[position];

        /* Calculating new state for this occurence of symbol */
        EC_stateTableRecord recordWithNewState = {position + tableSize};

        /* Writing this transform */
        nextStateTable[intervalBeginIndex[symbol]] = recordWithNewState;

        /* Move to next occurence inside of interval */
        intervalBeginIndex[symbol]++;
    }

    /* Freeing memory */
    free(intervalBeginIndex);
    free(spreadedSymbols);

    return SUCCESS;
}

EC_status EC_initializeEncoderContext(
        EC_encoderContext *context,
        EC_tableEncoderRecord *encoderTable,
        EC_stateTableRecord *statesTable,
        int tableLog) {

    /* Simple assigning */
    context->encoderTable = encoderTable;
    context->statesTable = statesTable;
    context->tableLog = tableLog;
    context->currentState = 1 << context->tableLog;
    context->availableBits = 8;
    context->numberOfUsedBytes = 1;
    context->bufferSize = 1 << tableLog;

    /* Memory allocation */
    context->pStreamStart = (char *) malloc(sizeof(char) * context->bufferSize);
    for (int i = 0; i < context->bufferSize; i++) {
        context->pStreamStart[i] = 0;
    }
    context->buffer = context->pStreamStart;

    return SUCCESS;
}

EC_status EC_flushEncoderContext(EC_encoderContext *context) {

    /* Simple assigning */
    context->encoderTable = NULL;
    context->statesTable = NULL;
    context->buffer = NULL;
    context->tableLog = 0;
    context->currentState = 0;
    context->availableBits = 0;
    context->numberOfUsedBytes = 0;
    context->bufferSize = 0;

    /* Memory freeing */
    free(context->pStreamStart);
    context->pStreamStart = NULL;

    return SUCCESS;
}

EC_status EC_encode(EC_encoderContext *context, char *src, int srcLength) {

    /* Encoding loop */
    for (int i = 0; i < srcLength; i++) {
        /* Getting symbol to encode */
        const char symbol = src[i];

        /* Getting maximal number of bits for concrete symbol */
        int bitsOut = context->encoderTable[symbol].bitsOut;

        /* If we don't cross the threshold then we should decrease number of bits to write */
        if (context->currentState < context->encoderTable[symbol].threshold) {
            bitsOut--;
        }

        /* Writing to stream */
        writeToStream(context, bitsOut);

        /* Getting next state */
        context->currentState = context->statesTable[
                context->currentState + context->encoderTable[symbol].offset
        ].nextState;
    }

    /* Reduce state by table size in order to normalize it for decoding */
    context->currentState -= (1 << context->tableLog);

    return SUCCESS;
}
