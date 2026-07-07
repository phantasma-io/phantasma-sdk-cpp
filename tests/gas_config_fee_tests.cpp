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
		params.freshRows = 0;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferFungible, LiveV1GasConfig(), params);
		Report(ctx,
		    estimate.expectedGasBill == 100000 && estimate.maxGas == 10 * 2 + 100000 + 1024ull * 250000 &&
		        estimate.maxData == 0,
		    "v1 transfer to an existing recipient bills work only");
	}

	// v1 transfer worst case (default 1 fresh row): the row quantum joins the byte fee and the
	// escrow shows up in maxData at the v1 price.
	{
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferFungible, LiveV1GasConfig());
		Report(ctx, estimate.expectedGasBill == 100000 + 250000 && estimate.maxData == 2,
		    "v1 transfer defaults include one fresh row");
	}

	// v2 transfer, default envelope 512 + 1 fresh row: blockData 513 -> 12825 byte units + 10
	// work units = 12835 units * 10000 = 128350000 kcal-base (above the 1e7 floor).
	{
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferFungible, V2GasConfig());
		Report(ctx,
		    estimate.expectedGasBill == 128350000 && estimate.maxGas == 128350000 + 128350000 / 4 &&
		        estimate.maxData == 200000,
		    "v2 transfer default envelope bill");
	}

	// v2 exact envelope: a measured 250-byte native transfer to an existing recipient bills
	// (10 + 250*25) * 10000.
	{
		NativeFeeParams params{};
		params.envelopeBytes = 250;
		params.freshRows = 0;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferFungible, V2GasConfig(), params);
		Report(ctx, estimate.expectedGasBill == (10 + 250 * 25) * 10000ull, "v2 transfer exact envelope bill");
	}

	// A tiny v2 tx can never bill below the consensus floor; the offer must also respect the
	// admission check maxGas >= minimumGasBill.
	{
		Blockchain::GasConfig config = V2GasConfig();
		config.minimumGasBill = 10000000000ull; // exaggerated floor above the computed bill
		NativeFeeParams params{};
		params.envelopeBytes = 250;
		params.freshRows = 0;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferFungible, config, params);
		Report(ctx, estimate.expectedGasBill == 10000000000ull && estimate.maxGas >= 10000000000ull,
		    "v2 floor applies to small bills");
	}

	// NFT transfers scale the work term per instance; under v2 each instance also recreates its
	// lookup row, so the escrow allowance is (count + 1) rows.
	{
		NativeFeeParams params{};
		params.count = 5;
		params.envelopeBytes = 300;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferNonFungible, V2GasConfig(), params);
		// work 5*10 units + bytes (300 envelope + 6 rows) * 25 units, all * 10000.
		Report(ctx, estimate.expectedGasBill == (50 + 306 * 25) * 10000ull && estimate.maxData == 6 * 200000ull,
		    "v2 NFT multi transfer scales units and rows");
	}

	// CreateToken: v1 charges unit-priced product fees through the multiplier; v2 pays the
	// direct kcal-base policy fee (no multiplier) plus the byte fee for its envelope.
	{
		NativeFeeParams params{};
		params.symbolLength = 4;
		params.freshRows = 0;
		params.envelopeBytes = 1000;
		const NativeFeeEstimate v1 = EstimateNativeFee(NativeFeeKind::CreateToken, LiveV1GasConfig(), params);
		const NativeFeeEstimate v2 = EstimateNativeFee(NativeFeeKind::CreateToken, V2GasConfig(), params);
		const uint64_t v1Want = (10000000000ull + 1250000000ull) * 10000;
		const uint64_t v2Want = (100000000000000ull + (100000000000000ull >> 3)) + 1000ull * 25 * 10000;
		Report(ctx, v1.expectedGasBill == v1Want && v2.expectedGasBill == v2Want,
		    "CreateToken bills unit fee under v1 and policy fee under v2");
	}

	// RegisterName halves the price per character after the first, under both models.
	{
		NativeFeeParams params{};
		params.nameLength = 8;
		params.freshRows = 0;
		params.envelopeBytes = 300;
		const NativeFeeEstimate v1 = EstimateNativeFee(NativeFeeKind::RegisterName, LiveV1GasConfig(), params);
		const NativeFeeEstimate v2 = EstimateNativeFee(NativeFeeKind::RegisterName, V2GasConfig(), params);
		Report(ctx,
		    v1.expectedGasBill == (10000000000000ull >> 7) * 10000 &&
		        v2.expectedGasBill == (100000000000000000ull >> 7) + 300ull * 25 * 10000,
		    "RegisterName length discount under both models");
	}

	// The Script kind budgets a generous VM unit allowance (default 5000 exceeds every script
	// in mainnet history) instead of pretending opcode costs are closed-form.
	{
		NativeFeeParams params{};
		params.envelopeBytes = 568;
		params.freshRows = 0;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::Script, V2GasConfig(), params);
		// (5000 vm units + (568 + 512 events) * 25) * 10000
		Report(ctx, estimate.expectedGasBill == (5000 + 1080 * 25) * 10000ull, "Script kind budgets a VM allowance");
	}

	// Envelope arithmetic mirrors SignedTxMsg: native kinds append bare 64-byte signatures,
	// call/script kinds append a length-prefixed 96-byte witness array.
	{
		Report(ctx,
		    EnvelopeBytesFor(NativeFeeKind::TransferFungible, 150) == 150 + 64 &&
		        EnvelopeBytesFor(NativeFeeKind::TransferFungible, 150, 2) == 150 + 128 &&
		        EnvelopeBytesFor(NativeFeeKind::CreateToken, 900) == 900 + 4 + 96 &&
		        EnvelopeBytesFor(NativeFeeKind::Script, 500, 2) == 500 + 4 + 192,
		    "Envelope bytes follow the witness layout");
	}

	// Guard rails: impossible inputs must never come back as a plausible quote. In builds with
	// exceptions enabled EstimateNativeFee raises PHANTASMA_EXCEPTION; in the default
	// no-exception configuration it returns the all-zero estimate. Accept either signal.
	{
		auto rejects = [&](NativeFeeKind kind, const NativeFeeParams& params)
		{
			try
			{
				const NativeFeeEstimate estimate = EstimateNativeFee(kind, LiveV1GasConfig(), params);
				return estimate.maxGas == 0 && estimate.expectedGasBill == 0;
			}
			catch( const std::exception& )
			{
				return true;
			}
		};
		NativeFeeParams noName{}; // RegisterName without nameLength
		NativeFeeParams longSymbol{};
		longSymbol.symbolLength = 11; // maxTokenSymbolLength is 10
		Report(ctx,
		    rejects(NativeFeeKind::RegisterName, noName) && rejects(NativeFeeKind::CreateToken, longSymbol),
		    "Invalid estimator inputs are rejected");
	}

	// feeShift semantics: the chain clamps shifts >= 64 to a zero work delta; the estimator
	// must match rather than undercharge/overcharge.
	{
		Blockchain::GasConfig config = LiveV1GasConfig();
		config.feeShift = 64;
		NativeFeeParams params{};
		params.freshRows = 0;
		const NativeFeeEstimate estimate = EstimateNativeFee(NativeFeeKind::TransferFungible, config, params);
		Report(ctx, estimate.expectedGasBill == 0, "Oversized feeShift zeroes scaled terms");
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
