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
    OBS_NOTIFY_CLEARED,
    OBS_NOTIFY_DELETED
};


enum __VariableType 
{
    VAR_TYPE_ERROR = 0,
    VAR_TYPE_NONE,
    VAR_TYPE_NUMBER,
    VAR_TYPE_BINARY
};

//types: const char* pszVariableName, size_t nVariableName, size_t nUserType, uint8_t nObservableEventType
typedef void (*obs_callback_func)(const char*, size_t, size_t, uint8_t);

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

bool DMLayer_ReleaseInstance (DMLayer* pDMLayer);

ObsVariable* DMLayer_CreateVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

ObsVariable* DMLayer_GetVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

void DMLayer_PrintVariables (DMLayer* pDMLayer);

bool DMLayer_ObserveVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, size_t* pnUserType);

bool DMLayer_AddObserverCallback (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, obs_callback_func pFunc);

size_t DMLayer_NotifyOnly (DMLayer* pDMLayer, const char* pszVariableName, size_t nUserType, size_t nVariableSize);

bool DMLayer_GetBinary (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, void* pBinData, size_t nBinSize);

size_t DMLayer_GetVariableBinarySize (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

uint8_t DMLayer_GetVariableType (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

size_t DMLayer_GetUserType (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

bool DMLayer_SetBinary (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, size_t nUserType, const void* pBinData, size_t nBinSize);

uint64_t DMLayer_GetNumber (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

bool DMLayer_SetNumber (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, size_t nUserType, uint64_t nValue);


#endif