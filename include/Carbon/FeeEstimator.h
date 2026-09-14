#pragma once
#ifndef PHANTASMA_API_INCLUDED
#error "Configure and include PhantasmaAPI.h first"
#endif

#include "DataBlockchain.h"

namespace phantasma::carbon {

// Tier-1 static fee calculator: the exact gas bill and storage escrow of the native operations,
// under both gas models (selected by GasConfig.version). It reproduces the chain's own settlement
// arithmetic together with the gas each contract path charges (node blockchain.cpp settlement plus
// the token and governance contract gas sites).
//
// An offline calculation is safe here because any change to those formulas ships as a new gas-model
// version. It never changes silently.
//
// To price a message instead of naming an operation by hand, use PlanFees in FeePlan.h.

// Native operations the Tier-1 fee calculator models exactly.
enum class NativeFeeKind
{
	// Fungible token transfer (TxTypes TransferFungible and its _GasPayer variant).
	TransferFungible,
	// NFT transfer of `count` instances (TxTypes TransferNonFungible_* and their _GasPayer
	// variants).
	TransferNonFungible,
	// Fungible mint (TxTypes MintFungible).
	MintFungible,
	// NFT mint of `count` instances with caller-supplied ROM (TxTypes MintNonFungible). The ROM is
	// stored exactly as submitted. A deterministic Phantasma mint stores a canonical ROM.
	//
	// The chain refuses this operation where governance has not allowed caller-supplied ROM ids,
	// and that is the state of the public chains today. The SDK carries no builder for it. The
	// model is kept because the calculator prices the operation correctly wherever it is allowed.
	MintNonFungible,
	// Deterministic Phantasma NFT mint of `count` instances
	// (TokenContract.MintPhantasmaNonFungible). The chain stores a canonical ROM. It holds the
	// public ROM, the derived Phantasma NFT id and a second copy of the public ROM, so storage
	// grows at twice the ROM size.
	MintPhantasmaNonFungible,
	// Fungible burn (TxTypes BurnFungible and its _GasPayer variant).
	BurnFungible,
	// NFT burn of `count` instances (TxTypes BurnNonFungible and its _GasPayer variant).
	BurnNonFungible,
	// TokenContract.CreateToken call. Set symbolLength when a symbol is used.
	CreateToken,
	// TokenContract.CreateTokenSeries call.
	CreateTokenSeries,
	// GovernanceContract.RegisterName call. nameLength is required.
	RegisterName,
	// Generic Phantasma VM script transaction (AllowGas/SpendGas pattern: stake, marketplace,
	// custom contract calls). Script opcode costs depend on chain state, and no formula gives them.
	// The estimate budgets scriptUnitsAllowance VM work units and scriptEventBytes of events on top
	// of the byte fee. For an exact script bill use the node-side estimator (EstimateTransaction).
	Script,
};

// Gas model v2 price of block-carried bytes, in gas units per byte.
//
// This is a versioned consensus constant of the v2 gas model (node data_blockchain.h
// kGasModelV2UnitsPerBlockDataByte), and it is kept out of the on-chain config on purpose. It
// changes only with a new gas model version, so a client can hold it as a constant and never read
// it per block.
constexpr uint64_t GasModelV2UnitsPerBlockDataByte = 25;

// Storage is escrowed per 1024-byte quantum of a row's key plus its value.
constexpr uint64_t StorageQuantumBytes = 1024;

// Serialized size of one witness-array entry (32-byte address + 64-byte signature).
constexpr uint32_t WitnessArrayEntryBytes = 96;

// Serialized size of one bare signature (native TxTypes carry no witness array).
constexpr uint32_t NativeSignatureBytes = 64;

// Says that the caller does not know which token an operation moves. Balance rows of the chain's
// gas and data tokens are free, and only the id tells which token this is, so an unknown id is
// priced as paid and the escrow ceiling can only come out too high.
constexpr uint64_t UnknownTokenId = (uint64_t)-1;

// An asset held at a burned NFT's own address. The burn returns it to the burner.
struct InfusedAsset {
	// The token's id, or UnknownTokenId.
	uint64_t tokenId = UnknownTokenId;
	// True for an NFT token. The burn returns every instance the address holds, see instanceCount.
	bool nonFungible = false;
	// The burner already holds this token, so no balance row is created when it comes back. The
	// default is the costlier reading. It moves the escrow ceiling and never the bill, because a
	// burn refunds more rows than the return creates.
	bool burnerHoldsToken = false;
	// How many instances of an NFT token the address holds. Each one costs a transfer and moves a
	// lookup row.
	uint32_t instanceCount = 1;
};

// Inputs of EstimateNativeFee. Under gas model v2 the chain bills every byte the transaction puts
// in the block and escrows every new storage row. The inputs are therefore the sizes the chain will
// see: the signed envelope, the serialized structures the operation stores, and the facts about
// existing state that decide whether a row is new.
//
// The inputs are of two kinds, and they are defaulted differently.
//
// Facts the CALLER CANNOT KNOW without reading chain state. Examples: whether the recipient already
// holds the token, whether a ROM carries an `_i` id, which mode a series mints in. Each field's
// default is the case that costs MORE. An estimate built from defaults is then an upper bound, and
// the settlement can only come out below it.
//
// Facts carried by the MESSAGE ITSELF. Examples: the instance count, the serialized sizes, whether
// the token being created is non-fungible or carries `pre_burn`. These are not guesses, so they
// have no safe default. Pass them. PlanFees reads every one of them out of the message.
struct NativeFeeParams {
	// Full signed transaction size in bytes. This is the envelope the block carries. Required under
	// gas model v2 (see EnvelopeBytesFor and EnvelopeBytes). Ignored under v1, which billed the
	// payload note alone. The batch entry point takes it from its transaction argument instead,
	// because one block carries one envelope however many operations the message performs.
	uint32_t envelopeBytes = 0;
	// User payload bytes attached to the tx (billed under gas model v1 only). The batch entry point
	// takes it from its transaction argument.
	uint32_t payloadBytes = 0;
	// Instance count for NFT kinds (transferred / minted / burned instances).
	uint32_t count = 1;
	// Token moved by a transfer / mint / burn. Balance rows of the chain's gas and data tokens are
	// free, so with the token id known the estimate escrows nothing for them.
	uint64_t tokenId = UnknownTokenId;
	// The recipient already holds this token, so its balance row exists and costs nothing.
	bool recipientHoldsToken = false;
	// The recipient is an NFT-derived address, which means an infusion. The chain reads that NFT's
	// owner, and that costs one extra query fee. Transfers and every mint kind pay it. A burn has
	// no recipient.
	//
	// This is a fact of the recipient's address form. It says nothing about chain state. PlanFees
	// derives it from the message's own recipient with TokenHelper::IsNftAddress, so only direct
	// callers of this calculator pass it.
	bool toIsNftAddress = false;
	// The token's balances can exceed int64. Such a token is called big-fungible.
	//
	// A fungible mint or burn answers with the RESULTING balance as a variable-length integer. That
	// is 9 bytes while the balance fits int64, and up to 33 bytes for an int256 balance. The
	// resulting balance is chain state, so the default prices the 33-byte maximum. The offer then
	// covers the bill and the difference is refunded. Set false for an ordinary int64 token and the
	// estimate is exact.
	bool bigFungible = true;
	// The token has been burned before, so its burnt counter row exists. The default is the first
	// burn, which creates it.
	bool tokenBurnedBefore = false;
	// The token's supply-tracking row exists. The chain drops that row when its balance reaches
	// exactly zero, so the row can be absent in two cases. A limited-supply token has its entire
	// supply in circulation, and the next burn recreates the row. An unlimited token has nothing
	// outstanding, and the next mint recreates it.
	//
	// The default prices the recreation in every mint and burn. That is one more storage quantum in
	// the bill and in the escrow ceiling, and it covers both cases. Set true for the exact quote
	// whenever the token is in neither of them. Rows of the chain's gas and data tokens are free
	// either way.
	bool supplyRowExists = false;
	// What the burned NFTs hold at their own addresses (BurnNonFungible). There is one entry per
	// asset per burned instance.
	//
	// The burn returns every one of them to the burner, and the chain charges for each. A fungible
	// token costs a transfer fee plus the owner-lookup query of the NFT-address source. An NFT
	// token costs an instance query, a transfer per instance, and that same lookup. A returned
	// token the burner does not hold also costs the burner's new balance row.
	//
	// This is chain state that the message does not carry, and an NFT can hold any number of
	// assets, so there is no costlier bound to assume. An empty list prices an empty address,
	// because a direct caller of this calculator states what it knows. PlanFees demands the list.
	const InfusedAsset* infusions = nullptr;
	uint32_t numInfusions = 0;
	// Token symbol length in characters (CreateToken). 0 = no symbol.
	uint32_t symbolLength = 0;
	// Serialized TokenInfo length (CreateToken). These are the Call arguments, and they become the
	// token-info row.
	uint32_t tokenInfoBytes = 0;
	// The token being created is non-fungible (CreateToken). It costs one more row, the series
	// counter.
	bool nonFungible = false;
	// The token metadata carries `pre_burn` (CreateToken). The burnt counter row is then created at
	// once.
	bool hasPreBurn = false;
	// The token metadata carries an inflation schedule (CreateToken). The next-inflation row is
	// then created.
	bool hasInflationSchedule = false;
	// The token metadata names a staking organisation (CreateToken). The creation looks that
	// organisation up, which costs one query fee.
	bool hasStakingOrganisation = false;
	// The token metadata names a staking reward token (CreateToken). The creation reads that
	// token's info, which costs one query fee.
	bool hasStakingRewardToken = false;
	// Serialized SeriesInfo length (CreateTokenSeries). These are the Call arguments after the
	// token id.
	uint32_t seriesInfoBytes = 0;
	// The series metadata carries a `_i` id (CreateTokenSeries). The meta-id lookup row is then
	// created. The metadata is schema-encoded like the ROM, so a caller holding only the bytes
	// cannot tell. The default pays for the row.
	bool seriesHasMetaId = true;
	// Registered name length in characters (RegisterName). Required for that kind.
	uint32_t nameLength = 0;
	// ROM bytes per minted or burned instance (MintNonFungible and BurnNonFungible: as stored;
	// MintPhantasmaNonFungible: the public ROM). This value applies to every instance. Set
	// romBytesPerInstance to size them one by one.
	uint32_t romBytes = 0;
	// One ROM size per instance, `count` entries. When set, romBytes is ignored.
	const uint32_t* romBytesPerInstance = nullptr;
	// RAM bytes per instance. The default is no RAM row.
	uint32_t ramBytes = 0;
	// One RAM size per instance, `count` entries. When set, ramBytes is ignored.
	const uint32_t* ramBytesPerInstance = nullptr;
	// The raw ROM carries a `_i` id, and the chain indexes it in one more row (MintNonFungible and
	// BurnNonFungible). The ROM is schema-encoded, so a caller holding only the bytes cannot tell.
	//
	// The default is true. On a mint that is the reading which escrows for the row. On a burn it is
	// the reading that mirrors what the mint created. A burn never prices on this fact either way
	// (see NativeFeeEstimate::deletedStorageQuanta). A Phantasma mint always has such an id and
	// ignores this input.
	bool romHasMetaId = true;
	// The series mints duplicated NFTs (MintPhantasmaNonFungible). A duplicated series costs one
	// more query fee per instance than a unique one, plus one per distinct series (see
	// distinctSeriesCount). A call whose instances mix duplicated and unique series is priced as if
	// every instance were duplicated.
	//
	// The default is true. A series' mode is chain state that the message does not carry, so only
	// the costlier reading is safe. A duplicated mint priced as unique is short by exactly those
	// query fees, and the planner offers the bill with no headroom, so the transaction aborts. Set
	// false only when the series is known to be unique. The saving is a few query fees.
	bool duplicatedSeries = true;
	// How many distinct series a duplicated Phantasma mint writes into (MintPhantasmaNonFungible
	// with duplicatedSeries). The chain reads each series' supply once per transaction, not once
	// per instance, so this is the count of distinct series ids in the call. It is never more than
	// `count`. It is ignored for a unique series, which does not read the supply at all.
	uint32_t distinctSeriesCount = 1;
	// VM work-unit allowance for the Script kind. The default exceeds every script seen in mainnet
	// history (max 3392 units) with margin.
	//
	// In a Call_Multi the allowance counts once per unmodelled call, because each of them can do
	// that much work. A batch of calls the model does not price therefore offers several times what
	// it will spend. The localnet measured 4.2x for one such call and 10.1x for ten. The difference
	// is refunded, and a caller who knows the calls can bring the offer down with this field.
	uint64_t scriptUnitsAllowance = 5000;
	// Event bytes allowance for the Script kind (Notify payloads count as block data). Counted per
	// unmodelled call like scriptUnitsAllowance.
	uint32_t scriptEventBytes = 512;
	// New storage quanta allowance for the Script kind. Counted per unmodelled call like
	// scriptUnitsAllowance.
	uint32_t scriptStorageQuanta = 4;
};

// One operation of a batched message. It carries the kind the operation is priced as and the inputs
// it is priced from. See EstimateNativeFeeBatch.
struct NativeFeePart {
	NativeFeeKind kind = NativeFeeKind::Script;
	NativeFeeParams params{};
};

// The estimate inputs that belong to the transaction itself. No operation inside it owns them,
// because the block carries one envelope however many operations the message performs.
struct NativeFeeTransactionParams {
	// Full signed transaction size in bytes. Required under gas model v2.
	uint32_t envelopeBytes = 0;
	// User payload bytes attached to the tx (billed under gas model v1 only).
	uint32_t payloadBytes = 0;
};

// Result of a Tier-1 fee estimate. It holds the quote and the storage rows it was computed from.
// Gas values are kcal-base (1 KCAL = 1e10 kcal-base); escrow is in data-token atoms (1 SOUL = 1e8
// atoms).
struct NativeFeeEstimate {
	// The gas offer that covers the bill (TxMsg maxGas). The Tier-1 calculator returns the bill
	// itself, floored at the chain's minimum offer. Unused gas is refunded, so a caller who wants
	// headroom adds it on top.
	uint64_t maxGas = 0;
	// The storage-escrow ceiling (TxMsg maxData). It prices every new row at the current row price.
	uint64_t maxData = 0;
	// The bill the chain formula yields for exactly the provided inputs. It is exact for every
	// native operation when the inputs describe the transaction and the state facts are right. For
	// the Script kind it is the budgeted allowance, and the chain settles below it.
	//
	// One caveat on the word exact. The chain scales each charge as it is made and adds the
	// results. This calculator scales their sum. The two agree when feeShift is zero, and that is
	// the case on every network running the v2 model today. Under a non-zero shift the rounding
	// differs. The difference always goes one way: this calculator quotes a few units MORE than the
	// chain settles, so the offer still covers the bill.
	uint64_t expectedGasBill = 0;
	// Storage quanta the operation creates. One quantum is 1024 bytes of a new paid row.
	uint64_t newStorageQuanta = 0;
	// Storage quanta the operation deletes. Their escrow is refunded at each row's own price.
	//
	// The field is informational and does not enter the bill. maxData covers the rows an operation
	// CREATES. The block-data term uses the net growth, and an operation that deletes more than it
	// creates floors that growth at zero anyway.
	//
	// A burn's figure is a lower bound, because the stored ROM is chain state that the message does
	// not carry. Nothing depends on tightening it.
	uint64_t deletedStorageQuanta = 0;
};

namespace FeeEstimatorDetail {

constexpr uint64_t kU64Max = ~(uint64_t)0;

// Sizes of the token-module rows a native operation writes, in bytes of key plus value. A row costs
// ceil((key + value) / 1024) quanta. The small fixed rows never leave the first quantum. The
// ROM-bearing rows are computed from the ROM the caller submits.
constexpr uint64_t kNftInstanceRowOverhead = 17 + 32 + 8 + 1 + 4; // key + originator + created + flags + ROM length prefix
constexpr uint64_t kNftRamRowOverhead = 17; // key; the RAM is stored bare
constexpr uint64_t kTokenInfoKeyBytes = 9;
constexpr uint64_t kSeriesInfoKeyBytes = 13;
// The canonical ROM of a deterministic Phantasma mint holds three things: the public ROM fields,
// the `_i` id (int256, 32 bytes), and a `rom` field with the public ROM again behind a 4-byte
// length prefix.
constexpr uint64_t kPhantasmaCanonicalRomOverhead = 32 + 4;
// Fungible mint and burn calls return the resulting balance as an IntX. That is 1 header byte plus
// 8 bytes for an int64 balance, and up to 1 plus 32 for an int256 balance on a big-fungible token.
constexpr uint64_t kIntXSmallResultBytes = 9;
constexpr uint64_t kIntXBigResultBytes = 33;

// Longest name or symbol this calculator will price. The chain's length-halved policy fee is
// defined up to this length. Past it no offline price exists, so the calculator refuses. A quoted
// number could differ from what the chain charges, and shifting a uint64_t by 64 or more is
// undefined behaviour here as much as it is on the node.
constexpr uint32_t kMaxPriceableLength = 64;

struct OperationModel {
	uint64_t workUnits = 0;
	uint64_t policyFee = 0;
	// Bytes the Call returns. They are block data, like the envelope.
	uint64_t resultBytes = 0;
	uint64_t newQuanta = 0;
	uint64_t deletedQuanta = 0;
};

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

// Chain fee scaling: (value * feeMultiplier) >> feeShift with a 128-bit intermediate, saturating to
// u64. Matches the validator's v2 MulShiftSaturateU64; for sane v1 configs (live values never
// overflow 64 bits) it is also bit-identical to the v1 math.
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

// Returns the fee shift for a symbol or name length, or -1 when no honest offline price exists. The
// -1 sentinel lets no-exception builds reject the input instead of quoting a wrong fee.
inline int SymbolShift(uint32_t length, uint8_t maxLength, bool& ok)
{
	if( length == 0 )
		return 0;
	const uint32_t shift = length - 1;
	// The chain asserts that the shift stays below maxNameLength and maxTokenSymbolLength. A longer
	// input could never be admitted, so it is rejected here. Quoting a fee for an impossible
	// transaction would be worse.
	if( maxLength != 0 && shift >= (uint32_t)maxLength )
	{
		PHANTASMA_EXCEPTION("fee estimate length parameter exceeds the chain maximum");
		ok = false;
		return -1;
	}
	// The calculator refuses instead of guessing, see kMaxPriceableLength. The transaction may well
	// be admitted. This says only that no honest price can be quoted for it offline.
	if( length > kMaxPriceableLength )
	{
		PHANTASMA_EXCEPTION("fee estimate length parameter cannot be priced offline");
		ok = false;
		return -1;
	}
	return (int)shift;
}

inline uint32_t RomSizeAt(const NativeFeeParams& params, uint32_t index)
{
	return params.romBytesPerInstance ? params.romBytesPerInstance[index] : params.romBytes;
}

inline uint32_t RamSizeAt(const NativeFeeParams& params, uint32_t index)
{
	return params.ramBytesPerInstance ? params.ramBytesPerInstance[index] : params.ramBytes;
}

// Returns how many distinct series a duplicated mint touches. More series than instances is
// impossible, because at least one instance writes into every series in the call. The check turns a
// caller's bookkeeping slip into an error. Without it the slip becomes an over-offer that nobody
// notices.
inline uint32_t DistinctSeries(const NativeFeeParams& params, uint32_t count, bool& ok)
{
	const uint32_t distinct = params.distinctSeriesCount;
	if( distinct < 1 || distinct > count )
	{
		PHANTASMA_EXCEPTION("fee estimate distinctSeriesCount must be between 1 and count");
		ok = false;
		return 1;
	}
	return distinct;
}

// What burning the NFTs gives back to the burner, priced as the transfers the chain performs. A
// fungible token costs one transfer plus the owner lookup of the NFT-address source. An NFT token
// costs one instance query, one transfer per instance, and that same lookup.
//
// Rows: a returned token the burner does not hold creates a balance row. That row is paid unless
// the token is the gas or data token, and only the id tells which token it is, so an unknown id is
// priced as paid. Every returned instance moves its lookup row, and the NFT address's own rows are
// deleted.
//
// The deletions always match or exceed the creations, so the returns never add block data. They add
// work, and they add rows to the escrow ceiling.
inline OperationModel ReturnedAssets(const NativeFeeParams& params, const Blockchain::GasConfig& config, bool& ok)
{
	OperationModel model{};
	for( uint32_t i = 0; i != params.numInfusions; ++i )
	{
		const InfusedAsset& asset = params.infusions[i];
		const bool freeRows = asset.tokenId == config.gasTokenId || asset.tokenId == config.dataTokenId;
		const uint64_t balanceRow = (asset.burnerHoldsToken || freeRows) ? 0 : 1;
		if( asset.nonFungible )
		{
			if( asset.instanceCount < 1 )
			{
				PHANTASMA_EXCEPTION("fee estimate instanceCount of a returned NFT token must be positive");
				ok = false;
				return OperationModel{};
			}
			model.workUnits = SaturatingAdd(model.workUnits,
			    SaturatingAdd(SaturatingMul(config.gasFeeQuery, 2), SaturatingMul(config.gasFeeTransfer, asset.instanceCount)));
			model.newQuanta += balanceRow + asset.instanceCount;
			model.deletedQuanta += 1 + asset.instanceCount;
		}
		else
		{
			model.workUnits = SaturatingAdd(model.workUnits, SaturatingAdd(config.gasFeeTransfer, config.gasFeeQuery));
			model.newQuanta += balanceRow;
			model.deletedQuanta += freeRows ? 0 : 1;
		}
	}
	return model;
}

} // namespace FeeEstimatorDetail

// Returns the storage quanta of one row, which is ceil((key + value) / 1024).
inline uint64_t StorageQuantaFor(uint64_t rowBytes)
{
	return FeeEstimatorDetail::CeilDiv(rowBytes, StorageQuantumBytes);
}

// Returns the bytes of the NFT ROM the chain stores for a deterministic Phantasma mint, computed
// from the public ROM the caller submits. The chain builds a canonical ROM out of the public
// fields, the derived Phantasma NFT id and a second copy of the public ROM. Storage therefore grows
// at roughly twice the submitted size.
inline uint64_t PhantasmaCanonicalRomBytes(uint64_t publicRomBytes)
{
	return publicRomBytes * 2 + FeeEstimatorDetail::kPhantasmaCanonicalRomOverhead;
}

namespace FeeEstimatorDetail {

// Work units, policy fee, result bytes and row changes of each operation. A contract path makes
// state lookups internally, and each one is charged as a query fee (gasFeeQuery). Those fees are
// part of the bill even though nothing in the message mentions them.
inline OperationModel BuildOperationModel(NativeFeeKind kind, const Blockchain::GasConfig& config, const NativeFeeParams& params, uint32_t count, bool& ok)
{
	const bool v2 = config.HasGasModelV2();
	const bool freeBalanceRows = params.tokenId == config.gasTokenId || params.tokenId == config.dataTokenId;
	const uint64_t recipientRow = (params.recipientHoldsToken || freeBalanceRows) ? 0 : 1;
	const uint64_t burntRow = (params.tokenBurnedBefore || freeBalanceRows) ? 0 : 1;
	// The supply-tracking row a mint or burn may have to recreate, see NativeFeeParams. Transfers
	// never touch it, and a creation always writes it, so only mints and burns price it.
	const uint64_t supplyRow = (params.supplyRowExists || freeBalanceRows) ? 0 : 1;
	const uint64_t balanceResultBytes = params.bigFungible ? kIntXBigResultBytes : kIntXSmallResultBytes;
	const uint64_t infusionQuery = params.toIsNftAddress ? config.gasFeeQuery : 0;

	OperationModel model{};
	switch( kind )
	{
	case NativeFeeKind::TransferFungible:
		model.workUnits = SaturatingAdd(config.gasFeeTransfer, infusionQuery);
		model.newQuanta = recipientRow;
		return model;

	case NativeFeeKind::TransferNonFungible:
		// Per instance, the owner's lookup row is deleted and the recipient's one is created. The
		// recipient's balance row may be new as well.
		model.workUnits = SaturatingAdd(SaturatingMul(config.gasFeeTransfer, count), infusionQuery);
		model.newQuanta = (uint64_t)count + recipientRow;
		model.deletedQuanta = count;
		return model;

	case NativeFeeKind::MintFungible:
		model.workUnits = SaturatingAdd(config.gasFeeTransfer, infusionQuery);
		model.resultBytes = balanceResultBytes;
		model.newQuanta = recipientRow + supplyRow;
		return model;

	case NativeFeeKind::BurnFungible:
		model.workUnits = config.gasFeeTransfer;
		model.resultBytes = balanceResultBytes;
		model.newQuanta = burntRow + supplyRow;
		return model;

	case NativeFeeKind::MintNonFungible: {
		// Per instance the mint writes the instance row with the ROM, the owner row and the lookup
		// row. It writes the RAM row when RAM is given, and the meta-id row when the ROM carries
		// `_i`. On top of that come the recipient's balance row and the supply row when it must be
		// recreated.
		uint64_t quanta = recipientRow + supplyRow;
		for( uint32_t i = 0; i != count; ++i )
		{
			quanta += StorageQuantaFor(kNftInstanceRowOverhead + RomSizeAt(params, i)) + 2;
			const uint32_t ram = RamSizeAt(params, i);
			if( ram > 0 )
				quanta += StorageQuantaFor(kNftRamRowOverhead + ram);
			if( params.romHasMetaId )
				quanta += 1;
		}
		model.workUnits = SaturatingAdd(SaturatingMul(config.gasFeeTransfer, count), infusionQuery);
		model.resultBytes = 4 + 8ull * count; // instance count + one u64 instance id each
		model.newQuanta = quanta;
		return model;
	}

	case NativeFeeKind::MintPhantasmaNonFungible: {
		// The same rows as MintNonFungible. The stored ROM is the canonical one and the meta-id row
		// is always present.
		uint64_t quanta = recipientRow + supplyRow;
		for( uint32_t i = 0; i != count; ++i )
		{
			quanta += StorageQuantaFor(kNftInstanceRowOverhead + PhantasmaCanonicalRomBytes(RomSizeAt(params, i))) + 3;
			const uint32_t ram = RamSizeAt(params, i);
			if( ram > 0 )
				quanta += StorageQuantaFor(kNftRamRowOverhead + ram);
		}
		// Per instance the chain charges the mint itself, the series lookup by meta id, and the
		// token-info read that the series-mode check performs.
		//
		// A duplicated series reads the token info a SECOND time per instance, to pick up the
		// series' shared ROM. It also reads that series' supply once per distinct series in the
		// call. The chain remembers a supply it has already read, so the supply fee does not scale
		// with the instance count the way the other three do.
		const uint64_t queriesPerInstance = params.duplicatedSeries ? 3 : 2;
		const uint64_t seriesSupplyQueries = params.duplicatedSeries ? DistinctSeries(params, count, ok) : 0;
		if( !ok )
			return OperationModel{};
		model.workUnits = SaturatingAdd(
		    SaturatingAdd(SaturatingMul(SaturatingAdd(config.gasFeeTransfer, SaturatingMul(config.gasFeeQuery, queriesPerInstance)), count),
		        SaturatingMul(config.gasFeeQuery, seriesSupplyQueries)),
		    infusionQuery);
		model.resultBytes = 4 + 40ull * count; // instance count + (32-byte Phantasma id + u64 instance id) each
		model.newQuanta = quanta;
		return model;
	}

	case NativeFeeKind::BurnNonFungible: {
		// The instance, owner and lookup rows are deleted and refunded, and so are the RAM and
		// meta-id rows when they exist. The burnt counter row is created on the token's first burn,
		// and the supply row when it must be recreated.
		//
		// Each instance's infusion sweep reads the NFT address balances twice. Whatever the sweep
		// finds is returned to the burner and charged as the transfers it takes, see ReturnedAssets.
		//
		// The deleted rows mirror what the mint created, and that is why the meta-id row is counted
		// the same way here. On a burn this total is reported and never billed, see
		// deletedStorageQuanta.
		uint64_t deleted = 0;
		for( uint32_t i = 0; i != count; ++i )
		{
			deleted += StorageQuantaFor(kNftInstanceRowOverhead + RomSizeAt(params, i)) + 2;
			const uint32_t ram = RamSizeAt(params, i);
			if( ram > 0 )
				deleted += StorageQuantaFor(kNftRamRowOverhead + ram);
			if( params.romHasMetaId )
				deleted += 1;
		}
		const OperationModel returned = ReturnedAssets(params, config, ok);
		if( !ok )
			return OperationModel{};
		model.workUnits = SaturatingAdd(
		    SaturatingMul(SaturatingAdd(config.gasFeeTransfer, SaturatingMul(config.gasFeeQuery, 2)), count), returned.workUnits);
		model.newQuanta = burntRow + supplyRow + returned.newQuanta;
		model.deletedQuanta = deleted + returned.deletedQuanta;
		return model;
	}

	case NativeFeeKind::CreateToken: {
		const int shift = SymbolShift(params.symbolLength, config.maxTokenSymbolLength, ok);
		if( !ok )
			return OperationModel{};
		const bool hasSymbol = params.symbolLength > 0;
		// The creation writes the symbol lookup row, the token info row and the null-address supply
		// row. An NFT token adds the series counter. `pre_burn` adds the burnt counter. An
		// inflation schedule adds the next-inflation row.
		model.newQuanta = (hasSymbol ? 1 : 0) + StorageQuantaFor(kTokenInfoKeyBytes + params.tokenInfoBytes) + 1 +
		                  (params.nonFungible ? 1 : 0) + (params.hasPreBurn ? 1 : 0) + (params.hasInflationSchedule ? 1 : 0);
		const uint64_t base = v2 ? config.policyFeeCreateTokenBase : config.gasFeeCreateTokenBase;
		const uint64_t symbol = hasSymbol ? ((v2 ? config.policyFeeCreateTokenSymbol : config.gasFeeCreateTokenSymbol) >> shift) : 0;
		// Validating the metadata looks up a staking organisation it names and reads a reward token
		// it names. Each costs one query fee, on top of the policy fee.
		const uint64_t metadataQueries =
		    SaturatingMul(config.gasFeeQuery, (params.hasStakingOrganisation ? 1 : 0) + (params.hasStakingRewardToken ? 1 : 0));
		model.workUnits = SaturatingAdd(v2 ? 0 : SaturatingAdd(base, symbol), metadataQueries);
		model.policyFee = v2 ? SaturatingAdd(base, symbol) : 0;
		model.resultBytes = 8; // the new token id, u64
		return model;
	}

	case NativeFeeKind::CreateTokenSeries:
		// The call writes the series info row and the series supply row. The metadata adds the
		// meta-id lookup row when it has `_i`.
		model.workUnits = v2 ? 0 : config.gasFeeCreateTokenSeries;
		model.policyFee = v2 ? config.policyFeeCreateTokenSeries : 0;
		model.resultBytes = 4; // the new series id, u32
		model.newQuanta = StorageQuantaFor(kSeriesInfoKeyBytes + params.seriesInfoBytes) + 1 + (params.seriesHasMetaId ? 1 : 0);
		return model;

	case NativeFeeKind::RegisterName: {
		if( params.nameLength == 0 )
		{
			PHANTASMA_EXCEPTION("fee estimate nameLength is required for RegisterName");
			ok = false;
			return OperationModel{};
		}
		const int shift = SymbolShift(params.nameLength, config.maxNameLength, ok);
		if( !ok )
			return OperationModel{};
		// Governance-module rows are free data, so the two name rows escrow nothing.
		model.workUnits = v2 ? 0 : (config.gasFeeRegisterName >> shift);
		model.policyFee = v2 ? (config.policyFeeRegisterName >> shift) : 0;
		return model;
	}

	case NativeFeeKind::Script:
		model.workUnits = params.scriptUnitsAllowance;
		model.resultBytes = params.scriptEventBytes;
		model.newQuanta = params.scriptStorageQuanta;
		return model;
	}

	PHANTASMA_EXCEPTION("unknown fee kind");
	ok = false;
	return OperationModel{};
}

// The model of one operation, with the input check that belongs to every kind. Split out so the
// single-operation and the batch entry points build their parts the same way.
inline OperationModel PartModel(const NativeFeePart& part, const Blockchain::GasConfig& config, bool& ok)
{
	if( part.params.count < 1 )
	{
		PHANTASMA_EXCEPTION("fee estimate count must be positive");
		ok = false;
		return OperationModel{};
	}
	return BuildOperationModel(part.kind, config, part.params, part.params.count, ok);
}

// Turns the operations a transaction performs into its bill, offer and escrow ceiling. One
// transaction is settled once. The work, policy fees, result bytes and rows add up. The envelope is
// counted once. The fee scaling and the minimum-bill floor apply to the total. The chain does the
// same with the counters it accumulates while the transaction runs.
inline NativeFeeEstimate Settle(const OperationModel& totals, const Blockchain::GasConfig& config, const NativeFeeTransactionParams& transaction, bool& ok)
{
	// Only the net growth of paid storage is block data. Deleted rows are refunded.
	const uint64_t workUnits = totals.workUnits;
	const uint64_t policyFee = totals.policyFee;
	const uint64_t resultBytes = totals.resultBytes;
	const uint64_t newQuanta = totals.newQuanta;
	const uint64_t netQuanta = newQuanta > totals.deletedQuanta ? newQuanta - totals.deletedQuanta : 0;

	NativeFeeEstimate result{};
	if( config.HasGasModelV2() )
	{
		if( transaction.envelopeBytes == 0 )
		{
			PHANTASMA_EXCEPTION("fee estimate envelopeBytes is required under gas model v2");
			ok = false;
			return NativeFeeEstimate{};
		}
		// v2: bill = MulShift(work + blockData * 25, mult, shift) + policyFee, floored at
		// minimumGasBill, where blockData = envelope + net storage quanta + Call result bytes.
		const uint64_t blockData = SaturatingAdd(SaturatingAdd(transaction.envelopeBytes, netQuanta), resultBytes);
		const uint64_t byteUnits = SaturatingMul(blockData, GasModelV2UnitsPerBlockDataByte);
		uint64_t bill = MulShift(SaturatingAdd(workUnits, byteUnits), config.feeMultiplier, config.feeShift);
		bill = SaturatingAdd(bill, policyFee);
		result.expectedGasBill = bill < config.minimumGasBill ? config.minimumGasBill : bill;
		result.maxGas = result.expectedGasBill < config.minimumGasOffer ? config.minimumGasOffer : result.expectedGasBill;
	}
	else
	{
		// v1: bill = (work * mult >> shift) + blockData * gasFeePerByte, where blockData = payload +
		// Call result bytes + net storage quanta. There is no envelope term and no floor. The v1
		// product prices ride the work term, see BuildOperationModel.
		const uint64_t work = MulShift(workUnits, config.feeMultiplier, config.feeShift);
		const uint64_t blockData = SaturatingAdd(SaturatingAdd(transaction.payloadBytes, resultBytes), netQuanta);
		result.expectedGasBill = SaturatingAdd(work, SaturatingMul(blockData, config.gasFeePerByte));
		// The offer has the same shape as the validator's own test-agent stdFee. It is a 2x
		// minimum-offer pad plus a flat 1 KiB block-data allowance on top of the work term.
		const uint64_t byteAllowance = blockData > 1024 ? blockData : 1024;
		result.maxGas = SaturatingAdd(SaturatingAdd(SaturatingMul(config.minimumGasOffer, 2), work),
		    SaturatingMul(byteAllowance, config.gasFeePerByte));
	}

	result.maxData = SaturatingMul(newQuanta, config.dataEscrowPerRow);
	result.newStorageQuanta = newQuanta;
	result.deletedStorageQuanta = totals.deletedQuanta;
	return result;
}

// Adds one operation into the running totals of the transaction that performs it.
inline void AddOperation(OperationModel& totals, const OperationModel& part)
{
	totals.workUnits = SaturatingAdd(totals.workUnits, part.workUnits);
	totals.policyFee = SaturatingAdd(totals.policyFee, part.policyFee);
	totals.resultBytes += part.resultBytes;
	totals.newQuanta += part.newQuanta;
	totals.deletedQuanta += part.deletedQuanta;
}

} // namespace FeeEstimatorDetail

// Returns the fee of a message that performs SEVERAL operations in one transaction. That message is
// a TxTypes Call_Multi. The chain runs its calls in a plain loop, with no per-call surcharge and no
// batch dispatch cost. It accumulates one gas bill, one result buffer and one change set over the
// whole transaction, and it bills the envelope once. A batch therefore costs the sum of its parts
// over work, policy fee, result bytes and rows, settled once.
//
// Rows are counted per part. Two parts that create the SAME row count it twice. Two burns of one
// token both count its burnt counter, and two transfers into one fresh address both count its
// balance row.
//
// Carrying "an earlier part already created it" forward is unsound in the direction that matters. A
// burn that empties a supply to exactly zero deletes the supply row again, so a later mint would be
// priced short, abort and be billed. Counting twice raises the escrow ceiling alone, and that is
// refunded.
//
// Invalid inputs raise PHANTASMA_EXCEPTION when exceptions are enabled and otherwise return an
// all-zero estimate. A zero maxGas offer can never be mistaken for a real quote. Pass `ok` to read
// the same answer as a flag, which is what a caller that has to branch on it should do.
//
// parts       - The operations the message performs, in call order.
// config      - Current chain gas config (see the getGasConfig RPC method).
// transaction - The sizes that belong to the transaction itself. No operation owns them.
// ok          - Set to false when the inputs were refused. Optional.
inline NativeFeeEstimate EstimateNativeFeeBatch(const NativeFeePart* parts, uint32_t numParts, const Blockchain::GasConfig& config, const NativeFeeTransactionParams& transaction, bool* ok = nullptr)
{
	using namespace FeeEstimatorDetail;
	bool accepted = true;
	if( ok )
		*ok = false;
	if( numParts != 0 && parts == nullptr )
	{
		PHANTASMA_EXCEPTION("fee estimate parts is required when numParts > 0");
		return NativeFeeEstimate{};
	}
	OperationModel totals{};
	for( uint32_t i = 0; i != numParts; ++i )
	{
		const OperationModel part = PartModel(parts[i], config, accepted);
		if( !accepted )
			return NativeFeeEstimate{};
		AddOperation(totals, part);
	}
	const NativeFeeEstimate result = Settle(totals, config, transaction, accepted);
	if( !accepted )
		return NativeFeeEstimate{};
	if( ok )
		*ok = true;
	return result;
}

// Returns the exact gas bill and storage escrow of one native operation.
//
// kind   - Operation kind.
// config - Current chain gas config (see the getGasConfig RPC method).
// params - The transaction's sizes and the state facts that decide row creation.
inline NativeFeeEstimate EstimateNativeFee(NativeFeeKind kind, const Blockchain::GasConfig& config, const NativeFeeParams& params = {}, bool* ok = nullptr)
{
	const NativeFeePart part{ kind, params };
	const NativeFeeTransactionParams transaction{ params.envelopeBytes, params.payloadBytes };
	return EstimateNativeFeeBatch(&part, 1, config, transaction, ok);
}

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

// Converts a completed estimateTransaction response (PhantasmaAPI::EstimateTransaction) into the
// same NativeFeeEstimate the Tier-1 calculator produces, so wallet code consumes both tiers
// identically: maxGas/maxData are the recommended ceilings and expectedGasBill is the exact
// settled bill. Returns false without touching `out` when the estimate reports wouldAbort - an
// aborted simulation has no recommendations (retry with a higher offer or fall back to the Tier-1
// estimator). This SDK favors a bool result over exceptions; check it before using `out`.
inline bool ToFeeEstimate(const rpc::EstimateTransactionResult& result, NativeFeeEstimate& out)
{
	if( result.wouldAbort )
		return false;
	out.maxGas = result.recommendedMaxGas;
	out.maxData = result.recommendedMaxData;
	out.expectedGasBill = result.gasBillKcalBase;
	return true;
}

// Envelope size (signed tx bytes as carried in the block) from a serialized unsigned message length
// and the number of signers. Use it when you hold bytes and no message. With the message in hand use
// Blockchain::EnvelopeBytes, which reads the witness layout from the message type and takes nothing
// on trust.
//
// Witness layout mirrors SignedTxMsg: native TxTypes append bare 64-byte signatures (one, or two for
// the _GasPayer variants); Call/Trade/Phantasma txs append a length-prefixed witness array (32-byte
// address + 64-byte signature per entry).
//
// witnessCount has no default on purpose. A fee kind does not tell a _GasPayer message, which
// carries two signatures, from the plain form that carries one. Both are the same kind. A default of
// one would size a two-signature envelope as a one-signature envelope and under-offer the
// transaction by 64 bytes.
inline uint32_t EnvelopeBytesFor(NativeFeeKind kind, uint32_t serializedMessageLength, uint32_t witnessCount)
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
		// CreateToken, CreateTokenSeries, MintPhantasmaNonFungible and RegisterName ride
		// TxTypes::Call. Script rides TxTypes::Phantasma. Both carry the witness array form.
		return serializedMessageLength + 4 + WitnessArrayEntryBytes * witnessCount;
	}
}

} // namespace phantasma::carbon
