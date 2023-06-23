/*
 * StreamCom.cpp
 *
 *  Created on: 12.11.2021
 *      Author: c.seidel
 */



#include "StreamCom.h"




//TREAM_COM_DEFAULT_LIST_ENABLE == true
//n StreamCom* mThis;
//n Service_t StreamCom_default_list;
//#endif

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
StreamCom::StreamCom(void) :
		m_cmdDelimiter(STREAM_COM_CDM_DELIMITER),
		m_paramDelimiter(STREAM_COM_PARAM_DELIMITER),
		m_stream(NULL)
{
#if STREAM_COM_DEFAULT_LIST_ENABLE == true
	for(uint16_t i = 0; i < STREAM_COM_DEFAULT_LIST_SIZE; i++)
	{
		m_serviceList.push_back(&StreamCom_default_list[i]);
	}
#endif
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
					
			for (uint8_t i = 0; i < m_serviceList.size(); i++)
			{
				if (v1.equals(m_serviceList[i]->token))
				{
					if(m_serviceList[i]->nParams != 0)
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
void StreamCom::init(Stream & stream, Service_t* paramList, uint16_t size)
{
	m_stream = &stream;
	mThis = this;
	
	Serial.print("MTHIS: "); Serial.println((uint32_t)mThis);

	for(uint16_t i = 0; i < size; i++)
	{
		m_serviceList.push_back(&paramList[i]);
	}
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
	Service_t* entry = m_serviceList[paramListIdx];

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

	if ( (m_serviceList[paramListIdx]->nParams <= STREAM_COM_MAX_PARAMETER) && 
	     (m_serviceList[paramListIdx]->nParams > 0))
	{
		for (uint32_t i = 0; (i < m_serviceList[paramListIdx]->nParams); i++)
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
	else if(m_serviceList[paramListIdx]->nParams == 0)
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
		case RAW:
		default:
			return static_cast<T>(0); //Return converted 0 because String don't need to be converted.
	}
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
void StreamCom::convertParameter(uint8_t paramListIdx)
{	
	Service_t* entry = (Service_t*)m_serviceList[paramListIdx];
	if(paramListIdx < m_serviceList.size())
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
				case RAW: 
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
	return m_serviceList[paramListIdx]->nParams > 0 ? true : false; 
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

		if(readString->length() > 0)
		{
			ret = true;
		}
	}
	return ret;
}

uint16_t StreamCom::getServiceQuantity(void)
{
	return m_serviceList.size();
}


/**
 * @brief Gibt eine Hilfe mit allen konfigurierten Parametern aus.
 */
void StreamCom::printHelp()
{
    m_stream->println("The following commands are available:");
	m_stream->println("");
	m_stream->println("Service: 0 ---------"); 
    for (uint16_t i = 0; i < m_serviceList.size(); i++)
    {
        const Service_t& paramList = *m_serviceList[i];
        m_stream->print("Command: ");
        m_stream->println(paramList.token);

        if (paramList.nParams > 0)
        {
            m_stream->println("Parameters:");

            for (uint8_t j = 0; j < paramList.nParams; j++)
            {
                m_stream->print("  - Parameter ");
                m_stream->print(j + 1);
                m_stream->print(": ");

                switch (paramList.paramTypes[j])
                {
                    case I8:
                        m_stream->println("Signed 8-bit integer");
                        break;
                    case I16:
                        m_stream->println("Signed 16-bit integer");
                        break;
                    case I32:
                        m_stream->println("Signed 32-bit integer");
                        break;
                    case I64:
                        m_stream->println("Signed 64-bit integer");
                        break;
                    case F:
                        m_stream->println("Floating-point number");
                        break;
                    case D:
                        m_stream->println("Double-precision floating-point number");
                        break;
                    case STR:
                        m_stream->println("String");
                        break;
					case NONE: 
						m_stream->println("No Parameter");
						break;
                    default:
                        m_stream->println("Unknown type");
                        break;
                }
            }
        }
        else
        {
            m_stream->println("No parameters.");
        }

		m_stream->print("Service: "); m_stream->print((uint16_t)i+1);  m_stream->println(" ---------");
    }
}

void StreamCom::addService(Service_t& service)
{
	if(serviceExists(service.token) == -1)
	{
		m_serviceList.push_back(&service);
	}
}

void StreamCom::deleteService(uint16_t service_entry)
{
	if(service_entry < m_serviceList.size())
	{
		m_serviceList.erase(m_serviceList.begin() + service_entry);
	}
}

void StreamCom::deleteService(const char* service_token)
{
	int16_t service_num = serviceExists(service_token);
	if(service_num >= 0)
	{
		deleteService(service_num);
	}
}

int16_t StreamCom::serviceExists(const char* serviceToken)
{
	bool exists = false;
	int16_t service_num = -1;
	for(uint16_t i = 0; (i < m_serviceList.size() && exists == false) ; i++)
	{
		if(strcmp(m_serviceList[i]->token,serviceToken) == 0)
		{
			exists = true;
			service_num = i;
		}
	}
	return service_num;
}