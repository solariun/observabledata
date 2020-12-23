/**
 * @file    obsvariant.h
 * @author  Gustavo Campos (www.github.com/solariun)
 * @brief   Observable variant variable
 * @version 0.1
 * @date    2020-12-22
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef DMLAYER_H
#define DMLAYER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/**
 * @brief  Definitions
 */

#ifdef VERIFY
#undef VERIFY
#endif 

#define VERIFY(term,message,ret) if (!(term)){fprintf (stderr, "OBSVAR:%s[%u](%s):ERROR:[%s]\n", __FUNCTION__, __LINE__, #term, (message [0] == '\0' ? strerror (errno) : message)); return ret;}

/**
 * @brief typedefs private strctures 
*/

enum _notifyType
{
    OBS_NOTIFY_NOTIFY,
    OBS_NOTIFY_CREATED,
    OBS_NOTIFY_CHANGED,
    OBS_NOTIFY_CLEARED
};

typedef void (*obs_callback_func)(const char*, size_t, uint8_t);

typedef struct Observable Observable;

typedef struct ObsVariable ObsVariable;

/**
 * @brief typedefs public strctures 
*/

typedef struct
{
    bool enable;
    ObsVariable* pObsVariables;
} DMLayer;


/**
 * @brief public Functions 
 */


extern void DMLayer_YeldContext();

DMLayer* DMLayer_CreateInstance();

ObsVariable* DMLayer_CreateVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

ObsVariable* DMLayer_GetVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

void DMLayer_PrintVariables (DMLayer* pDMLayer);

bool DMLayer_ObserveVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

bool DMLayer_AddObserverCallback (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, obs_callback_func pFunc);

size_t DMLayer_NotifyOnly (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

bool DMLayer_ReleaseInstance (DMLayer* pDMLayer);

#endif