#include "test_cases.h"

namespace testcases {
using namespace testutil;

// The JSON of token schemas names the fields of a Struct or Array_Struct field under "fields", and
// TokenSchemasBuilder::FromJson builds that field's struct schema from them. FromJson compiles only with
// PHANTASMA_RAPIDJSON, so these tests are a binary of their own. The expected schemas are what the
// TypeScript SDK builds from the same JSON, and the refusals carry the C# SDK's messages.

namespace {

// The display fields every NFT token schema needs, so that FromJson accepts the schemas.
const std::string kStandardRomFields =
    R"({ "name": "name", "type": "String" },)"
    R"({ "name": "description", "type": "String" },)"
    R"({ "name": "imageURL", "type": "String" },)"
    R"({ "name": "infoURL", "type": "String" },)"
    R"({ "name": "royalties", "type": "Int32" })";

const std::string kStats =
    R"({ "name": "stats", "type": "Struct", "fields": [{ "name": "level", "type": "Int32" }, { "name": "class", "type": "String" }] })";

// Reads schemas whose ROM list holds the display fields, then `extraField`.
bool RomFromJson(const std::string& extraField, TokenSchemasOwned& schemas, std::string& error)
{
	const std::string json = R"({ "seriesMetadata": [], "rom": [)" + kStandardRomFields + "," + extraField + R"(], "ram": [] })";
	return TokenSchemasBuilder::FromJson(json, schemas, error);
}

void RunTokenSchemasJsonTests(TestContext& ctx)
{
	{
		TokenSchemasOwned schemas;
		std::string error;
		const std::string got = RomFromJson(kStats, schemas, error) ? RomFieldSchemaHex(schemas, "stats") : error;
		Report(ctx, got == kStatsSchemaHex, "a Struct field gets the schema of its struct, in the given order", got);
	}
	{
		TokenSchemasOwned schemas;
		std::string error;
		const std::string items = R"({ "name": "items", "type": "Array_Struct", "fields": [{ "name": "id", "type": "Int64" }] })";
		const std::string got = RomFromJson(items, schemas, error) ? RomFieldSchemaHex(schemas, "items") : error;
		Report(ctx, got == kItemsSchemaHex, "an Array_Struct field gets the schema of each of its structs", got);
	}
	{
		TokenSchemasOwned schemas;
		std::string error;
		const std::string outer = R"({ "name": "outer", "type": "Struct", "fields": [)" + kStats + "] }";
		const std::string got = RomFromJson(outer, schemas, error) ? RomFieldSchemaHex(schemas, "outer") : error;
		Report(ctx, got == kOuterSchemaHex, "a struct field inside a struct field gets its schema too", got);
	}

	// The chain writes the values of a Struct or Array_Struct field by its struct's schema, so such a
	// field needs its fields, and no other field can have them. A message names the field by its path:
	// the list, then each struct field above it.
	const struct
	{
		const char* name;
		std::string field;
		const char* message;
	} refused[] = {
		{ "a Struct field without fields is refused", R"({ "name": "stats", "type": "Struct" })",
		    "rom.stats has the type Struct and needs a fields array" },
		{ "a Struct field whose fields are not an array is refused", R"({ "name": "stats", "type": "Struct", "fields": {} })",
		    "rom.stats has the type Struct and needs a fields array" },
		{ "an Array_Struct field whose struct has no fields is refused", R"({ "name": "items", "type": "Array_Struct", "fields": [] })",
		    "rom.items has the type Array_Struct and needs at least one field" },
		{ "fields on a field of another type are refused", R"({ "name": "title", "type": "String", "fields": [] })",
		    "rom.title has the type String and cannot have fields" },
		{ "a malformed field type inside a struct names the struct field",
		    R"({ "name": "stats", "type": "Struct", "fields": [{ "name": "level", "type": 5 }] })", "rom.stats field type must be string" },
		{ "a field name missing inside a struct names the struct field",
		    R"({ "name": "stats", "type": "Struct", "fields": [{ "type": "Int32" }] })", "rom.stats field name must be string" },
		{ "a field inside a struct that is not an object is refused", R"({ "name": "stats", "type": "Struct", "fields": [7] })",
		    "rom.stats field must be an object" },
		{ "a struct field inside a struct field is named by its whole path",
		    R"({ "name": "outer", "type": "Struct", "fields": [{ "name": "stats", "type": "Struct" }] })",
		    "rom.outer.stats has the type Struct and needs a fields array" },
		{ "an unknown type inside a struct is refused",
		    R"({ "name": "stats", "type": "Struct", "fields": [{ "name": "level", "type": "Int33" }] })", "Unknown VmType: Int33" },
		{ "a list entry that is not an object is refused", "5", "rom field must be an object" },
		{ "a list entry whose name is not a string is refused", R"({ "name": 1, "type": "String" })", "rom field name must be string" },
	};
	for( const auto& c : refused )
	{
		TokenSchemasOwned schemas;
		std::string error;
		const bool read = RomFromJson(c.field, schemas, error);
		Report(ctx, !read && error == c.message, c.name, read ? "the schemas were accepted" : error);
	}
	{
		TokenSchemasOwned schemas;
		std::string error;
		const std::string json =
		    R"({ "seriesMetadata": [{ "name": "set", "type": "Struct" }], "rom": [)" + kStandardRomFields + R"(], "ram": [] })";
		const bool read = TokenSchemasBuilder::FromJson(json, schemas, error);
		Report(ctx, !read && error == "seriesMetadata.set has the type Struct and needs a fields array",
		    "a struct field of the series metadata is named by its list", read ? "the schemas were accepted" : error);
	}

	// What FromJson read before struct fields existed reads as it did.
	{
		TokenSchemasOwned schemas;
		std::string error;
		const std::string json = R"({ "seriesMetadata": [], "rom": [)" + kStandardRomFields + R"(], "ram": [] })";
		Report(ctx, TokenSchemasBuilder::FromJson(json, schemas, error) && RomFieldSchemaHex(schemas, "royalties") == "09726F79616C746965730A",
		    "schemas without struct fields read as before", error);
	}
	{
		TokenSchemasOwned schemas;
		std::string error;
		const std::string json = R"({ "seriesMetadata": [], "rom": [)" + kStandardRomFields + "] }";
		const bool read = TokenSchemasBuilder::FromJson(json, schemas, error);
		Report(ctx, !read && error == "ram must be an array", "a missing list is refused as before", read ? "the schemas were accepted" : error);
	}
}

} // namespace
} // namespace testcases

int main()
{
	testutil::TestContext ctx;
	testcases::RunTokenSchemasJsonTests(ctx);

	if( ctx.failed == 0 )
	{
		std::cout << "All " << ctx.total << " token schemas JSON tests passed." << std::endl;
		return 0;
	}

	std::cerr << ctx.failed << " of " << ctx.total << " token schemas JSON tests failed." << std::endl;
	return 1;
}
