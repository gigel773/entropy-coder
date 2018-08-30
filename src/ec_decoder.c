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
    Reads and return read number of bits from stream
Parameters:
    context         pointer to decoding context
    numberOfBits    number of bits to read from stream
*/
static int readFromStream(EC_decoderContext *context, int numberOfBits) {

    int result = 0;

    if (numberOfBits > context->availableBits) {
        /* Read as many as possible */
        int mask = (1 << context->availableBits) - 1;
        result = *context->buffer & mask;

        /* Get next buffer */
        context->buffer--;

        /* Read rest */
        int rest = numberOfBits - context->availableBits;
        mask = (1 << rest) - 1;
        result |= ((*context->buffer & mask) << context->availableBits);
        *context->buffer >>= rest;
        context->availableBits = 8 - rest;
    } else {
        int mask = (1 << numberOfBits) - 1;
        result = *context->buffer & mask;
        *context->buffer >>= numberOfBits;
        context->availableBits -= numberOfBits;
    }

    return result;
}

/* ------ Main external functions implementation ------ */

EC_status EC_buildDecoderTable(
        EC_tableDecoderRecord *dstTable,
        unsigned int *frequencies,
        int frequenciesLength,
        int tableLog) {

    /* Variables */
    int tableSize = 1 << tableLog;
    char *spreadedSymbols = (char *) malloc(sizeof(char) * tableSize);
    int *symbolBeginIndex = (int *) malloc(sizeof(int) * tableSize);

    /* Set begining index */
    for (char symbol = 0; symbol < frequenciesLength; symbol++) {
        symbolBeginIndex[symbol] = frequencies[symbol];
    }

    /* Spread symbols */
    spreadSymbols(spreadedSymbols, frequencies, frequenciesLength, tableLog);

    /* Build table */
    for (int position = 0; position < tableSize; position++) {
        const char symbol = spreadedSymbols[position];
        const int newState = symbolBeginIndex[symbol];
        int highestBit;
        _BitScanReverse(&highestBit, newState);
        const int numberOfBits = tableLog - highestBit;
        const int nextStateBaseline = (newState << numberOfBits) - tableSize;

        EC_tableDecoderRecord tmpRecord = {
                symbol,
                numberOfBits,
                nextStateBaseline
        };

        dstTable[position] = tmpRecord;
        symbolBeginIndex[symbol]++;
    }

    /* Freeing memory */
    free(spreadedSymbols);
    free(symbolBeginIndex);

    return SUCCESS;
}

EC_status EC_initializeDecoderContext(
        EC_decoderContext *decoderContext,
        EC_encoderContext *encoderContext,
        EC_tableDecoderRecord *decoderTable) {

    /* Simple assigning */
    decoderContext->decoderTable = decoderTable;
    decoderContext->currentState = encoderContext->currentState;
    decoderContext->tableLog = encoderContext->tableLog;
    decoderContext->bufferSize = encoderContext->numberOfUsedBytes;
    decoderContext->availableBits = 8 - encoderContext->availableBits;

    /* Memory allocation */
    decoderContext->pStreamStart = (char *) malloc(sizeof(char) * decoderContext->bufferSize);
    decoderContext->buffer = decoderContext->pStreamStart + decoderContext->bufferSize - 1;

    /* Stream copying */
    for (int i = 0; i < decoderContext->bufferSize; i++) {
        decoderContext->pStreamStart[i] = encoderContext->pStreamStart[i];
    }

    return SUCCESS;
}

EC_status EC_flushDecoderContext(EC_decoderContext *context) {

    /* Simple assigning */
    context->buffer = NULL;
    context->bufferSize = 0;
    context->currentState = 0;
    context->tableLog = 0;
    context->availableBits = 0;

    /* Memory freeing */
    free(context->pStreamStart);
    context->pStreamStart = NULL;

    return SUCCESS;
}

EC_status EC_decode(EC_decoderContext *context, char *dst, int dstLength) {

    /* Decoding loop */
    for (int i = (dstLength - 1); i >= 0; i--) {
        /* Getting symbol (literally decoding) */
        char symbol = context->decoderTable[context->currentState].symbol;

        /* Reading additional bits from stream */
        int rest = readFromStream(context, context->decoderTable[context->currentState].numberOfBits);

        /* Getting next state */
        context->currentState = context->decoderTable[context->currentState].nextStateBaseline + rest;

        /* Writing decoded symbol to memory */
        dst[i] = symbol;
    }

    return SUCCESS;
}
