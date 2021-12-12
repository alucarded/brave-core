/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contribution/pending_contribution_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "base/guid.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/delay_generator.h"
#include "bat/ledger/internal/core/sql_store.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {

namespace {

constexpr base::TimeDelta kPendingExpiration = base::Days(90);

// TODO: We need to clean up this table. "viewing_id" is not used. "added_date"
// should be "created_at". "publisher_id" should be "publisher_key". We should
// also add the original publisher status (but we don't want to use the old
// status values). We could probably get rid of the processed_publisher table
// if we only notify the observer when a publisher transitions from not verified
// to verified (at all).

class AddJob : public BATLedgerJob<bool> {
 public:
  void Start(PendingContributionType type,
             const std::string& publisher_key,
             double amount) {
    if (publisher_key.empty() || amount <= 0)
      return Complete(false);

    static const char kSQL[] = R"sql(
      INSERT OR REPLACE INTO pending_contribution
        (publisher_id, amount, added_date, type)
      VALUES (?, ?, ?, ?)
    )sql";

    double created_at = base::Time::Now().ToDoubleT();
    int64_t type_id = static_cast<int64_t>(mojom::RewardsType::ONE_TIME_TIP);

    if (type == PendingContributionType::kRecurring)
      type_id = static_cast<int64_t>(mojom::RewardsType::RECURRING_TIP);

    context()
        .Get<SQLStore>()
        .Run(kSQL, publisher_key, amount, created_at, type_id)
        .Then(ContinueWith(&AddJob::OnInsertComplete));
  }

 private:
  void OnInsertComplete(SQLReader reader) { Complete(reader.Succeeded()); }
};

class DeleteJob : public BATLedgerJob<bool> {
 public:
  void Start(int64_t id) {
    static const char kSQL[] = R"sql(
      DELETE FROM pending_contribution WHERE pending_contribution_id = ?
    )sql";

    context().Get<SQLStore>().Run(kSQL, id).Then(
        ContinueWith(&DeleteJob::OnDeleted));
  }

 private:
  void OnDeleted(SQLReader reader) { Complete(reader.Succeeded()); }
};

class ClearJob : public BATLedgerJob<bool> {
 public:
  void Start() {
    static const char kSQL[] = R"sql(
      DELETE FROM pending_contribution
    )sql";

    context().Get<SQLStore>().Run(kSQL).Then(
        ContinueWith(&ClearJob::OnDeleted));
  }

 private:
  void OnDeleted(SQLReader reader) { Complete(reader.Succeeded()); }
};

class ProcessJob : public BATLedgerJob<bool> {
 public:
  struct PendingInfo {
    int64_t id = 0;
    std::string publisher_key;
    std::string publisher_name;
    double amount = 0;
  };

  void Start() {
    context().LogInfo(FROM_HERE) << "Starting pending contribution processing";

    static const char kSQL[] = R"sql(
      DELETE FROM pending_contribution
      WHERE added_date < ?
    )sql";

    base::Time cutoff = base::Time::Now() - kPendingExpiration;

    context()
        .Get<SQLStore>()
        .Run(kSQL, cutoff.ToDoubleT())
        .Then(ContinueWith(&ProcessJob::OnExipiredRecordsDeleted));
  }

 private:
  void OnExipiredRecordsDeleted(SQLReader reader) {
    if (!reader.Succeeded()) {
      context().LogError(FROM_HERE) << "Unable to delete expired pending "
                                       "contributions";
      return Complete(false);
    }

    static const char kSQL[] = R"sql(
      SELECT
        pc.pending_contribution_id,
        pc.publisher_id,
        pi.name,
        pc.amount
      FROM pending_contribution as pc
      LEFT JOIN publisher_info AS pi ON pi.publisher_id = pc.publisher_id
    )sql";

    context().Get<SQLStore>().Query(kSQL).Then(
        ContinueWith(&ProcessJob::OnPendingQueryComplete));
  }

  void OnPendingQueryComplete(SQLReader reader) {
    if (!reader.Succeeded()) {
      context().LogError(FROM_HERE) << "Unable to query pending contributions";
      return Complete(false);
    }

    while (reader.Step()) {
      pending_records_.push_back(
          PendingInfo{.id = reader.ColumnInt64(0),
                      .publisher_key = reader.ColumnString(1),
                      .publisher_name = reader.ColumnString(2),
                      .amount = reader.ColumnDouble(3)});
    }

    ProcessNext();
  }

  void ProcessNext() {
    // TODO: This "is_testing" business is gross. How should we handle this
    // instead? Why is it gross? Because we have all of these random is_testing
    // things sprinkled around ad-hoc. Also, we need to avoid global variables.
    base::TimeDelta delay =
        ledger::is_testing ? base::Seconds(2) : base::Seconds(45);

    context()
        .Get<DelayGenerator>()
        .RandomDelay(FROM_HERE, delay)
        .Then(ContinueWith(&ProcessJob::OnProcessNextDelayElapsed));
  }

  void OnProcessNextDelayElapsed(bool success) {
    if (pending_records_.empty()) {
      context().LogInfo(FROM_HERE) << "Pending contribution processing "
                                   << "complete";
      context().GetLedgerImpl()->contribution()->ProcessContributionQueue();
      return Complete(true);
    }

    current_record_ = std::move(pending_records_.back());
    pending_records_.pop_back();

    context().GetLedgerImpl()->publisher()->FetchServerPublisherInfo(
        current_record_.publisher_key,
        CreateLambdaCallback(&ProcessJob::OnServerPublisherInfoFetched));
  }

  void OnServerPublisherInfoFetched(mojom::ServerPublisherInfoPtr info) {
    if (!info) {
      context().LogInfo(FROM_HERE) << "Could not obtain publisher info for "
                                   << current_record_.publisher_key;
      return ProcessNext();
    }

    switch (info->status) {
      case mojom::PublisherStatus::NOT_VERIFIED:
      case mojom::PublisherStatus::CONNECTED: {
        return ProcessNext();
      }
      default: {
        break;
      }
    }

    context().LogInfo(FROM_HERE) << "Processing pending contribution for "
                                 << current_record_.publisher_key;

    // TODO: I suppose that we need to ensure that the user can actually process
    // the contribution. We'd need to have the user's balance but we'd also need
    // to know their external wallets and things like that. Another wrinkle is
    // that while the user's balance may be sufficient for one pending tip it
    // might not be sufficient for the next one. I believe that the original
    // design was to basically wait until processing finished for each pending
    // tip before moving to the next one (hence the delay?).

    std::vector<mojom::ContributionQueuePublisherPtr> queue_publishers;
    auto publisher = mojom::ContributionQueuePublisher::New();
    publisher->publisher_key = current_record_.publisher_key;
    publisher->amount_percent = 100.0;
    queue_publishers.push_back(std::move(publisher));

    // TODO: This naming is not good. These are queue entries, not queues.

    auto queue = mojom::ContributionQueue::New();
    queue->id = base::GenerateGUID();
    queue->type = mojom::RewardsType::ONE_TIME_TIP;
    queue->amount = current_record_.amount;
    queue->partial = false;
    queue->publishers = std::move(queue_publishers);

    // TODO: We shouldn't even need this mojom business - a contribution queue
    // manager API should allow us to add with "normal" data.

    context().GetLedgerImpl()->database()->SaveContributionQueue(
        std::move(queue), CreateLambdaCallback(&ProcessJob::OnQueueSaved));
  }

  void OnQueueSaved(mojom::Result result) {
    if (result != mojom::Result::LEDGER_OK) {
      context().LogError(FROM_HERE) << "Unable to save contribution queue item "
                                    << "for pending contribution";
      return Complete(false);
    }

    context()
        .StartJob<DeleteJob>(current_record_.id)
        .Then(ContinueWith(&ProcessJob::OnPendingRecordDeleted));
  }

  void OnPendingRecordDeleted(bool success) {
    if (!success) {
      // TODO: Not sure if we should continue on or not. We can't delete the
      // queue entry we just added, can we? We need to delete the added
      // contibution queue item if we are unable to remove the pending record.
      // Actually, we should probably mark it as inactive first, and then delete
      // it later.
      return Complete(false);
    }

    context().GetLedgerClient()->OnContributeUnverifiedPublishers(
        mojom::Result::PENDING_PUBLISHER_REMOVED, current_record_.publisher_key,
        current_record_.publisher_name);

    const char kSQL[] = R"sql(
      INSERT OR IGNORE INTO processed_publisher (publisher_key) VALUES (?)
    )sql";

    context()
        .Get<SQLStore>()
        .Run(kSQL, current_record_.publisher_key)
        .Then(ContinueWith(&ProcessJob::OnProcessedPublisherInserted));
  }

  void OnProcessedPublisherInserted(SQLReader reader) {
    if (!reader.Succeeded()) {
      context().LogError(FROM_HERE) << "Failed to insert into processed "
                                       "publisher table";
    } else if (reader.Step()) {
      uint64_t inserted_rows = reader.ColumnInt64(0);
      if (inserted_rows > 0) {
        // TODO: This isn't quite right for the external wallet mismatch case.
        // The publisher could already be verified and *also* still have a
        // mismatch by the time that we get here.
        context().GetLedgerClient()->OnContributeUnverifiedPublishers(
            mojom::Result::VERIFIED_PUBLISHER, current_record_.publisher_key,
            current_record_.publisher_name);
      }
    }

    ProcessNext();
  }

  PendingInfo current_record_;
  std::vector<PendingInfo> pending_records_;
};

}  // namespace

const char PendingContributionManager::kContextKey[] =
    "pending-contribution-processor";

PendingContributionManager::PendingContributionManager() = default;

PendingContributionManager::~PendingContributionManager() = default;

Future<bool> PendingContributionManager::ProcessPendingContributions() {
  return process_cache_.GetFuture(
      [&]() { return context().StartJob<ProcessJob>(); });
}

Future<bool> PendingContributionManager::DeletePendingContribution(int64_t id) {
  return context().StartJob<DeleteJob>(id);
}

Future<bool> PendingContributionManager::ClearPendingContributions() {
  return context().StartJob<ClearJob>();
}

Future<bool> PendingContributionManager::AddPendingContribution(
    PendingContributionType type,
    const std::string& publisher_key,
    double amount) {
  return context().StartJob<AddJob>(type, publisher_key, amount);
}

}  // namespace ledger
