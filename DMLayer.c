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
    char* pszValue;

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

ObsVariable* DMLayer_CreateVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize)
{
    ObsVariable* pVariable;

    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", NULL);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", NULL);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", NULL);
    VERIFY ((pVariable = malloc (sizeof (ObsVariable))) != NULL, "", NULL);

    VERIFY (DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize) == NULL, "Variable already exist, and will not be added", NULL);

    pVariable->nVariableID = DMLayer_GetVariableID (pszVariableName, nVariableSize);
    pVariable->pszValue = NULL;
    pVariable->nLastEventPoint = 0;
    pVariable->pLastObserver = NULL;

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
            printf ("%4zu\t[%-8X]\t[%s]\n", nCount++, pVariable->nVariableID, pVariable->pszValue);

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

static ObsVariable* DMLayer_GetVariableByID (DMLayer* pDMLayer, uint32_t nTargetID)
{
    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", NULL);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", NULL);

    {
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

bool DMLayer_ObserveVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize)
{
    bool nReturn = false;

    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", false);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", false);

    {
        ObsVariable* pVariable;
        uint8_t nLastEvent;

        VERIFY ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) != NULL,
                "Variable already exist, and will not be added",
                NULL);

        nLastEvent = pVariable->nLastEventPoint;

        do
        {
            if (pVariable->nLastEventPoint != nLastEvent)
            {
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

        if ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) != NULL)
        {
            Observable* pObservable = NULL;

            VERIFY ((pObservable = malloc (sizeof (Observable))) != NULL, "", false);

            pObservable->callback = pFunc;
            pObservable->pPrev = pVariable->pLastObserver;

            pVariable->pLastObserver = pObservable;

            nReturn = true;
        }
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

bool DMLayer_CleanUpObserverCallback (DMLayer* pDMLayer, uint32_t nVariableID)
{
    bool nReturn = false;

    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);

    {
        ObsVariable* pVariable = NULL;

        if ((pVariable = DMLayer_GetVariableByID (pDMLayer, nVariableID)) != NULL)
        {
            Observable* pObservable = pVariable->pLastObserver;
            Observable* pTemp = NULL;

            while (pObservable != NULL)
            {
                pTemp = pObservable;
                pObservable = pObservable->pPrev;

                free (pTemp);
            }
        }
    }

    return nReturn;
}

bool DMLayer_CleanUpVariables(DMLayer* pDMLayer)
{
    bool nReturn = false;

    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);

    //Disable all interactions
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
                VERIFY (DMLayer_CleanUpObserverCallback (pDMLayer, pVariable->nVariableID) == true, "Logic Error deleting callbacks", false);
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

static size_t DMLayer_Notify(DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, uint8_t nNotifyTpe)
{
    size_t nReturn = 0;

    VERIFY (NULL != pDMLayer, "Error, DMLayer is invalid.", false);
    VERIFY (false != pDMLayer->enable, "Error, DMLayer is disabled.", false);
    VERIFY (NULL != pszVariableName && 0 != nVariableSize, "Variable is null or empty.", false);

    {
        ObsVariable* pVariable = NULL;

        if ((pVariable = DMLayer_GetVariable (pDMLayer, pszVariableName, nVariableSize)) != NULL)
        {
            Observable* pObservable = pVariable->pLastObserver;

            while (pObservable != NULL)
            {
                //Notifying all observables for a giving Variable
                pObservable->callback (pszVariableName, nVariableSize, nNotifyTpe);
                
                nReturn++;

                pObservable = pObservable->pPrev;
            }
        }
    }

    return nReturn;
}

size_t DMLayer_NotifyOnly (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize)
{
    //NOTE: Verifications will be done by the DMLayer_Notify (no need to overcharge it here)

    return DMLayer_Notify (pDMLayer, pszVariableName, nVariableSize, OBS_NOTIFY_NOTIFY);
}

