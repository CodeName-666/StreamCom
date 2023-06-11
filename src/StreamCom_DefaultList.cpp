#include "StreamCom.h"


#if STREAM_COM_DEFAULT_LIST_ENABLE == true
    #define STREAM_COM_DEFAULT_LIST_SIZE                                           2u
#endif

/* Architecture related includes*/
#if ARDUINO_ARCH_ESP32
    #include "Esp.h"
#elif ARDUINO_ARCH_AVR
    #include <avr/io.h>
    #include <avr/wdt.h>
#else 
    #warning "STREAMCOM ECU DEFAULTS NOT SUPPORTED YET"
#endif


static void StreamCom_Reset(Stream* stream, void* args, uint32_t nParams)
{
    stream->print("STREAM_COM: Reset ESP \r\n");   
#if ARDUINO_ARCH_ESP32
    ESP.restart();
#elif ARDUINO_ARCH_AVR
    wdt_enable(WDTO_15MS);  // Aktiviert den Watchdog-Timer
    while (1) {} 
#endif

}


static void StreamCom_Help(Stream* stream, void* args, uint32_t nParams)
{
    StreamCom* streamCom = STEAMCOM_GET_PTR(StreamCom,args,0);
    streamCom->printHelp();

}


#if STREAM_COM_DEFAULT_LIST_ENABLE == true

StreamCom* mThis;						

ParamList_t StreamCom_default_list[STREAM_COM_DEFAULT_LIST_SIZE] = 
{
/*Nr.  | TOKEN          |   POINTER_TO_PARAMS         |    TYPE_OF_PARAMS    | SIZE  | CALLBACK       |*/
/* 1*/ {"RESET"			, {mThis,NULL,NULL,NULL}       , {RAW,NONE,NONE,NONE},  0    , StreamCom_Reset},
/* 2*/ {"HELP"          , {mThis,NULL,NULL,NULL}       , {RAW,NONE,NONE,NONE},  0    , StreamCom_Help },

};


#endif


