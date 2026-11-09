#include "test_cases.h"

namespace testcases {
using namespace testutil;

// The contract seal of a token is its metadata field `_sealed_c` holding the Int64 1. The chain
// refuses a VM contract for such a token and any removal of the field, and it refuses `_sealed_c`
// with any other type or value. These tests hold the builders of this SDK to that one form. They also
// check the Token.UpdateTokenMetadata call that carries the seal to an existing token: its arguments,
// its refusals, its limits and its signed envelope.

namespace {

// What the TypeScript SDK writes for TokenCreationFields() with { sealContract: true }. A struct is
// written sorted by field name, so `_sealed_c` comes first: the u32 field count 5, the
// length-prefixed name, the type byte of Int64 (0x0C) and the value 1. The four string fields follow
// as an unsealed creation writes them.
const std::string kSealedCreationHex =
    "05000000"
    "095F7365616C65645F63"
    "0C0100000000000000"
    "0B6465736372697074696F6E"
    "164D79207465737420746F6B656E206465736372697074696F6E00"
    "0469636F6E"
    "16646174613A696D6167652F706E673B6261736536342C6956424F5277304B47676F414141414E5355684555674141414145"
    "41414141424341514141414331484177434141414143306C45515652346E474D41415141414251414244516F7474414141"
    "4141424A52553545726B4A6767673D3D00"
    "046E616D65"
    "164D79207465737420746F6B656E2100"
    "0375726C"
    "16687474703A2F2F6578616D706C652E636F6D00";

// The arguments of a seal update of token 42, as the C# SDK's tests pin them. The chain reads a u64
// token id, then a dynamic struct: a u32 field count, and per field a length-prefixed name, a type
// byte and the value.
const std::string kSealUpdateArgsHex =
    "2A00000000000000"
    "01000000"
    "095F7365616C65645F63"
    "0C0100000000000000";

// The arguments of an update of token 9 that sets the string field `n` to "alpha", as the C# SDK's
// tests pin them. A string value is its type byte 0x16, then the text and a terminating zero.
const std::string kStringUpdateArgsHex =
    "0900000000000000"
    "01000000"
    "016E"
    "16616C70686100";

// Returns the arguments of the envelope's call as upper-case hex.
std::string ArgsHex(const TxEnvelope& env)
{
	return ToUpper(BytesToHex(BytesFromView(env.msg.call.args)));
}

// Returns an update that holds `field` alone. The update points at `field`, so `field` has to outlive
// it.
VmDynamicStruct Update(VmNamedDynamicVariable& field)
{
	return VmDynamicStruct::Sort(1, &field);
}

} // namespace

void RunTokenSealTests(TestContext& ctx)
{
	const Bytes32 owner(ByteArray(32, 3));

	{
		TokenMetadataOptions options;
		options.sealContract = true;
		ByteArray metadata;
		std::string error;
		const bool built = TokenMetadataBuilder::BuildAndSerialize(TokenCreationFields(), metadata, error, options);
		const std::string got = ToUpper(BytesToHex(metadata));
		Report(ctx, built && got == kSealedCreationHex, "a sealed creation writes _sealed_c beside the string fields",
		    built ? got : error);

		Allocator alloc;
		ReadView r(metadata.data(), metadata.size());
		VmDynamicStruct read{};
		const bool decoded = built && Read(read, r, alloc);
		const VmDynamicVariable* seal = decoded ? read[SmallString("_sealed_c")] : nullptr;
		Report(ctx, decoded && read.numFields == 5 && seal && seal->type == VmType::Int64 && seal->data.int64 == 1,
		    "the seal of a creation reads back as the Int64 1");
	}
	{
		ByteArray metadata;
		std::string error;
		const bool built = TokenMetadataBuilder::BuildAndSerialize(TokenCreationFields(), metadata, error);
		Allocator alloc;
		ReadView r(metadata.data(), metadata.size());
		VmDynamicStruct read{};
		const bool decoded = built && Read(read, r, alloc);
		Report(ctx, decoded && read.numFields == 4 && !read[SmallString("_sealed_c")],
		    "a creation without the option carries no seal", error);
	}
	{
		std::vector<std::pair<std::string, std::string>> fields = TokenCreationFields();
		fields.push_back({ "_sealed_c", "1" });
		ByteArray metadata;
		std::string error;
		ExpectRefused(ctx, "a string _sealed_c is refused with the option that seals instead", "TokenMetadataOptions::sealContract = true",
		    TokenMetadataBuilder::BuildAndSerialize(fields, metadata, error), error);
	}

	// The update holds the seal alone, so the chain keeps every other field of the token.
	{
		Allocator alloc;
		const VmDynamicStruct update = TokenMetadataBuilder::BuildContractSealUpdate(alloc);
		const bool ok = update.numFields == 1 && update.fields[0].name == SmallString("_sealed_c") &&
		                update.fields[0].value.type == VmType::Int64 && update.fields[0].value.data.int64 == 1;
		Report(ctx, ok, "the seal update holds the one field _sealed_c = Int64 1");
	}
	{
		Allocator alloc;
		TxEnvelope env;
		std::string error;
		const bool built = UpdateTokenMetadataTxHelper::BuildTx(42, TokenMetadataBuilder::BuildContractSealUpdate(alloc), owner, env, error);
		const bool ok = built && env.msg.type == Blockchain::TxTypes::Call && env.msg.gasFrom == owner &&
		                env.msg.call.moduleId == (uint32_t)ModuleId::Token &&
		                env.msg.call.methodId == (uint32_t)TokenContract_Methods::UpdateTokenMetadata && ArgsHex(env) == kSealUpdateArgsHex;
		Report(ctx, ok, "a seal update is a Token.UpdateTokenMetadata call with the token id, then the update", built ? ArgsHex(env) : error);
	}
	{
		VmNamedDynamicVariable field{ SmallString("n"), VmDynamicVariable("alpha") };
		TxEnvelope env;
		std::string error;
		const bool built = UpdateTokenMetadataTxHelper::BuildTx(9, Update(field), owner, env, error);
		Report(ctx, built && ArgsHex(env) == kStringUpdateArgsHex, "an update of a string field writes the token id, then the field",
		    built ? ArgsHex(env) : error);
	}
	{
		Allocator alloc;
		TxEnvelope env;
		std::string error;
		ExpectRefused(ctx, "an update of token id 0, which the chain never assigns, is refused", "the chain assigns no token id 0",
		    UpdateTokenMetadataTxHelper::BuildTx(0, TokenMetadataBuilder::BuildContractSealUpdate(alloc), owner, env, error), error);
	}
	// An update without fields would change nothing and still pay the fee.
	{
		TxEnvelope env;
		std::string error;
		ExpectRefused(ctx, "an update without fields is refused", "A metadata update needs at least one field",
		    UpdateTokenMetadataTxHelper::BuildTx(42, VmDynamicStruct{}, owner, env, error), error);

		VmDynamicStruct counted{};
		counted.numFields = 1;
		std::string countedError;
		ExpectRefused(ctx, "an update that counts a field it does not point at is refused", "A metadata update needs at least one field",
		    UpdateTokenMetadataTxHelper::BuildTx(42, counted, owner, env, countedError), countedError);
	}
	// The value 1 of another integer type matters here. The value lives in a union, so a check of the
	// value alone would read an Int32 1 as 1.
	{
		VmNamedDynamicVariable asString{ SmallString("_sealed_c"), VmDynamicVariable("1") };
		VmNamedDynamicVariable asInt32{ SmallString("_sealed_c"), VmDynamicVariable((int32_t)1) };
		VmNamedDynamicVariable asTwo{ SmallString("_sealed_c"), VmDynamicVariable((int64_t)2) };
		TxEnvelope env;
		std::string stringError, int32Error, twoError;
		ExpectRefused(ctx, "a seal given as a string is refused", "_sealed_c must be the Int64 1",
		    UpdateTokenMetadataTxHelper::BuildTx(42, Update(asString), owner, env, stringError), stringError);
		ExpectRefused(ctx, "a seal given as an Int32 is refused", "_sealed_c must be the Int64 1",
		    UpdateTokenMetadataTxHelper::BuildTx(42, Update(asInt32), owner, env, int32Error), int32Error);
		ExpectRefused(ctx, "a seal of another value is refused", "_sealed_c must be the Int64 1",
		    UpdateTokenMetadataTxHelper::BuildTx(42, Update(asTwo), owner, env, twoError), twoError);
	}
	// An empty Array_Dynamic removes a field. The seal is never removed: the chain refuses that for a
	// sealed token, and a token without a seal has none.
	{
		VmNamedDynamicVariable removal{ SmallString("_sealed_c"), MetadataFieldRemoval() };
		TxEnvelope env;
		std::string error;
		ExpectRefused(ctx, "an update that removes the seal is refused", "_sealed_c must be the Int64 1",
		    UpdateTokenMetadataTxHelper::BuildTx(42, Update(removal), owner, env, error), error);
	}
	{
		Allocator alloc;
		TxLimits limits{};
		limits.maxGas = 30000;
		limits.maxData = 123;
		limits.expiry = 1759711416000ll;
		TxEnvelope env;
		std::string error;
		const bool built =
		    UpdateTokenMetadataTxHelper::BuildTx(42, TokenMetadataBuilder::BuildContractSealUpdate(alloc), owner, env, error, limits);
		Report(ctx, built && env.msg.maxGas == 30000 && env.msg.maxData == 123 && env.msg.expiry == 1759711416000ll,
		    "UpdateTokenMetadataTxHelper writes the limits it was given", error);
	}

	// Built, planned, signed and read back the way a wallet would. The planner has no formula for this
	// call, so its plan is the script budget and an upper bound.
	{
		const std::string wif = "KwPpBSByydVKqStGHAnZzQofCqhDmD2bfRgc9BmZqM3ZmsdWJw4d";
		const PhantasmaKeys keys = PhantasmaKeys::FromWIF(wif.c_str(), (int)wif.size());
		const Bytes32 ownerKey = ToBytes32(keys.GetPublicKey());

		Allocator alloc;
		TxEnvelope env;
		std::string error;
		const bool built = UpdateTokenMetadataTxHelper::BuildTx(42, TokenMetadataBuilder::BuildContractSealUpdate(alloc), ownerKey, env, error);

		FeePlanOptions options;
		options.witnessCount = 1; // the owner signs alone
		FeePlan plan;
		const bool planned = built && env.msg.maxGas == 0 && PlanFees(env.msg, PlanningConfig(), options, plan);
		Report(ctx, planned && plan.kinds.size() == 1 && plan.kinds[0] == NativeFeeKind::Script && !plan.exact,
		    "a metadata update is planned with the script budget, as an upper bound", error);

		env.msg = plan.Apply(env.msg);
		const ByteArray envelope = SignAndSerialize(env, keys);

		Allocator readStorage;
		Blockchain::SignedTxMsg parsed{};
		ByteView signedPortion{};
		const bool read = planned && Blockchain::ParseSignedTx(parsed, signedPortion, ByteView{ envelope.data(), envelope.size() }, readStorage);
		const bool ok = read && parsed.msg.type == Blockchain::TxTypes::Call && parsed.msg.maxGas == plan.maxGas &&
		                parsed.msg.call.moduleId == (uint32_t)ModuleId::Token &&
		                parsed.msg.call.methodId == (uint32_t)TokenContract_Methods::UpdateTokenMetadata &&
		                ToUpper(BytesToHex(BytesFromView(parsed.msg.call.args))) == kSealUpdateArgsHex &&
		                parsed.witnesses.numWitnesses == 1 && parsed.witnesses.witnesses[0].address == ownerKey;
		Report(ctx, ok, "a signed metadata update reads back with its call and its signer");
	}
}

} // namespace testcases
