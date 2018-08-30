#pragma once

#include <immintrin.h>
#include <math.h>

/* ------ Defines ------ */
#define EC_TABLE_STEP(tableSize) (((tableSize) >> 1) + ((tableSize) >> 3) + 3)

/* ------ Structures ------ */

/*
Description:
    One row that contains encoding information for specific symbol
Members:
    bitsOut     maximal number of bits that should be used to encode symbol
    threshold   border to understand that we have to use maximum number of bits or minimal
    offset      number that in sum with shifted state gives us index of next state
*/
typedef struct {
    int bitsOut;
    int threshold;
    int offset;
} EC_tableEncoderRecord;

/*
Description:
    One row that contains decoding information for specific symbol
Members:
    symbol              symbol that is associated with state equals to row index
    numberOfBits        number of bits to read from stream
    nextStateBaseline   number gives us next state if we sum it and bits read from stream
*/
typedef struct {
    char symbol;
    int numberOfBits;
    int nextStateBaseline;
} EC_tableDecoderRecord;

/*
Description:
    One row that contains next state
Members:
    nextState   next state for previous that is index of the row
*/
typedef struct {
    int nextState;
} EC_stateTableRecord;

/*
Description:
    Context that contains necessary information for encoding array
Members:
    encoderTable        first table that was built with FSE_buildEncoderTable
    statesTable         second table that was built with EC_buildEncoderTable
    tableLog            power of 2 to get table size
    currentState        current state that contains information about all already encoded symbols
    bufferSize          size of stream
    availableBits       number of bits without useful information that current buffer contains
    numberOfUsedBytes   number of bytes in stream that contains encoded information
    pBufferStart        pointer to stream beginning
    buffer              pointer to current not completed byte in stream
*/
typedef struct {
    EC_tableEncoderRecord *encoderTable;
    EC_stateTableRecord *statesTable;
    int tableLog;
    int currentState;
    int bufferSize;
    int availableBits;
    int numberOfUsedBytes;
    char *pStreamStart;
    char *buffer;
} EC_encoderContext;

/*
Description:
    Context that contains necessary information for decoding
Members:
    decoderTable    table that was built with EC_buildDecoderTable
    tableLog        power of 2 to get table size
    currentState    current state that contains information about all left encoded symbols
    bufferSize      size of stream
    availableBits   number of bits that contains useful information
    pStreamStart    pointer to stream begining
    buffer          pointer to current not completed byte in stream
*/
typedef struct {
    EC_tableDecoderRecord *decoderTable;
    int tableLog;
    int currentState;
    int bufferSize;
    int availableBits;
    char *pStreamStart;
    char *buffer;
} EC_decoderContext;

typedef enum {
    SUCCESS,
    FAILURE
} EC_status;

/* ------ Functions API ------ */

/*
Description:
    Builds two tables for given number of occurrences
Parameters:
    dstTable        pointer to table that will contain information for every symbol with size of alphabet
                    (maximal bits to encode, threshold, offset for subrange)
    nextStateTable  table that contains next state for previous state with size of (1 << tableLog)
    frequencies     array that contains number of occurence for every symbol
    tableLog        power of 2 to get table size
*/
EC_status EC_buildEncoderTable(
        EC_tableEncoderRecord *dstTable,
        EC_stateTableRecord *nextStateTable,
        unsigned int *frequencies,
        int frequenciesLength,
        int tableLog
);

/*
Description:
    Initializes context for FSE encoding
Parameters:
    context         pointer to context that we want to initialize
    encoderTable    pointer to the first table that we've get after calling FSE_buildEncoderTable
    statesTable     pointer to the second table that we've get after calling EC_buildEncoderTable
    tableLog        power of 2 to get table size
Note:
    Use it after building table
*/
EC_status EC_initializeEncoderContext(
        EC_encoderContext *context,
        EC_tableEncoderRecord *encoderTable,
        EC_stateTableRecord *statesTable,
        int tableLog
);

/*
Description:
    Flushes encoder context so you will NOT be able to restore it
Parameters:
    context     pointer to context that has to be flushed
Note:
    Be really confident that you will NOT need context information if using this function
*/
EC_status EC_flushEncoderContext(EC_encoderContext *context);

/*
Description:
    Encodes given array using encoding tables
Parameters:
    context     pointer to initialized encoding context
    src         array to be encoded
*/
EC_status EC_encode(EC_encoderContext *context, char *src, int srcLength);

/*
Description:
    Builds table for given number of occurrences
Parameters:
    dstTable        pointer to table that will contain information for every symbol with size of (1 << tableLog)
                    (symbol, number of bits to read from stream, new state baseline)
    frequencies     array that contains number of occurence for every symbol
    tableLog        power of 2 to get table size
*/
EC_status EC_buildDecoderTable(
        EC_tableDecoderRecord *dstTable,
        unsigned int *frequencies,
        int frequenciesLength,
        int tableLog
);

/*
Description:
    Initializes context for FSE decoding
Parameters:
    decoderContext      pointer to context that we want to initialize
    encoderContext      pointer to encoder context after encoding
    decoderTable        pointer to table that we've get after calling EC_buildDecoderTable
Note:
    Use it after building table
*/
EC_status EC_initializeDecoderContext(
        EC_decoderContext *decoderContext,
        EC_encoderContext *encoderContext,
        EC_tableDecoderRecord *decoderTable
);

/*
Description:
    Flushes decoder context so you will NOT be able to restore it
Parameters:
    context     pointer to context that has to be flushed
Note:
    Be really confident that you will NOT need context information if using this function
*/
EC_status EC_flushDecoderContext(EC_decoderContext *context);

/*
Description:
    Decodes given number of symbols and writes it it to destination array
Parameters:
    context     pointer to initialized decoder context
    dst         pointer to destination array
    dstLength   obviously number of symbols to decode
*/
EC_status EC_decode(EC_decoderContext *context, char *dst, int dstLength);

EC_status EC_normalizeArray(char *src, int srcLength, char *dst, char *minimalValue);

EC_status EC_returnToInitialNorma(char *src, int srcLength, char *dst, char normalizedFactor);

EC_status EC_buildHistogram(char *src, int srcLength, unsigned int *dst, int dstLength);
