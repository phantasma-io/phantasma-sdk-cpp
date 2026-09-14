#pragma once
#ifndef PHANTASMA_API_INCLUDED
#error "Configure and include PhantasmaAPI.h first"
#endif

#include "FeeEstimator.h"
#include "Tx.h"

namespace phantasma::carbon {

// Plans the gas offer and the storage ceiling of a message FROM THE MESSAGE ITSELF. Its type and
// contents decide the operation model, its signed size is computed with placeholder witnesses, and
// the chain config supplies the prices. Nothing here touches the network: fetch the config with
// PhantasmaAPI::GetGasConfig and pass it in.

// Arguments of the token-module calls that carry a token movement, read back out of a TxMsgCall.
//
// A wallet that batches operations sends them as module calls and not as native transaction types.
// The fee model prices the two identically. The planner therefore has to recover from the call what
// the native message would have carried in named fields: which token moves, where it goes and how
// many instances.
//
// Only the leading fields the fee model needs are read. The amount that follows them is a
// variable-length IntX, and nothing here depends on it.
//
// Each reader answers false on a buffer too short to hold its fields. A caller planning a malformed
// call then gets an error instead of a number. The chain would refuse that call anyway.
struct TokenCallArgs {
	// `Token.TransferFungible(to, from, tokenId, amount)`.
	static bool ReadTransferFungible(const ByteView& args, Bytes32& to, uint64_t& tokenId)
	{
		ReadView r((void*)args.bytes, args.length);
		Bytes32 from{};
		Read(to, r);
		Read(from, r); // the holder the tokens leave
		tokenId = Read8u(r);
		return !r.Failure();
	}

	// `Token.TransferNonFungible(to, from, tokenId, instanceIds)`.
	static bool ReadTransferNonFungible(const ByteView& args, Bytes32& to, uint64_t& tokenId, uint32_t& instanceCount)
	{
		ReadView r((void*)args.bytes, args.length);
		Bytes32 from{};
		Read(to, r);
		Read(from, r);
		tokenId = Read8u(r);
		instanceCount = (uint32_t)ReadLengthFor(r, sizeof(uint64_t));
		return !r.Failure();
	}

	// `Token.MintFungible(tokenId, to, amount)`.
	static bool ReadMintFungible(const ByteView& args, Bytes32& to, uint64_t& tokenId)
	{
		ReadView r((void*)args.bytes, args.length);
		tokenId = Read8u(r);
		Read(to, r);
		return !r.Failure();
	}

	// `Token.BurnFungible(tokenId, from, amount)`.
	static bool ReadBurnFungible(const ByteView& args, uint64_t& tokenId)
	{
		ReadView r((void*)args.bytes, args.length);
		tokenId = Read8u(r);
		return !r.Failure();
	}

	// `Token.BurnNonFungible(tokenId, address, instanceIds)`.
	static bool ReadBurnNonFungible(const ByteView& args, uint64_t& tokenId, PHANTASMA_VECTOR<uint64_t>& instanceIds)
	{
		ReadView r((void*)args.bytes, args.length);
		Bytes32 holder{};
		tokenId = Read8u(r);
		Read(holder, r); // the holder whose instances are burned
		const uint32_t count = (uint32_t)ReadLengthFor(r, sizeof(uint64_t));
		instanceIds.clear();
		for( uint32_t i = 0; i != count && !r.Failure(); ++i )
		{
			instanceIds.push_back(Read8u(r));
		}
		return !r.Failure();
	}
};

// One NFT instance a message burns.
struct BurnedInstance {
	uint64_t tokenId = 0;
	uint64_t instanceId = 0;
};

// Facts about chain state and signing that a message does not carry. The fee depends on them.
//
// Every state fact DEFAULTS to the reading that costs more. A plan that states nothing is an upper
// bound, and the settlement can only come out below it. Each default is documented on
// NativeFeeParams, which holds the same fields; they are not repeated here, so the two cannot drift
// apart.
//
// The facts the message itself carries are absent: counts, sizes, token ids, the NFT-address
// recipient, nonFungible, pre_burn. PlanFees reads those out of the message. A value from the caller
// could only contradict it.
//
// A plain bool cannot say "unstated", so a caller who states the costlier value and one who states
// nothing arrive here the same way. Both get a bound, and FeePlan::exact reads false for both.
// Stating a costlier fact buys nothing, so nothing is lost.
//
// A Call_Multi performs several operations under one set of options. The state facts describe every
// call in the batch, and `infusions` lists what ALL of its burns give back.
struct FeePlanOptions {
	// An ordinary wallet may well know these four.
	bool recipientHoldsToken = false;
	bool bigFungible = true;
	bool tokenBurnedBefore = false;
	bool supplyRowExists = false;
	// These three need the token's schema or the series' metadata. Set them only if you read them.
	bool duplicatedSeries = true;
	bool romHasMetaId = true;
	bool seriesHasMetaId = true;
	// What every burned NFT holds at its own address. It has no costlier reading, because an NFT can
	// hold any number of assets, so PlanFees demands it: a burn planned as if the addresses were
	// empty is short by every returned asset, and it then aborts and is billed on every retry.
	//
	// Set infusionsRead once the list is filled. An empty list with the flag set states that the
	// instances hold nothing.
	bool infusionsRead = false;
	const InfusedAsset* infusions = nullptr;
	uint32_t numInfusions = 0;
	// These three size the allowance for a VM script. No formula predicts what a script costs. Their
	// defaults are the ones documented on NativeFeeParams.
	uint64_t scriptUnitsAllowance = 5000;
	uint32_t scriptEventBytes = 512;
	uint32_t scriptStorageQuanta = 4;
	// How many witnesses will sign a Call / Call_Multi / Trade / Phantasma message. Required for
	// those four types and for no other.
	//
	// The caller chooses the witness set of those four, and nothing in the message says how large it
	// will be. Each witness adds 96 bytes that the chain bills. Every other type fixes its own
	// witness set, so a count that disagrees with it is refused. Zero means unstated.
	uint32_t witnessCount = 0;
};

// A fee plan for one message: the estimate, what it was computed from, and how to apply it.
struct FeePlan : NativeFeeEstimate {
	// The operations the message was recognised as, in call order. The bill was computed from them.
	// An ordinary message has one entry. A Call_Multi has one entry per inner call.
	//
	// Every kind except NativeFeeKind::Script is priced with the chain's own formula for that
	// operation. Script covers VM scripts and unmodelled calls. Their work depends on execution, so
	// they can only be budgeted (see scriptUnitsAllowance and its neighbours).
	//
	// This field says what was priced. How firm the number is, `exact` answers. That field accounts
	// for both causes: a budgeted part, and a state fact the plan had to assume.
	PHANTASMA_VECTOR<NativeFeeKind> kinds;
	// True when this bill is a prediction of the settlement. False when it is an upper bound on it.
	// A wallet showing a fee reads this one field to choose between "0.0073 KCAL" and "up to 0.0073
	// KCAL".
	//
	// The field is false in two cases. A chain-state fact was left at its costlier default and that
	// reading decided part of the price. Or some part of the message had to be budgeted.
	//
	// bigFungible = true is the one fact that claims nothing about chain state. It asks the model to
	// price the widest answer a variable-length balance can have. A plan that rests on it reports
	// false however it was arrived at.
	bool exact = false;
	// The signed size the plan was computed for. These are the bytes the block will carry.
	uint32_t envelopeBytes = 0;

	// Returns a copy of `msg` with maxGas and maxData set to this plan. The input is left untouched.
	Blockchain::TxMsg Apply(const Blockchain::TxMsg& msg) const
	{
		Blockchain::TxMsg planned = msg;
		planned.maxGas = maxGas;
		planned.maxData = maxData;
		return planned;
	}
};

namespace FeePlanDetail {

// The state facts every branch passes through. Whether the recipient is an NFT-derived address is
// NOT here: the address form decides that, and the message carries the address, so each branch reads
// it out. seriesHasMetaId and duplicatedSeries belong to one branch each and are set there.
inline NativeFeeParams StateFacts(const FeePlanOptions& options)
{
	NativeFeeParams params{};
	params.recipientHoldsToken = options.recipientHoldsToken;
	params.bigFungible = options.bigFungible;
	params.tokenBurnedBefore = options.tokenBurnedBefore;
	params.supplyRowExists = options.supplyRowExists;
	params.romHasMetaId = options.romHasMetaId;
	return params;
}

inline NativeFeePart ScriptPlan(const FeePlanOptions& options)
{
	NativeFeePart part{};
	part.kind = NativeFeeKind::Script;
	part.params.scriptUnitsAllowance = options.scriptUnitsAllowance;
	part.params.scriptEventBytes = options.scriptEventBytes;
	part.params.scriptStorageQuanta = options.scriptStorageQuanta;
	return part;
}

inline NativeFeePart DescribeAs(NativeFeeKind kind, const NativeFeeParams& params)
{
	NativeFeePart part{};
	part.kind = kind;
	part.params = params;
	return part;
}

// What the NFTs hold is chain state, and it has no costlier bound, so the planner demands it.
inline bool RequireInfusions(const FeePlanOptions& options, NativeFeeParams& params)
{
	if( !options.infusionsRead )
	{
		PHANTASMA_EXCEPTION("a burn returns whatever the NFT holds: set infusionsRead with the assets the instances carry, empty when they carry nothing");
		return false;
	}
	params.infusions = options.infusions;
	params.numInfusions = options.numInfusions;
	return true;
}

// Reads the fields of a serialized TokenInfo that decide what a CreateToken call costs: the symbol
// length, the non-fungible flag, and the metadata keys whose presence creates a row or costs a
// lookup. The row itself is the Call arguments as submitted, so its size is measured outside.
inline bool ReadCreateTokenFacts(const ByteView& args, NativeFeeParams& params)
{
	Allocator alloc;
	ReadView r((void*)args.bytes, args.length, alloc, ReadView::InPlace);
	intx maxSupply = intx::Zero();
	if( !Read(maxSupply, r) )
		return false;
	const uint8_t flags = Read1(r);
	Read1(r); // decimals
	Bytes32 owner{};
	Read(owner, r);
	SmallString symbol{};
	Read(symbol, r);
	if( r.Failure() )
		return false;
	ByteView metadata{};
	if( !ReadArray(metadata, r, alloc) || r.Failure() )
		return false;

	params.symbolLength = symbol.length;
	params.tokenInfoBytes = (uint32_t)args.length;
	params.nonFungible = (flags & TokenFlags_NonFungible) != 0;
	// The metadata decides which extra rows the creation writes and which lookups its validation
	// costs. The plan can read it, because it is a named struct.
	if( metadata.length != 0 )
	{
		ReadView mr(metadata, alloc, ReadView::InPlace);
		VmDynamicStruct fields{};
		if( !Read(fields, mr, alloc) )
			return false;
		params.hasPreBurn = fields[StandardMeta::Token::pre_burn] != nullptr;
		params.hasInflationSchedule = fields[StandardMeta::Token::inflation_period] != nullptr;
		params.hasStakingOrganisation = fields[StandardMeta::Token::staking_org_id] != nullptr;
		params.hasStakingRewardToken = fields[StandardMeta::Token::staking_reward_token] != nullptr;
	}
	return true;
}

// Reads the arguments of a deterministic Phantasma mint: the token id, the recipient, and one entry
// per minted instance carrying its series id, its public ROM and its RAM.
//
// Each instance names the series it is minted into, so the number of DISTINCT series a duplicated
// mint touches is readable from the call and the caller never has to supply it. The chain charges
// its per-series supply reads once per distinct series. The ids are compared one by one against the
// ones already seen, which is enough for the instance count of a single call.
inline bool ReadPhantasmaMintFacts(const ByteView& args, NativeFeeParams& params, std::vector<uint32_t>& roms, std::vector<uint32_t>& rams)
{
	Allocator alloc;
	ReadView r((void*)args.bytes, args.length, alloc, ReadView::InPlace);
	params.tokenId = Read8u(r);
	Bytes32 to{};
	Read(to, r);
	if( r.Failure() )
		return false;
	params.toIsNftAddress = TokenHelper::IsNftAddress(to);
	const uint32_t count = (uint32_t)Read4u(r);
	if( r.Failure() || count > r.length )
		return false;

	std::vector<intx> seriesIds;
	roms.clear();
	rams.clear();
	for( uint32_t i = 0; i != count; ++i )
	{
		intx seriesId = intx::Zero();
		if( !Read(seriesId, r) )
			return false;
		ByteView rom{}, ram{};
		if( !ReadArray(rom, r, alloc) || !ReadArray(ram, r, alloc) || r.Failure() )
			return false;
		roms.push_back((uint32_t)rom.length);
		rams.push_back((uint32_t)ram.length);
		bool seen = false;
		for( size_t s = 0; s != seriesIds.size() && !seen; ++s )
		{
			seen = seriesIds[s] == seriesId;
		}
		if( !seen )
		{
			seriesIds.push_back(seriesId);
		}
	}
	params.count = count;
	params.romBytesPerInstance = roms.empty() ? nullptr : &roms.front();
	params.ramBytesPerInstance = rams.empty() ? nullptr : &rams.front();
	params.distinctSeriesCount = seriesIds.empty() ? 1 : (uint32_t)seriesIds.size();
	return true;
}

// Holds the per-instance ROM and RAM sizes a described part points at. NativeFeeParams takes those
// as borrowed arrays, so the storage has to outlive the estimate; one of these lives beside the part
// list for as long as a plan is being computed.
struct DescribedParts {
	std::vector<NativeFeePart> parts;
	// Moving a std::vector hands over its heap buffer, so the pointers a part borrows into these
	// stay valid while the outer lists grow.
	std::vector<std::vector<uint32_t>> romSizes;
	std::vector<std::vector<uint32_t>> ramSizes;
};

inline bool DescribeCall(const Blockchain::TxMsgCall& call, const FeePlanOptions& options, bool takeInfusions, DescribedParts& described);

// What a message does, written as the calculator's operations. An ordinary message gives one entry.
// A Call_Multi gives one entry per inner call. That is what lets a batch be priced.
inline bool Describe(const Blockchain::TxMsg& msg, const FeePlanOptions& options, DescribedParts& described)
{
	NativeFeeParams params = StateFacts(options);
	switch( msg.type )
	{
	case Blockchain::TxTypes::TransferFungible:
		params.tokenId = msg.transferFt.tokenId;
		params.toIsNftAddress = TokenHelper::IsNftAddress(msg.transferFt.to);
		described.parts.push_back(DescribeAs(NativeFeeKind::TransferFungible, params));
		return true;
	case Blockchain::TxTypes::TransferFungible_GasPayer:
		params.tokenId = msg.transferFtGasPayer.tokenId;
		params.toIsNftAddress = TokenHelper::IsNftAddress(msg.transferFtGasPayer.to);
		described.parts.push_back(DescribeAs(NativeFeeKind::TransferFungible, params));
		return true;

	case Blockchain::TxTypes::TransferNonFungible_Single:
		params.tokenId = msg.transferNftSingle.tokenId;
		params.count = 1;
		params.toIsNftAddress = TokenHelper::IsNftAddress(msg.transferNftSingle.to);
		described.parts.push_back(DescribeAs(NativeFeeKind::TransferNonFungible, params));
		return true;
	case Blockchain::TxTypes::TransferNonFungible_Single_GasPayer:
		params.tokenId = msg.transferNftSingleGasPayer.tokenId;
		params.count = 1;
		params.toIsNftAddress = TokenHelper::IsNftAddress(msg.transferNftSingleGasPayer.to);
		described.parts.push_back(DescribeAs(NativeFeeKind::TransferNonFungible, params));
		return true;
	case Blockchain::TxTypes::TransferNonFungible_Multi:
		params.tokenId = msg.transferNftMulti.tokenId;
		params.count = msg.transferNftMulti.numInstanceIds;
		params.toIsNftAddress = TokenHelper::IsNftAddress(msg.transferNftMulti.to);
		described.parts.push_back(DescribeAs(NativeFeeKind::TransferNonFungible, params));
		return true;
	case Blockchain::TxTypes::TransferNonFungible_Multi_GasPayer:
		params.tokenId = msg.transferNftMultiGasPayer.tokenId;
		params.count = msg.transferNftMultiGasPayer.numInstanceIds;
		params.toIsNftAddress = TokenHelper::IsNftAddress(msg.transferNftMultiGasPayer.to);
		described.parts.push_back(DescribeAs(NativeFeeKind::TransferNonFungible, params));
		return true;

	case Blockchain::TxTypes::MintFungible:
		params.tokenId = msg.mintFungible.tokenId;
		params.toIsNftAddress = TokenHelper::IsNftAddress(msg.mintFungible.to);
		described.parts.push_back(DescribeAs(NativeFeeKind::MintFungible, params));
		return true;

	case Blockchain::TxTypes::BurnFungible:
		params.tokenId = msg.burnFungible.tokenId;
		described.parts.push_back(DescribeAs(NativeFeeKind::BurnFungible, params));
		return true;
	case Blockchain::TxTypes::BurnFungible_GasPayer:
		params.tokenId = msg.burnFungibleGasPayer.tokenId;
		described.parts.push_back(DescribeAs(NativeFeeKind::BurnFungible, params));
		return true;

	case Blockchain::TxTypes::MintNonFungible:
		params.tokenId = msg.mintNonFungible.tokenId;
		params.romBytes = (uint32_t)msg.mintNonFungible.rom.length;
		params.ramBytes = (uint32_t)msg.mintNonFungible.ram.length;
		params.toIsNftAddress = TokenHelper::IsNftAddress(msg.mintNonFungible.to);
		described.parts.push_back(DescribeAs(NativeFeeKind::MintNonFungible, params));
		return true;

	case Blockchain::TxTypes::BurnNonFungible:
	case Blockchain::TxTypes::BurnNonFungible_GasPayer:
		// The stored ROM is chain state that the message does not carry, so the deleted quanta are a
		// lower bound. The offer is unaffected. A burn deletes more rows than it creates, and only
		// the rows it creates are escrowed.
		params.tokenId = msg.type == Blockchain::TxTypes::BurnNonFungible ? msg.burnNonFungible.tokenId : msg.burnNonFungibleGasPayer.tokenId;
		params.count = 1;
		if( !RequireInfusions(options, params) )
			return false;
		described.parts.push_back(DescribeAs(NativeFeeKind::BurnNonFungible, params));
		return true;

	case Blockchain::TxTypes::Call:
		return DescribeCall(msg.call, options, true, described);

	case Blockchain::TxTypes::Call_Multi: {
		// The chain runs the calls in a loop and bills their sum, so the plan is the sum of their
		// models. `infusions` covers every burn in the batch. The returns cost the same wherever
		// they are counted, so the first burn takes the whole list and the burns after it take none.
		bool takeInfusions = true;
		for( uint32_t i = 0; i != msg.callMulti.numCalls; ++i )
		{
			const size_t before = described.parts.size();
			if( !DescribeCall(msg.callMulti.calls[i], options, takeInfusions, described) )
				return false;
			if( described.parts.size() > before && described.parts.back().kind == NativeFeeKind::BurnNonFungible )
			{
				takeInfusions = false;
			}
		}
		return true;
	}

	case Blockchain::TxTypes::Trade:
	case Blockchain::TxTypes::Phantasma:
		described.parts.push_back(ScriptPlan(options));
		return true;

	case Blockchain::TxTypes::Phantasma_Raw:
	default:
		// A raw Gen2 envelope carries its own fee fields and its own signatures. This planner prices
		// Carbon messages, so it refuses. A quoted number would be one it cannot size.
		PHANTASMA_EXCEPTION("cannot plan fees for this transaction type");
		return false;
	}
}

inline bool DescribeCall(const Blockchain::TxMsgCall& call, const FeePlanOptions& options, bool takeInfusions, DescribedParts& described)
{
	// A call can build its arguments at execution time from the results of earlier calls. Such a
	// call carries none of them yet. There is nothing to read a price from, so it takes the script
	// budget.
	if( call.sections.numArgSections_negative < 0 )
	{
		described.parts.push_back(ScriptPlan(options));
		return true;
	}
	NativeFeeParams params = StateFacts(options);
	if( call.moduleId == (uint32_t)ModuleId::Token )
	{
		switch( (TokenContract_Methods)call.methodId )
		{
		// The five token movements below cost exactly what they cost as native transaction types.
		// Both paths enter the same contract method. They arrive as module calls because a wallet
		// batched them.
		//
		// TokenContract_Methods::MintNonFungible is absent on purpose. The chain refuses it under
		// mainnet SR 50 whichever way it arrives, so there is nothing to price.
		case TokenContract_Methods::TransferFungible: {
			Bytes32 to{};
			if( !TokenCallArgs::ReadTransferFungible(call.args, to, params.tokenId) )
				return false;
			params.toIsNftAddress = TokenHelper::IsNftAddress(to);
			described.parts.push_back(DescribeAs(NativeFeeKind::TransferFungible, params));
			return true;
		}
		case TokenContract_Methods::TransferNonFungible: {
			Bytes32 to{};
			if( !TokenCallArgs::ReadTransferNonFungible(call.args, to, params.tokenId, params.count) )
				return false;
			params.toIsNftAddress = TokenHelper::IsNftAddress(to);
			described.parts.push_back(DescribeAs(NativeFeeKind::TransferNonFungible, params));
			return true;
		}
		case TokenContract_Methods::MintFungible: {
			Bytes32 to{};
			if( !TokenCallArgs::ReadMintFungible(call.args, to, params.tokenId) )
				return false;
			params.toIsNftAddress = TokenHelper::IsNftAddress(to);
			described.parts.push_back(DescribeAs(NativeFeeKind::MintFungible, params));
			return true;
		}
		case TokenContract_Methods::BurnFungible:
			if( !TokenCallArgs::ReadBurnFungible(call.args, params.tokenId) )
				return false;
			described.parts.push_back(DescribeAs(NativeFeeKind::BurnFungible, params));
			return true;
		case TokenContract_Methods::BurnNonFungible: {
			PHANTASMA_VECTOR<uint64_t> instanceIds;
			if( !TokenCallArgs::ReadBurnNonFungible(call.args, params.tokenId, instanceIds) )
				return false;
			params.count = (uint32_t)instanceIds.size();
			// Only the first burn of a batch takes the returned assets. They cost the same wherever
			// they are counted, and the list covers every burn in the message.
			if( takeInfusions && !RequireInfusions(options, params) )
				return false;
			described.parts.push_back(DescribeAs(NativeFeeKind::BurnNonFungible, params));
			return true;
		}
		case TokenContract_Methods::CreateToken: {
			// The token-info row is the Call arguments as submitted. The chain stores the TokenInfo
			// it was given, metadata included, and measured bills confirm that the row equals the
			// arguments.
			NativeFeeParams create{};
			if( !ReadCreateTokenFacts(call.args, create) )
				return false;
			described.parts.push_back(DescribeAs(NativeFeeKind::CreateToken, create));
			return true;
		}
		case TokenContract_Methods::CreateTokenSeries: {
			// The arguments are the u64 token id followed by the SeriesInfo, which becomes the row.
			NativeFeeParams series{};
			series.seriesInfoBytes = call.args.length > 8 ? (uint32_t)(call.args.length - 8) : 0;
			series.seriesHasMetaId = options.seriesHasMetaId;
			described.parts.push_back(DescribeAs(NativeFeeKind::CreateTokenSeries, series));
			return true;
		}
		case TokenContract_Methods::MintPhantasmaNonFungible: {
			described.romSizes.push_back(std::vector<uint32_t>());
			described.ramSizes.push_back(std::vector<uint32_t>());
			params.duplicatedSeries = options.duplicatedSeries;
			if( !ReadPhantasmaMintFacts(call.args, params, described.romSizes.back(), described.ramSizes.back()) )
				return false;
			described.parts.push_back(DescribeAs(NativeFeeKind::MintPhantasmaNonFungible, params));
			return true;
		}
		default:
			described.parts.push_back(ScriptPlan(options));
			return true;
		}
	}
	if( call.moduleId == (uint32_t)ModuleId::Governance && call.methodId == (uint32_t)GovernanceContract_Methods::RegisterName )
	{
		// The arguments are the 32-byte address followed by the name as a SmallString.
		ReadView r((void*)call.args.bytes, call.args.length);
		Bytes32 address{};
		Read(address, r);
		SmallString name{};
		Read(name, r);
		if( r.Failure() )
			return false;
		NativeFeeParams registerName{};
		registerName.nameLength = name.length;
		described.parts.push_back(DescribeAs(NativeFeeKind::RegisterName, registerName));
		return true;
	}
	described.parts.push_back(ScriptPlan(options));
	return true;
}

// Returns true if a fact the caller left at its costlier default changed this quote.
//
// The message is priced a second time, with every state fact at its CHEAPER reading. The two quotes
// are then compared. If they agree, the costlier defaults decided nothing and the bill is a
// prediction.
//
// The question is asked this way to keep ONE definition of which facts an operation reads: the
// operation models themselves. A list written here would drift from them as the models change.
//
// The answer is also per message, not per kind. That matters. A gas-token transfer does not depend
// on recipientHoldsToken at all, because the chain's own rows are free. A plan that reported the
// fact as assumed would send every ordinary transfer to the "up to" branch.
//
// `infusions` is not flipped. It has no cheaper reading, and the planner demands it.
inline bool AssumptionsMattered(const Blockchain::TxMsg& msg, const Blockchain::GasConfig& config, const FeePlanOptions& options, uint32_t envelopeBytes, const NativeFeeEstimate& quoted)
{
	FeePlanOptions cheapest = options;
	cheapest.recipientHoldsToken = true;
	cheapest.tokenBurnedBefore = true;
	cheapest.supplyRowExists = true;
	cheapest.bigFungible = false;
	cheapest.romHasMetaId = false;
	cheapest.seriesHasMetaId = false;
	cheapest.duplicatedSeries = false;

	DescribedParts described;
	if( !Describe(msg, cheapest, described) )
		return true;
	const NativeFeeTransactionParams transaction{ envelopeBytes, 0 };
	const NativeFeeEstimate cheaper = EstimateNativeFeeBatch(
	    described.parts.empty() ? nullptr : &described.parts.front(), (uint32_t)described.parts.size(), config, transaction);
	return cheaper.expectedGasBill != quoted.expectedGasBill || cheaper.maxGas != quoted.maxGas || cheaper.maxData != quoted.maxData;
}

} // namespace FeePlanDetail

// Plans the gas offer and the storage ceiling of `msg` against the chain's current prices.
//
// Answers false without touching `out` when the message cannot be planned: a type this planner does
// not price, a burn whose returned assets the caller has not read, a witness count that disagrees
// with the message, or arguments too short to read. In builds with exceptions enabled the same cases
// raise PHANTASMA_EXCEPTION. This SDK favors a bool result over exceptions; check it before using
// `out`.
inline bool PlanFees(const Blockchain::TxMsg& msg, const Blockchain::GasConfig& config, const FeePlanOptions& options, FeePlan& out)
{
	using namespace FeePlanDetail;
	// A witness-array message does not say how many signatures it will carry, and each one is 96
	// billed bytes. An assumed single witness would under-offer every multi-party transaction by 96
	// bytes per extra signature, and the chain would reject it. So the count is demanded here.
	// EstimateNativeFeeBatch takes the same stance on a missing envelope size.
	uint32_t required = 0;
	const bool fixedWitnesses = Blockchain::RequiredWitnessCount(msg.type, required);
	if( !fixedWitnesses && options.witnessCount == 0 )
	{
		PHANTASMA_EXCEPTION("this transaction type chooses its own witnesses: set witnessCount to plan one");
		return false;
	}
	if( fixedWitnesses && options.witnessCount != 0 && options.witnessCount != required )
	{
		PHANTASMA_EXCEPTION("this transaction type fixes its own witness count");
		return false;
	}

	DescribedParts described;
	if( !Describe(msg, options, described) )
		return false;
	const uint32_t envelopeBytes = Blockchain::EnvelopeBytes(msg, options.witnessCount);
	const NativeFeeTransactionParams transaction{ envelopeBytes, 0 };
	const NativeFeePart* parts = described.parts.empty() ? nullptr : &described.parts.front();
	const uint32_t numParts = (uint32_t)described.parts.size();
	bool priced = false;
	const NativeFeeEstimate estimate = EstimateNativeFeeBatch(parts, numParts, config, transaction, &priced);
	if( !priced )
		return false; // the calculator refused the inputs and has already reported why

	bool budgeted = false;
	out = FeePlan{};
	static_cast<NativeFeeEstimate&>(out) = estimate;
	out.kinds.clear();
	for( uint32_t i = 0; i != numParts; ++i )
	{
		out.kinds.push_back(parts[i].kind);
		budgeted = budgeted || parts[i].kind == NativeFeeKind::Script;
	}
	out.envelopeBytes = envelopeBytes;
	out.exact = !budgeted && !AssumptionsMattered(msg, config, options, envelopeBytes, estimate);
	return true;
}

// Returns the NFT instances `msg` burns. It covers the native burn types, a Token.BurnNonFungible
// call, and every such call inside a Call_Multi.
//
// A burn returns whatever the instance's own address holds, and the chain charges for each returned
// asset. A caller with a chain to ask reads those assets per instance and hands the union to
// FeePlanOptions::infusions.
//
// This function sits beside the decomposition that decides which calls are burns, so the two cannot
// come to disagree. It answers false when a call's arguments are too short to read.
inline bool BurnedInstances(const Blockchain::TxMsg& msg, PHANTASMA_VECTOR<BurnedInstance>& out)
{
	out.clear();
	auto burnedByCall = [&out](const Blockchain::TxMsgCall& call)
	{
		if( call.sections.numArgSections_negative < 0 || call.moduleId != (uint32_t)ModuleId::Token ||
		    call.methodId != (uint32_t)TokenContract_Methods::BurnNonFungible )
		{
			return true;
		}
		uint64_t tokenId = 0;
		PHANTASMA_VECTOR<uint64_t> instanceIds;
		if( !TokenCallArgs::ReadBurnNonFungible(call.args, tokenId, instanceIds) )
			return false;
		for( size_t i = 0; i != instanceIds.size(); ++i )
		{
			out.push_back(BurnedInstance{ tokenId, instanceIds[i] });
		}
		return true;
	};

	switch( msg.type )
	{
	case Blockchain::TxTypes::BurnNonFungible:
		out.push_back(BurnedInstance{ msg.burnNonFungible.tokenId, msg.burnNonFungible.instanceId });
		return true;
	case Blockchain::TxTypes::BurnNonFungible_GasPayer:
		out.push_back(BurnedInstance{ msg.burnNonFungibleGasPayer.tokenId, msg.burnNonFungibleGasPayer.instanceId });
		return true;
	case Blockchain::TxTypes::Call:
		return burnedByCall(msg.call);
	case Blockchain::TxTypes::Call_Multi:
		for( uint32_t i = 0; i != msg.callMulti.numCalls; ++i )
		{
			if( !burnedByCall(msg.callMulti.calls[i]) )
				return false;
		}
		return true;
	default:
		return true;
	}
}

} // namespace phantasma::carbon
