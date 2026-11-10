#include "test_cases.h"

namespace testcases {
using namespace testutil;

// NFT data and series metadata are written by their schema: only the values, in schema order. Data
// written by a schema is read back by the same schema, as the chain reads it. A Struct or Array_Struct
// field inside is written by that field's own struct schema, and a Dynamic value with its own type. The
// expected bytes are what the TypeScript SDK writes for the same schema and values, and the C# SDK's
// tests compose the same layouts.

void RunVmSchemaTests(TestContext& ctx)
{
	// A Dynamic field is written as its value's type byte and then the value. An Array_Dynamic field is
	// a count and then each item in the same way.
	{
		VmNamedVariableSchema fields[] = {
			MakeSchema("extra", VmType::Dynamic),
			MakeSchema("extras", VmType::Array),
			MakeSchema("note", VmType::String),
		};
		const VmStructSchema schema{ 3, fields, VmStructSchema::Flag_None };
		const std::vector<MetadataField> metadata = {
			{ "extra", MetadataValue::FromDynamic(VmType::Int64, MetadataValue::FromString("9007199254740993")) },
			{ "extras", MetadataValue::FromArray({ MetadataValue::FromDynamic(VmType::String, MetadataValue::FromString("a")),
			                MetadataValue::FromDynamic(VmType::Int32, MetadataValue::FromInt64(7)) }) },
			{ "note", MetadataValue::FromString("after the dynamic values") },
		};
		const std::string expected =
		    "0C0100000000002000"
		    "02000000"
		    "166100"
		    "0A07000000"
		    "6166746572207468652064796E616D69632076616C75657300";
		std::string hex, error;
		const bool written = WriteBySchema(schema, metadata, hex, error);
		Report(ctx, written && hex == expected, "Dynamic and Array_Dynamic fields are written with the type of each value", written ? hex : error);

		// A Dynamic field reads back as a Dynamic around the value, which keeps its own type, so the data
		// read writes back to the same bytes. The items of an Array_Dynamic field read back as themselves.
		Allocator alloc;
		const ByteArray bytes = HexToBytes(expected);
		ReadView r((void*)bytes.data(), bytes.size());
		VmDynamicStruct read{};
		const bool decoded = Read(read, schema, r, alloc);
		const VmDynamicVariable* extra = decoded ? read[SmallString("extra")] : nullptr;
		const VmDynamicVariable* extras = decoded ? read[SmallString("extras")] : nullptr;
		const VmDynamicVariable* note = decoded ? read[SmallString("note")] : nullptr;
		const VmDynamicVariable* held = extra && extra->type == VmType::Dynamic ? extra->data.dynamic : nullptr;
		const bool ok = held && held->type == VmType::Int64 && held->data.int64 == 9007199254740993ull && extras &&
		                extras->arrayLength == 2 && extras->data.dynamicArray[0].type == VmType::String &&
		                std::string(extras->data.dynamicArray[0].data.string) == "a" && extras->data.dynamicArray[1].type == VmType::Int32 &&
		                extras->data.dynamicArray[1].data.int32 == 7 && note && std::string(note->data.string) == "after the dynamic values";
		Report(ctx, ok, "Dynamic and Array_Dynamic fields read back by their schema");

		ByteArray again;
		WriteView w(again);
		const bool rewritten = decoded && Write(read, schema, w);
		Report(ctx, rewritten && ToUpper(BytesToHex(again)) == expected, "the data read back writes the same bytes", ToUpper(BytesToHex(again)));
	}

	// A value that names its own type can be Dynamic as well: the type byte of Dynamic, then the value it
	// holds with that value's type byte.
	{
		const ByteArray bytes = HexToBytes("000C0500000000000000");
		Allocator alloc;
		ReadView r((void*)bytes.data(), bytes.size());
		VmDynamicVariable read{};
		const bool decoded = Read(read, r, alloc);
		const VmDynamicVariable* held = decoded && read.type == VmType::Dynamic ? read.data.dynamic : nullptr;
		Report(ctx, held && held->type == VmType::Int64 && held->data.int64 == 5, "a Dynamic value that names its type reads back wrapped");
		Report(ctx, decoded && ToUpper(BytesToHex(CarbonSerialize(read))) == "000C0500000000000000",
		    "a Dynamic value that names its type writes back unchanged");
	}

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
