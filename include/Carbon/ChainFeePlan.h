#pragma once
#ifndef PHANTASMA_API_INCLUDED
#error "Configure and include PhantasmaAPI.h first"
#endif

#include "FeeInfusions.h"
#include "FeePlan.h"

#include <chrono>

// Planning a fee against the chain the message is going to.
//
// `PlanFees` prices a message from the message and a gas config, and it demands the facts it cannot
// derive. This planner has a chain to ask: it holds the gas config, refreshes it when it is stale,
// and reads what a burned NFT holds instead of demanding it. What a plan means and how firm it is:
// FeePlan.h.

#if defined(PHANTASMA_HTTPCLIENT)

namespace phantasma::carbon {

class ChainFeePlanner
{
  public:
	// `configTtlMs` is how long a read gas config is used before it is read again. The config changes
	// only when a governance resolution changes it, so a minute is generous.
	explicit ChainFeePlanner(rpc::PhantasmaAPI& api, int64_t configTtlMs = 60000)
	    : m_api(api), m_ttlMs(configTtlMs)
	{
	}

	// The chain's gas config, read when the held one is missing or older than the TTL.
	bool Config(Blockchain::GasConfig& out, std::string& outError, bool refresh = false)
	{
		const int64_t now = NowMs();
		if( !refresh && m_haveConfig && now - m_fetchedAt < m_ttlMs )
		{
			out = m_config;
			return true;
		}

		rpc::PhantasmaError error;
		const rpc::GasConfigResult answer = m_api.GetGasConfig(&error);
		if( error.code != 0 )
		{
			outError = std::string("the gas config could not be read: ") + error.message.c_str();
			return false;
		}
		m_config = ToGasConfig(answer.gasConfig);
		m_haveConfig = true;
		m_fetchedAt = now;
		out = m_config;
		return true;
	}

	// Forgets the held config. The next plan reads it again.
	void Invalidate() { m_haveConfig = false; }

	// Prices `msg` against the chain's current prices. A burn's returned assets are read off the
	// chain unless the caller stated them, because the planner has no costlier bound for that set.
	bool Plan(const Blockchain::TxMsg& msg, const FeePlanOptions& options, FeePlan& out, std::string& outError)
	{
		Blockchain::GasConfig config{};
		if( !Config(config, outError) )
		{
			return false;
		}

		FeePlanOptions filled = options;
		PHANTASMA_VECTOR<InfusedAsset> infusions;
		if( !filled.infusionsRead )
		{
			PHANTASMA_VECTOR<BurnedInstance> burned;
			if( !BurnedInstances(msg, burned) )
			{
				outError = "a burn in this message has arguments too short to read";
				return false;
			}
			if( burned.empty() )
			{
				// Nothing is burned, so nothing comes back and there is nothing to read.
				filled.infusionsRead = true;
			}
			else
			{
				rpc::PhantasmaError error;
				if( !ReadInfusedAssets(m_api, msg, infusions, &error) )
				{
					outError = std::string("the assets the burned instances hold could not be read: ") + error.message.c_str();
					return false;
				}
				filled.infusions = infusions.empty() ? nullptr : &infusions.front();
				filled.numInfusions = (uint32_t)infusions.size();
				filled.infusionsRead = true;
			}
		}

		if( !PlanFees(msg, config, filled, out) )
		{
			outError = "the message cannot be priced";
			return false;
		}
		return true;
	}

  private:
	static int64_t NowMs()
	{
		return (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
		    std::chrono::system_clock::now().time_since_epoch())
		    .count();
	}

	rpc::PhantasmaAPI& m_api;
	int64_t m_ttlMs = 60000;
	Blockchain::GasConfig m_config{};
	bool m_haveConfig = false;
	int64_t m_fetchedAt = 0;
};

} // namespace phantasma::carbon

#endif
