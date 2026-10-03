#include "test_cases.h"

namespace testcases {
using namespace testutil;

// Reading a transaction message back from the wire. The round trip covers every transaction type:
// each message below gives every field a different value, so a reader that drops a field or takes
// two of them in the wrong order writes different bytes on the way out. The rest of the file pins
// what a reader must refuse: a truncated image, an unknown type, and a declared count larger than
// the message that carries it.

namespace {

using Blockchain::TxTypes;

Bytes32 Address(uint8_t seed)
{
	Bytes32 out{};
	for( int i = 0; i != Bytes32::length; ++i )
	{
		out.bytes[i] = (uint8_t)(seed + i);
	}
	return out;
}

Bytes64 Signature(uint8_t seed)
{
	Bytes64 out{};
	for( int i = 0; i != Bytes64::length; ++i )
	{
		out.bytes[i] = (uint8_t)(seed + i);
	}
	return out;
}

ByteView View(const ByteArray& bytes)
{
	return ByteView{ bytes.empty() ? nullptr : bytes.data(), bytes.size() };
}

Blockchain::TxMsg BaseMsg(TxTypes type)
{
	Blockchain::TxMsg msg;
	msg.type = type;
	msg.expiry = 1759711416000ll;
	msg.maxGas = 42600000;
	msg.maxData = 400000;
	msg.gasFrom = Address(1);
	msg.payload = SmallString("reader-test");
	return msg;
}

// The messages the round trip and the truncation tests walk. The list holds one of every type the
// TxTypes enum declares, so a type added to the enum without a case in the reader shows up here as
// a missing entry rather than as silence.
struct Sample {
	std::string name;
	Blockchain::TxMsg msg;
};

// Bytes the sample messages point at. A TxMsg carries views, so what they point at has to outlive
// the message itself.
struct SampleStorage {
	ByteArray callArgs = ByteArray(16, 0x5A);
	ByteArray innerArgs = ByteArray(8, 0x31);
	ByteArray rom = ByteArray(6, 0x71);
	ByteArray ram = ByteArray(3, 0x72);
	ByteArray script = ByteArray(12, 0x73);
	ByteArray rawTransaction = ByteArray(20, 0x74);
	std::vector<uint64_t> instanceIds = { 11, 22, 33 };
	Blockchain::TxMsgCall innerCalls[2] = {};
	Blockchain::TxMsgTransferFungible_GasPayer tradeTransferF[1] = {};
	Blockchain::TxMsgTransferNonFungible_Single_GasPayer tradeTransferN[1] = {};
	Blockchain::TxMsgMintFungible tradeMintF[1] = {};
	Blockchain::TxMsgBurnFungible_GasPayer tradeBurnF[1] = {};
	Blockchain::TxMsgMintNonFungible tradeMintN[1] = {};
	Blockchain::TxMsgBurnNonFungible_GasPayer tradeBurnN[1] = {};
};

Blockchain::TxMsgCall MakeCall(uint32_t moduleId, uint32_t methodId, const ByteArray& args)
{
	Blockchain::TxMsgCall call{};
	call.moduleId = moduleId;
	call.methodId = methodId;
	call.args = View(args);
	return call;
}

std::vector<Sample> MakeSamples(SampleStorage& storage)
{
	std::vector<Sample> samples;

	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::Call);
		msg.call = MakeCall(3, 7, storage.callArgs);
		samples.push_back({ "Call", msg });
	}
	{
		storage.innerCalls[0] = MakeCall(3, 7, storage.innerArgs);
		storage.innerCalls[1] = MakeCall(4, 9, storage.callArgs);
		Blockchain::TxMsg msg = BaseMsg(TxTypes::Call_Multi);
		msg.callMulti.numCalls = 2;
		msg.callMulti.calls = storage.innerCalls;
		samples.push_back({ "Call_Multi", msg });
	}
	{
		storage.tradeTransferF[0] = Blockchain::TxMsgTransferFungible_GasPayer{ Address(20), Address(30), 5, 600 };
		storage.tradeTransferN[0] = Blockchain::TxMsgTransferNonFungible_Single_GasPayer{ Address(21), Address(31), 6, 601 };
		storage.tradeMintF[0].tokenId = 7;
		storage.tradeMintF[0].amount.x() = intx((uint64_t)602);
		storage.tradeMintF[0].to = Address(22);
		storage.tradeBurnF[0].tokenId = 8;
		storage.tradeBurnF[0].amount.x() = intx((uint64_t)603);
		storage.tradeBurnF[0].from = Address(23);
		storage.tradeMintN[0].tokenId = 9;
		storage.tradeMintN[0].to = Address(24);
		storage.tradeMintN[0].seriesId = 4;
		storage.tradeMintN[0].rom = View(storage.rom);
		storage.tradeMintN[0].ram = View(storage.ram);
		storage.tradeBurnN[0] = Blockchain::TxMsgBurnNonFungible_GasPayer{ 10, Address(25), 604 };

		Blockchain::TxMsg msg = BaseMsg(TxTypes::Trade);
		msg.trade = Blockchain::TxMsgTrade{};
		msg.trade.numTransferF = 1;
		msg.trade.transferF = storage.tradeTransferF;
		msg.trade.numTransferN = 1;
		msg.trade.transferN = storage.tradeTransferN;
		msg.trade.numMintF = 1;
		msg.trade.mintF = storage.tradeMintF;
		msg.trade.numBurnF = 1;
		msg.trade.burnF = storage.tradeBurnF;
		msg.trade.numMintN = 1;
		msg.trade.mintN = storage.tradeMintN;
		msg.trade.numBurnN = 1;
		msg.trade.burnN = storage.tradeBurnN;
		samples.push_back({ "Trade", msg });
	}
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::TransferFungible);
		msg.transferFt = Blockchain::TxMsgTransferFungible{ Address(2), 101, 102 };
		samples.push_back({ "TransferFungible", msg });
	}
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::TransferFungible_GasPayer);
		msg.transferFtGasPayer = Blockchain::TxMsgTransferFungible_GasPayer{ Address(3), Address(4), 103, 104 };
		samples.push_back({ "TransferFungible_GasPayer", msg });
	}
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::TransferNonFungible_Single);
		msg.transferNftSingle = Blockchain::TxMsgTransferNonFungible_Single{ Address(5), 105, 106 };
		samples.push_back({ "TransferNonFungible_Single", msg });
	}
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::TransferNonFungible_Single_GasPayer);
		msg.transferNftSingleGasPayer =
		    Blockchain::TxMsgTransferNonFungible_Single_GasPayer{ Address(6), Address(7), 107, 108 };
		samples.push_back({ "TransferNonFungible_Single_GasPayer", msg });
	}
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::TransferNonFungible_Multi);
		msg.transferNftMulti = Blockchain::TxMsgTransferNonFungible_Multi{};
		msg.transferNftMulti.to = Address(8);
		msg.transferNftMulti.tokenId = 109;
		msg.transferNftMulti.numInstanceIds = (uint32_t)storage.instanceIds.size();
		msg.transferNftMulti.instanceIds = storage.instanceIds.data();
		samples.push_back({ "TransferNonFungible_Multi", msg });
	}
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::TransferNonFungible_Multi_GasPayer);
		msg.transferNftMultiGasPayer = Blockchain::TxMsgTransferNonFungible_Multi_GasPayer{};
		msg.transferNftMultiGasPayer.to = Address(9);
		msg.transferNftMultiGasPayer.from = Address(10);
		msg.transferNftMultiGasPayer.tokenId = 110;
		msg.transferNftMultiGasPayer.numInstanceIds = (uint32_t)storage.instanceIds.size();
		msg.transferNftMultiGasPayer.instanceIds = storage.instanceIds.data();
		samples.push_back({ "TransferNonFungible_Multi_GasPayer", msg });
	}
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::MintFungible);
		msg.mintFungible = Blockchain::TxMsgMintFungible{};
		msg.mintFungible.tokenId = 111;
		msg.mintFungible.to = Address(11);
		msg.mintFungible.amount.x() = intx((uint64_t)112);
		samples.push_back({ "MintFungible", msg });
	}
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::BurnFungible);
		msg.burnFungible = Blockchain::TxMsgBurnFungible{};
		msg.burnFungible.tokenId = 113;
		msg.burnFungible.amount.x() = intx((uint64_t)114);
		samples.push_back({ "BurnFungible", msg });
	}
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::BurnFungible_GasPayer);
		msg.burnFungibleGasPayer = Blockchain::TxMsgBurnFungible_GasPayer{};
		msg.burnFungibleGasPayer.tokenId = 115;
		msg.burnFungibleGasPayer.from = Address(12);
		msg.burnFungibleGasPayer.amount.x() = intx((uint64_t)116);
		samples.push_back({ "BurnFungible_GasPayer", msg });
	}
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::MintNonFungible);
		msg.mintNonFungible = Blockchain::TxMsgMintNonFungible{};
		msg.mintNonFungible.tokenId = 117;
		msg.mintNonFungible.to = Address(13);
		msg.mintNonFungible.seriesId = 3;
		msg.mintNonFungible.rom = View(storage.rom);
		msg.mintNonFungible.ram = View(storage.ram);
		samples.push_back({ "MintNonFungible", msg });
	}
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::BurnNonFungible);
		msg.burnNonFungible = Blockchain::TxMsgBurnNonFungible{ 118, 119 };
		samples.push_back({ "BurnNonFungible", msg });
	}
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::BurnNonFungible_GasPayer);
		msg.burnNonFungibleGasPayer = Blockchain::TxMsgBurnNonFungible_GasPayer{ 120, Address(14), 121 };
		samples.push_back({ "BurnNonFungible_GasPayer", msg });
	}
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::Phantasma);
		msg.phantasma = Blockchain::TxMsgPhantasma{};
		msg.phantasma.nexus = SmallString("mainnet");
		msg.phantasma.chain = SmallString("main");
		msg.phantasma.script = View(storage.script);
		samples.push_back({ "Phantasma", msg });
	}
	{
		Blockchain::TxMsg msg = BaseMsg(TxTypes::Phantasma_Raw);
		msg.phantasmaRaw = Blockchain::TxMsgPhantasma_Raw{};
		msg.phantasmaRaw.transaction = View(storage.rawTransaction);
		samples.push_back({ "Phantasma_Raw", msg });
	}

	return samples;
}

void ExpectRoundTrip(TestContext& ctx, const Sample& sample)
{
	const ByteArray image = Blockchain::SerializeTx(sample.msg);
	Allocator alloc;
	Blockchain::TxMsg parsed{};
	if( !Blockchain::ParseTx(parsed, View(image), alloc) )
	{
		Report(ctx, false, "read back " + sample.name, "the message did not parse");
		return;
	}
	const ByteArray again = Blockchain::SerializeTx(parsed);
	Report(ctx, again == image, "read back " + sample.name, BytesToHex(again) + " vs " + BytesToHex(image));
}

// Every prefix of a valid image cuts at least one field the reader needs, so every prefix must be
// refused. This is where a reader that cannot report the end of the stream shows itself.
void ExpectTruncationRefused(TestContext& ctx, const Sample& sample)
{
	const ByteArray image = Blockchain::SerializeTx(sample.msg);
	for( size_t length = 0; length < image.size(); ++length )
	{
		Allocator alloc;
		Blockchain::TxMsg parsed{};
		if( Blockchain::ParseTx(parsed, ByteView{ image.data(), length }, alloc) )
		{
			Report(ctx, false, "refuse a truncated " + sample.name, "accepted a prefix of " + std::to_string(length) + " bytes");
			return;
		}
	}
	Report(ctx, true, "refuse a truncated " + sample.name);
}

void RunRoundTripTests(TestContext& ctx)
{
	SampleStorage storage;
	const std::vector<Sample> samples = MakeSamples(storage);

	// One sample per transaction type, counted so that a type added to the enum without a sample is
	// a failure here and not an untested branch.
	Report(ctx, samples.size() == (size_t)TxTypes::Phantasma_Raw + 1, "a sample exists for every transaction type",
	    std::to_string(samples.size()) + " samples");

	for( const Sample& sample : samples )
	{
		ExpectRoundTrip(ctx, sample);
		ExpectTruncationRefused(ctx, sample);
	}
}

void RunRefusalTests(TestContext& ctx)
{
	SampleStorage storage;
	const std::vector<Sample> samples = MakeSamples(storage);

	{
		// A type byte outside the enum names no payload layout.
		ByteArray image = Blockchain::SerializeTx(samples.front().msg);
		image[0] = 0x7F;
		Allocator alloc;
		Blockchain::TxMsg parsed{};
		Report(ctx, !Blockchain::ParseTx(parsed, View(image), alloc), "refuse an unknown transaction type");
	}

	{
		// A count of instance ids far larger than the message that declares it. The reader must
		// refuse before it allocates anything.
		ByteArray buffer;
		WriteView w(buffer);
		Write1((uint8_t)TxTypes::TransferNonFungible_Multi, w);
		Write8((int64_t)1759711416000ll, w);
		Write8u(42600000, w);
		Write8u(400000, w);
		Write(Address(1), w);
		Write(SmallString(""), w);
		Write(Address(2), w);
		Write8u(101, w);
		Write4((int32_t)0x7FFFFFFF, w);

		Allocator alloc;
		Blockchain::TxMsg parsed{};
		Report(ctx, !Blockchain::ParseTx(parsed, View(buffer), alloc), "refuse an instance count larger than the message");
	}

	{
		// A payload whose length byte promises more characters than the image carries. The string
		// reader has to report that, or the message parses with characters nobody wrote.
		ByteArray buffer;
		WriteView w(buffer);
		Write1((uint8_t)TxTypes::BurnNonFungible, w);
		Write8((int64_t)1759711416000ll, w);
		Write8u(42600000, w);
		Write8u(400000, w);
		Write(Address(1), w);
		Write1((uint8_t)12, w);
		Write1((uint8_t)'a', w);

		Allocator alloc;
		Blockchain::TxMsg parsed{};
		Report(ctx, !Blockchain::ParseTx(parsed, View(buffer), alloc), "refuse a payload cut short of its length");
	}
}

// The wire order of the two messages that carry an address and an amount. The bytes here are built
// field by field, so the test states the order itself and does not agree with the writer by
// construction. Reading these in the wrong order parses without an error and yields a different
// token and a different amount.
void RunFieldOrderTests(TestContext& ctx)
{
	{
		ByteArray buffer;
		WriteView w(buffer);
		Write1((uint8_t)TxTypes::MintFungible, w);
		Write8((int64_t)1759711416000ll, w);
		Write8u(42600000, w);
		Write8u(400000, w);
		Write(Address(1), w);
		Write(SmallString(""), w);
		Write8u(0x1122334455667788ull, w);
		Write(Address(2), w);
		Write(intx((uint64_t)500), w);

		Allocator alloc;
		Blockchain::TxMsg parsed{};
		const bool ok = Blockchain::ParseTx(parsed, View(buffer), alloc);
		const bool fieldsOk = ok && parsed.mintFungible.tokenId == 0x1122334455667788ull &&
		                      parsed.mintFungible.to == Address(2) && parsed.mintFungible.amount.x() == intx((uint64_t)500);
		Report(ctx, fieldsOk, "MintFungible carries the address before the amount");
	}

	{
		ByteArray buffer;
		WriteView w(buffer);
		Write1((uint8_t)TxTypes::BurnFungible_GasPayer, w);
		Write8((int64_t)1759711416000ll, w);
		Write8u(42600000, w);
		Write8u(400000, w);
		Write(Address(1), w);
		Write(SmallString(""), w);
		Write8u(0x99AABBCCDDEEFF00ull, w);
		Write(Address(3), w);
		Write(intx((uint64_t)700), w);

		Allocator alloc;
		Blockchain::TxMsg parsed{};
		const bool ok = Blockchain::ParseTx(parsed, View(buffer), alloc);
		const bool fieldsOk = ok && parsed.burnFungibleGasPayer.tokenId == 0x99AABBCCDDEEFF00ull &&
		                      parsed.burnFungibleGasPayer.from == Address(3) &&
		                      parsed.burnFungibleGasPayer.amount.x() == intx((uint64_t)700);
		Report(ctx, fieldsOk, "BurnFungible_GasPayer carries the address before the amount");
	}
}

// The three witness layouts a signed envelope can have, and the part of it a signature is made over.
void RunSignedEnvelopeTests(TestContext& ctx)
{
	const std::string wif = "KwPpBSByydVKqStGHAnZzQofCqhDmD2bfRgc9BmZqM3ZmsdWJw4d";
	const PhantasmaKeys sender = PhantasmaKeys::FromWIF(wif.c_str(), (int)wif.size());
	const Bytes32 senderPub = ToBytes32(sender.GetPublicKey());

	{
		// One bare signature, and the signer is the gas payer the message already names.
		Blockchain::TxMsg msg = BaseMsg(TxTypes::TransferFungible);
		msg.gasFrom = senderPub;
		msg.transferFt = Blockchain::TxMsgTransferFungible{ Address(2), 101, 102 };
		const ByteArray envelope = Blockchain::TxMsgSigner::SignAndSerialize(msg, sender);

		Allocator alloc;
		Blockchain::SignedTxMsg parsed{};
		ByteView signedPortion{};
		const bool ok = Blockchain::ParseSignedTx(parsed, signedPortion, View(envelope), alloc);
		const bool fieldsOk = ok && parsed.witnesses.numWitnesses == 1 && parsed.witnesses.witnesses[0].address == senderPub &&
		                      BytesFromView(signedPortion) == Blockchain::SerializeTx(msg);
		Report(ctx, fieldsOk, "read a singly signed envelope");
	}

	{
		// Two bare signatures: the gas payer first, then the account that owns the assets.
		Blockchain::TxMsg msg = BaseMsg(TxTypes::BurnFungible_GasPayer);
		msg.burnFungibleGasPayer = Blockchain::TxMsgBurnFungible_GasPayer{};
		msg.burnFungibleGasPayer.tokenId = 115;
		msg.burnFungibleGasPayer.from = Address(12);
		msg.burnFungibleGasPayer.amount.x() = intx((uint64_t)116);

		const Witness witnesses[2] = { { msg.gasFrom, Signature(1) }, { Address(12), Signature(2) } };
		Blockchain::SignedTxMsg signedMsg;
		signedMsg.msg = msg;
		signedMsg.witnesses = Witnesses{ 2, witnesses };

		ByteArray envelope;
		WriteView w(envelope);
		Write(signedMsg, w);

		Allocator alloc;
		Blockchain::SignedTxMsg parsed{};
		ByteView signedPortion{};
		const bool ok = Blockchain::ParseSignedTx(parsed, signedPortion, View(envelope), alloc);
		const bool fieldsOk = ok && parsed.witnesses.numWitnesses == 2 && parsed.witnesses.witnesses[0] == witnesses[0] &&
		                      parsed.witnesses.witnesses[1] == witnesses[1];
		Report(ctx, fieldsOk, "read a gas-payer envelope with both signers");
	}

	{
		// A counted array of address plus signature, which is what the call types carry.
		ByteArray args(16, 0x5A);
		Blockchain::TxMsg msg = BaseMsg(TxTypes::Call);
		msg.call = MakeCall(3, 7, args);

		const Witness witnesses[2] = { { Address(40), Signature(3) }, { Address(50), Signature(4) } };
		Blockchain::SignedTxMsg signedMsg;
		signedMsg.msg = msg;
		signedMsg.witnesses = Witnesses{ 2, witnesses };

		ByteArray envelope;
		WriteView w(envelope);
		Write(signedMsg, w);

		Allocator alloc;
		Blockchain::SignedTxMsg parsed{};
		ByteView signedPortion{};
		const bool ok = Blockchain::ParseSignedTx(parsed, signedPortion, View(envelope), alloc);
		const bool fieldsOk = ok && parsed.witnesses.numWitnesses == 2 && parsed.witnesses.witnesses[0] == witnesses[0] &&
		                      parsed.witnesses.witnesses[1] == witnesses[1] &&
		                      BytesFromView(signedPortion) == Blockchain::SerializeTx(msg);
		Report(ctx, fieldsOk, "read a call envelope with its witness array");
	}

	{
		// A raw Gen2 envelope keeps its signatures inside its own payload and appends none.
		ByteArray rawTransaction(20, 0x74);
		Blockchain::TxMsg msg = BaseMsg(TxTypes::Phantasma_Raw);
		msg.phantasmaRaw = Blockchain::TxMsgPhantasma_Raw{};
		msg.phantasmaRaw.transaction = View(rawTransaction);

		Blockchain::SignedTxMsg signedMsg;
		signedMsg.msg = msg;
		signedMsg.witnesses = Witnesses{ 0, nullptr };

		ByteArray envelope;
		WriteView w(envelope);
		Write(signedMsg, w);

		Allocator alloc;
		Blockchain::SignedTxMsg parsed{};
		ByteView signedPortion{};
		const bool ok = Blockchain::ParseSignedTx(parsed, signedPortion, View(envelope), alloc);
		Report(ctx, ok && parsed.witnesses.numWitnesses == 0, "read a raw envelope that appends no signature");
	}

	{
		// A singly signed envelope without its signature. The witness layout is part of the image,
		// so a missing signature is a refusal and not an envelope with an empty witness.
		Blockchain::TxMsg msg = BaseMsg(TxTypes::TransferFungible);
		msg.gasFrom = senderPub;
		msg.transferFt = Blockchain::TxMsgTransferFungible{ Address(2), 101, 102 };
		const ByteArray image = Blockchain::SerializeTx(msg);

		Allocator alloc;
		Blockchain::SignedTxMsg parsed{};
		ByteView signedPortion{};
		Report(ctx, !Blockchain::ParseSignedTx(parsed, signedPortion, View(image), alloc),
		    "refuse an envelope whose signature is missing");
	}
}

// Signing a message with more than one key. A _GasPayer message needs two signatures and a call
// needs as many as the caller chose; the SDK could produce only one until now.
void RunMultiSignerTests(TestContext& ctx)
{
	const std::string gasWif = "KwPpBSByydVKqStGHAnZzQofCqhDmD2bfRgc9BmZqM3ZmsdWJw4d";
	const std::string assetWif = "KwVG94yjfVg1YKFyRxAGtug93wdRbmLnqqrFV6Yd2CiA9KZDAp4H";
	const PhantasmaKeys gasPayer = PhantasmaKeys::FromWIF(gasWif.c_str(), (int)gasWif.size());
	const PhantasmaKeys assetOwner = PhantasmaKeys::FromWIF(assetWif.c_str(), (int)assetWif.size());
	const Bytes32 gasPub = ToBytes32(gasPayer.GetPublicKey());
	const Bytes32 assetPub = ToBytes32(assetOwner.GetPublicKey());

	Blockchain::TxMsg msg = BaseMsg(TxTypes::BurnFungible_GasPayer);
	msg.gasFrom = gasPub;
	msg.burnFungibleGasPayer = Blockchain::TxMsgBurnFungible_GasPayer{};
	msg.burnFungibleGasPayer.tokenId = 7;
	msg.burnFungibleGasPayer.from = assetPub;
	msg.burnFungibleGasPayer.amount.x() = intx((uint64_t)3);

	{
		// Both signatures, in the order the type fixes. The envelope has to read back with both
		// signers named, and each signature has to verify against the message they signed.
		ByteArray envelope;
		std::string error;
		const bool ok = Blockchain::TxMsgSigner::SignAndSerialize(msg, { &gasPayer, &assetOwner }, envelope, error);

		Allocator alloc;
		Blockchain::SignedTxMsg parsed{};
		ByteView signedPortion{};
		const bool read = ok && Blockchain::ParseSignedTx(parsed, signedPortion, View(envelope), alloc);
		const bool fieldsOk = read && parsed.witnesses.numWitnesses == 2 &&
		                      parsed.witnesses.witnesses[0].address == gasPub &&
		                      parsed.witnesses.witnesses[1].address == assetPub &&
		                      BytesFromView(signedPortion) == Blockchain::SerializeTx(msg);
		Report(ctx, fieldsOk, "a gas-payer message is signed by both accounts", error);

		if( fieldsOk )
		{
			const ByteArray signed_ = BytesFromView(signedPortion);
			const phantasma::Address gasAddress = gasPayer.GetAddress();
			const phantasma::Address assetAddress = assetOwner.GetAddress();
			const bool gasOk = Ed25519Signature((const Byte*)parsed.witnesses.witnesses[0].signature.bytes, Bytes64::length)
			                       .Verify(signed_.data(), (int)signed_.size(), &gasAddress, 1);
			const bool assetOk = Ed25519Signature((const Byte*)parsed.witnesses.witnesses[1].signature.bytes, Bytes64::length)
			                         .Verify(signed_.data(), (int)signed_.size(), &assetAddress, 1);
			Report(ctx, gasOk && assetOk, "both signatures verify against the signed portion");
		}
	}
	{
		// The order is not the caller's choice for this type: the gas payer signs first.
		ByteArray envelope;
		std::string error;
		const bool ok = Blockchain::TxMsgSigner::SignAndSerialize(msg, { &assetOwner, &gasPayer }, envelope, error);
		ExpectRefused(ctx, "a gas-payer message refuses the signers in the wrong order", "gas payer", ok, error);
	}
	{
		// One signature is not enough for a type that fixes two.
		ByteArray envelope;
		std::string error;
		const bool ok = Blockchain::TxMsgSigner::SignAndSerialize(msg, { &gasPayer }, envelope, error);
		ExpectRefused(ctx, "a gas-payer message refuses a single signer", "fixes how many", ok, error);
	}
	{
		// A call carries a witness array, so it takes as many signers as the caller gives it.
		ByteArray args(16, 0x5A);
		Blockchain::TxMsg call = BaseMsg(TxTypes::Call);
		call.gasFrom = gasPub;
		call.call = MakeCall(3, 7, args);

		ByteArray envelope;
		std::string error;
		const bool ok = Blockchain::TxMsgSigner::SignAndSerialize(call, { &gasPayer, &assetOwner }, envelope, error);

		Allocator alloc;
		Blockchain::SignedTxMsg parsed{};
		ByteView signedPortion{};
		const bool read = ok && Blockchain::ParseSignedTx(parsed, signedPortion, View(envelope), alloc);
		Report(ctx, read && parsed.witnesses.numWitnesses == 2 && parsed.witnesses.witnesses[0].address == gasPub && parsed.witnesses.witnesses[1].address == assetPub,
		    "a call takes as many witnesses as it was given", error);
	}
	{
		ByteArray envelope;
		std::string error;
		const bool ok = Blockchain::TxMsgSigner::SignAndSerialize(msg, {}, envelope, error);
		ExpectRefused(ctx, "signing refuses an empty signer list", "at least one signer", ok, error);
	}
	{
		// A message whose fees were never planned carries a zero gas offer. Both signing calls refuse
		// it. The one-key call has no error string: it throws when exceptions are enabled and answers
		// an empty envelope when they are not, as in this test binary.
		Blockchain::TxMsg unplanned = msg;
		unplanned.maxGas = 0;
		ByteArray envelope;
		std::string error;
		const bool ok = Blockchain::TxMsgSigner::SignAndSerialize(unplanned, { &gasPayer, &assetOwner }, envelope, error);
		ExpectRefused(ctx, "signing refuses a message with no gas offer", "no gas offer", ok, error);

		bool refused = false;
		PHANTASMA_TRY
		{
			refused = Blockchain::TxMsgSigner::SignAndSerialize(unplanned, gasPayer).empty();
		}
		PHANTASMA_CATCH_ALL()
		{
			refused = true;
		}
		Report(ctx, refused, "signing with one key refuses a message with no gas offer");
	}
}

// The native builders. Each one must produce the message type the chain prices, with the accounts
// in the places that type names, and it must leave the offer at zero until a plan sets it.
void RunNativeBuilderTests(TestContext& ctx)
{
	const Bytes32 owner = Address(1);
	const Bytes32 to = Address(2);
	const Bytes32 payer = Address(3);
	const uint64_t instances[3] = { 11, 22, 33 };

	{
		const Blockchain::TxMsg msg = NativeTxHelper::TransferFungible({ owner, nullptr }, to, 5, 700);
		Report(ctx,
		    msg.type == TxTypes::TransferFungible && msg.gasFrom == owner && msg.transferFt.to == to &&
		        msg.transferFt.tokenId == 5 && msg.transferFt.amount == 700 && msg.maxGas == 0 && msg.expiry != 0,
		    "a fungible transfer is built unplanned, with the sender paying");
	}
	{
		const Blockchain::TxMsg msg = NativeTxHelper::TransferFungible({ owner, &payer }, to, 5, 700);
		Report(ctx,
		    msg.type == TxTypes::TransferFungible_GasPayer && msg.gasFrom == payer &&
		        msg.transferFtGasPayer.from == owner && msg.transferFtGasPayer.to == to,
		    "naming a gas payer selects the two-signature transfer");
	}
	{
		Blockchain::TxMsg msg;
		std::string error;
		const bool ok = NativeTxHelper::TransferNonFungible({ owner, nullptr }, to, 9, instances, 1, msg, error);
		Report(ctx,
		    ok && msg.type == TxTypes::TransferNonFungible_Single && msg.transferNftSingle.instanceId == 11,
		    "one instance uses the single-instance type", error);
	}
	{
		Blockchain::TxMsg msg;
		std::string error;
		const bool ok = NativeTxHelper::TransferNonFungible({ owner, &payer }, to, 9, instances, 3, msg, error);
		Report(ctx,
		    ok && msg.type == TxTypes::TransferNonFungible_Multi_GasPayer && msg.gasFrom == payer &&
		        msg.transferNftMultiGasPayer.numInstanceIds == 3 &&
		        msg.transferNftMultiGasPayer.instanceIds == instances,
		    "three instances with a gas payer use the multi-instance gas-payer type", error);
	}
	{
		Blockchain::TxMsg msg;
		std::string error;
		const bool ok = NativeTxHelper::TransferNonFungible({ owner, nullptr }, to, 9, instances, 0, msg, error);
		ExpectRefused(ctx, "an NFT transfer refuses an empty instance list", "must not be empty", ok, error);
	}
	{
		const Blockchain::TxMsg msg = NativeTxHelper::MintFungible(owner, to, 5, intx((uint64_t)900));
		Report(ctx,
		    msg.type == TxTypes::MintFungible && msg.gasFrom == owner && msg.mintFungible.to == to &&
		        msg.mintFungible.amount.x() == intx((uint64_t)900),
		    "a mint is paid for by the token owner");
	}
	{
		const Blockchain::TxMsg msg = NativeTxHelper::BurnFungible({ owner, &payer }, 5, intx((uint64_t)3));
		Report(ctx,
		    msg.type == TxTypes::BurnFungible_GasPayer && msg.gasFrom == payer && msg.burnFungibleGasPayer.from == owner,
		    "a gas-payer burn names the account the atoms leave");
	}
	{
		const Blockchain::TxMsg msg = NativeTxHelper::BurnNonFungible({ owner, nullptr }, 9, 77);
		Report(ctx, msg.type == TxTypes::BurnNonFungible && msg.burnNonFungible.instanceId == 77,
		    "an NFT burn carries the instance");
	}
	{
		// A builder writes the limits it is given, and the planner is what fills them normally.
		const TxLimits limits{ 1234, 5678, 99 };
		const Blockchain::TxMsg msg = NativeTxHelper::TransferFungible({ owner, nullptr }, to, 5, 700, limits);
		Report(ctx, msg.maxGas == 1234 && msg.maxData == 5678 && msg.expiry == 99,
		    "a builder writes the limits it was given");
	}
}

} // namespace

void RunTxReaderTests(testutil::TestContext& ctx)
{
	RunNativeBuilderTests(ctx);
	RunMultiSignerTests(ctx);
	RunRoundTripTests(ctx);
	RunRefusalTests(ctx);
	RunFieldOrderTests(ctx);
	RunSignedEnvelopeTests(ctx);
}

} // namespace testcases
