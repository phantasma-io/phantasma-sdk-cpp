#include "test_cases.h"

namespace testcases {
using namespace testutil;

// Every builder here answers false and says why instead of raising alone, so each refusal below is
// checked in this build too. PHANTASMA_EXCEPTION expands to nothing unless the caller asks for
// exceptions, and these tests do not.
static ByteArray BuildTokenMetadata()
{
	const std::string png = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR4nGMAAQAABQABDQottAAAAABJRU5ErkJggg==";
	std::vector<std::pair<std::string, std::string>> fields = {
		{ "name", "My test token!" },
		{ "description", "My test token description" },
		{ "icon", png },
		{ "url", "http://example.com" },
	};
	ByteArray out;
	std::string error;
	TokenMetadataBuilder::BuildAndSerialize(fields, out, error);
	return out;
}

void RunTokenBuilderValidationTests(TestContext& ctx)
{
	const ByteArray metadata = BuildTokenMetadata();
	const Bytes32 creator{};
	const intx maxSupply = ParseIntx("0");
	const intx bigSupply = ParseIntx("9223372036854775808");

	ByteArray tokenSchemas;
	{
		std::string error;
		const TokenSchemasOwned standard = TokenSchemasBuilder::PrepareStandardTokenSchemas();
		tokenSchemas = TokenSchemasBuilder::Serialize(standard.View());
		(void)error;
	}

	// One refusal per guard, each read back from the message the builder returns.
	{
		TokenInfoOwned info;
		std::string error;
		ExpectRefused(ctx, "TokenInfoBuilder empty symbol", "Empty string is invalid",
		    TokenInfoBuilder::Build("", maxSupply, false, 0, creator, metadata, info, error), error);
	}
	{
		TokenInfoOwned info;
		std::string error;
		ExpectRefused(ctx, "TokenInfoBuilder long symbol", "Too long",
		    TokenInfoBuilder::Build(std::string(256, 'A'), maxSupply, false, 0, creator, metadata, info, error), error);
	}
	{
		TokenInfoOwned info;
		std::string error;
		ExpectRefused(ctx, "TokenInfoBuilder invalid symbol", "Anything outside A-Z",
		    TokenInfoBuilder::Build("AB1", maxSupply, false, 0, creator, metadata, info, error), error);
	}
	{
		TokenInfoOwned info;
		std::string error;
		const ByteArray empty;
		ExpectRefused(ctx, "TokenInfoBuilder metadata required", "metadata is required",
		    TokenInfoBuilder::Build("ABC", maxSupply, false, 0, creator, empty, info, error), error);
	}
	{
		TokenInfoOwned info;
		std::string error;
		ExpectRefused(ctx, "TokenInfoBuilder NFT supply Int64", "NFT maximum supply must fit into Int64",
		    TokenInfoBuilder::Build("NFT", bigSupply, true, 0, creator, metadata, tokenSchemas, info, error), error);
	}
	{
		TokenInfoOwned info;
		std::string error;
		ExpectRefused(ctx, "TokenInfoBuilder NFT schemas required", "tokenSchemas is required",
		    TokenInfoBuilder::Build("NFT", maxSupply, true, 0, creator, metadata, info, error), error);
	}

	{
		TokenInfoOwned unlimited, finiteSmall, finiteBig, nft;
		std::string error;
		const bool built =
		    TokenInfoBuilder::Build("UNLIMITED", ParseIntx("0"), false, 8, creator, metadata, unlimited, error) &&
		    TokenInfoBuilder::Build("SMALL", ParseIntx("1000000"), false, 8, creator, metadata, finiteSmall, error) &&
		    TokenInfoBuilder::Build("BIG", ParseIntx("9223372036854775808"), false, 8, creator, metadata, finiteBig, error) &&
		    TokenInfoBuilder::Build("NFT", maxSupply, true, 0, creator, metadata, tokenSchemas, nft, error);
		Report(ctx, built, "TokenInfoBuilder accepts every valid shape", error);
		Report(ctx, unlimited.view.flags == TokenFlags_BigFungible, "TokenInfoBuilder unlimited fungible flags");
		Report(ctx, finiteSmall.view.flags == TokenFlags_None, "TokenInfoBuilder finite small fungible flags");
		Report(ctx, finiteBig.view.flags == TokenFlags_BigFungible, "TokenInfoBuilder finite big fungible flags");
		Report(ctx, nft.view.flags == TokenFlags_NonFungible, "TokenInfoBuilder NFT flags");
	}

	// The series metadata is required. This call used to dereference a null pointer here.
	{
		SeriesInfoOwned series;
		std::string error;
		ExpectRefused(ctx, "SeriesInfoBuilder metadata required", "series metadata is required",
		    SeriesInfoBuilder::Build(1, 1, creator, ByteArray{}, series, error), error);
	}
	{
		SeriesInfoOwned series;
		std::string error;
		const ByteArray seriesMetadata = { (Byte)0x01, (Byte)0x02 };
		Report(ctx, SeriesInfoBuilder::Build(1, 1, creator, seriesMetadata, series, error) && series.View().metadata.length == seriesMetadata.size(),
		    "SeriesInfoBuilder keeps the metadata it was given", error);
	}

	{
		TokenSchemasOwned schemas;
		std::string error;
		ExpectRefused(ctx, "TokenSchemasBuilder missing metadata", "Mandatory metadata field not found: name",
		    TokenSchemasBuilder::BuildFromFields({}, {}, {}, schemas, error), error);
	}
	{
		TokenSchemasOwned schemas;
		std::string error;
		std::vector<FieldType> seriesFields = { FieldType{ "name", VmType::Int32 } };
		ExpectRefused(ctx, "TokenSchemasBuilder type mismatch", "Type mismatch for field name",
		    TokenSchemasBuilder::BuildFromFields(seriesFields, {}, {}, schemas, error), error);
	}
	{
		TokenSchemasOwned schemas;
		std::string error;
		std::vector<FieldType> seriesFields = { FieldType{ "Name", VmType::String } };
		ExpectRefused(ctx, "TokenSchemasBuilder case mismatch", "Case mismatch for field name",
		    TokenSchemasBuilder::BuildFromFields(seriesFields, {}, {}, schemas, error), error);
	}
}

// The standard token schemas, as the wire carries them. The bytes are the ones the shared fixture
// `carbon_tx_builder_vectors.tsv` embeds in its create-token NFT case, and every other SDK in the
// set produces them, so a token created through this SDK has to carry the same field names and
// types. The test exists because `TokenSchemasOwned::View()` returned a view into a local copy for
// one day: the copy died with the call and the first field name serialized as whatever was left in
// that memory.
void RunTokenSchemasWireTests(testutil::TestContext& ctx)
{
	const std::string expected =
	    "03000000025F690E046D6F64650603726F6D020007000000025F690E03726F6D02046E616D65160B6465736372"
	    "697074696F6E1608696D61676555524C1607696E666F55524C1609726F79616C746965730A000000000001";

	const TokenSchemasOwned standard = TokenSchemasBuilder::PrepareStandardTokenSchemas();
	const ByteArray blob = CarbonSerialize(standard.View());
	Report(ctx, ToUpper(BytesToHex(blob)) == expected, "the standard token schemas keep their wire form",
	    ToUpper(BytesToHex(blob)));

	// The same object after a copy. A schema that pointed into the object it was copied from would
	// answer with the copy's memory here.
	const TokenSchemasOwned copied = standard;
	const ByteArray copiedBlob = CarbonSerialize(copied.View());
	Report(ctx, ToUpper(BytesToHex(copiedBlob)) == expected, "a copied schema set keeps its wire form",
	    ToUpper(BytesToHex(copiedBlob)));
}

// What the pre-flight makes of a message, before any chain is asked. The chain-facing half needs an
// HTTP client, which the test binary has none of; what is checked here is the part that decides
// whether there is anything to ask about, and the symbol it would ask about.
void RunPreflightSubjectTests(testutil::TestContext& ctx)
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
	if( !TokenMetadataBuilder::BuildAndSerialize(metaFields, metadata, error) )
	{
		Report(ctx, false, "the pre-flight fixture builds its metadata", error);
		return;
	}

	TokenInfoOwned info;
	if( !TokenInfoBuilder::Build("MYFEE", intx((uint64_t)0), false, 2, owner, metadata, info, error) )
	{
		Report(ctx, false, "the pre-flight fixture builds its token info", error);
		return;
	}

	{
		const TxEnvelope env = CreateTokenTxHelper::BuildTx(info.View(), owner);
		String symbol;
		const bool ok = PreflightSubject(env.msg, symbol);
		Report(ctx, ok && symbol == String("MYFEE"), "the pre-flight reads the symbol a creation would register", symbol);
	}
	{
		// Every other message has nothing to check: the fee the pre-flight protects is the token
		// creation's alone.
		const Blockchain::TxMsg msg = NativeTxHelper::TransferFungible({ owner }, owner, 1, 1);
		String symbol;
		Report(ctx, !PreflightSubject(msg, symbol), "a transfer has nothing for the pre-flight to check");
	}
	{
		// A call to another method of the same module is not a creation either.
		Blockchain::TxMsg msg = NativeTxHelper::TransferFungible({ owner }, owner, 1, 1);
		msg.type = Blockchain::TxTypes::Call;
		ByteArray args(8, 0);
		msg.call = Blockchain::TxMsgCall{ (uint32_t)ModuleId::Token, (uint32_t)TokenContract_Methods::BurnFungible,
			ByteView{ args.data(), args.size() }, {} };
		String symbol;
		Report(ctx, !PreflightSubject(msg, symbol), "another token call has nothing for the pre-flight to check");
	}
	{
		// Arguments that are not a token info are refused rather than read as one.
		Blockchain::TxMsg msg = NativeTxHelper::TransferFungible({ owner }, owner, 1, 1);
		msg.type = Blockchain::TxTypes::Call;
		ByteArray args(4, 0x7F);
		msg.call = Blockchain::TxMsgCall{ (uint32_t)ModuleId::Token, (uint32_t)TokenContract_Methods::CreateToken,
			ByteView{ args.data(), args.size() }, {} };
		String symbol;
		Report(ctx, !PreflightSubject(msg, symbol), "a creation whose arguments cannot be read is refused");
	}
}

// A decimal amount a person typed, into the atoms the chain counts.
void RunTokenAmountTests(testutil::TestContext& ctx)
{
	uint64_t atoms = 0;
	Report(ctx, ParseTokenAmount(String("1.25"), 8, atoms) && atoms == 125000000, "a decimal amount becomes atoms");
	Report(ctx, ParseTokenAmount(String("1"), 8, atoms) && atoms == 100000000, "a whole amount becomes atoms");
	Report(ctx, ParseTokenAmount(String("0.0000000001"), 10, atoms) && atoms == 1, "the smallest KCAL amount is one atom");
	Report(ctx, ParseTokenAmount(String("0"), 8, atoms) && atoms == 0, "zero is read");
	Report(ctx, !ParseTokenAmount(String(""), 8, atoms), "an empty amount is refused");
	Report(ctx, !ParseTokenAmount(String("1.2.3"), 8, atoms), "two decimal points are refused");
	Report(ctx, !ParseTokenAmount(String("1,25"), 8, atoms), "a comma is refused");
	Report(ctx, !ParseTokenAmount(String("-1"), 8, atoms), "a negative amount is refused");
	// The refusal that matters most: a digit the token cannot hold would be dropped, and the caller
	// would send an amount it never wrote.
	Report(ctx, !ParseTokenAmount(String("1.123456789"), 8, atoms), "more fractional digits than the token has are refused");
	Report(ctx, !ParseTokenAmount(String("184467440737.09551616"), 8, atoms), "an amount past 64 bits is refused");

	// What FormatTokenAmount writes, ParseTokenAmount reads back.
	const uint64_t original = 4260000000ull;
	Report(ctx, ParseTokenAmount(FormatTokenAmount(original, 10), 10, atoms) && atoms == original,
	    "an amount survives the round trip through its decimal form");
}

} // namespace testcases
