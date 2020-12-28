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
#include <time.h>
#include <unistd.h>

#ifndef bool
#include <stdbool.h>
#endif

#include "CorePartition.h"

#include "DMLayer.h"

#include <arduino.h>

DMLayer* pDMLayer = NULL;

const char pszProducer[] = "THREAD/PRODUCE/VALUE";

const char pszBinProducer[] = "THREAD/PRODUCE/BIN/VALUE";

void Thread_Producer (void* pValue)
{
    int nRand = 10;

    while (true)
    {
        bool nResponse = false;
        
        nRand ++; 
        
        nResponse =  DMLayer_SetNumber (pDMLayer, pszProducer, strlen (pszProducer), CorePartition_GetID (), nRand);
        
        TRACE ("[%s (%zu)]: func: (%u), nRand: [%u]\n", __FUNCTION__, CorePartition_GetID(), nResponse, nRand);

        CorePartition_Yield ();
    }
}

int nValues[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void Consumer_Callback_Notify (DMLayer* pDMLayer, const char* pszVariable, size_t nVarSize, size_t nUserType, uint8_t nNotifyType)
{
    bool nSuccess = false;
    
    nValues[nUserType] = (int)DMLayer_GetNumber (pDMLayer, pszVariable, nVarSize, &nSuccess);

    TRACE ("->[%s]: Variable: [%*s]\n", __FUNCTION__, (int) nVarSize, pszVariable);
    
    VERIFY (nSuccess == true, "Error, variable is invalid", );
    
    TRACE ("->[%s]: from: [%zu], Type: [%u] -> Value: [%u]\n", __FUNCTION__, nUserType, nNotifyType, nValues[nUserType]);

    DMLayer_SetBinary (pDMLayer, pszBinProducer, strlen (pszBinProducer), (size_t)CorePartition_GetID (), (void*)nValues, sizeof (nValues));
}

void Thread_Consumer (void* pValue)
{
    DMLayer_AddObserverCallback (pDMLayer, pszProducer, strlen (pszProducer), Consumer_Callback_Notify);

    int nRemoteValues[10];
    int nCount = 0;
    size_t nUserType = 0;

    while (DMLayer_ObserveVariable (pDMLayer, pszBinProducer, strlen (pszBinProducer), &nUserType) || CorePartition_Yield ())
    {
        TRACE ("[%s] From: [%zu] -> type: [%u - bin: %u], size: [%zu]\n",
                __FUNCTION__,
                nUserType,
                DMLayer_GetVariableType (pDMLayer, pszBinProducer, strlen (pszBinProducer)),
                (uint8_t)VAR_TYPE_BINARY,
                DMLayer_GetVariableBinarySize (pDMLayer, pszBinProducer, strlen (pszBinProducer)));

        if (DMLayer_GetVariableType (pDMLayer, pszBinProducer, strlen (pszBinProducer)) == VAR_TYPE_BINARY)
        {
            DMLayer_GetBinary (pDMLayer, pszBinProducer, strlen (pszBinProducer), nRemoteValues, sizeof (nRemoteValues));

            Serial.print ("[");
            Serial.print (__FUNCTION__);
            Serial.print ("], Values: ");
                    
            for (nCount = 0; nCount < sizeof (nRemoteValues) / sizeof (nRemoteValues[0]); nCount++)
            {
                Serial.print ("[");
                Serial.print (nRemoteValues[nCount]);
                Serial.print ("] ");
            }

            Serial.println ("");
        }
    }
}

/// Espcializing CorePartition Tick as Milleseconds
uint32_t CorePartition_GetCurrentTick ()
{
    return (uint32_t)millis ();
}

/// Specializing CorePartition Idle time
/// @param nSleepTime How log the sleep will lest
void CorePartition_SleepTicks (uint32_t nSleepTime)
{
    delay (nSleepTime);
}

static void StackOverflowHandler ()
{
    Serial.print ("[");
    Serial.print (__FUNCTION__);
    Serial.print ("] Thread #");
    Serial.print (CorePartition_GetID ());
    Serial.print (", Stack: ");
    Serial.print (CorePartition_GetStackSize ());
    Serial.print ("/");
    Serial.print (CorePartition_GetMaxStackSize ());
    Serial.print (" bytes max.");
    Serial.flush ();
}

void DMLayer_YieldContext ()
{
    CorePartition_Yield ();
}

void setup ()
{
    // Initialize serial and wait for port to open:
     Serial.begin (115200);

     while (!Serial)
     {
     };

     delay (1000);

     Serial.print ("DMLayer Demo");
     Serial.println (CorePartition_version);
     Serial.println ("");

     Serial.println ("Starting up Threads....");
     Serial.flush ();
     Serial.flush ();
    
    // start random
    srand ((unsigned int)(time_t)time (NULL));

    VERIFY ((pDMLayer = DMLayer_CreateInstance ()) != NULL, "Error creating DMLayer instance", );

    Serial.println ("DMLayer Created....");
    Serial.flush ();
    Serial.flush ();

#ifdef __AVR__
#define STACK_PRODUCER sizeof(size_t) * 10
#define STACK_CONSUMER sizeof(size_t) * 35
#else
#define STACK_PRODUCER sizeof(size_t) * 20
#define STACK_CONSUMER sizeof(size_t) * 45
#endif
    
    assert (CorePartition_Start (11));

    assert (CorePartition_SetStackOverflowHandler (StackOverflowHandler));

    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, STACK_PRODUCER, 1));

    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, STACK_PRODUCER, 300));

    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, STACK_PRODUCER, 300));

    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, STACK_PRODUCER, 500));
    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, STACK_PRODUCER, 500));

    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, STACK_PRODUCER, 50));

    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, STACK_PRODUCER, 800));
    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, STACK_PRODUCER, 800));

    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, STACK_PRODUCER, 1000));

    assert (CorePartition_CreateSecureThread (Thread_Producer, NULL, STACK_PRODUCER, 60000));

    VERIFY (CorePartition_CreateThread (Thread_Consumer, NULL, STACK_CONSUMER, 200), "Error creating consumer thread", );
    
    
    CorePartition_Join ();

}

void loop()
{
    
}
