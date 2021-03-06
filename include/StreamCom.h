/*
 * StreamCom.h
 *
 *  Created on: 12.11.2021
 *      Author: c.seidel
 */

#ifndef StreamCom_H_
#define StreamCom_H_

#include "Arduino.h"

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
 * @brief Callback function for the StreamCom configuration
 */
typedef void (*StreamCom_Callback)(Stream* stream, void* args, uint32_t nParams);

enum Types_e{
   I8  = 0,  //!< Type int8_t
   I16    ,  //!< Type int16_t
   I32    ,	 //!< Type int32_t
   I64    ,  //!< Type int64_t
   F      ,	 //!< Type float
   D      ,	 //!< Type double
   STR    ,	 //!< Type String
   NONE		 //!< No type/Not used
};

typedef struct
{
	/**
	 * @brief String Command/Token to identify Input
	 * 							            
	 * Unique name the command. Each command needs a token followed by the @see STREAM_COM_CDM_DELIMITER 
	 * to be identifed correctly.
	 * Example when STREAM_COM_CDM_DELIMITER is "=": 
	 * Config......: tocken = "TestCmd"
	 * SerialInput.: TestCmd=....
	 */ 
	const char* token;	
	/**
	 * @brief Pointer to each parameter of this command
	 * 
	 * This array contains all pointer to the parameter used for this command. This can be
	 * each type defined in the @see Types_e enumerate.
	 * 
	 * Example: 3 Parameter of max. STREAM_COM_MAX_PARAMETER needed. 
	 * Needed Parameter: int8_t i, int16_t j, float k
	 * Code: params = {&i, &j, &k, nullptr}
	 * 
	 */
	void* params[STREAM_COM_MAX_PARAMETER];			//!< Pointer each parameter which should be uest for command
	Types_e paramTypes[STREAM_COM_MAX_PARAMETER];   //!< Type of each paramter, to convert string input correctly
	uint32_t nParams;								//!< Number of parameter used for this command
	StreamCom_Callback callback;					//!< Callback which should be executed

}ParamList_t;


class StreamCom
{

	public:
		StreamCom(void);
		void loop(void);
		void init(Stream & stream, ParamList_t* paramList, uint16_t size);

	private:
		void stringSplit(String* strToSplit, String* strToStore,const char *delimiter);
		bool stringVerify(String* readString);
		bool splitParameter(String* paramStr, uint8_t paramListIdx);
		void convertParameter(uint8_t paramListIdx);
		template<typename T> T convert(Types_e type, uint8_t paramIdx);
		bool executeCommand(String* paramStr, uint8_t paramListIdx);
		void executeCallback(uint8_t paramListIdx);
		bool paramsAvailable(uint8_t paramListIdx);

	private:
		ParamList_t* m_paramList;
		uint16_t m_list_size;
		String m_params[STREAM_COM_MAX_PARAMETER];

		const char* m_cmdDelimiter;
		const char* m_paramDelimiter;
		Stream* m_stream;


};
#endif /* StreamCom_H_ */
