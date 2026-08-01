//------------------------------------------------------------------------------
// Phantasma SDK: core typedefs and the pluggable JSON contract.
//------------------------------------------------------------------------------
// Part of PhantasmaAPI.h, split out so that the RPC value and extended-event models can be
// written in their own headers instead of growing the single API file. It is included after the
// macro configuration block of PhantasmaAPI.h and depends on it, so it is not a standalone header.
//------------------------------------------------------------------------------
#pragma once

#ifndef PHANTASMA_API_CONFIGURED
#error "Include PhantasmaAPI.h instead: Rpc/Prelude.h needs its macro configuration."
#endif

namespace phantasma {
#ifdef PHANTASMA_CHAR
typedef PHANTASMA_CHAR Char;
#else
#ifdef _UNICODE
typedef wchar_t Char;
#else
typedef char Char;
#endif
#endif

#ifndef PHANTASMA_STRLEN
#ifdef _UNICODE
#define PHANTASMA_STRLEN(x) wcslen(x)
#else
#define PHANTASMA_STRLEN(x) strlen(x)
#endif
#endif

#ifndef PHANTASMA_STRTOINT
#ifdef _UNICODE
#define PHANTASMA_STRTOINT(x) std::wcstoll(x, 0, 10)
#else
#define PHANTASMA_STRTOINT(x) std::strtoll(x, 0, 10)
#endif
#endif

#ifdef PHANTASMA_BYTE
typedef PHANTASMA_BYTE Byte;
#else
typedef uint8_t Byte;
#endif

typedef PHANTASMA_VECTOR<Byte> ByteArray;

#ifdef PHANTASMA_INT32
typedef PHANTASMA_INT32 Int32;
#else
typedef int32_t Int32;
#endif

#ifdef PHANTASMA_UINT32
typedef PHANTASMA_UINT32 UInt32;
#else
typedef uint32_t UInt32;
#endif

#ifdef PHANTASMA_INT64
typedef PHANTASMA_INT64 Int64;
#else
typedef int64_t Int64;
#endif

#ifdef PHANTASMA_UINT64
typedef PHANTASMA_UINT64 UInt64;
#else
typedef uint64_t UInt64;
#endif

#ifdef PHANTASMA_STRING
typedef PHANTASMA_STRING String;
#else
#ifdef _UNICODE
typedef std::wstring String;
#else
typedef std::string String;
#endif
#endif

#ifdef PHANTASMA_STRINGBUILDER
typedef PHANTASMA_STRINGBUILDER StringBuilder;
#else
#ifdef _UNICODE
typedef std::wstringstream StringBuilder;
#else
typedef std::stringstream StringBuilder;
#endif
#endif

#ifdef PHANTASMA_JSONVALUE
typedef PHANTASMA_JSONVALUE JSONValue;
#elif __cplusplus > 201402L
#ifdef _UNICODE
typedef std::wstring_view JSONValue;
#else
typedef std::string_view JSONValue;
#endif
#else
typedef String JSONValue;
#endif

#ifdef PHANTASMA_JSONARRAY
typedef PHANTASMA_JSONARRAY JSONArray;
#else
typedef JSONValue JSONArray;
#endif

#ifdef PHANTASMA_JSONDOCUMENT
typedef PHANTASMA_JSONDOCUMENT JSONDocument;
#else
typedef String JSONDocument;
#endif

#ifdef PHANTASMA_JSONBUILDER
typedef PHANTASMA_JSONBUILDER JSONBuilder;
#else
struct JSONBuilder // A VERY simple json string builder. Highly recommended that you provide a real JSON library instead!
{
	StringBuilder s;
	bool empty = true;
	operator StringBuilder&() { return s; }
	void AddKey(const Char* key)
	{
		if( !empty )
		{
			s << ", ";
		}
		empty = false;
		s << '"' << key << "\": ";
	}
	void AddValues() {}
	void AddValues(const char* arg) { s << '"' << arg << '"'; }
	void AddValues(bool arg) { s << (arg ? PHANTASMA_LITERAL("true") : PHANTASMA_LITERAL("false")); }
	template<class T>
	void AddValues(T arg) { s << arg; }
	template<class T, class... Args>
	void AddValues(T arg0, Args... args)
	{
		AddValues(arg0);
		s << ", ";
		AddValues(args...);
	}

	void BeginObject() { s << "{"; }
	void AddString(const Char* key, const Char* value)
	{
		AddKey(key);
		s << '"' << value << '"';
	}
	template<class... Args>
	void AddArray(const Char* key, Args... args)
	{
		AddKey(key);
		s << '[';
		AddValues(args...);
		s << ']';
	}
	// Writes key: [[values...]] (+ optional trailing scalars). Batch endpoints (getAccountInfos)
	// take their address list as a NESTED json array inside params, which the scalar-only
	// AddValues path above cannot express.
	void AddNestedStrings(const String* values, int count)
	{
		s << '[';
		for( int i = 0; i < count; ++i )
		{
			if( i )
				s << ", ";
			s << '"' << values[i] << '"';
		}
		s << ']';
	}
	void AddArrayWithNestedStringArray(const Char* key, const String* values, int count)
	{
		AddKey(key);
		s << '[';
		AddNestedStrings(values, count);
		s << ']';
	}
	template<class... Args>
	void AddArrayWithNestedStringArray(const Char* key, const String* values, int count, Args... args)
	{
		AddKey(key);
		s << '[';
		AddNestedStrings(values, count);
		s << ", ";
		AddValues(args...);
		s << ']';
	}
	void EndObject() { s << "}"; }
};
#endif

#ifdef PHANTASMA_HTTPCLIENT
typedef PHANTASMA_HTTPCLIENT HttpClient;
//JSONDocument HttpPost(HttpClient&, const Char* uri, const JSONBuilder&, PhantasmaError* out_error);
#endif

//If providing a JSON library (highly recommended that you do!), then you must provide these functions yourself:
namespace json {
#ifndef PHANTASMA_JSONBUILDER
JSONValue Parse(const JSONDocument&);
bool LookupBool(const JSONValue&, const Char* field, bool& out_error);
Int32 LookupInt32(const JSONValue&, const Char* field, bool& out_error);
UInt32 LookupUInt32(const JSONValue&, const Char* field, bool& out_error);
Int64 LookupInt64(const JSONValue&, const Char* field, bool& out_error);
UInt64 LookupUInt64(const JSONValue&, const Char* field, bool& out_error);
String LookupString(const JSONValue&, const Char* field, bool& out_error);
JSONValue LookupValue(const JSONValue&, const Char* field, bool& out_error);
JSONArray LookupArray(const JSONValue&, const Char* field, bool& out_error);
bool HasField(const JSONValue&, const Char* field, bool& out_error);
bool HasArrayField(const JSONValue&, const Char* field, bool& out_error);
bool AsBool(const JSONValue&, bool& out_error);
Int32 AsInt32(const JSONValue&, bool& out_error);
UInt32 AsUInt32(const JSONValue&, bool& out_error);
Int64 AsInt64(const JSONValue&, bool& out_error);
UInt64 AsUInt64(const JSONValue&, bool& out_error);
String AsString(const JSONValue&, bool& out_error);
JSONArray AsArray(const JSONValue&, bool& out_error);
bool IsString(const JSONValue&, bool& out_error);
bool IsArray(const JSONValue&, bool& out_error);
bool IsObject(const JSONValue&, bool& out_error);

int ArraySize(const JSONArray&, bool& out_error);
JSONValue IndexArray(const JSONArray&, int index, bool& out_error);

// Calls visit(name, value) for every member of a JSON object, in document order. Required wherever
// the field names are chain data rather than a fixed schema: VM values (token metadata, NFT
// properties) and the metadata maps inside special-resolution arguments cannot be read with the
// by-name lookups above. It is a template rather than a plain function because JSONValue is a
// reference type under some adaptors and therefore cannot be returned inside a container.
template<class Visitor>
void VisitObjectFields(const JSONValue&, Visitor&& visit, bool& out_error);

// Renders any scalar as text: a string as-is, a number or a boolean as its JSON text, a null as
// the empty string. VM values need this because the node writes every scalar as a string, but an
// older node - or a hand-written response - can still answer an untyped number, and normalizing it
// is better than failing the whole answer.
String ScalarText(const JSONValue&, bool& out_error);

// Renders any value back as JSON text. Extended events keep the payloads this build cannot type -
// an event kind or a call method a newer node answers - so that nothing the node sent is dropped,
// and that requires handing the value back as text rather than as a reference into a document the
// caller does not own.
String ToText(const JSONValue&, bool& out_error);

void BeginObject(JSONBuilder&);
void AddString(JSONBuilder&, const Char* key, const Char* value);
template<class... Args>
void AddArray(JSONBuilder&, const Char* key, Args... args);
// Writes key: [[values...], args...] - a nested string array as the first params element, used by
// batch endpoints (getAccountInfos). Only instantiated when those endpoints are called.
template<class... Args>
void AddArrayWithNestedStringArray(JSONBuilder&, const Char* key, const String* values, int count, Args... args);
void EndObject(JSONBuilder&);
#endif
} // namespace json
} // namespace phantasma
