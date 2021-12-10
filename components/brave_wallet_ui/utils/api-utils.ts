import * as BraveWallet from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import { EthereumChain, ERCToken } from '../constants/types'
import { ETH } from '../options/asset-options'

export const GetTokenParam = (selectedNetwork: EthereumChain, token: ERCToken): string => {
  const isEthereumNetwork = selectedNetwork.chainId === BraveWallet.MAINNET_CHAIN_ID
  return isEthereumNetwork ? token.symbol === ETH.asset.symbol ? token.symbol.toLowerCase() : token.contractAddress : token.symbol.toLowerCase()
}
