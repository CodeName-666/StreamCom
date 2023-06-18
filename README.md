# StreamCom
 
This library allows you to integrate a simple interface, CMD similar. It allows you to configure different commands, parameters and if needed also callback for interaction between the command line and the arduino software.
The library uses the Arduino Stream interface, which allows you to use each communication interface which depends on the Stream class. Therefore it is really easy to switch between communication interfaces 
 
 
## Usage of different Streams
 
As mentioned, the StreamCom Library use the Arduino Stream-Class as communication interface, therefore each communication interface which drives from `Stream` can be used to communicate with this library
 
 
```c++
 
 
/*TelnetStream.h*/
class TelnetStreamClass : public Stream 
{
    ...
};
---
/*HardwareSerial.h*/
class HardwareSerial: public Stream
{
    ...
};
 
 
--- 
 
/*main.cpp*/
#include <Arduino.h>
#include "StreamCom.h"
#include "TelnetStream.h"
 
 
StreamCom streamComSerial;          //Communicate over Serial/USART
StreamCom streamComTelnet;          //Communicate over WIFI
 
 
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
 
```
 
## Configuration of StreamCom
 
The library depends on the configuration of the commands. This needs to be done once and handover to the StreamCom. For the configuration, a parameter of type `ParamList_t` needs to be defined.
 
 
### Configuration struct Service_t
 
**- Command:**

Each entry needs a `token` which represents the command of a service. 
 
**- Parameter:**

Each command can contain a set of parameters, which will be stored in the array `params`. There are a max number of allowed parameter which is defined over the macro `STREAM_COM_MAX_PARAMETER` and is **4**as default. 
 
If needed, a new definition of `STREAM_COM_MAX_PARAMETER` can be done to change the maximum number of parameter.
 
**- Parameter Types:**

For each parameter which was configured in the `params` pointer, a type needs to be defined. This is needed,because the StreamCom Library needs to which type a parameter has. 
 
The following types are available: 
 
 
```c++
enum Types_e{
   I8  = 0,  //!< Type int8_t
   I16    ,  //!< Type int16_t
   I32    ,  //!< Type int32_t
   I64    ,  //!< Type int64_t
   F      ,  //!< Type float
   D      ,  //!< Type double
   STR    ,  //!< Type String
   NONE      //!< No type/Not used
};
 
```
 
**- Number of Parameter**

This number shall include the number of configured parameter. This value can differ from the max allowed number of parameter, but it has to be smaller than the max. number.
 
 
**- Callback Function [Optional]**

For each service an own callback can be configured. Each time the service was called, this callback will be executed. The callback is defined as following:
 
```c++
typedef void (*StreamCom_Callback)(Stream* stream, void* args, uint32_t nParams);
```
The configuration can looks like this example: 
 
```c++
void foo(Stream* stream, void* args, uint32_t nParams) {}
```
The callback is optional. All parameters which will be sent to the StreamCom Library will be read and converted to the appropriate data type and updated to the corresponding parameter of the configuration.
 
### Example of a StreamCom Service_t configuration:
 
This example shows a set of three services: 
```c++
/*=== Command and Parameter definition =============================================*/
Service_t paramlist[NUMBER_OF_COMMANDS] = {
/*-----|  CMD  |      Param Ptr List         |     Param Type List  | NrPar| Clbk |-*/
/*[0]*/{"SET_I", {&set_i_var,NULL ,NULL, NULL} ,{I32 ,NONE,NONE,NONE},   1 , NULL},
/*[1]*/{"PID"  , {&p        ,&i   ,&d  , NULL} ,{I32 ,F   ,F   ,NONE},   3 , set_pid}
};
 
```
1.  **Service:** *SET_I*

    - Command: "RESET"
    - Parameter: 1 parameter - &set_i_var
    - Parameter Type: 1 times "I32", 3 times "NONE"
    - Number Parameter: 1 
    - Callback: Not used... 
 
...
 
### Defaulte Services:

There are some default services configured, which can be used from the beginning:

    - RESET - Creates a SW - Reset on the ECU
    - HELP  - Gives us detailed information about the configured/added Services
    - NUM   - Returns the number of Services

With futher updates there will be futher new default services. 
 
## Integration of the StreamCom Library: 
 
The integration is quite simple. The biggest task is the definition of the parameter list. After the definitions was done, the integration of the StreamCom library can be done with two function calls
 
### Integration Example:
 
```c++
 
void setup(void) 
{
    /*.... Usage of SteamCom over Serial ...*/
    Serial.begin(115200);
    streamComSerial.init(Serial,paramlist,NUMBER_OF_COMMANDS);                  //Init of the StreamCom Library 
}
 
 
void loop(void) 
{
    streamComSerial.loop();                                                     //Cyclic call of the main function to handle the requests.
}
 
 
```

