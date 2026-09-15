#pragma once
#ifndef PHANTASMA_API_INCLUDED
#error "Configure and include PhantasmaAPI.h first"
#endif

#include "FeeInfusions.h"
#include "FeePlan.h"
#include "Preflight.h"
#include "Tx.h"

// Build, plan, check, sign and send in one call.
//
// The four steps stay available on their own: a hardware wallet or a multi-party flow needs the
// message and the plan in its own hands. This is the path for a caller that has neither.

#if defined(PHANTASMA_HTTPCLIENT)

namespace phantasma::carbon {

struct SendOptions {
	// The facts the planner cannot read out of the message. Leave a fact unstated and it is taken at
	// the reading that costs more, which always covers.
	FeePlanOptions fees;
	// Ask the chain whether a token creation's symbol is free before anything is signed. The check
	// costs one query and no fee, and it protects the largest single price in the protocol.
	bool preflight = true;
	// A token id the chain is known to hold, used by that check to tell a free symbol from a node
	// that answers nothing. The gas token's id is the obvious one.
	uint64_t controlTokenId = 0;
	// Read what every burned NFT holds at its own address, which the planner demands and has no
	// costlier bound for. Leave it on unless the caller has filled `fees.infusions` itself.
	bool readInfusions = true;
};

// Returns the transaction hash the node accepted. Answers false without sending when the pre-flight
// refuses the message, when the plan cannot be made, or when the signers do not match what the
// message names; `outError` then says which.
//
// `config` is the chain's gas configuration. Read it once with GetGasConfig and keep it: it changes
// only when a governance resolution changes it.
inline bool SendTransaction(
    rpc::PhantasmaAPI& api,
    const Blockchain::TxMsg& msg,
    const std::vector<const PhantasmaKeys*>& signers,
    const Blockchain::GasConfig& config,
    const SendOptions& options,
    String& outHash,
    std::string& outError)
{
	if( options.preflight )
	{
		const PreflightResult check = PreflightTransaction(api, msg, options.controlTokenId);
		if( check.verdict == PreflightVerdict::Taken )
		{
			outError = std::string("token symbol ") + check.subject.c_str() + " is already taken";
			PHANTASMA_EXCEPTION(outError.c_str());
			return false;
		}
	}

	FeePlanOptions fees = options.fees;
	PHANTASMA_VECTOR<InfusedAsset> infusions;
	if( options.readInfusions && !fees.infusionsRead )
	{
		rpc::PhantasmaError infusionError;
		if( !ReadInfusedAssets(api, msg, infusions, &infusionError) )
		{
			outError = std::string("the assets the burned instances hold could not be read: ") + infusionError.message.c_str();
			PHANTASMA_EXCEPTION(outError.c_str());
			return false;
		}
		fees.infusions = infusions.empty() ? nullptr : &infusions.front();
		fees.numInfusions = (uint32_t)infusions.size();
		fees.infusionsRead = true;
	}

	// Only the witness-array types take a witness count, and the signer list is the answer. For
	// every other type the message fixes its own slots.
	uint32_t required = 0;
	if( !Blockchain::RequiredWitnessCount(msg.type, required) && fees.witnessCount == 0 )
	{
		fees.witnessCount = (uint32_t)signers.size();
	}

	FeePlan plan;
	if( !PlanFees(msg, config, fees, plan) )
	{
		outError = "the message cannot be priced";
		return false;
	}

	ByteArray envelope;
	if( !Blockchain::TxMsgSigner::SignAndSerialize(plan.Apply(msg), signers, envelope, outError) )
	{
		return false;
	}

	rpc::PhantasmaError sendError;
	outHash = api.SendCarbonTransaction(Base16::Encode(envelope.data(), (int)envelope.size()).c_str(), &sendError);
	if( sendError.code != 0 )
	{
		outError = std::string("the node refused the envelope: ") + sendError.message.c_str();
		return false;
	}
	return true;
}

} // namespace phantasma::carbon

#endif
