//------------------------------------------------------------------------------
// Phantasma SDK: typing the arguments of a special-resolution call.
//------------------------------------------------------------------------------
// Part of PhantasmaAPI.h; include that file, not this one.
//------------------------------------------------------------------------------
#pragma once

#include "SpecialResolutionArgumentsGovernance.h"
#include "SpecialResolutionArgumentsToken.h"
#include "SpecialResolutionArgumentsVm.h"

namespace phantasma {
namespace rpc {

typedef std::shared_ptr<SpecialResolutionArgumentsBase> SpecialResolutionArgumentsPtr;
typedef SpecialResolutionArgumentsPtr (*SpecialResolutionArgumentsParser)(const JSONValue&, bool&);

// Adapts one shape parser to the table below.
template<class T, T (*Parse)(const JSONValue&, bool&)>
SpecialResolutionArgumentsPtr MakeSpecialResolutionArguments(const JSONValue& value, bool& jsonErr)
{
	return std::make_shared<T>(Parse(value, jsonErr));
}

struct SpecialResolutionArgumentsEntry {
	const Char* module;
	const Char* method;
	SpecialResolutionArgumentsParser parse;
};

// Module and method to the shape of that call's arguments. Mirrors the converter of the C# SDK and
// the node's SpecialResolutionHelper, which build these answers. A pair missing here is not an
// error: the node answers the raw argument buffer for anything it cannot decode, and an unmodeled
// pair keeps its JSON.
inline const SpecialResolutionArgumentsEntry* SpecialResolutionArgumentsTable(int& out_count)
{
	static const SpecialResolutionArgumentsEntry table[] = {
		{ PHANTASMA_LITERAL("governance"), PHANTASMA_LITERAL("SetGasConfig"),
		    &MakeSpecialResolutionArguments<GasConfigArguments, ParseGasConfigArguments> },
		{ PHANTASMA_LITERAL("governance"), PHANTASMA_LITERAL("SetChainConfig"),
		    &MakeSpecialResolutionArguments<ChainConfigArguments, ParseChainConfigArguments> },
		{ PHANTASMA_LITERAL("governance"), PHANTASMA_LITERAL("SpecialResolution"),
		    &MakeSpecialResolutionArguments<NestedResolutionArguments, ParseNestedResolutionArguments> },
		{ PHANTASMA_LITERAL("governance"), PHANTASMA_LITERAL("SetMetadata"),
		    &MakeSpecialResolutionArguments<MetadataArguments, ParseMetadataArguments> },
		{ PHANTASMA_LITERAL("governance"), PHANTASMA_LITERAL("SetNodeConfig"),
		    &MakeSpecialResolutionArguments<NodeConfigArguments, ParseNodeConfigArguments> },
		{ PHANTASMA_LITERAL("governance"), PHANTASMA_LITERAL("RegisterName"),
		    &MakeSpecialResolutionArguments<RegisterNameArguments, ParseRegisterNameArguments> },
		{ PHANTASMA_LITERAL("governance"), PHANTASMA_LITERAL("LookupName"),
		    &MakeSpecialResolutionArguments<AddressArguments, ParseAddressArguments> },
		{ PHANTASMA_LITERAL("governance"), PHANTASMA_LITERAL("LookupAddress"),
		    &MakeSpecialResolutionArguments<NameArguments, ParseNameArguments> },

		{ PHANTASMA_LITERAL("phantasma_vm"), PHANTASMA_LITERAL("ExecuteScript"),
		    &MakeSpecialResolutionArguments<ExecuteScriptArguments, ParseExecuteScriptArguments> },
		{ PHANTASMA_LITERAL("phantasma_vm"), PHANTASMA_LITERAL("RegisterTokenContract"),
		    &MakeSpecialResolutionArguments<RegisterTokenContractArguments, ParseRegisterTokenContractArguments> },
		{ PHANTASMA_LITERAL("phantasma_vm"), PHANTASMA_LITERAL("DeployContract"),
		    &MakeSpecialResolutionArguments<DeployContractArguments, ParseDeployContractArguments> },
		{ PHANTASMA_LITERAL("phantasma_vm"), PHANTASMA_LITERAL("IsContractDeployed"),
		    &MakeSpecialResolutionArguments<NameArguments, ParseNameArguments> },
		{ PHANTASMA_LITERAL("phantasma_vm"), PHANTASMA_LITERAL("SetConfig"),
		    &MakeSpecialResolutionArguments<PhantasmaVmConfigArguments, ParsePhantasmaVmConfigArguments> },
		{ PHANTASMA_LITERAL("phantasma_vm"), PHANTASMA_LITERAL("ImportContracts"),
		    &MakeSpecialResolutionArguments<ImportContractsArguments, ParseImportContractsArguments> },
		{ PHANTASMA_LITERAL("phantasma_vm"), PHANTASMA_LITERAL("RepairSeries"),
		    &MakeSpecialResolutionArguments<RepairSeriesArguments, ParseRepairSeriesArguments> },
		{ PHANTASMA_LITERAL("phantasma_vm"), PHANTASMA_LITERAL("RepairToken"),
		    &MakeSpecialResolutionArguments<RepairTokenArguments, ParseRepairTokenArguments> },

		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("TransferFungible"),
		    &MakeSpecialResolutionArguments<TransferFungibleArguments, ParseTransferFungibleArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("TransferNonFungible"),
		    &MakeSpecialResolutionArguments<TransferNonFungibleArguments, ParseTransferNonFungibleArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("CreateToken"),
		    &MakeSpecialResolutionArguments<CreateTokenArguments, ParseCreateTokenArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("MintFungible"),
		    &MakeSpecialResolutionArguments<MintFungibleArguments, ParseMintFungibleArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("BurnFungible"),
		    &MakeSpecialResolutionArguments<BurnFungibleArguments, ParseBurnFungibleArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("GetBalance"),
		    &MakeSpecialResolutionArguments<BalanceArguments, ParseBalanceArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("CreateTokenSeries"),
		    &MakeSpecialResolutionArguments<TokenSeriesArguments, ParseTokenSeriesArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("DeleteTokenSeries"),
		    &MakeSpecialResolutionArguments<TokenSeriesReferenceArguments, ParseTokenSeriesReferenceArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("MintNonFungible"),
		    &MakeSpecialResolutionArguments<MintNonFungibleArguments, ParseMintNonFungibleArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("BurnNonFungible"),
		    &MakeSpecialResolutionArguments<BurnNonFungibleArguments, ParseBurnNonFungibleArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("GetNonFungibleInfo"),
		    &MakeSpecialResolutionArguments<NonFungibleInfoArguments, ParseNonFungibleInfoArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("GetNonFungibleInfoByRomId"),
		    &MakeSpecialResolutionArguments<NonFungibleInfoByRomIdArguments, ParseNonFungibleInfoByRomIdArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("GetSeriesInfo"),
		    &MakeSpecialResolutionArguments<TokenSeriesReferenceArguments, ParseTokenSeriesReferenceArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("GetSeriesInfoByMetaId"),
		    &MakeSpecialResolutionArguments<SeriesInfoByMetaIdArguments, ParseSeriesInfoByMetaIdArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("GetTokenInfo"),
		    &MakeSpecialResolutionArguments<TokenReferenceArguments, ParseTokenReferenceArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("GetTokenInfoBySymbol"),
		    &MakeSpecialResolutionArguments<SymbolArguments, ParseSymbolArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("GetTokenSupply"),
		    &MakeSpecialResolutionArguments<TokenReferenceArguments, ParseTokenReferenceArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("GetSeriesSupply"),
		    &MakeSpecialResolutionArguments<TokenSeriesReferenceArguments, ParseTokenSeriesReferenceArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("GetTokenIdBySymbol"),
		    &MakeSpecialResolutionArguments<SymbolArguments, ParseSymbolArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("GetBalances"),
		    &MakeSpecialResolutionArguments<AddressArguments, ParseAddressArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("CreateMintedTokenSeries"),
		    &MakeSpecialResolutionArguments<CreateMintedTokenSeriesArguments, ParseCreateMintedTokenSeriesArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("ApplyInflation"),
		    &MakeSpecialResolutionArguments<TokenReferenceArguments, ParseTokenReferenceArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("UpdateTokenMetadata"),
		    &MakeSpecialResolutionArguments<UpdateTokenMetadataArguments, ParseUpdateTokenMetadataArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("GetNextTokenInflation"),
		    &MakeSpecialResolutionArguments<TokenReferenceArguments, ParseTokenReferenceArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("SetTokensConfig"),
		    &MakeSpecialResolutionArguments<TokensConfigArguments, ParseTokensConfigArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("UpdateSeriesMetadata"),
		    &MakeSpecialResolutionArguments<UpdateSeriesMetadataArguments, ParseUpdateSeriesMetadataArguments> },
		{ PHANTASMA_LITERAL("token"), PHANTASMA_LITERAL("MintPhantasmaNonFungible"),
		    &MakeSpecialResolutionArguments<MintPhantasmaNonFungibleArguments, ParseMintPhantasmaNonFungibleArguments> },
	};
	out_count = (int)(sizeof(table) / sizeof(table[0]));
	return table;
}

// Keeps the arguments of a call this build cannot type, exactly as the node answered them.
inline SpecialResolutionArgumentsPtr MakeUnrecognizedArguments(const JSONValue& value, bool& jsonErr)
{
	std::shared_ptr<UnrecognizedArguments> output = std::make_shared<UnrecognizedArguments>();
	output->json = json::ToText(value, jsonErr);
	return output;
}

// Types the arguments of one call from its module and method.
//
// Decoding is total: a module/method pair this build does not model, and a modeled pair whose
// payload does not match its shape, both come back as UnrecognizedArguments with the JSON
// preserved, so one odd call can never fail the transaction it belongs to. The C# reference SDK
// drops those instead; this SDK keeps them so that data answered by a node newer than the SDK is
// never silently lost.
inline SpecialResolutionArgumentsPtr ParseSpecialResolutionArguments(
    const String& module, const String& method, const JSONValue& value, bool& jsonErr)
{
	if( !json::IsObject(value, jsonErr) )
		return MakeUnrecognizedArguments(value, jsonErr);

	// The undecoded case is recognised by its content, not by the method name: a method this build
	// knows can still arrive as a raw dump from an older node, and reading that as the typed shape
	// would silently produce an object with every field empty.
	bool shapeErr = false;
	if( json::HasField(value, PHANTASMA_LITERAL("rawArgs"), shapeErr) )
	{
		std::shared_ptr<RawArguments> raw = std::make_shared<RawArguments>();
		raw->rawArgs = ReadArgumentString(value, PHANTASMA_LITERAL("rawArgs"), shapeErr);
		if( !shapeErr )
			return raw;
		return MakeUnrecognizedArguments(value, jsonErr);
	}

	int count = 0;
	const SpecialResolutionArgumentsEntry* table = SpecialResolutionArgumentsTable(count);
	for( int i = 0; i < count; ++i )
	{
		if( module != table[i].module || method != table[i].method )
			continue;
		shapeErr = false;
		SpecialResolutionArgumentsPtr arguments = table[i].parse(value, shapeErr);
		if( !shapeErr && arguments )
			return arguments;
		return MakeUnrecognizedArguments(value, jsonErr);
	}

	return MakeUnrecognizedArguments(value, jsonErr);
}

} // namespace rpc
} // namespace phantasma
