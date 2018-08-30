

EC_status EC_normalizeArray(char *src, int srcLength, char *dst, char *minimalValue) {

    /* Initial value */
    *minimalValue = src[0];

    /* Find minimal value in source */
    for (int i = 1; i < srcLength; i++) {
        if (src[i] < *minimalValue) {
            *minimalValue = src[i];
        }
    }

    /* Reduce source */
    for (int i = 0; i < srcLength; i++) {
        dst[i] = src[i] - *minimalValue;
    }

    return SUCCESS;
}

EC_status EC_returnToInitialNorma(char *src, int srcLength, char *dst, char normalizedFactor) {

    /* Main loop */
    for (int i = 0; i < srcLength; i++) {
        dst[i] = src[i] + normalizedFactor;
    }

    return SUCCESS;
}

EC_status EC_buildHistogram(char *src, int srcLength, unsigned int *dst, int dstLength) {

    /* Initialize destination state with zeros */
    for (int i = 0; i < dstLength; i++) {
        dst[i] = 0;
    }

    /* Main loop */
    for (int i = 0; i < srcLength; i++) {
        dst[src[i]]++;
    }

    return SUCCESS;
}
