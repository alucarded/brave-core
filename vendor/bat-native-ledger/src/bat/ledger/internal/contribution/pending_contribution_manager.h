/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_PENDING_CONTRIBUTION_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_PENDING_CONTRIBUTION_MANAGER_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/future.h"
#include "bat/ledger/internal/core/future_cache.h"

namespace ledger {

enum class PendingContributionType { kOneTime, kRecurring };

class PendingContributionManager : public BATLedgerContext::Object {
 public:
  static const char kContextKey[];

  static constexpr base::TimeDelta kExpiresAfter = base::Days(90);

  PendingContributionManager();

  ~PendingContributionManager() override;

  Future<bool> ProcessPendingContributions();

  Future<bool> DeletePendingContribution(int64_t contribution_id);

  Future<bool> ClearPendingContributions();

  struct AddInfo {
    PendingContributionType type;
    std::string publisher_key;
    double amount;
  };

  Future<bool> AddPendingContribution(PendingContributionType type,
                                      const std::string& publisher_key,
                                      double amount);

  Future<bool> AddPendingContributions(const std::vector<AddInfo>& list);

 private:
  FutureCache<bool> process_cache_;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CONTRIBUTION_PENDING_CONTRIBUTION_MANAGER_H_
