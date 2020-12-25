/**
 * @file    main.c
 * @author  Gustavo Campos (www.github.com/solariun)
 * @brief
 * @version 0.1
 * @date    2020-12-22
 *
 * @copyright Copyright (c) 2020
 *
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "CorePartition/CorePartition.h"

#include "DMLayer.h"

DMLayer* pDMLayer = NULL;

const char pszProducer[] = "THREAD/PRODUCE/VALUE";

const char pszBinProducer[] = "THREAD/PRODUCE/VALUE";

void Thread_Producer (void* pValue)
{
    int nRand = 10;

    while (true)
    {
        nRand ++; 

        printf ("[%s (%zu)]: func: (%u), nRand: [%u]\n", __FUNCTION__, CorePartition_GetID (), DMLayer_SetNumber (pDMLayer, pszProducer, strlen (pszProducer), CorePartition_GetID (), nRand), nRand);

        CorePartition_Yield ();
    }
}

int nValues[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void Consumer_Callback_Notify (const char* pszVariable, size_t nVarSize, size_t nUserType, uint8_t nNotifyType)
{
    nValues[nUserType] = (int)DMLayer_GetNumber (pDMLayer, pszProducer, sizeof (pszProducer));

    printf ("->[%s]: from: [%zu] Variable: [%s], Type: [%u] -> Value: [%u]\n", __FUNCTION__, nUserType, pszVariable, nNotifyType, nValues[nUserType]);

    //DMLayer_SetBinary (pDMLayer, pszBinProducer, strlen (pszBinProducer), (size_t)CorePartition_GetID (), (void*)nValues, sizeof (nValues));
}

void Thread_Consumer (void* pValue)
{
    DMLayer_AddObserverCallback (pDMLayer, pszProducer, strlen (pszProducer), Consumer_Callback_Notify);

    int nRemoteValues[10];
    int nCount = 0;
    size_t nUserType = 0;

    while (DMLayer_ObserveVariable (pDMLayer, pszBinProducer, strlen (pszBinProducer), &nUserType) || CorePartition_Yield ())
    {
        printf ("[%s] From: [%zu] -> type: [%u - bin: %u], size: [%zu]\n",
                __FUNCTION__,
                nUserType,
                DMLayer_GetVariableType (pDMLayer, pszBinProducer, strlen (pszBinProducer)),
                (uint8_t)VAR_TYPE_BINARY,
                DMLayer_GetVariableBinarySize (pDMLayer, pszBinProducer, strlen (pszBinProducer)));

        if (DMLayer_GetVariableType (pDMLayer, pszBinProducer, strlen (pszBinProducer)) == VAR_TYPE_BINARY)
        {
            DMLayer_GetBinary (pDMLayer, pszBinProducer, strlen (pszBinProducer), nRemoteValues, sizeof (nRemoteValues));

            printf ("[%s] Values: ", __FUNCTION__);

            for (nCount = 0; nCount < sizeof (nRemoteValues) / sizeof (nRemoteValues[0]); nCount++)
            {
                printf ("[%u] ", nRemoteValues[nCount]);
            }

            printf ("\n");
        }
    }
}

void CorePartition_SleepTicks (uint32_t nSleepTime)
{
    usleep ((useconds_t)nSleepTime * 1000);
}

uint32_t CorePartition_GetCurrentTick (void)
{
    struct timeval tp;
    gettimeofday (&tp, NULL);

    return (uint32_t)tp.tv_sec * 1000 + tp.tv_usec / 1000;  // get current timestamp in milliseconds
}

static void StackOverflowHandler ()
{
    printf ("Error, Thread#%zu Stack %zu / %zu max\n", CorePartition_GetID (), CorePartition_GetStackSize (), CorePartition_GetMaxStackSize ());
}

void DMLayer_YeldContext ()
{
    CorePartition_Yield ();
}

int main ()
{
    // start random
    srand ((time_t)time (NULL));

    VERIFY ((pDMLayer = DMLayer_CreateInstance ()) != NULL, "Error creating DMLayer instance", 1);

    assert (CorePartition_Start (10));

    assert (CorePartition_SetStackOverflowHandler (StackOverflowHandler));

    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, 500, 200));

    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, 500, 300));

    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, 500, 500));

    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, 500, 260));

    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, 500, 800));

    assert (CorePartition_CreateThread (Thread_Consumer, NULL, 500, 200));

    CorePartition_Join ();

    return 0;
}