/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_CALLBACK_ADAPTER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_CALLBACK_ADAPTER_H_

#include <utility>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

class CallbackAdapter {
 public:
  CallbackAdapter();
  ~CallbackAdapter();

  template <typename F>
  inline auto operator()(F fn) {
    return base::BindOnce(&CallbackWrapper<F>::template BindAdapter<F>,
                          weak_factory_.GetWeakPtr(), std::move(fn));
  }

  inline static mojom::Result ResultCode(bool success) {
    return success ? mojom::Result::LEDGER_OK : mojom::Result::LEDGER_OK;
  }

 private:
  template <typename F>
  struct CallbackWrapper;

  template <typename... Args>
  struct CallbackWrapper<void(Args...)> {
    template <typename F>
    static auto BindAdapter(base::WeakPtr<CallbackAdapter> weak_ptr,
                            F fn,
                            Args... args) {
      if (weak_ptr)
        fn(std::forward<Args>(args)...);
    }
  };

  template <typename C, typename... Args>
  struct CallbackWrapper<void (C::*)(Args...)>
      : public CallbackWrapper<void(Args...)> {};

  template <typename C, typename... Args>
  struct CallbackWrapper<void (C::*)(Args...) const>
      : public CallbackWrapper<void(Args...)> {};

  template <typename... Args>
  struct CallbackWrapper<void (*)(Args...)>
      : public CallbackWrapper<void(Args...)> {};

  template <typename F>
  struct CallbackWrapper
      : public CallbackWrapper<decltype(&std::decay<F>::type::operator())> {};

  base::WeakPtrFactory<CallbackAdapter> weak_factory_{this};
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_CALLBACK_ADAPTER_H_
