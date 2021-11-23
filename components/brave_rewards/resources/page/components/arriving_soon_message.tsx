/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { PaymentStatusView } from '../../shared/components/payment_status_view'

import * as style from './arriving_soon_message.style'

interface Props {
  earningsLastMonth: number
  nextPaymentDate: number
}

export function ArrivingSoonMessage (props: Props) {
  return (
    <style.root>
      <PaymentStatusView
        amount={props.earningsLastMonth}
        nextPaymentDate={props.nextPaymentDate}
      />
    </style.root>
  )
}
