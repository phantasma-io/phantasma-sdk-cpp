#include "test_cases.h"

#include "../include/Carbon/FeeEstimator.h"

namespace testcases {
using namespace testutil;

// Wire-format tests for the gas-model-v2 GasConfig extension plus the Tier-1 fee calculator.
// The chain serializes the 10 v2 config fields only for version >= 1; the version-0 image is
// frozen forever (historical replay). Expected fee numbers are hand-derived from the chain
// billing formula and pinned as constants so any formula regression fails loudly. The same
// fixtures and expectations exist in every SDK (parity suite).

namespace {

// The mainnet v1 values (feeMultiplier 10000, transfer 10 units, byte fee 250000, escrow 2).
Blockchain::GasConfig LiveV1GasConfig()
{
	Blockchain::GasConfig c{};
	c.version = 0;
	c.maxNameLength = 32;
	c.maxTokenSymbolLength = 10;
	c.feeShift = 0;
	c.maxStructureSize = 65536;
	c.feeMultiplier = 10000;
	c.gasTokenId = 2;
	c.dataTokenId = 1;
	c.minimumGasOffer = 10;
	c.dataEscrowPerRow = 2;
	c.gasFeeTransfer = 10;
	c.gasFeeQuery = 2;
	c.gasFeeCreateTokenBase = 10000000000ull;
	c.gasFeeCreateTokenSymbol = 10000000000ull;
	c.gasFeeCreateTokenSeries = 2500000000ull;
	c.gasFeePerByte = 250000;
	c.gasFeeRegisterName = 10000000000000ull;
	c.gasBurnRatioMul = 1;
	c.gasBurnRatioShift = 0;
	return c;
}

// The spec activation-package values for the v2 tail.
Blockchain::GasConfig V2GasConfig()
{
	Blockchain::GasConfig c = LiveV1GasConfig();
	c.version = 1;
	c.dataEscrowPerRow = 200000;
	c.minimumGasBill = 10000000;
	c.policyFeeCreateTokenBase = 100000000000000ull;
	c.policyFeeCreateTokenSymbol = 100000000000000ull;
	c.policyFeeCreateTokenSeries = 25000000000000ull;
	c.policyFeeRegisterName = 100000000000000000ull;
	c.legacyDataEscrowPerRow = 2;
	return c;
}

bool ReadGasConfig(const ByteArray& bytes, Blockchain::GasConfig& out)
{
	ReadView r((void*)bytes.data(), bytes.size());
	return Blockchain::Read(out, r);
}

// A transfer whose recipient already holds the token and whose envelope is measured. This is the
// shape most cases below start from: it creates no row, so each test adds exactly the one input it
// is about.
NativeFeeParams SettledTransfer(uint32_t envelopeBytes)
{
	NativeFeeParams params{};
	params.envelopeBytes = envelopeBytes;
	params.recipientHoldsToken = true;
	return params;
}

// The bill of a v2 transaction: (work units + block data bytes * 25) * feeMultiplier, with
// feeShift 0 and feeMultiplier 10000 in these fixtures.
uint64_t V2Bill(uint64_t workUnits, uint64_t blockDataBytes)
{
	return (workUnits + blockDataBytes * GasModelV2UnitsPerBlockDataByte) * 10000ull;
}

// True when the calculator refused the inputs. It reports the refusal through the ok flag, and in
// builds with exceptions enabled it throws instead.
bool Rejects(NativeFeeKind kind, const Blockchain::GasConfig& config, const NativeFeeParams& params)
{
	try
	{
		bool ok = false;
		const NativeFeeEstimate estimate = EstimateNativeFee(kind, config, params, &ok);
		return !ok && estimate.maxGas == 0 && estimate.expectedGasBill == 0;
	}
	catch( const std::exception& )
	{
		return true;
	}
}

// The gas config of the localnet the recorded matrix ran against. It differs from the mainnet
// package in the split ratios and in dataEscrowPerRow, neither of which enters a bill.
Blockchain::GasConfig LocalnetV2GasConfig()
{
	Blockchain::GasConfig c{};
	c.version = 1;
	c.maxNameLength = 255;
	c.maxTokenSymbolLength = 255;
	c.feeShift = 0;
	c.maxStructureSize = 1048576;
	c.feeMultiplier = 10000;
	c.gasTokenId = 1;
	c.dataTokenId = 2;
	c.minimumGasOffer = 10;
	c.dataEscrowPerRow = 50000;
	c.gasFeeTransfer = 10;
	c.gasFeeQuery = 10;
	c.gasFeeCreateTokenBase = 10000000000ull;
	c.gasFeeCreateTokenSymbol = 10000000000ull;
	c.gasFeeCreateTokenSeries = 2500000000ull;
	c.gasFeePerByte = 250000;
	c.gasFeeRegisterName = 10000000000000ull;
	c.gasBurnRatioMul = 1;
	c.gasBurnRatioShift = 1;
	c.minimumGasBill = 10000000;
	c.gasProducerRatioMul = 1;
	c.gasProducerRatioShift = 2;
	c.gasDappRatioMul = 1;
	c.gasDappRatioShift = 3;
	c.policyFeeCreateTokenBase = 100000000000ull;
	c.policyFeeCreateTokenSymbol = 100000000000ull;
	c.policyFeeCreateTokenSeries = 25000000000ull;
	c.policyFeeRegisterName = 1000000000000ull;
	c.legacyDataEscrowPerRow = 2;
	return c;
}

} // namespace

void RunGasConfigFeeTests(TestContext& ctx)
{
	// Version-0 configs must keep the exact pre-v2 wire size (113 bytes); any growth would
	// corrupt every historical block image.
	{
		const ByteArray bytes = CarbonSerialize(LiveV1GasConfig());
		Report(ctx, bytes.size() == 113, "GasConfig v0 keeps the legacy 113-byte layout",
		    std::to_string(bytes.size()));
	}

	// A version>=1 config appends the 66-byte v2 tail (8x u64 + 2x u8) after an unchanged head
	// encoding - the tail is a pure wire extension, it must not disturb the first 113 bytes.
	{
		const ByteArray v2Bytes = CarbonSerialize(V2GasConfig());
		Blockchain::GasConfig v0Twin = V2GasConfig();
		v0Twin.version = 0; // same head values, version-0 layout
		const ByteArray v0Bytes = CarbonSerialize(v0Twin);
		bool headMatches = v2Bytes.size() == 179 && v0Bytes.size() == 113 && v2Bytes[0] == 1 && v0Bytes[0] == 0;
		for( size_t i = 1; headMatches && i < 113; ++i )
			headMatches = v2Bytes[i] == v0Bytes[i];
		Report(ctx, headMatches, "GasConfig v2 appends the 66-byte tail after an unchanged head");
	}

	// Roundtrip preserves every v2 field; reading a v0 image into a dirty instance zeroes them.
	{
		Blockchain::GasConfig decoded{};
		const bool ok = ReadGasConfig(CarbonSerialize(V2GasConfig()), decoded);
		Report(ctx,
		    ok && decoded.HasGasModelV2() && decoded.dataEscrowPerRow == 200000 &&
		        decoded.minimumGasBill == 10000000 && decoded.policyFeeCreateTokenBase == 100000000000000ull &&
		        decoded.policyFeeRegisterName == 100000000000000000ull && decoded.legacyDataEscrowPerRow == 2,
		    "GasConfig v2 roundtrip preserves all fields");

		Blockchain::GasConfig dirty = V2GasConfig(); // nonzero v2 fields
		const bool okV0 = ReadGasConfig(CarbonSerialize(LiveV1GasConfig()), dirty);
		Report(ctx,
		    okV0 && !dirty.HasGasModelV2() && dirty.minimumGasBill == 0 && dirty.policyFeeCreateTokenBase == 0 &&
		        dirty.legacyDataEscrowPerRow == 0,
		    "GasConfig v0 read zeroes the v2 fields");
	}

	// A version>=1 image truncated to the version-0 length must FAIL to parse (Read returns
	// false), never silently produce a config with zeroed v2 prices (that would mean free
	// product actions).
	{
		ByteArray truncated = CarbonSerialize(V2GasConfig());
		truncated.resize(113);
		Blockchain::GasConfig out{};
		Report(ctx, !ReadGasConfig(truncated, out), "GasConfig truncated v2 image fails to parse");
	}

	// v1 transfer with an existing recipient row: bill is the pure work term 10 * 10000; the
	// offer mirrors the validator's test-agent stdFee shape (2x min offer + work + 1 KiB
	// byte allowance).
	{
		NativeFeeParams params{};
		params.recipientHoldsToken = true;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferFungible, LiveV1GasConfig(), params);
		Report(ctx,
		    estimate.expectedGasBill == 100000 && estimate.maxGas == 10 * 2 + 100000 + 1024ull * 250000 &&
		        estimate.maxData == 0,
		    "v1 transfer to an existing recipient bills work only");
	}

	// v1 transfer with nothing stated: the recipient's balance row is priced as new, so its
	// quantum joins the byte fee and its escrow shows up in maxData at the v1 price.
	{
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferFungible, LiveV1GasConfig());
		Report(ctx, estimate.expectedGasBill == 100000 + 250000 && estimate.maxData == 2,
		    "v1 transfer defaults price one fresh row");
	}

	// v2 transfer, measured envelope, settled recipient: the whole bill is the envelope and the
	// transfer fee. The offer IS the bill - unused gas is refunded, so no headroom is invented.
	{
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferFungible, V2GasConfig(), SettledTransfer(250));
		Report(ctx,
		    estimate.expectedGasBill == V2Bill(10, 250) && estimate.maxGas == estimate.expectedGasBill &&
		        estimate.maxData == 0 && estimate.newStorageQuanta == 0,
		    "v2 transfer with a settled recipient bills the envelope and the transfer");
	}

	// A recipient who does not hold the token yet adds one paid row. The row is a byte of block
	// data as well as an escrow.
	{
		NativeFeeParams params{};
		params.envelopeBytes = 250;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferFungible, V2GasConfig(), params);
		Report(ctx,
		    estimate.expectedGasBill == V2Bill(10, 250 + 1) && estimate.maxData == 200000 &&
		        estimate.newStorageQuanta == 1,
		    "v2 transfer to a fresh recipient escrows one row");
	}

	// Balance rows of the chain's gas and data tokens are free. With the token id known the
	// estimate escrows nothing for them, however little the caller states about the recipient.
	{
		NativeFeeParams params{};
		params.envelopeBytes = 250;
		params.tokenId = V2GasConfig().gasTokenId;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferFungible, V2GasConfig(), params);
		Report(ctx, estimate.expectedGasBill == V2Bill(10, 250) && estimate.maxData == 0,
		    "gas-token balance rows are free");
	}

	// A transfer into an NFT-derived address is an infusion: the chain reads that NFT's owner and
	// charges one query fee for the lookup.
	{
		NativeFeeParams params = SettledTransfer(250);
		params.toIsNftAddress = true;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferFungible, V2GasConfig(), params);
		Report(ctx, estimate.expectedGasBill == V2Bill(10 + 2, 250), "an infusion pays the owner lookup");
	}

	// Under v2 every envelope byte is billed, so an estimate without the measured size would be
	// wrong by the whole envelope. The calculator refuses instead of inventing a default.
	{
		NativeFeeParams params{};
		params.recipientHoldsToken = true;
		Report(ctx, Rejects(NativeFeeKind::TransferFungible, V2GasConfig(), params),
		    "v2 refuses an estimate without the envelope size");
	}

	// A tiny v2 tx can never bill below the consensus floor; the offer must also respect the
	// admission check maxGas >= minimumGasBill.
	{
		Blockchain::GasConfig config = V2GasConfig();
		config.minimumGasBill = 10000000000ull; // exaggerated floor above the computed bill
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferFungible, config, SettledTransfer(250));
		Report(ctx, estimate.expectedGasBill == 10000000000ull && estimate.maxGas >= 10000000000ull,
		    "v2 floor applies to small bills");
	}

	// NFT transfers scale the work term per instance. Per instance the sender's lookup row is
	// deleted and the recipient's is created, so only the recipient's new balance row grows the
	// block; the escrow ceiling still covers every row the transfer creates.
	{
		NativeFeeParams params{};
		params.count = 5;
		params.envelopeBytes = 300;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferNonFungible, V2GasConfig(), params);
		Report(ctx,
		    estimate.expectedGasBill == V2Bill(5 * 10, 300 + 1) && estimate.maxData == 6 * 200000ull &&
		        estimate.newStorageQuanta == 6 && estimate.deletedStorageQuanta == 5,
		    "v2 NFT multi transfer counts only the net row growth");
	}

	// A fungible mint or burn answers with the resulting balance as a variable-length integer.
	// Unstated, the model prices the widest answer; an ordinary int64 token is 24 bytes cheaper.
	{
		NativeFeeParams wide{};
		wide.envelopeBytes = 250;
		wide.recipientHoldsToken = true;
		wide.supplyRowExists = true;
		NativeFeeParams narrow = wide;
		narrow.bigFungible = false;
		const NativeFeeEstimate bigger = EstimateNativeFee(NativeFeeKind::MintFungible, V2GasConfig(), wide);
		const NativeFeeEstimate smaller = EstimateNativeFee(NativeFeeKind::MintFungible, V2GasConfig(), narrow);
		Report(ctx,
		    bigger.expectedGasBill == V2Bill(10, 250 + 33) && smaller.expectedGasBill == V2Bill(10, 250 + 9),
		    "a fungible mint bills the balance it returns");
	}

	// The supply-tracking row is dropped when a balance reaches exactly zero, so a mint or a burn
	// may have to recreate it. Unstated, both price the recreation.
	{
		NativeFeeParams params{};
		params.envelopeBytes = 250;
		params.bigFungible = false;
		params.tokenBurnedBefore = true;
		const NativeFeeEstimate assumed = EstimateNativeFee(NativeFeeKind::BurnFungible, V2GasConfig(), params);
		params.supplyRowExists = true;
		const NativeFeeEstimate stated = EstimateNativeFee(NativeFeeKind::BurnFungible, V2GasConfig(), params);
		Report(ctx,
		    assumed.expectedGasBill == V2Bill(10, 250 + 9 + 1) && assumed.maxData == 200000 &&
		        stated.expectedGasBill == V2Bill(10, 250 + 9) && stated.maxData == 0,
		    "a burn prices the supply row it may recreate");
	}

	// A token's first burn creates its burnt counter row.
	{
		NativeFeeParams params{};
		params.envelopeBytes = 250;
		params.bigFungible = false;
		params.supplyRowExists = true;
		const NativeFeeEstimate first = EstimateNativeFee(NativeFeeKind::BurnFungible, V2GasConfig(), params);
		params.tokenBurnedBefore = true;
		const NativeFeeEstimate again = EstimateNativeFee(NativeFeeKind::BurnFungible, V2GasConfig(), params);
		Report(ctx, first.newStorageQuanta == 1 && again.newStorageQuanta == 0,
		    "the first burn of a token creates its burnt counter");
	}

	// An NFT mint writes the instance row holding the ROM, the owner row and the lookup row per
	// instance, a RAM row where RAM is given, and the meta-id row when the ROM carries `_i`. The
	// ROM sizes may differ per instance.
	{
		const uint32_t roms[2] = { 100, 2000 };
		const uint32_t rams[2] = { 0, 40 };
		NativeFeeParams params{};
		params.envelopeBytes = 400;
		params.count = 2;
		params.recipientHoldsToken = true;
		params.supplyRowExists = true;
		params.romBytesPerInstance = roms;
		params.ramBytesPerInstance = rams;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::MintNonFungible, V2GasConfig(), params);
		// instance 0: ceil((62+100)/1024)=1 row + owner + lookup + meta-id = 4 quanta.
		// instance 1: ceil((62+2000)/1024)=3 rows + owner + lookup + meta-id + ceil((17+40)/1024)=1 = 7.
		const uint64_t quanta = 4 + 7;
		const uint64_t resultBytes = 4 + 8 * 2;
		Report(ctx,
		    estimate.newStorageQuanta == quanta && estimate.expectedGasBill == V2Bill(2 * 10, 400 + quanta + resultBytes) &&
		        estimate.maxData == quanta * 200000ull,
		    "an NFT mint sizes every instance row from its own ROM");
	}

	// The `_i` id in the ROM costs one lookup row per instance. A caller who has read the ROM
	// schema and knows there is none says so and stops paying for it.
	{
		NativeFeeParams params{};
		params.envelopeBytes = 400;
		params.count = 3;
		params.romBytes = 64;
		params.recipientHoldsToken = true;
		params.supplyRowExists = true;
		const NativeFeeEstimate assumed = EstimateNativeFee(NativeFeeKind::MintNonFungible, V2GasConfig(), params);
		params.romHasMetaId = false;
		const NativeFeeEstimate stated = EstimateNativeFee(NativeFeeKind::MintNonFungible, V2GasConfig(), params);
		Report(ctx, assumed.newStorageQuanta == stated.newStorageQuanta + 3,
		    "a ROM meta id costs one row per instance");
	}

	// A deterministic Phantasma mint stores a canonical ROM: the public ROM, the derived id, and
	// the public ROM again. It always writes the meta-id row, and it pays for the series lookup
	// and the series-mode check on top of the mint itself.
	{
		NativeFeeParams params{};
		params.envelopeBytes = 500;
		params.count = 2;
		params.romBytes = 100;
		params.recipientHoldsToken = true;
		params.supplyRowExists = true;
		params.duplicatedSeries = false;
		const NativeFeeEstimate unique = EstimateNativeFee(NativeFeeKind::MintPhantasmaNonFungible, V2GasConfig(), params);
		// Per instance: ceil((62 + 100*2 + 36)/1024) = 1 row + owner + lookup + meta id = 4 quanta.
		const uint64_t quanta = 2 * 4;
		const uint64_t resultBytes = 4 + 40 * 2;
		Report(ctx,
		    unique.newStorageQuanta == quanta &&
		        unique.expectedGasBill == V2Bill(2 * (10 + 2 * 2), 500 + quanta + resultBytes),
		    "a Phantasma mint stores the canonical ROM and pays two lookups per instance");

		params.duplicatedSeries = true;
		params.distinctSeriesCount = 2;
		const NativeFeeEstimate duplicated = EstimateNativeFee(NativeFeeKind::MintPhantasmaNonFungible, V2GasConfig(), params);
		// A duplicated series reads the token info once more per instance, and each distinct
		// series' supply once for the whole transaction.
		Report(ctx,
		    duplicated.expectedGasBill == V2Bill(2 * (10 + 3 * 2) + 2 * 2, 500 + quanta + resultBytes),
		    "a duplicated series pays one more query per instance and one per series");
	}

	// The distinct series count cannot exceed the instance count: at least one instance writes
	// into every series the call names. A slip there would silently become an over-offer.
	{
		NativeFeeParams params{};
		params.envelopeBytes = 500;
		params.count = 2;
		params.distinctSeriesCount = 3;
		Report(ctx, Rejects(NativeFeeKind::MintPhantasmaNonFungible, V2GasConfig(), params),
		    "distinctSeriesCount above the instance count is rejected");
	}

	// A burn refunds the rows the mint created and charges for whatever the instance's own address
	// held. A returned fungible token costs a transfer plus the owner lookup of its source, and a
	// balance row when the burner does not hold it yet.
	{
		InfusedAsset held[2] = {};
		held[0].tokenId = 77; // a fungible token the burner does not hold
		held[1].nonFungible = true;
		held[1].tokenId = 78;
		held[1].instanceCount = 2;
		held[1].burnerHoldsToken = true;
		NativeFeeParams params{};
		params.envelopeBytes = 300;
		params.count = 1;
		params.romBytes = 100;
		params.tokenBurnedBefore = true;
		params.supplyRowExists = true;
		params.infusions = held;
		params.numInfusions = 2;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::BurnNonFungible, V2GasConfig(), params);
		// Work: the burn itself plus two address reads, then the returns: one transfer + one query
		// for the fungible asset, and two queries + two transfers for the two NFT instances.
		const uint64_t work = (10 + 2 * 2) + (10 + 2) + (2 * 2 + 2 * 10);
		// Rows created: the fungible token's balance row for the burner, plus one lookup row per
		// returned instance. Rows deleted: the burned instance's row, its owner and lookup rows and
		// its meta-id row, then the NFT address's own balance row and the two instance lookups.
		Report(ctx,
		    estimate.expectedGasBill == V2Bill(work, 300) && estimate.newStorageQuanta == 1 + 2 &&
		        estimate.deletedStorageQuanta == (1 + 2 + 1) + 1 + (1 + 2),
		    "a burn charges for every asset it returns");
	}

	// CreateToken: v1 charges unit-priced product fees through the multiplier; v2 pays the
	// direct kcal-base policy fee (no multiplier) plus the byte fee for its envelope, its rows
	// and the token id it returns.
	{
		NativeFeeParams params{};
		params.symbolLength = 4;
		params.envelopeBytes = 1000;
		params.tokenInfoBytes = 300;
		// The symbol lookup row, the token info row and the null-address supply row.
		const uint64_t quanta = 1 + 1 + 1;
		const NativeFeeEstimate v1 = EstimateNativeFee(NativeFeeKind::CreateToken, LiveV1GasConfig(), params);
		const NativeFeeEstimate v2 = EstimateNativeFee(NativeFeeKind::CreateToken, V2GasConfig(), params);
		const uint64_t v1Want = (10000000000ull + 1250000000ull) * 10000 + (8 + quanta) * 250000;
		const uint64_t v2Want = (100000000000000ull + (100000000000000ull >> 3)) + (1000 + quanta + 8) * 25 * 10000;
		Report(ctx,
		    v1.expectedGasBill == v1Want && v2.expectedGasBill == v2Want && v2.newStorageQuanta == quanta,
		    "CreateToken bills unit fee under v1 and policy fee under v2");
	}

	// The token metadata decides which extra rows the creation writes and which lookups its
	// validation costs.
	{
		NativeFeeParams params{};
		params.symbolLength = 4;
		params.envelopeBytes = 1000;
		params.tokenInfoBytes = 300;
		params.nonFungible = true;
		params.hasPreBurn = true;
		params.hasInflationSchedule = true;
		params.hasStakingOrganisation = true;
		params.hasStakingRewardToken = true;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::CreateToken, V2GasConfig(), params);
		const uint64_t quanta = 3 + 3; // the three base rows plus series counter, burnt counter, next inflation
		const uint64_t policy = 100000000000000ull + (100000000000000ull >> 3);
		Report(ctx,
		    estimate.newStorageQuanta == quanta &&
		        estimate.expectedGasBill == policy + (2 * 2 + (1000 + quanta + 8) * 25) * 10000ull,
		    "the token metadata decides the extra rows and lookups");
	}

	// CreateTokenSeries writes the series info row and the series supply row, and a meta-id row
	// when the metadata carries `_i`.
	{
		NativeFeeParams params{};
		params.envelopeBytes = 700;
		params.seriesInfoBytes = 200;
		const NativeFeeEstimate assumed = EstimateNativeFee(NativeFeeKind::CreateTokenSeries, V2GasConfig(), params);
		params.seriesHasMetaId = false;
		const NativeFeeEstimate stated = EstimateNativeFee(NativeFeeKind::CreateTokenSeries, V2GasConfig(), params);
		Report(ctx,
		    assumed.newStorageQuanta == 3 && stated.newStorageQuanta == 2 &&
		        assumed.expectedGasBill == 25000000000000ull + (700 + 3 + 4) * 25 * 10000ull,
		    "CreateTokenSeries writes two rows and prices the meta-id row it may add");
	}

	// RegisterName halves the price per character after the first, under both models. Its rows are
	// governance-module data, which is free, so nothing is escrowed.
	{
		NativeFeeParams params{};
		params.nameLength = 8;
		params.envelopeBytes = 300;
		const NativeFeeEstimate v1 = EstimateNativeFee(NativeFeeKind::RegisterName, LiveV1GasConfig(), params);
		const NativeFeeEstimate v2 = EstimateNativeFee(NativeFeeKind::RegisterName, V2GasConfig(), params);
		Report(ctx,
		    v1.expectedGasBill == (10000000000000ull >> 7) * 10000 &&
		        v2.expectedGasBill == (100000000000000000ull >> 7) + 300ull * 25 * 10000 && v2.maxData == 0,
		    "RegisterName length discount under both models");
	}

	// The Script kind budgets a generous VM unit allowance (the default exceeds every script in
	// mainnet history) instead of pretending opcode costs are closed-form.
	{
		NativeFeeParams params{};
		params.envelopeBytes = 568;
		params.scriptStorageQuanta = 0;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::Script, V2GasConfig(), params);
		Report(ctx, estimate.expectedGasBill == V2Bill(5000, 568 + 512), "Script kind budgets a VM allowance");
	}

	// A batch is the sum of its parts over work, result bytes and rows, settled once. The envelope
	// is billed once however many operations the message performs.
	{
		NativeFeePart parts[2] = {};
		parts[0].kind = NativeFeeKind::TransferFungible;
		parts[1].kind = NativeFeeKind::TransferFungible;
		const NativeFeeTransactionParams transaction{ 400, 0 };
		const NativeFeeEstimate batch = EstimateNativeFeeBatch(parts, 2, V2GasConfig(), transaction);
		Report(ctx,
		    batch.expectedGasBill == V2Bill(2 * 10, 400 + 2) && batch.maxData == 2 * 200000ull,
		    "a batch adds its parts and bills one envelope");
	}

	// Two parts that create the SAME row count it twice. Carrying "an earlier part already made
	// it" forward would price a later operation short, and a short offer aborts and is billed.
	{
		NativeFeePart single[1] = {};
		single[0].kind = NativeFeeKind::TransferFungible;
		NativeFeePart twice[2] = {};
		twice[0].kind = NativeFeeKind::TransferFungible;
		twice[1].kind = NativeFeeKind::TransferFungible;
		twice[0].params.tokenId = 77;
		twice[1].params.tokenId = 77;
		single[0].params.tokenId = 77;
		const NativeFeeTransactionParams transaction{ 400, 0 };
		const NativeFeeEstimate one = EstimateNativeFeeBatch(single, 1, V2GasConfig(), transaction);
		const NativeFeeEstimate two = EstimateNativeFeeBatch(twice, 2, V2GasConfig(), transaction);
		Report(ctx, one.newStorageQuanta == 1 && two.newStorageQuanta == 2,
		    "a batch counts a shared row once per part");
	}

	// An empty batch is a message that performs no operation. It still carries an envelope, and
	// that is all it is billed for.
	{
		const NativeFeeTransactionParams transaction{ 320, 0 };
		const NativeFeeEstimate estimate = EstimateNativeFeeBatch(nullptr, 0, V2GasConfig(), transaction);
		Report(ctx,
		    estimate.expectedGasBill == V2Bill(0, 320) && estimate.maxData == 0 && estimate.newStorageQuanta == 0,
		    "an empty batch prices its envelope alone");
	}

	// Envelope arithmetic mirrors SignedTxMsg: native kinds append bare 64-byte signatures,
	// call/script kinds append a length-prefixed 96-byte witness array.
	{
		Report(ctx,
		    EnvelopeBytesFor(NativeFeeKind::TransferFungible, 150, 1) == 150 + 64 &&
		        EnvelopeBytesFor(NativeFeeKind::TransferFungible, 150, 2) == 150 + 128 &&
		        EnvelopeBytesFor(NativeFeeKind::CreateToken, 900, 1) == 900 + 4 + 96 &&
		        EnvelopeBytesFor(NativeFeeKind::Script, 500, 2) == 500 + 4 + 192,
		    "Envelope bytes follow the witness layout");
	}

	// A row costs one quantum per 1024 bytes of key plus value, and a Phantasma mint stores the
	// public ROM twice with the derived id between them.
	{
		Report(ctx,
		    StorageQuantaFor(0) == 0 && StorageQuantaFor(1) == 1 && StorageQuantaFor(1024) == 1 &&
		        StorageQuantaFor(1025) == 2 && PhantasmaCanonicalRomBytes(100) == 236,
		    "row quanta and the canonical ROM size");
	}

	// Guard rails: impossible inputs must never come back as a plausible quote.
	{
		NativeFeeParams noName{};
		noName.envelopeBytes = 300;
		NativeFeeParams longSymbol{};
		longSymbol.envelopeBytes = 300;
		longSymbol.symbolLength = 11; // maxTokenSymbolLength is 10
		NativeFeeParams zeroCount{};
		zeroCount.envelopeBytes = 300;
		zeroCount.count = 0;
		Report(ctx,
		    Rejects(NativeFeeKind::RegisterName, V2GasConfig(), noName) &&
		        Rejects(NativeFeeKind::CreateToken, V2GasConfig(), longSymbol) &&
		        Rejects(NativeFeeKind::TransferNonFungible, V2GasConfig(), zeroCount),
		    "Invalid estimator inputs are rejected");
	}

	// The chain shifts the name and symbol price right by (length - 1), and the only bound it
	// applies is the configured maximum LENGTH, which is 255 on the public chains. A shift of 64
	// or more is undefined behaviour in C++ on the node as much as here, so no honest offline
	// price exists past 64 characters and the calculator refuses to quote one.
	{
		Blockchain::GasConfig config = V2GasConfig();
		config.maxNameLength = 255;
		NativeFeeParams priceable{};
		priceable.envelopeBytes = 300;
		priceable.nameLength = 64;
		NativeFeeParams beyond = priceable;
		beyond.nameLength = 65;
		const NativeFeeEstimate last = EstimateNativeFee(NativeFeeKind::RegisterName, config, priceable);
		Report(ctx,
		    last.expectedGasBill == (100000000000000000ull >> 63) + 300ull * 25 * 10000 &&
		        Rejects(NativeFeeKind::RegisterName, config, beyond),
		    "a name longer than 64 characters cannot be priced offline");
	}

	// feeShift semantics: the chain clamps shifts >= 64 to a zero work delta; the estimator
	// must match rather than undercharge/overcharge.
	{
		Blockchain::GasConfig config = LiveV1GasConfig();
		config.feeShift = 64;
		NativeFeeParams params{};
		params.recipientHoldsToken = true;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferFungible, config, params);
		Report(ctx, estimate.expectedGasBill == 0, "Oversized feeShift zeroes scaled terms");
	}

	// Bills MEASURED on a chain, replayed through the calculator. Each row is a settled transaction
	// from the live matrix of 2026-09-07, and the expected number is what the chain actually
	// charged. This is the cheapest check the model has: a decomposition that reproduces a
	// previously measured bill to the atom cannot have drifted.
	//
	// Rows whose plan was a bound rather than a prediction are not here. The chain settles those
	// below the quoted number by design, so they prove nothing about the arithmetic.
	{
		const Blockchain::GasConfig live = LocalnetV2GasConfig();
		struct MeasuredRow {
			const char* label;
			NativeFeeKind kind;
			NativeFeeParams params;
			uint64_t measuredBillAtoms;
		};

		NativeFeeParams kcalTransfer{};
		kcalTransfer.envelopeBytes = 170;
		kcalTransfer.tokenId = live.gasTokenId;

		NativeFeeParams kcalTransferTwoSignatures = kcalTransfer;
		kcalTransferTwoSignatures.envelopeBytes = 266;

		NativeFeeParams kcalToNftAddress = kcalTransfer;
		kcalToNftAddress.toIsNftAddress = true;

		NativeFeeParams freshHolder{};
		freshHolder.envelopeBytes = 170;
		freshHolder.tokenId = 97;

		NativeFeeParams knownHolder = freshHolder;
		knownHolder.recipientHoldsToken = true;

		NativeFeeParams mintToFreshHolder{};
		mintToFreshHolder.envelopeBytes = 171;
		mintToFreshHolder.tokenId = 97;
		mintToFreshHolder.supplyRowExists = true;
		mintToFreshHolder.bigFungible = false;

		NativeFeeParams firstBurn{};
		firstBurn.envelopeBytes = 139;
		firstBurn.tokenId = 97;
		firstBurn.supplyRowExists = true;
		firstBurn.bigFungible = false;

		NativeFeeParams burnedBefore = firstBurn;
		burnedBefore.tokenBurnedBefore = true;

		// The symbol was seven characters and the serialized TokenInfo 204 bytes; both follow from
		// the recorded envelope and bill.
		NativeFeeParams createToken{};
		createToken.envelopeBytes = 374;
		createToken.symbolLength = 7;
		createToken.tokenInfoBytes = 204;

		NativeFeeParams createTokenPreBurn{};
		createTokenPreBurn.envelopeBytes = 368;
		createTokenPreBurn.symbolLength = 7;
		createTokenPreBurn.tokenInfoBytes = 198;
		createTokenPreBurn.hasPreBurn = true;

		NativeFeeParams shortName{};
		shortName.envelopeBytes = 311;
		shortName.nameLength = 12;

		// The longest name the calculator will price. Its policy fee shifts away to nothing, which
		// is what the chain charged.
		NativeFeeParams longestName{};
		longestName.envelopeBytes = 363;
		longestName.nameLength = 64;

		// One instance with a 172-byte public ROM, minted into a unique series for a holder who did
		// not hold the token yet.
		NativeFeeParams phantasmaMint{};
		phantasmaMint.envelopeBytes = 403;
		phantasmaMint.tokenId = 9;
		phantasmaMint.count = 1;
		phantasmaMint.romBytes = 172;
		phantasmaMint.supplyRowExists = true;
		phantasmaMint.duplicatedSeries = false;

		const MeasuredRow rows[] = {
			{ "native KCAL transfer", NativeFeeKind::TransferFungible, kcalTransfer, 42600000 },
			{ "KCAL transfer, gas payer (2 sig)", NativeFeeKind::TransferFungible, kcalTransferTwoSignatures, 66600000 },
			{ "KCAL transfer -> NFT address", NativeFeeKind::TransferFungible, kcalToNftAddress, 42700000 },
			{ "token transfer -> fresh holder", NativeFeeKind::TransferFungible, freshHolder, 42850000 },
			{ "token transfer -> holder again", NativeFeeKind::TransferFungible, knownHolder, 42600000 },
			{ "MintFungible to owner (fresh row)", NativeFeeKind::MintFungible, mintToFreshHolder, 45350000 },
			{ "BurnFungible, first burn", NativeFeeKind::BurnFungible, firstBurn, 37350000 },
			{ "BurnFungible, burned before", NativeFeeKind::BurnFungible, burnedBefore, 37100000 },
			{ "CreateToken fungible", NativeFeeKind::CreateToken, createToken, 101658750000ull },
			{ "CreateToken with pre_burn", NativeFeeKind::CreateToken, createTokenPreBurn, 101657500000ull },
			{ "RegisterName (12 chars)", NativeFeeKind::RegisterName, shortName, 566031250 },
			{ "RegisterName (64 chars)", NativeFeeKind::RegisterName, longestName, 90750000 },
			{ "NFT mint, ROM 172B", NativeFeeKind::MintPhantasmaNonFungible, phantasmaMint, 113300000 },
		};

		for( const MeasuredRow& row : rows )
		{
			const NativeFeeEstimate estimate = EstimateNativeFee(row.kind, live, row.params);
			Report(ctx, estimate.expectedGasBill == row.measuredBillAtoms,
			    std::string("measured bill reproduced: ") + row.label,
			    std::to_string(estimate.expectedGasBill) + " vs " + std::to_string(row.measuredBillAtoms));
		}
	}

	// The getGasConfig JSON model converts into the wire struct for the estimator.
	{
		rpc::GasConfigData data{};
		data.version = 1;
		data.maxNameLength = 32;
		data.maxTokenSymbolLength = 10;
		data.feeMultiplier = 10000;
		data.minimumGasOffer = 10;
		data.dataEscrowPerRow = 200000;
		data.gasFeeTransfer = 10;
		data.gasFeePerByte = 250000;
		data.hasV2Fields = true;
		data.minimumGasBill = 10000000;
		data.policyFeeRegisterName = 100000000000000000ull; // > 2^53, exercises 64-bit carry
		data.legacyDataEscrowPerRow = 2;
		const Blockchain::GasConfig config = ToGasConfig(data);
		Report(ctx,
		    config.HasGasModelV2() && config.dataEscrowPerRow == 200000 && config.minimumGasBill == 10000000 &&
		        config.policyFeeRegisterName == 100000000000000000ull && config.legacyDataEscrowPerRow == 2,
		    "ToGasConfig maps the JSON model to the wire struct");
	}

	// A completed estimateTransaction response converts into the Tier-1 NativeFeeEstimate so
	// wallet code consumes both tiers identically. recommendedMaxGas > 2^53 exercises 64-bit carry.
	{
		rpc::EstimateTransactionResult result{};
		result.wouldAbort = false;
		result.gasBillKcalBase = 10000000;
		result.dataRows = 1;
		result.dataEscrowAtoms = 200000;
		result.recommendedMaxGas = 100000000000000000ull; // > 2^53
		result.recommendedMaxData = 400000;
		NativeFeeEstimate estimate{};
		const bool ok = ToFeeEstimate(result, estimate);
		Report(ctx,
		    ok && estimate.maxGas == 100000000000000000ull && estimate.maxData == 400000 &&
		        estimate.expectedGasBill == 10000000,
		    "ToFeeEstimate maps a completed estimate to the Tier-1 struct");
	}

	// An aborted estimate has no recommendations; the converter must refuse rather than hand back
	// zero ceilings a wallet could sign with.
	{
		rpc::EstimateTransactionResult result{};
		result.wouldAbort = true;
		result.abortReason = PHANTASMA_LITERAL("gas fees [gas=3125 max=40]");
		result.gasBillKcalBase = 40; // aborts still settle a bill, but the ceilings are unknown
		NativeFeeEstimate estimate{};
		const bool ok = ToFeeEstimate(result, estimate);
		Report(ctx,
		    !ok && estimate.maxGas == 0 && estimate.maxData == 0,
		    "ToFeeEstimate refuses an aborted estimate");
	}
}

} // namespace testcases
