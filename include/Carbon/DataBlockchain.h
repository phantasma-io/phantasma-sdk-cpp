#pragma once
#ifndef PHANTASMA_API_INCLUDED
#error "Configure and include PhantasmaAPI.h first"
#endif

#include <cstdint>
#include <limits>
#include <vector>

#include "../Cryptography/EdDSA/Ed25519Signature.h"
#include "../Cryptography/KeyPair.h"
#include "Carbon.h"
#include "DataCommon.h"
#include "DataVm.h"

namespace phantasma::carbon::Blockchain {

enum class TxRejection
{
	Valid = 0,
	DataFormat = 1,
	GasFees = 2,
	DataFees = 3,
	Witnesses = 4,
	Expired = 5,
	Contract = 6,
	Payload = 7,
};

struct ChainConfig {
	uint8_t version = 0;
	uint8_t reserved1 = 0;
	uint8_t reserved2 = 0;
	uint8_t reserved3 = 0;
	uint32_t allowedTxTypes = 0;
	uint32_t expiryWindow = 0;
	uint32_t blockRateTarget = 0;
};

// On-chain gas configuration (governance module). The gas-model-v2 extension fields serialize
// only for version >= 1, mirroring the node's data_blockchain.h wire format exactly: the
// version-0 byte image is frozen forever for historical replay, and a version>=1 image
// truncated to the v0 length fails to parse (Read returns false).
struct GasConfig {
	uint8_t version = 0;
	uint8_t maxNameLength = 0;
	uint8_t maxTokenSymbolLength = 0;
	uint8_t feeShift = 0;
	uint32_t maxStructureSize = 0;
	uint64_t feeMultiplier = 0;
	uint64_t gasTokenId = 0;
	uint64_t dataTokenId = 0;
	uint64_t minimumGasOffer = 0;
	uint64_t dataEscrowPerRow = 0;
	uint64_t gasFeeTransfer = 0;
	uint64_t gasFeeQuery = 0;
	uint64_t gasFeeCreateTokenBase = 0;
	uint64_t gasFeeCreateTokenSymbol = 0;
	uint64_t gasFeeCreateTokenSeries = 0;
	uint64_t gasFeePerByte = 0;
	uint64_t gasFeeRegisterName = 0;
	uint64_t gasBurnRatioMul = 0;
	uint8_t gasBurnRatioShift = 0;

	// Gas-model-v2 extension (version >= 1 only).

	// Floor applied to every settled gas bill (kcal-base). 0 = no floor (v1-equivalent).
	uint64_t minimumGasBill = 0;
	// Fee split: bill portions credited to the block producer and to the tx gasTarget dapp
	// address. Same mul/shift fixed-point form as the burn ratio; 0/0 = v1-equivalent.
	uint64_t gasProducerRatioMul = 0;
	uint8_t gasProducerRatioShift = 0;
	uint64_t gasDappRatioMul = 0;
	uint8_t gasDappRatioShift = 0;
	// Product-decision prices ("policy fees") in kcal-base, charged directly with no fee
	// multiplier stage. Under v2 they replace the unit-priced gasFeeCreateToken* and
	// gasFeeRegisterName fields, which stay serialized for version-0 replay.
	uint64_t policyFeeCreateTokenBase = 0;
	// Halved per symbol char after the first (v1 rule kept).
	uint64_t policyFeeCreateTokenSymbol = 0;
	uint64_t policyFeeCreateTokenSeries = 0;
	// Shifted right by (nameLength-1) like the v1 field (v1 rule kept).
	uint64_t policyFeeRegisterName = 0;
	// The frozen pre-flip dataEscrowPerRow: storage rows existing before the v2 flip refund at
	// this price (exactly what they escrowed under v1). Immutable after the flip.
	uint64_t legacyDataEscrowPerRow = 0;

	// True when this config activates the gas-model-v2 billing rules (config version >= 1).
	// The gas model is gated by the config version, not by a chain feature level.
	bool HasGasModelV2() const { return version >= 1; }
};

struct MsgCallArgs {
	int32_t registerOffset = 0;
	ByteView args{};
};
struct MsgCallArgSections {
	int32_t numArgSections_negative = 0;
	MsgCallArgs* argSections = nullptr;
};

struct TxMsgCall {
	uint32_t moduleId = 0;
	uint32_t methodId = 0;
	ByteView args{};
	MsgCallArgSections sections{};
};

struct TxMsgCall_Multi {
	uint32_t numCalls = 0;
	TxMsgCall* calls = nullptr;
};

struct TxMsgSpecialResolution {
	uint64_t resolutionId = 0;
	TxMsgCall_Multi calls{};
};

struct TxMsgTransferFungible {
	Bytes32 to;
	uint64_t tokenId = 0;
	uint64_t amount = 0;
};
struct TxMsgTransferFungible_GasPayer {
	Bytes32 to;
	Bytes32 from;
	uint64_t tokenId = 0;
	uint64_t amount = 0;
};

struct TxMsgTransferNonFungible_Single {
	Bytes32 to;
	uint64_t tokenId = 0;
	uint64_t instanceId = 0;
};
struct TxMsgTransferNonFungible_Single_GasPayer {
	Bytes32 to;
	Bytes32 from;
	uint64_t tokenId = 0;
	uint64_t instanceId = 0;
};

struct TxMsgTransferNonFungible_Multi {
	Bytes32 to;
	uint64_t tokenId = 0;
	uint32_t numInstanceIds = 0;
	const uint64_t* instanceIds = nullptr;
};
struct TxMsgTransferNonFungible_Multi_GasPayer {
	Bytes32 to;
	Bytes32 from;
	uint64_t tokenId = 0;
	uint32_t numInstanceIds = 0;
	const uint64_t* instanceIds = nullptr;
};

struct TxMsgMintFungible {
	uint64_t tokenId = 0;
	intx_pod amount{};
	Bytes32 to;
};
struct TxMsgBurnFungible {
	uint64_t tokenId = 0;
	intx_pod amount{};
};
struct TxMsgBurnFungible_GasPayer {
	uint64_t tokenId = 0;
	intx_pod amount{};
	Bytes32 from;
};
struct TxMsgMintNonFungible {
	uint64_t tokenId = 0;
	Bytes32 to;
	uint32_t seriesId = 0;
	ByteView rom{};
	ByteView ram{};
};
struct TxMsgBurnNonFungible {
	uint64_t tokenId = 0;
	uint64_t instanceId = 0;
};
struct TxMsgBurnNonFungible_GasPayer {
	uint64_t tokenId = 0;
	Bytes32 from;
	uint64_t instanceId = 0;
};

struct TxMsgTrade {
	uint32_t numTransferF = 0;
	TxMsgTransferFungible_GasPayer* transferF = nullptr;
	uint32_t numTransferN = 0;
	TxMsgTransferNonFungible_Single_GasPayer* transferN = nullptr;
	uint32_t numMintF = 0;
	TxMsgMintFungible* mintF = nullptr;
	uint32_t numBurnF = 0;
	TxMsgBurnFungible_GasPayer* burnF = nullptr;
	uint32_t numMintN = 0;
	TxMsgMintNonFungible* mintN = nullptr;
	uint32_t numBurnN = 0;
	TxMsgBurnNonFungible_GasPayer* burnN = nullptr;
};

struct TxMsgPhantasma {
	SmallString nexus;
	SmallString chain;
	ByteView script{};
};

struct TxMsgPhantasma_Raw {
	ByteView transaction{};
};

enum class TxTypes : uint8_t
{
	Call = 0,
	Call_Multi = 1,
	Trade = 2,
	TransferFungible = 3,
	TransferFungible_GasPayer = 4,
	TransferNonFungible_Single = 5,
	TransferNonFungible_Single_GasPayer = 6,
	TransferNonFungible_Multi = 7,
	TransferNonFungible_Multi_GasPayer = 8,
	MintFungible = 9,
	BurnFungible = 10,
	BurnFungible_GasPayer = 11,
	MintNonFungible = 12,
	BurnNonFungible = 13,
	BurnNonFungible_GasPayer = 14,
	Phantasma = 15,
	Phantasma_Raw = 16,
};

enum class TxState : uint8_t
{
	Unknown = 0xFF,
	Rejected = 0,
	Completed = 1,
	Aborted = 2,
	Pending = 3,
};

struct TxMsg {
	constexpr static uint64_t NoMaxGas = (uint64_t)-1;
	constexpr static uint64_t NoMaxData = (uint64_t)-1;

	TxTypes type = TxTypes::Call;
	int64_t expiry = 0;
	uint64_t maxGas = 0;
	uint64_t maxData = 0;
	Bytes32 gasFrom{};
	SmallString payload{};
	union
	{
		TxMsgCall call;
		TxMsgCall_Multi callMulti;
		TxMsgTransferFungible transferFt;
		TxMsgTransferFungible_GasPayer transferFtGasPayer;
		TxMsgTransferNonFungible_Single transferNftSingle;
		TxMsgTransferNonFungible_Single_GasPayer transferNftSingleGasPayer;
		TxMsgTransferNonFungible_Multi transferNftMulti;
		TxMsgTransferNonFungible_Multi_GasPayer transferNftMultiGasPayer;
		TxMsgMintFungible mintFungible;
		TxMsgBurnFungible burnFungible;
		TxMsgBurnFungible_GasPayer burnFungibleGasPayer;
		TxMsgMintNonFungible mintNonFungible;
		TxMsgBurnNonFungible burnNonFungible;
		TxMsgBurnNonFungible_GasPayer burnNonFungibleGasPayer;
		TxMsgTrade trade;
		TxMsgPhantasma phantasma;
		TxMsgPhantasma_Raw phantasmaRaw;
	};

	TxMsg()
	    : type(TxTypes::Call), expiry(0), maxGas(0), maxData(0), gasFrom(), payload(), call{}
	{
	}
};

inline void Write(const Witness& in, WriteView& w)
{
	Write(in.address, w);
	Write(in.signature, w);
}

inline void Write(const Witnesses& in, WriteView& w)
{
	Write4((int32_t)in.numWitnesses, w);
	for( uint32_t i = 0; i != in.numWitnesses; ++i )
	{
		Write(in.witnesses[i], w);
	}
}

// Primitive serialization -----------------------------------------------------
inline void Write(const ChainConfig& in, WriteView& w)
{
	Write1(in.version, w);
	Write1(in.reserved1, w);
	Write1(in.reserved2, w);
	Write1(in.reserved3, w);
	Write4((int32_t)in.allowedTxTypes, w);
	Write4((int32_t)in.expiryWindow, w);
	Write4((int32_t)in.blockRateTarget, w);
}
inline void Write(const GasConfig& in, WriteView& w)
{
	Write1(in.version, w);
	Write1(in.maxNameLength, w);
	Write1(in.maxTokenSymbolLength, w);
	Write1(in.feeShift, w);
	Write4((int32_t)in.maxStructureSize, w);
	Write8u(in.feeMultiplier, w);
	Write8u(in.gasTokenId, w);
	Write8u(in.dataTokenId, w);
	Write8u(in.minimumGasOffer, w);
	Write8u(in.dataEscrowPerRow, w);
	Write8u(in.gasFeeTransfer, w);
	Write8u(in.gasFeeQuery, w);
	Write8u(in.gasFeeCreateTokenBase, w);
	Write8u(in.gasFeeCreateTokenSymbol, w);
	Write8u(in.gasFeeCreateTokenSeries, w);
	Write8u(in.gasFeePerByte, w);
	Write8u(in.gasFeeRegisterName, w);
	Write8u(in.gasBurnRatioMul, w);
	Write1(in.gasBurnRatioShift, w);
	if( in.version == 0 )
	{
		// Version-0 wire image must stay byte-identical to the pre-v2 layout.
		return;
	}
	Write8u(in.minimumGasBill, w);
	Write8u(in.gasProducerRatioMul, w);
	Write1(in.gasProducerRatioShift, w);
	Write8u(in.gasDappRatioMul, w);
	Write1(in.gasDappRatioShift, w);
	Write8u(in.policyFeeCreateTokenBase, w);
	Write8u(in.policyFeeCreateTokenSymbol, w);
	Write8u(in.policyFeeCreateTokenSeries, w);
	Write8u(in.policyFeeRegisterName, w);
	Write8u(in.legacyDataEscrowPerRow, w);
}
// Returns false on a truncated image (fallible-read convention, exceptions are optional in
// this SDK).
inline bool Read(GasConfig& out, ReadView& r)
{
	if( !(r.ReadBytes(out.version) &&
	        r.ReadBytes(out.maxNameLength) &&
	        r.ReadBytes(out.maxTokenSymbolLength) &&
	        r.ReadBytes(out.feeShift) &&
	        r.ReadBytes(out.maxStructureSize) &&
	        r.ReadBytes(out.feeMultiplier) &&
	        r.ReadBytes(out.gasTokenId) &&
	        r.ReadBytes(out.dataTokenId) &&
	        r.ReadBytes(out.minimumGasOffer) &&
	        r.ReadBytes(out.dataEscrowPerRow) &&
	        r.ReadBytes(out.gasFeeTransfer) &&
	        r.ReadBytes(out.gasFeeQuery) &&
	        r.ReadBytes(out.gasFeeCreateTokenBase) &&
	        r.ReadBytes(out.gasFeeCreateTokenSymbol) &&
	        r.ReadBytes(out.gasFeeCreateTokenSeries) &&
	        r.ReadBytes(out.gasFeePerByte) &&
	        r.ReadBytes(out.gasFeeRegisterName) &&
	        r.ReadBytes(out.gasBurnRatioMul) &&
	        r.ReadBytes(out.gasBurnRatioShift)) )
		return false;
	if( out.version == 0 )
	{
		// Version-0 rows carry no v2 tail; zero it so a reused instance never leaks stale values.
		out.minimumGasBill = 0;
		out.gasProducerRatioMul = 0;
		out.gasProducerRatioShift = 0;
		out.gasDappRatioMul = 0;
		out.gasDappRatioShift = 0;
		out.policyFeeCreateTokenBase = 0;
		out.policyFeeCreateTokenSymbol = 0;
		out.policyFeeCreateTokenSeries = 0;
		out.policyFeeRegisterName = 0;
		out.legacyDataEscrowPerRow = 0;
		return true;
	}
	// version >= 1: the tail is mandatory; a truncated image must FAIL to parse, never
	// silently produce a config with zeroed v2 prices.
	return r.ReadBytes(out.minimumGasBill) &&
	       r.ReadBytes(out.gasProducerRatioMul) &&
	       r.ReadBytes(out.gasProducerRatioShift) &&
	       r.ReadBytes(out.gasDappRatioMul) &&
	       r.ReadBytes(out.gasDappRatioShift) &&
	       r.ReadBytes(out.policyFeeCreateTokenBase) &&
	       r.ReadBytes(out.policyFeeCreateTokenSymbol) &&
	       r.ReadBytes(out.policyFeeCreateTokenSeries) &&
	       r.ReadBytes(out.policyFeeRegisterName) &&
	       r.ReadBytes(out.legacyDataEscrowPerRow);
}

// Tx message serialization ---------------------------------------------------
inline bool Read(MsgCallArgs& out, ReadView& reader, Allocator& alloc)
{
	const Byte* mark = (const Byte*)reader.Mark();
	int32_t value = 0;
	if( !Read(value, reader) )
	{
		return false;
	}
	if( value >= 0 )
	{
		reader.Rewind(mark);
		out.registerOffset = 0;
		return ReadArray(out.args, reader, alloc);
	}
	out.args = {};
	out.registerOffset = value;
	return true;
}

inline bool Read(MsgCallArgSections& out, ReadView& reader, Allocator& alloc)
{
	int32_t count = 0;
	if( !Read(count, reader) )
	{
		return false;
	}
	if( count >= 0 || count == std::numeric_limits<int32_t>::min() )
	{
		return false;
	}
	out.numArgSections_negative = count;
	const uint32_t length = (uint32_t)(-(int64_t)count);
	if( length == 0 )
	{
		out.argSections = nullptr;
		return true;
	}
	if( length > reader.length / 4 )
	{
		return false;
	}
	out.argSections = alloc.Alloc<MsgCallArgs>(length);
	for( uint32_t i = 0; i != length; ++i )
	{
		if( !Read(out.argSections[i], reader, alloc) )
		{
			return false;
		}
	}
	return true;
}

inline bool Read(TxMsgCall& out, ReadView& reader, Allocator& alloc)
{
	out.sections = {};
	out.args = {};
	out.sections.argSections = nullptr;
	out.sections.numArgSections_negative = 0;

	if( !(Read(out.moduleId, reader) && Read(out.methodId, reader)) )
	{
		return false;
	}
	const Byte* mark = (const Byte*)reader.Mark();
	int32_t len = 0;
	if( !Read(len, reader) )
	{
		return false;
	}
	reader.Rewind(mark);
	if( len >= 0 )
	{
		return ReadArray(out.args, reader, alloc);
	}
	return Read(out.sections, reader, alloc);
}

inline bool Read(TxMsgCall_Multi& out, ReadView& reader, Allocator& alloc)
{
	return ReadArray(out.numCalls, out.calls, reader, alloc, [&](TxMsgCall& call, ReadView& r)
	    { return Read(call, r, alloc); }, 12);
}

inline bool Read(TxMsgSpecialResolution& out, ReadView& reader, Allocator& alloc)
{
	return Read(out.resolutionId, reader) && Read(out.calls, reader, alloc);
}

// The payload readers below mirror the Write of the same type, field for field, and answer false on
// a truncated or malformed image. They use the fallible Read overloads throughout: Read1, Read4 and
// Read8 report the end of the stream through Throw::If, which does nothing when the SDK is built
// without exceptions, and a reader that cannot fail would hand the caller invented fields.
inline bool Read(TxMsgTransferFungible& out, ReadView& r)
{
	return Read(out.to, r) && Read(out.tokenId, r) && Read(out.amount, r);
}

inline bool Read(TxMsgTransferFungible_GasPayer& out, ReadView& r)
{
	return Read(out.to, r) && Read(out.from, r) && Read(out.tokenId, r) && Read(out.amount, r);
}

inline bool Read(TxMsgTransferNonFungible_Single& out, ReadView& r)
{
	return Read(out.to, r) && Read(out.tokenId, r) && Read(out.instanceId, r);
}

inline bool Read(TxMsgTransferNonFungible_Single_GasPayer& out, ReadView& r)
{
	return Read(out.to, r) && Read(out.from, r) && Read(out.tokenId, r) && Read(out.instanceId, r);
}

// Reads the instance id list of a multi-instance transfer. ReadArray bounds the declared count
// against the bytes that remain before it allocates, so a crafted count cannot ask for a huge
// buffer.
inline bool ReadInstanceIds(uint32_t& numInstanceIds, const uint64_t*& instanceIds, ReadView& r, Allocator& alloc)
{
	uint64_t* ids = nullptr;
	const bool ok = ReadArray(
	    numInstanceIds, ids, r, alloc, [](uint64_t& id, ReadView& reader)
	    { return Read(id, reader); }, sizeof(uint64_t));
	instanceIds = ids;
	return ok;
}

inline bool Read(TxMsgTransferNonFungible_Multi& out, ReadView& r, Allocator& alloc)
{
	return Read(out.to, r) && Read(out.tokenId, r) && ReadInstanceIds(out.numInstanceIds, out.instanceIds, r, alloc);
}

inline bool Read(TxMsgTransferNonFungible_Multi_GasPayer& out, ReadView& r, Allocator& alloc)
{
	return Read(out.to, r) && Read(out.from, r) && Read(out.tokenId, r) &&
	       ReadInstanceIds(out.numInstanceIds, out.instanceIds, r, alloc);
}

// The address comes before the amount here, and the plain BurnFungible below carries no address at
// all. Reading these three in the wrong order parses without an error and yields a different token
// and a different amount, so the order is taken from the Write of each type.
inline bool Read(TxMsgMintFungible& out, ReadView& r)
{
	return Read(out.tokenId, r) && Read(out.to, r) && Read(out.amount.x(), r);
}

inline bool Read(TxMsgBurnFungible& out, ReadView& r)
{
	return Read(out.tokenId, r) && Read(out.amount.x(), r);
}

inline bool Read(TxMsgBurnFungible_GasPayer& out, ReadView& r)
{
	return Read(out.tokenId, r) && Read(out.from, r) && Read(out.amount.x(), r);
}

inline bool Read(TxMsgMintNonFungible& out, ReadView& r, Allocator& alloc)
{
	return Read(out.tokenId, r) && Read(out.to, r) && Read(out.seriesId, r) && ReadArray(out.rom, r, alloc) &&
	       ReadArray(out.ram, r, alloc);
}

inline bool Read(TxMsgBurnNonFungible& out, ReadView& r)
{
	return Read(out.tokenId, r) && Read(out.instanceId, r);
}

inline bool Read(TxMsgBurnNonFungible_GasPayer& out, ReadView& r)
{
	return Read(out.tokenId, r) && Read(out.from, r) && Read(out.instanceId, r);
}

// The smallest wire image each element of a trade list can have. The array reader bounds a declared
// count by the bytes that remain divided by this, so the count of a crafted message cannot allocate
// past the message itself. An intx is at least a header byte plus eight bytes.
constexpr size_t MinBytesTradeTransferFungible = Bytes32::length * 2 + 8 + 8;
constexpr size_t MinBytesTradeTransferNonFungible = Bytes32::length * 2 + 8 + 8;
constexpr size_t MinBytesTradeMintFungible = 8 + Bytes32::length + 9;
constexpr size_t MinBytesTradeBurnFungible = 8 + Bytes32::length + 9;
constexpr size_t MinBytesTradeMintNonFungible = 8 + Bytes32::length + 4 + 4 + 4;
constexpr size_t MinBytesTradeBurnNonFungible = 8 + Bytes32::length + 8;

inline bool Read(TxMsgTrade& out, ReadView& r, Allocator& alloc)
{
	return ReadArray(
	           out.numTransferF, out.transferF, r, alloc,
	           [](TxMsgTransferFungible_GasPayer& item, ReadView& reader)
	           { return Read(item, reader); },
	           MinBytesTradeTransferFungible) &&
	       ReadArray(
	           out.numTransferN, out.transferN, r, alloc,
	           [](TxMsgTransferNonFungible_Single_GasPayer& item, ReadView& reader)
	           { return Read(item, reader); },
	           MinBytesTradeTransferNonFungible) &&
	       ReadArray(
	           out.numMintF, out.mintF, r, alloc, [](TxMsgMintFungible& item, ReadView& reader)
	           { return Read(item, reader); },
	           MinBytesTradeMintFungible) &&
	       ReadArray(
	           out.numBurnF, out.burnF, r, alloc,
	           [](TxMsgBurnFungible_GasPayer& item, ReadView& reader)
	           { return Read(item, reader); }, MinBytesTradeBurnFungible) &&
	       ReadArray(
	           out.numMintN, out.mintN, r, alloc,
	           [&alloc](TxMsgMintNonFungible& item, ReadView& reader)
	           { return Read(item, reader, alloc); },
	           MinBytesTradeMintNonFungible) &&
	       ReadArray(
	           out.numBurnN, out.burnN, r, alloc,
	           [](TxMsgBurnNonFungible_GasPayer& item, ReadView& reader)
	           { return Read(item, reader); },
	           MinBytesTradeBurnNonFungible);
}

inline bool Read(TxMsgPhantasma& out, ReadView& r, Allocator& alloc)
{
	return Read(out.nexus, r) && Read(out.chain, r) && ReadArray(out.script, r, alloc);
}

inline bool Read(TxMsgPhantasma_Raw& out, ReadView& r, Allocator& alloc)
{
	return ReadArray(out.transaction, r, alloc);
}

// Reads a transaction message: the header every type carries, then the payload of the type the
// first byte names. Everything the message points at is cloned into `alloc`, so `alloc` must outlive
// `out`. The input bytes themselves do not.
inline bool Read(TxMsg& out, ReadView& r, Allocator& alloc)
{
	uint8_t type = 0;
	if( !(Read(type, r) && Read(out.expiry, r) && Read(out.maxGas, r) && Read(out.maxData, r) && Read(out.gasFrom, r) &&
	        Read(out.payload, r)) )
	{
		return false;
	}

	switch( out.type = (TxTypes)type )
	{
	case TxTypes::Call:
		return Read(out.call, r, alloc);
	case TxTypes::Call_Multi:
		return Read(out.callMulti, r, alloc);
	case TxTypes::Trade:
		return Read(out.trade, r, alloc);
	case TxTypes::TransferFungible:
		return Read(out.transferFt, r);
	case TxTypes::TransferFungible_GasPayer:
		return Read(out.transferFtGasPayer, r);
	case TxTypes::TransferNonFungible_Single:
		return Read(out.transferNftSingle, r);
	case TxTypes::TransferNonFungible_Single_GasPayer:
		return Read(out.transferNftSingleGasPayer, r);
	case TxTypes::TransferNonFungible_Multi:
		return Read(out.transferNftMulti, r, alloc);
	case TxTypes::TransferNonFungible_Multi_GasPayer:
		return Read(out.transferNftMultiGasPayer, r, alloc);
	case TxTypes::MintFungible:
		return Read(out.mintFungible, r);
	case TxTypes::BurnFungible:
		return Read(out.burnFungible, r);
	case TxTypes::BurnFungible_GasPayer:
		return Read(out.burnFungibleGasPayer, r);
	case TxTypes::MintNonFungible:
		return Read(out.mintNonFungible, r, alloc);
	case TxTypes::BurnNonFungible:
		return Read(out.burnNonFungible, r);
	case TxTypes::BurnNonFungible_GasPayer:
		return Read(out.burnNonFungibleGasPayer, r);
	case TxTypes::Phantasma:
		return Read(out.phantasma, r, alloc);
	case TxTypes::Phantasma_Raw:
		return Read(out.phantasmaRaw, r, alloc);
	default:
		// A type byte the SDK does not know names no payload layout, so the rest cannot be read.
		return false;
	}
}

inline void Write(const TxMsgCall& in, WriteView& w)
{
	Write4((int32_t)in.moduleId, w);
	Write4((int32_t)in.methodId, w);
	if( in.sections.numArgSections_negative < 0 )
	{
		Write4(in.sections.numArgSections_negative, w);
		const uint32_t count = (uint32_t)(-in.sections.numArgSections_negative);
		for( uint32_t i = 0; i != count; ++i )
		{
			const MsgCallArgs& section = in.sections.argSections[i];
			if( section.registerOffset < 0 )
			{
				Write4(section.registerOffset, w);
			}
			else
			{
				Write4((int32_t)section.args.length, w);
				if( section.args.length )
				{
					WriteBytes(section.args.bytes, section.args.length, w);
				}
			}
		}
		return;
	}
	Write4((int32_t)in.args.length, w);
	if( in.args.length )
	{
		WriteBytes(in.args.bytes, in.args.length, w);
	}
}

inline void Write(const TxMsgCall_Multi& in, WriteView& w)
{
	Write4((int32_t)in.numCalls, w);
	for( uint32_t i = 0; i != in.numCalls; ++i )
	{
		Write(in.calls[i], w);
	}
}

inline void Write(const TxMsgSpecialResolution& in, WriteView& w)
{
	Write(in.resolutionId, w);
	Write(in.calls, w);
}

inline void Write(const TxMsgTransferFungible& in, WriteView& w)
{
	Write(in.to, w);
	Write8u(in.tokenId, w);
	Write8u(in.amount, w);
}

inline void Write(const TxMsgTransferFungible_GasPayer& in, WriteView& w)
{
	Write(in.to, w);
	Write(in.from, w);
	Write8u(in.tokenId, w);
	Write8u(in.amount, w);
}

inline void Write(const TxMsgTransferNonFungible_Single& in, WriteView& w)
{
	Write(in.to, w);
	Write8u(in.tokenId, w);
	Write8u(in.instanceId, w);
}

inline void Write(const TxMsgTransferNonFungible_Single_GasPayer& in, WriteView& w)
{
	Write(in.to, w);
	Write(in.from, w);
	Write8u(in.tokenId, w);
	Write8u(in.instanceId, w);
}

inline void Write(const TxMsgTransferNonFungible_Multi& in, WriteView& w)
{
	Write(in.to, w);
	Write8u(in.tokenId, w);
	Write4((int32_t)in.numInstanceIds, w);
	for( uint32_t i = 0; i != in.numInstanceIds; ++i )
	{
		Write8u(in.instanceIds[i], w);
	}
}

inline void Write(const TxMsgTransferNonFungible_Multi_GasPayer& in, WriteView& w)
{
	Write(in.to, w);
	Write(in.from, w);
	Write8u(in.tokenId, w);
	Write4((int32_t)in.numInstanceIds, w);
	for( uint32_t i = 0; i != in.numInstanceIds; ++i )
	{
		Write8u(in.instanceIds[i], w);
	}
}

inline void Write(const TxMsgMintFungible& in, WriteView& w)
{
	Write8u(in.tokenId, w);
	Write(in.to, w);
	Write(in.amount, w);
}

inline void Write(const TxMsgBurnFungible& in, WriteView& w)
{
	Write8u(in.tokenId, w);
	Write(in.amount, w);
}

inline void Write(const TxMsgBurnFungible_GasPayer& in, WriteView& w)
{
	Write8u(in.tokenId, w);
	Write(in.from, w);
	Write(in.amount, w);
}

inline void Write(const TxMsgMintNonFungible& in, WriteView& w)
{
	Write8u(in.tokenId, w);
	Write(in.to, w);
	Write4((int32_t)in.seriesId, w);
	WriteArray(ByteArray(in.rom.bytes, in.rom.bytes + in.rom.length), w);
	WriteArray(ByteArray(in.ram.bytes, in.ram.bytes + in.ram.length), w);
}

inline void Write(const TxMsgBurnNonFungible& in, WriteView& w)
{
	Write8u(in.tokenId, w);
	Write8u(in.instanceId, w);
}

inline void Write(const TxMsgBurnNonFungible_GasPayer& in, WriteView& w)
{
	Write8u(in.tokenId, w);
	Write(in.from, w);
	Write8u(in.instanceId, w);
}

inline void Write(const TxMsgTrade& in, WriteView& w)
{
	Write4((int32_t)in.numTransferF, w);
	for( uint32_t i = 0; i != in.numTransferF; ++i )
		Write(in.transferF[i], w);
	Write4((int32_t)in.numTransferN, w);
	for( uint32_t i = 0; i != in.numTransferN; ++i )
		Write(in.transferN[i], w);
	Write4((int32_t)in.numMintF, w);
	for( uint32_t i = 0; i != in.numMintF; ++i )
		Write(in.mintF[i], w);
	Write4((int32_t)in.numBurnF, w);
	for( uint32_t i = 0; i != in.numBurnF; ++i )
		Write(in.burnF[i], w);
	Write4((int32_t)in.numMintN, w);
	for( uint32_t i = 0; i != in.numMintN; ++i )
		Write(in.mintN[i], w);
	Write4((int32_t)in.numBurnN, w);
	for( uint32_t i = 0; i != in.numBurnN; ++i )
		Write(in.burnN[i], w);
}

inline void Write(const TxMsgPhantasma& in, WriteView& w)
{
	Write(in.nexus, w);
	Write(in.chain, w);
	WriteArray(ByteArray(in.script.bytes, in.script.bytes + in.script.length), w);
}

inline void Write(const TxMsgPhantasma_Raw& in, WriteView& w)
{
	WriteArray(ByteArray(in.transaction.bytes, in.transaction.bytes + in.transaction.length), w);
}

inline void Write(const TxMsg& msg, WriteView& w)
{
	Write1((uint8_t)msg.type, w);
	Write8(msg.expiry, w);
	Write8u(msg.maxGas, w);
	Write8u(msg.maxData, w);
	Write(msg.gasFrom, w);
	Write(msg.payload, w);

	switch( msg.type )
	{
	case TxTypes::Call:
		Write(msg.call, w);
		break;
	case TxTypes::Call_Multi:
		Write(msg.callMulti, w);
		break;
	case TxTypes::Trade:
		Write(msg.trade, w);
		break;
	case TxTypes::TransferFungible:
		Write(msg.transferFt, w);
		break;
	case TxTypes::TransferFungible_GasPayer:
		Write(msg.transferFtGasPayer, w);
		break;
	case TxTypes::TransferNonFungible_Single:
		Write(msg.transferNftSingle, w);
		break;
	case TxTypes::TransferNonFungible_Single_GasPayer:
		Write(msg.transferNftSingleGasPayer, w);
		break;
	case TxTypes::TransferNonFungible_Multi:
		Write(msg.transferNftMulti, w);
		break;
	case TxTypes::TransferNonFungible_Multi_GasPayer:
		Write(msg.transferNftMultiGasPayer, w);
		break;
	case TxTypes::MintFungible:
		Write(msg.mintFungible, w);
		break;
	case TxTypes::BurnFungible:
		Write(msg.burnFungible, w);
		break;
	case TxTypes::BurnFungible_GasPayer:
		Write(msg.burnFungibleGasPayer, w);
		break;
	case TxTypes::MintNonFungible:
		Write(msg.mintNonFungible, w);
		break;
	case TxTypes::BurnNonFungible:
		Write(msg.burnNonFungible, w);
		break;
	case TxTypes::BurnNonFungible_GasPayer:
		Write(msg.burnNonFungibleGasPayer, w);
		break;
	case TxTypes::Phantasma:
		Write(msg.phantasma, w);
		break;
	case TxTypes::Phantasma_Raw:
		Write(msg.phantasmaRaw, w);
		break;
	default:
		Throw::Assert(false, "Unsupported transaction type");
		break;
	}
}

struct SignedTxMsg {
	TxMsg msg;
	Witnesses witnesses{};
};

// Returns true and the account that owns the assets for a _GasPayer message. Those types are signed
// twice: the gas payer signs first, this account second. Returns false for every other type, which
// has no such account.
inline bool TryGetGasPayerFrom(const TxMsg& msg, Bytes32& out)
{
	switch( msg.type )
	{
	case TxTypes::TransferFungible_GasPayer:
		out = msg.transferFtGasPayer.from;
		return true;
	case TxTypes::TransferNonFungible_Single_GasPayer:
		out = msg.transferNftSingleGasPayer.from;
		return true;
	case TxTypes::TransferNonFungible_Multi_GasPayer:
		out = msg.transferNftMultiGasPayer.from;
		return true;
	case TxTypes::BurnFungible_GasPayer:
		out = msg.burnFungibleGasPayer.from;
		return true;
	case TxTypes::BurnNonFungible_GasPayer:
		out = msg.burnNonFungibleGasPayer.from;
		return true;
	default:
		return false;
	}
}

inline void Write(const SignedTxMsg& signedMsg, WriteView& w)
{
	Write(signedMsg.msg, w);
	const TxTypes type = signedMsg.msg.type;
	const Witnesses& witnessList = signedMsg.witnesses;

	switch( type )
	{
	case TxTypes::TransferFungible:
	case TxTypes::TransferNonFungible_Single:
	case TxTypes::TransferNonFungible_Multi:
	case TxTypes::MintFungible:
	case TxTypes::BurnFungible:
	case TxTypes::MintNonFungible:
	case TxTypes::BurnNonFungible:
		Throw::Assert(witnessList.numWitnesses == 1 && witnessList.witnesses && witnessList.witnesses[0].address == signedMsg.msg.gasFrom, "invalid witness");
		Write(witnessList.witnesses[0].signature, w);
		return;

	case TxTypes::TransferFungible_GasPayer:
	case TxTypes::TransferNonFungible_Single_GasPayer:
	case TxTypes::TransferNonFungible_Multi_GasPayer:
	case TxTypes::BurnFungible_GasPayer:
	case TxTypes::BurnNonFungible_GasPayer: {
		Bytes32 from{};
		Throw::Assert(TryGetGasPayerFrom(signedMsg.msg, from), "invalid witness");
		Throw::Assert(witnessList.numWitnesses == 2 &&
		                  witnessList.witnesses &&
		                  witnessList.witnesses[0].address == signedMsg.msg.gasFrom &&
		                  witnessList.witnesses[1].address == from,
		    "invalid witness");
		Write(witnessList.witnesses[0].signature, w);
		Write(witnessList.witnesses[1].signature, w);
		return;
	}

	case TxTypes::Call:
	case TxTypes::Call_Multi:
	case TxTypes::Trade:
	case TxTypes::Phantasma: {
		Write4((int32_t)witnessList.numWitnesses, w);
		for( uint32_t i = 0; i != witnessList.numWitnesses; ++i )
		{
			Write(witnessList.witnesses[i], w);
		}
		return;
	}

	case TxTypes::Phantasma_Raw:
		Throw::Assert(witnessList.numWitnesses == 0, "invalid witness");
		return;

	default:
		Throw::Assert(false, "Unsupported transaction type");
		return;
	}
}

// Helpers --------------------------------------------------------------------
inline ByteArray SerializeTx(const TxMsg& msg)
{
	ByteArray buffer;
	WriteView w(buffer);
	Write(msg, w);
	return buffer;
}

// Returns true, with the number of witnesses in `out`, for the message types whose witness set the
// message itself fixes. It returns false for the witness-array types, which are Call, Call_Multi,
// Trade and Phantasma. The caller chooses the witnesses of those four, and nothing in the message
// says how many there will be.
inline bool RequiredWitnessCount(TxTypes type, uint32_t& out)
{
	switch( type )
	{
	case TxTypes::TransferFungible:
	case TxTypes::TransferNonFungible_Single:
	case TxTypes::TransferNonFungible_Multi:
	case TxTypes::MintFungible:
	case TxTypes::BurnFungible:
	case TxTypes::MintNonFungible:
	case TxTypes::BurnNonFungible:
		out = 1;
		return true;
	case TxTypes::TransferFungible_GasPayer:
	case TxTypes::TransferNonFungible_Single_GasPayer:
	case TxTypes::TransferNonFungible_Multi_GasPayer:
	case TxTypes::BurnFungible_GasPayer:
	case TxTypes::BurnNonFungible_GasPayer:
		out = 2;
		return true;
	case TxTypes::Phantasma_Raw:
		out = 0;
		return true;
	default:
		return false;
	}
}

// Returns the size in bytes of `msg` once signed. That is the envelope the block carries and gas
// model v2 bills. No key is needed, because signatures are fixed-width: the size follows from the
// serialized message and the witness layout of its type.
//
// The witness layout differs by type. The native transaction types append bare 64-byte signatures.
// The call, trade and script types append a length-prefixed array of entries, each a 32-byte address
// plus a 64-byte signature. A raw Gen2 envelope carries its signatures inside itself.
//
// `witnessCount` is used only by the witness-array types. For every other type the message fixes the
// count and this argument is ignored.
inline uint32_t EnvelopeBytes(const TxMsg& msg, uint32_t witnessCount)
{
	const size_t messageBytes = SerializeTx(msg).size();
	uint32_t required = 0;
	if( RequiredWitnessCount(msg.type, required) )
	{
		if( msg.type == TxTypes::Phantasma_Raw )
			return (uint32_t)messageBytes;
		return (uint32_t)(messageBytes + (size_t)Bytes64::length * required);
	}
	return (uint32_t)(messageBytes + 4 + (size_t)(Bytes32::length + Bytes64::length) * witnessCount);
}

// Reads a signed transaction envelope: the message, then the witnesses in the layout its type uses.
// Write(const SignedTxMsg&) writes the three layouts this reads back.
//
// `outSignedPortion` is the part of the input a signature is made over. It points into the input
// bytes, so the caller must keep them alive for as long as it uses that view. Everything else the
// message points at is cloned into `alloc`.
inline bool Read(TxMsg& outMsg, Witnesses& outWitnesses, ByteView& outSignedPortion, ReadView& r, Allocator& alloc)
{
	const Byte* begin = r.bytes;
	if( !Read(outMsg, r, alloc) )
	{
		return false;
	}
	outSignedPortion = ByteView{ begin, (size_t)(r.bytes - begin) };

	uint32_t required = 0;
	if( !RequiredWitnessCount(outMsg.type, required) )
	{
		// Call, Call_Multi, Trade and Phantasma carry a counted array of address plus signature.
		uint32_t count = 0;
		Witness* list = nullptr;
		const bool ok = ReadArray(
		    count, list, r, alloc,
		    [](Witness& witness, ReadView& reader)
		    { return Read(witness.address, reader) && Read(witness.signature, reader); },
		    Bytes32::length + Bytes64::length);
		outWitnesses = Witnesses{ count, list };
		return ok;
	}

	if( required == 0 )
	{
		// A raw Gen2 envelope carries its signatures inside its own payload.
		outWitnesses = Witnesses{ 0, nullptr };
		return true;
	}

	// The other types append bare signatures, and the message itself names the signers: the gas
	// payer first, then the account that owns the assets when the type has one.
	Witness* list = alloc.Alloc<Witness>(required);
	list[0].address = outMsg.gasFrom;
	if( required > 1 && !TryGetGasPayerFrom(outMsg, list[1].address) )
	{
		return false;
	}
	for( uint32_t i = 0; i != required; ++i )
	{
		if( !Read(list[i].signature, r) )
		{
			return false;
		}
	}
	outWitnesses = Witnesses{ required, list };
	return true;
}

// Reads a transaction message from `bytes`. Returns false if the image is truncated, if its type
// byte is unknown, or if a field does not fit. `alloc` must outlive `out`.
inline bool ParseTx(TxMsg& out, const ByteView& bytes, Allocator& alloc)
{
	ReadView r(bytes, alloc, ReadView::InPlace);
	return Read(out, r, alloc);
}

// Reads a signed transaction envelope from `bytes`, the shape TxMsgSigner::SignAndSerialize
// produces and the chain accepts. `outSignedPortion` points into `bytes`, so `bytes` must outlive
// it, and `alloc` must outlive `out`.
inline bool ParseSignedTx(SignedTxMsg& out, ByteView& outSignedPortion, const ByteView& bytes, Allocator& alloc)
{
	ReadView r(bytes, alloc, ReadView::InPlace);
	return Read(out.msg, out.witnesses, outSignedPortion, r, alloc);
}

struct TxMsgSigner {
	// A zero gas offer marks a message that was built and never planned. Both signing calls refuse it
	// before anything is signed, as the other SDKs do.
	static constexpr const char* NoGasOffer = "Transaction has no gas offer: plan its fees or set maxGas before signing";
	static constexpr const char* AmountAboveInt64 = "Transfer amount is above the int64 maximum the chain accepts in a native transfer";

	// Returns true if the message is a native fungible transfer whose amount the chain refuses. The
	// message carries the amount as a u64, and the chain reads it as a signed 64-bit value, so it
	// refuses 2^63 or more for every fungible token, big-fungible ones included. A larger amount needs
	// a module call or a script transfer.
	static bool HasAmountAboveInt64(const TxMsg& msg)
	{
		const uint64_t firstRefused = (uint64_t)1 << 63;
		if( msg.type == TxTypes::TransferFungible )
		{
			return msg.transferFt.amount >= firstRefused;
		}
		if( msg.type == TxTypes::TransferFungible_GasPayer )
		{
			return msg.transferFtGasPayer.amount >= firstRefused;
		}
		return false;
	}

	// Signs a message with one key. Answers an empty envelope when exceptions are disabled and the
	// message has no gas offer or carries a transfer amount the chain refuses.
	static ByteArray SignAndSerialize(const TxMsg& msg, const PhantasmaKeys& keys)
	{
		if( msg.maxGas == 0 )
		{
			PHANTASMA_EXCEPTION(NoGasOffer);
			return ByteArray();
		}
		if( HasAmountAboveInt64(msg) )
		{
			PHANTASMA_EXCEPTION(AmountAboveInt64);
			return ByteArray();
		}

		const ByteArray serializedMsg = SerializeTx(msg);
		const Ed25519Signature sig = Ed25519Signature::Generate(keys, serializedMsg);
		Bytes64 sigBytes(sig.Bytes(), Ed25519Signature::Length);

		Witness witness{ Bytes32(keys.GetPublicKey()), sigBytes };

		SignedTxMsg signedMsg;
		signedMsg.msg = msg;
		signedMsg.witnesses = Witnesses{ 1, &witness };

		ByteArray buffer;
		WriteView w(buffer);
		Write(signedMsg, w);
		return buffer;
	}

	// Signs a message with several keys. A _GasPayer message is signed twice: the gas payer first,
	// then the account whose assets move. A call, a batch, a trade or a script carries as many
	// witnesses as the caller chose, in the order given, and the gas payer has to be among them
	// because the chain rejects a transaction its payer did not sign.
	//
	// Answers false without touching `out` when the message has no gas offer or a transfer amount the
	// chain refuses, when a key is missing, when the count is not the one the message's type fixes, or
	// when a signer is not the account that message names. Each key signs the same serialized
	// message, so the signatures can be produced anywhere, including on separate machines.
	static bool SignAndSerialize(
	    const Blockchain::TxMsg& msg, const std::vector<const PhantasmaKeys*>& keys, ByteArray& out, std::string& outError)
	{
		const auto refuse = [&outError](const char* why)
		{
			PHANTASMA_EXCEPTION(why);
			outError = why;
			return false;
		};
		if( msg.maxGas == 0 )
		{
			return refuse(NoGasOffer);
		}
		if( HasAmountAboveInt64(msg) )
		{
			return refuse(AmountAboveInt64);
		}
		if( keys.empty() )
		{
			return refuse("at least one signer is required");
		}
		for( size_t i = 0; i != keys.size(); ++i )
		{
			if( keys[i] == nullptr )
			{
				return refuse("a signer is null");
			}
		}

		uint32_t required = 0;
		if( RequiredWitnessCount(msg.type, required) )
		{
			if( keys.size() != required )
			{
				return refuse("this transaction type fixes how many signers it takes");
			}
			if( required >= 1 && Bytes32(keys[0]->GetPublicKey()) != msg.gasFrom )
			{
				return refuse("the first signer must be the gas payer the message names");
			}
			Bytes32 from{};
			if( required == 2 && (!TryGetGasPayerFrom(msg, from) || Bytes32(keys[1]->GetPublicKey()) != from) )
			{
				return refuse("the second signer must be the account the message takes the assets from");
			}
		}

		const ByteArray serializedMsg = SerializeTx(msg);
		PHANTASMA_VECTOR<Witness> witnesses;
		witnesses.reserve(keys.size());
		for( size_t i = 0; i != keys.size(); ++i )
		{
			const Ed25519Signature sig = Ed25519Signature::Generate(*keys[i], serializedMsg);
			witnesses.push_back(Witness{ Bytes32(keys[i]->GetPublicKey()), Bytes64(sig.Bytes(), Ed25519Signature::Length) });
		}

		SignedTxMsg signedMsg;
		signedMsg.msg = msg;
		signedMsg.witnesses = Witnesses{ (uint32_t)witnesses.size(), witnesses.data() };

		ByteArray buffer;
		WriteView w(buffer);
		Write(signedMsg, w);
		out = buffer;
		return true;
	}
};

} // namespace phantasma::carbon::Blockchain
