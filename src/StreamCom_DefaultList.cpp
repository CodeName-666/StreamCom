#include "StreamCom.h"


/* Architecture related includes*/
#if ARDUINO_ARCH_ESP32
    #include "Esp.h"
#elif ARDUINO_ARCH_AVR
    #include <avr/io.h>
    #include <avr/wdt.h>
#else 
    #warning "STREAMCOM ECU DEFAULTS NOT SUPPORTED YET"
#endif

#if STREAM_COM_DEFAULT_LIST_ENABLE == true

StreamCom* mThis;	

void StreamCom_Reset(Stream* stream, void* args, uint32_t nParams)
{
    stream->print("STREAM_COM: Reset ESP \r\n");   
#if ARDUINO_ARCH_ESP32
    ESP.restart();
#elif ARDUINO_ARCH_AVR
    wdt_enable(WDTO_15MS);  // Aktiviert den Watchdog-Timer
    while (1) {} 
#endif

}


void StreamCom_Help(Stream* stream, void* args, uint32_t nParams)
{
    if (mThis != NULL)
    {
        mThis->printHelp();
    }
    else 
    {
        stream->println("HELP: Could Not Print Help");   
    }
}

void StreamCom_Size(Stream* stream, void* args, uint32_t nParams)
{
    uint16_t size = 0;
    if (mThis != NULL)
    {
        size = mThis->getServiceQuantity();
        stream->print("There are: "); stream->print(size); stream->println(" Services definend");
    }
    else 
    {
        stream->println("HELP: Could Not Print Help");   
    }
}
					

Service_t StreamCom_default_list[STREAM_COM_DEFAULT_LIST_SIZE] = 
{
/*Nr.  | TOKEN          |   POINTER_TO_PARAMS         |    TYPE_OF_PARAMS    | SIZE  | CALLBACK       |*/
/* 1*/ {"RESET"			, {NULL,NULL,NULL,NULL}       , {NONE,NONE,NONE,NONE},  0    , StreamCom_Reset},
/* 2*/ {"HELP"          , {NULL,NULL,NULL,NULL}       , {NONE,NONE,NONE,NONE},  0    , StreamCom_Help },
/* 2*/ {"SIZE"          , {NULL,NULL,NULL,NULL}       , {NONE,NONE,NONE,NONE},  0    , StreamCom_Size },

};


#endif


