//------------------------------------------------------------------------------
// Phantasma SDK: the shared base of the special-resolution argument shapes.
//------------------------------------------------------------------------------
// Part of PhantasmaAPI.h; include that file, not this one.
//------------------------------------------------------------------------------
#pragma once

#include "VmValue.h"

namespace phantasma {
namespace rpc {

// Which shape a set of special-resolution arguments carries. The concrete shape is chosen by the
// module and method of the call that carries it; Raw and Unrecognized are the two fallbacks.
enum class SpecialResolutionArgumentType
{
	None,
	// The answering node itself could not decode the call: the argument buffer arrives as hex.
	Raw,
	// A module/method pair this build does not model, or a modeled pair whose payload did not
	// match its shape. The JSON is preserved as answered.
	Unrecognized,
	GasConfig,
	ChainConfig,
	NestedResolution,
	Metadata,
	NodeConfig,
	RegisterName,
	Address,
	Name,
	ExecuteScript,
	RegisterTokenContract,
	DeployContract,
	PhantasmaVmConfig,
	ImportContracts,
	RepairSeries,
	RepairToken,
	TokenReference,
	TokenSeriesReference,
	Symbol,
	TransferFungible,
	TransferNonFungible,
	MintFungible,
	BurnFungible,
	Balance,
	CreateToken,
	TokenSeries,
	CreateMintedTokenSeries,
	MintNonFungible,
	MintPhantasmaNonFungible,
	BurnNonFungible,
	NonFungibleInfo,
	NonFungibleInfoByRomId,
	SeriesInfoByMetaId,
	TokensConfig,
	UpdateTokenMetadata,
	UpdateSeriesMetadata
};

// Decoded arguments of one call inside a special resolution.
//
// A call holds this by pointer, so one call costs a pointer plus the shape it really carries -
// which matters because a single repair resolution can carry thousands of calls. Read the concrete
// shape with SpecialResolutionArgumentsAs:
//
//     if( const TransferFungibleArguments* transfer =
//             SpecialResolutionArgumentsAs<TransferFungibleArguments>(call.arguments.get()) )
//         use(transfer->token, transfer->amount);
//
// Every numeric field of every shape is a string: chain values are big integers, and JSON numbers
// lose precision above 2^53.
struct SpecialResolutionArgumentsBase {
	SpecialResolutionArgumentType type = SpecialResolutionArgumentType::None;
	virtual ~SpecialResolutionArgumentsBase() = default;
};

// Gives each shape its tag and the compile-time constant the type-safe cast checks against. The
// second parameter carries the inheritance of the reference models, where several token calls
// extend the token identity pair.
template<SpecialResolutionArgumentType Kind, class Base = SpecialResolutionArgumentsBase>
struct SpecialResolutionArgumentsOf : Base {
	static constexpr SpecialResolutionArgumentType ArgumentType = Kind;
	SpecialResolutionArgumentsOf() { this->type = Kind; }
};

// Returns the arguments as one concrete shape, or null when the call carries a different one.
template<class T>
const T* SpecialResolutionArgumentsAs(const SpecialResolutionArgumentsBase* arguments)
{
	if( !arguments || arguments->type != T::ArgumentType )
		return nullptr;
	return static_cast<const T*>(arguments);
}

struct RawArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::Raw> {
	String rawArgs;
};

struct UnrecognizedArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::Unrecognized> {
	// The arguments exactly as the node answered them, so that data this build cannot type is
	// still available to the caller instead of being dropped.
	String json;
};

// Reads a string field, treating an absent field as empty. A field that is present but is not a
// string sets the error flag, which is what tells the dispatcher that the payload does not match
// the shape it was read against.
inline String ReadArgumentString(const JSONValue& value, const Char* field, bool& jsonErr)
{
	if( !json::HasField(value, field, jsonErr) )
		return String();
	return json::LookupString(value, field, jsonErr);
}

// Reads an array of strings, treating an absent field as an empty array.
inline PHANTASMA_VECTOR<String> ReadArgumentStringArray(const JSONValue& value, const Char* field, bool& jsonErr)
{
	PHANTASMA_VECTOR<String> output;
	if( !json::HasArrayField(value, field, jsonErr) )
		return output;
	const JSONArray& array = json::LookupArray(value, field, jsonErr);
	const int size = json::ArraySize(array, jsonErr);
	output.reserve(size);
	for( int i = 0; i < size; ++i )
		output.push_back(json::AsString(json::IndexArray(array, i, jsonErr), jsonErr));
	return output;
}

// Reads a metadata map whose values are VM values, treating an absent field as empty.
inline PHANTASMA_VECTOR<VmField> ReadArgumentMetadata(const JSONValue& value, const Char* field, bool& jsonErr)
{
	PHANTASMA_VECTOR<VmField> output;
	if( !json::HasField(value, field, jsonErr) )
		return output;
	const JSONValue& metadata = json::LookupValue(value, field, jsonErr);
	if( !json::IsObject(metadata, jsonErr) )
	{
		jsonErr = true;
		return output;
	}
	VmFieldCollector collector{ &output, &jsonErr };
	json::VisitObjectFields(metadata, collector, jsonErr);
	return output;
}

// Reads the elements of an object array with a per-element parser, treating an absent field as an
// empty array.
template<class T, class Parser>
PHANTASMA_VECTOR<T> ReadArgumentObjectArray(const JSONValue& value, const Char* field, bool& jsonErr, Parser parse)
{
	PHANTASMA_VECTOR<T> output;
	if( !json::HasArrayField(value, field, jsonErr) )
		return output;
	const JSONArray& array = json::LookupArray(value, field, jsonErr);
	const int size = json::ArraySize(array, jsonErr);
	output.reserve(size);
	for( int i = 0; i < size; ++i )
		output.push_back(parse(json::IndexArray(array, i, jsonErr), jsonErr));
	return output;
}

} // namespace rpc
} // namespace phantasma
