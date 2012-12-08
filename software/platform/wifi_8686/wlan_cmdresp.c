/** @file wlan_cmdresp.c
 * @brief This file contains the handling of command
 * responses as well as events generated by firmware.
 *
 *  Copyright Marvell International Ltd. and/or its affiliates, 2003-2007
 */
/********************************************************
 Change log:
 10/10/05: Add Doxygen format comments
 11/11/05: Add support for WMM Status change event
 12/13/05: Add Proprietary periodic sleep support
 12/23/05: Fix bug in adhoc start where the current index was
 not properly being assigned before it was used.
 01/05/06: Add kernel 2.6.x support
 01/11/06: Conditionalize new scan/join structures.
 Update assoc response handling; entire IEEE response returned
 04/06/06: Add TSPEC, queue metrics, and MSDU expiry support
 04/10/06: Add hostcmd generic API
 04/18/06: Remove old Subscrive Event and add new Subscribe Event
 implementation through generic hostcmd API
 05/04/06: Add IBSS coalescing related new hostcmd response handling
 05/08/06: Remove PermanentAddr from Adapter
 06/08/06: Remove function HandleMICFailureEvent()
 08/29/06: Add ledgpio private command
 ********************************************************/

#include	"include.h"
#define ARPHRD_ETHER 	1		/* Ethernet 10Mbps		*/
/********************************************************
 Local Variables
 ********************************************************/

/********************************************************
 Global Variables
 ********************************************************/

/********************************************************
 Local Functions
 ********************************************************/
/** 
 *  @brief This function handles the command response of get_hw_spec
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param resp	   A pointer to HostCmd_DS_COMMAND
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int wlan_ret_get_hw_spec(WlanCard *cardinfo, HostCmd_DS_COMMAND * resp)
{
	u32 i;
	HostCmd_DS_GET_HW_SPEC *hwspec = &resp->params.hwspec;
	WlanCard *card = cardinfo;
	int ret = WLAN_STATUS_SUCCESS;

	card->fwCapInfo = hwspec->fwCapInfo;

	card->RegionCode = hwspec->RegionCode;
	WlanDebug(WlanMsg,"RegionCode %x,set 0x10",card->RegionCode);
	card->RegionCode = 0x10;
	for (i = 0; i < MRVDRV_MAX_REGION_CODE; i++)
	{
		/* use the region code to search for the index */
		if (card->RegionCode == RegionCodeToIndex[i])
		{
			break;
		}
	}

	/* if it's unidentified region code, use the default (USA) */
	if (i >= MRVDRV_MAX_REGION_CODE)
	{
		card->RegionCode = 0x10;
		WlanDebug(WlanMsg,"unidentified region code, use the default (USA)\n");
	}

	if (card->MyMacAddress[0] == 0xff)
	{
		rt_memmove(card->MyMacAddress, hwspec->PermanentAddr, ETH_ALEN);
		hexdump("default Mac address:", card->MyMacAddress, ETH_ALEN);
	}

	/* firmware release number */
	WlanDebug(WlanMsg, "FW release number: 0x%08x\n", hwspec->FWReleaseNumber);
	WlanDebug(WlanMsg, "HW version: 0x%08x\n", hwspec->Version);

	if (wlan_set_regiontable(card, card->RegionCode, 0))
	{
		ret = WLAN_STATUS_FAILURE;
		goto done;
	}

	if (wlan_set_universaltable(card, 0))
	{
		ret = WLAN_STATUS_FAILURE;
		goto done;
	}

done: 
    return ret;
}

/** 
 *  @brief This function handles the command response of fw_wakeup_method
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param resp	   A pointer to HostCmd_DS_COMMAND
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int wlan_ret_fw_wakeup_method(WlanCard *cardInfo,
		HostCmd_DS_COMMAND * resp)
{
	WlanCard *card = cardInfo;
	HostCmd_DS_802_11_FW_WAKEUP_METHOD *fwwm = &resp->params.fwwakeupmethod;
	u16 action;

	action = wlan_le16_to_cpu(fwwm->Action);

	switch (action)
	{
	case HostCmd_ACT_GET:
	case HostCmd_ACT_SET:
		card->fwWakeupMethod = wlan_le16_to_cpu(fwwm->Method);
		break;
	default:
		break;
	}
	return WLAN_STATUS_SUCCESS;
}
/** 
 *  @brief This function handles the command response of set_wep
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param resp	   A pointer to HostCmd_DS_COMMAND
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int wlan_ret_802_11_set_wep(WlanCard *cardinfo,
		HostCmd_DS_COMMAND * resp)
{
	return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function handles the command response of rf_tx_power
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param resp	   A pointer to HostCmd_DS_COMMAND
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int wlan_ret_802_11_rf_tx_power(WlanCard *cardinfo,
		HostCmd_DS_COMMAND * resp)
{
	HostCmd_DS_802_11_RF_TX_POWER *rtp = &resp->params.txp;
	WlanCard *card = cardinfo;
	u16 Action = (rtp->Action);

	card->TxPowerLevel = wlan_le16_to_cpu(rtp->CurrentLevel);

	if (Action == HostCmd_ACT_GET)
	{
		card->MaxTxPowerLevel = rtp->MaxPower;
		card->MinTxPowerLevel = rtp->MinPower;
	}

	WlanDebug(WlanMsg,"Current TxPower Level = %d,Max Power=%d, Min Power=%d\n",
			card->TxPowerLevel, card->MaxTxPowerLevel,
			card->MinTxPowerLevel);
	return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function handles the command response of multicast_address
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param resp	   A pointer to HostCmd_DS_COMMAND
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int wlan_ret_mac_multicast_adr(WlanCard *cardinfo,
		HostCmd_DS_COMMAND * resp)
{
	return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function handles the command response of rate_adapt_rateset
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param resp	   A pointer to HostCmd_DS_COMMAND
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int wlan_ret_802_11_rate_adapt_rateset(WlanCard *cardinfo,
		HostCmd_DS_COMMAND * resp)
{
	HostCmd_DS_802_11_RATE_ADAPT_RATESET *rates = &resp->params.rateset;
	WlanCard *card = cardinfo;

	if ((rates->Action) == HostCmd_ACT_GET)
	{
		card->HWRateDropMode = (rates->HWRateDropMode);
		card->Threshold = (rates->Threshold);
		card->FinalRate = (rates->FinalRate);
		card->RateBitmap = (rates->Bitmap);
	}

	return WLAN_STATUS_SUCCESS;
}

static int wlan_ret_802_11_snmp_mib(WlanCard *cardinfo,
		HostCmd_DS_COMMAND * resp)
{
	WlanCard *card = cardinfo;
	HostCmd_DS_802_11_SNMP_MIB *smib = &resp->params.smib;
	u16 OID = wlan_le16_to_cpu(smib->OID);
	u16 QueryType = wlan_le16_to_cpu(smib->QueryType);

	WlanDebug(WlanMsg, "SNMP_RESP: value of the OID = %x, QueryType=%x\n", OID,
			QueryType);

	if (QueryType == HostCmd_ACT_GEN_GET)
	{
		switch (OID)
		{
		case FragThresh_i:
			card->FragThsd = (*((u16*) (smib->Value)));
			WlanDebug(WlanEncy,"SNMP_RESP: FragThsd =%u\n",card->FragThsd);
			break;
		case RtsThresh_i:
			card->RTSThsd = (*((u16*) (smib->Value)));
			WlanDebug(WlanEncy,"SNMP_RESP: RTSThsd =%u\n", card->RTSThsd);
			break;
		case ShortRetryLim_i:
			card->TxRetryCount = (*((u16*) (smib->Value)));
			WlanDebug(WlanEncy,"SNMP_RESP: TxRetryCount =%u\n",card->TxRetryCount);
			break;
		default:
			break;
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function handles the command response of key_material
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param resp	   A pointer to HostCmd_DS_COMMAND
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int wlan_ret_802_11_key_material(WlanCard *cardinfo,
		HostCmd_DS_COMMAND * resp)
{
	HostCmd_DS_802_11_KEY_MATERIAL *pKey = &resp->params.keymaterial;
	WlanCard *card = cardinfo;

	if ((pKey->Action) == HostCmd_ACT_SET)
	{
		if (((pKey->KeyParamSet.KeyInfo) & KEY_INFO_TKIP_MCAST)
				|| ((pKey->KeyParamSet.KeyInfo) & KEY_INFO_AES_MCAST))
		{
			WlanDebug(WlanEncy,"Key: GTK is set\n");
			card->IsGTK_SET = TRUE;
		}
	}

	rt_memcpy(card->aeskey.KeyParamSet.Key, pKey->KeyParamSet.Key,
			sizeof(pKey->KeyParamSet.Key));

	return WLAN_STATUS_SUCCESS;
}

static int
wlan_ret_802_11_rssi(WlanCard *cardinfo, HostCmd_DS_COMMAND * resp)
{
    HostCmd_DS_802_11_RSSI_RSP *rssirsp = &resp->params.rssirsp;
    WlanCard *card = cardinfo;

    rt_kprintf("snr: %d, noise: %d\n", wlan_le16_to_cpu(rssirsp->SNR), wlan_le16_to_cpu(rssirsp->NoiseFloor));
    rt_kprintf("avg snr: %d, avg noise: %d\n", wlan_le16_to_cpu(rssirsp->AvgSNR), wlan_le16_to_cpu(rssirsp->AvgNoiseFloor));

    return WLAN_STATUS_SUCCESS;
}

static int
wlan_ret_802_11_get_log(WlanCard *cardinfo, HostCmd_DS_COMMAND* resp)
{
	HostCmd_DS_802_11_GET_LOG *getlog_resp = &resp->params.glog;

	rt_kprintf("WIFI GetLog:\n");
	rt_kprintf("McastTxFrameCount: %d\n", getlog_resp->mcasttxframe);
	rt_kprintf("McastRxFrameCount: %d\n", getlog_resp->mcastrxframe);

	rt_kprintf("TxFrameCount: %d\n", getlog_resp->txframe);
	rt_kprintf("RxFragCount: %d\n", getlog_resp->rxfrag);

	rt_kprintf("FailedCount: %d\n", getlog_resp->failed);
	rt_kprintf("AckFailure: %d\n", getlog_resp->ackfailure);
	rt_kprintf("RetryCount: %d\n", getlog_resp->retry);
	rt_kprintf("MultiRetryCount: %d\n", getlog_resp->multiretry);

	rt_kprintf("FrameDupCount: %d\n", getlog_resp->framedup);
	rt_kprintf("RTSSuccessCount: %d\n", getlog_resp->rtssuccess);
	rt_kprintf("RTSFailedCount: %d\n", getlog_resp->rtsfailure);
	rt_kprintf("FCSErrorCount: %d\n", getlog_resp->fcserror);

	return WLAN_STATUS_SUCCESS;
}

/********************************************************
 Global Functions
 ********************************************************/
int wlan_process_event(WlanCard *card)
{
	int ret = WLAN_STATUS_SUCCESS;
	u32 eventcause = card->EventCause >> SBI_EVENT_CAUSE_SHIFT;

	if (eventcause != MACREG_INT_CODE_PS_SLEEP && eventcause
			!= MACREG_INT_CODE_PS_AWAKE)
		WlanDebug(WlanMsg,"EVENT: 0x%x\n", eventcause);

	switch (eventcause)
	{
	case MACREG_INT_CODE_DUMMY_HOST_WAKEUP_SIGNAL:
		WlanDebug(WlanMsg,"EVENT: DUMMY_HOST_WAKEUP_SIGNAL\n");
		break;
	case MACREG_INT_CODE_LINK_SENSED:
		WlanDebug(WlanMsg,"EVENT: LINK_SENSED\n");
		break;

	case MACREG_INT_CODE_DEAUTHENTICATED:
		WlanDebug(WlanMsg,"EVENT: Deauthenticated\n");
		break;

	case MACREG_INT_CODE_DISASSOCIATED:
		WlanDebug(WlanMsg,"EVENT: Disassociated\n");
		break;

	case MACREG_INT_CODE_LINK_LOST:
		WlanDebug(WlanMsg,"EVENT: Link lost\n");
		break;

	case MACREG_INT_CODE_PS_SLEEP:
		WlanDebug(WlanMsg,"EVENT: SLEEP\n");
		break;

	case MACREG_INT_CODE_PS_AWAKE:
		WlanDebug(WlanMsg, "EVENT: AWAKE \n");
		break;

	case MACREG_INT_CODE_DEEP_SLEEP_AWAKE:
		WlanDebug(WlanMsg,"EVENT: DS_AWAKE\n");
		break;

	case MACREG_INT_CODE_HOST_SLEEP_AWAKE:
		WlanDebug(WlanMsg,"EVENT: HS_AWAKE\n");
		break;

	case MACREG_INT_CODE_MIC_ERR_UNICAST:
		WlanDebug(WlanMsg,"EVENT: UNICAST MIC ERROR\n");
		break;

	case MACREG_INT_CODE_MIC_ERR_MULTICAST:
		WlanDebug(WlanMsg, "EVENT: MULTICAST MIC ERROR\n");
		break;

	case MACREG_INT_CODE_MIB_CHANGED:
		break;
	case MACREG_INT_CODE_INIT_DONE:
		WlanDebug(WlanMsg, "EVENT: Init Done\n");
		break;

	case MACREG_INT_CODE_BG_SCAN_REPORT:
		WlanDebug(WlanMsg,"EVENT: BGS_REPORT\n");
		break;

	case MACREG_INT_CODE_WMM_STATUS_CHANGE:
		WlanDebug(WlanMsg,"EVENT: WMM status changed\n");
		break;

	case MACREG_INT_CODE_RSSI_LOW:
		WlanDebug(WlanMsg,"EVENT: RSSI_LOW\n");
		break;
	case MACREG_INT_CODE_SNR_LOW:
		WlanDebug(WlanMsg,"EVENT: SNR_LOW\n");
		break;
	case MACREG_INT_CODE_MAX_FAIL:
		WlanDebug(WlanMsg,"EVENT: MAX_FAIL\n");
		break;
	case MACREG_INT_CODE_RSSI_HIGH:
		WlanDebug(WlanMsg,"EVENT: RSSI_HIGH\n");
		break;
	case MACREG_INT_CODE_SNR_HIGH:
		WlanDebug(WlanMsg,"EVENT: SNR_HIGH\n");
		break;
	default:
		WlanDebug(WlanMsg,"EVENT: unknown event id: %#x\n", eventcause);
		break;
	}

	card->EventCause = 0;

	return ret;
}

/** 
 *  @brief This function handles the command response
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int wlan_process_rx_command(WlanCard *cardinfo)
{
	WlanCard *card = cardinfo;
	u16 RespCmd;
	HostCmd_DS_COMMAND *resp;
	int ret = WLAN_STATUS_SUCCESS;

	u16 Result;

	if (!card->CurCmd)
	{
		resp = (HostCmd_DS_COMMAND *) card->CmdResBuf;
		WlanDebug(WlanErr, "CMD_RESP: NULL CurCmd, 0x%x\n", resp->Command);
		ret = WLAN_STATUS_FAILURE;
		goto done;
	}

	resp = (HostCmd_DS_COMMAND *) card->CmdResBuf;
	RespCmd = resp->Command;
	Result = resp->Result;

	WlanDebug(WlanCmd, "CMD_RESP: 0x%x, result %d, len %d, seqno %d\n",RespCmd, Result, resp->Size, resp->SeqNum);

	if (!(RespCmd & 0x8000))
	{
		WlanDebug(WlanErr,"CMD_RESP: Invalid response to command!");
		card->CurCmdRetCode = WLAN_STATUS_FAILURE;
		ret = WLAN_STATUS_FAILURE;
		rt_event_send(&(card->cmdwaitevent), CmdWaitQWoken);
		goto done;
	}

	/* Store the response code to CurCmdRetCode. */
	card->CurCmdRetCode = resp->Result;

	/* If the command is not successful, cleanup and return failure */
	if ((Result != HostCmd_RESULT_OK || !(RespCmd & 0x8000)))
	{
		WlanDebug(WlanErr,"CMD_RESP: cmd %#x error, result=%#x\n",resp->Command, resp->Result);
		/*
		 * Handling errors here
		 */
		switch (RespCmd)
		{
		case HostCmd_RET_HW_SPEC_INFO:
			WlanDebug(WlanMsg,"CMD_RESP: HW spec command Failed\n");
			break;
		}
		card->CurCmdRetCode = WLAN_STATUS_FAILURE;
		rt_event_send(&(card->cmdwaitevent), CmdWaitQWoken);
		return WLAN_STATUS_FAILURE;
	}

	switch (RespCmd)
	{
	case HostCmd_RET_MAC_REG_ACCESS:
	case HostCmd_RET_BBP_REG_ACCESS:
	case HostCmd_RET_RF_REG_ACCESS:
		// ret = wlan_ret_reg_access(priv, RespCmd, resp);
		break;

	case HostCmd_RET_HW_SPEC_INFO:
		ret = wlan_ret_get_hw_spec(card, resp);
		break;

	case HostCmd_RET_802_11_SCAN:
		if (card->ScanPurpose == ScanMultichs)
		{
			ret = WlanparserScanResult(card, resp);
		}
		else if (card->ScanPurpose == ScanFilter)
		{
			ret = WlanparserScanResult(card, resp);
			// ret = wlan_ret_802_11_scan(card, resp);
		}
		break;

	case HostCmd_RET_MAC_CONTROL:
		WlanDebug(WlanMsg,"HostCmd_RET_MAC_CONTROL\r\n");
		break;

	case HostCmd_RET_802_11_ASSOCIATE:
		ret = wlan_ret_802_11_associate(card, resp);
		break;

	case HostCmd_RET_802_11_DEAUTHENTICATE:
		// ret = wlan_ret_802_11_disassociate(priv, resp);
		break;

	case HostCmd_RET_802_11_SET_WEP:
		ret = wlan_ret_802_11_set_wep(card, resp);
		break;

	case HostCmd_RET_802_11_RESET:
		// ret = wlan_ret_802_11_reset(priv, resp);
		break;

	case HostCmd_RET_802_11_RF_TX_POWER:
		ret = wlan_ret_802_11_rf_tx_power(card, resp);
		break;

	case HostCmd_RET_802_11_RADIO_CONTROL:
		// ret = wlan_ret_802_11_radio_control(priv, resp);
		break;
	case HostCmd_RET_802_11_SNMP_MIB:
		ret = wlan_ret_802_11_snmp_mib(card, resp);
		break;

	case HostCmd_RET_802_11_RF_ANTENNA:
		// ret = wlan_ret_802_11_rf_antenna(priv, resp);
		break;

	case HostCmd_RET_MAC_MULTICAST_ADR:
		ret = wlan_ret_mac_multicast_adr(card, resp);
		break;

	case HostCmd_RET_802_11_RATE_ADAPT_RATESET:
		ret = wlan_ret_802_11_rate_adapt_rateset(card, resp);
		break;
	case HostCmd_RET_802_11_RF_CHANNEL:
		// ret = wlan_ret_802_11_rf_channel(priv, resp);
		break;

	case HostCmd_RET_802_11_RSSI:
		ret = wlan_ret_802_11_rssi(card, resp);
		break;

	case HostCmd_RET_802_11_GET_LOG:
		ret = wlan_ret_802_11_get_log(card, resp);
		break;

	case HostCmd_RET_802_11_MAC_ADDRESS:
		// ret = wlan_ret_802_11_mac_address(priv, resp);
		break;

	case HostCmd_RET_802_11_CAL_DATA_EXT:
		// ret = wlan_ret_802_11_cal_data_ext(priv, resp);
		break;

	case HostCmd_RET_802_11_KEY_MATERIAL:
		ret = wlan_ret_802_11_key_material(card, resp);
		break;

	case HostCmd_RET_CMD_GSPI_BUS_CONFIG:
		WlanDebug(WlanMsg,"HostCmd_RET_CMD_GSPI_BUS_CONFIG\r\n");
		break;

	case HostCmd_RET_802_11D_DOMAIN_INFO:
		// ret = wlan_ret_802_11d_domain_info(priv, resp);
		break;

	case HostCmd_RET_802_11_BCA_CONFIG_TIMESHARE:
		// ret = wlan_ret_802_11_bca_timeshare(priv, resp);
		break;

	case HostCmd_RET_802_11_INACTIVITY_TIMEOUT:
		// *((u16 *) Adapter->CurCmd->pdata_buf) =
		//     wlan_le16_to_cpu(resp->params.inactivity_timeout.Timeout);
		break;
	case HostCmd_RET_802_11_BG_SCAN_CONFIG:
		break;

	case HostCmd_RET_802_11_FW_WAKE_METHOD:
		ret = wlan_ret_fw_wakeup_method(card, resp);
		break;

	case HostCmd_RET_802_11_SLEEP_PERIOD:
		//ret = wlan_ret_802_11_sleep_period(priv, resp);
		break;
	case HostCmd_RET_WMM_GET_STATUS:
		//ret = wlan_cmdresp_wmm_get_status(priv, resp);
		break;
	case HostCmd_RET_WMM_ADDTS_REQ:
		// ret = wlan_cmdresp_wmm_addts_req(priv, resp);
		break;
	case HostCmd_RET_WMM_DELTS_REQ:
		// ret = wlan_cmdresp_wmm_delts_req(priv, resp);
		break;
	case HostCmd_RET_WMM_QUEUE_CONFIG:
		// ret = wlan_cmdresp_wmm_queue_config(priv, resp);
		break;
	case HostCmd_RET_WMM_QUEUE_STATS:
		// ret = wlan_cmdresp_wmm_queue_stats(priv, resp);
		break;
	case HostCmd_CMD_TX_PKT_STATS:
		//  memcpy(Adapter->CurCmd->pdata_buf,
		//         &resp->params.txPktStats, sizeof(HostCmd_DS_TX_PKT_STATS));
		//  ret = WLAN_STATUS_SUCCESS;
		break;
	case HostCmd_RET_802_11_TPC_CFG:
		//  memmove(Adapter->CurCmd->pdata_buf,
		//          &resp->params.tpccfg, sizeof(HostCmd_DS_802_11_TPC_CFG));
		break;
	case HostCmd_RET_802_11_LED_CONTROL:
	{
		break;
	}
	case HostCmd_RET_802_11_CRYPTO:
		//ret = wlan_ret_cmd_802_11_crypto(priv, resp);
		break;

	case HostCmd_RET_GET_TSF:
		/*resp->params.gettsf.TsfValue =
		 wlan_le64_to_cpu(resp->params.gettsf.TsfValue);
		 memcpy(priv->adapter->CurCmd->pdata_buf,
		 &resp->params.gettsf.TsfValue, sizeof(u64));*/
		break;
	case HostCmd_RTE_802_11_TX_RATE_QUERY:
		// priv->adapter->TxRate =
		//     wlan_le16_to_cpu(resp->params.txrate.TxRate);
		break;

	case HostCmd_RET_VERSION_EXT:
		/* memcpy(Adapter->CurCmd->pdata_buf,
		 &resp->params.verext, sizeof(HostCmd_DS_VERSION_EXT));*/
		break;

	case HostCmd_RET_802_11_PS_MODE:
		break;

	default:
		WlanDebug(WlanMsg,"CMD_RESP: Unknown command response %#x\n",resp->Command);
		break;
	}

	if (card->CurCmd)
	{
		/* Clean up and Put current command back to CmdFreeQ */
		rt_event_send(&(cardinfo->cmdwaitevent), CmdWaitQWoken);
	}

done: 
    return ret;
}