#include "test_cases.h"

namespace testcases {
using namespace testutil;

// NFT data and series metadata are written by their schema: only the values, in schema order. Data
// written by a schema is read back by the same schema, as the chain reads it. A Struct or Array_Struct
// field inside is written by that field's own struct schema. The expected bytes are what the
// TypeScript SDK writes for the same schema and values, and the C# SDK's tests compose the same
// layouts.

void RunVmSchemaTests(TestContext& ctx)
{
	// A Struct or Array_Struct field is written by its own struct schema, without field names or types,
	// so it has to be read by that schema as well.
	{
		VmNamedVariableSchema statsFields[] = { MakeSchema("hp", VmType::Int32), MakeSchema("tag", VmType::String) };
		const VmStructSchema stats{ 2, statsFields, VmStructSchema::Flag_None };
		VmNamedVariableSchema itemFields[] = { MakeSchema("id", VmType::Int64) };
		const VmStructSchema item{ 1, itemFields, VmStructSchema::Flag_None };
		VmNamedVariableSchema fields[] = {
			MakeSchema("stats", VmType::Struct, &stats),
			MakeSchema("items", VmType::Array_Struct, &item),
			MakeSchema("note", VmType::String),
		};
		const VmStructSchema schema{ 3, fields, VmStructSchema::Flag_None };
		const std::vector<MetadataField> metadata = {
			{ "stats", MetadataValue::FromStruct({ { "hp", MetadataValue::FromInt64(7) }, { "tag", MetadataValue::FromString("x") } }) },
			{ "items", MetadataValue::FromArray({ MetadataValue::FromStruct({ { "id", MetadataValue::FromInt64(1) } }),
			               MetadataValue::FromStruct({ { "id", MetadataValue::FromInt64(2) } }) }) },
			{ "note", MetadataValue::FromString("after the structs") },
		};
		const std::string expected =
		    "070000007800"
		    "02000000"
		    "0100000000000000"
		    "0200000000000000"
		    "616674657220746865207374727563747300";
		std::string hex, error;
		const bool written = WriteBySchema(schema, metadata, hex, error);
		Report(ctx, written && hex == expected, "Struct and Array_Struct fields are written by their own schemas", written ? hex : error);

		Allocator alloc;
		const ByteArray bytes = HexToBytes(expected);
		ReadView r((void*)bytes.data(), bytes.size());
		VmDynamicStruct read{};
		const bool decoded = Read(read, schema, r, alloc);
		const VmDynamicVariable* readStats = decoded ? read[SmallString("stats")] : nullptr;
		const VmDynamicVariable* hp = readStats ? readStats->data.structure[SmallString("hp")] : nullptr;
		const VmDynamicVariable* tag = readStats ? readStats->data.structure[SmallString("tag")] : nullptr;
		const VmDynamicVariable* readItems = decoded ? read[SmallString("items")] : nullptr;
		const VmDynamicVariable* first = readItems && readItems->arrayLength == 2 ? readItems->data.structureArray.structs[0][SmallString("id")] : nullptr;
		const VmDynamicVariable* second = readItems && readItems->arrayLength == 2 ? readItems->data.structureArray.structs[1][SmallString("id")] : nullptr;
		const VmDynamicVariable* note = decoded ? read[SmallString("note")] : nullptr;
		const bool ok = hp && hp->data.int32 == 7 && tag && std::string(tag->data.string) == "x" && first && first->data.int64 == 1 && second &&
		                second->data.int64 == 2 && note && std::string(note->data.string) == "after the structs";
		Report(ctx, ok, "Struct and Array_Struct fields read back by their own schemas");
	}
}

} // namespace testcases
