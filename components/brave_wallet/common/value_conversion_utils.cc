/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/guid.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"

namespace brave_wallet {

absl::optional<mojom::EthereumChain> ValueToEthereumChain(
    const base::Value& value) {
  mojom::EthereumChain chain;
  const base::DictionaryValue* params_dict = nullptr;
  if (!value.GetAsDictionary(&params_dict) || !params_dict)
    return absl::nullopt;

  const std::string* chain_id = params_dict->FindStringKey("chainId");
  if (!chain_id) {
    return absl::nullopt;
  }
  chain.chain_id = *chain_id;

  const std::string* chain_name = params_dict->FindStringKey("chainName");
  if (chain_name) {
    chain.chain_name = *chain_name;
  }

  const base::Value* explorerUrlsListValue =
      params_dict->FindListKey("blockExplorerUrls");
  if (explorerUrlsListValue) {
    for (const auto& entry : explorerUrlsListValue->GetList())
      chain.block_explorer_urls.push_back(entry.GetString());
  }

  const base::Value* iconUrlsValue = params_dict->FindListKey("iconUrls");
  if (iconUrlsValue) {
    for (const auto& entry : iconUrlsValue->GetList())
      chain.icon_urls.push_back(entry.GetString());
  }

  const base::Value* rpcUrlsValue = params_dict->FindListKey("rpcUrls");
  if (rpcUrlsValue) {
    for (const auto& entry : rpcUrlsValue->GetList())
      chain.rpc_urls.push_back(entry.GetString());
  }
  const base::Value* nativeCurrencyValue =
      params_dict->FindDictKey("nativeCurrency");
  chain.decimals = 0;
  if (nativeCurrencyValue) {
    const std::string* symbol_name = nativeCurrencyValue->FindStringKey("name");
    if (symbol_name) {
      chain.symbol_name = *symbol_name;
    }
    const std::string* symbol = nativeCurrencyValue->FindStringKey("symbol");
    if (symbol) {
      chain.symbol = *symbol;
    }
    absl::optional<int> decimals = nativeCurrencyValue->FindIntKey("decimals");
    if (decimals) {
      chain.decimals = decimals.value();
    }
  }

  chain.is_eip1559 = params_dict->FindBoolKey("is_eip1559").value_or(false);
  return chain;
}

base::Value EthereumChainToValue(const mojom::EthereumChainPtr& chain) {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("chainId", chain->chain_id);
  dict.SetStringKey("chainName", chain->chain_name);
  dict.SetBoolKey("is_eip1559", chain->is_eip1559);

  base::ListValue blockExplorerUrlsValue;
  if (!chain->block_explorer_urls.empty()) {
    for (const auto& url : chain->block_explorer_urls) {
      blockExplorerUrlsValue.Append(url);
    }
  }
  dict.SetKey("blockExplorerUrls", std::move(blockExplorerUrlsValue));

  base::ListValue iconUrlsValue;
  if (!chain->icon_urls.empty()) {
    for (const auto& url : chain->icon_urls) {
      iconUrlsValue.Append(url);
    }
  }
  dict.SetKey("iconUrls", std::move(iconUrlsValue));

  base::ListValue rpcUrlsValue;
  for (const auto& url : chain->rpc_urls) {
    rpcUrlsValue.Append(url);
  }
  dict.SetKey("rpcUrls", std::move(rpcUrlsValue));
  base::Value currency(base::Value::Type::DICTIONARY);
  currency.SetStringKey("name", chain->symbol_name);
  currency.SetStringKey("symbol", chain->symbol);
  currency.SetIntKey("decimals", chain->decimals);
  dict.SetKey("nativeCurrency", std::move(currency));
  return dict;
}

base::Value PermissionRequestResponseToValue(const url::Origin& origin, const std::vector<std::string> accounts) {
  base::ListValue container_list;
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("id", base::GenerateGUID());

  base::ListValue context_list;
  context_list.Append("https://github.com/MetaMask/rpc-cap");
  dict.SetKey("context", std::move(context_list));

  base::ListValue caveats_list;
  base::Value caveats_obj1(base::Value::Type::DICTIONARY);
  caveats_obj1.SetStringKey("name", "primaryAccountOnly");
  caveats_obj1.SetStringKey("type", "limitResponseLength");
  caveats_obj1.SetIntKey("value", 1);
  caveats_list.Append(std::move(caveats_obj1));
  base::Value caveats_obj2(base::Value::Type::DICTIONARY);
  caveats_obj2.SetStringKey("name", "exposedAccounts");
  caveats_obj2.SetStringKey("type", "filterResponse");
  base::ListValue filter_response_list;
  for (auto account: accounts) {
    filter_response_list.Append(base::Value(account));
  }
  caveats_obj2.SetKey("value", std::move(filter_response_list));
  caveats_list.Append(std::move(caveats_obj2));
  dict.SetKey("caveats", std::move(caveats_list));

  dict.SetDoubleKey("date",  base::Time::Now().ToJsTime());
  dict.SetStringKey("invoker", origin.Serialize());
  dict.SetStringKey("parentCapability", "eth_accounts");
  container_list.Append(std::move(dict));
  return container_list;
}

}  // namespace brave_wallet
