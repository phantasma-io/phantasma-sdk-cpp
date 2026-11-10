#pragma once
#ifndef PHANTASMA_API_INCLUDED
#error "Configure and include PhantasmaAPI.h first"
#endif

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../DataBlockchain.h"
#include "../../Numerics/Base16.h"

#ifdef PHANTASMA_RAPIDJSON
#include "rapidjson/document.h"
#endif

namespace phantasma::carbon {

struct TokenSchemas {
	VmStructSchema seriesMetadata{};
	VmStructSchema rom{};
	VmStructSchema ram{};
};

inline void Write(const TokenSchemas& in, WriteView& w);

struct TokenSchemasOwned {
	TokenSchemas view{};
	std::vector<VmNamedVariableSchema> seriesFields;
	std::vector<VmNamedVariableSchema> romFields;
	std::vector<VmNamedVariableSchema> ramFields;
	// The field lists of the struct schemas inside Struct and Array_Struct fields, at every depth. The
	// schemas in the three vectors above point into them, and a copy of this object keeps those
	// pointers. So the lists are shared between copies, and nothing changes them once they are built.
	std::vector<std::shared_ptr<const std::vector<VmNamedVariableSchema>>> nestedFields{};

	TokenSchemasOwned() = default;

	// `view` points into the three vectors of THIS object, so a copy of it would point into the
	// object it was copied from. Rebind repoints it here, and View answers a schema that already is.
	void Rebind()
	{
		view.seriesMetadata.numFields = (uint32_t)seriesFields.size();
		view.seriesMetadata.fields = seriesFields.empty() ? nullptr : seriesFields.data();
		view.rom.numFields = (uint32_t)romFields.size();
		view.rom.fields = romFields.empty() ? nullptr : romFields.data();
		view.ram.numFields = (uint32_t)ramFields.size();
		view.ram.fields = ramFields.empty() ? nullptr : ramFields.data();
	}

	// Returns a schema that points at THIS object's three vectors, so this object has to outlive it.
	// It must not be built by rebinding a local copy: that copy's vectors die with it and the caller
	// is left reading freed memory.
	TokenSchemas View() const
	{
		TokenSchemas out = view;
		out.seriesMetadata.numFields = (uint32_t)seriesFields.size();
		out.seriesMetadata.fields = seriesFields.empty() ? nullptr : seriesFields.data();
		out.rom.numFields = (uint32_t)romFields.size();
		out.rom.fields = romFields.empty() ? nullptr : romFields.data();
		out.ram.numFields = (uint32_t)ramFields.size();
		out.ram.fields = ramFields.empty() ? nullptr : ramFields.data();
		return out;
	}
};

struct FieldType {
	std::string name;
	VmType type = VmType::Dynamic;
	// The fields of a Struct field, or of each struct of an Array_Struct field. Other types have none,
	// and TokenSchemasBuilder::BuildFromFields reads them for those two types only.
	std::vector<FieldType> fields{};
};

struct MetadataValue;

// The value of a Dynamic field, or of one item of an Array_Dynamic field. Such a field can hold a value
// of any VM type, and the chain stores the type with the value, so the type is named here. The value
// is given as for a field of that type. The type cannot be Dynamic, Array_Dynamic, Struct or
// Array_Struct. MetadataValue::FromDynamic builds one.
struct MetadataDynamicValue {
	VmType type = VmType::Dynamic;
	// A pointer, because a value cannot hold a value of its own type directly. It points at a const
	// value, so the copies that share it cannot change it.
	std::shared_ptr<const MetadataValue> value{};
};

struct MetadataValue {
	enum class Kind
	{
		Null,
		String,
		Bytes,
		Bytes16,
		Bytes32,
		Bytes64,
		Int64,
		UInt64,
		Int256,
		UInt256,
		Struct,
		Array,
		Dynamic,
	};

	Kind kind = Kind::Null;
	std::string stringValue{};
	ByteArray bytesValue{};
	Bytes16 bytes16Value{};
	Bytes32 bytes32Value{};
	Bytes64 bytes64Value{};
	int64_t int64Value = 0;
	uint64_t uint64Value = 0;
	int256 int256Value{};
	uint256 uint256Value{};
	std::vector<MetadataValue> arrayValue{};
	std::vector<std::pair<std::string, MetadataValue>> structValue{};
	MetadataDynamicValue dynamicValue{};

	static MetadataValue FromString(const std::string& value)
	{
		MetadataValue out;
		out.kind = Kind::String;
		out.stringValue = value;
		return out;
	}

	static MetadataValue FromBytes(const ByteArray& value)
	{
		MetadataValue out;
		out.kind = Kind::Bytes;
		out.bytesValue = value;
		return out;
	}

	static MetadataValue FromBytes16(const Bytes16& value)
	{
		MetadataValue out;
		out.kind = Kind::Bytes16;
		out.bytes16Value = value;
		return out;
	}

	static MetadataValue FromBytes32(const Bytes32& value)
	{
		MetadataValue out;
		out.kind = Kind::Bytes32;
		out.bytes32Value = value;
		return out;
	}

	static MetadataValue FromBytes64(const Bytes64& value)
	{
		MetadataValue out;
		out.kind = Kind::Bytes64;
		out.bytes64Value = value;
		return out;
	}

	static MetadataValue FromInt64(int64_t value)
	{
		MetadataValue out;
		out.kind = Kind::Int64;
		out.int64Value = value;
		return out;
	}

	static MetadataValue FromUInt64(uint64_t value)
	{
		MetadataValue out;
		out.kind = Kind::UInt64;
		out.uint64Value = value;
		return out;
	}

	static MetadataValue FromInt256(const int256& value)
	{
		MetadataValue out;
		out.kind = Kind::Int256;
		out.int256Value = value;
		return out;
	}

	static MetadataValue FromUInt256(const uint256& value)
	{
		MetadataValue out;
		out.kind = Kind::UInt256;
		out.uint256Value = value;
		return out;
	}

	static MetadataValue FromStruct(const std::vector<std::pair<std::string, MetadataValue>>& value)
	{
		MetadataValue out;
		out.kind = Kind::Struct;
		out.structValue = value;
		return out;
	}

	static MetadataValue FromArray(const std::vector<MetadataValue>& value)
	{
		MetadataValue out;
		out.kind = Kind::Array;
		out.arrayValue = value;
		return out;
	}

	// Returns the value of a Dynamic field: a value of type `type`, given as for a field of that type.
	static MetadataValue FromDynamic(VmType type, const MetadataValue& value)
	{
		MetadataValue out;
		out.kind = Kind::Dynamic;
		out.dynamicValue.type = type;
		out.dynamicValue.value = std::make_shared<const MetadataValue>(value);
		return out;
	}
};

struct MetadataField {
	std::string name;
	MetadataValue value;
};

inline bool EqualsIgnoreCase(const std::string& a, const std::string& b)
{
	if( a.size() != b.size() )
	{
		return false;
	}
	for( size_t i = 0; i != a.size(); ++i )
	{
		if( tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]) )
		{
			return false;
		}
	}
	return true;
}

struct MetadataHelper {
	inline static const std::vector<FieldType> SeriesDefaultMetadataFields = {
		FieldType{ StandardMeta::id.c_str(), VmType::Int256 },
		FieldType{ "mode", VmType::Int8 },
		FieldType{ "rom", VmType::Bytes },
	};

	inline static const std::vector<FieldType> NftDefaultMetadataFields = {
		FieldType{ StandardMeta::id.c_str(), VmType::Int256 },
		FieldType{ "rom", VmType::Bytes },
	};

	inline static const std::vector<FieldType> StandardMetadataFields = {
		FieldType{ "name", VmType::String },
		FieldType{ "description", VmType::String },
		FieldType{ "imageURL", VmType::String },
		FieldType{ "infoURL", VmType::String },
		FieldType{ "royalties", VmType::Int32 },
	};

	static const MetadataField* FindMetadataField(const std::vector<MetadataField>& fields, const std::string& name)
	{
		for( const auto& field : fields )
		{
			if( EqualsIgnoreCase(field.name, name) )
			{
				return &field;
			}
		}
		return nullptr;
	}

	// Answers true with an empty value when the field is absent, which is what "optional" means here,
	// and false only when a present field cannot be read.
	static bool GetOptionalBytesField(const std::vector<MetadataField>& fields, const std::string& name, ByteArray& out, std::string& outError)
	{
		const MetadataField* found = FindMetadataField(fields, name);
		if( !found )
		{
			out = {};
			return true;
		}
		return EnsureBytes(name, found->value, out, outError);
	}

	static bool PushMetadataField(
	    const VmNamedVariableSchema& fieldSchema,
	    std::vector<VmNamedDynamicVariable>& fields,
	    const std::vector<MetadataField>& metadataFields,
	    Allocator& alloc,
	    std::string& outError)
	{
		const MetadataField* found = nullptr;
		for( const auto& field : metadataFields )
		{
			if( field.name == fieldSchema.name.c_str() )
			{
				found = &field;
				break;
			}
		}
		if( !found )
		{
			const MetadataField* caseMismatch = nullptr;
			for( const auto& field : metadataFields )
			{
				if( EqualsIgnoreCase(field.name, fieldSchema.name.c_str()) )
				{
					caseMismatch = &field;
					break;
				}
			}
			if( caseMismatch )
			{
				const std::string invalid = "Metadata field '" + std::string(fieldSchema.name.c_str()) +
				                            "' provided in incorrect case: '" + caseMismatch->name + "'";
				PHANTASMA_EXCEPTION_MESSAGE("Metadata field case mismatch", invalid);
				outError = invalid;
				return false;
			}

			const std::string missing = "Metadata field '" + std::string(fieldSchema.name.c_str()) + "' is mandatory";
			PHANTASMA_EXCEPTION_MESSAGE("Metadata field missing", missing);
			outError = missing;
			return false;
		}

		VmDynamicVariable normalized;
		if( !NormalizeMetadataValue(fieldSchema.schema, fieldSchema.name.c_str(), found->value, alloc, normalized, outError) )
		{
			return false;
		}
		fields.push_back(VmNamedDynamicVariable{ fieldSchema.name, normalized });
		return true;
	}

	static bool NormalizeMetadataValue(
	    const VmVariableSchema& schema,
	    const std::string& fieldName,
	    const MetadataValue& value,
	    Allocator& alloc,
	    VmDynamicVariable& out,
	    std::string& outError)
	{
		const uint8_t raw = (uint8_t)schema.type;
		const bool isArray = (raw & (uint8_t)VmType::Array) != 0;
		if( isArray )
		{
			if( value.kind != MetadataValue::Kind::Array )
			{
				const std::string invalid = "Metadata field '" + fieldName + "' must be provided as an array";
				PHANTASMA_EXCEPTION_MESSAGE("Metadata field type mismatch", invalid);
				outError = invalid;
				return false;
			}
			const VmType baseType = (VmType)(raw & ~(uint8_t)VmType::Array);
			return NormalizeArrayValue(baseType, fieldName, value.arrayValue, schema.structure, alloc, out, outError);
		}
		return NormalizeScalarValue(schema.type, fieldName, value, schema.structure, alloc, out, outError);
	}

  private:
	static std::string TrimWhitespace(const std::string& text)
	{
		size_t start = 0;
		while( start < text.size() && isspace((unsigned char)text[start]) )
		{
			++start;
		}
		size_t end = text.size();
		while( end > start && isspace((unsigned char)text[end - 1]) )
		{
			--end;
		}
		return text.substr(start, end - start);
	}

	// Every helper below answers false and fills outError instead of raising alone. PHANTASMA_EXCEPTION
	// does nothing in the default build, so a guard that only raises validates nothing there and the
	// code after it runs on the input it rejected. The file already answers this way in
	// TokenSchemasBuilder::Verify.
	static bool DecodeHex(const std::string& fieldName, const std::string& hex, ByteArray& out, std::string& outError)
	{
		const std::string invalid = "Metadata field '" + fieldName + "' must be a byte array or hex string";
		std::string trimmed = TrimWhitespace(hex);
		if( trimmed.empty() )
		{
			PHANTASMA_EXCEPTION_MESSAGE("Metadata bytes invalid", invalid);
			outError = invalid;
			return false;
		}
		if( trimmed.rfind("0x", 0) == 0 || trimmed.rfind("0X", 0) == 0 )
		{
			trimmed = trimmed.substr(2);
		}
		if( trimmed.empty() )
		{
			out = {};
			return true;
		}
		if( (trimmed.size() % 2) != 0 )
		{
			PHANTASMA_EXCEPTION_MESSAGE("Metadata bytes invalid", invalid);
			outError = invalid;
			return false;
		}

		try
		{
			out = Base16::Decode(trimmed.c_str(), (int)trimmed.size());
			return true;
		}
		catch( ... )
		{
			PHANTASMA_EXCEPTION_MESSAGE("Metadata bytes invalid", invalid);
			outError = invalid;
			return false;
		}
	}

	static bool EnsureBytes(const std::string& fieldName, const MetadataValue& value, ByteArray& out, std::string& outError)
	{
		switch( value.kind )
		{
		case MetadataValue::Kind::Bytes:
			out = value.bytesValue;
			return true;
		case MetadataValue::Kind::String:
			return DecodeHex(fieldName, value.stringValue, out, outError);
		default:
			break;
		}
		const std::string invalid = "Metadata field '" + fieldName + "' must be a byte array or hex string";
		PHANTASMA_EXCEPTION_MESSAGE("Metadata bytes invalid", invalid);
		outError = invalid;
		return false;
	}

	static bool EnsureFixedBytes(const std::string& fieldName, const MetadataValue& value, size_t expectedLength, ByteArray& out, std::string& outError)
	{
		if( !EnsureBytes(fieldName, value, out, outError) )
		{
			return false;
		}
		if( out.size() != expectedLength )
		{
			const std::string invalid = "Metadata field '" + fieldName + "' must be exactly " + std::to_string(expectedLength) + " bytes";
			PHANTASMA_EXCEPTION_MESSAGE("Metadata bytes invalid", invalid);
			outError = invalid;
			return false;
		}
		return true;
	}

	static bool EnsureBytes16(const std::string& fieldName, const MetadataValue& value, Bytes16& out, std::string& outError)
	{
		if( value.kind == MetadataValue::Kind::Bytes16 )
		{
			out = value.bytes16Value;
			return true;
		}
		ByteArray bytes;
		if( !EnsureFixedBytes(fieldName, value, Bytes16::length, bytes, outError) )
		{
			return false;
		}
		out = Bytes16(bytes);
		return true;
	}

	static bool EnsureBytes32(const std::string& fieldName, const MetadataValue& value, Bytes32& out, std::string& outError)
	{
		if( value.kind == MetadataValue::Kind::Bytes32 )
		{
			out = value.bytes32Value;
			return true;
		}
		ByteArray bytes;
		if( !EnsureFixedBytes(fieldName, value, Bytes32::length, bytes, outError) )
		{
			return false;
		}
		out = Bytes32(bytes);
		return true;
	}

	static bool EnsureBytes64(const std::string& fieldName, const MetadataValue& value, Bytes64& out, std::string& outError)
	{
		if( value.kind == MetadataValue::Kind::Bytes64 )
		{
			out = value.bytes64Value;
			return true;
		}
		ByteArray bytes;
		if( !EnsureFixedBytes(fieldName, value, Bytes64::length, bytes, outError) )
		{
			return false;
		}
		out = Bytes64(bytes);
		return true;
	}

	// Returns false and fills outError when the value is not a string or holds only whitespace.
	// Otherwise `out` is the text exactly as given. The chain stores text as it comes, and a unique
	// series takes its NFT id from the ROM bytes, so trimming here would give the same input a
	// different NFT id than the other SDKs give it.
	static bool EnsureNonEmptyString(const std::string& fieldName, const MetadataValue& value, std::string& out, std::string& outError)
	{
		if( value.kind != MetadataValue::Kind::String )
		{
			const std::string invalid = "Metadata field '" + fieldName + "' must be a string";
			PHANTASMA_EXCEPTION_MESSAGE("Metadata string invalid", invalid);
			outError = invalid;
			return false;
		}
		if( TrimWhitespace(value.stringValue).empty() )
		{
			const std::string invalid = "Metadata field '" + fieldName + "' is mandatory";
			PHANTASMA_EXCEPTION_MESSAGE("Metadata string invalid", invalid);
			outError = invalid;
			return false;
		}
		out = value.stringValue;
		return true;
	}

	static bool EnsureIntegerInRange(
	    const std::string& fieldName,
	    const MetadataValue& value,
	    int64_t min,
	    int64_t max,
	    uint64_t unsignedMax,
	    const char* label,
	    uint64_t& out,
	    std::string& outError)
	{
		const std::string outOfRange = "Metadata field '" + fieldName + "' must be between " + std::to_string(min) +
		                               " and " + std::to_string(max) + " or between 0 and " + std::to_string(unsignedMax) +
		                               " (" + label + ")";
		if( value.kind == MetadataValue::Kind::Int64 )
		{
			const int64_t v = value.int64Value;
			if( v < min || v > max )
			{
				PHANTASMA_EXCEPTION_MESSAGE("Metadata integer invalid", outOfRange);
				outError = outOfRange;
				return false;
			}
			out = (uint64_t)v;
			return true;
		}
		if( value.kind == MetadataValue::Kind::UInt64 )
		{
			const uint64_t v = value.uint64Value;
			if( v > unsignedMax )
			{
				PHANTASMA_EXCEPTION_MESSAGE("Metadata integer invalid", outOfRange);
				outError = outOfRange;
				return false;
			}
			out = v;
			return true;
		}

		const std::string notANumber = "Metadata field '" + fieldName + "' must be a number";
		PHANTASMA_EXCEPTION_MESSAGE("Metadata integer invalid", notANumber);
		outError = notANumber;
		return false;
	}

	// The range of an Int64 or Int256 metadata value: the signed range, or 0 to the unsigned maximum. A
	// value above the signed maximum is stored as its two's complement. The bounds are decimal text,
	// because the Int256 bounds fit no C++ integer type.
	struct IntegerBounds {
		const char* label;
		const char* min;
		const char* max;
		const char* unsignedMax;
	};
	inline static const IntegerBounds Int64Bounds{ "Int64", "-9223372036854775808", "9223372036854775807", "18446744073709551615" };
	inline static const IntegerBounds Int256Bounds{
		"Int256",
		"-57896044618658097711785492504343953926634992332820282019728792003956564819968",
		"57896044618658097711785492504343953926634992332820282019728792003956564819967",
		"115792089237316195423570985008687907853269984665640564039457584007913129639935",
	};

	// Refuses a value that is neither an integer nor a string of decimal digits.
	static bool RefuseNonInteger(const std::string& fieldName, std::string& outError)
	{
		const std::string invalid = "Metadata field '" + fieldName + "' must be an integer or a string of decimal digits";
		PHANTASMA_EXCEPTION_MESSAGE("Metadata integer invalid", invalid);
		outError = invalid;
		return false;
	}

	// Checks an Int64 or Int256 metadata value given as text. A JSON number above 2^53 - 1 loses digits
	// in JavaScript, so such values are often written as strings. The text must be decimal digits with
	// an optional leading minus, the whole string: no spaces, no plus sign, no hex, no line break. The
	// value must lie within `bounds`, as a number must. On success `negative` and `digits` hold its sign
	// and its digits without leading zeros.
	static bool CheckDecimalInteger(
	    const std::string& fieldName,
	    const std::string& text,
	    const IntegerBounds& bounds,
	    bool& negative,
	    std::string& digits,
	    std::string& outError)
	{
		negative = !text.empty() && text[0] == '-';
		size_t first = negative ? 1 : 0;
		if( first == text.size() )
		{
			return RefuseNonInteger(fieldName, outError);
		}
		for( size_t i = first; i != text.size(); ++i )
		{
			if( text[i] < '0' || text[i] > '9' )
			{
				return RefuseNonInteger(fieldName, outError);
			}
		}
		while( first + 1 < text.size() && text[first] == '0' )
		{
			++first;
		}
		digits = text.substr(first);

		// Without leading zeros, the longer of two digit strings is the larger number, and two strings of
		// the same length compare as numbers character by character.
		const std::string limit = negative ? std::string(bounds.min).substr(1) : std::string(bounds.unsignedMax);
		if( digits.size() > limit.size() || (digits.size() == limit.size() && digits > limit) )
		{
			const std::string outOfRange = "Metadata field '" + fieldName + "' must be between " + bounds.min + " and " +
			                               bounds.max + " or between 0 and " + bounds.unsignedMax + " (" + bounds.label + ")";
			PHANTASMA_EXCEPTION_MESSAGE("Metadata integer invalid", outOfRange);
			outError = outOfRange;
			return false;
		}
		return true;
	}

	// Reads an Int64 metadata value: an Int64 or UInt64 value, or a string of decimal digits. `out` is
	// its two's complement.
	static bool EnsureInt64(const std::string& fieldName, const MetadataValue& value, uint64_t& out, std::string& outError)
	{
		if( value.kind == MetadataValue::Kind::String )
		{
			bool negative = false;
			std::string digits;
			if( !CheckDecimalInteger(fieldName, value.stringValue, Int64Bounds, negative, digits, outError) )
			{
				return false;
			}
			// The bounds keep the digits within 64 bits, so strtoull reads them exactly.
			const uint64_t magnitude = std::strtoull(digits.c_str(), nullptr, 10);
			out = negative ? (uint64_t)0 - magnitude : magnitude;
			return true;
		}
		if( value.kind != MetadataValue::Kind::Int64 && value.kind != MetadataValue::Kind::UInt64 )
		{
			return RefuseNonInteger(fieldName, outError);
		}
		return EnsureIntegerInRange(
		    fieldName,
		    value,
		    std::numeric_limits<int64_t>::min(),
		    std::numeric_limits<int64_t>::max(),
		    std::numeric_limits<uint64_t>::max(),
		    "Int64",
		    out,
		    outError);
	}

	// Reads an Int256 metadata value: a value of any of the four integer kinds, or a string of decimal
	// digits. `out` is its two's complement.
	static bool EnsureInt256(const std::string& fieldName, const MetadataValue& value, uint256& out, std::string& outError)
	{
		switch( value.kind )
		{
		case MetadataValue::Kind::Int64:
			out = int256(value.int64Value).Unsigned();
			return true;
		case MetadataValue::Kind::UInt64:
			out = uint256(value.uint64Value);
			return true;
		case MetadataValue::Kind::Int256:
			out = value.int256Value.Unsigned();
			return true;
		case MetadataValue::Kind::UInt256:
			out = value.uint256Value;
			return true;
		case MetadataValue::Kind::String: {
			bool negative = false;
			std::string digits;
			if( !CheckDecimalInteger(fieldName, value.stringValue, Int256Bounds, negative, digits, outError) )
			{
				return false;
			}
			// The bounds keep the digits within 256 bits, which FromString reads exactly. With the minus
			// sign it returns the two's complement.
			const std::string number = negative ? "-" + digits : digits;
			out = uint256::FromString(number.c_str(), (uint32_t)number.size(), 10);
			return true;
		}
		default:
			return RefuseNonInteger(fieldName, outError);
		}
	}

	// Normalizes the value of a Dynamic field, or of one item of an Array_Dynamic field, to the value it
	// holds: a value of its own type, which the chain writes with that type and without a schema. A
	// MetadataDynamicValue names one type, so it cannot give the types of the fields of a Struct or
	// Array_Struct. It takes no Dynamic or Array_Dynamic value either, because no use is known for typed
	// values nested in a typed value. So it holds a scalar or an array of scalars.
	static bool NormalizeDynamicValue(
	    const std::string& fieldName,
	    const MetadataValue& value,
	    Allocator& alloc,
	    VmDynamicVariable& out,
	    std::string& outError)
	{
		const auto refuse = [&outError](const std::string& why)
		{
			PHANTASMA_EXCEPTION_MESSAGE("Metadata dynamic value invalid", why);
			outError = why;
			return false;
		};
		if( value.kind != MetadataValue::Kind::Dynamic || !value.dynamicValue.value )
		{
			return refuse("Metadata field '" + fieldName + "' is Dynamic and must be given as a MetadataDynamicValue");
		}
		const VmType type = value.dynamicValue.type;
		const char* typeName = VmTypeName(type);
		if( !typeName )
		{
			return refuse("Metadata field '" + fieldName + "' names an unknown type '" + std::to_string((int)type) + "'");
		}
		if( type == VmType::Dynamic || type == VmType::Array || type == VmType::Struct || type == VmType::Array_Struct )
		{
			return refuse("Metadata field '" + fieldName + "' cannot hold a Dynamic value of type " + typeName);
		}
		VmVariableSchema schema{};
		schema.type = type;
		return NormalizeMetadataValue(schema, fieldName, *value.dynamicValue.value, alloc, out, outError);
	}

	static bool NormalizeScalarValue(
	    VmType type,
	    const std::string& fieldName,
	    const MetadataValue& value,
	    const VmStructSchema& structSchema,
	    Allocator& alloc,
	    VmDynamicVariable& out,
	    std::string& outError)
	{
		switch( type )
		{
		case VmType::String: {
			std::string text;
			if( !EnsureNonEmptyString(fieldName, value, text, outError) )
				return false;
			out = VmDynamicVariable(alloc.Clone(text.c_str()));
			return true;
		}
		case VmType::Int8: {
			uint64_t raw = 0;
			if( !EnsureIntegerInRange(fieldName, value, -0x80, 0x7f, 0xff, "Int8", raw, outError) )
				return false;
			out = VmDynamicVariable((uint8_t)raw);
			return true;
		}
		case VmType::Int16: {
			uint64_t raw = 0;
			if( !EnsureIntegerInRange(fieldName, value, -0x8000, 0x7fff, 0xffff, "Int16", raw, outError) )
				return false;
			out = VmDynamicVariable((uint16_t)raw);
			return true;
		}
		case VmType::Int32: {
			uint64_t raw = 0;
			if( !EnsureIntegerInRange(fieldName, value, -0x80000000LL, 0x7fffffffLL, 0xffffffffULL, "Int32", raw, outError) )
				return false;
			out = VmDynamicVariable((uint32_t)raw);
			return true;
		}
		case VmType::Int64: {
			uint64_t raw = 0;
			if( !EnsureInt64(fieldName, value, raw, outError) )
				return false;
			out = VmDynamicVariable((uint64_t)raw);
			return true;
		}
		case VmType::Int256: {
			uint256 raw{};
			if( !EnsureInt256(fieldName, value, raw, outError) )
				return false;
			out = VmDynamicVariable(raw);
			return true;
		}
		case VmType::Bytes: {
			ByteArray bytes;
			if( !EnsureBytes(fieldName, value, bytes, outError) )
				return false;
			out = VmDynamicVariable(alloc.Clone(ByteView{ bytes.data(), bytes.size() }));
			return true;
		}
		case VmType::Bytes16: {
			Bytes16 bytes;
			if( !EnsureBytes16(fieldName, value, bytes, outError) )
				return false;
			out = VmDynamicVariable(bytes);
			return true;
		}
		case VmType::Bytes32: {
			Bytes32 bytes;
			if( !EnsureBytes32(fieldName, value, bytes, outError) )
				return false;
			out = VmDynamicVariable(bytes);
			return true;
		}
		case VmType::Bytes64: {
			Bytes64 bytes;
			if( !EnsureBytes64(fieldName, value, bytes, outError) )
				return false;
			out = VmDynamicVariable(bytes);
			return true;
		}
		case VmType::Struct: {
			VmDynamicStruct structure;
			if( !NormalizeStructValue(fieldName, structSchema, value, alloc, structure, outError) )
				return false;
			out = VmDynamicVariable(structure);
			return true;
		}
		case VmType::Dynamic: {
			// The field stores a Dynamic around the value it holds. Write writes a Dynamic field only from
			// that form, and the chain reads such a field back into it.
			VmDynamicVariable held;
			if( !NormalizeDynamicValue(fieldName, value, alloc, held, outError) )
				return false;
			VmDynamicVariable* storage = alloc.Alloc<VmDynamicVariable>(1);
			*storage = held;
			out = VmDynamicVariable();
			out.type = VmType::Dynamic;
			out.data.dynamic = storage;
			return true;
		}
		default:
			break;
		}

		const std::string unsupported = "Metadata field '" + fieldName + "' has unsupported type";
		PHANTASMA_EXCEPTION_MESSAGE("Metadata field unsupported", unsupported);
		outError = unsupported;
		return false;
	}

	static bool NormalizeArrayValue(
	    VmType type,
	    const std::string& fieldName,
	    const std::vector<MetadataValue>& values,
	    const VmStructSchema& structSchema,
	    Allocator& alloc,
	    VmDynamicVariable& out,
	    std::string& outError)
	{
		const uint32_t count = (uint32_t)values.size();
		out = VmDynamicVariable();
		out.type = (VmType)((uint8_t)VmType::Array | (uint8_t)type);
		out.arrayLength = count;
		const auto element = [&](uint32_t i)
		{ return fieldName + "[" + std::to_string(i) + "]"; };

		switch( type )
		{
		case VmType::String: {
			const char** arr = alloc.Alloc<const char*>(count);
			for( uint32_t i = 0; i != count; ++i )
			{
				std::string text;
				if( !EnsureNonEmptyString(element(i), values[i], text, outError) )
					return false;
				arr[i] = alloc.Clone(text.c_str());
			}
			out.data.stringArray = arr;
			return true;
		}
		case VmType::Int8: {
			uint8_t* arr = alloc.Alloc<uint8_t>(count);
			for( uint32_t i = 0; i != count; ++i )
			{
				uint64_t raw = 0;
				if( !EnsureIntegerInRange(element(i), values[i], -0x80, 0x7f, 0xff, "Int8", raw, outError) )
					return false;
				arr[i] = (uint8_t)raw;
			}
			out.data.int8Array = arr;
			return true;
		}
		case VmType::Int16: {
			uint16_t* arr = alloc.Alloc<uint16_t>(count);
			for( uint32_t i = 0; i != count; ++i )
			{
				uint64_t raw = 0;
				if( !EnsureIntegerInRange(element(i), values[i], -0x8000, 0x7fff, 0xffff, "Int16", raw, outError) )
					return false;
				arr[i] = (uint16_t)raw;
			}
			out.data.int16Array = arr;
			return true;
		}
		case VmType::Int32: {
			uint32_t* arr = alloc.Alloc<uint32_t>(count);
			for( uint32_t i = 0; i != count; ++i )
			{
				uint64_t raw = 0;
				if( !EnsureIntegerInRange(element(i), values[i], -0x80000000LL, 0x7fffffffLL, 0xffffffffULL, "Int32", raw, outError) )
					return false;
				arr[i] = (uint32_t)raw;
			}
			out.data.int32Array = arr;
			return true;
		}
		case VmType::Int64: {
			uint64_t* arr = alloc.Alloc<uint64_t>(count);
			for( uint32_t i = 0; i != count; ++i )
			{
				if( !EnsureInt64(element(i), values[i], arr[i], outError) )
					return false;
			}
			out.data.int64Array = arr;
			return true;
		}
		case VmType::Int256: {
			uint256* arr = alloc.Alloc<uint256>(count);
			for( uint32_t i = 0; i != count; ++i )
			{
				if( !EnsureInt256(element(i), values[i], arr[i], outError) )
					return false;
			}
			out.data.int256Array = arr;
			return true;
		}
		case VmType::Bytes: {
			ByteView* arr = alloc.Alloc<ByteView>(count);
			for( uint32_t i = 0; i != count; ++i )
			{
				ByteArray bytes;
				if( !EnsureBytes(element(i), values[i], bytes, outError) )
					return false;
				arr[i] = alloc.Clone(ByteView{ bytes.data(), bytes.size() });
			}
			out.data.bytesArray = arr;
			return true;
		}
		case VmType::Bytes16: {
			Bytes16* arr = alloc.Alloc<Bytes16>(count);
			for( uint32_t i = 0; i != count; ++i )
			{
				if( !EnsureBytes16(element(i), values[i], arr[i], outError) )
					return false;
			}
			out.data.bytes16Array = arr;
			return true;
		}
		case VmType::Bytes32: {
			Bytes32* arr = alloc.Alloc<Bytes32>(count);
			for( uint32_t i = 0; i != count; ++i )
			{
				if( !EnsureBytes32(element(i), values[i], arr[i], outError) )
					return false;
			}
			out.data.bytes32Array = arr;
			return true;
		}
		case VmType::Bytes64: {
			Bytes64* arr = alloc.Alloc<Bytes64>(count);
			for( uint32_t i = 0; i != count; ++i )
			{
				if( !EnsureBytes64(element(i), values[i], arr[i], outError) )
					return false;
			}
			out.data.bytes64Array = arr;
			return true;
		}
		case VmType::Struct: {
			VmDynamicStruct* arr = alloc.Alloc<VmDynamicStruct>(count);
			for( uint32_t i = 0; i != count; ++i )
			{
				if( !NormalizeStructValue(element(i), structSchema, values[i], alloc, arr[i], outError) )
					return false;
			}
			out.data.structureArray = { structSchema, arr };
			return true;
		}
		case VmType::Dynamic: {
			// The chain writes each item with its own type, so the array holds the values themselves.
			VmDynamicVariable* arr = alloc.Alloc<VmDynamicVariable>(count);
			for( uint32_t i = 0; i != count; ++i )
			{
				if( !NormalizeDynamicValue(element(i), values[i], alloc, arr[i], outError) )
					return false;
			}
			out.data.dynamicArray = arr;
			return true;
		}
		default:
			break;
		}

		const std::string unsupported = "Metadata field '" + fieldName + "' has unsupported array type";
		PHANTASMA_EXCEPTION_MESSAGE("Metadata field unsupported", unsupported);
		outError = unsupported;
		return false;
	}

	static bool NormalizeStructValue(
	    const std::string& fieldName,
	    const VmStructSchema& structSchema,
	    const MetadataValue& value,
	    Allocator& alloc,
	    VmDynamicStruct& out,
	    std::string& outError)
	{
		const std::string shape = "Metadata field '" + fieldName + "' must be provided as an object or array of fields";
		if( structSchema.numFields == 0 )
		{
			const std::string invalid = "Metadata field '" + fieldName + "' is missing struct schema";
			PHANTASMA_EXCEPTION_MESSAGE("Metadata struct invalid", invalid);
			outError = invalid;
			return false;
		}

		std::vector<std::pair<std::string, MetadataValue>> provided;
		if( value.kind == MetadataValue::Kind::Struct )
		{
			provided = value.structValue;
		}
		else if( value.kind == MetadataValue::Kind::Array )
		{
			for( const auto& item : value.arrayValue )
			{
				if( item.kind != MetadataValue::Kind::Struct )
				{
					PHANTASMA_EXCEPTION_MESSAGE("Metadata struct invalid", shape);
					outError = shape;
					return false;
				}
				const auto nameIt = std::find_if(
				    item.structValue.begin(),
				    item.structValue.end(),
				    [](const std::pair<std::string, MetadataValue>& f)
				    { return f.first == "name"; });
				const auto valueIt = std::find_if(
				    item.structValue.begin(),
				    item.structValue.end(),
				    [](const std::pair<std::string, MetadataValue>& f)
				    { return f.first == "value"; });
				if( nameIt == item.structValue.end() || valueIt == item.structValue.end() ||
				    nameIt->second.kind != MetadataValue::Kind::String )
				{
					PHANTASMA_EXCEPTION_MESSAGE("Metadata struct invalid", shape);
					outError = shape;
					return false;
				}
				provided.push_back({ nameIt->second.stringValue, valueIt->second });
			}
		}
		else
		{
			PHANTASMA_EXCEPTION_MESSAGE("Metadata struct invalid", shape);
			outError = shape;
			return false;
		}

		std::vector<VmNamedDynamicVariable> fields;
		fields.reserve(structSchema.numFields);

		for( uint32_t i = 0; i != structSchema.numFields; ++i )
		{
			const VmNamedVariableSchema& childSchema = structSchema.fields[i];
			const std::string childName = childSchema.name.c_str();
			const auto exact = std::find_if(
			    provided.begin(),
			    provided.end(),
			    [&](const std::pair<std::string, MetadataValue>& f)
			    { return f.first == childName; });
			if( exact == provided.end() )
			{
				const auto caseMismatch = std::find_if(
				    provided.begin(),
				    provided.end(),
				    [&](const std::pair<std::string, MetadataValue>& f)
				    { return EqualsIgnoreCase(f.first, childName); });
				const std::string invalid =
				    caseMismatch != provided.end()
				        ? "Metadata field '" + childName + "' provided in incorrect case inside '" + fieldName + "': '" + caseMismatch->first + "'"
				        : "Metadata field '" + fieldName + "." + childName + "' is mandatory";
				PHANTASMA_EXCEPTION_MESSAGE("Metadata struct invalid", invalid);
				outError = invalid;
				return false;
			}

			VmDynamicVariable normalized;
			if( !NormalizeMetadataValue(childSchema.schema, fieldName + "." + childName, exact->second, alloc, normalized, outError) )
			{
				return false;
			}
			fields.push_back(VmNamedDynamicVariable{ childSchema.name, normalized });
		}

		for( const auto& providedField : provided )
		{
			const bool known = std::any_of(
			    structSchema.fields,
			    structSchema.fields + structSchema.numFields,
			    [&](const VmNamedVariableSchema& s)
			    { return EqualsIgnoreCase(s.name.c_str(), providedField.first); });
			if( !known )
			{
				const std::string invalid = "Metadata field '" + fieldName + "' received unknown property '" + providedField.first + "'";
				PHANTASMA_EXCEPTION_MESSAGE("Metadata struct invalid", invalid);
				outError = invalid;
				return false;
			}
		}

		VmNamedDynamicVariable* storage = alloc.Alloc<VmNamedDynamicVariable>(fields.size());
		for( size_t i = 0; i != fields.size(); ++i )
		{
			storage[i] = fields[i];
		}
		out = VmDynamicStruct::Sort((uint32_t)fields.size(), storage);
		return true;
	}
};

struct IdHelper {
	static uint256 GetRandomPhantasmaId()
	{
		uint8_t bytes[32];
		CryptoRandomBuffer(bytes, sizeof(bytes));
		return uint256::FromBytes(ByteView{ bytes, sizeof(bytes) });
	}
};

struct TokenSchemasBuilder {
  private:
	static bool ContainsField(const VmStructSchema& schema, const std::string& name, VmType type, std::string& outError)
	{
		for( uint32_t i = 0; i != schema.numFields; ++i )
		{
			const std::string candidate = schema.fields[i].name.c_str();
			if( candidate == name )
			{
				if( schema.fields[i].schema.type != type )
				{
					outError = "Type mismatch for field " + name;
					return false;
				}
				return true;
			}
			if( EqualsIgnoreCase(candidate, name) )
			{
				outError = "Case mismatch for field " + name + ", expected " + candidate;
				return false;
			}
		}
		return false;
	}

	static bool VerifyMandatory(const VmStructSchema& schema, const std::vector<FieldType>& mandatory, std::string& outError)
	{
		for( const auto& f : mandatory )
		{
			if( !ContainsField(schema, f.name, f.type, outError) )
			{
				if( outError.empty() )
				{
					outError = "Mandatory metadata field not found: " + f.name;
				}
				return false;
			}
		}
		return true;
	}

	static bool VerifyStandardMetadata(const VmStructSchema* first, const VmStructSchema* second, std::string& outError)
	{
		for( const auto& f : MetadataHelper::StandardMetadataFields )
		{
			bool found = false;
			std::string tempError;
			if( first && ContainsField(*first, f.name, f.type, tempError) )
			{
				found = true;
			}
			else if( !tempError.empty() )
			{
				outError = tempError;
				return false;
			}
			if( !found && second && ContainsField(*second, f.name, f.type, tempError) )
			{
				found = true;
			}
			else if( !found && !tempError.empty() )
			{
				outError = tempError;
				return false;
			}
			if( !found )
			{
				outError = "Mandatory metadata field not found: " + f.name;
				return false;
			}
		}
		return true;
	}

	// Adds the schema of `f` to `dest`. A Struct or Array_Struct field gets the schema of its struct,
	// built from f.fields with the same checks, in the given order and with no flags, as the other SDKs
	// build it. The field list of that schema is kept in `owner`.
	static bool AddField(std::vector<VmNamedVariableSchema>& dest, const FieldType& f, TokenSchemasOwned& owner, std::string& outError)
	{
		if( f.name.empty() )
		{
			outError = "Field name cannot be empty";
			return false;
		}
		for( const auto& existing : dest )
		{
			const std::string candidate = existing.name.c_str();
			if( candidate == f.name )
			{
				outError = "Duplicate field name: " + f.name;
				return false;
			}
			if( EqualsIgnoreCase(candidate, f.name) )
			{
				outError = "Case mismatch for field " + f.name + ", expected " + candidate;
				return false;
			}
		}
		VmVariableSchema schema{ f.type };
		if( (f.type == VmType::Struct || f.type == VmType::Array_Struct) && !f.fields.empty() )
		{
			const auto structFields = std::make_shared<std::vector<VmNamedVariableSchema>>();
			for( const auto& child : f.fields )
			{
				if( !AddField(*structFields, child, owner, outError) )
				{
					return false;
				}
			}
			schema.structure = VmStructSchema{ (uint32_t)structFields->size(), structFields->data(), VmStructSchema::Flag_None };
			owner.nestedFields.push_back(structFields);
		}
		dest.push_back(VmNamedVariableSchema{
		    SmallString(f.name.c_str(), (phantasma::carbon::size_t)f.name.size()),
		    schema });
		return true;
	}

	static bool Verify(const TokenSchemasOwned& owned, std::string& outError)
	{
		if( !VerifyMandatory(owned.view.seriesMetadata, MetadataHelper::SeriesDefaultMetadataFields, outError) )
		{
			return false;
		}
		if( !VerifyMandatory(owned.view.rom, MetadataHelper::NftDefaultMetadataFields, outError) )
		{
			return false;
		}
		if( !VerifyStandardMetadata(&owned.view.seriesMetadata, &owned.view.rom, outError) )
		{
			return false;
		}
		return true;
	}

  public:
	static TokenSchemasOwned PrepareStandardTokenSchemas(bool sharedMetadata = false)
	{
		TokenSchemasOwned owned;
		owned.seriesFields = {
			VmNamedVariableSchema{ StandardMeta::id, VmVariableSchema{ VmType::Int256 } },
			VmNamedVariableSchema{ SmallString("mode"), VmVariableSchema{ VmType::Int8 } },
			VmNamedVariableSchema{ SmallString("rom"), VmVariableSchema{ VmType::Bytes } },
		};
		if( sharedMetadata )
		{
			for( const auto& f : MetadataHelper::StandardMetadataFields )
			{
				owned.seriesFields.push_back(VmNamedVariableSchema{
				    SmallString(f.name.c_str(), (phantasma::carbon::size_t)f.name.size()),
				    VmVariableSchema{ f.type } });
			}
		}
		owned.romFields = {
			VmNamedVariableSchema{ StandardMeta::id, VmVariableSchema{ VmType::Int256 } },
			VmNamedVariableSchema{ SmallString("rom"), VmVariableSchema{ VmType::Bytes } },
		};
		if( !sharedMetadata )
		{
			for( const auto& f : MetadataHelper::StandardMetadataFields )
			{
				owned.romFields.push_back(VmNamedVariableSchema{
				    SmallString(f.name.c_str(), (phantasma::carbon::size_t)f.name.size()),
				    VmVariableSchema{ f.type } });
			}
		}
		owned.ramFields.clear();

		owned.view.seriesMetadata = VmStructSchema{ (uint32_t)owned.seriesFields.size(), owned.seriesFields.data(), VmStructSchema::Flag_None };
		owned.view.rom = VmStructSchema{ (uint32_t)owned.romFields.size(), owned.romFields.data(), VmStructSchema::Flag_None };
		owned.view.ram = VmStructSchema{ (uint32_t)owned.ramFields.size(), owned.ramFields.data(), VmStructSchema::Flag_DynamicExtras };
		return owned;
	}

	// Answers false without touching `out` when a field name is empty, duplicated or differs only in
	// case, in a list or among the fields of a struct, or when the finished schema fails Verify. The
	// three lists are sorted by name. The fields of a struct keep their given order.
	static bool BuildFromFields(
	    const std::vector<FieldType>& seriesFields,
	    const std::vector<FieldType>& romFields,
	    const std::vector<FieldType>& ramFields,
	    TokenSchemasOwned& out,
	    std::string& outError)
	{
		TokenSchemasOwned owned;
		const auto add = [&owned, &outError](std::vector<VmNamedVariableSchema>& dest, const std::vector<FieldType>& fields)
		{
			for( const auto& f : fields )
			{
				if( !AddField(dest, f, owned, outError) )
				{
					PHANTASMA_EXCEPTION_MESSAGE("Invalid token schema", outError);
					return false;
				}
			}
			return true;
		};

		const std::vector<FieldType> seriesDefaults = {
			FieldType{ StandardMeta::id.c_str(), VmType::Int256 },
			FieldType{ "mode", VmType::Int8 },
			FieldType{ "rom", VmType::Bytes },
		};
		if( !add(owned.seriesFields, seriesDefaults) || !add(owned.seriesFields, seriesFields) )
		{
			return false;
		}
		owned.view.seriesMetadata = VmStructSchema::Sort((uint32_t)owned.seriesFields.size(), owned.seriesFields.data(), false);

		const std::vector<FieldType> romDefaults = {
			FieldType{ StandardMeta::id.c_str(), VmType::Int256 },
			FieldType{ "rom", VmType::Bytes },
		};
		if( !add(owned.romFields, romDefaults) || !add(owned.romFields, romFields) )
		{
			return false;
		}
		owned.view.rom = VmStructSchema::Sort((uint32_t)owned.romFields.size(), owned.romFields.data(), false);

		if( !add(owned.ramFields, ramFields) )
		{
			return false;
		}
		owned.view.ram = VmStructSchema::Sort((uint32_t)owned.ramFields.size(), owned.ramFields.data(), owned.ramFields.empty());

		if( !Verify(owned, outError) )
		{
			PHANTASMA_EXCEPTION_MESSAGE("Invalid token schema", outError);
			return false;
		}
		out = owned;
		out.Rebind();
		return true;
	}

	static ByteArray BuildAndSerialize(const TokenSchemas* tokenSchemas)
	{
		TokenSchemasOwned owned = tokenSchemas ? TokenSchemasOwned() : PrepareStandardTokenSchemas();
		ByteArray buffer;
		WriteView w(buffer);
		if( tokenSchemas )
		{
			Write(*tokenSchemas, w);
		}
		else
		{
			Write(owned.View(), w);
		}
		return buffer;
	}

	static ByteArray Serialize(const TokenSchemas& tokenSchemas)
	{
		ByteArray buffer;
		WriteView w(buffer);
		Write(tokenSchemas, w);
		return buffer;
	}

	static std::string SerializeHex(const TokenSchemas& tokenSchemas)
	{
		const ByteArray bytes = Serialize(tokenSchemas);
		if( bytes.empty() )
		{
			return {};
		}
		const String encoded = Base16::Encode(&bytes.front(), (int)bytes.size(), false);
		return std::string(encoded.begin(), encoded.end());
	}

#ifdef PHANTASMA_RAPIDJSON
	// Reads token schemas from JSON: three arrays of fields, `seriesMetadata`, `rom` and `ram`. A field
	// is { "name": ..., "type": ... }, and a Struct or Array_Struct field also has "fields", the fields
	// of its struct in the same form.
	//
	// Answers false without touching `out` when the document is not an object, an array is missing or
	// malformed, a type name is unknown, a field lacks the fields its type needs or has fields its type
	// cannot have, or the schema the fields build fails BuildFromFields.
	static bool FromJson(const std::string& json, TokenSchemasOwned& out, std::string& outError)
	{
		rapidjson::Document doc;
		doc.Parse<rapidjson::kParseDefaultFlags>(json.c_str());
		if( doc.HasParseError() || !doc.IsObject() )
		{
			PHANTASMA_EXCEPTION_MESSAGE("TokenSchemas json invalid", "token_schemas must be a JSON object");
			outError = "token_schemas must be a JSON object";
			return false;
		}

		auto parseArray = [&](const char* key, std::vector<FieldType>& fields) -> bool
		{
			if( !doc.HasMember(key) || !doc[key].IsArray() )
			{
				outError = std::string(key) + " must be an array";
				PHANTASMA_EXCEPTION_MESSAGE("TokenSchemas json invalid", outError);
				return false;
			}
			for( auto it = doc[key].Begin(); it != doc[key].End(); ++it )
			{
				FieldType field;
				if( !ParseFieldType(*it, key, field, outError) )
				{
					return false;
				}
				fields.push_back(field);
			}
			return true;
		};

		std::vector<FieldType> seriesFields, romFields, ramFields;
		if( !parseArray("seriesMetadata", seriesFields) || !parseArray("rom", romFields) || !parseArray("ram", ramFields) )
		{
			return false;
		}
		return BuildFromFields(seriesFields, romFields, ramFields, out, outError);
	}

  private:
	// Reads one field of a token schema. `path` names the list the field stands in, for the error
	// messages: the key of the list, then the name of each struct field it is inside, as in rom.stats.
	static bool ParseFieldType(const rapidjson::Value& v, const std::string& path, FieldType& out, std::string& outError)
	{
		const auto refuse = [&outError](const std::string& why)
		{
			outError = why;
			PHANTASMA_EXCEPTION_MESSAGE("TokenSchemas json invalid", outError);
			return false;
		};
		if( !v.IsObject() )
		{
			return refuse(path + " field must be an object");
		}
		if( !v.HasMember("name") || !v["name"].IsString() )
		{
			return refuse(path + " field name must be string");
		}
		if( !v.HasMember("type") || !v["type"].IsString() )
		{
			return refuse(path + " field type must be string");
		}
		bool unknownType = false;
		const VmType type = VmTypeFromString(v["type"].GetString(), &unknownType);
		if( unknownType )
		{
			return refuse("Unknown VmType: " + std::string(v["type"].GetString()));
		}
		out.name = v["name"].GetString();
		out.type = type;
		const std::string fieldPath = path + "." + out.name;
		const std::string typeName = VmTypeName(type);

		// The chain keeps the schema of a struct inside the schema of a Struct or Array_Struct field, and
		// writes the field's values by it. So such a field cannot be built without its fields.
		if( type != VmType::Struct && type != VmType::Array_Struct )
		{
			if( v.HasMember("fields") )
			{
				return refuse(fieldPath + " has the type " + typeName + " and cannot have fields");
			}
			return true;
		}
		if( !v.HasMember("fields") || !v["fields"].IsArray() )
		{
			return refuse(fieldPath + " has the type " + typeName + " and needs a fields array");
		}
		const rapidjson::Value& children = v["fields"];
		if( children.Size() == 0 )
		{
			return refuse(fieldPath + " has the type " + typeName + " and needs at least one field");
		}
		for( auto it = children.Begin(); it != children.End(); ++it )
		{
			FieldType child;
			if( !ParseFieldType(*it, fieldPath, child, outError) )
			{
				return false;
			}
			out.fields.push_back(child);
		}
		return true;
	}
#endif
};

} // namespace phantasma::carbon
