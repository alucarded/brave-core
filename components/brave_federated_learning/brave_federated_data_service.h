/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_DATA_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_DATA_SERVICE_H_

#include <map>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequence_bound.h"
#include "sql/database.h"
#include "url/gurl.h"

namespace brave {

namespace federated_learning {
class AdNotificationTimingDataStore;
}

class BraveFederatedDataService {
 public:
  explicit BraveFederatedDataService(const base::FilePath& base_database_path);
  ~BraveFederatedDataService();

  BraveFederatedDataService(const BraveFederatedDataService&) = delete;
  BraveFederatedDataService& operator=(const BraveFederatedDataService&) =
      delete;

  void Init();

  void PurgeTaskDataStore(const std::string& task_name);
  bool PurgeDataService();

 private:
  void EnforceRetentionPolicyOnDataStores();
  void OnInitComplete(bool success);

  base::FilePath base_database_path_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::SequenceBound<federated_learning::AdNotificationTimingDataStore>
      ad_notification_timing_ds_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_DATA_SERVICE_H_
