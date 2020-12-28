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


#ifdef __cplusplus
extern "C"
{
#endif

#ifndef bool
#include <stdbool.h>
#endif

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


#define NOTRACE 1 ? (void) 0 : (void) printf

#ifdef __DEBUG__
#define VERIFY(term,message,ret) if (!(term)){fprintf (stderr, "OBSVAR:%s[%u](%s):ERROR:[%s]\n", __FUNCTION__, __LINE__, #term, (message [0] == '\0' ? strerror (errno) : message)); return ret;}

#define YYTRACE printf
#define TRACE YYTRACE

#else

#define VERIFY(term,message,ret) if (!(term)){return ret;}
#define YYTRACE NOTRACE
#define TRACE YYTRACE

#endif


/**
 * @brief typedefs private structures
*/

enum __notifyType
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

//types: const char* pszVariableName, size_t nVariableName, size_t nUserType, uint8_t nObservableEventType
typedef void (*obs_callback_func)(DMLayer*, const char*, size_t, size_t, uint8_t);

/**
 * @brief public Functions
 */

/**
 * @brief External Yield function interface
 */
extern void DMLayer_YieldContext(void);

/**
 * @brief Create API instance
 *
 * @returns *DMLayer new API structure pointer
 */
DMLayer* DMLayer_CreateInstance(void);

/**
 * @brief   Clean and release the DMLayer structure and data
 *
 * @param pDMLayer The DMLayer API structure pointer
 *
 * @returns true if it was successfully released
 */
bool DMLayer_ReleaseInstance (DMLayer* pDMLayer);

/**
 * @brief Create a new empty variable
 *
 * @param pDMLayer     DMLayer structure pointer
 * @param pszVariableName  C String variable name pointer
 * @param nVariableSize Size of the provided variable name
 *
 * @returns *ObsVariable pointer for new stored variable, on Error return NULL
 */
ObsVariable* DMLayer_CreateVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

/**
 * @brief Find and return a stored variable pointer
 *
 * @param pDMLayer  DMLayer structure pointer
 * @param pszVariableName  C String variable name pointer
 * @param nVariableSize  Size of the provided variable name
 *
 * @returns return the pointer of a found variable, on error return NULL.
 */
ObsVariable* DMLayer_GetVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

/**
 * @brief Prints all Variable name found
 */
void DMLayer_PrintVariables (DMLayer* pDMLayer);

/**
 * @brief Observe a variable event
 *
 * @param pDMLayer  DMLayer structure pointer
 * @param pszVariableName   C string variable name
 * @param nVariableSize Size of the provided variable name
 * @param pnUserType    if provided will receive the User defined type for the provided variable
 *
 * @returns Return false in case of error or variable not found.
 */
bool DMLayer_ObserveVariable (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, size_t* pnUserType);

/**
 * @brief Add a new observable function callback
 *
 * @param pDMLayer  DMLayer structure pointer
 * @param pszVariableName   C string variable name
 * @param nVariableSize Size of the provided variable name
 * @param pFunc typedef'ed function to be used as observable callback
 *
 * @returns Return false in case of error or variable not found.
 */
bool DMLayer_AddObserverCallback (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, obs_callback_func pFunc);

/**
 * @brief Only send a notification only event regardless if the variable has data or type of data
 *
 * @param pDMLayer  DMLayer structure pointer
 * @param pszVariableName   C String variable name pointer
 * @param nVariableSize Size of the provided variable name
 * @param nUserType User defined type to be notified along the dynamic type.
 *
 * @note User Defined type is a uint variable, avoid using zero.
 *
 * @returns Return how many callback observers where called.
 */
size_t DMLayer_NotifyOnly (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, size_t nUserType);

/**
 * @brief Find and return the Binary data stored in a variable
 *
 * @param pDMLayer  DMLayer structure pointer
 * @param pszVariableName   C string of the variable name
 * @param nVariableSize Size of the provided variable name
 * @param pBinData The bin data (void*) pointer to be stored
 * @param nBinSize  The size of bin data to be returned
 *
 * @note When using Binary Data, the variable will automatically adjust the internal variable storage based on the greatest data stored and will only release this memory if destroyed or a SetNumber is used to store a number
 *
 * @returns return false if error, variable not storing binary or variable not found.
 */
bool DMLayer_GetBinary (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, void* pBinData, size_t nBinSize);

/**
 * @brief If variable exist and is storing a binary data this function will return the size of the stored bin data.
 *
 * @param pDMLayer  DMLayer structure pointer
 * @param pszVariableName   C string variable name pointer
 * @param nVariableSize Size of the provided variable name
 *
 * @returns If variable exist and is storing a binary data this function will return the size of the stored bin data.
 */
size_t DMLayer_GetVariableBinarySize (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

/**
 * @brief If variable exist return the type of the stored data (enum __VariableType)
 *
 * @param pDMLayer  DMLayer structure pointer
 * @param pszVariableName   C string of a variable name pointer
 * @param nVariableSize Size of the provided variable name
 *
 * @returns return false in case of a error or the variable does not exist.
 */
uint8_t DMLayer_GetVariableType (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

/**
 * @brief If variable exist get the user type defined for the variable value
 *
 * @param pDMLayer  DMLayer structure pointer
 * @param pszVariableName   C string of a variable name pointer
 * @param nVariableSize Size of the provided variable name
 *
 * @returns If variable exist will return the User defined type for the set value. Otherwise will return ZERO
 */
size_t DMLayer_GetUserType (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize);

/**
 * @brief if the variable does not exist it will create and set a Binary Data as value
 *
 * @param pDMLayer  DMLayer structure pointer
 * @param pszVariableName   C string of the variable name
 * @param nVariableSize Size of the provided variable name
 * @param pBinData The bin data (void*) pointer to be stored
 * @param nBinSize  The size of bin data stored
 *
 * @note When using Binary Data, the variable will automatically adjust the internal variable storage based on the greatest data  stored and will only release this memory if destroyed or a SetNumber is used to store a number
 *
 * @returns Return false in case of error
 */
bool DMLayer_SetBinary (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, size_t nUserType, const void* pBinData, size_t nBinSize);

/**
 * @brief If variable exist and if a number variable return the stored value.
 *
 * @param pDMLayer  DMLayer structure pointer
 * @param pszVariableName   C string of a variable name pointer
 * @param nVariableSize Size of the provided variable name
 * @param pnSuccess On success the variable will hold true, otherwise false
 *
 * @returns If variable exist will return the User defined type for the set value. Otherwise will return ZERO
 */
uint64_t DMLayer_GetNumber (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, bool* pnSuccess);

/**
 * @brief If the variable exist it will create it and set a number as value
 *
 * @param pDMLayer  DMLayer structure pointer
 * @param pszVariableName   C string of a variable name pointer
 * @param nVariableSize Size of the provided variable name
 * @param nUserType The User defined type to be broadcasted to observers
 * @param nValue    The number value to be ser as Variable value.
 *
 * @note The value will be always int64_t, use cast for storing or retrieving numbers.
 *
 * @returns return false in case of error
 */
bool DMLayer_SetNumber (DMLayer* pDMLayer, const char* pszVariableName, size_t nVariableSize, size_t nUserType, uint64_t nValue);




#ifdef __cplusplus
}
#endif

#endif
