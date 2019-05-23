#pragma once
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <list>

namespace ssj
{
	// Typdefines and forward declaration
	class JSONElement;
	class ObjectElement;
	class ArrayElement;
	class StringElement;
	class BoolElement;
	class NullElement;
	class NumberElement;
	typedef std::shared_ptr<JSONElement> JSONElementPtr;
	typedef std::shared_ptr<ObjectElement> ObjectElementPtr;
	typedef std::shared_ptr<ArrayElement> ArrayElementPtr;
	typedef std::shared_ptr<StringElement> StringElementPtr;
	typedef std::shared_ptr<BoolElement> BoolElementPtr;
	typedef std::shared_ptr<NullElement> NullElementPtr;
	typedef std::shared_ptr<NumberElement> NumberElementPtr;

	// JSONElement is the abstract base class for all JSON elements
	class JSONElement
	{
	protected:
		typedef enum _EType
		{	OBJECT_T,
			ARRAY_T,
			STRING_T,
			BOOLEAN_T,
			NUMBER_T,
			NULL_T
		} EType;

	protected:
		std::weak_ptr<JSONElement>	m_pParent;
		const EType					m_Type;
		const std::string			m_Key;

	public:
		JSONElement(EType type, const std::string& key) : m_Type(type), m_Key(key) { };
		virtual ~JSONElement() { };

		bool IsObject() { return m_Type == OBJECT_T; }
		bool IsArray() { return m_Type == ARRAY_T; }
		bool IsString() { return m_Type == STRING_T; }
		bool IsBool() { return m_Type == BOOLEAN_T; }
		bool IsNumber() { return m_Type == NUMBER_T; }
		bool IsNull() { return m_Type == NULL_T; }
		void SetParent(std::weak_ptr<JSONElement> pParent) { m_pParent = pParent; }
		std::weak_ptr<JSONElement> Parent() { return m_pParent; }
		virtual std::string ToString() const = 0;

	protected:

		inline std::string FormatKey() const
		{
			if (m_Key.length() > 0)
			{
				std::ostringstream out;
				out << "\"" << m_Key << "\"" << ":";
				return out.str();
			}
			return "";
		};
	};

	// ObjectElement represents the JSON objects
	//	- Objects are surrounded by curly braces {}
	//	- Objects are written in key/value pairs
	//		* Keys must be in string format (always surrounded by double quotes "")
	//		* Values must be a valid JSON data type (string, number, object, array, boolean or null)
	class ObjectElement : public JSONElement
	{
	private:
		std::list<JSONElementPtr>	m_Children;

	public:
		ObjectElement(const std::string& key) : JSONElement(JSONElement::OBJECT_T, key) { }
		virtual ~ObjectElement() { }

		void AddChild(JSONElementPtr pChild)
		{ 
			m_Children.push_back(pChild); 
		};

		virtual std::string ToString() const override
		{
			std::ostringstream out;
			out << FormatKey() << "{";
			bool bFirst = true;
			for (auto& child : m_Children)
			{
				out << (bFirst ? "" : ",") << child->ToString();
				bFirst = false;
			}
			out << "}";
			return out.str();
		};
	};

	// ArrayElement represents the JSON arrays
	//	- Array values are separated by commas (,)
	//	- Array values must be of type string, number, object, array, boolean or null.
	class ArrayElement : public JSONElement
	{
	private:
		std::vector<JSONElementPtr>	m_Elements;

	public:
		ArrayElement(const std::string& key) : JSONElement(JSONElement::ARRAY_T, key) { }
		virtual ~ArrayElement() { }

		void AddElement(JSONElementPtr pChild) 
		{ 
			m_Elements.push_back(pChild); 
		};
		
		virtual std::string ToString() const override
		{
			std::ostringstream out;
			out << FormatKey() << "[";
			bool bFirst = true;
			for (auto& element : m_Elements)
			{
				out << (bFirst ? "" : ",") << element->ToString();
				bFirst = false;
			}
			out << "]";
			return out.str();
		};
	};

	// StringElement represents the JSON string type
	//	- Strings in JSON must be written in double quotes (")
	class StringElement : public JSONElement
	{
	private:
		std::string m_Value;

	public:
		StringElement(const std::string& key) : JSONElement(JSONElement::STRING_T, key) { }
		virtual ~StringElement() {}

		void SetString(const std::string& str)
		{
			m_Value = str;
		}

		virtual std::string ToString() const override
		{
			std::ostringstream out;
			out << FormatKey() << "\"" << m_Value << "\"";
			return out.str();
		}
	};
	
	// NumberElement represents the JSON number type
	//	- Numbers in JSON must be an integer or floating point.
	class NumberElement : public JSONElement
	{
	private:
		typedef enum
		{
			TYPE_UNDEFINED,
			TYPE_INTEGER,
			TYPE_DECIMAL
		} EType;

		long long m_Integer;
		long double m_Decimal;
		int m_DecimalPrecision;
		EType m_Type;

	public:
		NumberElement(const std::string& key) : JSONElement(JSONElement::NUMBER_T, key), m_Type(TYPE_UNDEFINED), m_DecimalPrecision(0) { }
		virtual ~NumberElement() {}

		void SetInteger(long long n) 
		{ 
			m_Integer = n; 
			m_Type = TYPE_INTEGER; 
		}

		void SetDecimal(long double d) 
		{ 
			m_Decimal = d; 
			m_Type = TYPE_DECIMAL; 
		}

		void SetDecimalPrecision(int precision)
		{
			m_DecimalPrecision = precision;
		}

		virtual std::string ToString() const override
		{
			std::string str = FormatKey();
			switch (m_Type)
			{
			case TYPE_INTEGER:
				str += std::to_string(m_Integer);
				break;

			case TYPE_DECIMAL:
			{
				if (m_DecimalPrecision > 0)
				{
					std::ostringstream out;
					out.precision(m_DecimalPrecision);
					out << std::fixed << m_Decimal;
					str += out.str();
				}
				else
				{
					str += std::to_string(m_Decimal);
				}
			} break;

			default:
				break;
			}
			return str;
		}
	};

	// BoolElement represents the JSON boolean type
	//	- Boolean values in JSON must be true/false (written without double quotes)
	class BoolElement : public JSONElement
	{
	private:
		bool m_bValue;

	public:
		BoolElement(const std::string& key): JSONElement(JSONElement::BOOLEAN_T, key) { }
		virtual ~BoolElement() {}

		void SetBoolean(bool b)
		{ 
			m_bValue = b; 
		}

		virtual std::string ToString() const override
		{
			std::string str = FormatKey();
			str += m_bValue ? "true" : "false";
			return str;
		}
	};

	// NullElement represents the JSON null type
	//	- Values in JSON can be null (written without double quotes)
	class NullElement : public JSONElement
	{
	public:
		NullElement(const std::string& key) : JSONElement(JSONElement::NULL_T, key) { }
		virtual ~NullElement() {}
		virtual std::string ToString() const override
		{
			std::string str = FormatKey();
			str += "null";
			return str;
		}
	};
}

