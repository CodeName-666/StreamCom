/*
 * StreamCom.cpp
 *
 *  Created on: 12.11.2021
 *      Author: c.seidel
 */



#include "StreamCom.h"


/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
StreamCom::StreamCom(void) :
		m_cmdDelimiter(STREAM_COM_CDM_DELIMITER),
		m_paramDelimiter(STREAM_COM_PARAM_DELIMITER),
		m_stream(NULL)
{

}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
void StreamCom::loop(void)
{

	String str,v1,v2;
	bool status = false;
	bool found = false;
	bool is_verified = false;

	if (m_stream->available() > 0)
	{
		str = m_stream->readString();
		is_verified = stringVerify(&str);
		
		if(is_verified)
		{
			/*Split the read String into two parts........*/
			stringSplit(&str,&v1,m_cmdDelimiter); /*Set Split string as initialization*/
			stringSplit(NULL,&v2,m_cmdDelimiter); /*Set NULL to use the given string for the next part*/
					
			for (uint8_t i = 0; i < m_list_size; i++)
			{
				if (v1.equals(m_paramList[i].token))
				{
					if(m_paramList[i].nParams != 0)
					{
						status = executeCommand(&v2,i);
					}
					else
					{
						status = executeCommand(NULL,i);
					}
					found = true;
				}
			}

			if (status == false && found == true)
			{
				m_stream->println(F("...ERROR: CANNOT EXECUTE FUNCTION..."));
			}
			else if(status == false && found == false)
			{
				m_stream->print(F("...UNKNOWN TOKEN - ")); m_stream->print(str);
				m_stream->print(F(" - Status = ")); m_stream->print(status);
				m_stream->print(F(" - Found = ")); m_stream->print(found);
				m_stream->println("");


			}
			else
			{
				/*... DO NOTHING...*/
			}
		}
		else 
		{
			m_stream->println(F("...EMPTY STRING RECEIVED ..."));
		}

	}
	return;
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
void StreamCom::init(Stream & stream, ParamList_t* paramList, uint16_t size)
{
	m_stream = &stream;
	m_paramList = paramList;
	m_list_size = size;

}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
bool StreamCom::executeCommand(String* paramStr, uint8_t paramListIdx)
{
	bool status = false;

	if(paramStr != NULL)
	{
		status = splitParameter(paramStr,paramListIdx);
		if(status == true)
		{
			convertParameter(paramListIdx);
		}
	}
	else 
	{
		status = !paramsAvailable(paramListIdx);
	}

	executeCallback(paramListIdx);
	return status;
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
void StreamCom::stringSplit(String* strToSplit, String* strToStore, const char* delimiter)
{
	char* avPtr = NULL;
	if(strToStore != NULL)
	{
		if(strToSplit != NULL)
		{

			avPtr = strtok((char*)strToSplit->c_str(), delimiter);
		}
		else
		{
			avPtr = strtok(NULL, delimiter);
		}

		if(avPtr != NULL)
		{
			*strToStore = String(avPtr);
		}
		
	}

	return;
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
void StreamCom::executeCallback(uint8_t paramListIdx)
{
	ParamList_t* entry = (ParamList_t*)&m_paramList[paramListIdx];
	if(entry->callback != nullptr)
	{
		entry->callback(m_stream, entry->params,entry->nParams);
	}
	return;
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
bool StreamCom::splitParameter(String* paramStr, uint8_t paramListIdx)
{
	bool firstCall = true;
	bool ret = false;

	if ( (m_paramList[paramListIdx].nParams <= STREAM_COM_MAX_PARAMETER) && 
	     (m_paramList[paramListIdx].nParams > 0))
	{
		for (uint32_t i = 0; (i < m_paramList[paramListIdx].nParams); i++)
		{
			m_params[i]= "";

			if(firstCall == true)
			{
				stringSplit(paramStr,&m_params[i],m_paramDelimiter);
				firstCall = false;
			}
			else
			{
				stringSplit(NULL,&m_params[i],m_paramDelimiter);
			}
		}
		ret = true;
	}
	else if(m_paramList[paramListIdx].nParams == 0)
	{
		/*Nothing needs to be done. Skip this code...*/
		ret = false;
	}
	else
	{
		m_stream->println(F("...NUMBER OF PARAMETER OUT OF BOUNDS..."));
		ret = false;
	}
	return ret;
}


/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
template<typename T> T StreamCom::convert(Types_e type, uint8_t paramIdx)
{
	switch(type)
	{
		case I8 :
 		case I16:
 		case I32:
		case I64:
			return static_cast<T>(m_params[paramIdx].toInt());
		case F:
		case D:
			return static_cast<T>(m_params[paramIdx].toDouble());
		case STR:
		default:
			return static_cast<T>(0); //Return converted 0 because String don't need to be converted.
	}
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
void StreamCom::convertParameter(uint8_t paramListIdx)
{	
	ParamList_t* entry = (ParamList_t*)&m_paramList[paramListIdx];
	if(paramListIdx < m_list_size)
	{
		for(uint8_t i = 0; i < entry->nParams; i++)
		{
			uint8_t index = i;
			switch(entry->paramTypes[i])
			{
				case I8  :
				{
					int8_t* i8 = static_cast<int8_t*>(entry->params[index]);
					*i8 = convert<int8_t>(I8 ,i);
					break;
				}
				case I16 :
				{
					int16_t* i16 = static_cast<int16_t*>(entry->params[index]);
					*i16 = convert<int16_t>(I16,index);
					break;
				}
				case I32 :
				{
					int32_t* i32 = static_cast<int32_t*>(entry->params[index]);
					*i32 = convert<int32_t>(I32,index);
					break;
				}
				case I64 :
				{
					int64_t* i64 = static_cast<int64_t*>(entry->params[index]);
					*i64 = convert<int64_t>(I64,index);
					break;
				}
				case F   :
				{
					float* f = static_cast<float*>(entry->params[index]);
					*f = convert<float>(F  ,index);
					break;
				}
				case D   :
				{
					double * d = static_cast<double*>(entry->params[index]);
					*d = convert<double>(D  ,index);
					break;
				}
				case STR : /* String is a special case. No convertion needed.*/
				{
					String* str = static_cast<String*>(entry->params[index]);
					*str = String(m_params[index]);
					break;
				}
				case NONE:
				default:
				{

					break;
				}
			}
		}
	}
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
bool StreamCom::paramsAvailable(uint8_t paramListIdx)
{
	return m_paramList[paramListIdx].nParams > 0 ? true : false; 
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
bool StreamCom::stringVerify(String* readString)
{
	bool ret = false;
	bool cr_lf_eos;
	if(readString != NULL)
	{
		readString->trim();
		cr_lf_eos = readString->endsWith("\r\n");
		if(cr_lf_eos)
		{
			readString->remove(readString->length()-3,2);
		}

		if(!readString->isEmpty())
		{
			ret = true;
		}
	}
	return ret;
}
