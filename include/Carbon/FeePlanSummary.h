#pragma once
#ifndef PHANTASMA_API_INCLUDED
#error "Configure and include PhantasmaAPI.h first"
#endif

#include "FeePlan.h"
#include "../Domain/DomainSettings.h"

namespace phantasma::carbon {

// A fee plan in the units a person reads. Gas is shown in KCAL and the storage deposit in SOUL.
struct FeePlanSummary {
	// What the transaction will cost in gas, as a decimal KCAL amount.
	String gasBill;
	// The gas offer written into the transaction. The difference to gasBill is refunded.
	String gasOffer;
	// The storage deposit the transaction may take, as a decimal SOUL amount. The chain escrows it
	// while the transaction settles and refunds it when the rows it paid for are deleted. A wallet
	// shows it apart from the fee, as a refundable deposit.
	String storageCeiling;
};

// Renders an amount held in a token's smallest unit as a decimal string. Trailing zeros of the
// fraction are dropped, so 73000000 with 10 decimals reads "0.0073" and 20000000000 reads "2".
//
// Answers an empty string for a decimals value the chain would never admit (the token contract
// caps them at DomainSettings::MAX_TOKEN_DECIMALS), and raises PHANTASMA_EXCEPTION where exceptions
// are enabled.
inline String FormatTokenAmount(uint64_t atoms, int decimals)
{
	if( decimals < 0 || decimals > DomainSettings::MAX_TOKEN_DECIMALS )
	{
		PHANTASMA_EXCEPTION("token decimals are out of range");
		return String();
	}
	String digits;
	uint64_t value = atoms;
	do
	{
		digits.insert(digits.begin(), (Char)(PHANTASMA_LITERAL('0') + (int)(value % 10)));
		value /= 10;
	} while( value != 0 );
	// One digit in front of the decimal point at the least, so a fraction always has a whole part.
	while( (int)digits.size() <= decimals )
	{
		digits.insert(digits.begin(), PHANTASMA_LITERAL('0'));
	}

	const size_t split = digits.size() - (size_t)decimals;
	String fraction = digits.substr(split);
	while( !fraction.empty() && fraction[fraction.size() - 1] == PHANTASMA_LITERAL('0') )
	{
		fraction.resize(fraction.size() - 1);
	}
	String out = digits.substr(0, split);
	if( !fraction.empty() )
	{
		out += PHANTASMA_LITERAL(".");
		out += fraction;
	}
	return out;
}

// Renders a plan in the units a person reads.
//
// gasDecimals  - decimals of the chain's gas token. The default is what KCAL has.
// dataDecimals - decimals of the chain's data token. The default is what SOUL has.
inline FeePlanSummary SummarizeFeePlan(const FeePlan& plan, int gasDecimals = DomainSettings::FuelTokenDecimals, int dataDecimals = DomainSettings::StakingTokenDecimals)
{
	FeePlanSummary summary;
	summary.gasBill = FormatTokenAmount(plan.expectedGasBill, gasDecimals);
	summary.gasOffer = FormatTokenAmount(plan.maxGas, gasDecimals);
	summary.storageCeiling = FormatTokenAmount(plan.maxData, dataDecimals);
	return summary;
}

} // namespace phantasma::carbon
