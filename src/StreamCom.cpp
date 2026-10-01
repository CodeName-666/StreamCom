/**
 * @file StreamCom.cpp
 * @brief Stream command parsing and service dispatch.
 *
 *  Created on: 12.11.2021
 *      Author: c.seidel
 */

#include "StreamCom.h"
#include <errno.h>
#include <float.h>
#include <math.h>

/** @brief Parse a signed decimal, including the full int64_t range on AVR.
 * @param[in] text Input. @param[out] value Result. @return True for a complete valid number. */
static bool parseInteger(const String &text, int64_t &value)
{
	const char *pCharacter = text.c_str();
	bool       isNegative  = false;
	uint64_t   magnitude   = 0U;
	uint64_t   limit       = 0U;
	bool       isValid     = false;

	if ((*pCharacter == '-') || (*pCharacter == '+'))
	{
		isNegative = (*pCharacter == '-');
		++pCharacter;
	}
	isValid = (*pCharacter != '\0');
	limit = static_cast<uint64_t>(INT64_MAX) + (isNegative ? 1U : 0U);
	while (isValid && (*pCharacter != '\0'))
	{
		const uint8_t digit = static_cast<uint8_t>(*pCharacter - '0');

		if ((*pCharacter < '0') || (*pCharacter > '9'))
		{
			isValid = false;
		}
		else
		{
			if (magnitude > (limit - digit) / 10U)
			{
				isValid = false;
			}
			else
			{
				magnitude = magnitude * 10U + digit;
			}
		}
		++pCharacter;
	}
	if (isValid)
	{
		if (isNegative && magnitude == static_cast<uint64_t>(INT64_MAX) + 1U)
		{
			value = INT64_MIN;
		}
		else
		{
			value = isNegative ? -static_cast<int64_t>(magnitude) : static_cast<int64_t>(magnitude);
		}
	}
	return isValid;
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
StreamCom::StreamCom(void) : m_serviceList(), m_parameters{}, m_commandDelimiter(STREAM_COM_CDM_DELIMITER),
							 m_parameterDelimiter(STREAM_COM_PARAM_DELIMITER),
							 m_stream(NULL), m_receiveBuffer{}, m_receivedByteCount(0U),
							 m_lastReceivedByteMs(0U), m_shouldDiscardInput(false), m_isProcessingInput(false)
{
#if STREAM_COM_DEFAULT_LIST_ENABLE == true
	for (uint16_t defaultServiceIndex = 0; defaultServiceIndex < STREAM_COM_DEFAULT_LIST_SIZE; defaultServiceIndex++)
	{
		m_serviceList.push_back(&StreamCom_default_list[defaultServiceIndex]);
	}
#endif
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
void StreamCom::loop(void)
{
	if ((m_stream != nullptr) && !m_isProcessingInput)
	{
		m_isProcessingInput = true;
		// Bound each call even when the producer continuously sends data.
		for (uint16_t consumedByteCount = 0U; (consumedByteCount < 64U) && (m_stream->available() > 0); ++consumedByteCount)
		{
			const int16_t receivedValue     = static_cast<int16_t>(m_stream->read());
			const char    receivedCharacter = static_cast<char>(receivedValue);

			if (receivedValue < 0)
			{
				break;
			}
			m_lastReceivedByteMs = static_cast<uint32_t>(millis());
			if ((receivedCharacter == '\r') || (receivedCharacter == '\n'))
			{
				if (!m_shouldDiscardInput && (m_receivedByteCount > 0U))
				{
					m_receiveBuffer[m_receivedByteCount] = '\0';
					processInput(m_receiveBuffer, m_receivedByteCount);
				}
				m_receivedByteCount = 0U;
				m_shouldDiscardInput = false;
			}
			else if (!m_shouldDiscardInput)
			{
				if ((receivedCharacter == '\0') || (m_receivedByteCount >= STREAM_COM_RX_BUFFER_SIZE - 1U))
				{
					m_shouldDiscardInput = true;
					m_receivedByteCount = 0U;
					(void)m_stream->println(F("...ERROR: INVALID OR OVERSIZE INPUT..."));
				}
				else
				{
					m_receiveBuffer[m_receivedByteCount++] = receivedCharacter;
				}
			}
		}
#if STREAM_COM_IDLE_TIMEOUT_MS > 0
		if ((m_stream->available() <= 0) &&
			(static_cast<uint32_t>(static_cast<uint32_t>(millis()) - m_lastReceivedByteMs) >= STREAM_COM_IDLE_TIMEOUT_MS))
		{
			if (!m_shouldDiscardInput && (m_receivedByteCount > 0U))
			{
				m_receiveBuffer[m_receivedByteCount] = '\0';
				processInput(m_receiveBuffer, m_receivedByteCount);
			}
			m_receivedByteCount = 0U;
			m_shouldDiscardInput = false;
		}
#endif
		m_isProcessingInput = false;
	}
}

void StreamCom::processInput(const char *pReceivedText, uint16_t receivedLength)
{

	String receivedCommand = pReceivedText == nullptr ? "" : pReceivedText;

	String commandToken{};
	String parameterText{};
	const char *pSeparator        = nullptr;
	uint16_t   delimiterOffset    = 0U;
	bool       isSplitComplete    = true;
	bool       isSuccessful       = false;
	bool       hasMatchingService = false;
	bool       isCommandValid     = false;

	if ((m_stream != nullptr) && (pReceivedText != nullptr) && (receivedCommand.length() == receivedLength))
	{
		isCommandValid = validateCommandText(&receivedCommand);

		if (isCommandValid)
		{
			// Split only once: a String parameter may itself contain '='.
			pSeparator = strpbrk(receivedCommand.c_str(), m_commandDelimiter);
			if (pSeparator != nullptr)
			{
				delimiterOffset = static_cast<uint16_t>(pSeparator - receivedCommand.c_str());
				commandToken = receivedCommand.substring(0U, delimiterOffset);
				parameterText = receivedCommand.substring(delimiterOffset + 1U);
				isSplitComplete = commandToken.length() == delimiterOffset &&
					parameterText.length() == receivedCommand.length() - delimiterOffset - 1U;
			}
			else
			{
				commandToken = receivedCommand;
				isSplitComplete = commandToken.length() == receivedCommand.length();
			}
			commandToken.trim();

			for (uint16_t serviceIndex = 0; isSplitComplete && serviceIndex < m_serviceList.size(); serviceIndex++)
			{
				if (commandToken.equals(m_serviceList[serviceIndex]->token))
				{
					if (m_serviceList[serviceIndex]->nParams != 0)
					{
						isSuccessful = executeCommand(&parameterText, serviceIndex);
					}
					else
					{
						parameterText.trim();
						isSuccessful = (parameterText.length() == 0U) && executeCommand(NULL, serviceIndex);
					}
					hasMatchingService = true;
					break; // A callback may change the registry; dispatch exactly once.
				}
			}

			if (isSuccessful == false && hasMatchingService == true)
			{
				(void)m_stream->println(F("...ERROR: CANNOT EXECUTE FUNCTION..."));
			}
			else if (isSuccessful == false && hasMatchingService == false)
			{
				(void)m_stream->print(F("...UNKNOWN TOKEN - "));
				(void)m_stream->print(receivedCommand);
				(void)m_stream->print(F(" - Status = "));
				(void)m_stream->print(isSuccessful);
				(void)m_stream->print(F(" - Found = "));
				(void)m_stream->print(hasMatchingService);
				(void)m_stream->println("");
			}
			else
			{
				/*... DO NOTHING...*/
			}
		}
		else
		{
			(void)m_stream->println(F("...EMPTY STRING RECEIVED ..."));
		}
	}
	return;
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
void StreamCom::init(Stream &stream, Service_t *pServiceList, uint16_t serviceCount)
{
	m_stream = &stream;
	m_serviceList.clear();
	m_receivedByteCount = 0U;
	m_shouldDiscardInput = false;
	m_lastReceivedByteMs = static_cast<uint32_t>(millis());
#if STREAM_COM_DEFAULT_LIST_ENABLE == true
	for (uint16_t serviceIndex = 0U; serviceIndex < STREAM_COM_DEFAULT_LIST_SIZE; ++serviceIndex)
	{
		m_serviceList.push_back(&StreamCom_default_list[serviceIndex]);
	}
#endif

	for (uint16_t serviceIndex = 0; (pServiceList != nullptr) && (serviceIndex < serviceCount); serviceIndex++)
	{
		addService(pServiceList[serviceIndex]);
	}
	if ((pServiceList == nullptr) && (serviceCount != 0U))
	{
		(void)m_stream->println(F("...ERROR: NULL SERVICE LIST..."));
	}
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
bool StreamCom::executeCommand(String *pParameterText, uint16_t serviceIndex)
{
	bool isSuccessful = false;

	if (pParameterText != NULL)
	{
		isSuccessful = splitParameters(pParameterText, serviceIndex);
		if (isSuccessful == true)
		{
			isSuccessful = convertParameters(serviceIndex);
		}
	}
	else
	{
		isSuccessful = !hasParameters(serviceIndex);
	}

	if (isSuccessful)
	{
		executeCallback(serviceIndex);
	}
	return isSuccessful;
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
void StreamCom::executeCallback(uint16_t serviceIndex)
{
	Service_t *pService           = m_serviceList[serviceIndex];
	void      *pCallbackArguments = pService->params;

	if (pService->callback != nullptr)
	{
#if STREAM_COM_DEFAULT_LIST_ENABLE == true
		for (uint16_t defaultServiceIndex = 0U; defaultServiceIndex < STREAM_COM_DEFAULT_LIST_SIZE; ++defaultServiceIndex)
		{
			if (pService == &StreamCom_default_list[defaultServiceIndex])
			{
				pCallbackArguments = this;
			}
		}
#endif
		pService->callback(m_stream, pCallbackArguments, pService->nParams);
	}
	return;
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
bool StreamCom::splitParameters(String *pParameterText, uint16_t serviceIndex)
{
	bool     isValid        = (pParameterText != nullptr) && (serviceIndex < m_serviceList.size());
	uint32_t parameterCount = 0U;
	uint16_t fieldStart     = 0U;

	if (isValid)
	{
		parameterCount = m_serviceList[serviceIndex]->nParams;
		isValid = (parameterCount > 0U) && (parameterCount <= STREAM_COM_MAX_PARAMETER);
		for (uint32_t parameterIndex = 0U; isValid && (parameterIndex < parameterCount); ++parameterIndex)
		{
			const char *pSeparator = strpbrk(pParameterText->c_str() + fieldStart, m_parameterDelimiter);

			const uint16_t fieldEnd = pSeparator == nullptr ? static_cast<uint16_t>(pParameterText->length()) :
				static_cast<uint16_t>(pSeparator - pParameterText->c_str());
			isValid = (fieldEnd > fieldStart) && ((pSeparator != nullptr) == (parameterIndex + 1U < parameterCount));
			if (isValid)
			{
				m_parameters[parameterIndex] = pParameterText->substring(fieldStart, fieldEnd);
				isValid = m_parameters[parameterIndex].length() == static_cast<uint16_t>(fieldEnd - fieldStart);
				fieldStart = static_cast<uint16_t>(fieldEnd + 1U);
			}
		}
	}
	return isValid;
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
template <typename T>
T StreamCom::convertNumericParameter(Types_e type, uint8_t parameterIndex)
{
	T       convertedValue = static_cast<T>(0);
	int64_t integerValue   = 0;

	switch (type)
	{
	case I8:
	case I16:
	case I32:
	case I64:
	{
		(void)parseInteger(m_parameters[parameterIndex], integerValue); // Already validated before committing.
		convertedValue = static_cast<T>(integerValue);
		break;
	}
	case F:
	case D:
		convertedValue = static_cast<T>(strtod(m_parameters[parameterIndex].c_str(), nullptr));
		break;
	case STR:
	case RAW:
	default:
		break; // Strings and RAW are not numeric conversions.
	}
	return convertedValue;
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
bool StreamCom::convertParameters(uint16_t serviceIndex)
{
	bool      isValid   = serviceIndex < m_serviceList.size();
	Service_t *pService = isValid ? m_serviceList[serviceIndex] : nullptr;

	// Validate every field before modifying any application value.
	for (uint32_t parameterIndex = 0U; isValid && (parameterIndex < pService->nParams); ++parameterIndex)
	{
		const Types_e parameterType  = pService->paramTypes[parameterIndex];
		String        &parameterText = m_parameters[parameterIndex];
		int64_t       integerValue   = 0;
		char          *pNumberEnd    = nullptr;
		double        numberValue    = 0.0;
		double        magnitude      = 0.0;

		if (parameterType <= D)
		{
			parameterText.trim();
		}
		if (parameterType <= I64)
		{
			isValid = parseInteger(parameterText, integerValue);
			switch (parameterType)
			{
				case I8: isValid = isValid && integerValue >= INT8_MIN && integerValue <= INT8_MAX; break;
				case I16: isValid = isValid && integerValue >= INT16_MIN && integerValue <= INT16_MAX; break;
				case I32: isValid = isValid && integerValue >= INT32_MIN && integerValue <= INT32_MAX; break;
				default: break; // I64 range is checked by parseInteger.
			}
		}
		else if ((parameterType == F) || (parameterType == D))
		{
			errno = 0;
			numberValue = strtod(parameterText.c_str(), &pNumberEnd);
			isValid = pNumberEnd != parameterText.c_str() && *pNumberEnd == '\0' && errno != ERANGE && isfinite(numberValue);
			if (isValid && (parameterType == F))
			{
				magnitude = fabs(numberValue);
				isValid = magnitude <= static_cast<double>(FLT_MAX) &&
					!((magnitude > 0.0) && (magnitude < static_cast<double>(FLT_MIN)));
			}
		}
		else if (parameterType == STR)
		{
			isValid = static_cast<String *>(pService->params[parameterIndex])->reserve(parameterText.length());
		}
		else
		{
			isValid = parameterType == RAW;
		}
	}
	if (isValid)
	{
		for (uint8_t parameterIndex = 0; parameterIndex < pService->nParams; parameterIndex++)
		{
			void *pTarget = pService->params[parameterIndex];

			switch (pService->paramTypes[parameterIndex])
			{
			case I8:
			{
				*static_cast<int8_t *>(pTarget) = convertNumericParameter<int8_t>(I8, parameterIndex);
				break;
			}
			case I16:
			{
				*static_cast<int16_t *>(pTarget) = convertNumericParameter<int16_t>(I16, parameterIndex);
				break;
			}
			case I32:
			{
				*static_cast<int32_t *>(pTarget) = convertNumericParameter<int32_t>(I32, parameterIndex);
				break;
			}
			case I64:
			{
				*static_cast<int64_t *>(pTarget) = convertNumericParameter<int64_t>(I64, parameterIndex);
				break;
			}
			case F:
			{
				*static_cast<float *>(pTarget) = convertNumericParameter<float>(F, parameterIndex);
				break;
			}
			case D:
			{
				*static_cast<double *>(pTarget) = convertNumericParameter<double>(D, parameterIndex);
				break;
			}
			case STR: /* String is a special case. No convertion needed.*/
			{
				*static_cast<String *>(pTarget) = m_parameters[parameterIndex];
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
	return isValid;
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
bool StreamCom::hasParameters(uint16_t serviceIndex)
{
	return m_serviceList[serviceIndex]->nParams > 0 ? true : false;
}

/*******************************************************************************
 *  FUNCTION:
 ******************************************************************************/
bool StreamCom::validateCommandText(String *pCommandText)
{
	bool isValid = false;

	if (pCommandText != NULL)
	{
		pCommandText->trim();

		if (pCommandText->length() > 0)
		{
			isValid = true;
		}
	}
	return isValid;
}

uint16_t StreamCom::getServiceQuantity(void)
{
	return static_cast<uint16_t>(m_serviceList.size());
}

/**
 * @brief Gibt eine Hilfe mit allen konfigurierten Parametern aus.
 */
void StreamCom::printHelp()
{
	if (m_stream != nullptr)
	{
		(void)m_stream->println("The following commands are available:");
		(void)m_stream->println("");
		for (uint16_t serviceIndex = 0; serviceIndex < m_serviceList.size(); serviceIndex++)
		{
			const Service_t &service = *m_serviceList[serviceIndex];

			(void)m_stream->print("Service: ");
			(void)m_stream->print(serviceIndex);
			(void)m_stream->println(" ---------");
			(void)m_stream->print("Command: ");
			(void)m_stream->println(service.token);

			if (service.nParams > 0)
			{
				(void)m_stream->println("Parameters:");

				for (uint8_t parameterIndex = 0; parameterIndex < service.nParams; parameterIndex++)
				{
					(void)m_stream->print("  - Parameter ");
					(void)m_stream->print(parameterIndex + 1);
					(void)m_stream->print(": ");

					switch (service.paramTypes[parameterIndex])
					{
					case I8:
						(void)m_stream->println("Signed 8-bit integer");
						break;
					case I16:
						(void)m_stream->println("Signed 16-bit integer");
						break;
					case I32:
						(void)m_stream->println("Signed 32-bit integer");
						break;
					case I64:
						(void)m_stream->println("Signed 64-bit integer");
						break;
					case F:
						(void)m_stream->println("Floating-point number");
						break;
					case D:
						(void)m_stream->println("Double-precision floating-point number");
						break;
					case STR:
						(void)m_stream->println("String");
						break;
					case RAW:
						(void)m_stream->println("Raw application context (not converted)");
						break;
					case NONE:
						(void)m_stream->println("No Parameter");
						break;
					default:
						(void)m_stream->println("Unknown type");
						break;
					}
				}
			}
			else
			{
				(void)m_stream->println("No parameters.");
			}
		}
	}
}

void StreamCom::addService(Service_t &service)
{
	bool isValid = (service.token != nullptr) && (service.nParams <= STREAM_COM_MAX_PARAMETER);

	if (isValid)
	{
		isValid = (*service.token != '\0') && (strpbrk(service.token, " \t\r\n") == nullptr) &&
			(strpbrk(service.token, m_commandDelimiter) == nullptr) && (strlen(service.token) < STREAM_COM_RX_BUFFER_SIZE) &&
			(findServiceIndex(service.token) < 0) && (m_serviceList.size() < UINT16_MAX);
	}
	for (uint32_t parameterIndex = 0U; isValid && parameterIndex < service.nParams; ++parameterIndex)
	{
		const Types_e parameterType = service.paramTypes[parameterIndex];

		isValid = (parameterType >= I8) && (parameterType <= RAW) &&
			((parameterType == RAW) || (service.params[parameterIndex] != nullptr));
	}
	if (isValid)
	{
		m_serviceList.push_back(&service);
	}
	else if (m_stream != nullptr)
	{
		(void)m_stream->println(F("...ERROR: INVALID OR DUPLICATE SERVICE..."));
	}
}

void StreamCom::deleteService(uint16_t serviceIndex)
{
	if (serviceIndex < m_serviceList.size())
	{
		(void)m_serviceList.erase(m_serviceList.begin() + serviceIndex);
	}
}

void StreamCom::deleteService(const char *pServiceToken)
{
	int32_t serviceIndex = findServiceIndex(pServiceToken);

	if (serviceIndex >= 0)
	{
		deleteService(static_cast<uint16_t>(serviceIndex));
	}
}

int32_t StreamCom::findServiceIndex(const char *pServiceToken)
{
	bool    hasMatchingService = false;
	int32_t serviceIndex       = -1;

	for (uint16_t candidateIndex = 0; (pServiceToken != nullptr && candidateIndex < m_serviceList.size() && hasMatchingService == false); candidateIndex++)
	{
		if (strcmp(m_serviceList[candidateIndex]->token, pServiceToken) == 0)
		{
			hasMatchingService = true;
			serviceIndex = candidateIndex;
		}
	}
	return serviceIndex;
}
