/*
 * Stupid Simple JSON was developed mainly for educational purposes. 
 * However, this program can be used for simple projects where reliability 
 * and efficiency is not a priority.
 *
 * Copyright 2019 Jussi Kallio
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software 
 * is furnished to do so.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "JSONElement.h"

namespace ssj
{
	class JSON
	{
	public:

		// All possible errors that can be thrown
		typedef enum
		{
			E_NO_ERROR = 0,								// No error
			E_INVALID_JSON = 1001,						// Invalid JSON file

			E_LEXING_ERR_INVALID_VALUE = 1101,			// Lexing error: Failed to parse JSON value
			E_LEXING_ERR_INVALID_STRING = 1102,			// Lexing error: Failed to parse JSON string literal
			
			E_INVALID_JSON_KEY_STRING = 1201,			// Valid JSON key string must be surrounded with double quotes
			E_INVALID_JSON_KEY_COLON = 1202,			// Valid JSON key string must be followed by colon character ':'

			E_PARSE_ERR_INVALID_STRING = 1301,			// Parsing error: Valid string literal must have surrounding double quotes
			E_PARSE_ERR_UNKNOWN_ELEMENT = 1302,			// Parsing error: Failed to recognize JSON element
			E_PARSE_ERR_INVALID_NUMBER = 1303,			// Parsing error: Failed to convert string to valid number
			E_PARSE_ERROR_INVALID_LITERAL_CASE = 1304,	// Parsing error: Invalid case in literal value

			E_PARSE_ERR_INVALID_OBJECT = 1401,			// Parsing error: Invalid JSON object element
			E_PARSE_ERR_OBJECT_OPENING_BRACKET = 1402,	// Parsing error: Expected opening bracket '{'
			E_PARSE_ERR_OBJECT_CLOSING_BRACKET = 1403,	// Parsing error: Expected closing bracket '}'

			E_PARSE_ERR_INVALID_ARRAY = 1501,			// Parsing error: Invalid JSON array element
			E_PARSE_ERR_ARRAY_OPENING_BRACKET = 1502,	// Parsing error: Expected opening bracket '['
			E_PARSE_ERR_ARRAY_CLOSING_BRACKET = 1503	// Parsing error: Expected opening bracket ']'
		} Error;

		// Construction
		JSON(const std::string& json, /*out*/ Error* pErr = nullptr);
		~JSON();

		// Public Methods
		ObjectElementPtr GetRootObject() { return m_pRootObject; }
		static const char* ErrorToSring(Error e);
		void SetAcceptCaseInsensitiveLiterals(bool bAccept) { m_bAcceptCaseInsensitiveLiterals = bAccept;  }

	private:

		// Private JSON parsing function
		std::vector<std::string> Lex(const std::string& str);
		std::vector<std::string>::iterator Parse(const std::vector<std::string>::iterator begin, const std::vector<std::string>::iterator end, /*out*/ JSONElementPtr& pOutElement, const std::string& key);
		std::vector<std::string>::iterator ParseObjectElement(const std::vector<std::string>::iterator begin, const std::vector<std::string>::iterator end, /*out*/ ObjectElementPtr& pOutObject, const std::string& key);
		std::vector<std::string>::iterator ParseArrayElement(const std::vector<std::string>::iterator begin, const std::vector<std::string>::iterator end, /*out*/ ArrayElementPtr& pOutArray, const std::string& key);
		std::vector<std::string>::iterator ParseStringElement(const std::vector<std::string>::iterator it, /*out*/ StringElementPtr& pOutString, const std::string& key);
		std::vector<std::string>::iterator ParseNumberElement(const std::vector<std::string>::iterator it, /*out*/ NumberElementPtr& pOutNumber, const std::string& key);
		std::vector<std::string>::iterator ParseKey(const std::vector<std::string>::iterator begin, const std::vector<std::string>::iterator end, /*out*/ std::string& outKey);

		// Private helper functions
		bool IsValidNumber(const std::vector<std::string>::iterator it);
		bool IsValidDecimal(const std::vector<std::string>::iterator it);
		size_t FindClosingQuotationMark(const std::string& str, size_t offset);
		size_t FindClosingCharacter(const std::string& str, size_t offset);
		static bool CharCompare(const char& c1, const char& c2);
		static bool EqualIgnoreCase(const std::string& str1, const std::string& str2);

	private:
		
		// Private members
		ObjectElementPtr	m_pRootObject;
		bool				m_bAcceptCaseInsensitiveLiterals;
	};
}

