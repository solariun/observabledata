/**
 * @file    obsvariant.c
 * @author  Gustavo Campos (www.github.com/solariun)
 * @brief 
 * @version 0.1
 * @date    2020-12-22
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/errno.h>
#include <errno.h>

#include "DMLayer.h"

struct Observable
{
    uint8_t obsType;
    void* event;
    Observable* pPrev;
};

enum _varType
{
    VAR_TYPE_NONE = 0,
    VAR_TYPE_VARIANT,
    VAR_TYPE_NOTIFY
};

struct ObsVariable
{
    uint32_t    nVariableID;
    char*       pszValue;
    uint8_t     nVarType;

    ObsVariable* pPrev;    
};

/*
//                                      16   12   5
// this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
// This works out to be 0x1021, but the way the algorithm works
// lets us use 0x8408 (the reverse of the bit pattern).  The high
// bit is always assumed to be set, thus we only use 16 bits to
// represent the 17 bit value.
*/

#define POLY 0x8408

uint16_t DMLayer_CRC16 (const uint8_t* pData, size_t nSize, uint16_t nCRC)
{
    unsigned char nCount;
    unsigned int data;

    nCRC = ~nCRC;

    if (nSize == 0) return (~nCRC);

    do
    {
        for (nCount = 0, data = (unsigned int)0xff & *pData++; nCount < 8; nCount++, data >>= 1)
        {
            if ((nCRC & 0x0001) ^ (data & 0x0001))
                nCRC = (nCRC >> 1) ^ POLY;
            else
                nCRC >>= 1;
        }
    } while (--nSize);

    nCRC = ~nCRC;
    data = nCRC;
    nCRC = (nCRC << 8) | (data >> 8 & 0xff);

    return (nCRC);
}

static int32_t DMLayer_GetVariableID (const char* pszTopic, size_t length)
{
    return ((int32_t) ((DMLayer_CRC16 ((const uint8_t*)pszTopic, length, 0) << 16) |
                       DMLayer_CRC16 ((const uint8_t*)pszTopic, length, 0x8408)));
}

ObsVariable* DMLayer_CreateVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize)
{
    ObsVariable* pVariable;

    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", NULL);
    VERIFY (NULL != pszVariableName && '\0' != pszVariableName[0], "Variable is null or empty.", NULL);
    VERIFY ((pVariable = malloc (sizeof (ObsVariable))) != NULL, "", NULL);

    pVariable->nVariableID = DMLayer_GetVariableID(pszVariableName, nVariableSize);
    pVariable->nVarType = VAR_TYPE_NONE;
    pVariable->pszValue = NULL;
    pVariable->pPrev = pDMLayer->pObsVariables;

    pDMLayer->pObsVariables = pVariable;

    return pVariable;
}

void DMLayer_PrintVariables (DMLayer* pDMLayer)
{
    size_t nCount = 0;

    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.",);

    ObsVariable* pVariable = pDMLayer->pObsVariables;

    printf ("[Listing variables]--------------------------------\n");

    while (pVariable != NULL)
    {
        printf ("%4zu\t[%-8X]\t%u\n", nCount++, pVariable->nVariableID, pVariable->nVarType);

        pVariable = pVariable->pPrev;
    }

    printf ("--------------------------------------------------\n");
}

DMLayer* DMLayer_CreateInstance()
{
    DMLayer* pDMLayer = NULL;

    VERIFY ((pDMLayer = malloc (sizeof (DMLayer))) != NULL, "", NULL);
    
    pDMLayer->pObsVariables = NULL;

    return pDMLayer;
}

