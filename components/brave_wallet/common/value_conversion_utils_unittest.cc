/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/value_conversion_utils.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

TEST(ValueConversionUtilsUnitTest, ValueToEthereumChainTest) {
  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ValueToEthereumChain(base::JSONReader::Read(R"({
      "chainId": "0x5",
      "chainName": "Goerli",
      "rpcUrls": [
        "https://goerli.infura.io/v3/INSERT_API_KEY_HERE",
        "https://second.infura.io/"
      ],
      "iconUrls": [
        "https://xdaichain.com/fake/example/url/xdai.svg",
        "https://xdaichain.com/fake/example/url/xdai.png"
      ],
      "nativeCurrency": {
        "name": "Goerli ETH",
        "symbol": "gorETH",
        "decimals": 18
      },
      "blockExplorerUrls": ["https://goerli.etherscan.io"],
      "is_eip1559": true
    })")
                                               .value());
    ASSERT_TRUE(chain);
    EXPECT_EQ("0x5", chain->chain_id);
    EXPECT_EQ("Goerli", chain->chain_name);
    EXPECT_EQ(size_t(2), chain->rpc_urls.size());
    EXPECT_EQ("https://goerli.infura.io/v3/INSERT_API_KEY_HERE",
              chain->rpc_urls.front());
    EXPECT_EQ("https://second.infura.io/", chain->rpc_urls.back());
    EXPECT_EQ("https://goerli.etherscan.io",
              chain->block_explorer_urls.front());
    EXPECT_EQ("Goerli ETH", chain->symbol_name);
    EXPECT_EQ("gorETH", chain->symbol);
    EXPECT_EQ(18, chain->decimals);
    EXPECT_EQ("https://xdaichain.com/fake/example/url/xdai.svg",
              chain->icon_urls.front());
    EXPECT_EQ("https://xdaichain.com/fake/example/url/xdai.png",
              chain->icon_urls.back());
    EXPECT_TRUE(chain->is_eip1559);
  }
  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ValueToEthereumChain(base::JSONReader::Read(R"({
      "chainId": "0x5"
    })")
                                               .value());
    ASSERT_TRUE(chain);
    EXPECT_EQ("0x5", chain->chain_id);
    ASSERT_TRUE(chain->chain_name.empty());
    ASSERT_TRUE(chain->rpc_urls.empty());
    ASSERT_TRUE(chain->icon_urls.empty());
    ASSERT_TRUE(chain->block_explorer_urls.empty());
    ASSERT_TRUE(chain->symbol_name.empty());
    ASSERT_TRUE(chain->symbol.empty());
    ASSERT_FALSE(chain->is_eip1559);
    EXPECT_EQ(chain->decimals, 0);
  }

  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ValueToEthereumChain(base::JSONReader::Read(R"({
    })")
                                               .value());
    ASSERT_FALSE(chain);
  }
  {
    absl::optional<brave_wallet::mojom::EthereumChain> chain =
        brave_wallet::ValueToEthereumChain(base::JSONReader::Read(R"([
          ])")
                                               .value());
    ASSERT_FALSE(chain);
  }
}

TEST(ValueConversionUtilsUnitTest, EthereumChainToValueTest) {
  brave_wallet::mojom::EthereumChain chain(
      "chain_id", "chain_name", {"https://url1.com"}, {"https://url1.com"},
      {"https://url1.com"}, "symbol_name", "symbol", 11, true);
  base::Value value = brave_wallet::EthereumChainToValue(chain.Clone());
  EXPECT_EQ(*value.FindStringKey("chainId"), chain.chain_id);
  EXPECT_EQ(*value.FindStringKey("chainName"), chain.chain_name);
  EXPECT_EQ(*value.FindStringPath("nativeCurrency.name"), chain.symbol_name);
  EXPECT_EQ(*value.FindStringPath("nativeCurrency.symbol"), chain.symbol);
  EXPECT_EQ(*value.FindIntPath("nativeCurrency.decimals"), chain.decimals);
  EXPECT_EQ(value.FindBoolKey("is_eip1559").value(), true);
  for (const auto& entry : value.FindListKey("rpcUrls")->GetList()) {
    ASSERT_NE(std::find(chain.rpc_urls.begin(), chain.rpc_urls.end(),
                        entry.GetString()),
              chain.rpc_urls.end());
  }

  for (const auto& entry : value.FindListKey("iconUrls")->GetList()) {
    ASSERT_NE(std::find(chain.icon_urls.begin(), chain.icon_urls.end(),
                        entry.GetString()),
              chain.icon_urls.end());
  }
  auto* blocked_urls = value.FindListKey("blockExplorerUrls");
  for (const auto& entry : blocked_urls->GetList()) {
    ASSERT_NE(std::find(chain.block_explorer_urls.begin(),
                        chain.block_explorer_urls.end(), entry.GetString()),
              chain.block_explorer_urls.end());
  }

  auto result = brave_wallet::ValueToEthereumChain(value);
  ASSERT_TRUE(result->Equals(chain));
}

TEST(ValueConversionUtilsUnitTest, PermissionRequestResponseToValue) {
  url::Origin origin = url::Origin::Create(GURL("https://brave.com"));
  std::vector<std::string> accounts { "0xA99D71De40D67394eBe68e4D0265cA6C9D421029" };
  base::Value value = PermissionRequestResponseToValue(origin, accounts);

  base::ListValue* list_value;
  ASSERT_TRUE(value.GetAsList(&list_value));
  ASSERT_EQ(list_value->size(), 1);

  //const auto &obj1 = value.GetList();

  //std::string* id = obj1.FindStringKey("id");
  //ASSERT_NE(id, nullptr);

  // "[{\"caveats\":[{\"name\":\"primaryAccountOnly\",\"type\":\"limitResponseLength\",\"value\":1},{\"name\":\"exposedAccounts\",\"type\":\"filterResponse\",\"value\":[\"0xA99D71De40D67394eBe68e4D0265cA6C9D421029\"]}],\"context\":[\"https://github.com/MetaMask/rpc-cap\"],\"date\":1.637594791027276e+12,\"id\":\"2485c0da-2131-4801-9918-26e8de929a29\",\"invoker\":\"https://brave.com\",\"parentCapability\":\"eth_accounts\"}]"
  //
  //
//  std::string json;
//  base::JSONWriter::Write(value, &json);
//  EXPECT_EQ(json, "hi");
}

/*
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
*/

}  // namespace brave_wallet
