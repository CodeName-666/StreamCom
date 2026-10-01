/** @file StreamCom_DefaultList.cpp @brief Standard commands with instance context. */
#include "StreamCom.h"

/* Architecture related includes*/
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
#include "Esp.h"
#elif defined(ARDUINO_ARCH_AVR)
#include <avr/io.h>
#include <avr/wdt.h>
#endif

#if STREAM_COM_DEFAULT_LIST_ENABLE == true

/** @brief Reset the selected MCU. @param[in,out] pStream Response stream.
 * @param[in] pCallbackArguments Unused context. @param[in] parameterCount Unused parameter count.
 * @note Not ISR-safe. AVR waits for the watchdog reset intentionally. */
void StreamCom_Reset(Stream *pStream, void *pCallbackArguments, uint32_t parameterCount)
{
    (void)pCallbackArguments;
    (void)parameterCount;
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    if (pStream != nullptr)
    {
        (void)pStream->print("STREAM_COM: Reset \r\n");
    }
    ESP.restart();
#elif defined(ARDUINO_ARCH_AVR)
    if (pStream != nullptr)
    {
        (void)pStream->print("STREAM_COM: Reset \r\n");
    }
    wdt_enable(WDTO_15MS); // Aktiviert den Watchdog-Timer
    while (1)
    {
    }
#else
    if (pStream != nullptr)
    {
        (void)pStream->println("...ERROR: RESET NOT SUPPORTED ON THIS PLATFORM...");
    }
#endif
}

/** @brief Print help for the receiving instance. @param[in,out] pStream Response stream.
 * @param[in] pCallbackArguments Instance pointer. @param[in] parameterCount Unused count. */
void StreamCom_Help(Stream *pStream, void *pCallbackArguments, uint32_t parameterCount)
{
    StreamCom *pInstance = static_cast<StreamCom *>(pCallbackArguments);

    (void)parameterCount;
    if ((pInstance != nullptr) && (pStream != nullptr))
    {
        pInstance->printHelp();
    }
    else if (pStream != nullptr)
    {
        (void)pStream->println("HELP: Could Not Print Help");
    }
}

/** @brief Print the receiving instance's service count. @param[in,out] pStream Response stream.
 * @param[in] pCallbackArguments Instance pointer. @param[in] parameterCount Unused count. */
void StreamCom_Size(Stream *pStream, void *pCallbackArguments, uint32_t parameterCount)
{
    StreamCom *pInstance   = static_cast<StreamCom *>(pCallbackArguments);
    uint16_t  serviceCount = 0U;

    (void)parameterCount;
    if ((pInstance != nullptr) && (pStream != nullptr))
    {
        serviceCount = pInstance->getServiceQuantity();
        (void)pStream->print("There are: ");
        (void)pStream->print(serviceCount);
        (void)pStream->println(" Services defined");
    }
    else if (pStream != nullptr)
    {
        (void)pStream->println("SIZE: Could Not Read Service Count");
    }
}

Service_t StreamCom_default_list[STREAM_COM_DEFAULT_LIST_SIZE] =
    {
        /*Nr.  | TOKEN          |   POINTER_TO_PARAMS         |    TYPE_OF_PARAMS    | SIZE  | CALLBACK       |*/
        /* 1*/ {"RESET", {}, {}, 0, StreamCom_Reset},
        /* 2*/ {"HELP", {}, {}, 0, StreamCom_Help},
        /* 3*/ {"SIZE", {}, {}, 0, StreamCom_Size},

};

#endif
