/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file CanonBlockChain.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <mutex>
#include <libdevcore/Log.h>
#include <libdevcore/Exceptions.h>
#include <libethcore/Common.h>
#include <libethcore/BlockInfo.h>
#include <libethcore/Ethash.h>
#include <libdevcore/Guards.h>
#include "BlockDetails.h"
#include "Account.h"
#include "BlockChain.h"

namespace dev
{

namespace eth
{

// TODO: Move all this Genesis stuff into Genesis.h/.cpp
std::unordered_map<Address, Account> const& genesisState();

/**
 * @brief Implements the blockchain database. All data this gives is disk-backed.
 * @threadsafe
 * @todo Make not memory hog (should actually act as a cache and deallocate old entries).
 */
template <class Sealer>
class CanonBlockChain: public FullBlockChain<Sealer>
{
public:
	CanonBlockChain(WithExisting _we = WithExisting::Trust, ProgressCallback const& _pc = ProgressCallback()): CanonBlockChain<Sealer>(std::string(), _we, _pc) {}
	CanonBlockChain(std::string const& _path, WithExisting _we = WithExisting::Trust, ProgressCallback const& _pc = ProgressCallback()):
		FullBlockChain<Sealer>(createGenesisBlock(), AccountMap(), _path)
	{
		BlockChain::openDatabase(_path, _we, _pc);
	}
	~CanonBlockChain() {}

	/// @returns the genesis block as its RLP-encoded byte array.
	/// @note This is slow as it's constructed anew each call. Consider genesis() instead.
	static bytes createGenesisBlock()
	{
		RLPStream block(3);
		block.appendList(Sealer::BlockHeader::Fields)
				<< h256() << EmptyListSHA3 << h160() << EmptyTrie << EmptyTrie << EmptyTrie << LogBloom() << 1 << 0 << (u256(1) << 255) << 0 << (unsigned)0 << std::string();
		bytes sealFields = typename Sealer::BlockHeader().sealFieldsRLP();
		block.appendRaw(sealFields, Sealer::BlockHeader::SealFields);
		block.appendRaw(RLPEmptyList);
		block.appendRaw(RLPEmptyList);
		return block.out();
	}
};

template <>
class CanonBlockChain<Ethash>: public FullBlockChain<Ethash>
{
public:
	CanonBlockChain(WithExisting _we = WithExisting::Trust, ProgressCallback const& _pc = ProgressCallback()): CanonBlockChain(std::string(), _we, _pc) {}
	CanonBlockChain(std::string const& _path, WithExisting _we = WithExisting::Trust, ProgressCallback const& _pc = ProgressCallback());
	~CanonBlockChain() {}

	/// Reopen everything.
	virtual void reopen(WithExisting _we = WithExisting::Trust, ProgressCallback const& _pc = ProgressCallback());

	/// @returns the genesis block header.
	static Ethash::BlockHeader const& genesis();

	/// @returns the genesis block as its RLP-encoded byte array.
	/// @note This is slow as it's constructed anew each call. Consider genesis() instead.
	static bytes createGenesisBlock();

	/// @returns the genesis block's state.
	/// @note This is slow as it's constructed anew each call. Consider genesis() instead.
	static AccountMap const& createGenesisState();

	/// @returns the genesis block's state root; or h256() if not known.
	static h256 knownGenesisStateRoot();

	/// Alter all the genesis block's state by giving a JSON string with account details.
	/// @warning Unless you're very careful, make sure you call this right at the start of the
	/// program, before anything has had the chance to use this class at all.
	static void setGenesis(std::string const& _genesisInfoJSON);

	/// Override the genesis block's extraData field.
	static void forceGenesisExtraData(bytes const& _genesisExtraData);

	/// Override the genesis block's difficulty field.
	static void forceGenesisDifficulty(u256 const& _genesisDifficulty);

	/// Override the genesis block's gasLimit field.
	static void forceGenesisGasLimit(u256 const& _genesisGasLimit);

	/// @returns true if any of the overrides are in force.
	static bool isNonStandard() { return s_genesisExtraData.size() || s_genesisDifficulty || s_genesisGasLimit; }

private:
	/// Static genesis info and its lock.
	static boost::shared_mutex x_genesis;
	static std::unique_ptr<Ethash::BlockHeader> s_genesis;
	static Nonce s_nonce;
	static std::string s_genesisStateJSON;
	static bytes s_genesisExtraData;
	static u256 s_genesisDifficulty;
	static u256 s_genesisGasLimit;
};

}
}
