#include "test_cases.h"

namespace testcases {
using namespace testutil;

// Asking the account queries about an address by its TYPE, and the reader that uses it.
//
// An account query takes the address as text, and the node decodes that text as a Phantasma address
// or as a 64-hex Carbon account key, whichever `addressType` names. An NFT instance holds its infused
// assets at its own Carbon address, so the fee planner can only assemble the list of what a burn
// returns when the queries carry that parameter.

namespace {

Bytes32 Address(uint8_t seed)
{
	Bytes32 out{};
	for( int i = 0; i != Bytes32::length; ++i )
	{
		out.bytes[i] = (uint8_t)(seed + i);
	}
	return out;
}

// The parameter goes last in every one of these requests, which is where the node reads it, and the
// rest of the array keeps the order and the values it had.
void RunRequestTests(TestContext& ctx)
{
	{
		JSONBuilder request;
		rpc::PhantasmaJsonAPI::MakeGetTokenBalanceRequest(request, "ADDR", "SOUL", "main", false, rpc::AddressType::Carbon);
		ReportJsonRpcRequest(
		    ctx,
		    request,
		    "getTokenBalance",
		    "[\"ADDR\", \"SOUL\", \"main\", false, \"Carbon\"]",
		    "API GetTokenBalance builder sends the address type last");
	}
	{
		JSONBuilder request;
		rpc::PhantasmaJsonAPI::MakeGetAccountFungibleTokensRequest(request, "ADDR", "", 0, 100, "cursor", false, rpc::AddressType::Carbon);
		ReportJsonRpcRequest(
		    ctx,
		    request,
		    "getAccountFungibleTokens",
		    "[\"ADDR\", \"\", 0, 100, \"cursor\", false, \"Carbon\"]",
		    "API GetAccountFungibleTokens builder sends the address type last");
	}
	{
		JSONBuilder request;
		rpc::PhantasmaJsonAPI::MakeGetAccountNFTsRequest(request, "ADDR", "", 0, 0, 100, "cursor", false, false, rpc::AddressType::Carbon);
		ReportJsonRpcRequest(
		    ctx,
		    request,
		    "getAccountNFTs",
		    "[\"ADDR\", \"\", 0, 0, 100, \"cursor\", false, false, \"Carbon\"]",
		    "API GetAccountNFTs builder sends the address type last");
	}
	{
		JSONBuilder request;
		rpc::PhantasmaJsonAPI::MakeGetAccountOwnedTokensRequest(request, "ADDR", "", 0, 100, "cursor", false, rpc::AddressType::Carbon);
		ReportJsonRpcRequest(
		    ctx,
		    request,
		    "getAccountOwnedTokens",
		    "[\"ADDR\", \"\", 0, 100, \"cursor\", false, \"Carbon\"]",
		    "API GetAccountOwnedTokens builder sends the address type last");
	}
	{
		JSONBuilder request;
		rpc::PhantasmaJsonAPI::MakeGetAccountOwnedTokenSeriesRequest(request, "ADDR", "", 0, 100, "cursor", false, rpc::AddressType::Carbon);
		ReportJsonRpcRequest(
		    ctx,
		    request,
		    "getAccountOwnedTokenSeries",
		    "[\"ADDR\", \"\", 0, 100, \"cursor\", false, \"Carbon\"]",
		    "API GetAccountOwnedTokenSeries builder sends the address type last");
	}
	{
		// The overload without the parameter still sends the array it always sent. An older node
		// rejects a request with a parameter it does not declare, so this one has to stay as it is.
		JSONBuilder request;
		rpc::PhantasmaJsonAPI::MakeGetAccountFungibleTokensRequest(request, "ADDR", "", 0, 100, "cursor", true);
		ReportJsonRpcRequest(
		    ctx,
		    request,
		    "getAccountFungibleTokens",
		    "[\"ADDR\", \"\", 0, 100, \"cursor\", true]",
		    "API GetAccountFungibleTokens builder without an address type is unchanged");
	}
	{
		// The default type is the one an ordinary base58 address needs, and it has to reach the wire
		// as its own word. The node reads the word case-insensitively, so this pins the spelling and
		// not the node's tolerance.
		JSONBuilder request;
		rpc::PhantasmaJsonAPI::MakeGetAccountOwnedTokensRequest(request, "ADDR", "", 0, 100, "cursor", true, rpc::AddressType::Phantasma);
		ReportJsonRpcRequest(
		    ctx,
		    request,
		    "getAccountOwnedTokens",
		    "[\"ADDR\", \"\", 0, 100, \"cursor\", true, \"Phantasma\"]",
		    "the Phantasma address type reaches the wire as its own word");
	}
	{
		// The two entry points that shipped with the address type as text keep that form and gained
		// a checked one. Both have to build the same request.
		JSONBuilder fromEnum;
		rpc::PhantasmaJsonAPI::MakeGetOrganizationMemberRequest(fromEnum, "masters", "ADDR", false, rpc::AddressType::Carbon);
		ReportJsonRpcRequest(
		    ctx,
		    fromEnum,
		    "getOrganizationMember",
		    "[\"masters\", \"ADDR\", false, \"Carbon\"]",
		    "API GetOrganizationMember builder takes the address type as a value");
	}
}

void RunAddressTextTests(TestContext& ctx)
{
	{
		// The node reads these 64 characters back into the same 32 bytes, in the same order, so the
		// first byte of the address is the first pair of characters.
		const String text = ToCarbonAddressText(Address(0));
		Report(ctx, text == String("000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F"), "a Carbon address renders as 64 hex characters", text);
	}
	{
		// A real NFT address, as a node reported it: this is the `carbonNftAddress` a localnet
		// returned on 2026-09-14 for carbon token 114, instance 4294967297, and it is the address
		// that NFT's infused assets sit at. The SDK has to derive the same 32 bytes and spell them
		// the same way, or the account queries are asked about the wrong account.
		const String text = ToCarbonAddressText(TokenHelper::GetNftAddress(114, 4294967297ull));
		Report(ctx, text == String("0000000000000000000000000000000172000000000000000100000001000000"), "an NFT instance address matches the one the node reports", text);
	}
}

void RunParseDecimalTests(TestContext& ctx)
{
	uint64_t value = 0;
	Report(ctx, ParseDecimalU64(String("0"), value) && value == 0, "a decimal zero is read");
	Report(ctx, ParseDecimalU64(String("18446744073709551615"), value) && value == 18446744073709551615ull, "the largest 64-bit decimal is read");
	Report(ctx, !ParseDecimalU64(String(""), value), "an empty string is refused");
	Report(ctx, !ParseDecimalU64(String("12x4"), value), "a string with a character that is not a digit is refused");
	Report(ctx, !ParseDecimalU64(String("-1"), value), "a negative number is refused");
	Report(ctx, !ParseDecimalU64(String("18446744073709551616"), value), "a number past 64 bits is refused");
}

// A page of a cursor-paginated query, as the node would answer it.
rpc::CursorPaginatedResult<String> Page(const char* item, const char* nextCursor)
{
	rpc::CursorPaginatedResult<String> page;
	page.result.push_back(String(item));
	page.cursor = String(nextCursor);
	return page;
}

void RunPagingTests(TestContext& ctx)
{
	{
		// Three pages, and the last one ends the walk by returning no cursor.
		PHANTASMA_VECTOR<String> items;
		rpc::PhantasmaError error;
		int calls = 0;
		const bool ok = ReadAllPages<String>(
		    items,
		    [&](const Char* cursor)
		    {
			    ++calls;
			    if( String(cursor).empty() )
				    return Page("a", "c1");
			    if( String(cursor) == String("c1") )
				    return Page("b", "c2");
			    return Page("c", "");
		    },
		    error);
		const bool itemsOk = items.size() == 3 && items[0] == String("a") && items[1] == String("b") && items[2] == String("c");
		Report(ctx, ok && calls == 3 && itemsOk && error.code == 0, "the page walk follows the node's cursor to the end");
	}
	{
		// A node that hands back a cursor it already gave would keep the walk going forever.
		PHANTASMA_VECTOR<String> items;
		rpc::PhantasmaError error;
		int calls = 0;
		const bool ok = ReadAllPages<String>(
		    items, [&](const Char*)
		    { ++calls; return Page("a", "same"); }, error);
		Report(ctx, ok && calls == 2 && items.size() == 2 && error.code == 0, "the page walk stops on a cursor it has already seen");
	}
	{
		// A node that keeps handing out fresh cursors is stopped by the page cap, and that is a
		// failure and not a short answer.
		PHANTASMA_VECTOR<String> items;
		rpc::PhantasmaError error;
		int calls = 0;
		const bool ok = ReadAllPages<String>(
		    items,
		    [&](const Char*)
		    {
			    ++calls;
			    rpc::CursorPaginatedResult<String> page;
			    page.result.push_back(String("a"));
			    page.cursor = String("c") + std::to_string(calls);
			    return page;
		    },
		    error);
		Report(ctx, !ok && calls == 1000 && error.code == rpc::PhantasmaError::InvalidJSON, "the page walk stops past the page cap and reports it");
	}
	{
		// A query that failed ends the walk at once, with whatever the query reported.
		PHANTASMA_VECTOR<String> items;
		rpc::PhantasmaError error;
		int calls = 0;
		const bool ok = ReadAllPages<String>(
		    items,
		    [&](const Char*)
		    {
			    ++calls;
			    error.code = rpc::PhantasmaError::HttpError;
			    return Page("a", "c1");
		    },
		    error);
		Report(ctx, !ok && calls == 1 && error.code == rpc::PhantasmaError::HttpError, "the page walk stops when a query fails");
	}
}

} // namespace

void RunAccountAddressTypeTests(testutil::TestContext& ctx)
{
	RunRequestTests(ctx);
	RunAddressTextTests(ctx);
	RunParseDecimalTests(ctx);
	RunPagingTests(ctx);
}

} // namespace testcases
