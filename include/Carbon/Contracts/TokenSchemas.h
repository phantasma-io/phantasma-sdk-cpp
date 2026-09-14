#pragma once
#ifndef PHANTASMA_API_INCLUDED
#error "Configure and include PhantasmaAPI.h first"
#endif

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
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

	TokenSchemas View() const
	{
		TokenSchemasOwned copy = *this;
		copy.Rebind();
		return copy.view;
	}
};

struct FieldType {
	std::string name;
	VmType type = VmType::Dynamic;
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

	static bool EnsureNonEmptyString(const std::string& fieldName, const MetadataValue& value, std::string& out, std::string& outError)
	{
		if( value.kind != MetadataValue::Kind::String )
		{
			const std::string invalid = "Metadata field '" + fieldName + "' must be a string";
			PHANTASMA_EXCEPTION_MESSAGE("Metadata string invalid", invalid);
			outError = invalid;
			return false;
		}
		out = TrimWhitespace(value.stringValue);
		if( out.empty() )
		{
			const std::string invalid = "Metadata field '" + fieldName + "' is mandatory";
			PHANTASMA_EXCEPTION_MESSAGE("Metadata string invalid", invalid);
			outError = invalid;
			return false;
		}
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
			if( !EnsureIntegerInRange(
			        fieldName,
			        value,
			        std::numeric_limits<int64_t>::min(),
			        std::numeric_limits<int64_t>::max(),
			        std::numeric_limits<uint64_t>::max(),
			        "Int64",
			        raw,
			        outError) )
				return false;
			out = VmDynamicVariable((uint64_t)raw);
			return true;
		}
		case VmType::Int256: {
			if( value.kind == MetadataValue::Kind::Int256 )
			{
				out = VmDynamicVariable(value.int256Value);
				return true;
			}
			if( value.kind == MetadataValue::Kind::UInt256 )
			{
				out = VmDynamicVariable(value.uint256Value);
				return true;
			}
			if( value.kind == MetadataValue::Kind::Int64 )
			{
				out = VmDynamicVariable(int256(value.int64Value));
				return true;
			}
			if( value.kind == MetadataValue::Kind::UInt64 )
			{
				out = VmDynamicVariable(uint256(value.uint64Value));
				return true;
			}
			const std::string invalid = "Metadata field '" + fieldName + "' must be a number (Int256)";
			PHANTASMA_EXCEPTION_MESSAGE("Metadata integer invalid", invalid);
			outError = invalid;
			return false;
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
				uint64_t raw = 0;
				if( !EnsureIntegerInRange(
				        element(i),
				        values[i],
				        std::numeric_limits<int64_t>::min(),
				        std::numeric_limits<int64_t>::max(),
				        std::numeric_limits<uint64_t>::max(),
				        "Int64",
				        raw,
				        outError) )
					return false;
				arr[i] = raw;
			}
			out.data.int64Array = arr;
			return true;
		}
		case VmType::Int256: {
			uint256* arr = alloc.Alloc<uint256>(count);
			for( uint32_t i = 0; i != count; ++i )
			{
				const MetadataValue& v = values[i];
				if( v.kind == MetadataValue::Kind::UInt256 )
				{
					arr[i] = v.uint256Value;
				}
				else if( v.kind == MetadataValue::Kind::Int256 )
				{
					arr[i] = v.int256Value.Unsigned();
				}
				else if( v.kind == MetadataValue::Kind::Int64 )
				{
					arr[i] = int256(v.int64Value).Unsigned();
				}
				else if( v.kind == MetadataValue::Kind::UInt64 )
				{
					arr[i] = uint256(v.uint64Value);
				}
				else
				{
					const std::string invalid = "Metadata field '" + element(i) + "' must be a number (Int256)";
					PHANTASMA_EXCEPTION_MESSAGE("Metadata integer invalid", invalid);
					outError = invalid;
					return false;
				}
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

	static bool AddField(std::vector<VmNamedVariableSchema>& dest, const FieldType& f, std::string& outError)
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
		dest.push_back(VmNamedVariableSchema{
		    SmallString(f.name.c_str(), (phantasma::carbon::size_t)f.name.size()),
		    VmVariableSchema{ f.type } });
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

	// Answers false without touching `out` when a field name is empty, duplicated, differs only in
	// case, or when the finished schema fails Verify.
	static bool BuildFromFields(
	    const std::vector<FieldType>& seriesFields,
	    const std::vector<FieldType>& romFields,
	    const std::vector<FieldType>& ramFields,
	    TokenSchemasOwned& out,
	    std::string& outError)
	{
		TokenSchemasOwned owned;
		const auto add = [&outError](std::vector<VmNamedVariableSchema>& dest, const std::vector<FieldType>& fields)
		{
			for( const auto& f : fields )
			{
				if( !AddField(dest, f, outError) )
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
	// Answers false without touching `out` when the document is not an object, an array is missing or
	// malformed, a type name is unknown, or the schema the fields build fails BuildFromFields.
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
				const rapidjson::Value& v = *it;
				if( !v.IsObject() || !v.HasMember("name") || !v.HasMember("type") || !v["name"].IsString() || !v["type"].IsString() )
				{
					outError = std::string(key) + " entries must contain name and type";
					PHANTASMA_EXCEPTION_MESSAGE("TokenSchemas json invalid", outError);
					return false;
				}
				bool unknownType = false;
				const VmType vmType = VmTypeFromString(v["type"].GetString(), &unknownType);
				if( unknownType )
				{
					outError = "Unknown VmType: " + std::string(v["type"].GetString());
					PHANTASMA_EXCEPTION_MESSAGE("TokenSchemas json invalid", outError);
					return false;
				}
				fields.push_back(FieldType{ v["name"].GetString(), vmType });
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
#endif
};

} // namespace phantasma::carbon
