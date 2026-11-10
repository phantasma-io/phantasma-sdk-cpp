#include "test_cases.h"

namespace testcases {
using namespace testutil;

// The values MetadataHelper takes for NFT data and series metadata: Int64 and Int256 values given as
// strings of decimal digits. NFT data is written by its schema: only the values, in schema order, so
// the bytes below are the layout the chain reads.
// The expected bytes are what the TypeScript SDK writes for the same schema and values, and the C# SDK's
// tests compose the same layouts.

namespace {

// Returns the bytes of one value written as the only field of type `type`, or the refusal.
std::string FieldBytes(VmType type, const MetadataValue& value)
{
	VmNamedVariableSchema field = MakeSchema("v", type);
	const VmStructSchema schema{ 1, &field, VmStructSchema::Flag_None };
	std::string hex, error;
	return WriteBySchema(schema, { { "v", value } }, hex, error) ? hex : error;
}

// Pushes the one field `schema` names. Answers what MetadataHelper::PushMetadataField answers.
bool PushOne(const VmNamedVariableSchema& schema, const MetadataValue& value, Allocator& alloc, std::vector<VmNamedDynamicVariable>& fields, std::string& error)
{
	const std::vector<MetadataField> metadata = { { schema.name.c_str(), value } };
	return MetadataHelper::PushMetadataField(schema, fields, metadata, alloc, error);
}

const char* const kInt64Range =
    "must be between -9223372036854775808 and 9223372036854775807 or between 0 and 18446744073709551615 (Int64)";
const char* const kInt256Range =
    "must be between -57896044618658097711785492504343953926634992332820282019728792003956564819968 and "
    "57896044618658097711785492504343953926634992332820282019728792003956564819967 or between 0 and "
    "115792089237316195423570985008687907853269984665640564039457584007913129639935 (Int256)";

void RunDigitStringTests(TestContext& ctx)
{
	// A value can arrive as text, for example from JSON written by a tool that keeps a number above
	// 2^53 - 1 as a string, because a JSON number that large loses digits in JavaScript.
	{
		Allocator alloc;
		std::vector<VmNamedDynamicVariable> fields;
		std::string error;
		const bool pushed = PushOne(MakeSchema("supply", VmType::Int64), MetadataValue::FromString("9007199254740993"), alloc, fields, error);
		Report(ctx, pushed && fields[0].value.type == VmType::Int64 && fields[0].value.data.int64 == 9007199254740993ull,
		    "an Int64 field takes a string of decimal digits above the JavaScript safe range", error);
	}

	// The range of a number holds for a string too: the signed range, or 0 to the unsigned maximum. A
	// value above the signed maximum is written as its two's complement.
	{
		const std::pair<const char*, const char*> edges[] = {
			{ "9007199254740993", "0100000000002000" },
			{ "-12", "F4FFFFFFFFFFFFFF" },
			{ "-9223372036854775808", "0000000000000080" },
			{ "9223372036854775807", "FFFFFFFFFFFFFF7F" },
			{ "18446744073709551615", "FFFFFFFFFFFFFFFF" },
			// Leading zeros do not count against the range, and minus zero is zero.
			{ "0000000000000000000000000042", "2A00000000000000" },
			{ "-0", "0000000000000000" },
		};
		for( const auto& edge : edges )
		{
			const std::string got = FieldBytes(VmType::Int64, MetadataValue::FromString(edge.first));
			Report(ctx, got == edge.second, std::string("an Int64 field writes the digit string ") + edge.first, got);
		}
	}
	{
		Allocator alloc;
		std::vector<VmNamedDynamicVariable> fields;
		std::string above, below;
		ExpectRefused(ctx, "an Int64 digit string of 2^64 is refused", std::string("Metadata field 'supply' ") + kInt64Range,
		    PushOne(MakeSchema("supply", VmType::Int64), MetadataValue::FromString("18446744073709551616"), alloc, fields, above), above);
		ExpectRefused(ctx, "an Int64 digit string below -2^63 is refused", std::string("Metadata field 'supply' ") + kInt64Range,
		    PushOne(MakeSchema("supply", VmType::Int64), MetadataValue::FromString("-9223372036854775809"), alloc, fields, below), below);
	}
	{
		const std::pair<const char*, const char*> edges[] = {
			{ "-123", "8185" },
			{ "-57896044618658097711785492504343953926634992332820282019728792003956564819968",
			    "A00000000000000000000000000000000000000000000000000000000000000080" },
			{ "57896044618658097711785492504343953926634992332820282019728792003956564819967",
			    "20FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF7F" },
			// 2^256 - 1 is the two's complement of -1, and the chain's layout writes it so.
			{ "115792089237316195423570985008687907853269984665640564039457584007913129639935", "80" },
			{ "18446744073709551615", "08FFFFFFFFFFFFFFFF" },
		};
		for( const auto& edge : edges )
		{
			const std::string got = FieldBytes(VmType::Int256, MetadataValue::FromString(edge.first));
			Report(ctx, got == edge.second, std::string("an Int256 field writes the digit string ") + edge.first, got);
		}
	}
	// Numbers keep the forms they had: a value written as a number and as digits gives the same bytes.
	{
		const std::string negative = FieldBytes(VmType::Int256, MetadataValue::FromInt64(-123));
		Report(ctx, negative == "8185", "an Int256 field writes a negative Int64 value", negative);
		const std::string unsignedMax = FieldBytes(VmType::Int256, MetadataValue::FromUInt64(std::numeric_limits<uint64_t>::max()));
		Report(ctx, unsignedMax == "08FFFFFFFFFFFFFFFF", "an Int256 field writes a UInt64 value", unsignedMax);
	}
	{
		Allocator alloc;
		std::vector<VmNamedDynamicVariable> fields;
		std::string above, below;
		ExpectRefused(ctx, "an Int256 digit string of 2^256 is refused", std::string("Metadata field 'delta' ") + kInt256Range,
		    PushOne(MakeSchema("delta", VmType::Int256),
		        MetadataValue::FromString("115792089237316195423570985008687907853269984665640564039457584007913129639936"), alloc, fields, above),
		    above);
		ExpectRefused(ctx, "an Int256 digit string below -2^255 is refused", std::string("Metadata field 'delta' ") + kInt256Range,
		    PushOne(MakeSchema("delta", VmType::Int256),
		        MetadataValue::FromString("-57896044618658097711785492504343953926634992332820282019728792003956564819969"), alloc, fields, below),
		    below);
	}

	// Only decimal digits with an optional leading minus count, and the whole string must be them.
	{
		const std::pair<const char*, const char*> notDigits[] = {
			{ "0x10", "a hex string" },
			{ "12\n", "a line break after the digits" },
			{ "+5", "a plus sign" },
			{ " 5", "a space before the digits" },
			{ "", "an empty string" },
			{ "-", "a minus sign alone" },
			{ "1e3", "an exponent" },
		};
		for( const auto& text : notDigits )
		{
			Allocator alloc;
			std::vector<VmNamedDynamicVariable> fields;
			std::string error;
			ExpectRefused(ctx, std::string("an Int64 field refuses ") + text.second,
			    "Metadata field 'supply' must be an integer or a string of decimal digits",
			    PushOne(MakeSchema("supply", VmType::Int64), MetadataValue::FromString(text.first), alloc, fields, error), error);
		}
	}
	{
		Allocator alloc;
		std::vector<VmNamedDynamicVariable> fields;
		std::string hexError, bytesError, int32Error;
		ExpectRefused(ctx, "an Int256 field refuses a hex string", "Metadata field 'delta' must be an integer or a string of decimal digits",
		    PushOne(MakeSchema("delta", VmType::Int256), MetadataValue::FromString("0x10"), alloc, fields, hexError), hexError);
		ExpectRefused(ctx, "an Int64 field refuses a value that is neither an integer nor a string",
		    "Metadata field 'supply' must be an integer or a string of decimal digits",
		    PushOne(MakeSchema("supply", VmType::Int64), MetadataValue::FromBytes({ (Byte)1 }), alloc, fields, bytesError), bytesError);
		// Strings are taken for Int64 and Int256 only, the two widths that JSON numbers cannot hold.
		ExpectRefused(ctx, "an Int32 field refuses a string of decimal digits", "Metadata field 'royalties' must be a number",
		    PushOne(MakeSchema("royalties", VmType::Int32), MetadataValue::FromString("7"), alloc, fields, int32Error), int32Error);
	}

	{
		const std::string got = FieldBytes(VmType::Array_Int256,
		    MetadataValue::FromArray({ MetadataValue::FromString("1"),
		        MetadataValue::FromString("115792089237316195423570985008687907853269984665640564039457584007913129639935") }));
		Report(ctx, got == "02000000010180", "the items of an Array_Int256 field take strings of decimal digits", got);
	}
	{
		Allocator alloc;
		std::vector<VmNamedDynamicVariable> fields;
		std::string error;
		const bool pushed = PushOne(MakeSchema("ids", VmType::Array_Int64),
		    MetadataValue::FromArray({ MetadataValue::FromString("1"), MetadataValue::FromString("-9223372036854775808") }), alloc, fields, error);
		const VmDynamicVariable* value = pushed ? &fields[0].value : nullptr;
		Report(ctx,
		    value && value->arrayLength == 2 && value->data.int64Array[0] == 1 && value->data.int64Array[1] == 0x8000000000000000ull,
		    "the items of an Array_Int64 field take strings of decimal digits", error);

		std::string itemError;
		ExpectRefused(ctx, "an item outside the range is named by its index", std::string("Metadata field 'ids[1]' ") + kInt64Range,
		    PushOne(MakeSchema("ids", VmType::Array_Int64),
		        MetadataValue::FromArray({ MetadataValue::FromString("1"), MetadataValue::FromString("18446744073709551616") }), alloc, fields,
		        itemError),
		    itemError);
	}
}

} // namespace

void RunMetadataValueTests(TestContext& ctx)
{
	RunDigitStringTests(ctx);
}

} // namespace testcases
