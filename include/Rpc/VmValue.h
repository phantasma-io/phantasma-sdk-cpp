//------------------------------------------------------------------------------
// Phantasma SDK: values decoded from VM storage.
//------------------------------------------------------------------------------
// Part of PhantasmaAPI.h; include that file, not this one.
//------------------------------------------------------------------------------
#pragma once

#include "Prelude.h"

namespace phantasma {
namespace rpc {

enum class VmValueKind
{
	Text,
	Items,
	Fields
};

struct VmValue;

// One field of a VM struct. The value is held behind a pointer because the type is recursive and
// the container macros of this SDK are not required to accept an incomplete element type.
struct VmField {
	String name;
	std::shared_ptr<VmValue> value;
};

// A value decoded from VM storage: a scalar, an array, or a struct.
//
// VM values are dynamically typed, so the wire carries the plain JSON value - a string, an array
// or an object - and the shape itself says which of the three it is. Nothing is packed into a JSON
// string, which is what these values used to be before the 2026-08 node series.
//
// Scalars are always text: chain numbers are big integers and JSON numbers lose precision above
// 2^53, so the node writes them as decimal strings, and byte values arrive as hex. Struct field
// names arrive exactly as the chain stores them; the node does not rename dictionary keys.
struct VmValue {
	VmValueKind kind = VmValueKind::Text;
	String text;
	PHANTASMA_VECTOR<std::shared_ptr<VmValue>> items;
	PHANTASMA_VECTOR<VmField> fields;

	bool IsText() const { return kind == VmValueKind::Text; }
	bool IsItems() const { return kind == VmValueKind::Items; }
	bool IsFields() const { return kind == VmValueKind::Fields; }

	// The scalar content; empty for an array or a struct.
	const String& Text() const { return text; }

	int ItemCount() const { return IsItems() ? (int)items.size() : 0; }

	// One array element, or null for a scalar, a struct, or an index out of range.
	const VmValue* Item(int index) const
	{
		if( !IsItems() || index < 0 || index >= (int)items.size() )
			return nullptr;
		return items[index].get();
	}

	int FieldCount() const { return IsFields() ? (int)fields.size() : 0; }

	// One field of a struct by name, or null for a scalar, an array, or a missing field.
	const VmValue* Field(const Char* name) const
	{
		if( !IsFields() || !name )
			return nullptr;
		for( const VmField& field : fields )
		{
			if( field.name == name )
				return field.value.get();
		}
		return nullptr;
	}
};

// Collects the fields of a VM struct. A named visitor rather than a lambda, because the object
// enumeration is a template and a local type as its argument would rely on a C++20 relaxation.
struct VmFieldCollector {
	PHANTASMA_VECTOR<VmField>* fields;
	bool* error;

	void operator()(const String& name, const JSONValue& member) const;
};

// Reads one VM value from the JSON the node answered.
//
// An explicit null becomes an empty scalar; the node omits empty values instead of answering null.
// Nesting depth is bounded by the JSON parser itself, which rejects input nested deeper than its
// own limit before anything reaches this function.
PHANTASMA_FUNCTION VmValue ParseVmValue(const JSONValue& value, bool& out_error);

#if defined(PHANTASMA_IMPLEMENTATION)

PHANTASMA_FUNCTION VmValue ParseVmValue(const JSONValue& value, bool& jsonErr)
{
	VmValue output{};
	if( json::IsArray(value, jsonErr) )
	{
		output.kind = VmValueKind::Items;
		const JSONArray& array = json::AsArray(value, jsonErr);
		const int size = json::ArraySize(array, jsonErr);
		output.items.reserve(size);
		for( int i = 0; i < size; ++i )
		{
			output.items.push_back(std::make_shared<VmValue>(ParseVmValue(json::IndexArray(array, i, jsonErr), jsonErr)));
		}
		return output;
	}

	if( json::IsObject(value, jsonErr) )
	{
		output.kind = VmValueKind::Fields;
		VmFieldCollector collector{ &output.fields, &jsonErr };
		json::VisitObjectFields(value, collector, jsonErr);
		return output;
	}

	output.kind = VmValueKind::Text;
	output.text = json::ScalarText(value, jsonErr);
	return output;
}

PHANTASMA_FUNCTION void VmFieldCollector::operator()(const String& name, const JSONValue& member) const
{
	VmField field;
	field.name = name;
	field.value = std::make_shared<VmValue>(ParseVmValue(member, *error));
	fields->push_back(field);
}

#endif

} // namespace rpc
} // namespace phantasma
