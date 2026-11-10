#include "test_cases.h"

namespace testcases {
using namespace testutil;

// A Struct or Array_Struct field carries the schema of its struct, and the chain writes the field's
// values by it. TokenSchemasBuilder::BuildFromFields builds that schema from FieldType::fields, in the
// given order and with no flags. The expected schema bytes are what the TypeScript SDK builds for the
// same fields from schemas JSON.

namespace {

// The five display fields every NFT token schema needs, so that BuildFromFields accepts the schemas,
// then `extra`.
std::vector<FieldType> RomFields(const FieldType& extra)
{
	return {
		FieldType{ "name", VmType::String },
		FieldType{ "description", VmType::String },
		FieldType{ "imageURL", VmType::String },
		FieldType{ "infoURL", VmType::String },
		FieldType{ "royalties", VmType::Int32 },
		extra,
	};
}

FieldType Stats()
{
	return FieldType{ "stats", VmType::Struct, { FieldType{ "level", VmType::Int32 }, FieldType{ "class", VmType::String } } };
}

} // namespace

void RunTokenSchemasNestedTests(TestContext& ctx)
{
	{
		TokenSchemasOwned schemas;
		std::string error;
		const bool built = TokenSchemasBuilder::BuildFromFields({}, RomFields(Stats()), {}, schemas, error);
		const std::string got = built ? RomFieldSchemaHex(schemas, "stats") : error;
		Report(ctx, got == kStatsSchemaHex, "a Struct field gets the schema of its struct, in the given order and with no flags", got);
	}
	{
		TokenSchemasOwned schemas;
		std::string error;
		const FieldType items{ "items", VmType::Array_Struct, { FieldType{ "id", VmType::Int64 } } };
		const bool built = TokenSchemasBuilder::BuildFromFields({}, RomFields(items), {}, schemas, error);
		const std::string got = built ? RomFieldSchemaHex(schemas, "items") : error;
		Report(ctx, got == kItemsSchemaHex, "an Array_Struct field gets the schema of each of its structs", got);
	}
	{
		TokenSchemasOwned schemas;
		std::string error;
		const bool built = TokenSchemasBuilder::BuildFromFields({}, RomFields(FieldType{ "outer", VmType::Struct, { Stats() } }), {}, schemas, error);
		const std::string got = built ? RomFieldSchemaHex(schemas, "outer") : error;
		Report(ctx, got == kOuterSchemaHex, "a struct field inside a struct field gets its schema too", got);
	}

	// A copy points at the struct schemas the original built, so it has to keep them alive. The original
	// is gone before the copy is read, and blocks of the size of its lists are handed out again in
	// between. A copy that pointed at freed lists would read those blocks instead.
	{
		TokenSchemasOwned copy;
		{
			TokenSchemasOwned original;
			std::string error;
			TokenSchemasBuilder::BuildFromFields({}, RomFields(FieldType{ "outer", VmType::Struct, { Stats() } }), {}, original, error);
			copy = original;
		}
		std::vector<std::vector<VmNamedVariableSchema>> reused;
		for( size_t i = 0; i != 16; ++i )
		{
			reused.push_back(std::vector<VmNamedVariableSchema>(1 + i % 2, MakeSchema("zz", VmType::Bytes64)));
		}
		const std::string got = RomFieldSchemaHex(copy, "outer");
		Report(ctx, got == kOuterSchemaHex, "a copy of the schemas keeps their struct schemas after the original is gone", got);
	}

	// The display fields come in the order of this SDK's sorted ROM schema, and the struct's fields in
	// the order of its own schema. The bytes are what the TypeScript SDK writes for a ROM schema in that
	// order.
	{
		const std::vector<MetadataField> metadata = {
			{ "name", MetadataValue::FromString("Hero") },
			{ "description", MetadataValue::FromString("An NFT with a struct field") },
			{ "imageURL", MetadataValue::FromString("https://example.invalid/hero.png") },
			{ "infoURL", MetadataValue::FromString("https://example.invalid/hero") },
			{ "royalties", MetadataValue::FromInt64(0) },
			// The keys of a struct can come in any order. The data follows the struct's schema.
			{ "stats", MetadataValue::FromStruct({ { "class", MetadataValue::FromString("mage") }, { "level", MetadataValue::FromInt64(3) } }) },
		};
		const std::string expected =
		    "416E204E46542077697468206120737472756374206669656C6400"
		    "68747470733A2F2F6578616D706C652E696E76616C69642F6865726F2E706E6700"
		    "68747470733A2F2F6578616D706C652E696E76616C69642F6865726F00"
		    "4865726F00"
		    "00000000"
		    "03000000"
		    "6D61676500";
		TokenSchemasOwned schemas;
		ByteArray rom;
		std::string error;
		const bool built = TokenSchemasBuilder::BuildFromFields({}, RomFields(Stats()), {}, schemas, error) &&
		                   PhantasmaNftRomBuilder::BuildAndSerialize(schemas.View().rom, metadata, rom, error);
		const std::string got = built ? ToUpper(BytesToHex(rom)) : error;
		Report(ctx, got == expected, "the NFT data of a struct field follows the struct's own schema", got);
	}

	// The fields of a struct are checked as the fields of the three lists are.
	{
		TokenSchemasOwned schemas;
		std::string error;
		const FieldType twice{ "stats", VmType::Struct, { FieldType{ "level", VmType::Int32 }, FieldType{ "level", VmType::Int64 } } };
		ExpectRefused(ctx, "a struct whose fields repeat a name is refused", "Duplicate field name: level",
		    TokenSchemasBuilder::BuildFromFields({}, RomFields(twice), {}, schemas, error), error);
	}
}

} // namespace testcases
