#include "test_cases.h"

namespace testcases {
using namespace testutil;

void RunCarbonTxExtraTests(TestContext& ctx)
{
	const std::string senderWif = "KwPpBSByydVKqStGHAnZzQofCqhDmD2bfRgc9BmZqM3ZmsdWJw4d";
	const std::string receiverWif = "KwVG94yjfVg1YKFyRxAGtug93wdRbmLnqqrFV6Yd2CiA9KZDAp4H";
	const PhantasmaKeys sender = PhantasmaKeys::FromWIF(senderWif.c_str(), (int)senderWif.size());
	const PhantasmaKeys receiver = PhantasmaKeys::FromWIF(receiverWif.c_str(), (int)receiverWif.size());
	const Bytes32 senderPub = ToBytes32(sender.GetPublicKey());
	const Bytes32 receiverPub = ToBytes32(receiver.GetPublicKey());

	const int64_t expiry = 1759711416000;
	const uint64_t maxGas = 10000000;
	const uint64_t maxData = 1000;
	const SmallString payload("test-payload");
	const intx amount = ParseIntx("100000000");

	{
		Blockchain::TxMsg msg;
		msg.type = Blockchain::TxTypes::TransferFungible_GasPayer;
		msg.expiry = expiry;
		msg.maxGas = maxGas;
		msg.maxData = maxData;
		msg.gasFrom = senderPub;
		msg.payload = payload;
		msg.transferFtGasPayer = Blockchain::TxMsgTransferFungible_GasPayer{ receiverPub, senderPub, 1, 100000000 };

		const std::string expected =
		    "04C04EF9B6990100008096980000000000E803000000000000F94A8E45BDF1E37A8466B951849E92D1BAF870F49D1D04CD204D0BC9FE4308960C746573742D7061796C6F6164D4C5061B81C4682B27A0CFC6459CD9D7892EB60A43F73DD1060B6C478AA7C3D8F94A8E45BDF1E37A8466B951849E92D1BAF870F49D1D04CD204D0BC9FE430896010000000000000000E1F50500000000";
		const std::string got = ToUpper(BytesToHex(CarbonSerialize(msg)));
		Report(ctx, got == expected, "TxMsg TransferFungible GasPayer vector", got + " vs " + expected);
	}

	{
		Blockchain::TxMsg msg;
		msg.type = Blockchain::TxTypes::BurnFungible_GasPayer;
		msg.expiry = expiry;
		msg.maxGas = maxGas;
		msg.maxData = maxData;
		msg.gasFrom = senderPub;
		msg.payload = payload;
		msg.burnFungibleGasPayer = Blockchain::TxMsgBurnFungible_GasPayer{ 1, (const intx_pod&)amount, senderPub };

		const std::string expected =
		    "0BC04EF9B6990100008096980000000000E803000000000000F94A8E45BDF1E37A8466B951849E92D1BAF870F49D1D04CD204D0BC9FE4308960C746573742D7061796C6F61640100000000000000F94A8E45BDF1E37A8466B951849E92D1BAF870F49D1D04CD204D0BC9FE4308960800E1F50500000000";
		const std::string got = ToUpper(BytesToHex(CarbonSerialize(msg)));
		Report(ctx, got == expected, "TxMsg BurnFungible GasPayer vector", got + " vs " + expected);
	}

	{
		Blockchain::TxMsg msg;
		msg.type = Blockchain::TxTypes::MintFungible;
		msg.expiry = expiry;
		msg.maxGas = maxGas;
		msg.maxData = maxData;
		msg.gasFrom = senderPub;
		msg.payload = payload;
		msg.mintFungible = Blockchain::TxMsgMintFungible{ 1, (const intx_pod&)amount, receiverPub };

		const std::string expected =
		    "09C04EF9B6990100008096980000000000E803000000000000F94A8E45BDF1E37A8466B951849E92D1BAF870F49D1D04CD204D0BC9FE4308960C746573742D7061796C6F61640100000000000000D4C5061B81C4682B27A0CFC6459CD9D7892EB60A43F73DD1060B6C478AA7C3D80800E1F50500000000";
		const std::string got = ToUpper(BytesToHex(CarbonSerialize(msg)));
		Report(ctx, got == expected, "TxMsg MintFungible vector", got + " vs " + expected);
	}

	{
		TxLimits limits{};
		limits.expiry = expiry;
		TxEnvelope refused;
		std::string error;
		ExpectRefused(ctx, "MintPhantasmaNonFungibleTxHelper rejects null tokens pointer", "tokens is required",
		    MintPhantasmaNonFungibleTxHelper::BuildTx(42, senderPub, receiverPub, 1, nullptr, refused, error, limits), error);
		ExpectRefused(ctx, "MintPhantasmaNonFungibleTxHelper rejects zero-count mint", "must not be empty",
		    MintPhantasmaNonFungibleTxHelper::BuildTx(42, senderPub, receiverPub, 0, nullptr, refused, error, limits), error);
	}

	// A builder carries no prices: the offer stays zero until the message is planned, and the
	// caller's own limits are written through unchanged.
	{
		PhantasmaNftMintInfo tokens[3]{};
		tokens[0].phantasmaSeriesId.x() = intx((uint64_t)1);
		tokens[1].phantasmaSeriesId.x() = intx((uint64_t)2);
		tokens[2].phantasmaSeriesId.x() = intx((uint64_t)3);

		TxEnvelope unplanned;
		std::string buildError;
		Report(ctx, MintPhantasmaNonFungibleTxHelper::BuildTx(42, senderPub, receiverPub, 3, tokens, unplanned, buildError),
		    "MintPhantasmaNonFungibleTxHelper builds an unplanned mint", buildError);
		Report(ctx, unplanned.msg.maxGas == 0 && unplanned.msg.maxData == 0,
		    "a builder leaves the message unplanned");
		Report(ctx, unplanned.msg.expiry > UnixTimeMs() && unplanned.msg.expiry <= UnixTimeMs() + DefaultExpiryMs,
		    "a builder stamps the default expiry");

		TxLimits limits{};
		limits.maxGas = 30000;
		limits.maxData = 123;
		limits.expiry = expiry;
		TxEnvelope planned;
		Report(ctx, MintPhantasmaNonFungibleTxHelper::BuildTx(42, senderPub, receiverPub, 3, tokens, planned, buildError, limits),
		    "MintPhantasmaNonFungibleTxHelper builds a mint with limits", buildError);
		Report(ctx, planned.msg.maxGas == 30000 && planned.msg.maxData == 123 && planned.msg.expiry == expiry,
		    "a builder writes the limits it was given");
	}

	// The chain refuses an expiry at or beyond now + expiryWindow, so the whole window is available
	// only less a margin for the clock it is compared against.
	{
		const int64_t within = ExpiryWithin(3600000, 5000);
		Report(ctx, within > UnixTimeMs() + 3590000 && within <= UnixTimeMs() + 3595000,
		    "ExpiryWithin takes the chain window less the margin");
	}

	// Token.CreateToken answers the new token id as a u64; a 4-byte read would keep only its low half.
	{
		ByteArray payload;
		WriteView w(payload);
		Write8u(0x0000000500000007ull, w);
		Report(ctx, CreateTokenTxHelper::ParseResult(ToUpper(BytesToHex(payload))) == 0x0000000500000007ull,
		    "CreateTokenTxHelper ParseResult reads the whole u64 token id");
	}

	ExpectNoThrow(ctx, "MintPhantasmaNonFungibleTxHelper ParseResult preserves exact 32-byte Phantasma ids", [&]()
	    {
		PhantasmaNftMintResult low{};
		low.phantasmaNftId.bytes[0] = 0x7B;
		low.carbonInstanceId = 7;

		PhantasmaNftMintResult high{};
		high.phantasmaNftId.bytes[0] = 0x2A;
		high.phantasmaNftId.bytes[31] = 0x80;
		high.carbonInstanceId = 8;

		ByteArray payload;
		WriteView w(payload);
		Write((uint32_t)2, w);
		Write(low, w);
		Write(high, w);

		const std::vector<PhantasmaNftMintResult> parsed = MintPhantasmaNonFungibleTxHelper::ParseResult(ToUpper(BytesToHex(payload)));
		if( parsed.size() != 2 )
		{
			throw std::runtime_error("unexpected safe-mint result count");
		}
		if( !(parsed[0].phantasmaNftId == low.phantasmaNftId) || parsed[0].carbonInstanceId != 7 )
		{
			throw std::runtime_error("unexpected first safe-mint result");
		}
		if( !(parsed[1].phantasmaNftId == high.phantasmaNftId) || parsed[1].carbonInstanceId != 8 )
		{
			throw std::runtime_error("unexpected second safe-mint result");
		} });
}

} // namespace testcases
