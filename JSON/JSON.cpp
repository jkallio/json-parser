#include "stdafx.h"
#include <iostream>
#include <cstddef>
#include <algorithm>
#include <string>
#include <cctype>
#include <memory>
#include "JSON.h"

using namespace ssj;
using std::string;
using std::vector;

JSON::JSON(const string& json, Error* pErr)
{	// Takes JSON string as input and stores the parsed JSON structure as member.
	// May throw JSON::EError in case an error is occured.
	// If pErr output parameter is passed, it will be populated instead of throwing a JSON::EError.

	if (pErr != nullptr)
		*pErr = E_NO_ERROR;

	// According to RFC 7159 literal names MUST be lowercase.
	m_bAcceptCaseInsensitiveLiterals = true;

	try
	{
		auto tokens = Lex(json);
		JSONElementPtr pRoot = nullptr;
		auto it = Parse(tokens.begin(), tokens.end(), pRoot, "");
		if (pRoot != nullptr && pRoot->IsObject())
		{
			m_pRootObject = std::dynamic_pointer_cast<ObjectElement>(pRoot);
		}
		else
		{
			throw E_INVALID_JSON;
		}
	}
	catch (Error e)
	{
		if (pErr != nullptr)
		{
			*pErr = e;
		}
		else
		{
			throw;
		}
	}
}

JSON::~JSON()
{
}

vector<string> JSON::Lex(const string& str)
{	// Returns tokenized list of JSON keys/values and structural elements.
	// Structural characters, such as brackets and commas, are returned as separate items.
	// Whitespaces are removed the input JSON string.
	// String literals and keys will still include double quotes in the returned list.

	vector<string> tokens;
	for (unsigned int pos = 0; pos < str.length(); ++pos)
	{
		switch (str[pos])
		{
		// Ignorable whitespaces
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			break;

		// JSON structural characters
		case '{':
		case '}':
		case '[':
		case ']':
		case ':':
		case ',':
			tokens.push_back(str.substr(pos, 1));
			break;

		// JSON String literals
		case '\"':
			{
				size_t endPos = FindClosingQuotationMark(str, pos+1);
				if (endPos != string::npos)
				{
					tokens.push_back(str.substr(pos, endPos - pos + 1));
					pos = endPos;
				}
				else
				{
					throw E_LEXING_ERR_INVALID_STRING;
				}
			} break;

		// JSON values (numbers, booleans, nulls)
		default:
			{
				size_t endPos = FindClosingCharacter(str, pos);
				if (endPos != string::npos)
				{
					tokens.push_back(str.substr(pos, endPos - pos));
					pos = endPos - 1;
				}
				else
				{
					throw E_LEXING_ERR_INVALID_VALUE;
				}
			} break;
		}
	}
	return tokens;
}

vector<string>::iterator JSON::Parse(const vector<string>::iterator begin, const vector<string>::iterator end, JSONElementPtr& pOutElement, const string& key)
{	// Parses the next JSON element in the list of tokenized JSON elements.
	// Returns the iterator to the last token of the parsed element (e.g. in case of JSON object this token will be '}')
	// shared_ptr of the parsed element is populated in the "pOutElement" argument.

	for (auto it = begin; it != end; ++it)
	{
		if (*it == "{")
		{
			ObjectElementPtr pObj = nullptr;
			it = ParseObjectElement(it, end, pObj, key);
			if (pObj != nullptr)
			{
				pOutElement = std::dynamic_pointer_cast<JSONElement>(pObj);
				return it;
			}
		}
		else if (*it == "[")
		{
			ArrayElementPtr pArr = nullptr;
			it = ParseArrayElement(it, end, pArr, key);
			if (pArr != nullptr)
			{
				pOutElement = std::dynamic_pointer_cast<JSONElement>(pArr);
				return it;
			}
		}
		else if (it->length() > 0 && it->front() == '\"')
		{
			StringElementPtr pStr = nullptr;
			it = ParseStringElement(it, pStr, key);
			if (pStr != nullptr)
			{
				pOutElement = std::dynamic_pointer_cast<JSONElement>(pStr);
				return it;
			}
		}
		else if (EqualIgnoreCase(*it, "true") || EqualIgnoreCase(*it, "false"))
		{	
			if (!m_bAcceptCaseInsensitiveLiterals && (*it != "true" && *it != "false"))
			{
				throw E_PARSE_ERROR_INVALID_LITERAL_CASE;
			}
			auto pBool = BoolElementPtr(new BoolElement(key));
			pBool->SetBoolean(it->front() == 't');
			pOutElement = std::dynamic_pointer_cast<JSONElement>(pBool);
			return it;
		}
		else if (EqualIgnoreCase(*it, "null"))
		{
			if (!m_bAcceptCaseInsensitiveLiterals && *it != "null")
			{
				throw E_PARSE_ERROR_INVALID_LITERAL_CASE;
			}
			auto pNull = NullElementPtr(new NullElement(key));
			pOutElement = std::dynamic_pointer_cast<JSONElement>(pNull);
			return it;
		}
		else if (IsValidNumber(it))
		{
			NumberElementPtr pNumber = nullptr;
			it = ParseNumberElement(it, pNumber, key);
			if (pNumber != nullptr)
			{
				pOutElement = std::dynamic_pointer_cast<JSONElement>(pNumber);
				return it;
			}
		}
		else
		{
			throw E_PARSE_ERR_UNKNOWN_ELEMENT;
		}
	}
	return end;
}

vector<string>::iterator JSON::ParseObjectElement(const vector<string>::iterator begin, const vector<string>::iterator end, ObjectElementPtr& pOutObject, const string& key)
{	// Parses the next JSON object element from tokenized list of JSON parameters.
	// Object element must begin with '{' and end with '}'. Otherwise JSON::Error is thrown.
	// All children are expected to be valid JSON key/value pairs. Otherwise JSON::Error is thrown.
	// Children elements must be separated with comma character (,). Otherwise JSON::Error is thrown.

	auto it = begin;
	if (*it != "{")
	{
		throw E_PARSE_ERR_OBJECT_OPENING_BRACKET; // Expected opening bracket
	}
	
	pOutObject = ObjectElementPtr(new ObjectElement(key));
	while (++it != end && *it != "}")
	{
		if (*it != ",")
		{
			string childKey = "";
			it = ParseKey(it, end, childKey);

			JSONElementPtr pChild = nullptr;
			it = Parse(it, end, pChild, childKey);

			pChild->SetParent(pOutObject);
			pOutObject->AddChild(pChild);

			if (*next(it) != "," && *next(it) != "}")
			{
				throw E_PARSE_ERR_INVALID_OBJECT; // Exptected comma separator or closing bracket
			}
		}
	}

	if (*it != "}")
	{
		throw E_PARSE_ERR_OBJECT_CLOSING_BRACKET; // Expected closing bracket
	}
	return it;
}

vector<string>::iterator JSON::ParseArrayElement(const vector<string>::iterator begin, const vector<string>::iterator end, ArrayElementPtr& pOutArray, const string& key)
{	// Parses the next JSON array element from tokenized list of JSON parameters.
	// Array element must begin with '[' and end with ']'. Otherwise JSON::Error is thrown.
	// All values in array must be valid JSON values and separated with comma character. Otherwise JSON::Error is thrown.
	// Array elements must not have keys. Otherwise JSON::Error is thrown.

	auto it = begin;
	if (*it != "[")
	{
		throw E_PARSE_ERR_ARRAY_OPENING_BRACKET; // Exptected opening bracket
	}

	pOutArray = ArrayElementPtr(new ArrayElement(key));
	while (++it != end && *it != "]")
	{
		if (*it != ",")
		{
			JSONElementPtr pItem = nullptr;
			it = Parse(it, end, pItem, "");

			pItem->SetParent(pOutArray);
			pOutArray->AddElement(pItem);

			if (*next(it) != "," && *next(it) != "]")
			{
				throw E_PARSE_ERR_INVALID_ARRAY; // Expected comma separator or closing bracket
			}
		}
	}

	if (*it != "]")
	{
		throw E_PARSE_ERR_ARRAY_CLOSING_BRACKET; // Expected closing bracket
	}

	return it;
}

vector<string>::iterator JSON::ParseStringElement(const vector<string>::iterator it, StringElementPtr& pOutString, const string& key)
{	// Parses the next JSON string value from tokenized list of JSON parameters.
	// String literal must be surrounded with double quotes. Otherwise JSON::Error is thrown.

	if (it->length() > 2 && it->front() == '\"' && it->back() == '\"')
	{
		pOutString = StringElementPtr(new StringElement(key));
		pOutString->SetString(it->substr(1, it->length() - 2));
	}
	else
	{
		throw E_PARSE_ERR_INVALID_STRING; // Valid string literal must have surrounding double quotes.
	}
	return it;
}

vector<string>::iterator JSON::ParseNumberElement(const vector<string>::iterator it, NumberElementPtr& pOutNumber, const string& key)
{	// Parses the next JSON number value from tokenized list of JSON parameters.
	// Numbers must be valid integers/decimals. Otherwise JSON::Error is thrown.
	
	try
	{
		pOutNumber = NumberElementPtr(new NumberElement(key));
		if (IsValidDecimal(it))
		{
			pOutNumber->SetDecimal(std::stold(*it));
			size_t pos = it->find_last_of('.');
			if (pos != string::npos)
			{
				pOutNumber->SetDecimalPrecision(it->length() - pos - 1);
			}
		}
		else
		{
			pOutNumber->SetInteger(std::stoll(*it));
		}
	}
	catch (const std::invalid_argument&) 
	{
		throw E_PARSE_ERR_INVALID_NUMBER;
	}
	return it;
}

vector<string>::iterator JSON::ParseKey(const vector<string>::iterator begin, const vector<string>::iterator end, string& outKey)
{	// Parses the next JSON key from tokenized list of JSON parameters.
	// Valid JSON key must be surrounded with double quotes. Otherwise JSON::Error is thrown.
	// Key must follow a colon character. Otherwise JSON::Error is thrown.

	auto it = begin;
	if (it != end && it->length() > 2 && it->front() == '\"' && it->back() == '\"')
	{
		outKey = it->substr(1, it->length() - 2);
		++it;
	}
	else
	{
		throw E_INVALID_JSON_KEY_STRING; // Valid JSON key must have surrounding double quotes
	}

	if (it == end || *it != ":")
	{
		throw E_INVALID_JSON_KEY_COLON; // Expecting ':' after JSON key
	}
	return next(it);
}

bool JSON::IsValidNumber(const vector<string>::iterator it)
{	// Checks if the number string contains only valid numeric characters
	// Number in JSON can be an integer, decimal, or exponent (like 1.0e+10)

	const string validChars = "1234567890+-.eE";
	for (const char& c1 : *it)
	{
		bool bFound = false;
		for (const char& c2 : validChars)
		{
			if (c1 == c2)
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			return false;
		}
	}
	return true;
}

bool JSON::IsValidDecimal(const vector<string>::iterator it)
{	// Returns true if the number is valid decimal number
	// TODO: Currently only searches for decimal separator

	for (const char& c : *it)
	{
		if (c == '.')
		{
			return true;
		}
	}
	return false;
}

size_t JSON::FindClosingQuotationMark(const string& str, size_t offset)
{	// Returns the position of next double quote character in JSON string literal (from offset)
	// Escaped double quotes are ignored '\"' 
	
	bool bFound = false;
	size_t endPos = 0;
	while (!bFound && endPos != string::npos)
	{
		endPos = str.find('\"', offset);
		if (endPos != string::npos)
		{
			// Ignore escaped quotations
			bFound = str[endPos - 1] != '\\';
			offset = endPos + 1;
		}
	}
	return endPos;
}

size_t JSON::FindClosingCharacter(const string& str, size_t offset)
{	// Returns the position of next value closing character in JSON string (from offset)
	// Whitespaces are also concidered as 'closing character'

	const string closingChars = "{}[], \r\n\t";
	for (size_t pos = offset; pos < str.length(); ++pos)
	{
		for (const char &c : closingChars)
		{
			if (str[pos] == c)
			{
				return pos;
			}
		}
	}
	return string::npos;
}

bool JSON::CharCompare(const char& c1, const char& c2)
{	// Helper for case-insensitive string compare.
	// Returns true if characters are the same (ignoring case)

	if (c1 == c2 || (std::toupper(c1) == std::toupper(c2)))
	{
		return true;
	}
	return false;
}

bool JSON::EqualIgnoreCase(const std::string& s1, const std::string& s2)
{	// Helper for case-insensitive string compare.
	// Returns true if strings are the same (ignoring case)

	return ((s1.size() == s2.size()) && std::equal(s1.begin(), s1.end(), s2.begin(), &JSON::CharCompare));
}

const char* JSON::ErrorToSring(Error e)
{	// Returns description for an error

	switch (e)
	{
	case E_NO_ERROR:			return "No error";
	case E_INVALID_JSON:		return "Invalid JSON file";

	case E_LEXING_ERR_INVALID_VALUE:	return "Lexing error: Failed to parse JSON value";
	case E_LEXING_ERR_INVALID_STRING:	return "Lexing error: Failed to parse JSON string literal";
	
	case E_INVALID_JSON_KEY_STRING:		return "Parsing error: Invalid key found (missing double quotes)";
	case E_INVALID_JSON_KEY_COLON:		return "Parsing error: Invalid key found (missing colon)";
	
	case E_PARSE_ERR_INVALID_STRING:	return "Parsing error: Invalid string found (missing double quotes)";
	case E_PARSE_ERR_UNKNOWN_ELEMENT:	return "Parsing error: Failed to recognize JSON element";
	case E_PARSE_ERR_INVALID_NUMBER:	return "Parsing error: Failed to convert number from string";
	case E_PARSE_ERROR_INVALID_LITERAL_CASE:	return "Parsing error: Invalid case in JSON literal name";

	case E_PARSE_ERR_INVALID_OBJECT:			return "Parsing error: Invalid JSON object element";
	case E_PARSE_ERR_OBJECT_OPENING_BRACKET:	return "Parsing error: Invalid JSON object element (missing opening bracket '{')";
	case E_PARSE_ERR_OBJECT_CLOSING_BRACKET:	return "Parsing error: Invalid JSON object element (missing closing bracket '}')";
		
	case E_PARSE_ERR_INVALID_ARRAY:				return "Parsing error: Invalid JSON array element";
	case E_PARSE_ERR_ARRAY_OPENING_BRACKET:		return "Parsing error: Invalid JSON array element (missing opening bracket '[')";
	case E_PARSE_ERR_ARRAY_CLOSING_BRACKET:		return "Parsing error: Invalid JSON object element (missing closing bracket ']')";
	}
	return "Unknown error";
}

