// Copyright (c) 2014-2018, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <vector>
#include <iostream>
#include <stdint.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/variant.hpp>

#include "include_base_utils.h"
#include "common/boost_serialization_helper.h"
#include "common/command_line.h"

#include "cryptonote_basic/account_boost_serialization.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "misc_language.h"

#undef LOKI_DEFAULT_LOG_CATEGORY
#define LOKI_DEFAULT_LOG_CATEGORY "tests.core"

#define TESTS_DEFAULT_FEE ((uint64_t)200000000) // 2 * pow(10, 8)

struct callback_entry
{
  std::string callback_name;
  BEGIN_SERIALIZE_OBJECT()
    FIELD(callback_name)
  END_SERIALIZE()

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & callback_name;
  }
};

template<typename T>
struct serialized_object
{
  serialized_object() { }

  serialized_object(const cryptonote::blobdata& a_data)
    : data(a_data)
  {
  }

  cryptonote::blobdata data;
  BEGIN_SERIALIZE_OBJECT()
    FIELD(data)
    END_SERIALIZE()

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & data;
  }
};

typedef serialized_object<cryptonote::block> serialized_block;
typedef serialized_object<cryptonote::transaction> serialized_transaction;

struct event_visitor_settings
{
  int valid_mask;
  bool txs_keeped_by_block;
  crypto::secret_key service_node_key;

  enum settings
  {
    set_txs_keeped_by_block = 1 << 0,
    set_service_node_key = 1 << 1
  };

  event_visitor_settings(int a_valid_mask = 0, bool a_txs_keeped_by_block = false)
    : valid_mask(a_valid_mask)
    , txs_keeped_by_block(a_txs_keeped_by_block)
  {
  }

  static event_visitor_settings make_set_service_node_key(const crypto::secret_key& a_service_node_key)
  {
    event_visitor_settings settings(set_service_node_key);
    settings.service_node_key = a_service_node_key;
    return settings;
  }

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & valid_mask;
    ar & txs_keeped_by_block;
    ar & service_node_key;
  }
};

VARIANT_TAG(binary_archive, callback_entry, 0xcb);
VARIANT_TAG(binary_archive, cryptonote::account_base, 0xcc);
VARIANT_TAG(binary_archive, serialized_block, 0xcd);
VARIANT_TAG(binary_archive, serialized_transaction, 0xce);
VARIANT_TAG(binary_archive, event_visitor_settings, 0xcf);

typedef boost::variant<cryptonote::block, cryptonote::transaction, std::vector<cryptonote::transaction>, cryptonote::account_base, callback_entry, serialized_block, serialized_transaction, event_visitor_settings> test_event_entry;
typedef std::unordered_map<crypto::hash, const cryptonote::transaction*> map_hash2tx_t;

class test_chain_unit_base
{
public:
  typedef boost::function<bool (cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)> verify_callback;
  typedef std::map<std::string, verify_callback> callbacks_map;

  void register_callback(const std::string& cb_name, verify_callback cb);
  bool verify(const std::string& cb_name, cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
private:
  callbacks_map m_callbacks;
};

using sn_contributor_t = std::pair<cryptonote::account_public_address, uint64_t>;

class test_generator
{
public:
  struct block_info
  {
    block_info()
      : prev_id()
      , already_generated_coins(0)
      , block_weight(0)
    {
    }

    block_info(crypto::hash a_prev_id, uint64_t an_already_generated_coins, size_t a_block_weight, cryptonote::block a_block)
      : prev_id(a_prev_id)
      , already_generated_coins(an_already_generated_coins)
      , block_weight(a_block_weight)
      , block(a_block)
    {
    }

    crypto::hash prev_id;
    uint64_t already_generated_coins;
    size_t block_weight;
    cryptonote::block block;
  };

  enum block_fields
  {
    bf_none      = 0,
    bf_major_ver = 1 << 0,
    bf_minor_ver = 1 << 1,
    bf_timestamp = 1 << 2,
    bf_prev_id   = 1 << 3,
    bf_miner_tx  = 1 << 4,
    bf_tx_hashes = 1 << 5,
    bf_diffic    = 1 << 6,
    bf_hf_version= 1 << 8
  };

  void get_block_chain(std::vector<block_info>& blockchain,        const crypto::hash& head, size_t n) const;
  void get_block_chain(std::vector<cryptonote::block>& blockchain, const crypto::hash& head, size_t n) const;

  void get_last_n_block_weights(std::vector<uint64_t>& block_weights, const crypto::hash& head, size_t n) const;
  uint64_t get_already_generated_coins(const crypto::hash& blk_id) const;
  uint64_t get_already_generated_coins(const cryptonote::block& blk) const;

  void add_block(const cryptonote::block& blk, size_t tsx_size, std::vector<uint64_t>& block_weights, uint64_t already_generated_coins);
  bool construct_block(cryptonote::block& blk, uint64_t height, const crypto::hash& prev_id,
    const cryptonote::account_base& miner_acc, uint64_t timestamp, uint64_t already_generated_coins,
    std::vector<uint64_t>& block_weights, const std::list<cryptonote::transaction>& tx_list, const crypto::public_key& sn_pub_key = crypto::null_pkey,
    const std::vector<sn_contributor_t>& = {{{crypto::null_pkey, crypto::null_pkey}, STAKING_PORTIONS}});
  bool construct_block(cryptonote::block& blk, const cryptonote::account_base& miner_acc, uint64_t timestamp);
  bool construct_block(cryptonote::block& blk, const cryptonote::block& blk_prev, const cryptonote::account_base& miner_acc,
    const std::list<cryptonote::transaction>& tx_list = std::list<cryptonote::transaction>(), const crypto::public_key& sn_pub_key = crypto::null_pkey,
    const std::vector<sn_contributor_t>& = {{{crypto::null_pkey, crypto::null_pkey}, STAKING_PORTIONS}});

  bool construct_block_manually(cryptonote::block& blk, const cryptonote::block& prev_block,
    const cryptonote::account_base& miner_acc, int actual_params = bf_none, uint8_t major_ver = 0,
    uint8_t minor_ver = 0, uint64_t timestamp = 0, const crypto::hash& prev_id = crypto::hash(),
    const cryptonote::difficulty_type& diffic = 1, const cryptonote::transaction& miner_tx = cryptonote::transaction(),
    const std::vector<crypto::hash>& tx_hashes = std::vector<crypto::hash>(), size_t txs_sizes = 0);
  bool construct_block_manually_tx(cryptonote::block& blk, const cryptonote::block& prev_block,
    const cryptonote::account_base& miner_acc, const std::vector<crypto::hash>& tx_hashes, size_t txs_size);

  explicit test_generator(int hf_version = 7) : m_hf_version(hf_version) {}

  int m_hf_version;

private:
  std::unordered_map<crypto::hash, block_info> m_blocks_info;
};

/// ------------ Service Nodes -----------

struct last_reward_point {
  uint64_t height;
  uint64_t priority;
};

struct sn_registration {
  uint64_t valid_until; /// block height
  cryptonote::keypair keys;
  sn_contributor_t contribution;
  last_reward_point last_reward;
};

class sn_list
{
  std::vector<sn_registration> sn_owners_;

public:

  const sn_registration& at(size_t idx) const { return sn_owners_.at(idx); }

  const boost::optional<sn_registration> find_registration(const crypto::public_key& pk) const;

  void expire_old(uint64_t height);

  const boost::optional<crypto::public_key> get_winner_pk(uint64_t height);

  size_t size() const { return sn_owners_.size(); }

  void add_registrations(const std::vector<sn_registration>& regs);

  void remove_node(const crypto::public_key& pk);

  void handle_deregistrations(const std::vector<crypto::public_key>& dereg_buffer);

};

/// Service node and its index
struct sn_idx {
  crypto::public_key sn_pk;
  /// index in the sorted list of service nodes for a particular block
  size_t idx_in_quorum;
};

struct QuorumState {
  std::vector<sn_idx> voters;
  std::vector<sn_idx> to_test;
};

class dereg_tx_builder;
class linear_chain_generator
{

  private:
    test_generator gen_;
    std::vector<test_event_entry>& events_;
    const std::vector<std::pair<uint8_t, uint64_t>> hard_forks_;
    std::vector<cryptonote::block> blocks_;

    sn_list sn_list_;

    /// keep new registrations and deregistrations here until the next block
    std::vector<sn_registration> registration_buffer_;
    std::vector<crypto::public_key> deregistration_buffer_;

    cryptonote::account_base first_miner_;

    /// Get hardfork version at specified height
    uint8_t get_hf_version_at(uint64_t height) const;

  public:
    linear_chain_generator(std::vector<test_event_entry> &events, const std::vector<std::pair<uint8_t, uint64_t>> &hard_forks)
      : gen_(), events_(events), hard_forks_(hard_forks)
    { }

    uint64_t                              height() const { return get_block_height(blocks_.back()); }
    const std::vector<cryptonote::block>& blocks() const { return blocks_; }

    cryptonote::account_base create_account();

    void create_genesis_block();

    void create_block(const std::vector<cryptonote::transaction>& txs = {});

    cryptonote::block create_block_on_fork(const cryptonote::block& prev, const std::vector<cryptonote::transaction>& txs = {});

    int get_hf_version() const;

    void rewind_until_version(int hard_fork_version);
    void rewind_blocks_n(int n);
    void rewind_blocks();

    cryptonote::transaction create_tx(const cryptonote::account_base& miner,
                                      const cryptonote::account_base& acc,
                                      uint64_t amount,
                                      uint64_t fee = TESTS_DEFAULT_FEE);

    cryptonote::transaction create_registration_tx(const cryptonote::account_base& acc, const cryptonote::keypair& sn_keys);

    cryptonote::transaction create_registration_tx();

    const cryptonote::account_base& first_miner() const { return first_miner_; }

    /// Note: should be carefull with returing a reference to vector elements
    const cryptonote::block& chain_head() const { return blocks_.back(); }

    /// get a copy of the service node list
    sn_list get_sn_list() const { return sn_list_; }

    void set_sn_list(const sn_list& list) { sn_list_ = list; }

    QuorumState get_quorum_idxs(const cryptonote::block& block) const;

    QuorumState get_quorum_idxs(uint64_t height) const;

    cryptonote::transaction create_deregister_tx(const crypto::public_key& pk, uint64_t height, const std::vector<sn_idx>& voters, uint64_t fee, bool commit);

    dereg_tx_builder build_deregister(const crypto::public_key& pk, bool commit = true);

    crypto::public_key get_test_pk(uint32_t idx) const;

    boost::optional<uint32_t> get_idx_in_tested(const crypto::public_key& pk, uint64_t height) const;

    void deregister(const crypto::public_key& pk);

};

class dereg_tx_builder {

  linear_chain_generator& gen_;
  const crypto::public_key& pk_;

  /// the height at which `pk_` is to be found
  boost::optional<uint64_t> height_ = boost::none;

  boost::optional<uint64_t> fee_ = boost::none;

  boost::optional<const std::vector<sn_idx>&> voters_ = boost::none;

  /// whether to actually remove SN from the list
  bool commit_;

  public:
    dereg_tx_builder(linear_chain_generator& gen, const crypto::public_key& pk, bool commit)
      : gen_(gen), pk_(pk), commit_(commit)
    {}

    dereg_tx_builder&& with_height(uint64_t height) {
      height_ = height;
      return std::move(*this);
    }

    dereg_tx_builder&& with_fee(uint64_t fee) {
      fee_ = fee;
      return std::move(*this);
    }

    dereg_tx_builder&& with_voters(const std::vector<sn_idx>& voters)
    {
      voters_ = voters;
      return std::move(*this);
    }

    cryptonote::transaction build()
    {
      const auto height = height_ ? *height_ : gen_.height();
      const auto voters = voters_ ? *voters_ : gen_.get_quorum_idxs(height).voters;
      return gen_.create_deregister_tx(pk_, height, voters, fee_.value_or(0), commit_);
    }

};

inline cryptonote::difficulty_type get_test_difficulty() {return 1;}
void fill_nonce(cryptonote::block& blk, const cryptonote::difficulty_type& diffic, uint64_t height);

crypto::public_key get_output_key(const cryptonote::keypair& txkey, const cryptonote::account_public_address& addr, size_t output_index);

bool construct_miner_tx_manually(size_t height, uint64_t already_generated_coins,
                                 const cryptonote::account_public_address& miner_address, cryptonote::transaction& tx,
                                 uint64_t fee, cryptonote::keypair* p_txkey = 0);

cryptonote::transaction construct_tx_with_fee(std::vector<test_event_entry>& events, const cryptonote::block& blk_head,
                                            const cryptonote::account_base& acc_from, const cryptonote::account_base& acc_to,
                                            uint64_t amount, uint64_t fee);

void get_confirmed_txs(const std::vector<cryptonote::block>& blockchain, const map_hash2tx_t& mtx, map_hash2tx_t& confirmed_txs);
bool find_block_chain(const std::vector<test_event_entry>& events, std::vector<cryptonote::block>& blockchain, map_hash2tx_t& mtx, const crypto::hash& head);

void fill_tx_sources_and_multi_destinations(const std::vector<test_event_entry>& events, const cryptonote::block& blk_head,
                                            const cryptonote::account_base& from, const cryptonote::account_base& to,
                                            uint64_t const *amount, int num_amounts, uint64_t fee, size_t nmix,
                                            std::vector<cryptonote::tx_source_entry>& sources,
                                            std::vector<cryptonote::tx_destination_entry>& destinations, uint64_t *change_amount = nullptr);

void fill_tx_sources_and_destinations(const std::vector<test_event_entry>& events, const cryptonote::block& blk_head,
                                      const cryptonote::account_base& from, const cryptonote::account_base& to,
                                      uint64_t amount, uint64_t fee, size_t nmix,
                                      std::vector<cryptonote::tx_source_entry>& sources,
                                      std::vector<cryptonote::tx_destination_entry>& destinations, uint64_t *change_amount = nullptr);

class TxBuilder {

  /// required fields
  const std::vector<test_event_entry>& m_events;
  const size_t m_hf_version;
  cryptonote::transaction& m_tx;
  const cryptonote::block& m_head;
  const cryptonote::account_base& m_from;
  const cryptonote::account_base& m_to;
  uint64_t m_amount;
  uint64_t m_fee;
  uint64_t m_unlock_time;
  std::vector<uint8_t> m_extra;

  /// optional fields
  bool m_per_output_unlock = false;
  bool m_is_staking = false;

  /// this makes sure we didn't forget to build it
  bool m_finished = false;

public:
  TxBuilder(const std::vector<test_event_entry>& events,
            cryptonote::transaction& tx,
            const cryptonote::block& head,
            const cryptonote::account_base& from,
            const cryptonote::account_base& to,
            uint64_t amount,
            uint8_t hf_version)
    : m_events(events)
    , m_tx(tx)
    , m_head(head)
    , m_from(from)
    , m_to(to)
    , m_amount(amount)
    , m_hf_version(hf_version)
    , m_fee(TESTS_DEFAULT_FEE)
    , m_unlock_time(0)
  {}

  TxBuilder&& with_fee(uint64_t fee) {
    m_fee = fee;
    return std::move(*this);
  }

  TxBuilder&& with_extra(const std::vector<uint8_t>& extra) {
    m_extra = extra;
    return std::move(*this);
  }

  TxBuilder&& is_staking(bool val) {
    m_is_staking = val;
    return std::move(*this);
  }

  TxBuilder&& with_unlock_time(uint64_t val) {
    m_unlock_time = val;
    return std::move(*this);
  }

  TxBuilder&& with_per_output_unlock(bool val) {
    m_per_output_unlock = val;
    return std::move(*this);
  }

  ~TxBuilder() {
    if (!m_finished) {
      std::cerr << "Tx building not finished\n";
      abort();
    }
  }

  bool build()
  {
    m_finished = true;

    std::vector<cryptonote::tx_source_entry> sources;
    std::vector<cryptonote::tx_destination_entry> destinations;
    uint64_t change_amount;

    const auto nmix = 9;
    fill_tx_sources_and_destinations(
      m_events, m_head, m_from, m_to, m_amount, m_fee, nmix, sources, destinations, &change_amount);

    const bool is_subaddr = false;

    cryptonote::tx_destination_entry change_addr{ change_amount, m_from.get_keys().m_account_address, is_subaddr };

    return cryptonote::construct_tx(
      m_from.get_keys(), sources, destinations, change_addr, m_extra, m_tx, m_unlock_time, m_hf_version, m_is_staking);
  }
};

/// Get the amount transferred to `account` in `tx` as output `i`
uint64_t get_amount(const cryptonote::account_base& account, const cryptonote::transaction& tx, int i);
uint64_t get_balance(const cryptonote::account_base& addr, const std::vector<cryptonote::block>& blockchain, const map_hash2tx_t& mtx);
uint64_t get_unlocked_balance(const cryptonote::account_base& addr, const std::vector<cryptonote::block>& blockchain, const map_hash2tx_t& mtx);

//--------------------------------------------------------------------------
template<class t_test_class>
auto do_check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool tx_added, size_t event_index, const cryptonote::transaction& tx, t_test_class& validator, int)
  -> decltype(validator.check_tx_verification_context(tvc, tx_added, event_index, tx))
{
  return validator.check_tx_verification_context(tvc, tx_added, event_index, tx);
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool do_check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool tx_added, size_t /*event_index*/, const cryptonote::transaction& /*tx*/, t_test_class&, long)
{
  // Default block verification context check
  if (tvc.m_verifivation_failed)
    throw std::runtime_error("Transaction verification failed");
  return true;
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool tx_added, size_t event_index, const cryptonote::transaction& tx, t_test_class& validator)
{
  // SFINAE in action
  return do_check_tx_verification_context(tvc, tx_added, event_index, tx, validator, 0);
}
//--------------------------------------------------------------------------
template<class t_test_class>
auto do_check_tx_verification_context(const std::vector<cryptonote::tx_verification_context>& tvcs, size_t tx_added, size_t event_index, const std::vector<cryptonote::transaction>& txs, t_test_class& validator, int)
  -> decltype(validator.check_tx_verification_context(tvcs, tx_added, event_index, txs))
{
  return validator.check_tx_verification_context(tvcs, tx_added, event_index, txs);
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool do_check_tx_verification_context(const std::vector<cryptonote::tx_verification_context>& tvcs, size_t tx_added, size_t /*event_index*/, const std::vector<cryptonote::transaction>& /*txs*/, t_test_class&, long)
{
  // Default block verification context check
  for (const cryptonote::tx_verification_context &tvc: tvcs)
    if (tvc.m_verifivation_failed)
      throw std::runtime_error("Transaction verification failed");
  return true;
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool check_tx_verification_context(const std::vector<cryptonote::tx_verification_context>& tvcs, size_t tx_added, size_t event_index, const std::vector<cryptonote::transaction>& txs, t_test_class& validator)
{
  // SFINAE in action
  return do_check_tx_verification_context(tvcs, tx_added, event_index, txs, validator, 0);
}
//--------------------------------------------------------------------------
template<class t_test_class>
auto do_check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_index, const cryptonote::block& blk, t_test_class& validator, int)
  -> decltype(validator.check_block_verification_context(bvc, event_index, blk))
{
  return validator.check_block_verification_context(bvc, event_index, blk);
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool do_check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t /*event_index*/, const cryptonote::block& /*blk*/, t_test_class&, long)
{
  // Default block verification context check
  if (bvc.m_verifivation_failed)
    throw std::runtime_error("Block verification failed");
  return true;
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_index, const cryptonote::block& blk, t_test_class& validator)
{
  // SFINAE in action
  return do_check_block_verification_context(bvc, event_index, blk, validator, 0);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
template<class t_test_class>
struct push_core_event_visitor: public boost::static_visitor<bool>
{
private:
  cryptonote::core& m_c;
  const std::vector<test_event_entry>& m_events;
  t_test_class& m_validator;
  size_t m_ev_index;

  bool m_txs_keeped_by_block;

public:
  push_core_event_visitor(cryptonote::core& c, const std::vector<test_event_entry>& events, t_test_class& validator)
    : m_c(c)
    , m_events(events)
    , m_validator(validator)
    , m_ev_index(0)
    , m_txs_keeped_by_block(false)
  {
  }

  void event_index(size_t ev_index)
  {
    m_ev_index = ev_index;
  }

  bool operator()(const event_visitor_settings& settings)
  {
    log_event("event_visitor_settings");

    if (settings.valid_mask & event_visitor_settings::set_txs_keeped_by_block)
    {
      m_txs_keeped_by_block = settings.txs_keeped_by_block;
    }

    return true;
  }

  bool operator()(const cryptonote::transaction& tx) const
  {
    log_event("cryptonote::transaction");

    cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    size_t pool_size = m_c.get_pool_transactions_count();
    m_c.handle_incoming_tx(t_serializable_object_to_blob(tx), tvc, m_txs_keeped_by_block, false, false);
    bool tx_added = pool_size + 1 == m_c.get_pool_transactions_count();
    bool r = check_tx_verification_context(tvc, tx_added, m_ev_index, tx, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "tx verification context check failed");
    return true;
  }

  bool operator()(const std::vector<cryptonote::transaction>& txs) const
  {
    log_event("cryptonote::transaction");

    std::vector<cryptonote::blobdata> tx_blobs;
    std::vector<cryptonote::tx_verification_context> tvcs;
     cryptonote::tx_verification_context tvc0 = AUTO_VAL_INIT(tvc0);
    for (const auto &tx: txs)
    {
      tx_blobs.push_back(t_serializable_object_to_blob(tx));
      tvcs.push_back(tvc0);
    }
    size_t pool_size = m_c.get_pool_transactions_count();
    m_c.handle_incoming_txs(tx_blobs, tvcs, m_txs_keeped_by_block, false, false);
    size_t tx_added = m_c.get_pool_transactions_count() - pool_size;
    bool r = check_tx_verification_context(tvcs, tx_added, m_ev_index, txs, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "tx verification context check failed");
    return true;
  }

  bool operator()(const cryptonote::block& b) const
  {
    log_event("cryptonote::block");

    cryptonote::block_verification_context bvc = AUTO_VAL_INIT(bvc);
    m_c.handle_incoming_block(t_serializable_object_to_blob(b), bvc);
    bool r = check_block_verification_context(bvc, m_ev_index, b, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "block verification context check failed");
    return r;
  }

  bool operator()(const callback_entry& cb) const
  {
    log_event(std::string("callback_entry ") + cb.callback_name);
    return m_validator.verify(cb.callback_name, m_c, m_ev_index, m_events);
  }

  bool operator()(const cryptonote::account_base& ab) const
  {
    log_event("cryptonote::account_base");
    return true;
  }

  bool operator()(const serialized_block& sr_block) const
  {
    log_event("serialized_block");

    cryptonote::block_verification_context bvc = AUTO_VAL_INIT(bvc);
    m_c.handle_incoming_block(sr_block.data, bvc);

    cryptonote::block blk;
    std::stringstream ss;
    ss << sr_block.data;
    binary_archive<false> ba(ss);
    ::serialization::serialize(ba, blk);
    if (!ss.good())
    {
      blk = cryptonote::block();
    }
    bool r = check_block_verification_context(bvc, m_ev_index, blk, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "block verification context check failed");
    return true;
  }

  bool operator()(const serialized_transaction& sr_tx) const
  {
    log_event("serialized_transaction");

    cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    size_t pool_size = m_c.get_pool_transactions_count();
    m_c.handle_incoming_tx(sr_tx.data, tvc, m_txs_keeped_by_block, false, false);
    bool tx_added = pool_size + 1 == m_c.get_pool_transactions_count();

    cryptonote::transaction tx;
    std::stringstream ss;
    ss << sr_tx.data;
    binary_archive<false> ba(ss);
    ::serialization::serialize(ba, tx);
    if (!ss.good())
    {
      tx = cryptonote::transaction();
    }

    bool r = check_tx_verification_context(tvc, tx_added, m_ev_index, tx, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "transaction verification context check failed");
    return true;
  }

private:
  void log_event(const std::string& event_type) const
  {
    MGINFO_YELLOW("=== EVENT # " << m_ev_index << ": " << event_type);
  }
};
//--------------------------------------------------------------------------
template<class t_test_class>
inline bool replay_events_through_core(cryptonote::core& cr, const std::vector<test_event_entry>& events, t_test_class& validator)
{
  TRY_ENTRY();

  //init core here

  CHECK_AND_ASSERT_MES(typeid(cryptonote::block) == events[0].type(), false, "First event must be genesis block creation");
  cr.set_genesis_block(boost::get<cryptonote::block>(events[0]));

  bool r = true;
  push_core_event_visitor<t_test_class> visitor(cr, events, validator);
  for(size_t i = 1; i < events.size() && r; ++i)
  {
    visitor.event_index(i);
    r = boost::apply_visitor(visitor, events[i]);
  }

  return r;

  CATCH_ENTRY_L0("replay_events_through_core", false);
}
//--------------------------------------------------------------------------
template<typename t_test_class>
struct get_test_options {
  const std::vector<std::pair<uint8_t, uint64_t>> hard_forks = {{7, 0}};
  const cryptonote::test_options test_options = {
    hard_forks
  };
};

//--------------------------------------------------------------------------
template<class t_test_class>
inline bool do_replay_events(std::vector<test_event_entry>& events)
{
  boost::program_options::options_description desc("Allowed options");
  cryptonote::core::init_options(desc);
  boost::program_options::variables_map vm;
  bool r = command_line::handle_error_helper(desc, [&]()
  {
    boost::program_options::store(boost::program_options::basic_parsed_options<char>(&desc), vm);
    boost::program_options::notify(vm);
    return true;
  });
  if (!r)
    return false;

  cryptonote::cryptonote_protocol_stub pr; //TODO: stub only for this kind of test, make real validation of relayed objects
  cryptonote::core c(&pr);
  // FIXME: make sure that vm has arg_testnet_on set to true or false if
  // this test needs for it to be so.
  get_test_options<t_test_class> gto;
  if (!c.init(vm, &gto.test_options))
  {
    MERROR("Failed to init core");
    return false;
  }
  c.get_blockchain_storage().get_db().set_batch_transactions(true);

  // start with a clean pool
  std::vector<crypto::hash> pool_txs;
  if (!c.get_pool_transaction_hashes(pool_txs))
  {
    MERROR("Failed to flush txpool");
    return false;
  }
  c.get_blockchain_storage().flush_txes_from_pool(pool_txs);

  t_test_class validator;
  bool ret = replay_events_through_core<t_test_class>(c, events, validator);
  return ret;
}
//--------------------------------------------------------------------------
template<class t_test_class>
inline bool do_replay_file(const std::string& filename)
{
  std::vector<test_event_entry> events;
  if (!tools::unserialize_obj_from_file(events, filename))
  {
    MERROR("Failed to deserialize data from file: ");
    return false;
  }
  return do_replay_events<t_test_class>(events);
}

//--------------------------------------------------------------------------
#define GENERATE_ACCOUNT(account) \
    cryptonote::account_base account; \
    account.generate();

#define GENERATE_MULTISIG_ACCOUNT(account, threshold, total) \
    CHECK_AND_ASSERT_MES(threshold >= 2 && threshold <= total, false, "Invalid multisig scheme"); \
    std::vector<cryptonote::account_base> account(total); \
    do \
    { \
      for (size_t msidx = 0; msidx < total; ++msidx) \
        account[msidx].generate(); \
      make_multisig_accounts(account, threshold); \
    } while(0)

#define MAKE_ACCOUNT(VEC_EVENTS, account) \
  cryptonote::account_base account; \
  account.generate(); \
  VEC_EVENTS.push_back(account);

#define DO_CALLBACK(VEC_EVENTS, CB_NAME) \
{ \
  callback_entry CALLBACK_ENTRY; \
  CALLBACK_ENTRY.callback_name = CB_NAME; \
  VEC_EVENTS.push_back(CALLBACK_ENTRY); \
}

#define REGISTER_CALLBACK(CB_NAME, CLBACK) \
  register_callback(CB_NAME, boost::bind(&CLBACK, this, _1, _2, _3));

#define REGISTER_CALLBACK_METHOD(CLASS, METHOD) \
  register_callback(#METHOD, boost::bind(&CLASS::METHOD, this, _1, _2, _3));

#define MAKE_GENESIS_BLOCK(VEC_EVENTS, BLK_NAME, MINER_ACC, TS)                       \
  test_generator generator;                                               \
  cryptonote::block BLK_NAME;                                                           \
  generator.construct_block(BLK_NAME, MINER_ACC, TS);                                 \
  VEC_EVENTS.push_back(BLK_NAME);

/// TODO: use hf_ver from test options
#define MAKE_GENESIS_BLOCK_WITH_HF_VERSION(VEC_EVENTS, BLK_NAME, MINER_ACC, TS, HF_VER)                       \
  test_generator generator(HF_VER);                                               \
  cryptonote::block BLK_NAME;                                                           \
  generator.construct_block(BLK_NAME, MINER_ACC, TS);                                 \
  VEC_EVENTS.push_back(BLK_NAME);

#define MAKE_NEXT_BLOCK(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC)                  \
  cryptonote::block BLK_NAME;                                                           \
  generator.construct_block(BLK_NAME, PREV_BLOCK, MINER_ACC);                         \
  VEC_EVENTS.push_back(BLK_NAME);

#define MAKE_NEXT_BLOCK_V2(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, WINNER, SN_INFO)            \
  cryptonote::block BLK_NAME;                                                           \
  generator.construct_block(BLK_NAME, PREV_BLOCK, MINER_ACC, {}, WINNER, SN_INFO);                   \
  VEC_EVENTS.push_back(BLK_NAME);

#define MAKE_NEXT_BLOCK_TX1(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, TX1)         \
  cryptonote::block BLK_NAME;                                                           \
  {                                                                                   \
    std::list<cryptonote::transaction> tx_list;                                         \
    tx_list.push_back(TX1);                                                           \
    generator.construct_block(BLK_NAME, PREV_BLOCK, MINER_ACC, tx_list);              \
  }                                                                                   \
  VEC_EVENTS.push_back(BLK_NAME);

#define MAKE_NEXT_BLOCK_TX_LIST(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, TXLIST)  \
  cryptonote::block BLK_NAME;                                                           \
  generator.construct_block(BLK_NAME, PREV_BLOCK, MINER_ACC, TXLIST);                 \
  VEC_EVENTS.push_back(BLK_NAME);

#define REWIND_BLOCKS_N(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, COUNT)           \
  cryptonote::block BLK_NAME;                                                           \
  {                                                                                   \
    cryptonote::block blk_last = PREV_BLOCK;                                            \
    for (size_t i = 0; i < COUNT; ++i)                                                \
    {                                                                                 \
      MAKE_NEXT_BLOCK(VEC_EVENTS, blk, blk_last, MINER_ACC);                          \
      blk_last = blk;                                                                 \
    }                                                                                 \
    BLK_NAME = blk_last;                                                              \
  }

#define REWIND_BLOCKS_N_V2(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, COUNT, WINNER, SN_INFO) \
  cryptonote::block BLK_NAME;                                                           \
  {                                                                                   \
    cryptonote::block blk_last = PREV_BLOCK;                                            \
    for (size_t i = 0; i < COUNT; ++i)                                                \
    {                                                                                 \
      MAKE_NEXT_BLOCK_V2(VEC_EVENTS, blk, blk_last, MINER_ACC, WINNER, SN_INFO);      \
      blk_last = blk;                                                                 \
    }                                                                                 \
    BLK_NAME = blk_last;                                                              \
  }

#define REWIND_BLOCKS(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC) REWIND_BLOCKS_N(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW)

cryptonote::transaction make_registration_tx(std::vector<test_event_entry>& events,
                                             const cryptonote::account_base& account,
                                             const cryptonote::keypair& service_node_keys,
                                             uint64_t operator_cut,
                                             const std::vector<cryptonote::account_public_address>& addresses,
                                             const std::vector<uint64_t>& portions,
                                             const cryptonote::block& head,
                                             uint8_t hf_version);

cryptonote::transaction make_default_registration_tx(std::vector<test_event_entry>& events,
                                                     const cryptonote::account_base& account,
                                                     const cryptonote::keypair& service_node_keys,
                                                     const cryptonote::block& head,
                                                     uint8_t hf_version);


cryptonote::transaction make_deregistration_tx(const std::vector<test_event_entry>& events,
                                               const cryptonote::account_base& account,
                                               const cryptonote::block& head,
                                               const cryptonote::tx_extra_service_node_deregister& deregister, uint8_t hf_version, uint64_t fee);

// NOTE(loki): These macros assume hardfork version 7 and are from the old Monero testing code
#define MAKE_TX_MIX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, NMIX, HEAD)                       \
  cryptonote::transaction TX_NAME;                                                           \
  TxBuilder(VEC_EVENTS, TX_NAME, HEAD, FROM, TO, AMOUNT, cryptonote::network_version_7).build(); \
  VEC_EVENTS.push_back(TX_NAME);

#define MAKE_TX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, HEAD) MAKE_TX_MIX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, 9, HEAD)

#define MAKE_TX_MIX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, NMIX, HEAD)             \
  {                                                                                      \
    cryptonote::transaction t;                                                             \
    TxBuilder(VEC_EVENTS, t, HEAD, FROM, TO, AMOUNT, cryptonote::network_version_7).build(); \
    SET_NAME.push_back(t);                                                               \
    VEC_EVENTS.push_back(t);                                                             \
  }

#define MAKE_TX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD) MAKE_TX_MIX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, 9, HEAD)


#define MAKE_TX_LIST_START(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD) \
    std::list<cryptonote::transaction> SET_NAME; \
    MAKE_TX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD);

#define MAKE_MINER_TX_MANUALLY(TX, BLK)         \
  transaction TX;                     \
  if (!construct_miner_tx(get_block_height(BLK)+1, 0, generator.get_already_generated_coins(BLK), \
    0, 0, miner_account.get_keys().m_account_address, TX, {}, 7)) \
    return false;

#define SET_EVENT_VISITOR_SETT(VEC_EVENTS, SETT, VAL) VEC_EVENTS.push_back(event_visitor_settings(SETT, VAL));

#define GENERATE(filename, genclass) \
    { \
        std::vector<test_event_entry> events; \
        genclass g; \
        g.generate(events); \
        if (!tools::serialize_obj_to_file(events, filename)) \
        { \
            MERROR("Failed to serialize data to file: " << filename); \
            throw std::runtime_error("Failed to serialize data to file"); \
        } \
    }


#define PLAY(filename, genclass) \
    if(!do_replay_file<genclass>(filename)) \
    { \
      MERROR("Failed to pass test : " << #genclass); \
      return 1; \
    }

#define GENERATE_AND_PLAY(genclass)                                                                        \
  if (list_tests)                                                                                          \
    std::cout << #genclass << std::endl;                                                                   \
  else if (filter.empty() || boost::regex_match(std::string(#genclass), match, boost::regex(filter)))      \
  {                                                                                                        \
    std::vector<test_event_entry> events;                                                                  \
    ++tests_count;                                                                                         \
    bool generated = false;                                                                                \
    try                                                                                                    \
    {                                                                                                      \
      genclass g;                                                                                          \
      generated = g.generate(events);;                                                                     \
    }                                                                                                      \
    catch (const std::exception& ex)                                                                       \
    {                                                                                                      \
      MERROR(#genclass << " generation failed: what=" << ex.what());                                       \
    }                                                                                                      \
    catch (...)                                                                                            \
    {                                                                                                      \
      MERROR(#genclass << " generation failed: generic exception");                                        \
    }                                                                                                      \
    if (generated && do_replay_events< genclass >(events))                                                 \
    {                                                                                                      \
      MGINFO_GREEN("#TEST# Succeeded " << #genclass);                                                      \
    }                                                                                                      \
    else                                                                                                   \
    {                                                                                                      \
      MERROR("#TEST# Failed " << #genclass);                                                               \
      failed_tests.push_back(#genclass);                                                                   \
    }                                                                                                      \
  }

#define CALL_TEST(test_name, function)                                                                     \
  {                                                                                                        \
    if(!function())                                                                                        \
    {                                                                                                      \
      MERROR("#TEST# Failed " << test_name);                                                               \
      return 1;                                                                                            \
    }                                                                                                      \
    else                                                                                                   \
    {                                                                                                      \
      MGINFO_GREEN("#TEST# Succeeded " << test_name);                                                      \
    }                                                                                                      \
  }

#define QUOTEME(x) #x
#define DEFINE_TESTS_ERROR_CONTEXT(text) const char* perr_context = text;
#define CHECK_TEST_CONDITION(cond) CHECK_AND_ASSERT_MES(cond, false, "[" << perr_context << "] failed: \"" << QUOTEME(cond) << "\"")
#define CHECK_EQ(v1, v2) CHECK_AND_ASSERT_MES(v1 == v2, false, "[" << perr_context << "] failed: \"" << QUOTEME(v1) << " == " << QUOTEME(v2) << "\", " << v1 << " != " << v2)
#define CHECK_NOT_EQ(v1, v2) CHECK_AND_ASSERT_MES(!(v1 == v2), false, "[" << perr_context << "] failed: \"" << QUOTEME(v1) << " != " << QUOTEME(v2) << "\", " << v1 << " == " << v2)
#define MK_COINS(amount) (UINT64_C(amount) * COIN)
