------------------------------------------------------------------------------
 Low-level API
------------------------------------------------------------------------------
  The PhantasmaJsonAPI namespace can construct JSON requests and parse JSON responses,
   but you are responsible for sending/receiving these messages via HTTP on your own.
   You can call `PhantasmaJsonAPI::Uri()` to determine where to send them.

     void PhantasmaJsonAPI::MakeGetAccountInfoRequest(JSONBuilder, account);
     bool PhantasmaJsonAPI::ParseGetAccountInfoResponse(JSONValue, AccountInfo);
     void PhantasmaJsonAPI::MakeGetAccountRequest(JSONBuilder, addressText);
     bool PhantasmaJsonAPI::ParseGetAccountResponse(JSONValue, Account);
     void PhantasmaJsonAPI::MakeGetAccountsRequest(JSONBuilder, accountText, extended, checkAddressReservedByte);
     bool PhantasmaJsonAPI::ParseGetAccountsResponse(JSONValue, vector<Account>);
     void PhantasmaJsonAPI::MakeLookUpNameRequest(JSONBuilder, name);
     bool PhantasmaJsonAPI::ParseLookUpNameResponse(JSONValue, String);
     void PhantasmaJsonAPI::MakeGetBlockHeightRequest(JSONBuilder, chainInput);
     bool PhantasmaJsonAPI::ParseGetBlockHeightResponse(JSONValue, Int32);
     void PhantasmaJsonAPI::MakeGetBlockTransactionCountByHashRequest(JSONBuilder, chainAddressOrName, blockHash);
     bool PhantasmaJsonAPI::ParseGetBlockTransactionCountByHashResponse(JSONValue, Int32);
     void PhantasmaJsonAPI::MakeGetBlockByHashRequest(JSONBuilder, blockHash);
     bool PhantasmaJsonAPI::ParseGetBlockByHashResponse(JSONValue, Block);
     void PhantasmaJsonAPI::MakeGetBlockByHeightRequest(JSONBuilder, chainInput, height);
     bool PhantasmaJsonAPI::ParseGetBlockByHeightResponse(JSONValue, Block);
     void PhantasmaJsonAPI::MakeGetLatestBlockRequest(JSONBuilder, chainInput);
     bool PhantasmaJsonAPI::ParseGetLatestBlockResponse(JSONValue, Block);
     void PhantasmaJsonAPI::MakeGetTransactionByBlockHashAndIndexRequest(JSONBuilder, chainAddressOrName, blockHash, index);
     bool PhantasmaJsonAPI::ParseGetTransactionByBlockHashAndIndexResponse(JSONValue, Transaction);
     void PhantasmaJsonAPI::MakeGetAddressTransactionsRequest(JSONBuilder, addressText, page, pageSize);
     bool PhantasmaJsonAPI::ParseGetAddressTransactionsResponse(JSONValue, AccountTransactions);
     void PhantasmaJsonAPI::MakeGetAddressTransactionCountRequest(JSONBuilder, addressText, chainInput);
     bool PhantasmaJsonAPI::ParseGetAddressTransactionCountResponse(JSONValue, Int32);
     void PhantasmaJsonAPI::MakeSendRawTransactionRequest(JSONBuilder, txData);
     bool PhantasmaJsonAPI::ParseSendRawTransactionResponse(JSONValue, String);
     void PhantasmaJsonAPI::MakeSendCarbonTransactionRequest(JSONBuilder, txData);
     bool PhantasmaJsonAPI::ParseSendCarbonTransactionResponse(JSONValue, String);
     void PhantasmaJsonAPI::MakeInvokeRawScriptRequest(JSONBuilder, chainInput, scriptData);
     bool PhantasmaJsonAPI::ParseInvokeRawScriptResponse(JSONValue, Script);
     void PhantasmaJsonAPI::MakeGetTransactionRequest(JSONBuilder, hashText);
     bool PhantasmaJsonAPI::ParseGetTransactionResponse(JSONValue, Transaction);
     void PhantasmaJsonAPI::MakeGetChainsRequest(JSONBuilder);
     bool PhantasmaJsonAPI::ParseGetChainsResponse(JSONValue, vector<Chain>);
     void PhantasmaJsonAPI::MakeGetChainRequest(JSONBuilder, name, extended);
     bool PhantasmaJsonAPI::ParseGetChainResponse(JSONValue, Chain);
     void PhantasmaJsonAPI::MakeGetNexusRequest(JSONBuilder, extended);
     bool PhantasmaJsonAPI::ParseGetNexusResponse(JSONValue, Nexus);
     void PhantasmaJsonAPI::MakeGetOrganizationRequest(JSONBuilder, name, includeMemberCount);
     bool PhantasmaJsonAPI::ParseGetOrganizationResponse(JSONValue, Organization);
     void PhantasmaJsonAPI::MakeGetOrganizationsRequest(JSONBuilder, pageSize, cursor, includeMemberCount);
     bool PhantasmaJsonAPI::ParseGetOrganizationsResponse(JSONValue, CursorPaginatedResult<Organization>);
     void PhantasmaJsonAPI::MakeGetOrganizationMembersRequest(JSONBuilder, name, pageSize, cursor, includeMemberTime);
     bool PhantasmaJsonAPI::ParseGetOrganizationMembersResponse(JSONValue, CursorPaginatedResult<OrganizationMember>);
     void PhantasmaJsonAPI::MakeGetOrganizationMemberRequest(JSONBuilder, name, address, checkAddressReservedByte, addressType);
     bool PhantasmaJsonAPI::ParseGetOrganizationMemberResponse(JSONValue, OrganizationMember);
     void PhantasmaJsonAPI::MakeGetLeaderboardRequest(JSONBuilder, name);
     bool PhantasmaJsonAPI::ParseGetLeaderboardResponse(JSONValue, Leaderboard);
     void PhantasmaJsonAPI::MakeGetTokensRequest(JSONBuilder, extended, ownerAddress);
     bool PhantasmaJsonAPI::ParseGetTokensResponse(JSONValue, vector<Token>);
     void PhantasmaJsonAPI::MakeGetTokenRequest(JSONBuilder, symbol, extended, carbonTokenId);
     bool PhantasmaJsonAPI::ParseGetTokenResponse(JSONValue, Token);
     void PhantasmaJsonAPI::MakeGetTokenSeriesRequest(JSONBuilder, symbol, carbonTokenId, pageSize, cursor);
     bool PhantasmaJsonAPI::ParseGetTokenSeriesResponse(JSONValue, CursorPaginatedResult<TokenSeries>);
     void PhantasmaJsonAPI::MakeGetTokenSeriesByIdRequest(JSONBuilder, symbol, carbonTokenId, seriesId, carbonSeriesId);
     bool PhantasmaJsonAPI::ParseGetTokenSeriesByIdResponse(JSONValue, TokenSeries);
     void PhantasmaJsonAPI::MakeGetTokenNFTsRequest(JSONBuilder, carbonTokenId, carbonSeriesId, pageSize, cursor, extended);
     bool PhantasmaJsonAPI::ParseGetTokenNFTsResponse(JSONValue, CursorPaginatedResult<TokenData>);
     void PhantasmaJsonAPI::MakeGetTokenDataRequest(JSONBuilder, symbol, IDtext);
     bool PhantasmaJsonAPI::ParseGetTokenDataResponse(JSONValue, TokenData);
     void PhantasmaJsonAPI::MakeGetNFTRequest(JSONBuilder, symbol, IDtext, extended);
     bool PhantasmaJsonAPI::ParseGetNFTResponse(JSONValue, TokenData);
     void PhantasmaJsonAPI::MakeGetNFTsRequest(JSONBuilder, symbol, IDtext, extended);
     bool PhantasmaJsonAPI::ParseGetNFTsResponse(JSONValue, vector<TokenData>);
     void PhantasmaJsonAPI::MakeGetTokenBalanceRequest(JSONBuilder, addressText, tokenSymbol, chainInput);
     void PhantasmaJsonAPI::MakeGetTokenBalanceRequest(JSONBuilder, addressText, tokenSymbol, chainInput, checkAddressReservedByte, addressType);
     bool PhantasmaJsonAPI::ParseGetTokenBalanceResponse(JSONValue, Balance);
     void PhantasmaJsonAPI::MakeGetAccountFungibleTokensRequest(JSONBuilder, account, tokenSymbol, carbonTokenId, pageSize, cursor, checkAddressReservedByte);
     void PhantasmaJsonAPI::MakeGetAccountFungibleTokensRequest(JSONBuilder, account, tokenSymbol, carbonTokenId, pageSize, cursor, checkAddressReservedByte, addressType);
     bool PhantasmaJsonAPI::ParseGetAccountFungibleTokensResponse(JSONValue, CursorPaginatedResult<Balance>);
     void PhantasmaJsonAPI::MakeGetAccountNFTsRequest(JSONBuilder, account, tokenSymbol, carbonTokenId, carbonSeriesId, pageSize, cursor, extended, checkAddressReservedByte);
     void PhantasmaJsonAPI::MakeGetAccountNFTsRequest(JSONBuilder, account, tokenSymbol, carbonTokenId, carbonSeriesId, pageSize, cursor, extended, checkAddressReservedByte, addressType);
     bool PhantasmaJsonAPI::ParseGetAccountNFTsResponse(JSONValue, CursorPaginatedResult<TokenData>);
     void PhantasmaJsonAPI::MakeGetAccountOwnedTokensRequest(JSONBuilder, account, tokenSymbol, carbonTokenId, pageSize, cursor, checkAddressReservedByte);
     void PhantasmaJsonAPI::MakeGetAccountOwnedTokensRequest(JSONBuilder, account, tokenSymbol, carbonTokenId, pageSize, cursor, checkAddressReservedByte, addressType);
     bool PhantasmaJsonAPI::ParseGetAccountOwnedTokensResponse(JSONValue, CursorPaginatedResult<Token>);
     void PhantasmaJsonAPI::MakeGetAccountOwnedTokenSeriesRequest(JSONBuilder, account, tokenSymbol, carbonTokenId, pageSize, cursor, checkAddressReservedByte);
     void PhantasmaJsonAPI::MakeGetAccountOwnedTokenSeriesRequest(JSONBuilder, account, tokenSymbol, carbonTokenId, pageSize, cursor, checkAddressReservedByte, addressType);
     bool PhantasmaJsonAPI::ParseGetAccountOwnedTokenSeriesResponse(JSONValue, CursorPaginatedResult<TokenSeries>);
     void PhantasmaJsonAPI::MakeGetAuctionsCountRequest(JSONBuilder, chainAddressOrName, symbol);
     bool PhantasmaJsonAPI::ParseGetAuctionsCountResponse(JSONValue, Int32);
     void PhantasmaJsonAPI::MakeGetAuctionsRequest(JSONBuilder, chainAddressOrName, symbol, page, pageSize);
     bool PhantasmaJsonAPI::ParseGetAuctionsResponse(JSONValue, vector<Auction>);
     void PhantasmaJsonAPI::MakeGetAuctionRequest(JSONBuilder, chainAddressOrName, symbol, IDtext);
     bool PhantasmaJsonAPI::ParseGetAuctionResponse(JSONValue, Auction);
     void PhantasmaJsonAPI::MakeGetArchiveRequest(JSONBuilder, hashText);
     bool PhantasmaJsonAPI::ParseGetArchiveResponse(JSONValue, Archive);
     void PhantasmaJsonAPI::MakeWriteArchiveRequest(JSONBuilder, hashText, blockIndex, blockContent);
     bool PhantasmaJsonAPI::ParseWriteArchiveResponse(JSONValue, bool);
     void PhantasmaJsonAPI::MakeReadArchiveRequest(JSONBuilder, hashText, blockIndex);
     bool PhantasmaJsonAPI::ParseReadArchiveResponse(JSONValue, String);
     void PhantasmaJsonAPI::MakeGetContractRequest(JSONBuilder, chainAddressOrName, contractName);
     bool PhantasmaJsonAPI::ParseGetContractResponse(JSONValue, Contract);
     void PhantasmaJsonAPI::MakeGetContractsRequest(JSONBuilder, chainAddressOrName, extended);
     bool PhantasmaJsonAPI::ParseGetContractsResponse(JSONValue, vector<Contract>);
     void PhantasmaJsonAPI::MakeGetContractByAddressRequest(JSONBuilder, chainAddressOrName, contractAddress);
     bool PhantasmaJsonAPI::ParseGetContractByAddressResponse(JSONValue, Contract);
     void PhantasmaJsonAPI::MakeGetVersionRequest(JSONBuilder);
     bool PhantasmaJsonAPI::ParseGetVersionResponse(JSONValue, BuildInfoResult);
     void PhantasmaJsonAPI::MakeGetPhantasmaVmConfigRequest(JSONBuilder, chainAddressOrName);
     bool PhantasmaJsonAPI::ParseGetPhantasmaVmConfigResponse(JSONValue, PhantasmaVmConfig);

------------------------------------------------------------------------------
 VM values and typed extended events
------------------------------------------------------------------------------
  The RPC models live in `include/Rpc/`, which `PhantasmaAPI.h` includes for you:

     Rpc/Prelude.h                            core typedefs and the JSON contract
     Rpc/VmValue.h                            VM values: scalar, array or struct
     Rpc/SpecialResolutionArgumentsBase.h     the argument tag, fallbacks and readers
     Rpc/SpecialResolutionArguments*.h        the 36 argument shapes, one file per module
     Rpc/SpecialResolutionArguments.h         module+method to shape dispatch
     Rpc/ExtendedEvents.h                     event payloads and the event envelope

  `TokenProperty::value` is a `VmValue`, so token metadata, series metadata,
   organization metadata and NFT properties keep the shape the chain stores:

     for( const TokenProperty& row : token.metadata )
     {
         if( row.value.IsText() )
             use(row.key, row.value.Text());
         else if( row.value.IsItems() && row.value.ItemCount() > 0 )
             use(row.key, row.value.Item(0)->Field("mul"));
     }

  Inside a special resolution, every call's arguments are typed by the call's
   module and method:

     if( const TransferFungibleArguments* transfer =
             SpecialResolutionArgumentsAs<TransferFungibleArguments>(call.arguments.get()) )
         use(transfer->token, transfer->amount, transfer->from);

  Decoding is total. A call method or an event kind this build does not model, and
   a modeled one whose payload does not match, keep the JSON they arrived with -
   in `UnrecognizedArguments::json` and `EventExtended::unknownData` - so a node
   newer than the SDK never costs you the data it answered, and never fails the
   block that carries it.

------------------------------------------------------------------------------
 High-level API
------------------------------------------------------------------------------
  If you have defined `PHANTASMA_HTTPCLIENT`, then you can construct a 
   PhantasmaAPI object, which provides a simplified API that hides the 
   internal JSON messaging.

     PhantasmaAPI phantasmaAPI(httpClient);
     AccountInfo = phantasmaAPI.GetAccountInfo(account, error);
     Account = phantasmaAPI.GetAccount(addressText, error);
     vector<Account> = phantasmaAPI.GetAccounts(accountText, extended, checkAddressReservedByte, error);
     String = phantasmaAPI.LookUpName(name, error);
     Int32 = phantasmaAPI.GetBlockHeight(chainInput, error);
     Int32 = phantasmaAPI.GetBlockTransactionCountByHash(chainAddressOrName, blockHash, error);
     Block = phantasmaAPI.GetBlockByHash(blockHash, error);
     Block = phantasmaAPI.GetBlockByHeight(chainInput, height, error);
     Block = phantasmaAPI.GetLatestBlock(chainInput, error);
     Transaction = phantasmaAPI.GetTransactionByBlockHashAndIndex(chainAddressOrName, blockHash, index, error);
     AccountTransactions = phantasmaAPI.GetAddressTransactions(addressText, page, pageSize, error);
     Int32 = phantasmaAPI.GetAddressTransactionCount(addressText, chainInput, error);
     String = phantasmaAPI.SendRawTransaction(txData, error);
     String = phantasmaAPI.SendCarbonTransaction(txData, error);
     Script = phantasmaAPI.InvokeRawScript(chainInput, scriptData, error);
     Transaction = phantasmaAPI.GetTransaction(hashText, error);
     vector<Chain> = phantasmaAPI.GetChains(error);
     Chain = phantasmaAPI.GetChain(name, extended, error);
     Nexus = phantasmaAPI.GetNexus(extended, error);
     Organization = phantasmaAPI.GetOrganization(name, includeMemberCount, error);
     CursorPaginatedResult<Organization> = phantasmaAPI.GetOrganizations(pageSize, cursor, includeMemberCount, error);
     CursorPaginatedResult<OrganizationMember> = phantasmaAPI.GetOrganizationMembers(name, pageSize, cursor, includeMemberTime, error);
     OrganizationMember = phantasmaAPI.GetOrganizationMember(name, address, checkAddressReservedByte, addressType, error);
     Leaderboard = phantasmaAPI.GetLeaderboard(name, error);
     vector<Token> = phantasmaAPI.GetTokens(extended, error);
     vector<Token> = phantasmaAPI.GetTokens(extended, ownerAddress, error);
     Token = phantasmaAPI.GetToken(symbol, extended, error);
     Token = phantasmaAPI.GetToken(symbol, extended, carbonTokenId, error);
     CursorPaginatedResult<TokenSeries> = phantasmaAPI.GetTokenSeries(symbol, carbonTokenId, pageSize, cursor, error);
     TokenSeries = phantasmaAPI.GetTokenSeriesById(symbol, carbonTokenId, seriesId, carbonSeriesId, error);
     CursorPaginatedResult<TokenData> = phantasmaAPI.GetTokenNFTs(carbonTokenId, carbonSeriesId, pageSize, cursor, extended, error);
     TokenData = phantasmaAPI.GetTokenData(symbol, IDtext, error);
     TokenData = phantasmaAPI.GetNFT(symbol, IDtext, extended, error);
     vector<TokenData> = phantasmaAPI.GetNFTs(symbol, IDtext, extended, error);
     Balance = phantasmaAPI.GetTokenBalance(addressText, tokenSymbol, chainInput, error);
     Balance = phantasmaAPI.GetTokenBalance(addressText, tokenSymbol, chainInput, checkAddressReservedByte, addressType, error);
     CursorPaginatedResult<Balance> = phantasmaAPI.GetAccountFungibleTokens(account, tokenSymbol, carbonTokenId, pageSize, cursor, checkAddressReservedByte, error);
     CursorPaginatedResult<Balance> = phantasmaAPI.GetAccountFungibleTokens(account, tokenSymbol, carbonTokenId, pageSize, cursor, checkAddressReservedByte, addressType, error);
     CursorPaginatedResult<TokenData> = phantasmaAPI.GetAccountNFTs(account, tokenSymbol, carbonTokenId, carbonSeriesId, pageSize, cursor, extended, checkAddressReservedByte, error);
     CursorPaginatedResult<TokenData> = phantasmaAPI.GetAccountNFTs(account, tokenSymbol, carbonTokenId, carbonSeriesId, pageSize, cursor, extended, checkAddressReservedByte, addressType, error);
     CursorPaginatedResult<Token> = phantasmaAPI.GetAccountOwnedTokens(account, tokenSymbol, carbonTokenId, pageSize, cursor, checkAddressReservedByte, error);
     CursorPaginatedResult<Token> = phantasmaAPI.GetAccountOwnedTokens(account, tokenSymbol, carbonTokenId, pageSize, cursor, checkAddressReservedByte, addressType, error);
     CursorPaginatedResult<TokenSeries> = phantasmaAPI.GetAccountOwnedTokenSeries(account, tokenSymbol, carbonTokenId, pageSize, cursor, checkAddressReservedByte, error);
     CursorPaginatedResult<TokenSeries> = phantasmaAPI.GetAccountOwnedTokenSeries(account, tokenSymbol, carbonTokenId, pageSize, cursor, checkAddressReservedByte, addressType, error);
     Int32 = phantasmaAPI.GetAuctionsCount(chainAddressOrName, symbol, error);
     vector<Auction> = phantasmaAPI.GetAuctions(chainAddressOrName, symbol, page, pageSize, error);
     Auction = phantasmaAPI.GetAuction(chainAddressOrName, symbol, IDtext, error);
     Archive = phantasmaAPI.GetArchive(hashText, error);
     bool = phantasmaAPI.WriteArchive(hashText, blockIndex, blockContent, error);
     String = phantasmaAPI.ReadArchive(hashText, blockIndex, error);
     Contract = phantasmaAPI.GetContract(chainAddressOrName, contractName, error);
     vector<Contract> = phantasmaAPI.GetContracts(chainAddressOrName, extended, error);
     Contract = phantasmaAPI.GetContractByAddress(chainAddressOrName, contractAddress, error);
     BuildInfoResult = phantasmaAPI.GetVersion(error);
     PhantasmaVmConfig = phantasmaAPI.GetPhantasmaVmConfig(chainAddressOrName, error);

------------------------------------------------------------------------------
 Carbon transaction fees
------------------------------------------------------------------------------
 A Carbon transaction is built, planned, signed and sent as four separate steps.
  The message is the same whether the key is in memory or in a hardware wallet.

     #include "Carbon/FeePlan.h"
     #include "Carbon/FeePlanSummary.h"
     using namespace phantasma::carbon;

     // 1. Build the message. A builder carries no prices: maxGas stays 0 until the
     //    fee is planned.
     TxEnvelope env = CreateTokenTxHelper::BuildTx(tokenInfo, creatorPublicKey);

 The transfers, mints and burns that need no VM script are built by `NativeTxHelper`:

     Blockchain::TxMsg msg = NativeTxHelper::TransferFungible({ owner }, recipient, tokenId, 100);
     // Naming a gas payer selects the two-signature form of the same operation:
     Blockchain::TxMsg paid = NativeTxHelper::TransferFungible({ owner, &gasPayer }, recipient, tokenId, 100);


     // 2. Read the chain's prices once, then plan this message against them.
     const Blockchain::GasConfig config = ToGasConfig(api.GetGasConfig().gasConfig);
     FeePlanOptions options;
     options.witnessCount = 1;      // a Call chooses its own witnesses
     options.infusionsRead = true;  // nothing is burned here
     FeePlan plan;
     if( !PlanFees(env.msg, config, options, plan) )
         return; // the message cannot be priced; the planner has reported why

     const FeePlanSummary summary = SummarizeFeePlan(plan);
     // summary.gasBill "0.00426", summary.gasOffer "0.00426", summary.storageCeiling "0"

     // 3-4. Sign and send the planned message.
     env.msg = plan.Apply(env.msg);
     const ByteArray signedTx = SignAndSerialize(env, keys);

 A message that takes more than one signature is signed through the signer. A
  `_GasPayer` message is signed twice, the gas payer first; a call carries as many
  witnesses as it was planned for, and the gas payer has to be one of them.

     ByteArray signedTx;
     std::string error;
     if( !Blockchain::TxMsgSigner::SignAndSerialize(env.msg, { &gasPayerKeys, &ownerKeys }, signedTx, error) )
         return; // the signer list does not match what this message names

 Before a token creation is signed, ask the chain whether the symbol is free:

     #include "Carbon/Preflight.h"

     const PreflightResult check = PreflightTransaction(api, env.msg, gasTokenId);
     if( check.verdict == PreflightVerdict::Taken )
         return; // sending this would pay the policy fee and get nothing back

 `Token.CreateToken` is charged its policy fee before the contract looks at the
  symbol, and that fee is the largest single price in the protocol. The check costs
  one query and no fee. Every message that is not a token creation answers
  `NotApplicable`.

 Under gas model v2 there are two things to know.

 1. Plan before you sign. The chain bills every byte the transaction puts in the
    block, and it escrows every storage row the transaction creates. A message
    therefore carries a gas offer and a storage ceiling, and both have to be set
    before signing. A message signed with a zero offer is never admitted.
 2. What the plan cannot know, it assumes expensive. Some of the price depends on
    chain state the message does not carry. Examples are whether the recipient
    already holds the token, and which mode a series mints in. Each fact is taken
    at the value that costs MORE. Unused gas is refunded, and a short offer is
    rejected. `plan.exact` says whether such an assumption decided this number.

 - `PlanFees` prices a message from the message itself. It touches no network.
   The signed size is computed without a key. The storage rows, call result bytes
   and gas sites of each native operation are priced with the chain's own formula.
 - `plan.kinds` says which operations were priced. A `Call_Multi` performs several,
   and each one is priced and then summed, because the chain bills a batch as the
   sum of its calls with the envelope counted once. One kind is a budget and not a
   formula: `NativeFeeKind::Script`, which covers VM scripts and unmodelled calls.
   Their work depends on execution.
 - `FeePlanOptions` is where you tighten point 2 by telling the planner a fact it
   would otherwise assume. Every field has a default and most callers change none.
 - `plan.exact` says whether the number is a prediction or a ceiling. It is true
   when nothing the plan had to assume could have changed it. It is false when an
   assumed fact decided part of the price, or when a part of the message had to be
   budgeted. A wallet shows the amount when the flag is true, and "up to" in front
   of the amount when it is false. The flag is answered by pricing the message a
   second time with every fact at its cheaper reading, so it is about THIS message
   and not about which fields you filled in. A KCAL transfer is exact without
   stating anything, because the chain's own token rows are free.
 - `EstimateNativeFee` and `EstimateNativeFeeBatch` are the calculator underneath.
   Call them directly when you hold sizes rather than a message.

 Which fact each operation reads. Only the facts an operation reads can move its
  price, so this table is the whole of what is worth stating. Every default is the
  reading that costs MORE. One storage quantum is `dataEscrowPerRow` of escrow plus
  25 gas units of block data, and the chain's `feeMultiplier` scales both. Balance
  rows of the gas and data tokens are free, so for those tokens
  `recipientHoldsToken` changes nothing.

  |`NativeFeeKind`           | facts it reads                                          | what the default assumes                                                                                         | what the default costs                                                     |
  |--------------------------|---------------------------------------------------------|------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------|
  |`TransferFungible`        | `recipientHoldsToken`                                   | the recipient has no row for this token                                                                          | 1 quantum                                                                  |
  |`TransferNonFungible`     | `recipientHoldsToken`                                   | the recipient has no row for this token                                                                          | 1 quantum                                                                  |
  |`MintFungible`            | `recipientHoldsToken`, `supplyRowExists`, `bigFungible` | no recipient row; the supply row was dropped and must be recreated; the resulting balance needs the widest answer | 1 quantum each, and 24 more result bytes (33 against 9)                    |
  |`BurnFungible`            | `tokenBurnedBefore`, `supplyRowExists`, `bigFungible`   | the token's burnt counter does not exist yet; the supply row must be recreated; widest answer                     | 1 quantum each, and 24 more result bytes                                   |
  |`MintNonFungible`         | `recipientHoldsToken`, `supplyRowExists`, `romHasMetaId`| as above, and the ROM carries an `_i` id, which is indexed in one more row                                        | 1 quantum each, and 1 quantum per instance for the id                      |
  |`MintPhantasmaNonFungible`| `recipientHoldsToken`, `supplyRowExists`, `duplicatedSeries` | as above, and the series mints duplicates                                                                    | 1 quantum each, and one query fee per instance plus one per distinct series|
  |`BurnNonFungible`         | `tokenBurnedBefore`, `supplyRowExists`, `infusions`     | burnt counter does not exist; supply row must be recreated                                                       | 1 quantum each. `infusions` has no default at all, see below               |
  |`CreateToken`             | none                                                    | nothing is assumed                                                                                               | the price comes from the message: the symbol length, the serialized `TokenInfo`, and which keys its metadata carries|
  |`CreateTokenSeries`       | `seriesHasMetaId`                                       | the series metadata carries an `_i` id                                                                           | 1 quantum                                                                  |
  |`RegisterName`            | none                                                    | nothing is assumed                                                                                               | governance rows are free data; the price is the length-shifted policy fee and the envelope|
  |`Script`                  | none                                                    | 5000 work units, 512 event bytes and 4 storage quanta, per unmodelled call                                       | a budget and never a prediction; `plan.exact` is false whenever one is present|

 - A burn returns whatever the NFT holds at its own address, and the chain charges
   for each returned asset: a transfer fee and an owner lookup per fungible token,
   an instance query, a transfer per instance and that lookup per NFT token, plus a
   balance row for a returned token the burner does not hold. That set is chain
   state with no costlier bound, so `PlanFees` demands it. Fill
   `FeePlanOptions::infusions` and set `infusionsRead`; an empty list states that
   the instances hold nothing. `ReadInfusedAssets` in `Carbon/FeeInfusions.h` fills
   that list from the chain for a whole message, or for one instance:

        #include "Carbon/FeeInfusions.h"

        PHANTASMA_VECTOR<InfusedAsset> infusions;
        if( !ReadInfusedAssets(api, env.msg, infusions) )
            return; // a query failed; the plan would be a guess
        options.infusions = infusions.empty() ? nullptr : &infusions.front();
        options.numInfusions = (uint32_t)infusions.size();
        options.infusionsRead = true;

   It asks the account queries about the NFT's own address, which is
   `TokenHelper::GetNftAddress(tokenId, instanceId)`, with `AddressType::Carbon`.
   `BurnedInstances` names the instances a message burns if you would rather read
   them one at a time.

   `AddressType` is what every account query takes to say how the node should read
   the address text: `Phantasma` for the base58 form, `Carbon` for a 64-hex account
   key. The node reads it as a word, and the SDK names the two it accepts, so a
   misspelling is a compile error and not a request the node refuses.
 - A contract call (`Call`, `Call_Multi`, `Trade`, `Phantasma`) chooses its own
   witnesses, and each one is 96 bytes the chain bills. So `PlanFees` asks for
   `witnessCount`. An assumed single witness would under-offer every multi-party
   transaction by 96 bytes per extra signature. The gas payer must be one of the
   witnesses, because the chain rejects a transaction its payer did not sign.
 - `Token.CreateToken` is charged its policy fee BEFORE the contract looks at the
   symbol, and that fee is the largest single price in the protocol. Sending a
   symbol that is already taken pays the fee and gets nothing back. Look the symbol
   up before sending.
 - `FormatTokenAmount` renders an atom count as a decimal string and
   `ParseTokenAmount` reads one back. KCAL has 10 decimals and SOUL has 8. The
   reader refuses an amount with more fractional digits than the token holds,
   because those digits would be dropped and the caller would send something it
   never wrote. `SummarizeFeePlan` renders a whole plan for display.

------------------------------------------------------------------------------
 Reading a Carbon transaction back
------------------------------------------------------------------------------
 `ParseTx` and `ParseSignedTx` read a message the way the chain reads it. Use them
  to show an incoming transaction, to check what a signer is about to sign, or to
  plan a fee for a message somebody else built.

     #include "Carbon/DataBlockchain.h"
     using namespace phantasma::carbon;

     const ByteView envelope{ bytes.data(), bytes.size() };

     Allocator storage;
     Blockchain::SignedTxMsg parsed;
     ByteView signedPortion;
     if( !Blockchain::ParseSignedTx(parsed, signedPortion, envelope, storage) )
         return; // truncated, or a transaction type this SDK does not know

     // parsed.msg is the message, parsed.witnesses are its signers, and
     // signedPortion is the part of the envelope each signature is made over.

 Two lifetimes matter. The message keeps views into `storage`, so the allocator has
  to outlive it. `signedPortion` points into the bytes you passed in, so those have
  to outlive it too.

------------------------------------------------------------------------------
 API configuration
------------------------------------------------------------------------------
 As different C++ projects may use different primitive types, you can use the 
  following #defines (BEFORE including `phantasma.h`) to override the default types.

 |#define                  | typedef                   | Default             | Notes                                                  |
 |-------------------------|---------------------------|---------------------|--------------------------------------------------------|
 |`PHANTASMA_BYTE`         | `phantasma::Byte`         | `uint8_t`           |                                                        |
 |`PHANTASMA_INT32`        | `phantasma::Int32`        | `int32_t`           |                                                        |
 |`PHANTASMA_UINT32`       | `phantasma::UInt32`       | `uint32_t`          |                                                        |
 |`PHANTASMA_INT64`        | `phantasma::Int64`        | `int64_t`           |                                                        |
 |`PHANTASMA_UINT64`       | `phantasma::UInt64`       | `uint64_t`          |                                                        |
 |`PHANTASMA_CHAR`         | `phantasma::Char`         | `char`              | See Unicode section                                    |
 |`PHANTASMA_STRING`       | `phantasma::String`       | `std::string`       | Must support construction from `const phantasma::Char*`|
 |`PHANTASMA_STRINGBUILDER`| `phantasma::StringBuilder`| `std::stringstream` |                                                        |
 |`PHANTASMA_VECTOR`       |                           | `std::vector`       | Must support `push_back` and `size` members            |
 |`PHANTASMA_JSONVALUE`    | `phantasma::JSONValue`    | `std::string_view`  | See JSON and Adaptors section                          |
 |`PHANTASMA_JSONARRAY`    | `phantasma::JSONArray`    | `JSONValue`         | See JSON and Adaptors section                          |
 |`PHANTASMA_JSONDOCUMENT` | `phantasma::JSONDocument` | `std::string`       | See JSON and Adaptors section                          |
 |`PHANTASMA_JSONBUILDER`  | `phantasma::JSONBuilder`  | `std::stringstream`*| See JSON and Adaptors section                          |
 |`PHANTASMA_HTTPCLIENT`   | `phantasma::HttpClient`   |                     | See HTTP and Adaptors section                          |

 The behavior of this header can further be modified by using the following 
  `#defines` (BEFORE including `phantasma.h`)
 
 |#define                                        | Notes                   |
 |-----------------------------------------------|-------------------------|
 |`PHANTASMA_EXCEPTION(message)`                 | See Exceptions section  |
 |`PHANTASMA_EXCEPTION_MESSAGE(message, String)` | See Exceptions section  |
 |`PHANTASMA_LITERAL(x)`                         | See Unicode section     |
 |`PHANTASMA_FUNCTION`                           | See Integration section |        
 |`PHANTASMA_IMPLEMENTATION`                     | See Integration section |

------------------------------------------------------------------------------
 Integration
------------------------------------------------------------------------------
 The core of API is provided in the "single header" style to support simple and 
  flexible integration into your project 
  (see https://github.com/nothings/single_file_libs / https://en.wikipedia.org/wiki/Header-only).
 The implementation of function bodies will be excluded unless you define
  `PHANTASMA_IMPLEMENTATION` before including `phantasma.h`.

 See the "Extended/Advanced usage" section, below for details on what is excluded
  from this single header file.

 Typical linking:
  In one CPP file, before including `phantasma.h`:

   `#define PHANTASMA_IMPLEMENTATION`
 
 Inline linking:
  In every CPP file that uses the API, before including `phantasma.h`:

   `#define PHANTASMA_IMPLEMENTATION`

   `#define PHANTASMA_FUNCTION inline`

 Aside from `PHANTASMA_IMPLEMENTATION` / `PHANTASMA_FUNCTION`, you should take 
  care to ensure that every other PHANTASMA_* macro is defined to the same value
  in all of your CPP files that use the phantasma API.

------------------------------------------------------------------------------
 Exceptions
------------------------------------------------------------------------------
 Support for C++ exceptions is opt-in. Before including `phantasma.h`, define
  the following to enable exceptions:

 `#define PHANTASMA_EXCEPTION_ENABLE`

 Alternatively, you can customize the exact type that is thrown by defining:

 `#define PHANTASMA_EXCEPTION(message)                 throw std::runtime_error(message)`

 `#define PHANTASMA_EXCEPTION_MESSAGE(message, string) throw std::runtime_error(string)`

------------------------------------------------------------------------------
 Unicode
------------------------------------------------------------------------------
 To build a wide-character version of the API, define the following before
  including `phantasma.h`:

 `#define PHANTASMA_CHAR          wchar_t`

 `#define PHANTASMA_LITERAL(x)    L ## x`

 `#define PHANTASMA_STRING        std::wstring`

 `#define PHANTASMA_STRINGBUILDER std::wstringstream`

 Alternatively, if `_UNICODE` is defined, then the above macros will be defined
  automatically.

 You should also provide a JSON and HTTP library with wide-character support.

------------------------------------------------------------------------------
 Adaptors
------------------------------------------------------------------------------
 Parts of the Phantasma SDK are designed to plug into external features, such
  as HTTP communications, JSON encoding and advanced cryptography.
 You can configure the SDK to connect to your own implemenations, or existing
  libraries.
 To make integration easier, we provide several "adaptor" header files that 
  contain the required configuration to connect the Phantasma SDK to existing
  popular open source libraries for different features:

 |Library   | Features     | #include file                       | Library URL                             |
 |----------|--------------|-------------------------------------|-----------------------------------------|
 |C++ REST  | HTTP + JSON  | `Adapters/PhantasmaAPI_cpprest.h`   | https://github.com/microsoft/cpprestsdk |
 |libcurl   | HTTP         | `Adapters/PhantasmaAPI_curl.h`      | https://curl.haxx.se/libcurl/           |
 |RapidJSON | JSON         | `Adapters/PhantasmaAPI_rapidjson.h` | http://rapidjson.org/                   |
 |Sodium    | Cryptography | `Adapters/PhantasmaAPI_sodium.h`    | https://libsodium.org                   |

------------------------------------------------------------------------------
 JSON
------------------------------------------------------------------------------
 This header contains JSON parsing and building code, but it is written to be
  as simple as possible (approx 200 lines of code) and is not high-performance
  or highly robust.

 It is recommended that you supply another JSON-parsing API, by defining the
  following macros before including `phantasma.h`:

  `#define PHANTASMA_JSONVALUE    Your_Json_Value_Type`

  `#define PHANTASMA_JSONARRAY    Your_Json_Array_Type`

  `#define PHANTASMA_JSONDOCUMENT Your_JSON_Document_Type`

  `#define PHANTASMA_JSONBUILDER  Your_Json_Serializer_Type`

 **The CPP REST and RapidJSON adaptors implement these macros.**

 Also, this header uses the following procedural API to interact with these types.
 If you have supplied your own JSON types, you must implement the following functions:

     namespace phantasma { namespace json {
     
        JSONValue Parse(const JSONDocument&);
     
        bool      LookupBool(   const JSONValue&, const Char* field, bool& out_error);
        Int32     LookupInt32(  const JSONValue&, const Char* field, bool& out_error);
        UInt32    LookupUInt32( const JSONValue&, const Char* field, bool& out_error);
        String    LookupString( const JSONValue&, const Char* field, bool& out_error);
        JSONValue LookupValue(  const JSONValue&, const Char* field, bool& out_error);
        JSONArray LookupArray(  const JSONValue&, const Char* field, bool& out_error);
        bool      HasField(     const JSONValue&, const Char* field, bool& out_error);
        bool      HasArrayField(const JSONValue&, const Char* field, bool& out_error);
     
        bool      AsBool(       const JSONValue&,                    bool& out_error);
        Int32     AsInt32(      const JSONValue&,                    bool& out_error);
        UInt32    AsUInt32(     const JSONValue&,                    bool& out_error);
        String    AsString(     const JSONValue&,                    bool& out_error);
        JSONArray AsArray(      const JSONValue&,                    bool& out_error);
        bool      IsArray(      const JSONValue&,                    bool& out_error);
        bool      IsObject(     const JSONValue&,                    bool& out_error);
        
        int       ArraySize(    const JSONArray&,                    bool& out_error);
        JSONValue IndexArray(   const JSONArray&, int index,         bool& out_error);

        // Enumerates the members of an object; needed by VM values and metadata maps, whose
        // field names are chain data rather than a fixed schema.
        template<class Visitor>
        void      VisitObjectFields(const JSONValue&, Visitor&& visit, bool& out_error);
        // Any scalar as text (a number or a boolean as its JSON text, a null as empty).
        String    ScalarText(   const JSONValue&,                    bool& out_error);
        // Any value back as JSON text; extended events keep unmodeled payloads this way.
        String    ToText(       const JSONValue&,                    bool& out_error);
     
                               void BeginObject(JSONBuilder&);
                               void AddString  (JSONBuilder&, const Char* key, const Char* value);
       template<class... Args> void AddArray   (JSONBuilder&, const Char* key, Args...);
                               void EndObject  (JSONBuilder&);
     }}

------------------------------------------------------------------------------
 HTTP
------------------------------------------------------------------------------
 This header does not contain a HTTP client, nor a dependency on any specific
  HTTP client library. If you do not supply a HTTP client library, then only
  the Low-level phantasma API (`PhantasmaJsonAPI`) is available.

 To enable the `PhantasmaAPI` class, defining the following macro before 
  including `phantasma.h`:
  
 `#define PHANTASMA_HTTPCLIENT   Your_HTTP_Client_Type`

 **The CPP REST and libcurl adaptors implement this macro.**

 Also, this header uses the following procedural API to interact with this type.
 If you have defined `PHANTASMA_HTTPCLIENT`, you must implement the following,
  function, which should perform a HTTP POST request and return the result:

     namespace phantasma {
      JSONDocument HttpPost(HttpClient&, const Char* uri, const JSONBuilder&);
     }

------------------------------------------------------------------------------
 Extended/Advanced usage
------------------------------------------------------------------------------
 This header file contains the entirety of the RPC API requried to communicate 
  with a Phantasma node. If you are not trying to create transactions, this 
  may be enough for you.

 However, for advanced usage, such as creating and signing transactions, much
  more code is required, including cryptography, N-bit ingeger arithmetic, etc.
 The other header files that are included in this distribution, in sub-folders
  listed below, provide these extra features:

  |Directory     | Features                                                              |
  |--------------|-----------------------------------------------------------------------|
  | Adapters     | Configuration for this library to communicate with 3rd party libraries|
  | Blockchain   | Transactions                                                          |
  | Cryptography | Public/Private keys, Signatures, Random numbers, Encryption           |
  | Numerics     | N-bit integer implementation. Base 16/58 ASCII encoding.              |
  | Security     | Practical memory protection.                                          |

------------------------------------------------------------------------------
 - Extended/Advanced usage - Security configuration
------------------------------------------------------------------------------
   To securely process transactions and private keys, it is strongly advised to 
   pair the PhantasmaAPI with strong 3rd party security library.

   **The Sodium adaptor implements these macros.**
   
  |#define                      |                                                                                                              |
  |-----------------------------|--------------------------------------------------------------------------------------------------------------|
  |`PHANTASMA_RANDOMBYTES`      | Fill a memory range with cryptographically secure pseudo-random numbers                                      |
  |`PHANTASMA_WIPEMEM`          | Fill a memory range with 0's in a way that won't be "optimized away"                                         |
  |`PHANTASMA_LOCKMEM`          | Pin the memory pages containing this range, and otherwise inform the OS that it contains secrets.            |
  |`PHANTASMA_UNLOCKMEM`        | Undo the actions of `PHANTASMA_LOCKMEM`, but also fill the memory range with 0's as with `PHANTASMA_WIPEMEM`.|
  |`PHANTASMA_SECURE_ALLOC`     | Similar to malloc, but should return dedicated pages that can have their access permissions modified.        |
  |`PHANTASMA_SECURE_FREE`      | Similar to free - used with allocations returned from `PHANTASMA_SECURE_ALLOC`                               |
  |`PHANTASMA_SECURE_NOACCESS`  | Used with allocations returned from `PHANTASMA_SECURE_ALLOC`. Mark the pages as non-readable.                |
  |`PHANTASMA_SECURE_READONLY`  | Used with allocations returned from `PHANTASMA_SECURE_ALLOC`. Mark the pages as read only.                   |
  |`PHANTASMA_SECURE_READWRITE` | Used with allocations returned from `PHANTASMA_SECURE_ALLOC`. Mark the pages as writable.                    |

------------------------------------------------------------------------------
 - Extended/Advanced usage - Cryptography configuration
------------------------------------------------------------------------------
  To create or validate transactions, an EdDSA Ed25519 implementation is requied.
   The libSodium adaptor implements these macros.
   
  |#define                                |                                                               |
  |---------------------------------------|---------------------------------------------------------------|
  |`PHANTASMA_Ed25519_PublicKeyFromSeed`  | Generate a 32 byte public key from a 32 byte seed.            |
  |`PHANTASMA_Ed25519_PrivateKeyFromSeed` | Generate a 64 byte public key from a 32 byte seed.            |
  |`PHANTASMA_Ed25519_SignDetached`       | Generate a 64 byte signature from a message and a private key.|
  |`PHANTASMA_Ed25519_ValidateDetached`   | Validate a 64 byte signature using a public key.              |
