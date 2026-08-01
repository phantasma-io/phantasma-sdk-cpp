//------------------------------------------------------------------------------
// Phantasma SDK: arguments of the token module calls a special resolution can carry.
//------------------------------------------------------------------------------
// Part of PhantasmaAPI.h; include that file, not this one.
//
// Shapes that repeat across methods share one type on purpose: a query by token id looks the same
// whichever query it is. The token identity pair is inherited rather than repeated, mirroring the
// reference models.
//------------------------------------------------------------------------------
#pragma once

#include "SpecialResolutionArgumentsBase.h"

namespace phantasma {
namespace rpc {

// Token identity: the resolved symbol plus the numeric id it was resolved from. Also the arguments
// of the plain token queries (GetTokenInfo, GetTokenSupply, ApplyInflation, GetNextTokenInflation).
struct TokenReferenceArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::TokenReference> {
	String token;
	String tokenId;
};

// Fills the inherited identity pair; every token shape carries it.
inline void ReadTokenReference(const JSONValue& value, TokenReferenceArguments& output, bool& jsonErr)
{
	output.token = ReadArgumentString(value, PHANTASMA_LITERAL("token"), jsonErr);
	output.tokenId = ReadArgumentString(value, PHANTASMA_LITERAL("tokenId"), jsonErr);
}

inline TokenReferenceArguments ParseTokenReferenceArguments(const JSONValue& value, bool& jsonErr)
{
	TokenReferenceArguments output;
	ReadTokenReference(value, output, jsonErr);
	return output;
}

// Addresses one series of a token: DeleteTokenSeries, GetSeriesInfo, GetSeriesSupply.
struct TokenSeriesReferenceArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::TokenSeriesReference, TokenReferenceArguments> {
	String seriesId;
};

inline TokenSeriesReferenceArguments ParseTokenSeriesReferenceArguments(const JSONValue& value, bool& jsonErr)
{
	TokenSeriesReferenceArguments output;
	ReadTokenReference(value, output, jsonErr);
	output.seriesId = ReadArgumentString(value, PHANTASMA_LITERAL("seriesId"), jsonErr);
	return output;
}

// A single symbol argument: GetTokenInfoBySymbol and GetTokenIdBySymbol.
struct SymbolArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::Symbol> {
	String symbol;
};

inline SymbolArguments ParseSymbolArguments(const JSONValue& value, bool& jsonErr)
{
	SymbolArguments output;
	output.symbol = ReadArgumentString(value, PHANTASMA_LITERAL("symbol"), jsonErr);
	return output;
}

// Arguments of token.TransferFungible.
struct TransferFungibleArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::TransferFungible, TokenReferenceArguments> {
	String from;
	String to;
	String amount;
};

inline TransferFungibleArguments ParseTransferFungibleArguments(const JSONValue& value, bool& jsonErr)
{
	TransferFungibleArguments output;
	ReadTokenReference(value, output, jsonErr);
	output.from = ReadArgumentString(value, PHANTASMA_LITERAL("from"), jsonErr);
	output.to = ReadArgumentString(value, PHANTASMA_LITERAL("to"), jsonErr);
	output.amount = ReadArgumentString(value, PHANTASMA_LITERAL("amount"), jsonErr);
	return output;
}

// Arguments of token.TransferNonFungible.
struct TransferNonFungibleArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::TransferNonFungible, TokenReferenceArguments> {
	String from;
	String to;
	PHANTASMA_VECTOR<String> instanceIds;
};

inline TransferNonFungibleArguments ParseTransferNonFungibleArguments(const JSONValue& value, bool& jsonErr)
{
	TransferNonFungibleArguments output;
	ReadTokenReference(value, output, jsonErr);
	output.from = ReadArgumentString(value, PHANTASMA_LITERAL("from"), jsonErr);
	output.to = ReadArgumentString(value, PHANTASMA_LITERAL("to"), jsonErr);
	output.instanceIds = ReadArgumentStringArray(value, PHANTASMA_LITERAL("instanceIds"), jsonErr);
	return output;
}

// Arguments of token.MintFungible.
struct MintFungibleArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::MintFungible, TokenReferenceArguments> {
	String to;
	String amount;
};

inline MintFungibleArguments ParseMintFungibleArguments(const JSONValue& value, bool& jsonErr)
{
	MintFungibleArguments output;
	ReadTokenReference(value, output, jsonErr);
	output.to = ReadArgumentString(value, PHANTASMA_LITERAL("to"), jsonErr);
	output.amount = ReadArgumentString(value, PHANTASMA_LITERAL("amount"), jsonErr);
	return output;
}

// Arguments of token.BurnFungible.
struct BurnFungibleArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::BurnFungible, TokenReferenceArguments> {
	String from;
	String amount;
};

inline BurnFungibleArguments ParseBurnFungibleArguments(const JSONValue& value, bool& jsonErr)
{
	BurnFungibleArguments output;
	ReadTokenReference(value, output, jsonErr);
	output.from = ReadArgumentString(value, PHANTASMA_LITERAL("from"), jsonErr);
	output.amount = ReadArgumentString(value, PHANTASMA_LITERAL("amount"), jsonErr);
	return output;
}

// Arguments of token.GetBalance.
struct BalanceArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::Balance, TokenReferenceArguments> {
	String address;
};

inline BalanceArguments ParseBalanceArguments(const JSONValue& value, bool& jsonErr)
{
	BalanceArguments output;
	ReadTokenReference(value, output, jsonErr);
	output.address = ReadArgumentString(value, PHANTASMA_LITERAL("address"), jsonErr);
	return output;
}

// Arguments of token.CreateToken.
struct CreateTokenArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::CreateToken> {
	String symbol;
	String owner;
	String maxSupply;
	String decimals;
	String flags;
	// Decoded metadata fields; empty when the token carries none.
	PHANTASMA_VECTOR<VmField> metadata;
	// NFT schema blob as hex; empty for fungible tokens.
	String tokenSchemas;
};

inline CreateTokenArguments ParseCreateTokenArguments(const JSONValue& value, bool& jsonErr)
{
	CreateTokenArguments output;
	output.symbol = ReadArgumentString(value, PHANTASMA_LITERAL("symbol"), jsonErr);
	output.owner = ReadArgumentString(value, PHANTASMA_LITERAL("owner"), jsonErr);
	output.maxSupply = ReadArgumentString(value, PHANTASMA_LITERAL("maxSupply"), jsonErr);
	output.decimals = ReadArgumentString(value, PHANTASMA_LITERAL("decimals"), jsonErr);
	output.flags = ReadArgumentString(value, PHANTASMA_LITERAL("flags"), jsonErr);
	output.metadata = ReadArgumentMetadata(value, PHANTASMA_LITERAL("metadata"), jsonErr);
	output.tokenSchemas = ReadArgumentString(value, PHANTASMA_LITERAL("tokenSchemas"), jsonErr);
	return output;
}

// A series definition, as carried by token.CreateTokenSeries.
struct TokenSeriesArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::TokenSeries, TokenReferenceArguments> {
	String owner;
	String maxMint;
	String maxSupply;
	// Decoded series metadata; empty when the token declares no schema for it.
	PHANTASMA_VECTOR<VmField> metadata;
	// Phantasma series id taken from the decoded metadata, when the schema carries one.
	String seriesId;
	// Metadata blob as hex, answered instead of metadata when it cannot be decoded.
	String metadataRaw;
};

// Fills the series definition; CreateMintedTokenSeries carries the same fields plus its own.
inline void ReadTokenSeries(const JSONValue& value, TokenSeriesArguments& output, bool& jsonErr)
{
	ReadTokenReference(value, output, jsonErr);
	output.owner = ReadArgumentString(value, PHANTASMA_LITERAL("owner"), jsonErr);
	output.maxMint = ReadArgumentString(value, PHANTASMA_LITERAL("maxMint"), jsonErr);
	output.maxSupply = ReadArgumentString(value, PHANTASMA_LITERAL("maxSupply"), jsonErr);
	output.metadata = ReadArgumentMetadata(value, PHANTASMA_LITERAL("metadata"), jsonErr);
	output.seriesId = ReadArgumentString(value, PHANTASMA_LITERAL("seriesId"), jsonErr);
	output.metadataRaw = ReadArgumentString(value, PHANTASMA_LITERAL("metadataRaw"), jsonErr);
}

inline TokenSeriesArguments ParseTokenSeriesArguments(const JSONValue& value, bool& jsonErr)
{
	TokenSeriesArguments output;
	ReadTokenSeries(value, output, jsonErr);
	return output;
}

// Arguments of token.CreateMintedTokenSeries.
struct CreateMintedTokenSeriesArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::CreateMintedTokenSeries, TokenSeriesArguments> {
	String recipient;
	PHANTASMA_VECTOR<String> roms;
	PHANTASMA_VECTOR<String> rams;
};

inline CreateMintedTokenSeriesArguments ParseCreateMintedTokenSeriesArguments(const JSONValue& value, bool& jsonErr)
{
	CreateMintedTokenSeriesArguments output;
	ReadTokenSeries(value, output, jsonErr);
	output.recipient = ReadArgumentString(value, PHANTASMA_LITERAL("recipient"), jsonErr);
	output.roms = ReadArgumentStringArray(value, PHANTASMA_LITERAL("roms"), jsonErr);
	output.rams = ReadArgumentStringArray(value, PHANTASMA_LITERAL("rams"), jsonErr);
	return output;
}

// One NFT to mint, addressed by the carbon series id.
struct NftMint {
	String seriesId;
	String rom;
	String ram;
};

inline NftMint ParseNftMint(const JSONValue& value, bool& jsonErr)
{
	NftMint output;
	output.seriesId = ReadArgumentString(value, PHANTASMA_LITERAL("seriesId"), jsonErr);
	output.rom = ReadArgumentString(value, PHANTASMA_LITERAL("rom"), jsonErr);
	output.ram = ReadArgumentString(value, PHANTASMA_LITERAL("ram"), jsonErr);
	return output;
}

// One NFT to mint, addressed by the 32-byte Phantasma series id.
struct PhantasmaNftMint {
	String phantasmaSeriesId;
	String rom;
	String ram;
};

inline PhantasmaNftMint ParsePhantasmaNftMint(const JSONValue& value, bool& jsonErr)
{
	PhantasmaNftMint output;
	output.phantasmaSeriesId = ReadArgumentString(value, PHANTASMA_LITERAL("phantasmaSeriesId"), jsonErr);
	output.rom = ReadArgumentString(value, PHANTASMA_LITERAL("rom"), jsonErr);
	output.ram = ReadArgumentString(value, PHANTASMA_LITERAL("ram"), jsonErr);
	return output;
}

// Arguments of token.MintNonFungible.
struct MintNonFungibleArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::MintNonFungible, TokenReferenceArguments> {
	String owner;
	PHANTASMA_VECTOR<NftMint> tokens;
};

inline MintNonFungibleArguments ParseMintNonFungibleArguments(const JSONValue& value, bool& jsonErr)
{
	MintNonFungibleArguments output;
	ReadTokenReference(value, output, jsonErr);
	output.owner = ReadArgumentString(value, PHANTASMA_LITERAL("owner"), jsonErr);
	output.tokens = ReadArgumentObjectArray<NftMint>(value, PHANTASMA_LITERAL("tokens"), jsonErr, ParseNftMint);
	return output;
}

// Arguments of token.MintPhantasmaNonFungible.
struct MintPhantasmaNonFungibleArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::MintPhantasmaNonFungible, TokenReferenceArguments> {
	String owner;
	PHANTASMA_VECTOR<PhantasmaNftMint> tokens;
};

inline MintPhantasmaNonFungibleArguments ParseMintPhantasmaNonFungibleArguments(const JSONValue& value, bool& jsonErr)
{
	MintPhantasmaNonFungibleArguments output;
	ReadTokenReference(value, output, jsonErr);
	output.owner = ReadArgumentString(value, PHANTASMA_LITERAL("owner"), jsonErr);
	output.tokens =
	    ReadArgumentObjectArray<PhantasmaNftMint>(value, PHANTASMA_LITERAL("tokens"), jsonErr, ParsePhantasmaNftMint);
	return output;
}

// Arguments of token.BurnNonFungible.
struct BurnNonFungibleArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::BurnNonFungible, TokenReferenceArguments> {
	String address;
	PHANTASMA_VECTOR<String> instanceIds;
};

inline BurnNonFungibleArguments ParseBurnNonFungibleArguments(const JSONValue& value, bool& jsonErr)
{
	BurnNonFungibleArguments output;
	ReadTokenReference(value, output, jsonErr);
	output.address = ReadArgumentString(value, PHANTASMA_LITERAL("address"), jsonErr);
	output.instanceIds = ReadArgumentStringArray(value, PHANTASMA_LITERAL("instanceIds"), jsonErr);
	return output;
}

// Arguments of token.GetNonFungibleInfo.
struct NonFungibleInfoArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::NonFungibleInfo, TokenReferenceArguments> {
	String instanceId;
	String getSchemas;
};

inline NonFungibleInfoArguments ParseNonFungibleInfoArguments(const JSONValue& value, bool& jsonErr)
{
	NonFungibleInfoArguments output;
	ReadTokenReference(value, output, jsonErr);
	output.instanceId = ReadArgumentString(value, PHANTASMA_LITERAL("instanceId"), jsonErr);
	output.getSchemas = ReadArgumentString(value, PHANTASMA_LITERAL("getSchemas"), jsonErr);
	return output;
}

// Arguments of token.GetNonFungibleInfoByRomId.
struct NonFungibleInfoByRomIdArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::NonFungibleInfoByRomId, TokenReferenceArguments> {
	String romId;
	String getSchemas;
};

inline NonFungibleInfoByRomIdArguments ParseNonFungibleInfoByRomIdArguments(const JSONValue& value, bool& jsonErr)
{
	NonFungibleInfoByRomIdArguments output;
	ReadTokenReference(value, output, jsonErr);
	output.romId = ReadArgumentString(value, PHANTASMA_LITERAL("romId"), jsonErr);
	output.getSchemas = ReadArgumentString(value, PHANTASMA_LITERAL("getSchemas"), jsonErr);
	return output;
}

// Arguments of token.GetSeriesInfoByMetaId.
struct SeriesInfoByMetaIdArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::SeriesInfoByMetaId, TokenReferenceArguments> {
	String romId;
};

inline SeriesInfoByMetaIdArguments ParseSeriesInfoByMetaIdArguments(const JSONValue& value, bool& jsonErr)
{
	SeriesInfoByMetaIdArguments output;
	ReadTokenReference(value, output, jsonErr);
	output.romId = ReadArgumentString(value, PHANTASMA_LITERAL("romId"), jsonErr);
	return output;
}

// Arguments of token.SetTokensConfig.
struct TokensConfigArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::TokensConfig> {
	String flags;
	// Names of the flags that are set, including a Reserved0xNN entry for unknown bits.
	PHANTASMA_VECTOR<String> flagsNames;
};

inline TokensConfigArguments ParseTokensConfigArguments(const JSONValue& value, bool& jsonErr)
{
	TokensConfigArguments output;
	output.flags = ReadArgumentString(value, PHANTASMA_LITERAL("flags"), jsonErr);
	output.flagsNames = ReadArgumentStringArray(value, PHANTASMA_LITERAL("flagsNames"), jsonErr);
	return output;
}

// Arguments of token.UpdateTokenMetadata.
struct UpdateTokenMetadataArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::UpdateTokenMetadata, TokenReferenceArguments> {
	PHANTASMA_VECTOR<VmField> metadata;
};

inline UpdateTokenMetadataArguments ParseUpdateTokenMetadataArguments(const JSONValue& value, bool& jsonErr)
{
	UpdateTokenMetadataArguments output;
	ReadTokenReference(value, output, jsonErr);
	output.metadata = ReadArgumentMetadata(value, PHANTASMA_LITERAL("metadata"), jsonErr);
	return output;
}

// Arguments of token.UpdateSeriesMetadata.
struct UpdateSeriesMetadataArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::UpdateSeriesMetadata, TokenReferenceArguments> {
	String seriesId;
	// Metadata blob as hex: this call carries it unschematized.
	String metadata;
};

inline UpdateSeriesMetadataArguments ParseUpdateSeriesMetadataArguments(const JSONValue& value, bool& jsonErr)
{
	UpdateSeriesMetadataArguments output;
	ReadTokenReference(value, output, jsonErr);
	output.seriesId = ReadArgumentString(value, PHANTASMA_LITERAL("seriesId"), jsonErr);
	output.metadata = ReadArgumentString(value, PHANTASMA_LITERAL("metadata"), jsonErr);
	return output;
}

} // namespace rpc
} // namespace phantasma
