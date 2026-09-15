#pragma once
#ifndef PHANTASMA_API_INCLUDED
#error "Configure and include PhantasmaAPI.h first"
#endif

#include "Contracts/Token.h"
#include "FeePlan.h"
#include "Tx.h"

// The check a caller runs BEFORE signing a token creation.
//
// `Token.CreateToken` is charged its policy fee before the contract looks at the symbol, and that
// fee is the largest single price in the protocol. A creation whose symbol is already taken pays it
// and gets nothing back. The check costs one query and no fee at all.
//
// It answers about the symbol only. Every other message is not applicable.

namespace phantasma::carbon {

enum class PreflightVerdict
{
	// The message is not a token creation, so there is nothing to check.
	NotApplicable,
	// The symbol resolves to a token that exists. Sending this message would pay the policy fee for
	// nothing.
	Taken,
	// The symbol resolves to nothing, and the node answered a control lookup, so the answer is the
	// chain's and not a broken connection.
	Free,
	// The node did not answer well enough to tell the two apart. `reason` says what came back.
	Unknown,
};

struct PreflightResult {
	PreflightVerdict verdict = PreflightVerdict::NotApplicable;
	// The symbol the verdict is about, empty when the message is not a token creation.
	String subject;
	// Why the answer is Unknown.
	String reason;
};

// Reads the symbol a CreateToken call would register. Answers false when the message is not a token
// creation, or when its arguments cannot be read.
inline bool PreflightSubject(const Blockchain::TxMsg& msg, String& outSymbol)
{
	if( msg.type != Blockchain::TxTypes::Call )
	{
		return false;
	}
	if( msg.call.moduleId != (uint32_t)ModuleId::Token || msg.call.methodId != (uint32_t)TokenContract_Methods::CreateToken )
	{
		return false;
	}

	Allocator alloc;
	ReadView r(msg.call.args, alloc, ReadView::InPlace);
	TokenInfo info{};
	if( !Read(info, r, alloc) || info.symbol.length == 0 )
	{
		return false;
	}
	outSymbol = String(info.symbol.bytes, info.symbol.bytes + info.symbol.length);
	return true;
}

#if defined(PHANTASMA_HTTPCLIENT)

// Asks the chain whether the symbol this message would register is taken. `controlTokenId` is a
// token id the chain is known to hold, and it decides the difference between "the symbol is free"
// and "this node answers nothing": a lookup that fails for the symbol AND for a token that exists
// says the node is the problem, not the symbol. Pass the gas token's id, which every chain has.
inline PreflightResult PreflightTransaction(rpc::PhantasmaAPI& api, const Blockchain::TxMsg& msg, uint64_t controlTokenId)
{
	PreflightResult result;
	String symbol;
	if( !PreflightSubject(msg, symbol) )
	{
		return result;
	}
	result.subject = symbol;

	rpc::PhantasmaError error;
	api.GetToken(symbol.c_str(), false, &error);
	if( error.code == 0 )
	{
		// A token came back, so the symbol resolves to one. Nothing else about it is read: the
		// question was only whether it exists.
		result.verdict = PreflightVerdict::Taken;
		return result;
	}
	const String tokenReason = error.message;

	if( controlTokenId == 0 )
	{
		result.verdict = PreflightVerdict::Unknown;
		result.reason = tokenReason;
		return result;
	}

	rpc::PhantasmaError controlError;
	api.GetToken(PHANTASMA_LITERAL(""), false, controlTokenId, &controlError);
	if( controlError.code == 0 )
	{
		result.verdict = PreflightVerdict::Free;
		return result;
	}
	result.verdict = PreflightVerdict::Unknown;
	result.reason = tokenReason;
	return result;
}

#endif

} // namespace phantasma::carbon
