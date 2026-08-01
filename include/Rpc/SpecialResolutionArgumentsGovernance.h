//------------------------------------------------------------------------------
// Phantasma SDK: arguments of the governance module calls a special resolution can carry.
//------------------------------------------------------------------------------
// Part of PhantasmaAPI.h; include that file, not this one.
//
// Field-for-field mirrors of the call payloads. Every numeric field is a string for the reason
// stated on SpecialResolutionArgumentsBase. Fields the reference models declare as optional are
// plain strings here and arrive empty when the node omits them; those fields have no legitimate
// empty value, so nothing is lost by not distinguishing absent from empty.
//------------------------------------------------------------------------------
#pragma once

#include "SpecialResolutionArgumentsBase.h"

namespace phantasma {
namespace rpc {

// Arguments of governance.SetGasConfig.
struct GasConfigArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::GasConfig> {
	String version;
	String maxNameLength;
	String maxTokenSymbolLength;
	String feeShift;
	String maxStructureSize;
	String feeMultiplier;
	String gasTokenId;
	String dataTokenId;
	String minimumGasOffer;
	String dataEscrowPerRow;
	String gasFeeTransfer;
	String gasFeeQuery;
	String gasFeeCreateTokenBase;
	String gasFeeCreateTokenSymbol;
	String gasFeeCreateTokenSeries;
	String gasFeePerByte;
	String gasFeeRegisterName;
	String gasBurnRatioMul;
	String gasBurnRatioShift;
	// Gas-model-v2 tail: answered only when the packaged config declares version >= 1, empty
	// otherwise.
	String minimumGasBill;
	String gasProducerRatioMul;
	String gasProducerRatioShift;
	String gasDappRatioMul;
	String gasDappRatioShift;
	String policyFeeCreateTokenBase;
	String policyFeeCreateTokenSymbol;
	String policyFeeCreateTokenSeries;
	String policyFeeRegisterName;
	String legacyDataEscrowPerRow;
};

inline GasConfigArguments ParseGasConfigArguments(const JSONValue& value, bool& jsonErr)
{
	GasConfigArguments output;
	output.version = ReadArgumentString(value, PHANTASMA_LITERAL("version"), jsonErr);
	output.maxNameLength = ReadArgumentString(value, PHANTASMA_LITERAL("maxNameLength"), jsonErr);
	output.maxTokenSymbolLength = ReadArgumentString(value, PHANTASMA_LITERAL("maxTokenSymbolLength"), jsonErr);
	output.feeShift = ReadArgumentString(value, PHANTASMA_LITERAL("feeShift"), jsonErr);
	output.maxStructureSize = ReadArgumentString(value, PHANTASMA_LITERAL("maxStructureSize"), jsonErr);
	output.feeMultiplier = ReadArgumentString(value, PHANTASMA_LITERAL("feeMultiplier"), jsonErr);
	output.gasTokenId = ReadArgumentString(value, PHANTASMA_LITERAL("gasTokenId"), jsonErr);
	output.dataTokenId = ReadArgumentString(value, PHANTASMA_LITERAL("dataTokenId"), jsonErr);
	output.minimumGasOffer = ReadArgumentString(value, PHANTASMA_LITERAL("minimumGasOffer"), jsonErr);
	output.dataEscrowPerRow = ReadArgumentString(value, PHANTASMA_LITERAL("dataEscrowPerRow"), jsonErr);
	output.gasFeeTransfer = ReadArgumentString(value, PHANTASMA_LITERAL("gasFeeTransfer"), jsonErr);
	output.gasFeeQuery = ReadArgumentString(value, PHANTASMA_LITERAL("gasFeeQuery"), jsonErr);
	output.gasFeeCreateTokenBase = ReadArgumentString(value, PHANTASMA_LITERAL("gasFeeCreateTokenBase"), jsonErr);
	output.gasFeeCreateTokenSymbol = ReadArgumentString(value, PHANTASMA_LITERAL("gasFeeCreateTokenSymbol"), jsonErr);
	output.gasFeeCreateTokenSeries = ReadArgumentString(value, PHANTASMA_LITERAL("gasFeeCreateTokenSeries"), jsonErr);
	output.gasFeePerByte = ReadArgumentString(value, PHANTASMA_LITERAL("gasFeePerByte"), jsonErr);
	output.gasFeeRegisterName = ReadArgumentString(value, PHANTASMA_LITERAL("gasFeeRegisterName"), jsonErr);
	output.gasBurnRatioMul = ReadArgumentString(value, PHANTASMA_LITERAL("gasBurnRatioMul"), jsonErr);
	output.gasBurnRatioShift = ReadArgumentString(value, PHANTASMA_LITERAL("gasBurnRatioShift"), jsonErr);
	output.minimumGasBill = ReadArgumentString(value, PHANTASMA_LITERAL("minimumGasBill"), jsonErr);
	output.gasProducerRatioMul = ReadArgumentString(value, PHANTASMA_LITERAL("gasProducerRatioMul"), jsonErr);
	output.gasProducerRatioShift = ReadArgumentString(value, PHANTASMA_LITERAL("gasProducerRatioShift"), jsonErr);
	output.gasDappRatioMul = ReadArgumentString(value, PHANTASMA_LITERAL("gasDappRatioMul"), jsonErr);
	output.gasDappRatioShift = ReadArgumentString(value, PHANTASMA_LITERAL("gasDappRatioShift"), jsonErr);
	output.policyFeeCreateTokenBase = ReadArgumentString(value, PHANTASMA_LITERAL("policyFeeCreateTokenBase"), jsonErr);
	output.policyFeeCreateTokenSymbol = ReadArgumentString(value, PHANTASMA_LITERAL("policyFeeCreateTokenSymbol"), jsonErr);
	output.policyFeeCreateTokenSeries = ReadArgumentString(value, PHANTASMA_LITERAL("policyFeeCreateTokenSeries"), jsonErr);
	output.policyFeeRegisterName = ReadArgumentString(value, PHANTASMA_LITERAL("policyFeeRegisterName"), jsonErr);
	output.legacyDataEscrowPerRow = ReadArgumentString(value, PHANTASMA_LITERAL("legacyDataEscrowPerRow"), jsonErr);
	return output;
}

// Arguments of governance.SetChainConfig.
struct ChainConfigArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::ChainConfig> {
	String version;
	String reserved1;
	String reserved2;
	String reserved3;
	String allowedTxTypes;
	String expiryWindow;
	String blockRateTarget;
};

inline ChainConfigArguments ParseChainConfigArguments(const JSONValue& value, bool& jsonErr)
{
	ChainConfigArguments output;
	output.version = ReadArgumentString(value, PHANTASMA_LITERAL("version"), jsonErr);
	output.reserved1 = ReadArgumentString(value, PHANTASMA_LITERAL("reserved1"), jsonErr);
	output.reserved2 = ReadArgumentString(value, PHANTASMA_LITERAL("reserved2"), jsonErr);
	output.reserved3 = ReadArgumentString(value, PHANTASMA_LITERAL("reserved3"), jsonErr);
	output.allowedTxTypes = ReadArgumentString(value, PHANTASMA_LITERAL("allowedTxTypes"), jsonErr);
	output.expiryWindow = ReadArgumentString(value, PHANTASMA_LITERAL("expiryWindow"), jsonErr);
	output.blockRateTarget = ReadArgumentString(value, PHANTASMA_LITERAL("blockRateTarget"), jsonErr);
	return output;
}

// Arguments of governance.SpecialResolution: a resolution nested inside another one. Its own calls
// are reported in the carrying call's calls, not here.
struct NestedResolutionArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::NestedResolution> {
	// Answered as a string here, unlike the numeric resolutionId of the resolution envelope.
	String resolutionId;
};

inline NestedResolutionArguments ParseNestedResolutionArguments(const JSONValue& value, bool& jsonErr)
{
	NestedResolutionArguments output;
	output.resolutionId = ReadArgumentString(value, PHANTASMA_LITERAL("resolutionId"), jsonErr);
	return output;
}

// Arguments of governance.SetMetadata.
struct MetadataArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::Metadata> {
	PHANTASMA_VECTOR<VmField> metadata;
};

inline MetadataArguments ParseMetadataArguments(const JSONValue& value, bool& jsonErr)
{
	MetadataArguments output;
	output.metadata = ReadArgumentMetadata(value, PHANTASMA_LITERAL("metadata"), jsonErr);
	return output;
}

// One node of a governance.SetNodeConfig call.
struct ConsensusNode {
	String id;
	String type;
};

inline ConsensusNode ParseConsensusNode(const JSONValue& value, bool& jsonErr)
{
	ConsensusNode output;
	output.id = ReadArgumentString(value, PHANTASMA_LITERAL("id"), jsonErr);
	output.type = ReadArgumentString(value, PHANTASMA_LITERAL("type"), jsonErr);
	return output;
}

// Arguments of governance.SetNodeConfig.
struct NodeConfigArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::NodeConfig> {
	PHANTASMA_VECTOR<ConsensusNode> nodes;
};

inline NodeConfigArguments ParseNodeConfigArguments(const JSONValue& value, bool& jsonErr)
{
	NodeConfigArguments output;
	output.nodes = ReadArgumentObjectArray<ConsensusNode>(value, PHANTASMA_LITERAL("nodes"), jsonErr, ParseConsensusNode);
	return output;
}

// Arguments of governance.RegisterName.
struct RegisterNameArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::RegisterName> {
	String address;
	String name;
};

inline RegisterNameArguments ParseRegisterNameArguments(const JSONValue& value, bool& jsonErr)
{
	RegisterNameArguments output;
	output.address = ReadArgumentString(value, PHANTASMA_LITERAL("address"), jsonErr);
	output.name = ReadArgumentString(value, PHANTASMA_LITERAL("name"), jsonErr);
	return output;
}

// A single address argument, shared by governance.LookupName and token.GetBalances.
struct AddressArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::Address> {
	String address;
};

inline AddressArguments ParseAddressArguments(const JSONValue& value, bool& jsonErr)
{
	AddressArguments output;
	output.address = ReadArgumentString(value, PHANTASMA_LITERAL("address"), jsonErr);
	return output;
}

// A single name argument, shared by governance.LookupAddress and phantasma_vm.IsContractDeployed.
struct NameArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::Name> {
	String name;
};

inline NameArguments ParseNameArguments(const JSONValue& value, bool& jsonErr)
{
	NameArguments output;
	output.name = ReadArgumentString(value, PHANTASMA_LITERAL("name"), jsonErr);
	return output;
}

} // namespace rpc
} // namespace phantasma
