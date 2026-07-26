
#define PHANTASMA_IMPLEMENTATION
#include "../../../include/Adapters/PhantasmaAPI_cpprest.h"
#include "../../../include/PhantasmaAPI.h"

using namespace phantasma::rpc;

int main()
{
	std::wstring host = L"http://localhost:7077";
	web::http::client::http_client http(host);
	PhantasmaAPI api(http);

	//std::wstring address = L"P2f7ZFuj6NfZ76ymNMnG3xRBT5hAMicDrQRHE4S7SoxEr";	//genesis address
	//std::wstring address = L"NztsEZP7dtrzRBagogUYVp6mgEFbhjZfvHMVkd2bYWJfE";	//nft address

	const wchar_t* wif = L"NztsEZP7dtrzRBagogUYVp6mgEFbhjZfvHMVkd2bYWJfE";

	try
	{
		// The account overview costs the same regardless of how much the address holds; balances are
		// fetched separately, one bounded page at a time (the node accepts pageSize 1..100).
		AccountInfo account = api.GetAccountInfo(wif);

		std::wcout << L"Balance description for address " << wif << std::endl;
		std::wcout << L"Name: " << account.name << L", staked: " << account.stake.amount << std::endl;

		phantasma::String cursor;
		for( ;; )
		{
			const auto page = api.GetAccountFungibleTokens(wif, L"", 0, 100, cursor.c_str(), true);
			for( size_t i = 0; i < page.result.size(); i++ )
			{
				std::wcout << page.result[i].amount << " " << page.result[i].symbol << " tokens available on " << page.result[i].chain << " chain" << std::endl;
			}
			if( page.cursor.empty() )
				break;
			cursor = page.cursor;
		}
	}
	catch( std::exception& e )
	{
		std::wcout << e.what();
	}
}
