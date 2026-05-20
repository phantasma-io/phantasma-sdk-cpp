#define PHANTASMA_IMPLEMENTATION
#include "test_cases.h"

namespace testcases {
using namespace testutil;

void RunApiJsonNumericFlexTests(TestContext& ctx)
{
	rpc::PhantasmaJsonAPI::UseRequestId("1");
	{
		bool err = false;
		const JSONDocument doc = R"({"id":"1","result":"321"})";
		const Int32 value = json::LookupInt32(json::Parse(doc), PHANTASMA_LITERAL("result"), err);
		Report(ctx, !err && value == 321, "API JSON LookupInt32 accepts quoted numeric");
	}
	{
		// The response parser must reject envelopes that cannot be matched to the generated request id.
		const JSONDocument numericId = R"({"id":1,"result":"321"})";
		rpc::PhantasmaError numericErr{};
		Int32 numericHeight = 0;
		const bool numericOk = rpc::PhantasmaJsonAPI::ParseGetBlockHeightResponse(json::Parse(numericId), numericHeight, &numericErr);
		Report(ctx, numericOk && numericErr.code == 0 && numericHeight == 321, "API response parser accepts matching numeric JSON-RPC id");

		const JSONDocument missingId = R"({"result":"321"})";
		rpc::PhantasmaError missingErr{};
		Int32 missingHeight = 0;
		const bool missingOk = rpc::PhantasmaJsonAPI::ParseGetBlockHeightResponse(json::Parse(missingId), missingHeight, &missingErr);
		Report(ctx, !missingOk && missingErr.code == rpc::PhantasmaError::InvalidRpcResponse, "API response parser rejects missing JSON-RPC id");

		const JSONDocument missingResult = R"({"id":"1"})";
		rpc::PhantasmaError missingResultErr{};
		Int32 missingResultHeight = 0;
		const bool missingResultOk = rpc::PhantasmaJsonAPI::ParseGetBlockHeightResponse(json::Parse(missingResult), missingResultHeight, &missingResultErr);
		Report(ctx,
		    !missingResultOk && missingResultErr.code == rpc::PhantasmaError::InvalidRpcResponse,
		    "API response parser rejects successful JSON-RPC envelopes without result");

		const JSONDocument nullId = R"({"id":null,"result":"321"})";
		rpc::PhantasmaError nullErr{};
		Int32 nullHeight = 0;
		const bool nullOk = rpc::PhantasmaJsonAPI::ParseGetBlockHeightResponse(json::Parse(nullId), nullHeight, &nullErr);
		Report(ctx, !nullOk && nullErr.code == rpc::PhantasmaError::InvalidRpcResponse, "API response parser rejects null JSON-RPC id");

		const JSONDocument mismatchedId = R"({"id":"2","result":"321"})";
		rpc::PhantasmaError mismatchErr{};
		Int32 mismatchHeight = 0;
		const bool mismatchOk = rpc::PhantasmaJsonAPI::ParseGetBlockHeightResponse(json::Parse(mismatchedId), mismatchHeight, &mismatchErr);
		Report(ctx, !mismatchOk && mismatchErr.code == rpc::PhantasmaError::InvalidRpcResponse, "API response parser rejects mismatched JSON-RPC id");

		const JSONDocument staleStringId = R"({"id":"0","result":"321"})";
		rpc::PhantasmaError staleStringErr{};
		Int32 staleStringHeight = 0;
		const bool staleStringOk = rpc::PhantasmaJsonAPI::ParseGetBlockHeightResponse(json::Parse(staleStringId), staleStringHeight, &staleStringErr);
		Report(ctx, !staleStringOk && staleStringErr.code == rpc::PhantasmaError::InvalidRpcResponse, "API response parser rejects stale string JSON-RPC id");

		const JSONDocument staleNumericId = R"({"id":0,"result":"321"})";
		rpc::PhantasmaError staleNumericErr{};
		Int32 staleNumericHeight = 0;
		const bool staleNumericOk = rpc::PhantasmaJsonAPI::ParseGetBlockHeightResponse(json::Parse(staleNumericId), staleNumericHeight, &staleNumericErr);
		Report(ctx, !staleNumericOk && staleNumericErr.code == rpc::PhantasmaError::InvalidRpcResponse, "API response parser rejects stale numeric JSON-RPC id");

		const JSONDocument wrongTypedId = R"({"id":{"bad":"id"},"result":"321"})";
		rpc::PhantasmaError wrongTypedErr{};
		Int32 wrongTypedHeight = 0;
		const bool wrongTypedOk = rpc::PhantasmaJsonAPI::ParseGetBlockHeightResponse(json::Parse(wrongTypedId), wrongTypedHeight, &wrongTypedErr);
		Report(ctx, !wrongTypedOk && wrongTypedErr.code == rpc::PhantasmaError::InvalidRpcResponse, "API response parser rejects non-scalar JSON-RPC id");

		const JSONDocument mismatchedErrorId = R"({"id":"0","error":{"code":-32603,"message":"Execution failed"}})";
		rpc::PhantasmaError mismatchedErrorErr{};
		Int32 mismatchedErrorHeight = 0;
		const bool mismatchedErrorOk = rpc::PhantasmaJsonAPI::ParseGetBlockHeightResponse(json::Parse(mismatchedErrorId), mismatchedErrorHeight, &mismatchedErrorErr);
		Report(ctx,
		    !mismatchedErrorOk && mismatchedErrorErr.code == rpc::PhantasmaError::InvalidRpcResponse,
		    "API response parser rejects JSON-RPC id mismatch before RPC error body");
	}
	{
		bool err = false;
		const Int64 value = json::AsInt64(JSONValue{ "\"-42\"" }, err);
		Report(ctx, !err && value == -42, "API JSON AsInt64 accepts quoted numeric");
	}
	{
		bool err = false;
		const UInt64 value = json::AsUInt64(JSONValue{ "\"18446744073709551615\"" }, err);
		Report(ctx, !err && value == std::numeric_limits<UInt64>::max(), "API JSON AsUInt64 accepts quoted numeric");
	}
	{
		rpc::PhantasmaError err{};
		Int32 height = 0;
		const JSONDocument doc = R"({"id":"1","result":"12345"})";
		const bool ok = rpc::PhantasmaJsonAPI::ParseGetBlockHeightResponse(json::Parse(doc), height, &err);
		Report(ctx, ok && err.code == 0 && height == 12345, "API GetBlockHeight parser accepts quoted numeric");
	}
	{
		// Generated request builders must not reuse one fixed id, and parser
		// validation must use the id carried by the request being parsed.
		JSONBuilder firstRequest;
		rpc::PhantasmaJsonAPI::MakeGetVersionRequest(firstRequest);
		JSONBuilder secondRequest;
		rpc::PhantasmaJsonAPI::MakeGetBlockHeightRequest(secondRequest, "main");

		const std::string firstJson = firstRequest.s.str();
		const std::string secondJson = secondRequest.s.str();
		const std::string firstRequestId = rpc::PhantasmaJsonAPI::RequestId(firstRequest);
		const std::string secondRequestId = rpc::PhantasmaJsonAPI::RequestId(secondRequest);
		Report(ctx,
		    !firstRequestId.empty() && !secondRequestId.empty() && firstRequestId != secondRequestId &&
		        firstJson.find("\"id\": \"" + firstRequestId + "\"") != std::string::npos &&
		        secondJson.find("\"id\": \"" + secondRequestId + "\"") != std::string::npos,
		    "API request builders generate distinct JSON-RPC ids");

		rpc::PhantasmaJsonAPI::UseRequestId(secondRequest);
		Report(ctx,
		    rpc::PhantasmaJsonAPI::RequestId(secondRequest) == String(PHANTASMA_LITERAL("1")),
		    "API UseRequestId releases generated JSON-RPC id storage");
		const JSONDocument staleDoc = String(PHANTASMA_LITERAL("{\"id\":\"")) + firstRequestId + String(PHANTASMA_LITERAL("\",\"result\":\"321\"}"));
		rpc::PhantasmaError staleErr{};
		Int32 staleHeight = 0;
		const bool staleOk = rpc::PhantasmaJsonAPI::ParseGetBlockHeightResponse(json::Parse(staleDoc), staleHeight, &staleErr);
		Report(ctx,
		    !staleOk && staleErr.code == rpc::PhantasmaError::InvalidRpcResponse,
		    "API response parser rejects stale generated JSON-RPC id");

		const JSONDocument matchingDoc = String(PHANTASMA_LITERAL("{\"id\":\"")) + secondRequestId + String(PHANTASMA_LITERAL("\",\"result\":\"321\"}"));
		rpc::PhantasmaError matchingErr{};
		Int32 matchingHeight = 0;
		const bool matchingOk = rpc::PhantasmaJsonAPI::ParseGetBlockHeightResponse(json::Parse(matchingDoc), matchingHeight, &matchingErr);
		Report(ctx,
		    matchingOk && matchingErr.code == 0 && matchingHeight == 321,
		    "API response parser accepts the matching generated JSON-RPC id");
		rpc::PhantasmaJsonAPI::UseRequestId("1");
	}
	{
		// Verify the current RPC shape includes the chain before the block hash.
		JSONBuilder request;
		rpc::PhantasmaJsonAPI::MakeGetBlockTransactionCountByHashRequest(request, "custom", "ABCDEF");
		const std::string json = request.s.str();
		Report(ctx,
		    json.find("\"method\": \"getBlockTransactionCountByHash\"") != std::string::npos &&
		        json.find("\"params\": [\"custom\", \"ABCDEF\"]") != std::string::npos,
		    "API GetBlockTransactionCountByHash request includes chain parameter");
	}
	{
		// Verify transaction lookup by block hash uses the chain-aware parameter order.
		JSONBuilder request;
		rpc::PhantasmaJsonAPI::MakeGetTransactionByBlockHashAndIndexRequest(request, "custom", "ABCDEF", 7);
		const std::string json = request.s.str();
		Report(ctx,
		    json.find("\"method\": \"getTransactionByBlockHashAndIndex\"") != std::string::npos &&
		        json.find("\"params\": [\"custom\", \"ABCDEF\", 7]") != std::string::npos,
		    "API GetTransactionByBlockHashAndIndex request includes chain parameter");
	}
	{
		// Verify Carbon token-series lookup sends both Phantasma and Carbon identifiers.
		JSONBuilder request;
		rpc::PhantasmaJsonAPI::MakeGetTokenSeriesByIdRequest(request, "CROWN", 17, "series-alpha", 3);
		const std::string json = request.s.str();
		Report(ctx,
		    json.find("\"method\": \"getTokenSeriesById\"") != std::string::npos &&
		        json.find("\"params\": [\"CROWN\", 17, \"series-alpha\", 3]") != std::string::npos,
		    "API GetTokenSeriesById request uses both token and series identifiers");
	}
	{
		// Verify build metadata parsing for the getVersion RPC helper.
		rpc::PhantasmaError err{};
		rpc::BuildInfoResult build{};
		const JSONDocument doc = R"({"id":"1","result":{"version":"1.2.3","commit":"abcdef","buildTimeUtc":"2026-04-28T00:00:00Z"}})";
		const bool ok = rpc::PhantasmaJsonAPI::ParseGetVersionResponse(json::Parse(doc), build, &err);
		Report(ctx,
		    ok && err.code == 0 && build.version == "1.2.3" && build.commit == "abcdef" && build.buildTimeUtc == "2026-04-28T00:00:00Z",
		    "API GetVersion parser reads build metadata");
	}
	{
		// Verify VM gas/config parsing for the getPhantasmaVmConfig RPC helper.
		rpc::PhantasmaError err{};
		rpc::PhantasmaVmConfig config{};
		const JSONDocument doc = R"({"id":"1","result":{"isStored":true,"featureLevel":2,"gasConstructor":"1","gasNexus":"2","gasOrganization":"3","gasAccount":"4","gasLeaderboard":"5","gasStandard":"6","gasOracle":"7","fuelPerContractDeploy":"8"}})";
		const bool ok = rpc::PhantasmaJsonAPI::ParseGetPhantasmaVmConfigResponse(json::Parse(doc), config, &err);
		Report(ctx,
		    ok && err.code == 0 && config.isStored && config.featureLevel == 2 && config.gasConstructor == "1" &&
		        config.gasNexus == "2" && config.gasOrganization == "3" && config.gasAccount == "4" &&
		        config.gasLeaderboard == "5" && config.gasStandard == "6" && config.gasOracle == "7" &&
		        config.fuelPerContractDeploy == "8",
		    "API GetPhantasmaVmConfig parser reads VM config");
	}
	{
		// Verify current Carbon NFT identity fields are preserved from TokenData responses.
		rpc::PhantasmaError err{};
		rpc::TokenData token{};
		const JSONDocument doc = R"({"id":"1","result":{"id":"1001","series":"55","carbonTokenId":"4","carbonSeriesId":"7","carbonNftAddress":"ABCDEF","mint":"42","chainName":"main","ownerAddress":"P-owner","creatorAddress":"P-creator","ram":"","rom":"CAFE","status":"Active","infusion":[],"properties":[{"key":"Name","value":"Crown #42"}]}})";
		const bool ok = rpc::PhantasmaJsonAPI::ParseGetTokenDataResponse(json::Parse(doc), token, &err);
		Report(ctx,
		    ok && err.code == 0 && token.id == "1001" && token.series == "55" && token.carbonTokenId == "4" &&
		        token.carbonSeriesId == "7" && token.carbonNftAddress == "ABCDEF" && token.properties.size() == 1 &&
		        token.properties[0].key == "Name",
		    "API GetTokenData parser preserves Carbon NFT identity");
	}
	{
		// Verify the current TokenSeries response shape parses without legacy-only fields.
		rpc::PhantasmaError err{};
		rpc::TokenSeries series{};
		const JSONDocument doc = R"({"id":"1","result":{"seriesId":"55","carbonTokenId":"4","carbonSeriesId":"7","ownerAddress":"P-owner","maxMint":"100","mintCount":"42","currentSupply":"41","maxSupply":"100","metadata":[{"key":"Name","value":"Series 55"}]}})";
		const bool ok = rpc::PhantasmaJsonAPI::ParseGetTokenSeriesByIdResponse(json::Parse(doc), series, &err);
		Report(ctx,
		    ok && err.code == 0 && series.seriesId == "55" && series.carbonTokenId == "4" &&
		        series.carbonSeriesId == "7" && series.ownerAddress == "P-owner" && series.maxMint == "100" &&
		        series.mintCount == "42" && series.currentSupply == "41" && series.maxSupply == "100" &&
		        series.mode == rpc::TokenSeriesMode::Unique && series.metadata.size() == 1 &&
		        series.metadata[0].value == "Series 55",
		    "API GetTokenSeriesById parser accepts current Carbon series shape");
	}
	{
		// Verify Carbon transaction metadata is not dropped from Transaction responses.
		rpc::PhantasmaError err{};
		rpc::Transaction tx{};
		const JSONDocument doc = R"({"id":"1","result":{"hash":"HASH","chainAddress":"CHAIN","timestamp":123,"blockHeight":456,"blockHash":"BLOCK","script":"","payload":"CAFE","carbonTxType":1,"carbonTxData":"BEEF","debugComment":"mint","events":[{"address":"P-event","contract":"gas","kind":"GasEscrow","name":"GasEscrow","data":"00"}],"extendedEvents":[{"address":"P-event","contract":"token","kind":"TokenCreate","data":{"symbol":"CROWN","maxSupply":"1","decimals":0,"isNonFungible":true,"carbonTokenId":4,"metadata":{"name":"Crown"}}}],"result":"","fee":"0","signatures":[{"kind":"Ed25519","data":"AA"}],"expiration":789,"state":"Halt","sender":"P-sender","gasPayer":"P-gas","gasTarget":"P-target","gasPrice":"1","gasLimit":"1000"}})";
		const bool ok = rpc::PhantasmaJsonAPI::ParseGetTransactionResponse(json::Parse(doc), tx, &err);
		Report(ctx,
		    ok && err.code == 0 && tx.hash == "HASH" && tx.payload == "CAFE" && tx.carbonTxType == 1 &&
		        tx.carbonTxData == "BEEF" && tx.debugComment == "mint" && tx.gasPayer == "P-gas" &&
		        tx.sender == "P-sender" && tx.gasTarget == "P-target" && tx.gasPrice == "1" &&
		        tx.gasLimit == "1000" && tx.expiration == 789 && tx.events.size() == 1 &&
		        tx.events[0].name == "GasEscrow" && tx.signatures.size() == 1 &&
		        tx.signatures[0].kind == "Ed25519" && tx.signatures[0].data == "AA" &&
		        tx.extendedEvents.size() == 1 && tx.extendedEvents[0].type == rpc::ExtendedEventType::TokenCreate &&
		        tx.extendedEvents[0].tokenCreate.carbonTokenId == 4,
		    "API GetTransaction parser preserves Carbon transaction metadata");
	}
	{
		// Extra stale wire field names must be ignored, not treated as aliases.
		rpc::PhantasmaError err{};
		rpc::TokenData token{};
		const JSONDocument doc = R"({"id":"1","result":{"id":"1001","ID":"stale","series":"55","carbonTokenId":"4","carbonSeriesId":"7","carbonNftAddress":"ABCDEF","mint":"42","chainName":"main","ownerAddress":"P-owner","creatorAddress":"P-creator","ram":"","rom":"CAFE","status":"Active","infusion":[],"properties":[{"key":"Name","value":"Crown #42","Key":"stale","Value":"stale"}]}})";
		const bool ok = rpc::PhantasmaJsonAPI::ParseGetTokenDataResponse(json::Parse(doc), token, &err);
		Report(ctx,
		    ok && err.code == 0 && token.id == "1001" && token.properties.size() == 1 &&
		        token.properties[0].key == "Name" && token.properties[0].value == "Crown #42",
		    "API GetTokenData parser ignores stale NFT/property field casing without alias mapping");
	}
	{
		rpc::PhantasmaError err{};
		rpc::Transaction tx{};
		const JSONDocument doc = R"({"id":"1","result":{"hash":"HASH","chainAddress":"CHAIN","timestamp":123,"blockHeight":456,"blockHash":"BLOCK","script":"","payload":"CAFE","events":[{"address":"P-event","contract":"gas","kind":"GasEscrow","name":"GasEscrow","data":"00","Kind":"stale","Data":"stale"}],"extendedEvents":[{"address":"P-event","contract":"token","kind":"TokenCreate","data":{"symbol":"CROWN","maxSupply":"1","decimals":0,"isNonFungible":true,"carbonTokenId":4},"Kind":"stale","Data":{"symbol":"stale"}}],"result":"","fee":"0","signatures":[{"kind":"Ed25519","data":"AA","Kind":"stale","Data":"stale"}],"expiration":789,"state":"Halt","sender":"P-sender","gasPayer":"P-gas","gasTarget":"P-target","gasPrice":"1","gasLimit":"1000"}})";
		const bool ok = rpc::PhantasmaJsonAPI::ParseGetTransactionResponse(json::Parse(doc), tx, &err);
		Report(ctx,
		    ok && err.code == 0 && tx.events.size() == 1 && tx.events[0].kind == "GasEscrow" &&
		        tx.events[0].data == "00" && tx.signatures.size() == 1 && tx.signatures[0].kind == "Ed25519" &&
		        tx.signatures[0].data == "AA" && tx.extendedEvents.size() == 1 &&
		        tx.extendedEvents[0].type == rpc::ExtendedEventType::TokenCreate,
		    "API GetTransaction parser ignores stale event/signature field casing without alias mapping");
	}
	{
		// Stale-only keys are ignored as old wire clutter; they must not populate current fields or fail the parse.
		rpc::PhantasmaError err{};
		rpc::TokenData token{};
		const JSONDocument doc = R"({"id":"1","result":{"ID":"stale","series":"55","carbonTokenId":"4","carbonSeriesId":"7","carbonNftAddress":"ABCDEF","mint":"42","chainName":"main","ownerAddress":"P-owner","creatorAddress":"P-creator","ram":"","rom":"CAFE","status":"Active","infusion":[],"properties":[{"Key":"stale","Value":"stale"}]}})";
		const bool ok = rpc::PhantasmaJsonAPI::ParseGetTokenDataResponse(json::Parse(doc), token, &err);
		Report(ctx,
		    ok && err.code == 0 && token.id.empty() && token.series == "55" && token.properties.size() == 1 &&
		        token.properties[0].key.empty() && token.properties[0].value.empty(),
		    "API GetTokenData parser ignores stale-only NFT/property field names without alias mapping");
	}
	{
		rpc::PhantasmaError err{};
		rpc::Transaction tx{};
		const JSONDocument doc = R"({"id":"1","result":{"hash":"HASH","chainAddress":"CHAIN","timestamp":123,"blockHeight":456,"blockHash":"BLOCK","script":"","payload":"CAFE","events":[{"address":"P-event","contract":"gas","Kind":"stale","name":"GasEscrow","Data":"stale"}],"extendedEvents":[{"address":"P-event","contract":"token","Kind":"TokenCreate","Data":{"symbol":"stale"}}],"result":"","fee":"0","signatures":[{"Kind":"stale","Data":"stale"}],"expiration":789,"state":"Halt","sender":"P-sender","gasPayer":"P-gas","gasTarget":"P-target","gasPrice":"1","gasLimit":"1000"}})";
		const bool ok = rpc::PhantasmaJsonAPI::ParseGetTransactionResponse(json::Parse(doc), tx, &err);
		Report(ctx,
		    ok && err.code == 0 && tx.events.size() == 1 && tx.events[0].kind.empty() &&
		        tx.events[0].data.empty() && tx.signatures.size() == 1 && tx.signatures[0].kind.empty() &&
		        tx.signatures[0].data.empty() && tx.extendedEvents.size() == 1 &&
		        tx.extendedEvents[0].type == rpc::ExtendedEventType::Unknown,
		    "API GetTransaction parser ignores stale-only event/signature field names without alias mapping");
	}
	{
		rpc::PhantasmaError err{};
		rpc::Token token{};
		const JSONDocument doc = R"({"id":"1","result":{"symbol":"CROWN","name":"Crown","decimals":0,"currentSupply":"1","maxSupply":"0","carbonID":"stale","burnedSupply":"0","address":"S-token","owner":"P-owner","flags":"Transferable","script":"","series":[],"metadata":[]}})";
		const bool ok = rpc::PhantasmaJsonAPI::ParseGetTokenResponse(json::Parse(doc), token, &err);
		Report(ctx,
		    ok && err.code == 0 && token.symbol == "CROWN" && token.carbonId.empty(),
		    "API GetToken parser ignores stale-only carbonID without alias mapping");
	}
}

} // namespace testcases
