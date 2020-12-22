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


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "CorePartition/CorePartition.h"

#include "DMLayer.h"

void CorePartition_SleepTicks (uint32_t nSleepTime)
{
    usleep ((useconds_t) nSleepTime * 1000);
}

uint32_t CorePartition_GetCurrentTick(void)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    
    return (uint32_t) tp.tv_sec * 1000 + tp.tv_usec / 1000; //get current timestamp in milliseconds
}

static void StackOverflowHandler ()
{
    printf ("Error, Thread#%zu Stack %zu / %zu max\n", CorePartition_GetID(), CorePartition_GetStackSize(), CorePartition_GetMaxStackSize());
}

DMLayer* pDMLayer = NULL;

int main ()
{

     assert (CorePartition_Start (10));

     assert (CorePartition_SetStackOverflowHandler (StackOverflowHandler));

     //Every 1000 cycles with a Stack page of 210 bytes
     //assert (CorePartition_CreateThread (Thread1, NULL,  210, 1000));

     //All the time with a Stack page of 150 bytes and
     //thread isolation
     //assert (CorePartition_CreateSecureThread (Thread2, NULL, 150, 2000));

     //assert (CorePartition_CreateSecureThread (Thread2, NULL, 150, 500));

     //CorePartition_Join();

    {
        VERIFY ((pDMLayer =DMLayer_CreateInstance ()) != NULL, "Error creating DMLayer instance", 1);

        DMLayer_CreateVariable (pDMLayer, "Variable 1", 10);

        DMLayer_CreateVariable (pDMLayer, "Variable 2", 10);

        DMLayer_CreateVariable (pDMLayer, "Variable 3", 10);

        DMLayer_CreateVariable (pDMLayer, "Variable 4", 10);

        DMLayer_CreateVariable (pDMLayer, "Variable 5", 10);
        
        DMLayer_PrintVariables (pDMLayer);
    
    }

    return 0;
}