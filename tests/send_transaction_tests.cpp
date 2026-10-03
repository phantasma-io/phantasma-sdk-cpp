// The one-step send asks the chain whether a token creation's symbol is taken before it signs
// anything. It refuses when the symbol is taken and when the node established nothing. These tests
// answer for the node and check what SendTransaction does with each answer, and what the one-step
// helper of RpcUtils does with a message its signer refuses.

#include "stub_http_client.h"

#include "test_cases.h"

#include "../include/Utils/RpcUtils.h"

namespace testcases {
using namespace testutil;

namespace {

// How the stub node answers the token lookups of one test.
struct TokenLookups {
	// A lookup by symbol finds a token, so the symbol is taken.
	bool symbolTaken = false;
	// The one id a lookup by id finds a token for. Zero finds nothing.
	uint64_t knownTokenId = 0;
};

// What the stub node was asked in one test.
struct NodeLog {
	// The ids of the lookups by id, in order.
	std::vector<uint64_t> lookedUpIds;
	int broadcasts = 0;
};

std::string Envelope(const std::string& id, const std::string& body)
{
	return "{\"jsonrpc\": \"2.0\", \"id\": \"" + id + "\", " + body + "}";
}

// A token as the node describes it. The pre-flight reads only whether one came back.
const std::string kFoundToken = "\"result\": {\"symbol\":\"KCAL\",\"name\":\"KCAL\",\"decimals\":10,"
                                "\"currentSupply\":\"0\",\"maxSupply\":\"0\",\"burnedSupply\":\"0\",\"address\":\"S\","
                                "\"owner\":\"P\",\"flags\":\"Fungible\",\"script\":\"\",\"series\":[],\"metadata\":[]}";
const std::string kNotFound = "\"error\": {\"code\": -32000, \"message\": \"Token not found\"}";

// Answers getToken and sendCarbonTransaction the way the node does, and writes down what it was
// asked. A lookup by symbol carries the params ["SYMBOL", false], a lookup by id ["", false, ID].
std::string AnswerNode(const std::string& request, const TokenLookups& lookups, NodeLog& log)
{
	const std::string id = Between(request, "\"id\": \"", "\"");
	const std::string method = Between(request, "\"method\": \"", "\"");
	if( method == "getToken" )
	{
		const std::string params = Between(request, "\"params\": [", "]");
		const size_t firstComma = params.find(',');
		const size_t lastComma = params.rfind(',');
		if( firstComma != lastComma )
		{
			const uint64_t tokenId = std::stoull(params.substr(lastComma + 1));
			log.lookedUpIds.push_back(tokenId);
			return Envelope(id, tokenId != 0 && tokenId == lookups.knownTokenId ? kFoundToken : kNotFound);
		}
		return Envelope(id, lookups.symbolTaken ? kFoundToken : kNotFound);
	}
	if( method == "sendCarbonTransaction" )
	{
		++log.broadcasts;
		return Envelope(id, "\"result\": \"CARBONTXHASH\"");
	}
	return Envelope(id, "\"error\": {\"code\": -32601, \"message\": \"Method not found\"}");
}

} // namespace

void RunSendTransactionTests(TestContext& ctx)
{
	const std::string wif = "KwPpBSByydVKqStGHAnZzQofCqhDmD2bfRgc9BmZqM3ZmsdWJw4d";
	const PhantasmaKeys keys = PhantasmaKeys::FromWIF(wif.c_str(), (int)wif.size());
	const Bytes32 owner = ToBytes32(keys.GetPublicKey());

	ByteArray metadata;
	std::string error;
	const std::vector<std::pair<std::string, std::string>> metaFields = {
		{ "name", "My test token!" },
		{ "icon", SampleIcon() },
		{ "url", "http://example.com" },
		{ "description", "My test token description" },
	};
	TokenInfoOwned info;
	if( !TokenMetadataBuilder::BuildAndSerialize(metaFields, metadata, error) ||
	    !TokenInfoBuilder::Build("MYFEE", intx((uint64_t)0), false, 2, owner, metadata, info, error) )
	{
		Report(ctx, false, "the send fixture builds its token creation", error);
		return;
	}
	// The message points into the envelope's own buffer, so the envelope has to outlive every send.
	const TxEnvelope env = CreateTokenTxHelper::BuildTx(info.View(), owner);
	const Blockchain::TxMsg& msg = env.msg;
	const Blockchain::GasConfig config = PlanningConfig();

	// Runs one send against a node that answers the token lookups as `lookups` says.
	const auto send = [&](const TokenLookups& lookups, const SendOptions& options, NodeLog& log, std::string& outError)
	{
		StubNode node;
		node.answer = [&](const std::string& request)
		{ return AnswerNode(request, lookups, log); };
		rpc::PhantasmaAPI api(node);
		String hash;
		return SendTransaction(api, msg, { &keys }, config, options, hash, outError);
	};

	{
		// The symbol resolves to a token. Sending would pay the creation fee for nothing.
		TokenLookups lookups;
		lookups.symbolTaken = true;
		NodeLog log;
		std::string sendError;
		const bool sent = send(lookups, SendOptions{}, log, sendError);
		ExpectRefused(ctx, "SendTransaction refuses a taken symbol", "is already taken", sent, sendError);
		Report(ctx, log.broadcasts == 0, "a taken symbol is not broadcast");
	}
	{
		// The node finds neither the symbol nor the control token, so nothing is established.
		TokenLookups lookups;
		NodeLog log;
		std::string sendError;
		const bool sent = send(lookups, SendOptions{}, log, sendError);
		ExpectRefused(ctx, "SendTransaction refuses when the lookups establish nothing", "could not establish", sent, sendError);
		Report(ctx, log.broadcasts == 0, "a symbol nothing is known about is not broadcast");
	}
	{
		// The default control is the gas token of the configuration. The node finds it and does not
		// find the symbol, so the symbol is free and the creation goes out.
		TokenLookups lookups;
		lookups.knownTokenId = config.gasTokenId;
		NodeLog log;
		std::string sendError;
		const bool sent = send(lookups, SendOptions{}, log, sendError);
		Report(ctx, sent && log.broadcasts == 1, "SendTransaction sends a creation whose symbol is free", sendError);
		Report(ctx, log.lookedUpIds == std::vector<uint64_t>{ config.gasTokenId }, "the default control lookup asks for the gas token");
	}
	{
		// A control token the caller names replaces the gas token.
		TokenLookups lookups;
		lookups.knownTokenId = 7;
		SendOptions options;
		options.controlTokenId = 7;
		NodeLog log;
		std::string sendError;
		const bool sent = send(lookups, options, log, sendError);
		Report(ctx, sent && log.lookedUpIds == std::vector<uint64_t>{ 7 }, "the control lookup asks for the token the caller names", sendError);
	}
	{
		// The helper signs and sends in one call. A message the signer refuses, here one with no gas
		// offer, is not sent, and the signer's reason reaches the caller. With exceptions the call
		// throws instead.
		Blockchain::TxMsg unplanned = msg;
		unplanned.maxGas = 0;
		NodeLog log;
		StubNode node;
		node.answer = [&](const std::string& request)
		{ return AnswerNode(request, TokenLookups{}, log); };
		rpc::PhantasmaAPI api(node);
		rpc::PhantasmaError error;
		bool refused = false;
		PHANTASMA_TRY
		{
			const String hash = SignAndSendCarbonTransaction(api, unplanned, keys, &error);
			refused = hash.empty() && error.code == rpc::PhantasmaError::Refused &&
			          std::string(error.message.c_str()).find("no gas offer") != std::string::npos;
		}
		PHANTASMA_CATCH_ALL()
		{
			refused = true;
		}
		Report(ctx, refused && log.broadcasts == 0, "the one-step helper sends nothing its signer refuses");
	}
}

} // namespace testcases
