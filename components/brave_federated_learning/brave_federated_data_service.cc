/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated_learning/brave_federated_data_service.h"

#include <map>
#include <string>
#include <utility>

#include "base/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/task_runner_util.h"
#include "base/threading/sequence_bound.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/brave_federated_learning/data_stores/ad_notification_timing_data_store.h"
#include "brave/components/brave_federated_learning/data_stores/data_store.h"
#include "sql/recovery.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave {

constexpr char kAdNotificationTaskName[] =
    "ad_notification_timing_federated_task";
constexpr char kAdNotificationTaskId[] = "0.0.1";
constexpr int kMaxNumberOfRecords = 50;
constexpr int kMaxRetentionDays = 30;

BraveFederatedDataService::BraveFederatedDataService(
    const base::FilePath& database_path)
    : base_database_path_(database_path),
      task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      ad_notification_timing_ds_(task_runner_, base_database_path_) {}

BraveFederatedDataService::~BraveFederatedDataService() {
  EnforceRetentionPolicyOnDataStores();
}

void BraveFederatedDataService::OnInitComplete(bool success) {
  if (success)
    EnforceRetentionPolicyOnDataStores();
}

void BraveFederatedDataService::Init() {
  ad_notification_timing_ds_
      .AsyncCall(&federated_learning::AdNotificationTimingDataStore::Init)
      .WithArgs(kAdNotificationTaskId, kAdNotificationTaskName,
                kMaxNumberOfRecords, kMaxRetentionDays)
      .Then(base::BindOnce(&BraveFederatedDataService::OnInitComplete,
                           base::Unretained(this)));
}

bool BraveFederatedDataService::PurgeDataService() {
  return sql::Database::Delete(base_database_path_);
}

void BraveFederatedDataService::EnforceRetentionPolicyOnDataStores() {
  ad_notification_timing_ds_.AsyncCall(
      &federated_learning::AdNotificationTimingDataStore::
          EnforceRetentionPolicy);
}

}  // namespace brave
