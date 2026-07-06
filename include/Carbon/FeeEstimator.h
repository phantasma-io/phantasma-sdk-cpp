#pragma once
#ifndef PHANTASMA_API_INCLUDED
#error "Configure and include PhantasmaAPI.h first"
#endif

#include "DataBlockchain.h"

namespace phantasma::carbon {

// Tier-1 static fee calculator: closed-form gas/data offers for native operations, exact under
// both gas models (selected by GasConfig.version). Mirrors the validator billing formula (node
// blockchain.cpp settlement + token/governance contract gas sites); any change to those
// formulas ships as a new gas-model version, never silently.

// Native operation kinds supported by the Tier-1 fee estimator.
enum class NativeFeeKind
{
	TransferFungible, // fungible token transfer (TxTypes TransferFungible / _GasPayer)
	TransferNonFungible, // NFT transfer of `count` instances
	MintFungible, // fungible mint
	MintNonFungible, // NFT mint of `count` instances
	BurnFungible, // fungible burn
	BurnNonFungible, // NFT burn of `count` instances
	CreateToken, // TokenContract.CreateToken call; set symbolLength when a symbol is used
	CreateTokenSeries, // TokenContract.CreateTokenSeries call
	RegisterName, // GovernanceContract.RegisterName call; nameLength is required
	// Generic Phantasma VM script transaction (AllowGas/SpendGas pattern: stake, marketplace,
	// custom contract calls). Script opcode costs are not closed-form in Tier-1; the estimate
	// budgets scriptUnitsAllowance VM work units on top of the byte fee. For an exact script
	// bill use the node-side Tier-2 estimator once available.
	Script,
};

// Gas model v2 price of block-carried bytes, in gas units per byte. A versioned consensus
// constant of the v2 gas model (node data_blockchain.h kGasModelV2UnitsPerBlockDataByte),
// deliberately not part of the on-chain config.
constexpr uint64_t GasModelV2UnitsPerBlockDataByte = 25;

// Serialized size of one witness-array entry (32-byte address + 64-byte signature).
constexpr uint32_t WitnessArrayEntryBytes = 96;

// Serialized size of one bare signature (native TxTypes carry no witness array).
constexpr uint32_t NativeSignatureBytes = 64;

// Optional inputs for EstimateNativeFee. The defaults produce a safe single-signer estimate;
// set fields for exactness (the unused part of a gas offer is always refunded, so generous
// values only lock the balance for the block, they cost nothing).
struct NativeFeeParams {
	// Instance count for NFT kinds (transferred/minted/burned instances).
	uint32_t count = 1;
	// Token symbol length in characters (CreateToken). 0 = no symbol.
	int symbolLength = 0;
	// Registered name length in characters (RegisterName kind, required).
	int nameLength = 0;
	// Full signed transaction size in bytes (the envelope carried in the block). 0 = use a
	// conservative per-kind default. Under gas model v2 every envelope byte is billed, so pass
	// the real size (see EnvelopeBytesFor) for exact numbers.
	uint32_t envelopeBytes = 0;
	// User payload bytes attached to the tx (billed under gas model v1).
	uint32_t payloadBytes = 0;
	// Maximum number of paid storage rows the transaction can create (fresh token-balance
	// rows, NFT lookup rows, ...). -1 = use the per-kind worst-case default; 0 = the tx cannot
	// create paid rows. SOUL/KCAL balance rows are free and never count. Determines maxData:
	// escrow = rows * dataEscrowPerRow.
	int64_t freshRows = -1;
	// Per-instance ROM+RAM bytes for MintNonFungible (stored state, drives escrow).
	uint32_t romRamBytes = 0;
	// VM work-unit allowance for the Script kind. The default (5000) exceeds every script seen
	// in mainnet history (max 3392 units) with margin.
	uint64_t scriptUnitsAllowance = 5000;
};

// Result of a Tier-1 fee estimate. Gas values are kcal-base (1 KCAL = 1e10 kcal-base); maxData
// is in data-token atoms (SOUL, 1 SOUL = 1e8 atoms).
struct NativeFeeEstimate {
	// Recommended gas offer (TxMsg maxGas). Includes deterministic headroom; the unused part is
	// refunded.
	uint64_t maxGas = 0;
	// Recommended storage-escrow ceiling (TxMsg maxData). Only the actually created rows are
	// escrowed.
	uint64_t maxData = 0;
	// The bill the chain formula yields for exactly the provided inputs (no headroom).
	uint64_t expectedGasBill = 0;
};

namespace FeeEstimatorDetail {

constexpr uint64_t kU64Max = ~(uint64_t)0;

inline uint64_t SaturatingAdd(uint64_t a, uint64_t b)
{
	return a > kU64Max - b ? kU64Max : a + b;
}

inline uint64_t SaturatingMul(uint64_t a, uint64_t b)
{
	if( a == 0 || b == 0 )
		return 0;
	return a > kU64Max / b ? kU64Max : a * b;
}

// Chain fee scaling: (value * feeMultiplier) >> feeShift with a 128-bit intermediate,
// saturating to u64. Matches the validator's v2 MulShiftSaturateU64; for sane v1 configs (live
// values never overflow 64 bits) it is also bit-identical to the v1 math.
inline uint64_t MulShift(uint64_t value, uint64_t multiplier, uint8_t shift)
{
	if( shift >= 64 )
		return 0; // the chain clamps oversized shifts to a zero delta
#if defined(__SIZEOF_INT128__)
	unsigned __int128 wide = (unsigned __int128)value * multiplier;
	wide >>= shift;
	return wide > kU64Max ? kU64Max : (uint64_t)wide;
#else
	// Portable 64x64 -> 128 via 32-bit halves.
	const uint64_t aLo = value & 0xFFFFFFFFu, aHi = value >> 32;
	const uint64_t bLo = multiplier & 0xFFFFFFFFu, bHi = multiplier >> 32;
	const uint64_t ll = aLo * bLo;
	const uint64_t lh = aLo * bHi;
	const uint64_t hl = aHi * bLo;
	const uint64_t hh = aHi * bHi;
	const uint64_t mid = (ll >> 32) + (lh & 0xFFFFFFFFu) + (hl & 0xFFFFFFFFu);
	uint64_t lo = (ll & 0xFFFFFFFFu) | (mid << 32);
	uint64_t hi = hh + (lh >> 32) + (hl >> 32) + (mid >> 32);
	if( shift == 0 )
		return hi ? kU64Max : lo;
	if( (hi >> shift) != 0 )
		return kU64Max;
	return (hi << (64 - shift)) | (lo >> shift);
#endif
}

inline uint64_t CeilDiv(uint64_t value, uint64_t divisor)
{
	return value / divisor + (value % divisor ? 1 : 0);
}

inline uint64_t RoundUp(uint64_t value, uint64_t step)
{
	return CeilDiv(value, step) * step;
}

// Returns the fee shift for a symbol/name length, or -1 when the length could never be
// admitted by the chain (shift must stay below maxNameLength / maxTokenSymbolLength). The -1
// sentinel lets no-exception builds reject the input instead of quoting a wrong fee.
inline int SymbolShift(int length, uint8_t maxLength)
{
	if( length < 0 )
	{
		PHANTASMA_EXCEPTION("fee estimate length parameter must not be negative");
		return -1;
	}
	if( length == 0 )
		return 0;
	const int shift = length - 1;
	if( maxLength != 0 && shift >= (int)maxLength )
	{
		PHANTASMA_EXCEPTION("fee estimate length parameter exceeds the chain maximum");
		return -1;
	}
	return shift;
}

// Worst-case paid rows the operation can create (drives maxData). Refund-only operations
// (burns) need no escrow allowance: refunds never require maxData budget.
inline uint64_t DefaultFreshRows(NativeFeeKind kind, uint32_t count, uint32_t romRamBytes)
{
	switch( kind )
	{
	case NativeFeeKind::TransferFungible:
	case NativeFeeKind::MintFungible:
		return 1; // recipient balance row may be fresh (SOUL/KCAL rows would be free)
	case NativeFeeKind::TransferNonFungible:
		// Per instance the chain deletes the sender's NFT-lookup row and creates the
		// recipient's (creation escrows at the current price, the deletion refunds the old
		// row's own deposit) + possibly a fresh recipient balance row.
		return (uint64_t)count + 1;
	case NativeFeeKind::MintNonFungible:
		// Per instance: owner row + lookup row + the instance state rows holding ROM/RAM
		// (1 KiB quanta), plus possibly a fresh recipient balance row.
		return (uint64_t)count * (2 + CeilDiv(romRamBytes, 1024)) + 1;
	case NativeFeeKind::BurnFungible:
	case NativeFeeKind::BurnNonFungible:
		return 0;
	case NativeFeeKind::CreateToken:
		return 8; // token info + symbol lookup + supply/config rows, metadata-dependent
	case NativeFeeKind::CreateTokenSeries:
		return 4;
	case NativeFeeKind::RegisterName:
		return 2; // name->address and address->name rows
	case NativeFeeKind::Script:
	default:
		return 4;
	}
}

// Conservative single-witness envelope defaults per kind; used only when the caller did not
// measure the real signed size. Generous by design: under v2 an oversized estimate only raises
// the refunded offer, never the settled bill.
inline uint64_t DefaultEnvelopeBytes(NativeFeeKind kind, uint32_t count, uint32_t romRamBytes)
{
	switch( kind )
	{
	case NativeFeeKind::TransferFungible:
	case NativeFeeKind::MintFungible:
	case NativeFeeKind::BurnFungible:
	case NativeFeeKind::RegisterName:
		return 512;
	case NativeFeeKind::TransferNonFungible:
	case NativeFeeKind::BurnNonFungible:
		return 512 + 8ull * count; // 8 bytes per carried instance id
	case NativeFeeKind::MintNonFungible:
		return 512 + (uint64_t)count * (64 + (uint64_t)romRamBytes); // ROM/RAM ride the envelope
	case NativeFeeKind::CreateToken:
		return 4096; // token metadata (icons, descriptions) dominates
	case NativeFeeKind::CreateTokenSeries:
		return 2048;
	case NativeFeeKind::Script:
	default:
		return 1024;
	}
}

inline uint64_t BillV2(uint64_t workUnits, uint64_t envelope, uint64_t eventBytes, uint64_t rows, uint64_t policyFee, const Blockchain::GasConfig& config)
{
	const uint64_t blockData = SaturatingAdd(SaturatingAdd(envelope, eventBytes), rows);
	const uint64_t byteUnits = SaturatingMul(blockData, GasModelV2UnitsPerBlockDataByte);
	uint64_t bill = MulShift(SaturatingAdd(workUnits, byteUnits), config.feeMultiplier, config.feeShift);
	bill = SaturatingAdd(bill, policyFee);
	return bill < config.minimumGasBill ? config.minimumGasBill : bill;
}

} // namespace FeeEstimatorDetail

// Converts a getGasConfig response (PhantasmaAPI::GetGasConfig) into the wire-format GasConfig
// consumed by EstimateNativeFee. A gas-model-v2 response missing its tail fields already failed
// the JSON parse (DeserializeGasConfigData requires them for version >= 1).
inline Blockchain::GasConfig ToGasConfig(const rpc::GasConfigData& data)
{
	Blockchain::GasConfig config{};
	config.version = (uint8_t)data.version;
	config.maxNameLength = (uint8_t)data.maxNameLength;
	config.maxTokenSymbolLength = (uint8_t)data.maxTokenSymbolLength;
	config.feeShift = (uint8_t)data.feeShift;
	config.maxStructureSize = data.maxStructureSize;
	config.feeMultiplier = data.feeMultiplier;
	config.gasTokenId = data.gasTokenId;
	config.dataTokenId = data.dataTokenId;
	config.minimumGasOffer = data.minimumGasOffer;
	config.dataEscrowPerRow = data.dataEscrowPerRow;
	config.gasFeeTransfer = data.gasFeeTransfer;
	config.gasFeeQuery = data.gasFeeQuery;
	config.gasFeeCreateTokenBase = data.gasFeeCreateTokenBase;
	config.gasFeeCreateTokenSymbol = data.gasFeeCreateTokenSymbol;
	config.gasFeeCreateTokenSeries = data.gasFeeCreateTokenSeries;
	config.gasFeePerByte = data.gasFeePerByte;
	config.gasFeeRegisterName = data.gasFeeRegisterName;
	config.gasBurnRatioMul = data.gasBurnRatioMul;
	config.gasBurnRatioShift = (uint8_t)data.gasBurnRatioShift;
	if( data.hasV2Fields )
	{
		config.minimumGasBill = data.minimumGasBill;
		config.gasProducerRatioMul = data.gasProducerRatioMul;
		config.gasProducerRatioShift = (uint8_t)data.gasProducerRatioShift;
		config.gasDappRatioMul = data.gasDappRatioMul;
		config.gasDappRatioShift = (uint8_t)data.gasDappRatioShift;
		config.policyFeeCreateTokenBase = data.policyFeeCreateTokenBase;
		config.policyFeeCreateTokenSymbol = data.policyFeeCreateTokenSymbol;
		config.policyFeeCreateTokenSeries = data.policyFeeCreateTokenSeries;
		config.policyFeeRegisterName = data.policyFeeRegisterName;
		config.legacyDataEscrowPerRow = data.legacyDataEscrowPerRow;
	}
	return config;
}

// Envelope size (signed tx bytes as carried in the block) from a serialized unsigned message
// length and the number of signers. Use with NativeFeeParams::envelopeBytes for exact v2
// estimates. Witness layout mirrors SignedTxMsg: native TxTypes append bare 64-byte signatures
// (one, or two for the _GasPayer variants); Call/Trade/Phantasma txs append a length-prefixed
// witness array (32-byte address + 64-byte signature per entry).
inline uint32_t EnvelopeBytesFor(NativeFeeKind kind, uint32_t serializedMessageLength, uint32_t witnessCount = 1)
{
	switch( kind )
	{
	case NativeFeeKind::TransferFungible:
	case NativeFeeKind::TransferNonFungible:
	case NativeFeeKind::MintFungible:
	case NativeFeeKind::MintNonFungible:
	case NativeFeeKind::BurnFungible:
	case NativeFeeKind::BurnNonFungible:
		return serializedMessageLength + NativeSignatureBytes * witnessCount;
	default:
		// CreateToken/CreateTokenSeries/RegisterName ride TxTypes.Call; Script rides
		// TxTypes.Phantasma - both carry the witness array form.
		return serializedMessageLength + 4 + WitnessArrayEntryBytes * witnessCount;
	}
}

// Estimates maxGas/maxData for a native operation under the given on-chain gas config (see the
// getGasConfig RPC method / PhantasmaAPI::GetGasConfig). Invalid inputs (zero count, missing
// nameLength, over-limit symbol/name lengths) raise PHANTASMA_EXCEPTION when exceptions are
// enabled and otherwise return an all-zero estimate - a zero maxGas offer can never be mistaken
// for a real quote.
inline NativeFeeEstimate EstimateNativeFee(NativeFeeKind kind, const Blockchain::GasConfig& config, const NativeFeeParams& params = {})
{
	using namespace FeeEstimatorDetail;
	NativeFeeEstimate result{};
	const uint32_t count = params.count;
	if( count == 0 )
	{
		PHANTASMA_EXCEPTION("fee estimate count must be positive");
		return result;
	}
	const bool v2 = config.HasGasModelV2();

	// Work units consumed by the operation itself (the ConsumeGas amounts in the token /
	// governance contracts) and, under v2, the direct kcal-base policy fee that replaces the
	// v1 unit-priced product prices.
	uint64_t workUnits = 0;
	uint64_t policyFee = 0;
	switch( kind )
	{
	case NativeFeeKind::TransferFungible:
	case NativeFeeKind::MintFungible:
	case NativeFeeKind::BurnFungible:
		workUnits = config.gasFeeTransfer;
		break;
	case NativeFeeKind::TransferNonFungible:
	case NativeFeeKind::MintNonFungible:
	case NativeFeeKind::BurnNonFungible:
		workUnits = SaturatingMul(config.gasFeeTransfer, count);
		break;
	case NativeFeeKind::CreateToken: {
		// Symbol price halves per character after the first; shift is validated by the chain
		// against maxTokenSymbolLength, mirror that bound here.
		const int shift = SymbolShift(params.symbolLength, config.maxTokenSymbolLength);
		if( shift < 0 )
			return result; // invalid input: all-zero estimate (see the function comment)
		if( v2 )
		{
			policyFee = config.policyFeeCreateTokenBase;
			if( params.symbolLength > 0 )
				policyFee = SaturatingAdd(policyFee, config.policyFeeCreateTokenSymbol >> shift);
		}
		else
		{
			workUnits = config.gasFeeCreateTokenBase;
			if( params.symbolLength > 0 )
				workUnits = SaturatingAdd(workUnits, config.gasFeeCreateTokenSymbol >> shift);
		}
		break;
	}
	case NativeFeeKind::CreateTokenSeries:
		if( v2 )
			policyFee = config.policyFeeCreateTokenSeries;
		else
			workUnits = config.gasFeeCreateTokenSeries;
		break;
	case NativeFeeKind::RegisterName: {
		if( params.nameLength <= 0 )
		{
			PHANTASMA_EXCEPTION("fee estimate nameLength is required for RegisterName");
			return result;
		}
		const int shift = SymbolShift(params.nameLength, config.maxNameLength);
		if( shift < 0 )
			return result; // invalid input: all-zero estimate (see the function comment)
		if( v2 )
			policyFee = config.policyFeeRegisterName >> shift;
		else
			workUnits = config.gasFeeRegisterName >> shift;
		break;
	}
	case NativeFeeKind::Script:
		workUnits = params.scriptUnitsAllowance;
		break;
	}

	const uint64_t rows = params.freshRows >= 0 ? (uint64_t)params.freshRows : DefaultFreshRows(kind, count, params.romRamBytes);
	const uint64_t envelope = params.envelopeBytes != 0 ? params.envelopeBytes : DefaultEnvelopeBytes(kind, count, params.romRamBytes);
	// Native TxTypes store no events in the block; script txs do (Notify), so budget some.
	const uint64_t eventBytes = kind == NativeFeeKind::Script ? 512u : 0u;

	if( v2 )
	{
		// v2 formula: bill = MulShift(workUnits + blockData*25, mult, shift) + policyFee,
		// floored at minimumGasBill. blockData = envelope + events + net storage quanta (quanta
		// are added to the byte count by the chain formula).
		result.expectedGasBill = BillV2(workUnits, envelope, eventBytes, rows, policyFee, config);
		// Offer headroom: +25% over the padded bill covers witness-size wiggle and event
		// variance; deterministic and always refunded down to the actual bill.
		const uint64_t padded = BillV2(workUnits, RoundUp(envelope, 128), eventBytes, rows, policyFee, config);
		result.maxGas = SaturatingAdd(padded, padded / 4);
		if( result.maxGas < config.minimumGasBill )
			result.maxGas = config.minimumGasBill;
		if( result.maxGas < config.minimumGasOffer )
			result.maxGas = config.minimumGasOffer;
	}
	else
	{
		// v1 formula: bill = (workUnits * mult >> shift) + blockData * gasFeePerByte where
		// blockData = payload + events + net storage quanta (no envelope term, no floor).
		const uint64_t work = MulShift(workUnits, config.feeMultiplier, config.feeShift);
		const uint64_t blockData = SaturatingAdd(SaturatingAdd(params.payloadBytes, eventBytes), rows);
		result.expectedGasBill = SaturatingAdd(work, SaturatingMul(blockData, config.gasFeePerByte));
		// Offer shape mirrors the validator's own test-agent stdFee: a 2x minimum-offer pad
		// plus a flat 1 KiB block-data allowance on top of the work term.
		const uint64_t byteAllowance = blockData > 1024 ? blockData : 1024;
		result.maxGas = SaturatingAdd(SaturatingAdd(config.minimumGasOffer * 2, work),
		    SaturatingMul(byteAllowance, config.gasFeePerByte));
	}

	result.maxData = FeeEstimatorDetail::SaturatingMul(rows, config.dataEscrowPerRow);
	return result;
}

} // namespace phantasma::carbon
