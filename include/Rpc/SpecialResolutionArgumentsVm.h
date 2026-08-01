//------------------------------------------------------------------------------
// Phantasma SDK: arguments of the phantasma_vm module calls a special resolution can carry,
// including the migration calls that rebuild contracts and series.
//------------------------------------------------------------------------------
// Part of PhantasmaAPI.h; include that file, not this one.
//------------------------------------------------------------------------------
#pragma once

#include "SpecialResolutionArgumentsBase.h"

namespace phantasma {
namespace rpc {

// Arguments of phantasma_vm.ExecuteScript.
struct ExecuteScriptArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::ExecuteScript> {
	String maxGas;
	String gasFrom;
	String script;
};

inline ExecuteScriptArguments ParseExecuteScriptArguments(const JSONValue& value, bool& jsonErr)
{
	ExecuteScriptArguments output;
	output.maxGas = ReadArgumentString(value, PHANTASMA_LITERAL("maxGas"), jsonErr);
	output.gasFrom = ReadArgumentString(value, PHANTASMA_LITERAL("gasFrom"), jsonErr);
	output.script = ReadArgumentString(value, PHANTASMA_LITERAL("script"), jsonErr);
	return output;
}

// Arguments of phantasma_vm.RegisterTokenContract.
struct RegisterTokenContractArguments
    : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::RegisterTokenContract> {
	String tokenId;
	String symbol;
	String script;
	String abi;
	// Resolved token symbol; empty when the token could not be resolved at answer time.
	String token;
};

inline RegisterTokenContractArguments ParseRegisterTokenContractArguments(const JSONValue& value, bool& jsonErr)
{
	RegisterTokenContractArguments output;
	output.tokenId = ReadArgumentString(value, PHANTASMA_LITERAL("tokenId"), jsonErr);
	output.symbol = ReadArgumentString(value, PHANTASMA_LITERAL("symbol"), jsonErr);
	output.script = ReadArgumentString(value, PHANTASMA_LITERAL("script"), jsonErr);
	output.abi = ReadArgumentString(value, PHANTASMA_LITERAL("abi"), jsonErr);
	output.token = ReadArgumentString(value, PHANTASMA_LITERAL("token"), jsonErr);
	return output;
}

// Arguments of phantasma_vm.DeployContract.
struct DeployContractArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::DeployContract> {
	String from;
	String contractName;
	String script;
	String abi;
};

inline DeployContractArguments ParseDeployContractArguments(const JSONValue& value, bool& jsonErr)
{
	DeployContractArguments output;
	output.from = ReadArgumentString(value, PHANTASMA_LITERAL("from"), jsonErr);
	output.contractName = ReadArgumentString(value, PHANTASMA_LITERAL("contractName"), jsonErr);
	output.script = ReadArgumentString(value, PHANTASMA_LITERAL("script"), jsonErr);
	output.abi = ReadArgumentString(value, PHANTASMA_LITERAL("abi"), jsonErr);
	return output;
}

// Arguments of phantasma_vm.SetConfig.
struct PhantasmaVmConfigArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::PhantasmaVmConfig> {
	String featureLevel;
	String gasConstructor;
	String gasNexus;
	String gasOrganization;
	String gasAccount;
	String gasLeaderboard;
	String gasStandard;
	String gasOracle;
	String fuelPerContractDeploy;
};

inline PhantasmaVmConfigArguments ParsePhantasmaVmConfigArguments(const JSONValue& value, bool& jsonErr)
{
	PhantasmaVmConfigArguments output;
	output.featureLevel = ReadArgumentString(value, PHANTASMA_LITERAL("featureLevel"), jsonErr);
	output.gasConstructor = ReadArgumentString(value, PHANTASMA_LITERAL("gasConstructor"), jsonErr);
	output.gasNexus = ReadArgumentString(value, PHANTASMA_LITERAL("gasNexus"), jsonErr);
	output.gasOrganization = ReadArgumentString(value, PHANTASMA_LITERAL("gasOrganization"), jsonErr);
	output.gasAccount = ReadArgumentString(value, PHANTASMA_LITERAL("gasAccount"), jsonErr);
	output.gasLeaderboard = ReadArgumentString(value, PHANTASMA_LITERAL("gasLeaderboard"), jsonErr);
	output.gasStandard = ReadArgumentString(value, PHANTASMA_LITERAL("gasStandard"), jsonErr);
	output.gasOracle = ReadArgumentString(value, PHANTASMA_LITERAL("gasOracle"), jsonErr);
	output.fuelPerContractDeploy = ReadArgumentString(value, PHANTASMA_LITERAL("fuelPerContractDeploy"), jsonErr);
	return output;
}

// A key/value row of contract storage; both sides are hex because they hold arbitrary bytes.
struct ContractStorageRow {
	String key;
	String value;
};

inline ContractStorageRow ParseContractStorageRow(const JSONValue& value, bool& jsonErr)
{
	ContractStorageRow output;
	output.key = ReadArgumentString(value, PHANTASMA_LITERAL("key"), jsonErr);
	output.value = ReadArgumentString(value, PHANTASMA_LITERAL("value"), jsonErr);
	return output;
}

// One map or list table of a contract, with every row it carries.
struct ContractStorageTable {
	String name;
	PHANTASMA_VECTOR<ContractStorageRow> rows;
};

inline ContractStorageTable ParseContractStorageTable(const JSONValue& value, bool& jsonErr)
{
	ContractStorageTable output;
	output.name = ReadArgumentString(value, PHANTASMA_LITERAL("name"), jsonErr);
	output.rows = ReadArgumentObjectArray<ContractStorageRow>(
	    value, PHANTASMA_LITERAL("rows"), jsonErr, ParseContractStorageRow);
	return output;
}

// One contract restored by a migration: identity, code and the whole of its stored state.
struct ImportedContract {
	String name;
	String address;
	String owner;
	String script;
	String abi;
	// Root-level contract variables.
	PHANTASMA_VECTOR<ContractStorageRow> rootVariables;
	// Map and list tables, including their backing rows.
	PHANTASMA_VECTOR<ContractStorageTable> tables;
};

inline ImportedContract ParseImportedContract(const JSONValue& value, bool& jsonErr)
{
	ImportedContract output;
	output.name = ReadArgumentString(value, PHANTASMA_LITERAL("name"), jsonErr);
	output.address = ReadArgumentString(value, PHANTASMA_LITERAL("address"), jsonErr);
	output.owner = ReadArgumentString(value, PHANTASMA_LITERAL("owner"), jsonErr);
	output.script = ReadArgumentString(value, PHANTASMA_LITERAL("script"), jsonErr);
	output.abi = ReadArgumentString(value, PHANTASMA_LITERAL("abi"), jsonErr);
	output.rootVariables = ReadArgumentObjectArray<ContractStorageRow>(
	    value, PHANTASMA_LITERAL("rootVariables"), jsonErr, ParseContractStorageRow);
	output.tables = ReadArgumentObjectArray<ContractStorageTable>(
	    value, PHANTASMA_LITERAL("tables"), jsonErr, ParseContractStorageTable);
	return output;
}

// Arguments of phantasma_vm.ImportContracts.
struct ImportContractsArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::ImportContracts> {
	String contractsCount;
	PHANTASMA_VECTOR<ImportedContract> contracts;
};

inline ImportContractsArguments ParseImportContractsArguments(const JSONValue& value, bool& jsonErr)
{
	ImportContractsArguments output;
	output.contractsCount = ReadArgumentString(value, PHANTASMA_LITERAL("contractsCount"), jsonErr);
	output.contracts = ReadArgumentObjectArray<ImportedContract>(
	    value, PHANTASMA_LITERAL("contracts"), jsonErr, ParseImportedContract);
	return output;
}

// The definition needed to rebuild one Phantasma series.
struct SeriesSupplement {
	String token;
	String tokenId;
	String phantasmaSeriesId;
	String maxSupply;
	String mintCount;
	String mode;
	String script;
	String abi;
	String rom;
};

inline SeriesSupplement ParseSeriesSupplement(const JSONValue& value, bool& jsonErr)
{
	SeriesSupplement output;
	output.token = ReadArgumentString(value, PHANTASMA_LITERAL("token"), jsonErr);
	output.tokenId = ReadArgumentString(value, PHANTASMA_LITERAL("tokenId"), jsonErr);
	output.phantasmaSeriesId = ReadArgumentString(value, PHANTASMA_LITERAL("phantasmaSeriesId"), jsonErr);
	output.maxSupply = ReadArgumentString(value, PHANTASMA_LITERAL("maxSupply"), jsonErr);
	output.mintCount = ReadArgumentString(value, PHANTASMA_LITERAL("mintCount"), jsonErr);
	output.mode = ReadArgumentString(value, PHANTASMA_LITERAL("mode"), jsonErr);
	output.script = ReadArgumentString(value, PHANTASMA_LITERAL("script"), jsonErr);
	output.abi = ReadArgumentString(value, PHANTASMA_LITERAL("abi"), jsonErr);
	output.rom = ReadArgumentString(value, PHANTASMA_LITERAL("rom"), jsonErr);
	return output;
}

// The mint-count repair of one Phantasma series.
struct SeriesMintCountRepair {
	String token;
	String tokenId;
	String phantasmaSeriesId;
	String importedLiveCount;
	String script;
	String abi;
};

inline SeriesMintCountRepair ParseSeriesMintCountRepair(const JSONValue& value, bool& jsonErr)
{
	SeriesMintCountRepair output;
	output.token = ReadArgumentString(value, PHANTASMA_LITERAL("token"), jsonErr);
	output.tokenId = ReadArgumentString(value, PHANTASMA_LITERAL("tokenId"), jsonErr);
	output.phantasmaSeriesId = ReadArgumentString(value, PHANTASMA_LITERAL("phantasmaSeriesId"), jsonErr);
	output.importedLiveCount = ReadArgumentString(value, PHANTASMA_LITERAL("importedLiveCount"), jsonErr);
	output.script = ReadArgumentString(value, PHANTASMA_LITERAL("script"), jsonErr);
	output.abi = ReadArgumentString(value, PHANTASMA_LITERAL("abi"), jsonErr);
	return output;
}

// Arguments of phantasma_vm.RepairSeries.
struct RepairSeriesArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::RepairSeries> {
	String supplementsCount;
	PHANTASMA_VECTOR<SeriesSupplement> supplements;
	String repairsCount;
	PHANTASMA_VECTOR<SeriesMintCountRepair> repairs;
};

inline RepairSeriesArguments ParseRepairSeriesArguments(const JSONValue& value, bool& jsonErr)
{
	RepairSeriesArguments output;
	output.supplementsCount = ReadArgumentString(value, PHANTASMA_LITERAL("supplementsCount"), jsonErr);
	output.supplements = ReadArgumentObjectArray<SeriesSupplement>(
	    value, PHANTASMA_LITERAL("supplements"), jsonErr, ParseSeriesSupplement);
	output.repairsCount = ReadArgumentString(value, PHANTASMA_LITERAL("repairsCount"), jsonErr);
	output.repairs = ReadArgumentObjectArray<SeriesMintCountRepair>(
	    value, PHANTASMA_LITERAL("repairs"), jsonErr, ParseSeriesMintCountRepair);
	return output;
}

// The repair of one token definition.
struct TokenRepair {
	String token;
	String tokenId;
	String symbol;
	String script;
	String abi;
	String tokenFlags;
	// Bitmask of the repair operations the chain was asked to perform. Kept numeric on purpose: a
	// new chain-side operation must not silently render as an unrelated name here.
	String repairMask;
};

inline TokenRepair ParseTokenRepair(const JSONValue& value, bool& jsonErr)
{
	TokenRepair output;
	output.token = ReadArgumentString(value, PHANTASMA_LITERAL("token"), jsonErr);
	output.tokenId = ReadArgumentString(value, PHANTASMA_LITERAL("tokenId"), jsonErr);
	output.symbol = ReadArgumentString(value, PHANTASMA_LITERAL("symbol"), jsonErr);
	output.script = ReadArgumentString(value, PHANTASMA_LITERAL("script"), jsonErr);
	output.abi = ReadArgumentString(value, PHANTASMA_LITERAL("abi"), jsonErr);
	output.tokenFlags = ReadArgumentString(value, PHANTASMA_LITERAL("tokenFlags"), jsonErr);
	output.repairMask = ReadArgumentString(value, PHANTASMA_LITERAL("repairMask"), jsonErr);
	return output;
}

// Arguments of phantasma_vm.RepairToken.
struct RepairTokenArguments : SpecialResolutionArgumentsOf<SpecialResolutionArgumentType::RepairToken> {
	String repairsCount;
	PHANTASMA_VECTOR<TokenRepair> repairs;
};

inline RepairTokenArguments ParseRepairTokenArguments(const JSONValue& value, bool& jsonErr)
{
	RepairTokenArguments output;
	output.repairsCount = ReadArgumentString(value, PHANTASMA_LITERAL("repairsCount"), jsonErr);
	output.repairs =
	    ReadArgumentObjectArray<TokenRepair>(value, PHANTASMA_LITERAL("repairs"), jsonErr, ParseTokenRepair);
	return output;
}

} // namespace rpc
} // namespace phantasma
