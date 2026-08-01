// Typed extended-event decoding: kind dispatch, per-method argument dispatch, raw fallbacks.
//
// Live fixtures were captured from https://devnet.phantasma.info/rpc on 2026-08-01 via
// getBlockByHeight("main", <height>); the height is stated on each test. Long hex payloads
// (contract scripts, ABIs, ROMs) are truncated to keep the fixtures readable - the field set, the
// field types and every other value are verbatim. Shape-only fixtures for the event kinds without
// a capturable live sample (token and market events) mirror the node's emission sites in
// RpcEventBuilder.TokenEvents.cs / RpcEventBuilder.MarketEvents.cs, whose serializer settings are
// the same ones verified live on the special-resolution family.
#include "test_cases.h"

namespace testcases {
using namespace testutil;

namespace {

// Parses one extended event from its JSON text.
rpc::EventExtended ParseEvent(const JSONDocument& document, bool& jsonErr)
{
	return rpc::ParseEventExtended(json::Parse(document), jsonErr);
}

// Parses one special-resolution call from its JSON text.
rpc::SpecialResolutionCall ParseCall(const JSONDocument& document, bool& jsonErr)
{
	return rpc::ParseSpecialResolutionCall(json::Parse(document), jsonErr);
}

// Wraps an arguments payload into the call that carries it, which is the only way the parser
// learns which shape to expect.
std::string CallDocument(const char* module, const char* method, const char* arguments)
{
	return std::string("{\"moduleId\":0,\"module\":\"") + module + "\",\"methodId\":0,\"method\":\"" + method +
	       "\",\"arguments\":" + arguments + "}";
}

} // namespace

void RunExtendedEventTests(TestContext& ctx)
{
	{
		// Devnet block 8,736,259: one SpecialResolution event whose single call is
		// phantasma_vm.RepairSeries with 3,649 supplements and 8,370 repairs. The fixture keeps
		// the first supplement and the first repair.
		bool err = false;
		const JSONDocument doc = R"({
			"address": "P2KJPTC82NAFEzXg3X4eA83JvyWQ8PJVaBop2fUUsKPBcou",
			"contract": "governance",
			"kind": "SpecialResolution",
			"data": {
				"resolutionId": 37,
				"description": "Special Resolution",
				"calls": [{
					"moduleId": 2, "module": "phantasma_vm", "methodId": 6, "method": "RepairSeries",
					"arguments": {
						"supplementsCount": "3649",
						"supplements": [{
							"token": "BRC", "tokenId": "23", "phantasmaSeriesId": "6472",
							"maxSupply": "1000", "mintCount": "30", "mode": "1",
							"script": "0004010D000403524F4D0300", "abi": "080A67657443726561746564",
							"rom": "010804076372656174656405"
						}],
						"repairsCount": "8370",
						"repairs": [{
							"token": "CROWN", "tokenId": "4", "phantasmaSeriesId": "0",
							"importedLiveCount": "10998", "script": "0004000E0000040D01040743",
							"abi": "04076765744E616D65040100"
						}]
					}
				}]
			}
		})";
		const rpc::EventExtended event = ParseEvent(doc, err);
		const rpc::RepairSeriesArguments* repair =
		    event.specialResolution.calls.empty()
		        ? nullptr
		        : rpc::SpecialResolutionArgumentsAs<rpc::RepairSeriesArguments>(
		              event.specialResolution.calls[0].arguments.get());
		Report(ctx,
		    !err && event.type == rpc::ExtendedEventType::SpecialResolution &&
		        event.specialResolution.resolutionId == 37 &&
		        event.specialResolution.description == "Special Resolution" && repair != nullptr &&
		        repair->supplementsCount == "3649" && repair->repairsCount == "8370" &&
		        repair->supplements.size() == 1 && repair->supplements[0].token == "BRC" &&
		        repair->supplements[0].phantasmaSeriesId == "6472" && repair->supplements[0].mintCount == "30" &&
		        repair->repairs.size() == 1 && repair->repairs[0].importedLiveCount == "10998",
		    "Extended event RepairSeries arguments decode to their typed shape");
	}
	{
		// Devnet block 8,736,266: resolution 44 carries 9,600 token.TransferFungible calls; this
		// is its first call verbatim.
		bool err = false;
		const JSONDocument doc = R"({
			"address": "P2KJPTC82NAFEzXg3X4eA83JvyWQ8PJVaBop2fUUsKPBcou",
			"contract": "governance",
			"kind": "SpecialResolution",
			"data": {
				"resolutionId": 44,
				"description": "Repair imported NFT fungible infusions",
				"calls": [{
					"moduleId": 1, "module": "token", "methodId": 0, "method": "TransferFungible",
					"arguments": {
						"from": "S3dPnV8dfdkHDHDcJiHY255FEUZCM7oAmDW78LpYZ4jveGW",
						"to": "S3dPnV8dfdkHDHDcJiHY255FEUZCM7oAmDW78LpYZ4jveGW",
						"amount": "10000000000", "token": "KCAL", "tokenId": "1"
					}
				}]
			}
		})";
		const rpc::EventExtended event = ParseEvent(doc, err);
		const rpc::TransferFungibleArguments* transfer =
		    event.specialResolution.calls.empty()
		        ? nullptr
		        : rpc::SpecialResolutionArgumentsAs<rpc::TransferFungibleArguments>(
		              event.specialResolution.calls[0].arguments.get());
		Report(ctx,
		    !err && event.specialResolution.resolutionId == 44 && transfer != nullptr &&
		        transfer->from == "S3dPnV8dfdkHDHDcJiHY255FEUZCM7oAmDW78LpYZ4jveGW" &&
		        transfer->to == transfer->from && transfer->amount == "10000000000" && transfer->token == "KCAL" &&
		        transfer->tokenId == "1",
		    "Extended event TransferFungible arguments decode to their typed shape");
	}
	{
		// Devnet block 8,736,257: phantasma_vm.ImportContracts restoring 70 contracts. This is the
		// shape a flat string map cannot carry at all - the contracts were dropped before.
		bool err = false;
		const JSONDocument doc = R"({
			"address": "P2KJPTC82NAFEzXg3X4eA83JvyWQ8PJVaBop2fUUsKPBcou",
			"contract": "governance",
			"kind": "SpecialResolution",
			"data": {
				"resolutionId": 36,
				"calls": [{
					"moduleId": 2, "module": "phantasma_vm", "methodId": 5, "method": "ImportContracts",
					"arguments": {
						"contractsCount": "70",
						"contracts": [
							{
								"name": "mail",
								"address": "S3d6cUXRwJbudV4ADbRtMz3P9527ts7D2Lh9h2J96m48FPW",
								"owner": "P2KFNXEbt65rQiWqogAzqkVGMqFirPmqPw8mQyxvRKsrXV8",
								"script": "0B", "abi": "090B507573684D657373616765",
								"rootVariables": [], "tables": []
							},
							{
								"name": "pharming",
								"address": "S3d6cUXRwJbudV4ADbRtMz3P9527ts7D2Lh9h2J96m48FPW",
								"owner": "P2KFNXEbt65rQiWqogAzqkVGMqFirPmqPw8mQyxvRKsrXV8",
								"script": "0B", "abi": "0906676574546F6B656E",
								"rootVariables": [{
									"key": "6D616E61676572",
									"value": "0100E9F4F69F677473684D2E201672A6AC30CA8F2A238C68"
								}],
								"tables": [{
									"name": "addrs_kcal_bnb",
									"rows": [{"key": "3C003E", "value": "0104040B534F554C41646472657373"}]
								}]
							}
						]
					}
				}]
			}
		})";
		const rpc::EventExtended event = ParseEvent(doc, err);
		const rpc::ImportContractsArguments* imported =
		    event.specialResolution.calls.empty()
		        ? nullptr
		        : rpc::SpecialResolutionArgumentsAs<rpc::ImportContractsArguments>(
		              event.specialResolution.calls[0].arguments.get());
		Report(ctx,
		    !err && imported != nullptr && imported->contractsCount == "70" && imported->contracts.size() == 2 &&
		        imported->contracts[0].name == "mail" && imported->contracts[0].rootVariables.empty() &&
		        imported->contracts[0].tables.empty() && imported->contracts[1].name == "pharming" &&
		        imported->contracts[1].rootVariables.size() == 1 &&
		        imported->contracts[1].rootVariables[0].key == "6D616E61676572" &&
		        imported->contracts[1].tables.size() == 1 &&
		        imported->contracts[1].tables[0].name == "addrs_kcal_bnb" &&
		        imported->contracts[1].tables[0].rows.size() == 1 &&
		        imported->contracts[1].tables[0].rows[0].key == "3C003E",
		    "Extended event ImportContracts keeps contract storage instead of dropping it");
	}
	{
		// A resolution can carry another resolution: the outer call holds the nested id in its
		// arguments and the nested calls in its calls, which dispatch exactly like top-level ones.
		bool err = false;
		const JSONDocument doc = R"({
			"moduleId": 0, "module": "governance", "methodId": 2, "method": "SpecialResolution",
			"arguments": {"resolutionId": "31"},
			"calls": [{
				"moduleId": 1, "module": "token", "methodId": 0, "method": "TransferFungible",
				"arguments": {"from": "S3dPn", "to": "S3dPn", "amount": "5", "token": "KCAL", "tokenId": "1"}
			}]
		})";
		const rpc::SpecialResolutionCall call = ParseCall(doc, err);
		const rpc::NestedResolutionArguments* nested =
		    rpc::SpecialResolutionArgumentsAs<rpc::NestedResolutionArguments>(call.arguments.get());
		const rpc::TransferFungibleArguments* transfer =
		    call.calls.empty() ? nullptr
		                       : rpc::SpecialResolutionArgumentsAs<rpc::TransferFungibleArguments>(
		                             call.calls[0].arguments.get());
		Report(ctx,
		    !err && nested != nullptr && nested->resolutionId == "31" && call.calls.size() == 1 &&
		        transfer != nullptr && transfer->amount == "5",
		    "Nested special resolution calls decode recursively");
	}
	{
		// An older node can answer a raw dump for a method this build models. Dispatching on the
		// method name first would decode that dump into a typed shape with every field empty.
		bool err = false;
		const JSONDocument doc = CallDocument("token", "TransferFungible", R"({"rawArgs":"0104040B534F554C"})");
		const rpc::SpecialResolutionCall call = ParseCall(doc, err);
		const rpc::RawArguments* raw = rpc::SpecialResolutionArgumentsAs<rpc::RawArguments>(call.arguments.get());
		Report(ctx, !err && raw != nullptr && raw->rawArgs == "0104040B534F554C",
		    "Raw arguments win over a known method");
	}
	{
		// A module/method pair this build does not model keeps its JSON, so a node newer than the
		// SDK never costs the caller the data it answered.
		bool err = false;
		const JSONDocument doc = CallDocument("token", "BrandNewMethod", R"({"brandNewField":"7"})");
		const rpc::SpecialResolutionCall call = ParseCall(doc, err);
		const rpc::UnrecognizedArguments* unrecognized =
		    rpc::SpecialResolutionArgumentsAs<rpc::UnrecognizedArguments>(call.arguments.get());
		Report(ctx, !err && unrecognized != nullptr && unrecognized->json == R"({"brandNewField":"7"})",
		    "Unmodeled method keeps its arguments verbatim");
	}
	{
		// The pair is modeled but the payload is not the modeled shape - a node whose fields
		// drifted. Keeping the JSON makes the drift visible instead of reporting empty fields.
		bool err = false;
		const JSONDocument doc = CallDocument("phantasma_vm", "RepairSeries", R"({"supplementsCount":["3649"]})");
		const rpc::SpecialResolutionCall call = ParseCall(doc, err);
		const rpc::UnrecognizedArguments* unrecognized =
		    rpc::SpecialResolutionArgumentsAs<rpc::UnrecognizedArguments>(call.arguments.get());
		Report(ctx, !err && unrecognized != nullptr && unrecognized->json == R"({"supplementsCount":["3649"]})",
		    "Mismatched known shape keeps its arguments verbatim");
	}
	{
		// token.CreateToken metadata values are VM values, not plain strings.
		bool err = false;
		const JSONDocument doc = CallDocument("token", "CreateToken",
		    R"({"symbol":"SOUL","owner":"P2K6h","maxSupply":"0","decimals":"8","flags":"199",)"
		    R"("metadata":{"_ia":[{"mul":"25","div":"10000"}],"name":"Phantasma Stake"}})");
		const rpc::SpecialResolutionCall call = ParseCall(doc, err);
		const rpc::CreateTokenArguments* created =
		    rpc::SpecialResolutionArgumentsAs<rpc::CreateTokenArguments>(call.arguments.get());
		const rpc::VmValue* interest = nullptr;
		const rpc::VmValue* name = nullptr;
		if( created )
		{
			for( const rpc::VmField& field : created->metadata )
			{
				if( field.name == "_ia" )
					interest = field.value.get();
				else if( field.name == "name" )
					name = field.value.get();
			}
		}
		const rpc::VmValue* mul = interest && interest->ItemCount() == 1 ? interest->Item(0)->Field("mul") : nullptr;
		Report(ctx,
		    !err && created != nullptr && created->decimals == "8" && name != nullptr &&
		        name->Text() == "Phantasma Stake" && interest != nullptr && interest->IsItems() && mul != nullptr &&
		        mul->Text() == "25",
		    "CreateToken arguments carry VM metadata with its structure intact");
	}
	{
		// A version 0 gas config has no gas-model-v2 tail on the wire; the tail fields stay empty
		// and the shape still decodes.
		bool err = false;
		const JSONDocument doc = CallDocument("governance", "SetGasConfig",
		    R"({"version":"0","maxNameLength":"255","feeMultiplier":"16","gasBurnRatioShift":"1"})");
		const rpc::SpecialResolutionCall call = ParseCall(doc, err);
		const rpc::GasConfigArguments* config =
		    rpc::SpecialResolutionArgumentsAs<rpc::GasConfigArguments>(call.arguments.get());

		bool tailErr = false;
		const JSONDocument tailDoc = CallDocument("governance", "SetGasConfig",
		    R"({"version":"1","feeMultiplier":"16","minimumGasBill":"21000","gasProducerRatioMul":"45"})");
		const rpc::SpecialResolutionCall tailCall = ParseCall(tailDoc, tailErr);
		const rpc::GasConfigArguments* tailConfig =
		    rpc::SpecialResolutionArgumentsAs<rpc::GasConfigArguments>(tailCall.arguments.get());

		Report(ctx,
		    !err && config != nullptr && config->feeMultiplier == "16" && config->minimumGasBill.empty() &&
		        !tailErr && tailConfig != nullptr && tailConfig->minimumGasBill == "21000" &&
		        tailConfig->gasProducerRatioMul == "45",
		    "Gas config v2 tail is optional");
	}
	{
		// A call without arguments must carry none rather than a fabricated empty shape.
		bool err = false;
		const JSONDocument doc = R"({"moduleId":1,"module":"token","methodId":0,"method":"TransferFungible"})";
		const rpc::SpecialResolutionCall call = ParseCall(doc, err);
		Report(ctx, !err && !call.arguments && call.calls.empty(),
		    "A call without arguments carries no shape");
	}
	{
		// Shape per RpcEventBuilder.TokenEvents.cs:153, including the metadata map the node renders
		// to strings. That map used to be dropped whenever the built-in JSON parser was in use.
		bool err = false;
		const JSONDocument doc = R"({
			"address": "P2KFNXEbt65rQiWqogAzqkVGMqFirPmqPw8mQyxvRKsrXV8",
			"contract": "token",
			"kind": "TokenCreate",
			"data": {
				"symbol": "CROWN", "maxSupply": "0", "decimals": 0, "isNonFungible": true,
				"carbonTokenId": 4, "metadata": {"name": "Crown", "description": "Phantasma Crown"}
			}
		})";
		const rpc::EventExtended event = ParseEvent(doc, err);
		Report(ctx,
		    !err && event.type == rpc::ExtendedEventType::TokenCreate && event.tokenCreate.symbol == "CROWN" &&
		        event.tokenCreate.isNonFungible && event.tokenCreate.carbonTokenId == 4 &&
		        event.tokenCreate.metadata.size() == 2 && event.tokenCreate.metadata.at("name") == "Crown" &&
		        event.tokenCreate.metadata.at("description") == "Phantasma Crown",
		    "TokenCreate event keeps its metadata map");
	}
	{
		// Shape per RpcEventBuilder.MarketEvents.cs:403; all three order kinds share it.
		bool err = false;
		const JSONDocument doc = R"({
			"address": "P2KFNXEbt65rQiWqogAzqkVGMqFirPmqPw8mQyxvRKsrXV8",
			"contract": "market",
			"kind": "OrderFilled",
			"data": {
				"baseSymbol": "CROWN", "quoteSymbol": "SOUL", "tokenId": "114421",
				"carbonBaseTokenId": 4, "carbonQuoteTokenId": 2, "carbonInstanceId": 7,
				"seller": "P2KFNXEbt65rQiWqogAzqkVGMqFirPmqPw8mQyxvRKsrXV8",
				"buyer": "P2K6hJ8LQ4dqiiTBUxKfLwCbGKRDvcnCkTQ8vHjaBGnDy5B",
				"price": "1000000000", "endPrice": "0",
				"startDate": 1785000000, "endDate": 1785600000, "type": "Fixed"
			}
		})";
		const rpc::EventExtended event = ParseEvent(doc, err);
		Report(ctx,
		    !err && event.type == rpc::ExtendedEventType::MarketOrder && event.marketOrder.tokenId == "114421" &&
		        event.marketOrder.carbonInstanceId == 7 && event.marketOrder.price == "1000000000" &&
		        event.marketOrder.startDate == 1785000000 && event.marketOrder.type == "Fixed",
		    "Market order events decode to their typed shape");
	}
	{
		// An event kind this build does not model keeps its payload instead of failing the answer.
		bool err = false;
		const JSONDocument doc =
		    R"({"address":"P","contract":"x","kind":"BrandNewKind","data":{"somethingNew":"7"}})";
		const rpc::EventExtended event = ParseEvent(doc, err);
		Report(ctx,
		    !err && event.type == rpc::ExtendedEventType::Unknown && event.unknownData == R"({"somethingNew":"7"})",
		    "Unknown event kind keeps its payload verbatim");
	}
	{
		// The kind names a modeled shape but the payload does not match it.
		bool err = false;
		const JSONDocument doc =
		    R"({"address":"P","contract":"token","kind":"TokenCreate","data":{"symbol":["CROWN"]}})";
		const rpc::EventExtended event = ParseEvent(doc, err);
		Report(ctx,
		    !err && event.type == rpc::ExtendedEventType::Unknown && event.tokenCreate.symbol.empty() &&
		        event.unknownData == R"({"symbol":["CROWN"]})",
		    "Mismatched event payload falls back to the raw shape");
	}
	{
		// getToken("SOUL", true) on devnet: 15 scalar metadata rows plus the "_ia" interest row,
		// which is an array of structs. That row is what a string-typed model cannot represent.
		bool err = false;
		const JSONDocument doc = R"({"id":"1","result":{
			"symbol":"SOUL","name":"Phantasma Stake","decimals":8,"currentSupply":"1","maxSupply":"0",
			"burnedSupply":"0","address":"S-token","owner":"P-owner","flags":"Transferable","script":"",
			"series":[],
			"metadata":[
				{"key":"name","value":"Phantasma Stake"},
				{"key":"_ia","value":[{"div":"10000","mul":"25","who":["P2K6h"]},{"fix":"1"}]}
			]
		}})";
		rpc::PhantasmaJsonAPI::UseRequestId("1");
		rpc::PhantasmaError parseError{};
		rpc::Token token{};
		const bool ok = rpc::PhantasmaJsonAPI::ParseGetTokenResponse(doc, token, &parseError);
		const rpc::VmValue* interest = token.metadata.size() == 2 ? &token.metadata[1].value : nullptr;
		const rpc::VmValue* first = interest && interest->ItemCount() == 2 ? interest->Item(0) : nullptr;
		const rpc::VmValue* mul = first ? first->Field("mul") : nullptr;
		const rpc::VmValue* who = first ? first->Field("who") : nullptr;
		Report(ctx,
		    ok && parseError.code == 0 && token.metadata.size() == 2 &&
		        token.metadata[0].value.Text() == "Phantasma Stake" && interest != nullptr && interest->IsItems() &&
		        mul != nullptr && mul->Text() == "25" && who != nullptr && who->ItemCount() == 1 &&
		        who->Item(0)->Text() == "P2K6h",
		    "Token metadata keeps structured VM values");
		(void)err;
	}

	{
		// Indented JSON must parse exactly like compact JSON. The built-in parser used to end an
		// object at the last value rather than at the closing brace, so any whitespace before that
		// brace made every enclosing array misparse - which is what an answer captured from a
		// pretty-printing proxy or a hand-written fixture looks like.
		bool compactErr = false;
		bool indentedErr = false;
		const JSONDocument compact =
		    R"({"moduleId":1,"module":"token","methodId":0,"method":"TransferFungible","arguments":{"from":"S3dPn","to":"S3dPn","amount":"5","token":"KCAL","tokenId":"1"},"calls":[]})";
		const JSONDocument indented = R"({
			"moduleId": 1,
			"module": "token",
			"methodId": 0,
			"method": "TransferFungible",
			"arguments": {
				"from": "S3dPn",
				"to": "S3dPn",
				"amount": "5",
				"token": "KCAL",
				"tokenId": "1"
			},
			"calls": []
		})";
		const rpc::SpecialResolutionCall compactCall = ParseCall(compact, compactErr);
		const rpc::SpecialResolutionCall indentedCall = ParseCall(indented, indentedErr);
		const rpc::TransferFungibleArguments* compactArguments =
		    rpc::SpecialResolutionArgumentsAs<rpc::TransferFungibleArguments>(compactCall.arguments.get());
		const rpc::TransferFungibleArguments* indentedArguments =
		    rpc::SpecialResolutionArgumentsAs<rpc::TransferFungibleArguments>(indentedCall.arguments.get());
		Report(ctx,
		    !compactErr && !indentedErr && compactArguments != nullptr && indentedArguments != nullptr &&
		        compactArguments->amount == indentedArguments->amount && indentedArguments->amount == "5" &&
		        indentedArguments->token == "KCAL",
		    "Indented JSON parses like compact JSON");
	}

	// Every module/method pair the node decodes, and the shape it must produce. The pairs mirror
	// the C# reference converter; a payload carrying one real field per pair proves the parser was
	// reached, and the table is checked against the dispatch table itself below.
	struct DispatchCase {
		const char* module;
		const char* method;
		const char* arguments;
		rpc::SpecialResolutionArgumentType type;
	};
	typedef rpc::SpecialResolutionArgumentType ArgType;
	static const DispatchCase dispatchCases[] = {
		{ "governance", "SetGasConfig", R"({"version":"1"})", ArgType::GasConfig },
		{ "governance", "SetChainConfig", R"({"version":"0"})", ArgType::ChainConfig },
		{ "governance", "SpecialResolution", R"({"resolutionId":"31"})", ArgType::NestedResolution },
		{ "governance", "SetMetadata", R"({"metadata":{"name":"Phantasma"}})", ArgType::Metadata },
		{ "governance", "SetNodeConfig", R"({"nodes":[{"id":"1","type":"Validator"}]})", ArgType::NodeConfig },
		{ "governance", "RegisterName", R"({"address":"P2K6h","name":"alex"})", ArgType::RegisterName },
		{ "governance", "LookupName", R"({"address":"P2K6h"})", ArgType::Address },
		{ "governance", "LookupAddress", R"({"name":"alex"})", ArgType::Name },
		{ "phantasma_vm", "ExecuteScript", R"({"maxGas":"10000","script":"0B"})", ArgType::ExecuteScript },
		{ "phantasma_vm", "RegisterTokenContract", R"({"tokenId":"4","symbol":"CROWN"})",
		    ArgType::RegisterTokenContract },
		{ "phantasma_vm", "DeployContract", R"({"from":"P2K6h","contractName":"mail"})", ArgType::DeployContract },
		{ "phantasma_vm", "IsContractDeployed", R"({"name":"mail"})", ArgType::Name },
		{ "phantasma_vm", "SetConfig", R"({"featureLevel":"3"})", ArgType::PhantasmaVmConfig },
		{ "phantasma_vm", "ImportContracts", R"({"contractsCount":"70","contracts":[]})", ArgType::ImportContracts },
		{ "phantasma_vm", "RepairSeries", R"({"supplementsCount":"3649"})", ArgType::RepairSeries },
		{ "phantasma_vm", "RepairToken", R"({"repairsCount":"2","repairs":[]})", ArgType::RepairToken },
		{ "token", "TransferFungible", R"({"from":"S3dPn","to":"S3dPn","amount":"1"})", ArgType::TransferFungible },
		{ "token", "TransferNonFungible", R"({"from":"S3dPn","instanceIds":["7"]})", ArgType::TransferNonFungible },
		{ "token", "CreateToken", R"({"symbol":"SOUL","decimals":"8"})", ArgType::CreateToken },
		{ "token", "MintFungible", R"({"to":"S3dPn","amount":"5"})", ArgType::MintFungible },
		{ "token", "BurnFungible", R"({"from":"S3dPn","amount":"5"})", ArgType::BurnFungible },
		{ "token", "GetBalance", R"({"address":"P2K6h","token":"KCAL"})", ArgType::Balance },
		{ "token", "CreateTokenSeries", R"({"owner":"P2K6h","maxMint":"100"})", ArgType::TokenSeries },
		{ "token", "DeleteTokenSeries", R"({"seriesId":"1","token":"CROWN"})", ArgType::TokenSeriesReference },
		{ "token", "MintNonFungible", R"({"owner":"P2K6h","tokens":[{"seriesId":"1"}]})", ArgType::MintNonFungible },
		{ "token", "BurnNonFungible", R"({"address":"P2K6h","instanceIds":["7"]})", ArgType::BurnNonFungible },
		{ "token", "GetNonFungibleInfo", R"({"instanceId":"7","getSchemas":"1"})", ArgType::NonFungibleInfo },
		{ "token", "GetNonFungibleInfoByRomId", R"({"romId":"CAFE","getSchemas":"1"})",
		    ArgType::NonFungibleInfoByRomId },
		{ "token", "GetSeriesInfo", R"({"seriesId":"1","token":"CROWN"})", ArgType::TokenSeriesReference },
		{ "token", "GetSeriesInfoByMetaId", R"({"romId":"CAFE","token":"CROWN"})", ArgType::SeriesInfoByMetaId },
		{ "token", "GetTokenInfo", R"({"token":"CROWN","tokenId":"4"})", ArgType::TokenReference },
		{ "token", "GetTokenInfoBySymbol", R"({"symbol":"CROWN"})", ArgType::Symbol },
		{ "token", "GetTokenSupply", R"({"token":"CROWN","tokenId":"4"})", ArgType::TokenReference },
		{ "token", "GetSeriesSupply", R"({"seriesId":"1","token":"CROWN"})", ArgType::TokenSeriesReference },
		{ "token", "GetTokenIdBySymbol", R"({"symbol":"CROWN"})", ArgType::Symbol },
		{ "token", "GetBalances", R"({"address":"P2K6h"})", ArgType::Address },
		{ "token", "CreateMintedTokenSeries", R"({"recipient":"P2K6h","roms":["CA"]})",
		    ArgType::CreateMintedTokenSeries },
		{ "token", "ApplyInflation", R"({"token":"SOUL","tokenId":"2"})", ArgType::TokenReference },
		{ "token", "UpdateTokenMetadata", R"({"metadata":{"name":"Crown"}})", ArgType::UpdateTokenMetadata },
		{ "token", "GetNextTokenInflation", R"({"token":"SOUL","tokenId":"2"})", ArgType::TokenReference },
		{ "token", "SetTokensConfig", R"({"flags":"3","flagsNames":["Transferable"]})", ArgType::TokensConfig },
		{ "token", "UpdateSeriesMetadata", R"({"seriesId":"1","metadata":"CAFE"})", ArgType::UpdateSeriesMetadata },
		{ "token", "MintPhantasmaNonFungible", R"({"owner":"P2K6h","tokens":[{"phantasmaSeriesId":"6472"}]})",
		    ArgType::MintPhantasmaNonFungible },
	};

	const int caseCount = (int)(sizeof(dispatchCases) / sizeof(dispatchCases[0]));
	for( int i = 0; i < caseCount; ++i )
	{
		bool err = false;
		const JSONDocument doc =
		    CallDocument(dispatchCases[i].module, dispatchCases[i].method, dispatchCases[i].arguments);
		const rpc::SpecialResolutionCall call = ParseCall(doc, err);
		const std::string label =
		    std::string("Arguments of ") + dispatchCases[i].module + "." + dispatchCases[i].method + " decode to " +
		    "their own shape";
		Report(ctx, !err && call.arguments && call.arguments->type == dispatchCases[i].type, label.c_str());
	}

	{
		// Guards the table above against drift: a pair added to the dispatch table without a case
		// here would otherwise ship untested.
		int tableCount = 0;
		const rpc::SpecialResolutionArgumentsEntry* table = rpc::SpecialResolutionArgumentsTable(tableCount);
		int covered = 0;
		for( int i = 0; i < tableCount; ++i )
		{
			for( int j = 0; j < caseCount; ++j )
			{
				if( std::string(table[i].module) == dispatchCases[j].module &&
				    std::string(table[i].method) == dispatchCases[j].method )
				{
					++covered;
					break;
				}
			}
		}
		Report(ctx, tableCount == caseCount && covered == tableCount,
		    "Every decoded module/method pair has a dispatch case");
	}
}

} // namespace testcases
