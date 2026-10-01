/** @file main.cpp @brief Serial command example for the supported Arduino boards. */
#include <Arduino.h>
#include "StreamCom.h"


/** @brief Number of application commands. */
#define NUMBER_OF_COMMANDS      2

/* Global Parameter definition for StreamCom usage */
int32_t gIntegerSetting = 0;
int32_t gProportionalGain = 0;
float gIntegralGain = 0.0f;
float gDerivativeGain = 0.0f;


/*==== CALLBACKS for COMMANDOS ===================== */
/**
 * @brief Print the values received by the PID command.
 *
 * @param[in,out] pStream Response stream.
 * @param[in] pCallbackArguments Targets for proportional, integral and derivative gain
 * @param[in] parameterCount 3
 */
void printPidValues(Stream* pStream, void* pCallbackArguments, uint32_t parameterCount)
{
    if ((pStream != nullptr) && (pCallbackArguments != nullptr) && (parameterCount == 3U))
    {
        (void)pStream->print("PID: ");
        (void)pStream->print(STREAMCOM_GET_VALUE(int32_t, pCallbackArguments, 0U));
        (void)pStream->print(';');
        (void)pStream->print(STREAMCOM_GET_VALUE(float, pCallbackArguments, 1U));
        (void)pStream->print(';');
        (void)pStream->println(STREAMCOM_GET_VALUE(float, pCallbackArguments, 2U));
    }
}


/*=== Commando and Parameter defintion =============================================*/
Service_t gServices[NUMBER_OF_COMMANDS] = {
/*-----|  CMD  |      Param Ptr List         |     Param Type List  | NrPar| Clbk |-*/
/*[0]*/{"SET_I", {&gIntegerSetting}, {I32}, 1, NULL},
/*[1]*/{"PID", {&gProportionalGain, &gIntegralGain, &gDerivativeGain}, {I32, F, F}, 3, printPidValues}
};


StreamCom gSerialCommands;



/**
 * @brief Initialize the example. Usage of StreamCom:
 *
 * a. Connect over Serial at 115200 baud.
 * b. Enter of Commands the following way (here for PID Command):
 *      PID=15;0.12;0.23
 *
 * c. Result: PID Callback [printPidValues] should be called and executed with
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
    gSerialCommands.init(Serial,gServices,NUMBER_OF_COMMANDS);


}


/** @brief Process available serial input without waiting for a complete line. */
void loop(void)
{
    gSerialCommands.loop();

}
