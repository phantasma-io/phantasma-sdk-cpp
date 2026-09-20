#pragma once
#ifdef PHANTASMA_API_INCLUDED
#error "Include this before PhantasmaAPI.h, the way an HTTP adaptor is included"
#endif

// The high-level rpc::PhantasmaAPI class and its methods exist only when an HTTP client type is
// configured, so the tests configure one here the way the curl and cpprest adaptors do: a client
// type, an HttpPost for it, and PHANTASMA_HTTPCLIENT naming it.
//
// The client answers from StubNode::Answer, which is defined in the test that uses it. Answering
// needs the SDK types, and this header is read before they exist; a member defined elsewhere is
// looked up when HttpPost is instantiated, which happens after them.

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

	std::string Answer(const std::string& request);
};

template<class Client>
static PHANTASMA_STRING HttpPost(Client& client, const PHANTASMA_CHAR*, const PHANTASMA_STRINGBUILDER& data, rpc::PhantasmaError*)
{
	return client.Answer(data.str());
}

} // namespace phantasma

#define PHANTASMA_HTTPCLIENT StubNode
