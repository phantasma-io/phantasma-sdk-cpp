#define PHANTASMA_IMPLEMENTATION
#define CURL_STATICLIB
#include "../../../include/Adapters/PhantasmaAPI_rapidjson.h"
#include "../../../include/Adapters/PhantasmaAPI_curl.h"
#include "../../../include/PhantasmaAPI.h"
#include <iostream>

using namespace phantasma;
using namespace phantasma::rpc;

//Sorry, I haven't actually bundled a compiled version of libCurl with the project.
//You have to download/build libCurl yourself!
#pragma comment(lib, "libcurl_a.lib")

int main()
{
	std::string host = "http://localhost:7077";
	CurlClient http(host);
	PhantasmaAPI phantasmaAPI(http);

	//std::string address = "P2f7ZFuj6NfZ76ymNMnG3xRBT5hAMicDrQRHE4S7SoxEr";	//genesis address
	//std::string address = "NztsEZP7dtrzRBagogUYVp6mgEFbhjZfvHMVkd2bYWJfE";	//nft address

	const char* wif = "NztsEZP7dtrzRBagogUYVp6mgEFbhjZfvHMVkd2bYWJfE";

	PhantasmaError error;
	// The account overview costs the same regardless of how much the address holds; balances are
	// fetched separately, one bounded page at a time (the node accepts pageSize 1..100).
	AccountInfo account = phantasmaAPI.GetAccountInfo(wif, &error);

	if( !error.code )
	{
		std::cout << "Balance description for address " << wif << std::endl;
		std::cout << "Name: " << account.name << ", staked: " << account.stake.amount << std::endl;

		String cursor;
		for( ;; )
		{
			const auto page = phantasmaAPI.GetAccountFungibleTokens(wif, "", 0, 100, cursor.c_str(), true, &error);
			if( error.code )
				break;
			for( size_t i = 0; i < page.result.size(); i++ )
			{
				std::cout << page.result[i].amount << " " << page.result[i].symbol << " tokens available on " << page.result[i].chain << " chain" << std::endl;
			}
			if( page.cursor.empty() )
				break;
			cursor = page.cursor;
		}
	}
	else
	{
		std::cout << "Unable to communicate with the RPC node" << std::endl;
	}
}
