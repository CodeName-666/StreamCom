# StreamCom
 
This library allows you to integrate a simple CMD-like interface. It enables you to configure different commands, parameters, and, if needed, callbacks for interaction between the command line and the Arduino software. The library uses the Arduino Stream interface, making it easy to switch between communication interfaces.
 
## Usage of different Streams
 
As mentioned, the StreamCom Library uses the Arduino Stream class as the communication interface. Therefore, any communication interface that derives from `Stream` can be used to communicate with this library.
 
```c++
/*TelnetStream.h*/
class TelnetStreamClass : public Stream 
{
    ...
};

/*HardwareSerial.h*/
class HardwareSerial : public Stream
{
    ...
};
```
 
```cpp
/*main.cpp*/
#include <Arduino.h>
#include "StreamCom.h"
#include "TelnetStream.h"
 
StreamCom streamComSerial;          // Communicate over Serial/USART
StreamCom streamComTelnet;          // Communicate over WiFi
 
void setup(void) 
{
    /*.... Usage of StreamCom over Serial ...*/
    Serial.begin(115200);
    streamComSerial.init(Serial, paramlist, NUMBER_OF_COMMANDS);
 
    /*.... Usage of StreamCom over TelNet ...*/
    wifi_status = WiFi.begin(ssid, password);
    WiFi.setAutoConnect(true);
 
    TelnetStream.begin();
    streamComTelnet.init(TelnetStream, paramlist, NUMBER_OF_COMMANDS);
}
```
 
## Configuration of StreamCom
 
The library depends on the configuration of the commands. This needs to be done once and handed over to StreamCom. For the configuration, a parameter of type `Service_t` needs to be defined.
 
### Configuration struct Service_t
 
**- Command:**
 
Each entry needs a `token` which represents the command of a service.
 
**- Parameter:**
 
Each command can contain a set of parameters, which will be stored in the `params` array. There is a maximum number of allowed parameters defined by the macro `STREAM_COM_MAX_PARAMETER`, which defaults to **4**.
 
If needed, you can redefine `STREAM_COM_MAX_PARAMETER` to change the maximum number of parameters.
 
**- Parameter Types:**
 
For each parameter configured in the `params` pointer, a type needs to be defined. This is necessary because the StreamCom Library needs to know the type of each parameter.
 
The following types are available: 
 
```c++
enum Types_e {
    I8,   //!< Type int8_t
    I16,  //!< Type int16_t
    I32,  //!< Type int32_t
    I64,  //!< Type int64_t
    F,    //!< Type float
    D,    //!< Type double
    STR,  //!< Type String
    NONE  //!< No type/Not used
};
```
 
**- Number of Parameters**
 
This number should include the number of configured parameters. This value can differ from the maximum allowed number of parameters but must be smaller than the maximum number.
 
**- Callback Function [Optional]**
 
For each service, you can configure an optional callback. Each time the service is called, this callback will be executed. The callback is defined as follows:
 
```c++
typedef void (*StreamCom_Callback)(Stream* stream, void* args, uint32_t nParams);
```
 
The configuration can look like this example:
 
```c++
void foo(Stream* stream, void* args, uint32_t nParams) {}
```
 
The callback is optional. All parameters sent to the

 StreamCom Library will be read and converted to the appropriate data type and updated to the corresponding parameter of the configuration.
 
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
 
0.  **Service:** *SET_I*
 
    - Command: "SET_I"
    - Parameter: 1 parameter - &set_i_var
    - Parameter Type: 1 times "I32", 3 times "NONE"
    - Number Parameter: 1
    - Callback: Not used...

1.  **Service:** *PID*
 
    - Command: "PID"
    - Parameter: 3 parameter - &p | &i | &D
    - Parameter Type: 1 times "I32", 2 times "F" , 1 times "NONE"
    - Number Parameter: 3
    - Callback: set_pid
 
...
 
### Default Services:
 
There are some default services configured, which can be used from the beginning:
 
- RESET - Creates a SW - Reset on the ECU
- HELP  - Gives us detailed information about the configured/added Services
- NUM   - Returns the number of Services
 
With further updates, there will be further new default services.
 
## Integration of the StreamCom Library:
 
The integration is quite simple. The biggest task is the definition of the parameter list. After the definitions are done, the integration of the StreamCom library can be done with two function calls.
 
### Integration Example:
 
```c++
 
void setup(void) 
{
    /*.... Usage of StreamCom over Serial ...*/
    Serial.begin(115200);
    streamComSerial.init(Serial, paramlist, NUMBER_OF_COMMANDS);                  //Init of the StreamCom Library 
}
 
 
void loop(void) 
{
    streamComSerial.loop();                                                     //Cyclic call of the main function to handle the requests.
}
 
```
