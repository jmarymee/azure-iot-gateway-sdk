// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "module.h"
#include "azure_c_shared_utility/xlogging.h"

#include "azure_c_shared_utility/threadapi.h"
#include "coap.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/lock.h"

typedef struct COAP_HANDLE_DATA_TAG
{
	THREAD_HANDLE threadHandle;
	LOCK_HANDLE lockHandle;
	int stopThread;
	BROKER_HANDLE broker;

} COAP_HANDLE_DATA;

#define COAP_MESSAGE "coap world"

int COAPThread(void *param)
{
	COAP_HANDLE_DATA* handleData = param;

	MESSAGE_CONFIG msgConfig;
	MAP_HANDLE propertiesMap = Map_Create(NULL);
	if (propertiesMap == NULL)
	{
		LogError("unable to create a Map");
	}
	else
	{
		if (Map_AddOrUpdate(propertiesMap, "coap", "What de what what?") != MAP_OK)
		{
			LogError("unable to Map_AddOrUpdate");
		}
		else
		{
			msgConfig.size = strlen(COAP_MESSAGE);
			msgConfig.source = COAP_MESSAGE;

			msgConfig.sourceProperties = propertiesMap;

			MESSAGE_HANDLE coapMessage = Message_Create(&msgConfig);
			if (coapMessage == NULL)
			{
				LogError("unable to create \"COAP\" message");
			}
			else
			{
				while (1)
				{
					if (Lock(handleData->lockHandle) == LOCK_OK)
					{
						if (handleData->stopThread)
						{
							(void)Unlock(handleData->lockHandle);
							break; /*gets out of the thread*/
						}
						else
						{
							(void)Broker_Publish(handleData->broker, (MODULE_HANDLE)handleData, coapMessage);
							(void)Unlock(handleData->lockHandle);
						}
					}
					else
					{
						/*shall retry*/
					}
					(void)ThreadAPI_Sleep(5000); /*every 5 seconds*/
				}
				Message_Destroy(coapMessage);
			}
		}
	}
	return 0;
}

static MODULE_HANDLE COAP_Create(BROKER_HANDLE broker, const void* configuration)
{
	COAP_HANDLE_DATA* result;
	if (
		(broker == NULL) /*configuration is not used*/
		)
	{
		LogError("invalid arg broker=%p", broker);
		result = NULL;
	}
	else
	{
		result = malloc(sizeof(COAP_HANDLE_DATA));
		if (result == NULL)
		{
			LogError("unable to malloc");
		}
		else
		{
			result->lockHandle = Lock_Init();
			if (result->lockHandle == NULL)
			{
				LogError("unable to Lock_Init");
				free(result);
				result = NULL;
			}
			else
			{
				result->stopThread = 0;
				result->broker = broker;
				result->threadHandle = NULL;
			}
		}
	}
	return result;
}

static void* COAP_ParseConfigurationFromJson(const char* configuration)
{
	(void)configuration;
	return NULL;
}

static void COAP_FreeConfiguration(void* configuration)
{
	(void)configuration;
}

static void COAP_Start(MODULE_HANDLE module)
{
	COAP_HANDLE_DATA* handleData = module;
	if (handleData != NULL)
	{
		if (Lock(handleData->lockHandle) != LOCK_OK)
		{
			LogError("not able to Lock, still setting the thread to finish");
			handleData->stopThread = 1;
		}
		else
		{
			if (ThreadAPI_Create(&handleData->threadHandle, COAPThread, handleData) != THREADAPI_OK)
			{
				LogError("failed to spawn a thread");
				handleData->threadHandle = NULL;
			}
			(void)Unlock(handleData->lockHandle);
		}
	}
}

static void COAP_Destroy(MODULE_HANDLE module)
{
	/*first stop the thread*/
	COAP_HANDLE_DATA* handleData = module;
	int notUsed;
	if (Lock(handleData->lockHandle) != LOCK_OK)
	{
		LogError("not able to Lock, still setting the thread to finish");
		handleData->stopThread = 1;
	}
	else
	{
		handleData->stopThread = 1;
		Unlock(handleData->lockHandle);
	}

	if (handleData->threadHandle != NULL &&
		ThreadAPI_Join(handleData->threadHandle, &notUsed) != THREADAPI_OK)
	{
		LogError("unable to ThreadAPI_Join, still proceeding in _Destroy");
	}

	(void)Lock_Deinit(handleData->lockHandle);
	free(handleData);
}

static void COAP_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	/*no action, HelloWorld is not interested in any messages*/
}

static const MODULE_API_1 COAP_API_all =
{
	{ MODULE_API_VERSION_1 },
	COAP_ParseConfigurationFromJson,
	COAP_FreeConfiguration,
	COAP_Create,
	COAP_Destroy,
	COAP_Receive,
	COAP_Start
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(HELLOWORLD_MODULE)(MODULE_API_VERSION gateway_api_version)
#else
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
	const MODULE_API * api;
	if (gateway_api_version >= COAP_API_all.base.version)
	{
		api = (const MODULE_API*)&COAP_API_all;
	}
	else
	{
		api = NULL;
	}
	return api;
}
