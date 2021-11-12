#include <Arduino.h>
#include "StreamCom.h"
#include "TelnetStream.h"

#if __has_include("Login.h")
    #include "Login.h"
    const char* ssid = WIFI_SSID;            
    const char* password = WIFI_PASS;
#else 
    const char* ssid = "Wifi SSID here";            
    const char* password = "Wifi Password here";
#endif

#define NUMBER_OF_COMMANDS      3

/* Global Parameter definition for StreamCom usage */
uint32_t set_i_var = 0;
uint32_t p = 0;
float i = 0, d = 0;


/*==== CALLBACKS for COMMANDOS ===================== */
/**
 * @brief Callback for Commmand RESET
 * 
 * @param stream Ref to stream for response if needed
 * @param args No args here. All defined as NULL
 * @param nParams 0
 */
void reset(Stream* stream, void* args, uint32_t nParams)
{
    stream->println("Restart of ECU\n");
    ESP.restart();
}

/**
 * @brief Callback for Commmand PID
 * 
 * @param stream Ref to stream for response if needed
 * @param args No args here. All defined as NULL
 * @param nParams 3
 */
void set_pid(Stream* stream, void* args, uint32_t nParams)
{
    uint32_t currPVal = STEAMCOM_GET_VALUE(uint32_t,args,0);
    float currIVal    = STEAMCOM_GET_VALUE(float,args,1);
    float currDVal    = STEAMCOM_GET_VALUE(float,args,2);

    uint32_t* currPPtr = STEAMCOM_GET_PTR(uint32_t,args,0);
    float* currIPtr    = STEAMCOM_GET_PTR(float,args,1);
    float* currDPtr    = STEAMCOM_GET_PTR(float,args,2);

    stream->printf("PID Data Value Received: P = %i, I= %f, D = %f\r\n", currPVal, currIVal, currDVal);
    stream->printf("PID Data PTR Received: P = %i, I= %f, D = %f\r\n", *currPPtr, *currIPtr, *currDPtr);
    stream->printf("PID Data Compare:  P = %i, I= %f, D = %f\r\n", p,i,d);

    stream->printf("Size of received Params = %i\r\n", nParams);
}


/*=== Commando and Parameter defintion =============================================*/
ParamList_t paramlist[NUMBER_OF_COMMANDS] = {
/*-----|  CMD  |      Param Ptr List         |     Param Type List  | NrPar| Clbk |-*/
/*[0]*/{"RESET", {NULL      ,NULL ,NULL, NULL} ,{NONE,NONE,NONE,NONE},   0 , reset},
/*[1]*/{"SET_I", {&set_i_var,NULL ,NULL, NULL} ,{I32 ,NONE,NONE,NONE},   1 , NULL},
/*[2]*/{"PID"  , {&p        ,&i   ,&d  , NULL} ,{I32 ,F   ,F   ,NONE},   3 , set_pid}
};


StreamCom streamComSerial;
StreamCom streamComTelnet;

uint32_t last_set_i_var = 0; /* Parameter to print only on change*/
wl_status_t wifi_status;

/**
 * Usage of StreamCom: 
 * 
 * a. Connect over Serial/Telnet or other Stream interface.
 * b. Enter of Commands the following way (here for PID Command):
 *      PID=15;0.12;0.23
 * 
 * c. Result: PDI Callback [set_pid] should be called and executed with
 *    parsed parameter.
 * 
 * Hint: Token to identify Command and Data is '=' [default].
 *       Token to differ between parameter is ';' [default].
 * 
 */
void setup(void) 
{
    /*.... Usage of SteamCom over Serial ...*/
    Serial.begin(115200);
    streamComSerial.init(Serial,paramlist,NUMBER_OF_COMMANDS);
   
    /*.... Usage of SteamCom over TelNet ...*/
    wifi_status = WiFi.begin(ssid,password);
    WiFi.setAutoConnect(true);
    
    TelnetStream.begin();
    streamComTelnet.init(TelnetStream,paramlist,NUMBER_OF_COMMANDS);
}


void loop(void) 
{
    streamComSerial.loop();
    streamComTelnet.loop();

    /*--- Debug Code --------------------------------------------*/
    if(last_set_i_var != set_i_var)
    {
        Serial.printf("Update of SET_I_VAR = %i\r\n",set_i_var);
        last_set_i_var = set_i_var;
    }

    if(WiFi.status() != wifi_status)
    {
        wifi_status = WiFi.status();
        String ip = WiFi.localIP().toString();
        Serial.printf("WIFI Status changed to %i\r\n", wifi_status);
        Serial.print("Current IP: "); Serial.print(ip);Serial.println("");
    }

}