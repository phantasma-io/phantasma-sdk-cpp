#include "test_cases.h"

namespace testcases {
using namespace testutil;

// The chain keeps the token metadata keys that start with `_` for its own settings: staking,
// inflation and the contract seal. It refuses any other such key in the metadata a token is left
// with. It also refuses `_brn` in an update, because only genesis sets it. An update may still name
// another `_` key to remove it, with an empty Array_Dynamic, the only value the chain reads as a
// removal. These tests hold the builders of this SDK to the same rule.

namespace {

// The arguments of an update of token 42 that removes `_note`, as the C# SDK's tests pin them: the
// u64 token id, the u32 field count, the length-prefixed name, then the type byte of Array_Dynamic
// (0x01) and its u32 item count 0. The chain reads exactly this value as a removal.
const std::string kRemoveNoteArgsHex =
    "2A00000000000000"
    "01000000"
    "055F6E6F7465"
    "01"
    "00000000";

// Builds the update of token 42 that names one field. Answers what UpdateTokenMetadataTxHelper::BuildTx
// answers.
bool UpdateOneField(const char* key, const VmDynamicVariable& value, TxEnvelope& env, std::string& error)
{
	VmNamedDynamicVariable field{ SmallString(key), value };
	return UpdateTokenMetadataTxHelper::BuildTx(42, VmDynamicStruct::Sort(1, &field), Bytes32(ByteArray(32, 3)), env, error);
}

} // namespace

void RunTokenReservedKeysTests(TestContext& ctx)
{
	// `_sl` is a chain key. The chain takes it only as an Int64, and these fields are strings.
	for( const char* key : { "_note", "_sl", "_brn", "_phs" } )
	{
		std::vector<std::pair<std::string, std::string>> fields = TokenCreationFields();
		fields.push_back({ key, "x" });
		ByteArray metadata;
		std::string error;
		ExpectRefused(ctx, std::string("a string field named ") + key + " is refused", std::string(key) + " is reserved for the chain",
		    TokenMetadataBuilder::BuildAndSerialize(fields, metadata, error), error);
	}
	{
		std::vector<std::pair<std::string, std::string>> fields = TokenCreationFields();
		fields.push_back({ "my_note", "a" });
		fields.push_back({ "note_", "b" });
		ByteArray metadata;
		std::string error;
		const bool built = TokenMetadataBuilder::BuildAndSerialize(fields, metadata, error);

		Allocator alloc;
		ReadView r(metadata.data(), metadata.size());
		VmDynamicStruct read{};
		const bool decoded = built && Read(read, r, alloc);
		const VmDynamicVariable* inside = decoded ? read[SmallString("my_note")] : nullptr;
		const VmDynamicVariable* last = decoded ? read[SmallString("note_")] : nullptr;
		const bool ok = inside && inside->type == VmType::String && std::string(inside->data.string) == "a" &&
		                last && last->type == VmType::String && std::string(last->data.string) == "b";
		Report(ctx, ok, "a key whose underscore is not its first character is kept", error);
	}

	{
		TxEnvelope env;
		std::string valueError, removalError;
		ExpectRefused(ctx, "an update that sets _brn is refused", "_brn is set at genesis only",
		    UpdateOneField("_brn", VmDynamicVariable(uint256((uint64_t)70)), env, valueError), valueError);
		ExpectRefused(ctx, "an update that removes _brn is refused", "_brn is set at genesis only",
		    UpdateOneField("_brn", MetadataFieldRemoval(), env, removalError), removalError);
	}
	{
		TxEnvelope env;
		std::string noteError, scriptError, lookalikeError;
		ExpectRefused(ctx, "an update may not set _note", "_note is reserved for the chain: an update can only remove it",
		    UpdateOneField("_note", VmDynamicVariable("x"), env, noteError), noteError);
		// The chain reads `_phs` in block 0 only, so an update may not set it.
		const Byte script[] = { 1 };
		ExpectRefused(ctx, "an update may not set _phs", "_phs is reserved for the chain",
		    UpdateOneField("_phs", VmDynamicVariable(ByteView{ script, sizeof(script) }), env, scriptError), scriptError);
		// A key that resembles the contract seal is not the seal.
		ExpectRefused(ctx, "an update may not set _sealed", "_sealed is reserved for the chain",
		    UpdateOneField("_sealed", VmDynamicVariable((int64_t)1), env, lookalikeError), lookalikeError);
	}
	// A Dynamic value without data is written as a Dynamic around an empty Array_Dynamic. The chain
	// stores that as a value, so it removes nothing.
	{
		TxEnvelope env;
		std::string error;
		ExpectRefused(ctx, "a Dynamic value without data does not remove _note", "_note is reserved for the chain",
		    UpdateOneField("_note", VmDynamicVariable{}, env, error), error);
	}
	{
		TxEnvelope env;
		std::string error;
		const bool built = UpdateOneField("_note", MetadataFieldRemoval(), env, error);
		const std::string got = built ? ToUpper(BytesToHex(BytesFromView(env.msg.call.args))) : error;
		Report(ctx, built && got == kRemoveNoteArgsHex, "an update removes such a key with an empty Array_Dynamic", got);
	}

	// The keys an update may set, as the chain names them. The SDK checks the reserve only, so an Int64
	// stands for every value here; the chain checks the type of each setting itself. The Int64 1 is
	// also the one value `_sealed_c` takes.
	for( const char* key : { "_soi", "_sot", "_srt", "_srp", "_srm", "_srd", "_sl", "_sbt", "_sbm", "_sbd", "_sbl",
	         "_ip", "_ia", "_iib", "_iie", "_ifr", "_sealed_c" } )
	{
		TxEnvelope env;
		std::string error;
		Report(ctx, UpdateOneField(key, VmDynamicVariable((int64_t)1), env, error), std::string("an update may set the chain key ") + key, error);
	}

	// Write sorts the fields of a struct, so a caller may pass them in any order. A lookup that needed
	// them sorted would miss these two keys, which follow `description` here.
	{
		VmNamedDynamicVariable brnLast[2] = {
			{ SmallString("description"), VmDynamicVariable("x") },
			{ SmallString("_brn"), VmDynamicVariable((int64_t)1) },
		};
		VmNamedDynamicVariable sealLast[2] = {
			{ SmallString("description"), VmDynamicVariable("x") },
			{ SmallString("_sealed_c"), VmDynamicVariable("1") },
		};
		VmDynamicStruct brnUpdate{};
		brnUpdate.numFields = 2;
		brnUpdate.fields = brnLast;
		VmDynamicStruct sealUpdate{};
		sealUpdate.numFields = 2;
		sealUpdate.fields = sealLast;
		const Bytes32 owner(ByteArray(32, 3));
		TxEnvelope env;
		std::string brnError, sealError;
		ExpectRefused(ctx, "_brn is found among fields that are not sorted", "_brn is set at genesis only",
		    UpdateTokenMetadataTxHelper::BuildTx(42, brnUpdate, owner, env, brnError), brnError);
		ExpectRefused(ctx, "_sealed_c is found among fields that are not sorted", "_sealed_c must be the Int64 1",
		    UpdateTokenMetadataTxHelper::BuildTx(42, sealUpdate, owner, env, sealError), sealError);
	}
}

} // namespace testcases
