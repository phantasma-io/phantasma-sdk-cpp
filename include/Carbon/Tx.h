#pragma once
#ifndef PHANTASMA_API_INCLUDED
#error "Configure and include PhantasmaAPI.h first"
#endif

#include "../Numerics/Base16.h"
#include <chrono>
#include <utility>
#include <limits>
#include "DataBlockchain.h"
#include "Modules.h"

namespace phantasma::carbon {

enum class ModuleId : uint32_t
{
	Internal = 0xFFFFFFFFu,
	Governance = 0u,
	Token = 1u,
	PhantasmaVm = 2u,
	Organization = 3u,
};

enum class TokenContract_Methods : uint32_t
{
	TransferFungible = 0,
	TransferNonFungible = 1,
	CreateToken = 2,
	MintFungible = 3,
	BurnFungible = 4,
	GetBalance = 5,
	CreateTokenSeries = 6,
	DeleteTokenSeries = 7,
	MintNonFungible = 8,
	BurnNonFungible = 9,
	GetInstances = 10,
	GetNonFungibleInfo = 11,
	GetNonFungibleInfoByRomId = 12,
	GetSeriesInfo = 13,
	GetSeriesInfoByMetaId = 14,
	GetTokenInfo = 15,
	GetTokenInfoBySymbol = 16,
	GetTokenSupply = 17,
	GetSeriesSupply = 18,
	GetTokenIdBySymbol = 19,
	GetBalances = 20,
	CreateMintedTokenSeries = 21,
	ApplyInflation = 22,
	UpdateTokenMetadata = 23,
	GetNextTokenInflation = 24,
	SetTokensConfig = 25,
	UpdateSeriesMetadata = 26,
	MintPhantasmaNonFungible = 27,
};

// The governance module's methods, as the contract declares them.
enum class GovernanceContract_Methods : uint32_t
{
	Genesis = 0,
	RegisterName = 1,
	SpecialResolution = 2,
	SetGasConfig = 3,
	SetChainConfig = 4,
	SetMetadata = 5,
	SetNodeConfig = 6,
	GetSpecialResolutionCount = 7,
	LookupName = 8,
	LookupAddress = 9,
	SetFeatureLevel = 10,
	RepairStakingOrganizationMembership = 11,
	MigrateAddresses = 12,
};

struct TokenHelper {
	static Bytes32 GetNftAddress(uint64_t carbonTokenId, uint64_t instanceId)
	{
		uint8_t prefix[16] = {};
		prefix[15] = 1;

		ByteArray buffer;
		WriteView w(buffer);
		Write16(prefix, sizeof(prefix), w);
		Write8u(carbonTokenId, w);
		Write8u(instanceId, w);
		return Bytes32(View(buffer));
	}

	// Returns true if a 32-byte address is an NFT-derived address. Every minted instance owns such
	// an address, and assets infused into that NFT are sent to it.
	//
	// The test is syntactic and is the one the chain applies (carbon::IsNftAddress): fifteen zero
	// bytes, a 0x01 marker, then a nonzero token id and a nonzero instance id. PlanFees uses it to
	// price the recipient's owner lookup, which a transfer into such an address pays.
	static bool IsNftAddress(const Bytes32& address)
	{
		if( address.bytes[15] != 1 )
			return false;
		for( int i = 0; i != 15; ++i )
		{
			if( address.bytes[i] != 0 )
				return false;
		}
		uint64_t carbonTokenId = 0, instanceId = 0;
		memcpy(&carbonTokenId, address.bytes + 16, sizeof(carbonTokenId));
		memcpy(&instanceId, address.bytes + 24, sizeof(instanceId));
		return carbonTokenId != 0 && instanceId != 0;
	}
};

struct TxEnvelope {
	phantasma::carbon::Blockchain::TxMsg msg{};
	std::vector<ByteArray> buffers;

	const phantasma::carbon::Blockchain::TxMsg& View() const { return msg; }
};

// Explicit transaction limits a builder writes into the message.
//
// Builders carry no prices. A message built without maxGas has a zero gas offer, and that marks it
// as not yet planned. Plan it with PlanFees (Carbon/FeePlan.h) before signing, or set the offer
// here. Under gas model v2 the chain bills every byte the transaction puts in the block, and no
// fixed number predicts that.
struct TxLimits {
	// Gas offer in kcal-base (TxMsg maxGas). Zero means unplanned.
	uint64_t maxGas = 0;
	// Storage-escrow ceiling in data-token atoms (TxMsg maxData).
	uint64_t maxData = 0;
	// Expiry as a millisecond timestamp (TxMsg expiry). Zero means DefaultExpiryMs from now.
	//
	// A flow with a person in it should set this from the chain's own window instead. Examples are a
	// hardware wallet confirming and a wallet-link round trip. See ExpiryWithin.
	int64_t expiry = 0;
};

// Default lifetime of a message a builder stamps, in milliseconds.
//
// The chain reads expiry in milliseconds and refuses anything at or beyond now + expiryWindow.
// expiryWindow is a chain setting, and its node default is 60,000 ms.
//
// A default has to hold on the shortest window a chain may run. The value is also compared against
// the NODE's clock, so it has to survive the two clocks disagreeing. That is why it keeps a quarter
// of a minute of headroom and does not take the whole 60,000.
//
// A chain that allows longer reports its own window as expiryWindow in getGasConfig.
constexpr int64_t DefaultExpiryMs = 45000;

inline int64_t UnixTimeMs()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// Returns the expiry for a message built to be signed and sent now.
inline int64_t DefaultExpiry()
{
	return UnixTimeMs() + DefaultExpiryMs;
}

// Returns the latest expiry a chain with this window will still admit, less a margin for the clock
// the node compares it against. Use it when a person sits between building a transaction and
// signing it. The chain's window is usually far longer than DefaultExpiryMs, and the whole of it is
// available.
//
// expiryWindowMs - the chain's window, from getGasConfig.
// marginMs       - headroom for clock skew and the trip to the node.
//
// Answers 0 for a window a margin leaves nothing of.
inline int64_t ExpiryWithin(int64_t expiryWindowMs, int64_t marginMs = 5000)
{
	if( expiryWindowMs <= 0 || marginMs < 0 || expiryWindowMs - marginMs <= 0 )
	{
		PHANTASMA_EXCEPTION("expiry window must leave a positive lifetime after the margin");
		return 0;
	}
	return UnixTimeMs() + (expiryWindowMs - marginMs);
}

namespace TxLimitsDetail {

// Writes the limits into a message the builders assemble.
inline void Apply(phantasma::carbon::Blockchain::TxMsg& msg, const TxLimits& limits)
{
	msg.maxGas = limits.maxGas;
	msg.maxData = limits.maxData;
	msg.expiry = limits.expiry == 0 ? DefaultExpiry() : limits.expiry;
}

} // namespace TxLimitsDetail

// Builders for the native transaction types: the transfers, mints and burns that need no VM script.
//
// A builder assembles the message and nothing else. The fee is planned afterwards from the message
// itself with PlanFees, and the witnesses sign with TxMsgSigner. Without a plan the message carries
// a zero offer, which no chain admits.
//
// The native types come in pairs. In the plain form the account whose tokens move also pays the gas
// and signs alone. In the _GasPayer form a second account pays the gas and both sign, the payer
// first. Naming a gas payer selects the second form.
struct NativeTxHelper {
	// The account whose tokens move, and optionally the account that pays the gas instead.
	struct Parties {
		Bytes32 from{};
		const Bytes32* gasPayer = nullptr;
	};

	// Moves `amount` atoms of a fungible token. A big-fungible token whose balances pass int64 needs
	// a module call instead, because this message carries the amount as a u64.
	static Blockchain::TxMsg TransferFungible(
	    const Parties& parties, const Bytes32& to, uint64_t tokenId, uint64_t amount, const TxLimits& limits = {})
	{
		if( parties.gasPayer )
		{
			Blockchain::TxMsg msg = Base(Blockchain::TxTypes::TransferFungible_GasPayer, *parties.gasPayer, limits);
			msg.transferFtGasPayer = Blockchain::TxMsgTransferFungible_GasPayer{ to, parties.from, tokenId, amount };
			return msg;
		}
		Blockchain::TxMsg msg = Base(Blockchain::TxTypes::TransferFungible, parties.from, limits);
		msg.transferFt = Blockchain::TxMsgTransferFungible{ to, tokenId, amount };
		return msg;
	}

	// Moves whole NFT instances. One instance uses the single-instance type and several use the
	// multi-instance one, which is what the chain prices differently. `instanceIds` must outlive the
	// message: the multi-instance forms point at it.
	//
	// Answers false without touching `out` when no instance is named.
	static bool TransferNonFungible(
	    const Parties& parties,
	    const Bytes32& to,
	    uint64_t tokenId,
	    const uint64_t* instanceIds,
	    uint32_t numInstanceIds,
	    Blockchain::TxMsg& out,
	    std::string& outError,
	    const TxLimits& limits = {})
	{
		if( numInstanceIds == 0 || instanceIds == nullptr )
		{
			PHANTASMA_EXCEPTION("instanceIds must not be empty");
			outError = "instanceIds must not be empty";
			return false;
		}
		const bool single = numInstanceIds == 1;
		if( parties.gasPayer )
		{
			Blockchain::TxMsg msg = Base(
			    single ? Blockchain::TxTypes::TransferNonFungible_Single_GasPayer
			           : Blockchain::TxTypes::TransferNonFungible_Multi_GasPayer,
			    *parties.gasPayer, limits);
			if( single )
			{
				msg.transferNftSingleGasPayer =
				    Blockchain::TxMsgTransferNonFungible_Single_GasPayer{ to, parties.from, tokenId, instanceIds[0] };
			}
			else
			{
				msg.transferNftMultiGasPayer = Blockchain::TxMsgTransferNonFungible_Multi_GasPayer{};
				msg.transferNftMultiGasPayer.to = to;
				msg.transferNftMultiGasPayer.from = parties.from;
				msg.transferNftMultiGasPayer.tokenId = tokenId;
				msg.transferNftMultiGasPayer.numInstanceIds = numInstanceIds;
				msg.transferNftMultiGasPayer.instanceIds = instanceIds;
			}
			out = msg;
			return true;
		}
		Blockchain::TxMsg msg = Base(
		    single ? Blockchain::TxTypes::TransferNonFungible_Single : Blockchain::TxTypes::TransferNonFungible_Multi,
		    parties.from, limits);
		if( single )
		{
			msg.transferNftSingle = Blockchain::TxMsgTransferNonFungible_Single{ to, tokenId, instanceIds[0] };
		}
		else
		{
			msg.transferNftMulti = Blockchain::TxMsgTransferNonFungible_Multi{};
			msg.transferNftMulti.to = to;
			msg.transferNftMulti.tokenId = tokenId;
			msg.transferNftMulti.numInstanceIds = numInstanceIds;
			msg.transferNftMulti.instanceIds = instanceIds;
		}
		out = msg;
		return true;
	}

	// Mints fungible atoms. The token owner pays the gas and signs. The amount is an intx because a
	// mint also serves a big-fungible token, whose balances do not fit a u64.
	static Blockchain::TxMsg MintFungible(
	    const Bytes32& owner, const Bytes32& to, uint64_t tokenId, const intx& amount, const TxLimits& limits = {})
	{
		Blockchain::TxMsg msg = Base(Blockchain::TxTypes::MintFungible, owner, limits);
		msg.mintFungible = Blockchain::TxMsgMintFungible{};
		msg.mintFungible.tokenId = tokenId;
		msg.mintFungible.to = to;
		msg.mintFungible.amount.x() = amount;
		return msg;
	}

	// Burns fungible atoms out of `parties.from`.
	static Blockchain::TxMsg BurnFungible(
	    const Parties& parties, uint64_t tokenId, const intx& amount, const TxLimits& limits = {})
	{
		if( parties.gasPayer )
		{
			Blockchain::TxMsg msg = Base(Blockchain::TxTypes::BurnFungible_GasPayer, *parties.gasPayer, limits);
			msg.burnFungibleGasPayer = Blockchain::TxMsgBurnFungible_GasPayer{};
			msg.burnFungibleGasPayer.tokenId = tokenId;
			msg.burnFungibleGasPayer.from = parties.from;
			msg.burnFungibleGasPayer.amount.x() = amount;
			return msg;
		}
		Blockchain::TxMsg msg = Base(Blockchain::TxTypes::BurnFungible, parties.from, limits);
		msg.burnFungible = Blockchain::TxMsgBurnFungible{};
		msg.burnFungible.tokenId = tokenId;
		msg.burnFungible.amount.x() = amount;
		return msg;
	}

	// Burns one NFT instance. Whatever that instance holds at its own address comes back to the
	// burner, and the chain charges for each returned asset, which PlanFees prices from the list the
	// caller reads with ReadInfusedAssets.
	static Blockchain::TxMsg BurnNonFungible(
	    const Parties& parties, uint64_t tokenId, uint64_t instanceId, const TxLimits& limits = {})
	{
		if( parties.gasPayer )
		{
			Blockchain::TxMsg msg = Base(Blockchain::TxTypes::BurnNonFungible_GasPayer, *parties.gasPayer, limits);
			msg.burnNonFungibleGasPayer = Blockchain::TxMsgBurnNonFungible_GasPayer{ tokenId, parties.from, instanceId };
			return msg;
		}
		Blockchain::TxMsg msg = Base(Blockchain::TxTypes::BurnNonFungible, parties.from, limits);
		msg.burnNonFungible = Blockchain::TxMsgBurnNonFungible{ tokenId, instanceId };
		return msg;
	}

  private:
	static Blockchain::TxMsg Base(Blockchain::TxTypes type, const Bytes32& gasFrom, const TxLimits& limits)
	{
		Blockchain::TxMsg msg;
		msg.type = type;
		msg.gasFrom = gasFrom;
		msg.payload = SmallString();
		TxLimitsDetail::Apply(msg, limits);
		return msg;
	}
};

struct CreateTokenTxHelper {
	static TxEnvelope BuildTx(const TokenInfo& tokenInfo, const Bytes32& creatorPublicKey, const TxLimits& limits = {})
	{
		TxEnvelope env;
		env.buffers.push_back(CarbonSerialize(tokenInfo));

		env.msg.type = phantasma::carbon::Blockchain::TxTypes::Call;
		TxLimitsDetail::Apply(env.msg, limits);
		env.msg.gasFrom = creatorPublicKey;
		env.msg.payload = SmallString();
		env.msg.call = phantasma::carbon::Blockchain::TxMsgCall{
			(uint32_t)ModuleId::Token,
			(uint32_t)TokenContract_Methods::CreateToken,
			ByteView{ env.buffers.back().data(), env.buffers.back().size() },
			{}
		};
		return env;
	}

	static uint32_t ParseResult(const std::string& resultHex)
	{
		ByteArray bytes = Base16::Decode(resultHex.c_str(), (int)resultHex.size());
		ReadView r(bytes.empty() ? nullptr : &bytes.front(), bytes.size());
		return (uint32_t)Read4u(r);
	}
};

struct CreateTokenSeriesTxHelper {
	static TxEnvelope BuildTx(uint64_t tokenId, const SeriesInfo& seriesInfo, const Bytes32& creatorPublicKey, const TxLimits& limits = {})
	{
		TxEnvelope env;

		ByteArray argsBuffer;
		WriteView argsWriter(argsBuffer);
		Write8u(tokenId, argsWriter);
		Write(seriesInfo, argsWriter);
		env.buffers.push_back(argsBuffer);

		env.msg.type = phantasma::carbon::Blockchain::TxTypes::Call;
		TxLimitsDetail::Apply(env.msg, limits);
		env.msg.gasFrom = creatorPublicKey;
		env.msg.payload = SmallString();
		env.msg.call = phantasma::carbon::Blockchain::TxMsgCall{
			(uint32_t)ModuleId::Token,
			(uint32_t)TokenContract_Methods::CreateTokenSeries,
			ByteView{ env.buffers.back().data(), env.buffers.back().size() },
			{}
		};
		return env;
	}

	static uint32_t ParseResult(const std::string& resultHex)
	{
		ByteArray bytes = Base16::Decode(resultHex.c_str(), (int)resultHex.size());
		ReadView r(bytes.empty() ? nullptr : &bytes.front(), bytes.size());
		return (uint32_t)Read4u(r);
	}
};

struct MintNonFungibleTxHelper {
	static TxEnvelope BuildTx(
	    uint64_t tokenId,
	    uint32_t seriesId,
	    const Bytes32& senderPublicKey,
	    const Bytes32& receiverPublicKey,
	    const ByteArray& rom,
	    const ByteArray& ram,
	    const TxLimits& limits = {})
	{
		TxEnvelope env;
		env.msg.type = phantasma::carbon::Blockchain::TxTypes::MintNonFungible;
		TxLimitsDetail::Apply(env.msg, limits);
		env.msg.gasFrom = senderPublicKey;
		env.msg.payload = SmallString();
		env.msg.mintNonFungible = phantasma::carbon::Blockchain::TxMsgMintNonFungible{
			tokenId,
			receiverPublicKey,
			seriesId,
			ByteView{ rom.data(), rom.size() },
			ByteView{ ram.data(), ram.size() }
		};
		return env;
	}

	static std::vector<Bytes32> ParseResult(uint64_t carbonTokenId, const std::string& resultHex)
	{
		ByteArray bytes = Base16::Decode(resultHex.c_str(), (int)resultHex.size());
		ReadView r(bytes.empty() ? nullptr : &bytes.front(), bytes.size());
		std::vector<Bytes32> result;
		const uint32_t count = (uint32_t)ReadLengthFor(r, sizeof(uint64_t));
		result.reserve(count);
		for( uint32_t i = 0; i != count; ++i )
		{
			const uint64_t instanceId = Read8u(r);
			result.push_back(TokenHelper::GetNftAddress(carbonTokenId, instanceId));
		}
		return result;
	}
};

struct MintPhantasmaNonFungibleTxHelper {
	// Answers false without touching `out` when the token list is empty or absent. The envelope is
	// returned through `out` because a builder that can refuse needs somewhere to say so.
	static bool BuildTx(
	    uint64_t tokenId,
	    const Bytes32& senderPublicKey,
	    const Bytes32& receiverPublicKey,
	    uint32_t numTokens,
	    const PhantasmaNftMintInfo* tokens,
	    TxEnvelope& out,
	    std::string& outError,
	    const TxLimits& limits = {})
	{
		if( numTokens == 0 )
		{
			PHANTASMA_EXCEPTION("tokens must not be empty");
			outError = "tokens must not be empty";
			return false;
		}
		if( tokens == nullptr )
		{
			PHANTASMA_EXCEPTION("tokens is required when numTokens > 0");
			outError = "tokens is required when numTokens > 0";
			return false;
		}

		TxEnvelope env;

		ByteArray argsBuffer;
		WriteView argsWriter(argsBuffer);
		Write8u(tokenId, argsWriter);
		Write(receiverPublicKey, argsWriter);
		Write4((int32_t)numTokens, argsWriter);
		for( uint32_t i = 0; i != numTokens; ++i )
		{
			Write(tokens[i], argsWriter);
		}
		env.buffers.push_back(argsBuffer);

		env.msg.type = phantasma::carbon::Blockchain::TxTypes::Call;
		TxLimitsDetail::Apply(env.msg, limits);
		env.msg.gasFrom = senderPublicKey;
		env.msg.payload = SmallString();
		env.msg.call = phantasma::carbon::Blockchain::TxMsgCall{
			(uint32_t)ModuleId::Token,
			(uint32_t)TokenContract_Methods::MintPhantasmaNonFungible,
			ByteView{ env.buffers.back().data(), env.buffers.back().size() },
			{}
		};
		// Moved, never copied: env.msg.call.args is a view into env.buffers, and a copy of the vector
		// would leave that view pointing at the buffer of the envelope it was copied from.
		out = std::move(env);
		return true;
	}

	static bool BuildTx(
	    uint64_t tokenId,
	    const Bytes32& senderPublicKey,
	    const Bytes32& receiverPublicKey,
	    const std::vector<PhantasmaNftMintInfo>& tokens,
	    TxEnvelope& out,
	    std::string& outError,
	    const TxLimits& limits = {})
	{
		return BuildTx(tokenId, senderPublicKey, receiverPublicKey, (uint32_t)tokens.size(), tokens.empty() ? nullptr : &tokens.front(), out, outError, limits);
	}

	static std::vector<PhantasmaNftMintResult> ParseResult(const std::string& resultHex)
	{
		ByteArray bytes = Base16::Decode(resultHex.c_str(), (int)resultHex.size());
		ReadView r(bytes.empty() ? nullptr : &bytes.front(), bytes.size());
		std::vector<PhantasmaNftMintResult> results;
		const uint32_t count = (uint32_t)ReadLengthFor(r, Bytes32::length + sizeof(uint64_t));
		results.resize(count);
		for( uint32_t i = 0; i != count; ++i )
		{
			Read(results[i].phantasmaNftId, r);
			results[i].carbonInstanceId = Read8u(r);
		}
		return results;
	}
};

inline ByteArray SignAndSerialize(const TxEnvelope& env, const PhantasmaKeys& keys)
{
	return phantasma::carbon::Blockchain::TxMsgSigner::SignAndSerialize(env.msg, keys);
}

} // namespace phantasma::carbon
