#include "test_cases.h"

namespace testcases {
using namespace testutil;

void RunScriptBuilderTransactionTests(TestContext& ctx)
{
	const std::string expectedScriptHex =
	    "0D00030350340303000D000302102703000D000223220000000000000000000000000000000000000000000000000000000000000000000003000D000223220100AA53BE71FC41BC0889B694F4D6D03F7906A3D9A21705943CAF9632EEAFBB489503000D000408416C6C6F7747617303000D0004036761732D00012E010D0003010003000D00041D73797374656D2E6E657875732E70726F746F636F6C2E76657273696F6E03000D00042F50324B464579466576705166536157384734566A536D6857555A585234517247395951523148624D7054554370434C03000D00040A53696E676C65566F746503000D000409636F6E73656E7375732D00012E010D000223220100AA53BE71FC41BC0889B694F4D6D03F7906A3D9A21705943CAF9632EEAFBB489503000D0004085370656E6447617303000D0004036761732D00012E010B";
	const std::string expectedSignedTxHex =
	    "07746573746E6574046D61696EFD42010D00030350340303000D000302102703000D000223220000000000000000000000000000000000000000000000000000000000000000000003000D000223220100AA53BE71FC41BC0889B694F4D6D03F7906A3D9A21705943CAF9632EEAFBB489503000D000408416C6C6F7747617303000D0004036761732D00012E010D0003010003000D00041D73797374656D2E6E657875732E70726F746F636F6C2E76657273696F6E03000D00042F50324B464579466576705166536157384734566A536D6857555A585234517247395951523148624D7054554370434C03000D00040A53696E676C65566F746503000D000409636F6E73656E7375732D00012E010D000223220100AA53BE71FC41BC0889B694F4D6D03F7906A3D9A21705943CAF9632EEAFBB489503000D0004085370656E6447617303000D0004036761732D00012E010BD202964909436F6E73656E737573010140F1C0410D49A5EDF0945B0EE9FAFDF6CA1FC315118D545E07824BEF1BA1F00881C29419648FD0B8200A356D21FAF45C60F4B77279D931CE4D732F5896E93BFE0D";
	const std::string knownTxHex =
	    "07746573746E6574046D61696E03010203D2029649077061796C6F61640101404C033859A20A4FC2E469B3741FB05ACEDFEC24BFE92E07633680488665D79F916773FF40D0E81C4468E1C1487E6E1E6EEFDA5C5D7C53C15C4FB349C2349A1802";

	// A jump resolves to the offset after its label's NOP. A jump to a label the script never defines
	// is refused: the builder throws when exceptions are enabled and answers an empty script when they
	// are not, as in this test binary.
	{
		ScriptBuilder sb;
		sb.EmitJump(Opcode::JMP, "end").Emit(Opcode::NOP).EmitLabel("end");
		const ByteArray jumpScript = sb.EndScript();
		Report(ctx, jumpScript.size() > 2 && jumpScript[1] == 5 && jumpScript[2] == 0, "ScriptBuilder resolves a jump to its label");
	}
	{
		bool refused = false;
		PHANTASMA_TRY
		{
			ScriptBuilder sb;
			sb.EmitJump(Opcode::JMP, "nowhere");
			refused = sb.EndScript().empty();
		}
		PHANTASMA_CATCH_ALL()
		{
			refused = true;
		}
		Report(ctx, refused, "ScriptBuilder refuses a jump to an unknown label");
	}
	// The chain reads a jump target as a signed 16-bit number, so a jump reaches offset 32767 at most,
	// and a call target as an unsigned one. A farther target is refused like an unknown label.
	{
		// A JMP takes 3 bytes and the label's NOP 1, so after `fill` zero bytes the label sits at
		// fill + 4. A CALL takes 4 bytes, so its label sits at fill + 5.
		const auto jumpOver = [](int fill)
		{
			const ByteArray zeros((size_t)fill, (Byte)0);
			ScriptBuilder sb;
			sb.EmitJump(Opcode::JMP, "far").EmitRaw(zeros.data(), fill).EmitLabel("far");
			return sb.EndScript();
		};
		const auto callOver = [](int fill)
		{
			const ByteArray zeros((size_t)fill, (Byte)0);
			ScriptBuilder sb;
			sb.EmitCall("far", 1).EmitRaw(zeros.data(), fill).EmitLabel("far");
			return sb.EndScript();
		};
		const auto refused = [](const auto& build, int fill)
		{
			bool empty = false;
			PHANTASMA_TRY
			{
				empty = build(fill).empty();
			}
			PHANTASMA_CATCH_ALL()
			{
				empty = true;
			}
			return empty;
		};
		Report(ctx, !jumpOver(32763).empty(), "ScriptBuilder takes a jump to offset 32767");
		Report(ctx, refused(jumpOver, 32764), "ScriptBuilder refuses a jump to offset 32768");
		// 32769 is 0x8001: beyond the jump limit, and still a valid call target.
		const ByteArray call = callOver(32764);
		Report(ctx, call.size() > 3 && call[2] == 0x01 && call[3] == 0x80, "ScriptBuilder takes a call to offset 32769");
		Report(ctx, !callOver(65530).empty(), "ScriptBuilder takes a call to offset 65535");
		// 65536 does not fit two bytes. Without the check it would be cut to 0, a call to the start of
		// the script.
		Report(ctx, refused(callOver, 65531), "ScriptBuilder refuses a call to offset 65536");
	}

	const ByteArray script = BuildConsensusSingleVoteScript();
	const std::string scriptHex = ToUpper(BytesToHex(script));
	const std::string expectedScript = ToUpper(expectedScriptHex);
	Report(ctx, scriptHex == expectedScript, "ScriptBuilder vector", scriptHex + " vs " + expectedScript);

	const std::string wif = "L5UEVHBjujaR1721aZM5Zm5ayjDyamMZS9W35RE9Y9giRkdf3dVx";
	const PhantasmaKeys keys = PhantasmaKeys::FromWIF(wif.c_str(), (int)wif.size());
	const Timestamp expiration(1234567890);
	const char payloadText[] = "Consensus";
	const ByteArray payload(payloadText, payloadText + sizeof(payloadText) - 1);

	Transaction tx("testnet", "main", script, expiration, payload);
	tx.Sign(keys);
	const ByteArray signedTx = tx.ToByteArray(true);
	const std::string signedHex = ToUpper(BytesToHex(signedTx));
	const std::string expectedSigned = ToUpper(expectedSignedTxHex);
	Report(ctx, signedHex == expectedSigned, "Transaction signed vector", signedHex + " vs " + expectedSigned);
	Report(ctx, tx.SignatureIndex(keys.GetAddress()) == 0, "Transaction signature index");

	const PhantasmaKeys otherKeys = PhantasmaKeys::Generate();
	Report(ctx, tx.SignatureIndex(otherKeys.GetAddress()) == -1, "Transaction signature index mismatch");

	const ByteArray knownBytes = HexToBytes(knownTxHex);
	BinaryReader reader(knownBytes);
	const Transaction knownTx = Transaction::Unserialize(reader);
	const bool knownOk =
	    knownTx.NexusName() == PHANTASMA_LITERAL("testnet") &&
	    knownTx.ChainName() == PHANTASMA_LITERAL("main") &&
	    ToUpper(BytesToHex(knownTx.Script())) == "010203" &&
	    ToUpper(BytesToHex(knownTx.Payload())) == "7061796C6F6164" &&
	    knownTx.Expiration().Value == 1234567890u &&
	    knownTx.Signatures().size() == 1;
	Report(ctx, knownOk, "Transaction unserialize");

	// The signature count comes from the input. A count above the bytes left reads as no signatures;
	// allocating it would end the program.
	{
		// The known transaction's bytes before its signature count, and its one signature after it.
		const ByteArray unsignedBytes = knownTx.ToByteArray(false);
		const ByteArray signatureBytes(knownBytes.begin() + unsignedBytes.size() + 1, knownBytes.end());
		const auto readWithCount = [&](const ByteArray& count)
		{
			ByteArray bytes = unsignedBytes;
			bytes.insert(bytes.end(), count.begin(), count.end());
			bytes.insert(bytes.end(), signatureBytes.begin(), signatureBytes.end());
			BinaryReader countReader(bytes);
			return Transaction::Unserialize(countReader);
		};
		const Transaction farAbove = readWithCount(HexToBytes("FEFFFFFF7F"));
		Report(ctx, farAbove.Signatures().empty() && ToUpper(BytesToHex(farAbove.Script())) == "010203",
		    "Transaction unserialize reads a signature count above the bytes left as no signatures");
		const Transaction negative = readWithCount(HexToBytes("FFFFFFFFFFFFFFFFFF"));
		Report(ctx, negative.Signatures().empty() && ToUpper(BytesToHex(negative.Script())) == "010203",
		    "Transaction unserialize reads a negative signature count as no signatures");
	}

	TokenEventData tokenEvent("KCAL", BigInteger(42), "main");
	BinaryWriter tokenWriter;
	tokenWriter.WriteSerializable(tokenEvent);
	const TokenEventData tokenEventRoundTrip = Serialization<TokenEventData>::Unserialize(tokenWriter.ToArray());
	const bool tokenEventOk =
	    tokenEventRoundTrip.symbol == PHANTASMA_LITERAL("KCAL") &&
	    tokenEventRoundTrip.value == BigInteger(42) &&
	    tokenEventRoundTrip.chainName == PHANTASMA_LITERAL("main");
	Report(ctx, tokenEventOk, "TokenEventData serialize roundtrip");

	InfusionEventData infusionEvent("HOST", BigInteger(123), "KCAL", BigInteger(7), "main");
	BinaryWriter infusionWriter;
	infusionWriter.WriteSerializable(infusionEvent);
	const InfusionEventData infusionRoundTrip = Serialization<InfusionEventData>::Unserialize(infusionWriter.ToArray());
	const bool infusionOk =
	    infusionRoundTrip.baseSymbol == PHANTASMA_LITERAL("HOST") &&
	    infusionRoundTrip.tokenID == BigInteger(123) &&
	    infusionRoundTrip.infusedSymbol == PHANTASMA_LITERAL("KCAL") &&
	    infusionRoundTrip.infusedValue == BigInteger(7) &&
	    infusionRoundTrip.chainName == PHANTASMA_LITERAL("main");
	Report(ctx, infusionOk, "InfusionEventData serialize roundtrip");
}

} // namespace testcases
