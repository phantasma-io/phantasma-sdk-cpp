#include "test_cases.h"

#include "../include/Carbon/FeePlanSummary.h"

namespace testcases {
using namespace testutil;

// The planner: what it makes of each message, how firm the number is, and the coverage gate the fee
// model owes itself. The three tables below name every transaction type and every method of the two
// modules the planner dispatches on. Each table is walked over the whole enum, so a new type or a
// new method fails these tests until someone states what its fee is. A Script entry is a decision,
// never an omission.

namespace {

using Blockchain::TxTypes;

Blockchain::GasConfig PlanningConfig()
{
	Blockchain::GasConfig c{};
	c.version = 1;
	c.maxNameLength = 32;
	c.maxTokenSymbolLength = 10;
	c.feeShift = 0;
	c.maxStructureSize = 65536;
	c.feeMultiplier = 10000;
	c.gasTokenId = 2;
	c.dataTokenId = 1;
	c.minimumGasOffer = 10;
	c.dataEscrowPerRow = 200000;
	c.minimumGasBill = 10000000;
	c.gasFeeTransfer = 10;
	c.gasFeeQuery = 2;
	c.gasFeePerByte = 250000;
	c.policyFeeCreateTokenBase = 100000000000000ull;
	c.policyFeeCreateTokenSymbol = 100000000000000ull;
	c.policyFeeCreateTokenSeries = 25000000000000ull;
	c.policyFeeRegisterName = 100000000000000000ull;
	c.legacyDataEscrowPerRow = 2;
	return c;
}

// A token that is neither the gas nor the data token, so its balance rows are paid.
constexpr uint64_t kFungibleTokenId = 97;
constexpr uint64_t kNftTokenId = 9;

Bytes32 Address(uint8_t seed)
{
	Bytes32 out{};
	for( int i = 0; i != Bytes32::length; ++i )
	{
		out.bytes[i] = (uint8_t)(seed + i);
	}
	return out;
}

Blockchain::TxMsg BaseMsg(TxTypes type)
{
	Blockchain::TxMsg msg;
	msg.type = type;
	msg.expiry = 1700000000000ll;
	msg.gasFrom = Address(1);
	return msg;
}

ByteView View(const ByteArray& bytes)
{
	return ByteView{ bytes.empty() ? nullptr : bytes.data(), bytes.size() };
}

Blockchain::TxMsgCall ModuleCall(ModuleId moduleId, uint32_t methodId, const ByteArray& args)
{
	Blockchain::TxMsgCall call{};
	call.moduleId = (uint32_t)moduleId;
	call.methodId = methodId;
	call.args = View(args);
	return call;
}

Blockchain::TxMsgCall TokenCall(TokenContract_Methods method, const ByteArray& args)
{
	return ModuleCall(ModuleId::Token, (uint32_t)method, args);
}

Blockchain::TxMsg CallMsg(const Blockchain::TxMsgCall& call)
{
	Blockchain::TxMsg msg = BaseMsg(TxTypes::Call);
	msg.call = call;
	return msg;
}

// Arguments nothing reads. A method with no model reads none of its arguments, so any bytes reach
// the branch.
ByteArray AnyArgs()
{
	return ByteArray(8, 0);
}

ByteArray TransferFungibleArgs()
{
	ByteArray buffer;
	WriteView w(buffer);
	Write(Address(2), w); // to
	Write(Address(1), w); // from
	Write8u(kFungibleTokenId, w);
	Write(intx((uint64_t)1), w); // amount
	return buffer;
}

ByteArray TransferNonFungibleArgs(uint32_t instanceCount)
{
	ByteArray buffer;
	WriteView w(buffer);
	Write(Address(2), w);
	Write(Address(1), w);
	Write8u(kNftTokenId, w);
	Write4((int32_t)instanceCount, w);
	for( uint32_t i = 0; i != instanceCount; ++i )
	{
		Write8u(5 + i, w);
	}
	return buffer;
}

ByteArray MintFungibleArgs()
{
	ByteArray buffer;
	WriteView w(buffer);
	Write8u(kFungibleTokenId, w);
	Write(Address(2), w);
	Write(intx((uint64_t)1), w);
	return buffer;
}

ByteArray BurnFungibleArgs()
{
	ByteArray buffer;
	WriteView w(buffer);
	Write8u(kFungibleTokenId, w);
	Write(Address(1), w);
	Write(intx((uint64_t)1), w);
	return buffer;
}

ByteArray BurnNonFungibleArgs(uint32_t instanceCount)
{
	ByteArray buffer;
	WriteView w(buffer);
	Write8u(kNftTokenId, w);
	Write(Address(1), w);
	Write4((int32_t)instanceCount, w);
	for( uint32_t i = 0; i != instanceCount; ++i )
	{
		Write8u(11 + i, w);
	}
	return buffer;
}

// The token metadata a CreateToken call carries, as the serialized struct the chain stores. It is
// built here field by field rather than through TokenMetadataBuilder, whose own rules demand the
// four display fields that have nothing to do with the fee.
ByteArray TokenMetadata(bool withPreBurn)
{
	static const char* kName = "Test Token";
	static const char* kPreBurn = "1";
	std::vector<VmNamedDynamicVariable> fields;
	fields.push_back(VmNamedDynamicVariable{ SmallString("name"), VmDynamicVariable(kName) });
	if( withPreBurn )
	{
		fields.push_back(VmNamedDynamicVariable{ StandardMeta::Token::pre_burn, VmDynamicVariable(kPreBurn) });
	}
	const VmDynamicStruct meta = VmDynamicStruct::Sort((uint32_t)fields.size(), fields.data());
	ByteArray buffer;
	WriteView w(buffer);
	Write(meta, w);
	return buffer;
}

// A TokenInfo as the CreateToken call carries it.
ByteArray CreateTokenArgs(bool nonFungible, bool withPreBurn)
{
	const ByteArray metadata = TokenMetadata(withPreBurn);

	TokenInfo info{};
	info.maxSupply.x() = intx((uint64_t)1000000);
	info.flags = nonFungible ? TokenFlags_NonFungible : TokenFlags_None;
	info.decimals = 8;
	info.owner = Address(1);
	info.symbol = SmallString("TEST");
	info.metadata = View(metadata);
	ByteArray buffer;
	WriteView w(buffer);
	Write(info, w);
	return buffer;
}

ByteArray CreateTokenSeriesArgs()
{
	const ByteArray metadata = TokenMetadata(false);
	SeriesInfo series{};
	series.maxMint = 100;
	series.maxSupply = 100;
	series.owner = Address(1);
	series.metadata = View(metadata);
	ByteArray buffer;
	WriteView w(buffer);
	Write8u(kNftTokenId, w);
	Write(series, w);
	return buffer;
}

ByteArray MintPhantasmaArgs(uint32_t count, uint32_t distinctSeries)
{
	std::vector<ByteArray> roms(count, ByteArray(40, 7));
	ByteArray buffer;
	WriteView w(buffer);
	Write8u(kNftTokenId, w);
	Write(Address(2), w);
	Write4((int32_t)count, w);
	for( uint32_t i = 0; i != count; ++i )
	{
		PhantasmaNftMintInfo token{};
		token.phantasmaSeriesId.x() = intx((uint64_t)(1 + (i % distinctSeries)));
		token.rom = View(roms[i]);
		Write(token, w);
	}
	return buffer;
}

// The plan options a table row uses: nothing is known about chain state, and the burned instances
// are stated to hold nothing so a burn can be planned at all.
FeePlanOptions PlanFacts()
{
	FeePlanOptions options{};
	options.infusionsRead = true;
	options.witnessCount = 1;
	return options;
}

bool KindsAre(const FeePlan& plan, const NativeFeeKind* expected, size_t count)
{
	if( plan.kinds.size() != count )
		return false;
	for( size_t i = 0; i != count; ++i )
	{
		if( plan.kinds[i] != expected[i] )
			return false;
	}
	return true;
}

bool PlansAs(const Blockchain::TxMsg& msg, NativeFeeKind expected, const FeePlanOptions& options)
{
	FeePlan plan;
	try
	{
		if( !PlanFees(msg, PlanningConfig(), options, plan) )
			return false;
	}
	catch( const std::exception& )
	{
		return false;
	}
	return KindsAre(plan, &expected, 1);
}

bool PlansAs(const Blockchain::TxMsg& msg, NativeFeeKind expected)
{
	return PlansAs(msg, expected, PlanFacts());
}

// True when the planner refused the message. It answers false, and in builds with exceptions enabled
// it throws instead.
bool Refuses(const Blockchain::TxMsg& msg, const FeePlanOptions& options)
{
	FeePlan plan;
	try
	{
		return !PlanFees(msg, PlanningConfig(), options, plan);
	}
	catch( const std::exception& )
	{
		return true;
	}
}

// What the planner must make of each token-module method. The switch has no default, so a compiler
// warning names the method the moment one is added to the enum.
NativeFeeKind ExpectedTokenKind(TokenContract_Methods method)
{
	switch( method )
	{
	case TokenContract_Methods::TransferFungible:
		return NativeFeeKind::TransferFungible;
	case TokenContract_Methods::TransferNonFungible:
		return NativeFeeKind::TransferNonFungible;
	case TokenContract_Methods::CreateToken:
		return NativeFeeKind::CreateToken;
	case TokenContract_Methods::MintFungible:
		return NativeFeeKind::MintFungible;
	case TokenContract_Methods::BurnFungible:
		return NativeFeeKind::BurnFungible;
	case TokenContract_Methods::CreateTokenSeries:
		return NativeFeeKind::CreateTokenSeries;
	case TokenContract_Methods::BurnNonFungible:
		return NativeFeeKind::BurnNonFungible;
	case TokenContract_Methods::MintPhantasmaNonFungible:
		return NativeFeeKind::MintPhantasmaNonFungible;
	// MintNonFungible is budgeted on purpose: mainnet SR 50 refuses an explicit NFT mint whichever
	// way it arrives, so there is nothing to price. Everything else below has no closed-form model.
	case TokenContract_Methods::MintNonFungible:
	case TokenContract_Methods::GetBalance:
	case TokenContract_Methods::DeleteTokenSeries:
	case TokenContract_Methods::GetInstances:
	case TokenContract_Methods::GetNonFungibleInfo:
	case TokenContract_Methods::GetNonFungibleInfoByRomId:
	case TokenContract_Methods::GetSeriesInfo:
	case TokenContract_Methods::GetSeriesInfoByMetaId:
	case TokenContract_Methods::GetTokenInfo:
	case TokenContract_Methods::GetTokenInfoBySymbol:
	case TokenContract_Methods::GetTokenSupply:
	case TokenContract_Methods::GetSeriesSupply:
	case TokenContract_Methods::GetTokenIdBySymbol:
	case TokenContract_Methods::GetBalances:
	case TokenContract_Methods::CreateMintedTokenSeries:
	case TokenContract_Methods::ApplyInflation:
	case TokenContract_Methods::UpdateTokenMetadata:
	case TokenContract_Methods::GetNextTokenInflation:
	case TokenContract_Methods::SetTokensConfig:
	case TokenContract_Methods::UpdateSeriesMetadata:
		return NativeFeeKind::Script;
	}
	return NativeFeeKind::Script;
}

// The arguments a token-module method is planned from. A modelled method reads its own; the rest
// read none.
ByteArray TokenMethodArgs(TokenContract_Methods method)
{
	switch( method )
	{
	case TokenContract_Methods::TransferFungible:
		return TransferFungibleArgs();
	case TokenContract_Methods::TransferNonFungible:
		return TransferNonFungibleArgs(1);
	case TokenContract_Methods::CreateToken:
		return CreateTokenArgs(false, false);
	case TokenContract_Methods::MintFungible:
		return MintFungibleArgs();
	case TokenContract_Methods::BurnFungible:
		return BurnFungibleArgs();
	case TokenContract_Methods::CreateTokenSeries:
		return CreateTokenSeriesArgs();
	case TokenContract_Methods::BurnNonFungible:
		return BurnNonFungibleArgs(1);
	case TokenContract_Methods::MintPhantasmaNonFungible:
		return MintPhantasmaArgs(1, 1);
	default:
		return AnyArgs();
	}
}

} // namespace

void RunFeePlanTests(TestContext& ctx)
{
	const Blockchain::GasConfig config = PlanningConfig();

	// COVERAGE LEDGER, first closure: every transaction type the SDK carries is named here with what
	// the planner makes of it, and the loop walks the whole enum. A new type cannot be added and
	// quietly fall into the script budget.
	{
		const ByteArray burnArgs = BurnFungibleArgs();
		const ByteArray multiArgs = BurnFungibleArgs();
		Blockchain::TxMsgCall batched[2] = { TokenCall(TokenContract_Methods::BurnFungible, multiArgs),
			TokenCall(TokenContract_Methods::BurnFungible, multiArgs) };
		const uint64_t instanceIds[2] = { 5, 6 };
		const ByteArray rom(8, 3);
		const ByteArray script(8, 1);

		bool allNamed = true;
		bool allPlanned = true;
		for( int raw = 0; raw <= (int)TxTypes::Phantasma_Raw; ++raw )
		{
			const TxTypes type = (TxTypes)raw;
			Blockchain::TxMsg msg = BaseMsg(type);
			NativeFeeKind expected[2] = {};
			size_t expectedCount = 1;
			bool plannable = true;
			switch( type )
			{
			case TxTypes::Call:
				msg.call = TokenCall(TokenContract_Methods::BurnFungible, burnArgs);
				expected[0] = NativeFeeKind::BurnFungible;
				break;
			case TxTypes::Call_Multi:
				msg.callMulti.numCalls = 2;
				msg.callMulti.calls = batched;
				expected[0] = NativeFeeKind::BurnFungible;
				expected[1] = NativeFeeKind::BurnFungible;
				expectedCount = 2;
				break;
			case TxTypes::Trade:
				// A Trade packs its operations into named arrays and not into calls. Nothing reads
				// them yet, so it is budgeted. Modelling it is worth doing only when something
				// builds one.
				msg.trade = Blockchain::TxMsgTrade{};
				expected[0] = NativeFeeKind::Script;
				break;
			case TxTypes::TransferFungible:
				msg.transferFt = Blockchain::TxMsgTransferFungible{ Address(2), kFungibleTokenId, 1 };
				expected[0] = NativeFeeKind::TransferFungible;
				break;
			case TxTypes::TransferFungible_GasPayer:
				msg.transferFtGasPayer = Blockchain::TxMsgTransferFungible_GasPayer{ Address(2), Address(1), kFungibleTokenId, 1 };
				expected[0] = NativeFeeKind::TransferFungible;
				break;
			case TxTypes::TransferNonFungible_Single:
				msg.transferNftSingle = Blockchain::TxMsgTransferNonFungible_Single{ Address(2), kNftTokenId, 5 };
				expected[0] = NativeFeeKind::TransferNonFungible;
				break;
			case TxTypes::TransferNonFungible_Single_GasPayer:
				msg.transferNftSingleGasPayer = Blockchain::TxMsgTransferNonFungible_Single_GasPayer{ Address(2), Address(1), kNftTokenId, 5 };
				expected[0] = NativeFeeKind::TransferNonFungible;
				break;
			case TxTypes::TransferNonFungible_Multi:
				msg.transferNftMulti = Blockchain::TxMsgTransferNonFungible_Multi{ Address(2), kNftTokenId, 2, instanceIds };
				expected[0] = NativeFeeKind::TransferNonFungible;
				break;
			case TxTypes::TransferNonFungible_Multi_GasPayer:
				msg.transferNftMultiGasPayer = Blockchain::TxMsgTransferNonFungible_Multi_GasPayer{ Address(2), Address(1), kNftTokenId, 2, instanceIds };
				expected[0] = NativeFeeKind::TransferNonFungible;
				break;
			case TxTypes::MintFungible:
				msg.mintFungible = Blockchain::TxMsgMintFungible{};
				msg.mintFungible.tokenId = kFungibleTokenId;
				msg.mintFungible.amount.x() = intx((uint64_t)1);
				msg.mintFungible.to = Address(2);
				expected[0] = NativeFeeKind::MintFungible;
				break;
			case TxTypes::BurnFungible:
				msg.burnFungible = Blockchain::TxMsgBurnFungible{};
				msg.burnFungible.tokenId = kFungibleTokenId;
				msg.burnFungible.amount.x() = intx((uint64_t)1);
				expected[0] = NativeFeeKind::BurnFungible;
				break;
			case TxTypes::BurnFungible_GasPayer:
				msg.burnFungibleGasPayer = Blockchain::TxMsgBurnFungible_GasPayer{};
				msg.burnFungibleGasPayer.tokenId = kFungibleTokenId;
				msg.burnFungibleGasPayer.amount.x() = intx((uint64_t)1);
				msg.burnFungibleGasPayer.from = Address(1);
				expected[0] = NativeFeeKind::BurnFungible;
				break;
			case TxTypes::MintNonFungible:
				msg.mintNonFungible = Blockchain::TxMsgMintNonFungible{ kNftTokenId, Address(2), 1, View(rom), ByteView{} };
				expected[0] = NativeFeeKind::MintNonFungible;
				break;
			case TxTypes::BurnNonFungible:
				msg.burnNonFungible = Blockchain::TxMsgBurnNonFungible{ kNftTokenId, 5 };
				expected[0] = NativeFeeKind::BurnNonFungible;
				break;
			case TxTypes::BurnNonFungible_GasPayer:
				msg.burnNonFungibleGasPayer = Blockchain::TxMsgBurnNonFungible_GasPayer{ kNftTokenId, Address(1), 5 };
				expected[0] = NativeFeeKind::BurnNonFungible;
				break;
			case TxTypes::Phantasma:
				msg.phantasma = Blockchain::TxMsgPhantasma{ SmallString("simnet"), SmallString("main"), View(script) };
				expected[0] = NativeFeeKind::Script;
				break;
			case TxTypes::Phantasma_Raw:
				// A raw Gen2 envelope carries its own fee fields and its own signatures. This
				// planner prices Carbon messages, so it refuses.
				msg.phantasmaRaw = Blockchain::TxMsgPhantasma_Raw{ View(script) };
				plannable = false;
				break;
			default:
				allNamed = false;
				continue;
			}

			FeePlanOptions options = PlanFacts();
			uint32_t required = 0;
			if( Blockchain::RequiredWitnessCount(type, required) )
			{
				options.witnessCount = 0; // the message fixes its own witness set
			}
			if( !plannable )
			{
				allPlanned = allPlanned && Refuses(msg, options);
				continue;
			}
			FeePlan plan;
			const bool planned = PlanFees(msg, config, options, plan);
			allPlanned = allPlanned && planned && KindsAre(plan, expected, expectedCount);
		}
		Report(ctx, allNamed, "every transaction type is named by the coverage table");
		Report(ctx, allPlanned, "every transaction type plans as the table declares");
	}

	// COVERAGE LEDGER, second closure: a modelled call method is priced as the very kind its native
	// transaction already covers, so Token.TransferFungible adds nothing to the kinds a run has
	// seen and a new branch could appear without anything noticing. This table names every method
	// of the token module and walks the whole enum.
	{
		bool allPlanned = true;
		for( int raw = 0; raw <= (int)TokenContract_Methods::MintPhantasmaNonFungible; ++raw )
		{
			const TokenContract_Methods method = (TokenContract_Methods)raw;
			const ByteArray args = TokenMethodArgs(method);
			allPlanned = allPlanned && PlansAs(CallMsg(TokenCall(method, args)), ExpectedTokenKind(method));
		}
		Report(ctx, allPlanned, "every token-module method plans as the table declares");
	}

	// The same closure over the governance module.
	{
		bool allPlanned = true;
		for( int raw = 0; raw <= (int)GovernanceContract_Methods::MigrateAddresses; ++raw )
		{
			const GovernanceContract_Methods method = (GovernanceContract_Methods)raw;
			ByteArray args;
			NativeFeeKind expected = NativeFeeKind::Script;
			if( method == GovernanceContract_Methods::RegisterName )
			{
				WriteView w(args);
				Write(Address(1), w);
				Write(SmallString("alice"), w);
				expected = NativeFeeKind::RegisterName;
			}
			else
			{
				args = AnyArgs();
			}
			allPlanned = allPlanned && PlansAs(CallMsg(ModuleCall(ModuleId::Governance, (uint32_t)method, args)), expected);
		}
		Report(ctx, allPlanned, "every governance-module method plans as the table declares");
	}

	// A module the planner does not dispatch on is budgeted.
	{
		const ByteArray args = AnyArgs();
		Report(ctx, PlansAs(CallMsg(ModuleCall(ModuleId::Organization, 0, args)), NativeFeeKind::Script),
		    "a call to an unmodelled module takes the script budget");
	}

	// A call can build its arguments at execution time from the results of earlier calls. Such a
	// call carries none of them yet, so there is nothing to read a price from.
	{
		Blockchain::MsgCallArgs section{};
		section.registerOffset = -1;
		Blockchain::TxMsgCall call{};
		call.moduleId = (uint32_t)ModuleId::Token;
		call.methodId = (uint32_t)TokenContract_Methods::TransferFungible;
		call.sections.numArgSections_negative = -1;
		call.sections.argSections = &section;
		Report(ctx, PlansAs(CallMsg(call), NativeFeeKind::Script),
		    "a call with argument sections takes the script budget");
	}

	// The envelope the plan was computed for is the signed size of the message, and the plan writes
	// its numbers into a copy while the input is left alone.
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::TransferFungible);
		msg.transferFt = Blockchain::TxMsgTransferFungible{ Address(2), kFungibleTokenId, 1 };
		msg.maxGas = 1;
		msg.maxData = 2;
		FeePlanOptions options = PlanFacts();
		options.witnessCount = 0;
		FeePlan plan;
		const bool planned = PlanFees(msg, config, options, plan);
		const Blockchain::TxMsg applied = plan.Apply(msg);
		Report(ctx,
		    planned && plan.envelopeBytes == Blockchain::EnvelopeBytes(msg, 1) &&
		        applied.maxGas == plan.maxGas && applied.maxData == plan.maxData && msg.maxGas == 1 && msg.maxData == 2,
		    "a plan sizes the signed message and applies to a copy");
	}

	// A witness-array message does not say how many signatures it will carry, and each one is 96
	// billed bytes. The count is demanded rather than assumed. A type that fixes its own witness set
	// refuses a count that disagrees with it.
	{
		const ByteArray args = BurnFungibleArgs();
		FeePlanOptions unstated{};
		unstated.infusionsRead = true;
		Blockchain::TxMsg native = BaseMsg(TxTypes::TransferFungible);
		native.transferFt = Blockchain::TxMsgTransferFungible{ Address(2), kFungibleTokenId, 1 };
		FeePlanOptions wrongCount = PlanFacts();
		wrongCount.witnessCount = 2; // this type carries exactly one
		Report(ctx, Refuses(CallMsg(TokenCall(TokenContract_Methods::BurnFungible, args)), unstated) && Refuses(native, wrongCount),
		    "the witness count is demanded where the caller chooses it and refused where it cannot vary");
	}

	// Two witnesses add 96 bytes each to the envelope, and the bill grows with them.
	{
		const ByteArray args = BurnFungibleArgs();
		const Blockchain::TxMsg msg = CallMsg(TokenCall(TokenContract_Methods::BurnFungible, args));
		FeePlanOptions one = PlanFacts();
		FeePlanOptions two = PlanFacts();
		two.witnessCount = 2;
		FeePlan single, pair;
		const bool planned = PlanFees(msg, config, one, single) && PlanFees(msg, config, two, pair);
		Report(ctx,
		    planned && pair.envelopeBytes == single.envelopeBytes + 96 &&
		        pair.expectedGasBill == single.expectedGasBill + 96ull * 25 * 10000,
		    "each extra witness is 96 billed bytes");
	}

	// A batch is planned as the operations it performs, in call order, and its envelope is billed
	// once. Planning the two calls apart would bill two envelopes.
	{
		const ByteArray burnArgs = BurnFungibleArgs();
		const ByteArray transferArgs = TransferFungibleArgs();
		Blockchain::TxMsgCall calls[2] = { TokenCall(TokenContract_Methods::BurnFungible, burnArgs),
			TokenCall(TokenContract_Methods::TransferFungible, transferArgs) };
		Blockchain::TxMsg msg = BaseMsg(TxTypes::Call_Multi);
		msg.callMulti.numCalls = 2;
		msg.callMulti.calls = calls;
		FeePlan plan;
		const NativeFeeKind expected[2] = { NativeFeeKind::BurnFungible, NativeFeeKind::TransferFungible };
		const bool planned = PlanFees(msg, config, PlanFacts(), plan);
		Report(ctx, planned && KindsAre(plan, expected, 2), "a batch is planned in call order");
	}

	// An empty batch performs no operation. It still carries an envelope, and that is all it is
	// billed for.
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::Call_Multi);
		FeePlan plan;
		const bool planned = PlanFees(msg, config, PlanFacts(), plan);
		Report(ctx,
		    planned && plan.kinds.empty() && plan.maxData == 0 &&
		        plan.expectedGasBill == (uint64_t)plan.envelopeBytes * 25 * 10000,
		    "an empty batch prices its envelope alone");
	}

	// A burn returns whatever the NFT's own address holds, and that set has no costlier bound. The
	// planner demands it instead of pricing an empty address.
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::BurnNonFungible);
		msg.burnNonFungible = Blockchain::TxMsgBurnNonFungible{ kNftTokenId, 5 };
		FeePlanOptions unread{};
		FeePlanOptions read{};
		read.infusionsRead = true;
		FeePlan plan;
		Report(ctx, Refuses(msg, unread) && PlanFees(msg, config, read, plan),
		    "a burn cannot be planned until the returned assets are read");
	}

	// The returned assets cost the same wherever they are counted, so a batch of burns charges the
	// list once.
	{
		const ByteArray burnArgs = BurnNonFungibleArgs(1);
		Blockchain::TxMsgCall calls[2] = { TokenCall(TokenContract_Methods::BurnNonFungible, burnArgs),
			TokenCall(TokenContract_Methods::BurnNonFungible, burnArgs) };
		Blockchain::TxMsg msg = BaseMsg(TxTypes::Call_Multi);
		msg.callMulti.numCalls = 2;
		msg.callMulti.calls = calls;
		InfusedAsset held[1] = {};
		held[0].tokenId = 77;
		FeePlanOptions options = PlanFacts();
		options.infusions = held;
		options.numInfusions = 1;
		FeePlanOptions withoutReturns = PlanFacts();
		FeePlan withReturns, bare;
		const bool planned = PlanFees(msg, config, options, withReturns) && PlanFees(msg, config, withoutReturns, bare);
		// One returned fungible asset: a transfer plus the owner lookup of its source, and the
		// burner's new balance row.
		Report(ctx,
		    planned && withReturns.expectedGasBill == bare.expectedGasBill + (10 + 2) * 10000ull &&
		        withReturns.maxData == bare.maxData + 200000,
		    "a batch of burns charges the returned assets once");
	}

	// BurnedInstances sits beside the decomposition that decides which calls are burns, so the two
	// cannot come to disagree. A planner with a chain to ask reads each instance's own address.
	{
		PHANTASMA_VECTOR<BurnedInstance> burned;
		Blockchain::TxMsg native = BaseMsg(TxTypes::BurnNonFungible);
		native.burnNonFungible = Blockchain::TxMsgBurnNonFungible{ kNftTokenId, 5 };
		const bool nativeOk = BurnedInstances(native, burned) && burned.size() == 1 &&
		                      burned[0].tokenId == kNftTokenId && burned[0].instanceId == 5;

		const ByteArray burnArgs = BurnNonFungibleArgs(2);
		Blockchain::TxMsgCall calls[1] = { TokenCall(TokenContract_Methods::BurnNonFungible, burnArgs) };
		Blockchain::TxMsg batch = BaseMsg(TxTypes::Call_Multi);
		batch.callMulti.numCalls = 1;
		batch.callMulti.calls = calls;
		const bool batchOk = BurnedInstances(batch, burned) && burned.size() == 2 && burned[0].instanceId == 11 &&
		                     burned[1].instanceId == 12;

		Blockchain::TxMsg transfer = BaseMsg(TxTypes::TransferFungible);
		transfer.transferFt = Blockchain::TxMsgTransferFungible{ Address(2), kFungibleTokenId, 1 };
		const bool noneOk = BurnedInstances(transfer, burned) && burned.empty();

		Blockchain::TxMsg empty = BaseMsg(TxTypes::Call_Multi);
		const bool emptyOk = BurnedInstances(empty, burned) && burned.empty();

		Report(ctx, nativeOk && batchOk && noneOk && emptyOk, "BurnedInstances finds every burn a message performs");
	}

	// exact says whether the number is a prediction or a ceiling. A gas-token transfer does not
	// depend on any state fact, because the chain's own rows are free, so it is a prediction even
	// with nothing stated.
	{
		Blockchain::TxMsg gasToken = BaseMsg(TxTypes::TransferFungible);
		gasToken.transferFt = Blockchain::TxMsgTransferFungible{ Address(2), config.gasTokenId, 1 };
		Blockchain::TxMsg other = BaseMsg(TxTypes::TransferFungible);
		other.transferFt = Blockchain::TxMsgTransferFungible{ Address(2), kFungibleTokenId, 1 };
		FeePlanOptions options{};
		options.infusionsRead = true;
		FeePlan free, paid, stated;
		options.recipientHoldsToken = true;
		const bool statedOk = PlanFees(other, config, options, stated);
		options.recipientHoldsToken = false;
		const bool ok = PlanFees(gasToken, config, options, free) && PlanFees(other, config, options, paid) && statedOk;
		Report(ctx, ok && free.exact && !paid.exact && stated.exact,
		    "exact is false only when an assumed fact decided the price");
	}

	// A budgeted part makes the whole plan a ceiling, however firm the rest of it is.
	{
		const ByteArray script(8, 1);
		Blockchain::TxMsg msg = BaseMsg(TxTypes::Phantasma);
		msg.phantasma = Blockchain::TxMsgPhantasma{ SmallString("simnet"), SmallString("main"), View(script) };
		FeePlan plan;
		Report(ctx, PlanFees(msg, config, PlanFacts(), plan) && !plan.exact, "a budgeted part is never exact");
	}

	// bigFungible claims nothing about chain state: it asks the model to price the widest answer a
	// variable-length balance can have. A plan resting on it is a bound however it was arrived at.
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::BurnFungible);
		msg.burnFungible = Blockchain::TxMsgBurnFungible{};
		msg.burnFungible.tokenId = config.gasTokenId;
		msg.burnFungible.amount.x() = intx((uint64_t)1);
		FeePlanOptions wide{};
		wide.infusionsRead = true;
		FeePlanOptions narrow = wide;
		narrow.bigFungible = false;
		FeePlan bound, exact;
		const bool ok = PlanFees(msg, config, wide, bound) && PlanFees(msg, config, narrow, exact);
		Report(ctx, ok && !bound.exact && exact.exact, "a widest-answer balance makes the plan a bound");
	}

	// The planner reads the CreateToken arguments: the symbol length prices the policy fee, the
	// non-fungible flag and the metadata decide which rows the creation writes.
	{
		const ByteArray plain = CreateTokenArgs(false, false);
		const ByteArray rich = CreateTokenArgs(true, true);
		FeePlan simple, extra;
		const bool ok = PlanFees(CallMsg(TokenCall(TokenContract_Methods::CreateToken, plain)), config, PlanFacts(), simple) &&
		                PlanFees(CallMsg(TokenCall(TokenContract_Methods::CreateToken, rich)), config, PlanFacts(), extra);
		// The non-fungible flag adds the series counter row and `pre_burn` adds the burnt counter.
		Report(ctx, ok && extra.newStorageQuanta == simple.newStorageQuanta + 2,
		    "CreateToken reads its flags and metadata out of the call");
	}

	// Each instance of a deterministic Phantasma mint names the series it goes into, so the number
	// of distinct series is readable from the call and the caller never has to supply it. A
	// duplicated series reads each distinct series' supply once for the whole transaction.
	{
		const ByteArray oneSeries = MintPhantasmaArgs(4, 1);
		const ByteArray twoSeries = MintPhantasmaArgs(4, 2);
		FeePlan single, split;
		const bool ok = PlanFees(CallMsg(TokenCall(TokenContract_Methods::MintPhantasmaNonFungible, oneSeries)), config, PlanFacts(), single) &&
		                PlanFees(CallMsg(TokenCall(TokenContract_Methods::MintPhantasmaNonFungible, twoSeries)), config, PlanFacts(), split);
		Report(ctx, ok && split.expectedGasBill == single.expectedGasBill + 2ull * 10000,
		    "a Phantasma mint counts the distinct series in the call");
	}

	// A transfer into an NFT-derived address is an infusion, and the planner reads that from the
	// recipient's own address form.
	{
		Blockchain::TxMsg plain = BaseMsg(TxTypes::TransferFungible);
		plain.transferFt = Blockchain::TxMsgTransferFungible{ Address(2), kFungibleTokenId, 1 };
		Blockchain::TxMsg infused = BaseMsg(TxTypes::TransferFungible);
		infused.transferFt = Blockchain::TxMsgTransferFungible{ TokenHelper::GetNftAddress(kNftTokenId, 5), kFungibleTokenId, 1 };
		FeePlanOptions options{};
		options.infusionsRead = true;
		FeePlan ordinary, toNft;
		const bool ok = PlanFees(plain, config, options, ordinary) && PlanFees(infused, config, options, toNft);
		Report(ctx, ok && toNft.expectedGasBill == ordinary.expectedGasBill + 2ull * 10000,
		    "a transfer into an NFT address pays the owner lookup");
	}

	// The summary renders the plan in the units a person reads, dropping the trailing zeros a raw
	// atom count carries.
	{
		Report(ctx,
		    FormatTokenAmount(73000000, 10) == PHANTASMA_LITERAL("0.0073") &&
		        FormatTokenAmount(20000000000ull, 10) == PHANTASMA_LITERAL("2") &&
		        FormatTokenAmount(0, 10) == PHANTASMA_LITERAL("0") &&
		        FormatTokenAmount(200000, 8) == PHANTASMA_LITERAL("0.002") &&
		        FormatTokenAmount(5, 0) == PHANTASMA_LITERAL("5") &&
		        FormatTokenAmount(1, 19).empty(),
		    "amounts render as decimal strings without trailing zeros");

		Blockchain::TxMsg msg = BaseMsg(TxTypes::TransferFungible);
		msg.transferFt = Blockchain::TxMsgTransferFungible{ Address(2), kFungibleTokenId, 1 };
		FeePlanOptions options{};
		options.infusionsRead = true;
		FeePlan plan;
		const bool planned = PlanFees(msg, config, options, plan);
		const FeePlanSummary summary = SummarizeFeePlan(plan);
		Report(ctx,
		    planned && summary.gasBill == FormatTokenAmount(plan.expectedGasBill, 10) &&
		        summary.gasOffer == FormatTokenAmount(plan.maxGas, 10) &&
		        summary.storageCeiling == PHANTASMA_LITERAL("0.002"),
		    "a plan summary shows gas in KCAL and the deposit in SOUL");
	}

	// A malformed call cannot be priced. The chain would refuse it, and a number quoted for it would
	// be invented.
	{
		const ByteArray tooShort(4, 0);
		Report(ctx, Refuses(CallMsg(TokenCall(TokenContract_Methods::TransferFungible, tooShort)), PlanFacts()),
		    "a call whose arguments are too short is refused");
	}
}

} // namespace testcases
