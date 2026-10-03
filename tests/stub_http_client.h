#pragma once
#ifdef PHANTASMA_API_INCLUDED
#error "Include this before PhantasmaAPI.h, the way an HTTP adaptor is included"
#endif

// The high-level rpc::PhantasmaAPI class and its methods exist only when an HTTP client type is
// configured, so the tests configure one here the way the curl and cpprest adaptors do: a client
// type, an HttpPost for it, and PHANTASMA_HTTPCLIENT naming it.
//
// The client answers from StubNode::Answer, which script_transaction_expiry_tests.cpp defines: it
// answers a script broadcast. Answering needs the SDK types, and this header is read before they
// exist; a member defined elsewhere is looked up when HttpPost is instantiated, which happens after
// them. A test that needs other answers sets StubNode::answer.

#include <cstring>
#include <functional>
#include <string>
#include <sstream>

#ifndef PHANTASMA_STRING
#define PHANTASMA_STRING std::string
#endif
#ifndef PHANTASMA_STRINGBUILDER
#define PHANTASMA_STRINGBUILDER std::stringstream
#endif
#ifndef PHANTASMA_CHAR
#define PHANTASMA_CHAR char
#endif

namespace phantasma {
namespace rpc {
struct PhantasmaError;
}

// A node that keeps the request it was given and answers the way the real one does.
struct StubNode {
	// The hex of the transaction the last broadcast carried.
	std::string sentTx;
	// Answers every request when it is set. A test that needs other answers than a script broadcast
	// sets it.
	std::function<std::string(const std::string& request)> answer;

	std::string Answer(const std::string& request);
};

// The text between two markers of the JSON-RPC envelope. The stub reads the request that way
// because the tests carry no JSON parser of their own.
inline std::string Between(const std::string& text, const char* open, const char* close)
{
	const size_t start = text.find(open);
	if( start == std::string::npos )
		return {};
	const size_t from = start + strlen(open);
	const size_t end = text.find(close, from);
	if( end == std::string::npos )
		return {};
	return text.substr(from, end - from);
}

template<class Client>
static PHANTASMA_STRING HttpPost(Client& client, const PHANTASMA_CHAR*, const PHANTASMA_STRINGBUILDER& data, rpc::PhantasmaError*)
{
	return client.Answer(data.str());
}

} // namespace phantasma

#define PHANTASMA_HTTPCLIENT StubNode
