/*
 * StreamCom.h
 *
 *  Created on: 12.11.2021
 *      Author: c.seidel
 */

#ifndef StreamCom_H_
#define StreamCom_H_

#include "Arduino.h"

#ifndef STREAM_COM_DEFAULT_LIST_ENABLE
	#define STREAM_COM_DEFAULT_LIST_ENABLE									true
#endif

#ifndef STREAM_COM_MAX_PARAMETER
	#define STREAM_COM_MAX_PARAMETER										  4u
#endif

#ifndef STREAM_COM_CDM_DELIMITER
	#define STREAM_COM_CDM_DELIMITER							             "="
#endif

#ifndef STREAM_COM_PARAM_DELIMITER
	#define STREAM_COM_PARAM_DELIMITER								    	 ";"
#endif 

/**
 * @brief Marco to get the parameter values of a implemented callback function
 * to get a paramter of the Configuration
 * 
 * @param RTYPE Type of the parameter. For example uint16_t, uint32_t etc.
 * @param PTR Pointer to the arguments of the callback. 
 * @param PNUM The number of the paramter. Max. is the configured number of prameter of an config entry.
 * 
 * Each implemented callback has three prameter. [stream, args, nPrams]. This macro allows you to get the value
 * of the args pointer. This pointer refers to the paramter, which where configured in the ParamList_t configuration. 
 * To make it easy to get the value, this macro should be used.
 * 
 */
#define STEAMCOM_GET_VALUE(RTYPE,PTR,PNUM)							\
		**(RTYPE**)(PTR + PNUM * sizeof(void*))

/**
 * @brief Marco to get the parameter pointer of a implemented callback function
 * to get a paramter of the Configuration
 * 
 * @param RTYPE Type of the parameter. For example uint16_t, uint32_t etc.
 * @param PTR Pointer to the arguments of the callback. 
 * @param PNUM The number of the paramter. Max. is the configured number of prameter of an config entry.
 * 
 * Each implemented callback has three prameter. [stream, args, nPrams]. This macro allows you to get the pointer to the values
 * of the args pointer. This pointer refers to the paramter, which where configured in the ParamList_t configuration. 
 * To make it easy to get the value, this macro should be used.
 * 
 */
#define STEAMCOM_GET_PTR(RTYPE,PTR,PNUM)							\
		*(RTYPE**)(PTR + PNUM * sizeof(void*))

/**
 * @brief Definition of the StreamCom callback function type.
 *
 * This typedef defines the function type for a StreamCom callback. A StreamCom callback is a function that can be registered
 * and invoked by the StreamCom class to handle specific commands or actions. It takes several parameters:
 *
 * @param stream  A pointer to the Stream object that is used for communication.
 * @param args    A pointer to the arguments that are passed to the callback. The type and structure of the arguments depend on
 *                the specific implementation of the callback.
 * @param nParams The number of parameters passed to the callback.
 *
 * 
 * The StreamCom callback function should have a `void` return type and match the defined function signature. It can be used
 * to perform custom actions based on the received command and its parameters. The implementation of the callback function
 * should handle the desired logic and utilize the provided parameters to accomplish the intended functionality.
 *
 * Example usage:
 * @code{.cpp}
 * // Define a callback function with the StreamCom_Callback type
 * void myCallback(Stream* stream, void* args, uint32_t nParams) {
 *     // Handle the callback logic here
 * }
 *
 * @endcode
 *
 * @see StreamCom
 */
typedef void (*StreamCom_Callback)(Stream* stream, void* args, uint32_t nParams);

/**
 * @brief Enumeration representing the supported data types in StreamCom.
 *
 * The Types_e enumeration represents the supported data types that can be used in the StreamCom class for parameter conversion.
 * Each enumerator corresponds to a specific data type.
 *
 * @note The enumerators are assigned specific integer values to represent the corresponding data types.
 *
 * @see ParamList_t
 */
enum Types_e {
    I8   = 0,   //!< Represents the data type int8_t.
    I16  ,      //!< Represents the data type int16_t.
    I32  ,      //!< Represents the data type int32_t.
    I64  ,      //!< Represents the data type int64_t.
    F    ,      //!< Represents the data type float.
    D    ,      //!< Represents the data type double.
    STR  ,      //!< Represents the data type String.
    RAW,        //!< Represents the data type Classes/Structs.
    NONE        //!< Represents no type or indicates that the type is not used.
};


/**
 * @brief Structure representing a parameter list for a command in StreamCom.
 *
 * The ParamList_t structure represents a parameter list for a specific command in the StreamCom class. It holds information
 * about the command, its parameters, and the associated callback function.
 *
 * @param token     A string command/token used to identify the input.
 *                  This unique name is used to correctly identify the command and should be followed by the
 *                  STREAM_COM_CDM_DELIMITER (as defined elsewhere) to ensure proper identification.
 *                  For example, if STREAM_COM_CDM_DELIMITER is "=", a command might have the format "token = value",
 *                  where "token" is the command's token and "value" is the associated value passed as input.
 *
 * @param params    An array of pointers to each parameter of the command.
 *                  This array contains pointers to the parameters used by the command. Each parameter can be of any type
 *                  defined in the Types_e enumeration.
 *                  For example, if a command requires three parameters of types int8_t, int16_t, and float,
 *                  the params array would be defined as follows: params = {&i, &j, &k, nullptr}, where i, j, and k are
 *                  variables representing the respective parameters.
 *
 * @param paramTypes An array of parameter types.
 *                  This array defines the type of each parameter to convert the string input correctly.
 *                  The types are defined using the Types_e enumeration.
 *
 * @param nParams   The number of parameters used by the command.
 *
 * @param callback  A callback function associated with the command.
 *                  The callback function is executed when the command is invoked. It has a void return type and matches
 *                  the StreamCom_Callback function type (as defined elsewhere).
 *                  The callback function should handle the desired logic and utilize the provided parameters to
 *                  accomplish the intended functionality.
 *
 * Example usage:
 * @code{.cpp}
 * // Define a callback function for a command
 * void myCallback(Stream* stream, void* args, uint32_t nParams, Stream* mThis) {
 *     // Handle the callback logic here
 * }
 *
 * // Define a parameter list for a command
 * ParamList_t myCommand = {
 *     .token = "TestCmd",
 *     .params = {&i, &j, &k, nullptr},
 *     .paramTypes = {I8, I16, F, NONE},
 *     .nParams = 3,
 *     .callback = &myCallback
 * };
 * @endcode
 *
 * @see StreamCom
 * @see StreamCom_Callback
 * @see Types_e
 */
typedef struct
{
    const char* token;
    void* params[STREAM_COM_MAX_PARAMETER];
    Types_e paramTypes[STREAM_COM_MAX_PARAMETER];
    uint32_t nParams;
    StreamCom_Callback callback;

} ParamList_t;



/**
 * @brief Klasse zur Kommunikation über einen Stream.
 */
class StreamCom
{
public:
    /**
     * @brief Konstruktor für die StreamCom-Klasse.
     */
    StreamCom(void);

    /**
     * @brief Haupt-Loop für die StreamCom-Klasse.
     */
    void loop(void);

    /**
     * @brief Initialisiert die StreamCom-Klasse.
     * @param stream Der Stream, über den die Kommunikation erfolgen soll.
     * @param paramList Die Parameterliste.
     * @param size Die Größe der Parameterliste.
     */
    void init(Stream &stream, ParamList_t* paramList, uint16_t size);

    void printHelp(void);

private:
    /**
     * @brief Teilt einen String anhand eines Trennzeichens.
     * @param strToSplit Der zu teilende String.
     * @param strToStore Der Ziel-String zum Speichern der geteilten Teile.
     * @param delimiter Das Trennzeichen.
     */
    void stringSplit(String* strToSplit, String* strToStore, const char* delimiter);

    /**
     * @brief Überprüft, ob ein String gültig ist.
     * @param readString Der zu überprüfende String.
     * @return True, wenn der String gültig ist, ansonsten False.
     */
    bool stringVerify(String* readString);

    /**
     * @brief Teilt einen Parameter-String in einzelne Parameter und speichert sie in der Parameterliste.
     * @param paramStr Der zu teilende Parameter-String.
     * @param paramListIdx Der Index in der Parameterliste, in den die Parameter gespeichert werden sollen.
     * @return True, wenn die Aufteilung erfolgreich war, ansonsten False.
     */
    bool splitParameter(String* paramStr, uint8_t paramListIdx);

    /**
     * @brief Konvertiert einen Parameter in den entsprechenden Typ.
     * @param paramListIdx Der Index des Parameters in der Parameterliste.
     */
    void convertParameter(uint8_t paramListIdx);

    /**
     * @brief Konvertiert einen Parameter in den angegebenen Typ.
     * @tparam T Der Typ, in den der Parameter konvertiert werden soll.
     * @param type Der Typ des Parameters.
     * @param paramIdx Der Index des Parameters in der Parameterliste.
     * @return Der konvertierte Parameter.
     */
    template <typename T>
    T convert(Types_e type, uint8_t paramIdx);

    /**
     * @brief Führt einen Befehl aus.
     * @param paramStr Der Befehls-String.
     * @param paramListIdx Der Index des Parameters in der Parameterliste.
     * @return True, wenn der Befehl erfolgreich ausgeführt wurde, ansonsten False.
     */
    bool executeCommand(String* paramStr, uint8_t paramListIdx);

    /**
     * @brief Ruft die Callback-Funktion für einen Parameter auf.
     * @param paramListIdx Der Index des Parameters in der Parameterliste.
     */
    void executeCallback(uint8_t paramListIdx);

    /**
     * @brief Überprüft, ob Parameter für einen bestimmten Index verfügbar sind.
     * @param paramListIdx Der Index des Parameters in der Parameterliste.
     * @return True, wenn Parameter verfügbar sind, ansonsten False.
     */
    bool paramsAvailable(uint8_t paramListIdx);

private:
    ParamList_t* m_paramList;                        /**< Die Parameterliste. */
    uint16_t m_list_size;                            /**< Die Größe der Parameterliste. */
    String m_params[STREAM_COM_MAX_PARAMETER];       /**< Die Parameter. */

    const char* m_cmdDelimiter;                      /**< Das Trennzeichen für Befehle. */
    const char* m_paramDelimiter;                    /**< Das Trennzeichen für Parameter. */
    Stream* m_stream;                                /**< Der Stream für die Kommunikation. */
};

#endif /* StreamCom_H_ */
