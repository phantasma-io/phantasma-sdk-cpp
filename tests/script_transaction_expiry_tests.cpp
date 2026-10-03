// The script path builds, signs and broadcasts inside one call, so what it stamped is read back
// from the transaction the node was given. The stub node answers with the hash of what it
// received, which is what the node does and what the call checks before it returns.

#include "stub_http_client.h"

#include "test_cases.h"

#include "../include/Utils/RpcUtils.h"

namespace phantasma {

std::string StubNode::Answer(const std::string& request)
{
	if( answer )
	{
		return answer(request);
	}
	sentTx = Between(request, "\"params\": [\"", "\"]");

	// The reader keeps a reference to the array, so the array has to outlive it.
	const ByteArray bytes = Base16::Decode(sentTx);
	BinaryReader reader(bytes);
	const Transaction sent = Transaction::Unserialize(reader);
	return "{\"jsonrpc\": \"2.0\", \"id\": \"" + Between(request, "\"id\": \"", "\"") +
	       "\", \"result\": \"" + sent.GetHash().ToString() + "\"}";
}

} // namespace phantasma

namespace testcases {
using namespace testutil;

static Timestamp BroadcastExpiration(const std::string& sentTx)
{
	// The reader keeps a reference to the array, so the array has to outlive it.
	const ByteArray bytes = Base16::Decode(sentTx);
	BinaryReader reader(bytes);
	return Transaction::Unserialize(reader).Expiration();
}

void RunScriptTransactionExpiryTests(TestContext& ctx)
{
	const std::string wif = "KwPpBSByydVKqStGHAnZzQofCqhDmD2bfRgc9BmZqM3ZmsdWJw4d";
	const PhantasmaKeys keys = PhantasmaKeys::FromWIF(wif.c_str(), (int)wif.size());
	const ByteArray script{ 0x0d, 0x00, 0x0b };

	StubNode node;
	rpc::PhantasmaAPI api(node);

	// A chain refuses an expiry at or beyond its own window, and that window can be as short as the
	// node default. Both transaction paths are admitted by that same check, so they stamp the same
	// lifetime. This one counts in seconds while the Carbon default counts in milliseconds.
	const Timestamp::ValueType lifetime = (Timestamp::ValueType)(carbon::DefaultExpiryMs / 1000);
	const Timestamp::ValueType before = (Timestamp::ValueType)(carbon::UnixTimeMs() / 1000);
	SignAndSendTransaction(api, keys, "simnet", "main", script, ByteArray{});
	const Timestamp::ValueType after = (Timestamp::ValueType)(carbon::UnixTimeMs() / 1000);

	const Timestamp::ValueType stamped = BroadcastExpiration(node.sentTx).Value;
	Report(ctx, stamped >= before + lifetime && stamped <= after + lifetime,
	    "script transaction stamps the Carbon default lifetime",
	    "stamped " + std::to_string(stamped) + ", expected " + std::to_string(before + lifetime));

	SignAndSendTransaction(api, keys, "simnet", "main", script, ByteArray{}, nullptr, Timestamp(1900000000));
	Report(ctx, BroadcastExpiration(node.sentTx).Value == 1900000000u,
	    "script transaction keeps the expiration the caller passes");

	// An empty script is what the builder answers, without exceptions, for a script it refused. The
	// call sends nothing and says why; with exceptions it throws instead.
	{
		node.sentTx.clear();
		rpc::PhantasmaError error;
		bool refused = false;
		PHANTASMA_TRY
		{
			const String hash = SignAndSendTransaction(api, keys, "simnet", "main", ByteArray{}, ByteArray{}, &error);
			refused = hash.empty() && error.code == rpc::PhantasmaError::Refused;
		}
		PHANTASMA_CATCH_ALL()
		{
			refused = true;
		}
		Report(ctx, refused && node.sentTx.empty(), "script transaction with an empty script is refused and not sent");
	}
}

} // namespace testcases
