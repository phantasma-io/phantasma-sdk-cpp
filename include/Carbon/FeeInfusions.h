#pragma once
#ifndef PHANTASMA_API_INCLUDED
#error "Configure and include PhantasmaAPI.h first"
#endif

#include "../Numerics/Base16.h"
#include "FeeEstimator.h"
#include "FeePlan.h"
#include "Tx.h"

// Reads from the chain what an NFT instance holds, in the form the fee planner prices. A burn
// returns every one of those assets to the burner, and the chain charges for each, so `PlanFees`
// demands the list and has no costlier bound to fall back on.
//
// These are free functions and not methods of PhantasmaAPI, because that class is declared in
// PhantasmaAPI.h, which every Carbon header includes, so it cannot name a Carbon type.

namespace phantasma::carbon {

// The RPC renders a token id and a balance as decimal strings. Returns false on an empty string, on
// a character that is not a digit, and on a value past 64 bits.
inline bool ParseDecimalU64(const String& text, uint64_t& out)
{
	if( text.empty() )
	{
		return false;
	}
	uint64_t value = 0;
	for( size_t i = 0; i != text.size(); ++i )
	{
		const Char c = text[i];
		if( c < (Char)'0' || c > (Char)'9' )
		{
			return false;
		}
		const uint64_t digit = (uint64_t)(c - (Char)'0');
		if( value > ((uint64_t)-1 - digit) / 10 )
		{
			return false;
		}
		value = value * 10 + digit;
	}
	out = value;
	return true;
}

// Renders a Carbon address as the 64 hex characters the account queries accept with the address
// type "Carbon". The first byte of the address is the first pair of characters, and the node reads
// upper and lower case alike.
inline String ToCarbonAddressText(const Bytes32& address)
{
	return Base16::Encode((const Byte*)address.bytes, Bytes32::length);
}

// Walks a cursor-paginated query to its end, adding every item to `out`. The cursor the node returns
// drives the loop, and an item count never does. The loop stops on a cursor it has already seen and past a page cap, so a
// node that keeps handing out pages cannot keep it going forever.
template<class T, class PageFn>
inline bool ReadAllPages(PHANTASMA_VECTOR<T>& out, PageFn page, rpc::PhantasmaError& outError)
{
	const int maxPages = 1000;
	PHANTASMA_VECTOR<String> seen;
	String cursor;
	for( int i = 0; i != maxPages; ++i )
	{
		const rpc::CursorPaginatedResult<T> result = page(cursor.c_str());
		if( outError.code != 0 )
		{
			return false;
		}
		for( size_t item = 0; item != result.result.size(); ++item )
		{
			out.push_back(result.result[item]);
		}
		if( result.cursor.empty() )
		{
			return true;
		}
		for( size_t j = 0; j != seen.size(); ++j )
		{
			if( seen[j] == result.cursor )
			{
				return true;
			}
		}
		seen.push_back(result.cursor);
		cursor = result.cursor;
	}
	outError.code = rpc::PhantasmaError::InvalidJSON;
	return false;
}

#if defined(PHANTASMA_HTTPCLIENT)

// Adds to `out` what instance `instanceId` of token `tokenId` holds at its own address, in the form
// FeePlanOptions::infusions takes. Returns false when a query failed or when the node answered with
// a number that is not one; `out` is then not to be used.
//
// Fungible balances are resolved to token ids, so the free rows of the gas and data tokens are
// recognised. Whether the burner already holds a returned token is left at the costlier reading,
// which moves the escrow ceiling alone.
inline bool ReadInfusedAssets(
    rpc::PhantasmaAPI& api, uint64_t tokenId, uint64_t instanceId, PHANTASMA_VECTOR<InfusedAsset>& out, rpc::PhantasmaError* pout_error = nullptr)
{
	rpc::PhantasmaError errorDummy;
	rpc::PhantasmaError& outError = pout_error ? *pout_error : errorDummy;
	outError = rpc::PhantasmaError();

	const String address = ToCarbonAddressText(TokenHelper::GetNftAddress(tokenId, instanceId));
	const Char* noSymbol = PHANTASMA_LITERAL("");
	const UInt32 pageSize = 100;

	PHANTASMA_VECTOR<rpc::Balance> balances;
	if( !ReadAllPages<rpc::Balance>(
	        balances,
	        [&](const Char* cursor)
	        { return api.GetAccountFungibleTokens(address.c_str(), noSymbol, 0, pageSize, cursor, false, rpc::AddressType::Carbon, &outError); },
	        outError) )
	{
		return false;
	}
	for( size_t i = 0; i != balances.size(); ++i )
	{
		const rpc::Token token = api.GetToken(balances[i].symbol.c_str(), false, &outError);
		if( outError.code != 0 )
		{
			return false;
		}
		InfusedAsset asset;
		if( !ParseDecimalU64(token.carbonId, asset.tokenId) )
		{
			outError.code = rpc::PhantasmaError::InvalidJSON;
			return false;
		}
		asset.nonFungible = false;
		out.push_back(asset);
	}

	PHANTASMA_VECTOR<rpc::Token> owned;
	if( !ReadAllPages<rpc::Token>(
	        owned,
	        [&](const Char* cursor)
	        { return api.GetAccountOwnedTokens(address.c_str(), noSymbol, 0, pageSize, cursor, false, rpc::AddressType::Carbon, &outError); },
	        outError) )
	{
		return false;
	}
	for( size_t i = 0; i != owned.size(); ++i )
	{
		// The balance of an NFT token is the number of instances the address holds, and the chain
		// charges a transfer and a lookup for each one that comes back.
		const rpc::Balance balance =
		    api.GetTokenBalance(address.c_str(), owned[i].symbol.c_str(), PHANTASMA_LITERAL("main"), false, rpc::AddressType::Carbon, &outError);
		if( outError.code != 0 )
		{
			return false;
		}
		uint64_t instanceCount = 0;
		InfusedAsset asset;
		if( !ParseDecimalU64(owned[i].carbonId, asset.tokenId) || !ParseDecimalU64(balance.amount, instanceCount) ||
		    instanceCount > (uint64_t)(uint32_t)-1 )
		{
			outError.code = rpc::PhantasmaError::InvalidJSON;
			return false;
		}
		asset.nonFungible = true;
		asset.instanceCount = (uint32_t)instanceCount;
		out.push_back(asset);
	}

	return true;
}

// Adds to `out` what every instance a message burns holds. `BurnedInstances` names those instances,
// so this is the whole of what FeePlanOptions::infusions needs for that message. Set
// FeePlanOptions::infusionsRead when it returns true; an empty list then states that the instances
// hold nothing.
inline bool ReadInfusedAssets(
    rpc::PhantasmaAPI& api, const Blockchain::TxMsg& msg, PHANTASMA_VECTOR<InfusedAsset>& out, rpc::PhantasmaError* pout_error = nullptr)
{
	rpc::PhantasmaError errorDummy;
	rpc::PhantasmaError& outError = pout_error ? *pout_error : errorDummy;
	outError = rpc::PhantasmaError();

	PHANTASMA_VECTOR<BurnedInstance> instances;
	if( !BurnedInstances(msg, instances) )
	{
		// A burn call whose arguments are too short to read names no instance to ask about.
		outError.code = rpc::PhantasmaError::InvalidJSON;
		return false;
	}
	for( size_t i = 0; i != instances.size(); ++i )
	{
		if( !ReadInfusedAssets(api, instances[i].tokenId, instances[i].instanceId, out, pout_error) )
		{
			return false;
		}
	}
	return true;
}

#endif

} // namespace phantasma::carbon
