/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { PaymentStatusView } from '../payment_status_view'

import * as style from './pending_rewards_view.style'

interface Props {
  amount: number
  nextPaymentDate: number
}

export function PendingRewardsView (props: Props) {
  return (
    <style.root>
      <PaymentStatusView
        amount={props.amount}
        nextPaymentDate={props.nextPaymentDate}
      />
    </style.root>
  )
}
