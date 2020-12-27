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

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DMLayer.h"

struct Observable
{
    obs_callback_func callback;
    Observable* pPrev;
};

struct ObsVariable
{
    uint32_t nVariableID;
    uint8_t nLastEventPoint;
    char* pBinData;
    size_t nBinDataSize;
    size_t nBinAllocSize;
    uint64_t nValue;

    size_t nUserType;

    uint8_t nType;

    Observable* pLastObserver;
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
    return ((int32_t) ((DMLayer_CRC16 ((const uint8_t*)pszTopic, length, 0) << 16) | DMLayer_CRC16 ((const uint8_t*)pszTopic, length, 0x8408)));
}

static bool DMLayer_CleanUpObserverCallback (ObsVariable* pVariable)
{
    bool nReturn = false;

    VERIFY (pVariable != NULL, "Obs Variable is invalid.", false);

    {
        Observable* pObservable = pVariable->pLastObserver;
        Observable* pTemp = NULL;

        while (pObservable != NULL)
        {
            pTemp = pObservable;
            pObservable = pObservable->pPrev;

            free (pTemp);
        }

        nReturn = true;
    }

    return nReturn;
}

static bool DMLayer_ResetVariable (ObsVariable* pVariable, bool nCleanBin, bool nCleanObservables)
{
    VERIFY (pVariable != NULL, "Invalid variable.", false);

    if (pVariable->pBinData != NULL)
    {
        if (nCleanBin == true)
        {
            free (pVariable->pBinData);
            pVariable->pBinData = NULL;
            pVariable->nBinAllocSize = 0;
        }
        else
        {
            if (memset ((void*)pVariable->pBinData, 0, pVariable->nBinAllocSize) != pVariable->pBinData)
            {
                free (pVariable->pBinData);
                pVariable->pBinData = NULL;
                pVariable->nBinAllocSize = 0;

                pVariable->nType = VAR_TYPE_NONE;
            }
        }
    }

    if (nCleanObservables == true && pVariable->pLastObserver != NULL)
    {
        VERIFY (DMLayer_CleanUpObserverCallback (pVariable) == true, "Fail to cleanup observables", false);
        pVariable->pLastObserver = NULL;
    }

    pVariable->nValue = 0;
    pVariable->nLastEventPoint = 0;
    pVariable->nUserType = 0;

    return true;
}

ObsVariable* DMLayer_CreateVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize)
{
    ObsVariable* pVariable;

    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", NULL);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", NULL);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", NULL);
    VERIFY ((pVariable = malloc (sizeof (ObsVariable))) != NULL, "", NULL);

    VERIFY (DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize) == NULL, "Variable already exist, and will not be added", NULL);

    pVariable->nVariableID = DMLayer_GetVariableID (pszVariableName, nVariableSize);
    pVariable->pBinData = NULL;
    pVariable->pLastObserver = NULL;

    DMLayer_ResetVariable (pVariable, false, false);

    pVariable->pPrev = pDMLayer->pObsVariables;

    pDMLayer->pObsVariables = pVariable;

    return pVariable;
}

void DMLayer_PrintVariables (DMLayer* pDMLayer)
{
    size_t nCount = 0;

    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", );

    {
        ObsVariable* pVariable = pDMLayer->pObsVariables;

        printf ("[Listing variables]--------------------------------\n");

        while (pVariable != NULL)
        {
            printf ("%4zu\tMEM:[%8zX]\t[%-8X]\t[%s]\t OBS:[%8zX]\n", nCount++, (size_t) pVariable, pVariable->nVariableID, pVariable->pBinData, (size_t) pVariable->pLastObserver);

            pVariable = pVariable->pPrev;
        }

        printf ("--------------------------------------------------\n");
    }
}

ObsVariable* DMLayer_GetVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize)
{
    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", NULL);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", NULL);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", NULL);

    {
        uint32_t nTargetID = DMLayer_GetVariableID (pszVariableName, nVariableSize);
        ObsVariable* pVariable = pDMLayer->pObsVariables;
        while (pVariable != NULL)
        {
            if (pVariable->nVariableID == nTargetID)
            {
                return pVariable;
            }
            pVariable = pVariable->pPrev;
        }
    }

    return NULL;
}

DMLayer* DMLayer_CreateInstance ()
{
    DMLayer* pDMLayer = NULL;

    VERIFY ((pDMLayer = malloc (sizeof (DMLayer))) != NULL, "", NULL);

    pDMLayer->enable = true;
    pDMLayer->pObsVariables = NULL;

    return pDMLayer;
}

bool DMLayer_ObserveVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, size_t* pnUserType)
{
    bool nReturn = false;

    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", false);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", false);

    {
        ObsVariable* pVariable;
        uint8_t nLastEvent;

        VERIFY ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) != NULL, "Variable does not exist.", false);

        nLastEvent = pVariable->nLastEventPoint;

        do
        {
            if (pVariable->nLastEventPoint != nLastEvent)
            {
                if (pnUserType != NULL)
                {
                    *pnUserType = pVariable->nUserType;
                }

                nReturn = true;
                break;
            }

            DMLayer_YeldContext ();

        } while ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) != NULL);
    }

    return nReturn;
}

bool DMLayer_AddObserverCallback (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, obs_callback_func pFunc)
{
    bool nReturn = false;

    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", false);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", false);
    VERIFY (NULL != pFunc, "Function variable is null", false);

    {
        ObsVariable* pVariable;
        Observable* pObservable = NULL;

        if ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) == NULL)
        {
            VERIFY (pVariable = DMLayer_CreateVariable (pDMLayer, pszVariableName, nVariableSize), "Error creating Variable.", false);
            pVariable->nType = VAR_TYPE_NONE;
        }
        
        VERIFY ((pObservable = malloc (sizeof (Observable))) != NULL, "", false);

        pObservable->callback = pFunc;
        pObservable->pPrev = pVariable->pLastObserver;

        pVariable->pLastObserver = pObservable;

        printf ("[%s]: (%8zX) Adding observable callback: [%s](%8X) -> fund: [%8zX], pLastObservable: [%8zX]\n", __FUNCTION__, (size_t) pVariable, pszVariableName, pVariable->nVariableID, (size_t) pFunc, (size_t) pVariable->pLastObserver);
        DMLayer_PrintVariables (pDMLayer);

        nReturn = true;
    }

    return nReturn;
}

Observable* DMLayer_GetObserverCallback (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, obs_callback_func pFunc)
{
    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", NULL);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", false);
    VERIFY (NULL != pFunc, "Function variable is null", false);

    {
        ObsVariable* pVariable = NULL;

        if ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) != NULL)
        {
            Observable* pObservable = pVariable->pLastObserver;

            while (pObservable != NULL)
            {
                if (pObservable->callback == pFunc)
                {
                    return pObservable;
                }
            }
        }
    }

    return NULL;
}

bool DMLayer_RemoveObserverCallback (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, obs_callback_func pFunc)
{
    bool nReturn = false;

    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", false);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", false);
    VERIFY (NULL != pFunc, "Function variable is null", false);

    {
        ObsVariable* pVariable = NULL;

        if ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) != NULL)
        {
            Observable* pObservable = pVariable->pLastObserver;
            Observable* pObservablePrev = NULL;

            while (pObservable != NULL)
            {
                if (pObservable->callback == pFunc)
                {
                    if (pObservable == pVariable->pLastObserver)
                    {
                        VERIFY (pObservablePrev == NULL, "Logic Error, please review.", false);

                        pVariable->pLastObserver = pObservable->pPrev;
                    }
                    else
                    {
                        VERIFY (pObservablePrev != NULL, "Logic Error, please review.", false);

                        pObservablePrev->pPrev = pObservable->pPrev;
                    }

                    free (pObservable);

                    nReturn = true;
                }

                pObservablePrev = pObservable;
            }
        }
    }

    return nReturn;
}

bool DMLayer_CleanUpVariables (DMLayer* pDMLayer)
{
    bool nReturn = false;

    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);

    // Disable all interactions
    pDMLayer->enable = false;

    if (pDMLayer->pObsVariables != NULL)
    {
        ObsVariable* pVariable = pDMLayer->pObsVariables;
        ObsVariable* pTemp;

        while (pVariable != NULL)
        {
            pTemp = pVariable;

            if (pVariable->pLastObserver != NULL)
            {
                VERIFY (DMLayer_CleanUpObserverCallback (pVariable) == true, "Logic Error deleting callbacks", false);
            }

            pVariable = pVariable->pPrev;
            free (pTemp);
        }

        nReturn = true;
    }
    else
    {
        nReturn = true;
    }

    return nReturn;
}

bool DMLayer_ReleaseInstance (DMLayer* pDMLayer)
{
    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);

    VERIFY (DMLayer_CleanUpVariables (pDMLayer) == true, "Error cleaning up Variables and observers.", false);

    return true;
}

static size_t DMLayer_Notify (ObsVariable* pVariable, const char* pszVariableName, size_t nVariableSize, size_t nUserType, uint8_t nNotifyTpe)
{
    size_t nReturn = 0;

    VERIFY (NULL != pVariable, "Variable is not valid.", false);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", false);

    {
        Observable* pObservable = pVariable->pLastObserver;

        pVariable->nUserType = nUserType;

        pVariable->nLastEventPoint++;

        while (pObservable != NULL)
        {
            printf ("OBS-> Callback for [%*s]-> (%8zX)\n", (int)nVariableSize, pszVariableName, (size_t)pObservable->callback);

            // Notifying all observables for a giving Variable
            pObservable->callback (pszVariableName, nVariableSize, nUserType, nNotifyTpe);

            nReturn++;

            pObservable = pObservable->pPrev;
        }
    }

    return nReturn;
}

size_t DMLayer_NotifyOnly (DMLayer* pDMLayer, const char* pszVariableName, size_t nUserType, size_t nVariableSize)
{
    size_t nReturn = 0;

    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", false);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", false);

    {
        ObsVariable* pVariable = NULL;

        if ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) != NULL)
        {
            nReturn = DMLayer_Notify (pVariable, pszVariableName, nVariableSize, nUserType, OBS_NOTIFY_NOTIFY);
        }
    }

    return nReturn;
}

bool DMLayer_SetNumber (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, size_t nUserType, uint64_t nValue)
{
    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", false);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", false);

    {
        ObsVariable* pVariable = NULL;
        uint8_t nNotifyType = OBS_NOTIFY_CHANGED;

        if ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) == NULL)
        {
            VERIFY (pVariable = DMLayer_CreateVariable (pDMLayer, pszVariableName, nVariableSize), "Error creating Variable.", false);
        }
        else
        {
            VERIFY (DMLayer_ResetVariable (pVariable, true, false), "Error reseting variable", false);
        }

        nNotifyType = pVariable->nType == VAR_TYPE_NONE ? OBS_NOTIFY_CREATED : nNotifyType;

        pVariable->nType = VAR_TYPE_NUMBER;
        pVariable->nValue = (uint64_t)nValue;

        printf ("[%s] [%*s]=(%llu)\n", __FUNCTION__, (int) nVariableSize, pszVariableName, pVariable->nValue);

        if (pVariable->pLastObserver != NULL)
        {
            VERIFY (DMLayer_Notify (pVariable, pszVariableName, nVariableSize, nUserType, nNotifyType) > 0, "Error notifying observers", false);
        }
        else
        {
            printf ("[%s:%s(%8X)] No notifier list.\n", __FUNCTION__, pszVariableName, pVariable->nVariableID);
            DMLayer_PrintVariables (pDMLayer);
        }
    }

    return true;
}

uint64_t DMLayer_GetNumber (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize)
{
    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", 0);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", 0);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", 0);

    {
        ObsVariable* pVariable = NULL;

        if ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) != NULL && pVariable->nType == VAR_TYPE_NUMBER)
        {
            return (uint64_t)pVariable->nValue;
        }
    }

    return 0;
}

bool DMLayer_SetBinary (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, size_t nUserType, const void* pBinData, size_t nBinSize)
{
    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", false);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", false);
    VERIFY (NULL != pBinData && nBinSize > 0, "Bin data inconsistent.", false);

    {
        ObsVariable* pVariable = NULL;
        uint8_t nNotifyType = OBS_NOTIFY_CHANGED;

        if ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) == NULL)
        {
            VERIFY (pVariable = DMLayer_CreateVariable (pDMLayer, pszVariableName, nVariableSize), "Error creating Variable.", false);
        }
        else
        {
            VERIFY (DMLayer_ResetVariable (pVariable, true, false), "Error reseting variable", false);
        }

        pVariable->nType = VAR_TYPE_BINARY;

        if (pVariable->pBinData != NULL)
        {
            if (pVariable->nBinAllocSize < nBinSize)
            {
                VERIFY ((pVariable->pBinData = realloc (pVariable->pBinData, nBinSize)) == NULL, "", false);
                pVariable->nBinAllocSize = nBinSize;
            }
        }
        else
        {
            if ((pVariable->pBinData = malloc (nBinSize)) == NULL)
            {
                VERIFY (DMLayer_ResetVariable (pVariable, true, false), "Error reseting variable", false);
            }

            nNotifyType = OBS_NOTIFY_CREATED;
        }

        pVariable->nBinDataSize = nBinSize;
        memcpy ((void*)pVariable->pBinData, (void*)pBinData, nBinSize);

        if (pVariable->pLastObserver != NULL)
        {
            VERIFY (DMLayer_Notify (pVariable, pszVariableName, nVariableSize, nUserType, nNotifyType) == 0, "Error notifying observers", false);
        }
    }

    return true;
}

uint8_t DMLayer_GetVariableType (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize)
{
    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", false);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", false);

    {
        ObsVariable* pVariable = NULL;

        if ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) != NULL)
        {
            return pVariable->nType;
        }
    }

    return VAR_TYPE_ERROR;
}

size_t DMLayer_GetVariableBinarySize (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize)
{
    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", 0);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", 0);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", 0);

    {
        ObsVariable* pVariable = NULL;

        if ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) != NULL && pVariable->nType == VAR_TYPE_BINARY)
        {
            return pVariable->nBinDataSize;
        }
    }

    return 0;
}

size_t DMLayer_GetUserType (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize)
{
    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", 0);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", 0);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", 0);

    {
        ObsVariable* pVariable = NULL;

        if ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) != NULL && pVariable->nType == VAR_TYPE_BINARY)
        {
            return pVariable->nUserType;
        }
    }

    return 0;
}

bool DMLayer_GetBinary (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, void* pBinData, size_t nBinSize)
{
    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", false);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", false);
    VERIFY (NULL != pBinData && nBinSize > 0, "Bin data inconsistent.", false);

    {
        ObsVariable* pVariable = NULL;

        if ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) != NULL && pVariable->nType == VAR_TYPE_BINARY)
        {
            VERIFY (memcpy (pBinData, pVariable->pBinData, nBinSize) == pBinData, "", false);
        }
    }

    return false;
}