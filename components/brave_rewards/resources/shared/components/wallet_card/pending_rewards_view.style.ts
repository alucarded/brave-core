/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  color: var(--brave-palette-neutral900);
  font-size: 13px;
  line-height: 19px;
  text-align: left;

  a {
    color: var(--brave-palette-blurple500);
    font-weight: 600;
    text-decoration: none;
  }

  .rewards-payment-pending {
    background: #E8F4FF;
    padding: 7px 21px;
    display: flex;

    .icon {
      color: var(--brave-palette-blue500);
      height: 16px;
      width: auto;
      vertical-align: middle;
      margin-right: 8px;
      margin-bottom: 3px;
    }
  }

  .rewards-payment-amount {
    font-weight: 600;

    .plus {
      margin-right: 2px;
    }
  }

  .rewards-payment-completed {
    background: #E7FDEA;
    padding: 10px 15px;
    display: flex;
    align-items: center;

    .icon {
      height: 32px;
      width: auto;
      vertical-align: middle;
      margin-right: 13px;
    }
  }
`
