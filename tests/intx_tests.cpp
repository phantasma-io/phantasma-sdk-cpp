#include "test_cases.h"

namespace testcases {
using namespace testutil;

void RunIntXIs8ByteSafeTests(TestContext& ctx)
{
	const intx zero((int64_t)0);
	const intx minI64((int64_t)std::numeric_limits<int64_t>::min());
	const intx maxI64((int64_t)std::numeric_limits<int64_t>::max());
	Report(ctx, zero.Int256().Is8ByteSafe(), "IntX safe zero");
	Report(ctx, minI64.Int256().Is8ByteSafe(), "IntX safe min");
	Report(ctx, maxI64.Int256().Is8ByteSafe(), "IntX safe max");

	const intx tooLarge = intx::FromString("9223372036854775808", 0, 10, nullptr);
	Report(ctx, !tooLarge.Int256().Is8ByteSafe(), "IntX unsafe max+1");
	const intx tooSmall = intx::FromString("-9223372036854775809", 0, 10, nullptr);
	Report(ctx, !tooSmall.Int256().Is8ByteSafe(), "IntX unsafe min-1");

	const intx bigBacked(uint256::FromString("42", 0, 10, nullptr));
	Report(ctx, bigBacked.Int256().Is8ByteSafe(), "IntX safe big-backed");
}

void RunInt256SignGuardByteTests(TestContext& ctx)
{
	// Phantasma/C# signed BigInteger encoding appends a 33rd sign-extension byte when byte 31 already
	// uses the sign bit. Both FromBytes overloads ACCEPT that shape, and it is the only input longer
	// than the 32-byte object they decode into: the 33rd byte is metadata and must not be copied in.
	// These cases pin the accepted shapes and the exact 32-byte payload that must come out of them.
	// The one-byte overrun the bound prevents is a stack write past a local, so it is invisible to a
	// value check alone - run this file under -fsanitize=address to observe that half.
	const auto payloadMatches = [](const void* value, const uint8_t* expected)
	{
		return 0 == memcmp(value, expected, 32);
	};

	uint8_t positiveGuarded[33] = {};
	positiveGuarded[0] = 0x34;
	positiveGuarded[11] = 0x12;
	positiveGuarded[31] = 0x7F;
	positiveGuarded[32] = 0x00;
	const int256 positive = int256::FromBytes(ByteView{ positiveGuarded, sizeof(positiveGuarded) });
	Report(ctx, !positive.IsNegative(), "Int256 33-byte positive guard stays non-negative");
	Report(ctx, payloadMatches(&positive, positiveGuarded), "Int256 33-byte positive guard keeps the low 32 bytes");

	uint8_t negativeGuarded[33] = {};
	negativeGuarded[0] = 0x56;
	negativeGuarded[9] = 0xAB;
	negativeGuarded[31] = 0xE1;
	negativeGuarded[32] = 0xFF;
	const int256 negative = int256::FromBytes(ByteView{ negativeGuarded, sizeof(negativeGuarded) });
	Report(ctx, negative.IsNegative(), "Int256 33-byte negative guard stays negative");
	Report(ctx, payloadMatches(&negative, negativeGuarded), "Int256 33-byte negative guard keeps the low 32 bytes");

	uint8_t unsignedGuarded[33] = {};
	unsignedGuarded[0] = 0x9A;
	unsignedGuarded[5] = 0xBC;
	unsignedGuarded[19] = 0xDE;
	unsignedGuarded[31] = 0xF0;
	unsignedGuarded[32] = 0x00;
	const uint256 unsignedValue = uint256::FromBytes(ByteView{ unsignedGuarded, sizeof(unsignedGuarded) });
	Report(ctx, payloadMatches(&unsignedValue, unsignedGuarded), "UInt256 33-byte zero guard keeps the low 32 bytes");

	// The guard byte carries no value: the same 32 bytes with and without it must decode identically.
	const uint256 withoutGuard = uint256::FromBytes(ByteView{ unsignedGuarded, 32 });
	Report(ctx, 0 == memcmp(&unsignedValue, &withoutGuard, 32), "UInt256 guard byte does not change the value");
}

} // namespace testcases
