//------------------------------------------------------------------------------
// Phantasma SDK: typed extended-event data carried by extendedEvents of a transaction answer.
//------------------------------------------------------------------------------
// Part of PhantasmaAPI.h; include that file, not this one.
//
// The kind of the carrying event decides the shape of its data; inside a special resolution the
// module and method of each call decide the shape of its arguments. Both dispatches happen while
// parsing, so a consumer reads typed fields instead of walking JSON.
//
// Decoding is total: an event kind this build does not model, and a modeled kind whose payload does
// not match its shape, both come back as ExtendedEventType::Unknown with the JSON preserved in
// unknownData. One unexpected event can therefore never fail a whole block answer, and a kind that
// names a modeled shape while the type is Unknown is how a consumer detects that the answering node
// is newer than this SDK.
//
// Numeric fields follow the wire exactly: chain amounts and big-integer ids arrive as strings
// (JSON numbers lose precision above 2^53), while Carbon-side ids and timestamps are JSON numbers.
//------------------------------------------------------------------------------
#pragma once

#include "SpecialResolutionArguments.h"

namespace phantasma {
namespace rpc {

enum class ExtendedEventType
{
	Unknown,
	TokenCreate,
	TokenSeriesCreate,
	MarketOrder,
	SpecialResolution
};

// Data of a TokenCreate extended event.
struct TokenCreateData {
	String symbol;
	String maxSupply;
	UInt32 decimals = 0;
	bool isNonFungible = false;
	UInt64 carbonTokenId = 0;
	// Metadata values are rendered to strings by the node, so unlike the metadata of a token
	// response they are not VM values here. Keys arrive exactly as the chain stores them.
	PHANTASMA_MAP<String, String> metadata;
};

// Data of a TokenSeriesCreate extended event.
struct TokenSeriesCreateData {
	String symbol;
	// Phantasma series id, a big integer rendered as a string.
	String seriesId;
	UInt32 maxMint = 0;
	UInt32 maxSupply = 0;
	String owner;
	UInt64 carbonTokenId = 0;
	UInt32 carbonSeriesId = 0;
	PHANTASMA_MAP<String, String> metadata;
};

// Data of an OrderCreated, OrderCancelled or OrderFilled extended event. The three kinds share one
// shape; the carrying event's kind tells them apart.
struct MarketOrderData {
	String baseSymbol;
	String quoteSymbol;
	// Phantasma NFT id, a big integer rendered as a string.
	String tokenId;
	UInt64 carbonBaseTokenId = 0;
	UInt64 carbonQuoteTokenId = 0;
	UInt64 carbonInstanceId = 0;
	String seller;
	// The node repeats the seller here on a cancel: that path has no buyer by definition and the
	// payload shape stays stable.
	String buyer;
	String price;
	String endPrice;
	Int64 startDate = 0;
	Int64 endDate = 0;
	// Auction type name, for example "Fixed".
	String type;
};

// One call carried by a special resolution. The arguments are typed per method; calls carries the
// calls of a nested resolution and is empty everywhere else.
struct SpecialResolutionCall {
	UInt32 moduleId = 0;
	String module;
	UInt32 methodId = 0;
	String method;
	SpecialResolutionArgumentsPtr arguments;
	PHANTASMA_VECTOR<SpecialResolutionCall> calls;
};

// Data of a SpecialResolution extended event.
struct SpecialResolutionData {
	UInt64 resolutionId = 0;
	// Empty on most resolutions; the node omits it instead of answering null.
	String description;
	PHANTASMA_VECTOR<SpecialResolutionCall> calls;
};

struct EventExtended {
	String address;
	String contract;
	String kind;
	ExtendedEventType type = ExtendedEventType::Unknown;
	TokenCreateData tokenCreate;
	TokenSeriesCreateData tokenSeriesCreate;
	MarketOrderData marketOrder;
	SpecialResolutionData specialResolution;
	// The payload as answered, for an event kind this build does not model or a modeled kind whose
	// payload did not match. Empty for every typed event.
	String unknownData;
};

// Collects a metadata map whose values the node has already rendered to strings; a named visitor
// for the same reason as VmFieldCollector.
struct EventMetadataCollector {
	PHANTASMA_MAP<String, String>* metadata;
	bool* error;

	void operator()(const String& name, const JSONValue& member) const
	{
		(*metadata)[name] = json::ScalarText(member, *error);
	}
};

// Reads a metadata map whose values the node has already rendered to strings.
inline PHANTASMA_MAP<String, String> ParseEventMetadata(const JSONValue& value, const Char* field, bool& jsonErr)
{
	PHANTASMA_MAP<String, String> output;
	if( !json::HasField(value, field, jsonErr) )
		return output;
	const JSONValue& metadata = json::LookupValue(value, field, jsonErr);
	if( !json::IsObject(metadata, jsonErr) )
	{
		jsonErr = true;
		return output;
	}
	EventMetadataCollector collector{ &output, &jsonErr };
	json::VisitObjectFields(metadata, collector, jsonErr);
	return output;
}

inline TokenCreateData ParseTokenCreateData(const JSONValue& value, bool& jsonErr)
{
	TokenCreateData output;
	output.symbol = json::LookupString(value, PHANTASMA_LITERAL("symbol"), jsonErr);
	output.maxSupply = json::LookupString(value, PHANTASMA_LITERAL("maxSupply"), jsonErr);
	output.decimals = json::LookupUInt32(value, PHANTASMA_LITERAL("decimals"), jsonErr);
	output.isNonFungible = json::LookupBool(value, PHANTASMA_LITERAL("isNonFungible"), jsonErr);
	output.carbonTokenId = json::LookupUInt64(value, PHANTASMA_LITERAL("carbonTokenId"), jsonErr);
	output.metadata = ParseEventMetadata(value, PHANTASMA_LITERAL("metadata"), jsonErr);
	return output;
}

inline TokenSeriesCreateData ParseTokenSeriesCreateData(const JSONValue& value, bool& jsonErr)
{
	TokenSeriesCreateData output;
	output.symbol = json::LookupString(value, PHANTASMA_LITERAL("symbol"), jsonErr);
	output.seriesId = json::LookupString(value, PHANTASMA_LITERAL("seriesId"), jsonErr);
	output.maxMint = json::LookupUInt32(value, PHANTASMA_LITERAL("maxMint"), jsonErr);
	output.maxSupply = json::LookupUInt32(value, PHANTASMA_LITERAL("maxSupply"), jsonErr);
	output.owner = json::LookupString(value, PHANTASMA_LITERAL("owner"), jsonErr);
	output.carbonTokenId = json::LookupUInt64(value, PHANTASMA_LITERAL("carbonTokenId"), jsonErr);
	output.carbonSeriesId = json::LookupUInt32(value, PHANTASMA_LITERAL("carbonSeriesId"), jsonErr);
	output.metadata = ParseEventMetadata(value, PHANTASMA_LITERAL("metadata"), jsonErr);
	return output;
}

inline MarketOrderData ParseMarketOrderData(const JSONValue& value, bool& jsonErr)
{
	MarketOrderData output;
	output.baseSymbol = json::LookupString(value, PHANTASMA_LITERAL("baseSymbol"), jsonErr);
	output.quoteSymbol = json::LookupString(value, PHANTASMA_LITERAL("quoteSymbol"), jsonErr);
	output.tokenId = json::LookupString(value, PHANTASMA_LITERAL("tokenId"), jsonErr);
	output.carbonBaseTokenId = json::LookupUInt64(value, PHANTASMA_LITERAL("carbonBaseTokenId"), jsonErr);
	output.carbonQuoteTokenId = json::LookupUInt64(value, PHANTASMA_LITERAL("carbonQuoteTokenId"), jsonErr);
	output.carbonInstanceId = json::LookupUInt64(value, PHANTASMA_LITERAL("carbonInstanceId"), jsonErr);
	output.seller = json::LookupString(value, PHANTASMA_LITERAL("seller"), jsonErr);
	output.buyer = json::LookupString(value, PHANTASMA_LITERAL("buyer"), jsonErr);
	output.price = json::LookupString(value, PHANTASMA_LITERAL("price"), jsonErr);
	output.endPrice = json::LookupString(value, PHANTASMA_LITERAL("endPrice"), jsonErr);
	output.startDate = json::LookupInt64(value, PHANTASMA_LITERAL("startDate"), jsonErr);
	output.endDate = json::LookupInt64(value, PHANTASMA_LITERAL("endDate"), jsonErr);
	output.type = json::LookupString(value, PHANTASMA_LITERAL("type"), jsonErr);
	return output;
}

inline SpecialResolutionCall ParseSpecialResolutionCall(const JSONValue& value, bool& jsonErr);

// Reads a call list; a field that is absent or is not an array yields no calls.
inline PHANTASMA_VECTOR<SpecialResolutionCall> ParseSpecialResolutionCalls(
    const JSONValue& value, const Char* field, bool& jsonErr)
{
	PHANTASMA_VECTOR<SpecialResolutionCall> output;
	if( !json::HasArrayField(value, field, jsonErr) )
		return output;
	const JSONArray& array = json::LookupArray(value, field, jsonErr);
	const int size = json::ArraySize(array, jsonErr);
	output.reserve(size);
	for( int i = 0; i < size; ++i )
		output.push_back(ParseSpecialResolutionCall(json::IndexArray(array, i, jsonErr), jsonErr));
	return output;
}

inline SpecialResolutionCall ParseSpecialResolutionCall(const JSONValue& value, bool& jsonErr)
{
	SpecialResolutionCall output;
	output.moduleId = json::LookupUInt32(value, PHANTASMA_LITERAL("moduleId"), jsonErr);
	output.module = json::LookupString(value, PHANTASMA_LITERAL("module"), jsonErr);
	output.methodId = json::LookupUInt32(value, PHANTASMA_LITERAL("methodId"), jsonErr);
	output.method = json::LookupString(value, PHANTASMA_LITERAL("method"), jsonErr);
	if( json::HasField(value, PHANTASMA_LITERAL("arguments"), jsonErr) )
	{
		output.arguments = ParseSpecialResolutionArguments(
		    output.module, output.method, json::LookupValue(value, PHANTASMA_LITERAL("arguments"), jsonErr), jsonErr);
	}
	output.calls = ParseSpecialResolutionCalls(value, PHANTASMA_LITERAL("calls"), jsonErr);
	return output;
}

inline SpecialResolutionData ParseSpecialResolutionData(const JSONValue& value, bool& jsonErr)
{
	SpecialResolutionData output;
	output.resolutionId = json::LookupUInt64(value, PHANTASMA_LITERAL("resolutionId"), jsonErr);
	if( json::HasField(value, PHANTASMA_LITERAL("description"), jsonErr) )
		output.description = json::LookupString(value, PHANTASMA_LITERAL("description"), jsonErr);
	output.calls = ParseSpecialResolutionCalls(value, PHANTASMA_LITERAL("calls"), jsonErr);
	return output;
}

inline EventExtended ParseEventExtended(const JSONValue& value, bool& jsonErr)
{
	EventExtended output;
	output.address = json::LookupString(value, PHANTASMA_LITERAL("address"), jsonErr);
	output.contract = json::LookupString(value, PHANTASMA_LITERAL("contract"), jsonErr);
	if( json::HasField(value, PHANTASMA_LITERAL("kind"), jsonErr) )
		output.kind = json::LookupString(value, PHANTASMA_LITERAL("kind"), jsonErr);
	if( !json::HasField(value, PHANTASMA_LITERAL("data"), jsonErr) )
		return output;

	const JSONValue& data = json::LookupValue(value, PHANTASMA_LITERAL("data"), jsonErr);
	// A payload is read against the shape its kind names, with its own error flag: a mismatch must
	// leave the raw JSON to the caller instead of failing the answer that carries it.
	bool shapeErr = false;
	if( output.kind == PHANTASMA_LITERAL("TokenCreate") )
	{
		output.type = ExtendedEventType::TokenCreate;
		output.tokenCreate = ParseTokenCreateData(data, shapeErr);
	}
	else if( output.kind == PHANTASMA_LITERAL("TokenSeriesCreate") )
	{
		output.type = ExtendedEventType::TokenSeriesCreate;
		output.tokenSeriesCreate = ParseTokenSeriesCreateData(data, shapeErr);
	}
	else if( output.kind == PHANTASMA_LITERAL("OrderCreated") || output.kind == PHANTASMA_LITERAL("OrderCancelled") ||
	         output.kind == PHANTASMA_LITERAL("OrderFilled") )
	{
		output.type = ExtendedEventType::MarketOrder;
		output.marketOrder = ParseMarketOrderData(data, shapeErr);
	}
	else if( output.kind == PHANTASMA_LITERAL("SpecialResolution") )
	{
		output.type = ExtendedEventType::SpecialResolution;
		output.specialResolution = ParseSpecialResolutionData(data, shapeErr);
	}
	else
	{
		shapeErr = true;
	}

	if( shapeErr )
	{
		output.type = ExtendedEventType::Unknown;
		output.tokenCreate = TokenCreateData();
		output.tokenSeriesCreate = TokenSeriesCreateData();
		output.marketOrder = MarketOrderData();
		output.specialResolution = SpecialResolutionData();
		output.unknownData = json::ToText(data, jsonErr);
	}
	return output;
}

} // namespace rpc
} // namespace phantasma
