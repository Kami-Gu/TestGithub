/*****************************************************************************
** File: main_app_external.c
**
** Description:
**
** Copyright(c) 2008 Sunmedia Technologies - All Rights Reserved
**
** Author : qin.he
**
** $Id:  $
*****************************************************************************/

/*header file*/
#include <sys/ioctl.h>
#include "umf_debug.h"
#include "main_app.h"
#include "app_event.h"
#include "sysapp_table.h"
#include "sysapp_if.h"
#include "app_msg_filter.h"
#include "app_msg_filter_tbl.h"
#include "sysapp_timer.h"
#include "gobj_datastruct.h"
#include "al_fw.h"
#include "kmf_ioctl.h"
#include "ap_extern.h"
#include "util_ioctl.h"
#include "drv_blt_interface.h"
#include "osd_lib.h"
#include "gl_timer.h"

#ifdef CONFIG_ATV_SUPPORT
#include "atv_guiobj_table.h"
#endif

#if defined(CONFIG_DVB_SYSTEM_DVBT_SUPPORT)
#include "app_area_info.h"
#endif
#include "app_menumgr.h"
#include "app_audio.h"
#include "app_video.h"
#include "app_global.h"
#include "app_gui.h"
#include "app_power_control.h"
#include "app_systime.h"
#include "app_data_setting.h"

#include "main_guiobj_table.h"
#include "app_guiobj_source.h"
#include "app_guiobj_adjust.h"
//#include "app_guiobj_mainmenu.h"
#include "app_guiobj_fm_factory.h"
#include "app_guiobj_sleeptimer.h"
#include "pin_config.h"
#ifdef SUPPORT_CEC_TV
#include "umf_cec.h"
#endif
#include "app_guiobj_hdmilink.h"

#if defined (CONFIG_SUPPORT_USB_AUTO_UPGRADE) || defined (SUPPORT_FACTORY_AUTO_TEST)
#include "app_usb_upgrade.h"
#endif
#ifdef SUPPORT_WAKEUP_TIMER_IN_STANDBY
#include "app_guiobj_channel.h"
#endif
#include "app_factory.h"
#ifdef CONFIG_ATV_SUPPORT
#include "atv_app.h"
#endif
#ifdef CONFIG_DTV_SUPPORT
#if defined(CONFIG_DVB_SYSTEM) || defined(CONFIG_AUS_DVB_SYSTEM)
#include "dvb_app.h"
#include "dvb_guiobj_table.h"
#include "app_dvb_playback.h"
#endif
#ifdef CONFIG_ISDB_SYSTEM
#include "sbtvd_app.h"
#include "app_guiobj_sbtvd_table.h"
#endif
#ifdef CONFIG_SUPPORT_PVR
#include "mid_recorder.h"
#if defined(CONFIG_DVB_SYSTEM) || defined(CONFIG_AUS_DVB_SYSTEM)
#include "app_guiobj_dtv_pvr_rec.h"
#include "app_guiobj_dtv_pvrpower.h"
#include "app_guiobj_dtv_pvr_playinfo.h"
#endif
#ifdef CONFIG_ISDB_SYSTEM
#include "app_guiobj_sbtvd_pvr_rec.h"
#endif
#endif
#endif

#ifdef CONFIG_MEDIA_SUPPORT
#include "MM_popmsg_gui.h"
#endif
/*----------------Net------------------*/
#ifdef NET_SUPPORT
#include "net_daemon.h"
#include "net/net_app/net_config.h"
#include "app_net_instance.h"
#ifdef SUPPORT_NET_TESTING
#include "net/net_app/net_test.h"
#endif
#ifdef NET_ET_SUPPORT
#include "cbk_event.h"
#include "net_daemon.h"
#include "net/et/libet.h"
#endif
#ifdef SAMBA_SUPPORT
#include "app_network.h"
#endif
#ifdef NET_N32_SUPPORT
#include "net/dlmgr/dlmgr_if.h"
#include "net/net_app/net_config.h"
#endif
#ifdef NET_WIFI_SUPPORT
#include "dvb_gui_object_wifi_list.h"
#endif
/*+++++++++++++++++NET++++++++++++++++++++*/
#include "platform_venc.h"
#include "net/net_app/net_config.h"
#include "net_daemon.h"
#ifdef SUPPORT_NET_TESTING
#include "net/net_app/net_test.h"
#endif
#include "network_app.h"
#endif
#include "app_clone_data.h"
#include "main_app_external.h"
#ifdef SUPPORT_FACTORY_AUTO_TEST
//#include "Debug_msg.h"
#endif
#ifdef SUPPORT_SHOP_DEMO_MODE
#include "app_guiobj_picture.h"
#endif
#ifdef N32_GAME_SUPPORT
#include "app_clone_data.h"
#endif
#ifdef SUPPORT_LED_FLASH
#include "app_led_control.h"
#endif
#ifdef CONFIG_CIPLUS_SUPPORT
#include "app_dvb_ci_mmi.h"
#endif
#include "mid_partition_list.h"
#include "customize.h"
#if defined(CONFIG_AUTO_USB_STORE_IRSEQ) && defined(CONFIG_QSD_AUTOIR)
#include "umf_automation.h"
#endif
#include "tuner_demod_ioctl.h"
#ifdef SUPPORT_BLUETOOTH
#include "app_console.h"
#endif
#define MAIN_APP_DEBUG
#ifdef MAIN_APP_DEBUG
#undef mainapp_printf
#define mainapp_printf(fmt, arg...) UMFDBG(0,"[MAINAPP]:"fmt, ##arg)
#else
#define mainapp_printf(fmt, arg...) ((void) 0)
#endif

#define NUM_RESEND_MESSAGE_TYPE (DMN_EVENT_DEVICE_DETECT_MAX-DMN_EVENT_DEVICE_DETECT_MIN-1)
#define NUM_RESEND_MESSAGE_OF_TYPE (10)

#ifdef NET_SUPPORT
extern Common_SysSet_NetETConfigStorage_t stNetConfigStorage;
#endif

#ifdef SUPPORT_WAKEUP_TIMER_IN_STANDBY
extern Boolean g_fRecorderNeedEntryStandby;
#endif
#ifdef AC_ON_AUTO_GET_TIME
extern Boolean g_fBackgroundGetTime;
#endif

#ifdef NET_SUPPORT
extern int gMsgHandle;

#ifdef NET_ET_SUPPORT
static void _MAINAPP_EtMountHandle(MainAppUIEventHandlerInfo_t *pstEventInfo, UINT32 dDevNum);
static void _MAINAPP_EtUnmountHandle(MainAppUIEventHandlerInfo_t *pstEventInfo, UINT32 dDevNum);
#endif
#ifdef NET_N32_SUPPORT
static void _MAINAPP_N32MountHandle(MainAppUIEventHandlerInfo_t *pstEventInfo, UINT32 dDevNum);
static void _MAINAPP_N32UnmountHandle(MainAppUIEventHandlerInfo_t *pstEventInfo, UINT32 dDevNum);
#endif

#ifdef NET_ET_SUPPORT
MainAppUIEventHandlerInfo_t stUIEventHandlerInfo =
{
    STATUS_NULL,
    UI_EVENT_NULL,
    TOTAL_SYS_APP_SIZE
};
#endif
#endif

GUI_Object_Definition_t* MAIN_APP_GUI_Obj_List[MAINAPP_GUIOBJ_MAX] =
{
};

/* global  variables */
static bool g_bPowerOffFlag = FALSE;
static UINT32 gdResentMessage[NUM_RESEND_MESSAGE_TYPE][NUM_RESEND_MESSAGE_OF_TYPE] = {{0}};
static UINT32 gdResentParam[NUM_RESEND_MESSAGE_TYPE][NUM_RESEND_MESSAGE_OF_TYPE] = {{0}};

extern Boolean gUSBUpgradeBinIsSupperBin;
static Boolean g_CheckUpgrade = FALSE;//for mantis 0164559 @20120204
#if (defined(CONFIG_SUPPORT_USB_AUTO_UPGRADE) && defined(CONFIG_CIPLUS_SUPPORT))
static Boolean g_CheckUpgradeCIPLUS = FALSE;
#endif
#ifdef SUPPORT_FACTORY_AUTO_TEST
typedef struct{
	UINT32 dResendMessage;
	UINT32 dResendMessageParm;
}sFACResentMessage;
static sFACResentMessage gFACTESTResentMessage = {0,0};
static Boolean AcPowerOnNoRomBinFlag = FALSE;
#endif
#ifdef CONFIG_SUPPORT_SYSTEM_LIFETIME
#define SAVE_SYSTEM_LIFETIME_TIME  (15) //30 min
static UINT32 currtime;
static UINT32 usedtime;
static UINT32 SystemLifeTimerCount = 0;
#endif
#ifdef TIANLE_Board_Time
static UINT32 BoardCountTimer = 0;
static UINT32 BoardCountTimer_M = 0;
#endif

#ifdef TEAC_SYSTEMINFO_SUPPORT
static UINT32 currtime;
static UINT32 usedtime;
static UINT32 PanelCountTimer = 0;
static UINT32 DVDcurrtime;
static UINT32 DVDusedtime;
static UINT32 DVDCountTimer = 0;
static APP_Source_Type_t ePrevSourType = APP_SOURCE_MAX;
static APP_Source_Type_t eCurrSourType = APP_SOURCE_MAX;
#endif
#ifdef CONFIG_CIPLUS_SUPPORT
extern CIPlus_Sched_Update_t g_CIPlus_update;
#endif
#ifdef CONFIG_SUPPORT_PVR
extern UINT32 g_u32TimeAddOffset;
extern UINT8 g_bNeedReset;
static UINT8 dPvrinfoCount = 0;
#endif
#ifdef SUPPORT_USB_UPGRADE_LONG_PRESS_KEYPAD_POWER
extern UINT8 IsKeypadPowerOnPressRepeat;
UINT32 USB_ATTACHED = FALSE;
#endif
#ifdef CELLO_BATHROOMTV
static BOOLEAN g_bFakeStandby = FALSE;
BOOLEAN g_bRealStandby = FALSE;

void _MAINAPP_SetFakeStandbyStatue(BOOLEAN statue)
{
	g_bFakeStandby = statue;
}
BOOLEAN _MAINAPP_GetFakeStandbyStatue(void)
{
	return g_bFakeStandby;
}

BOOLEAN _MAINAPP_CheckIfEnterFakeStandbyStatue()
{
    UINT32  dSourceIndex = 0;
	UINT8 u8Mute = 0;
 	//APP_Source_Type_t eCurrSourType_temp = APP_SOURCE_MAX;	
	
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
					sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData));
	
	if (g_stFactoryUserData.n_FactSet_BurningMode == 1)
	{
		return FALSE;
	}
	
	if(g_bRealStandby==TRUE)
	{
		return FALSE;
	}
	if ((MAIN_APP_SUCCESS == MAINAPP_GetActiveSystemAppIndex(&dSourceIndex))&&(SYS_APP_DVB == dSourceIndex))
	{
#ifdef CONFIG_SUPPORT_PVR
		if (MID_RecorderIsCurRecording() == DRV_SUCCESS)
		{
			/*MID_REC_MODE MIDRecMode = MID_Recorder_GetRecMode();
			if ((MID_REC_MODE_MANUAL == MIDRecMode)||(MID_REC_MODE_TIMESHIFT_AFTER_REC == MIDRecMode))
			{
				{
					return FALSE;
				}
			}
			else*/
			{
				DVBApp_StopTimeshift(TRUE, TRUE);
			}
		}
#endif
	}
	
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO, 0,
		sizeof(APP_SETTING_UserInfo_t), &(g_stUserInfoData));
	u8Mute = g_stUserInfoData.Mute;

	if(g_bFakeStandby==FALSE)
	{//Enter fake standby
		Cmd_SetPanelPower(FALSE);
		APP_SetPanelPowerOffStatus(TRUE);
		_MAINAPP_SetFakeStandbyStatue(TRUE);
		APP_LED_SetLEDBasicLight(0);
		APP_GUIOBJ_SleepTimer_SetTimeoutPowerOffStatus(TRUE);
		MAINAPP_SetPowerOffState(TRUE);
		APP_Audio_SetMuteAll(TRUE);
	}
	else
	{//Leave fake standby
		Cmd_SetPanelPower(TRUE);
		APP_SetPanelPowerOffStatus(FALSE);
		_MAINAPP_SetFakeStandbyStatue(FALSE);
		APP_GUIOBJ_SleepTimer_SetTimeoutPowerOffStatus(FALSE);
		//APP_Audio_SetMute(TRUE, FALSE, APP_MUTE_MODE_STATEMAX, APP_SOURCE_MAX);
		APP_LED_SetLEDBasicLight(1);
		MAINAPP_SetPowerOffState(FALSE);
		if(APP_SWITCH_ON == u8Mute)
		{
			SYSAPP_GOBJ_SendMsgToSingleGUIObject(dSourceIndex, APP_GUIOBJ_MUTE, APP_INTRA_EVENT_HIDE_MUTE, 0);
			g_stUserInfoData.Mute = APP_SWITCH_OFF;
			AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO,
							 ITEM_OFFSET(APP_SETTING_UserInfo_t, Mute),
							 sizeof(g_stUserInfoData.Mute), &(g_stUserInfoData.Mute));
		}
		APP_Audio_SetMuteAll(FALSE);
	}
	return TRUE;
}
#endif
#ifdef SUPPORT_SHOP_DEMO_MODE_IN_MAINMENU
extern  UINT32 ShopDemoCountTimer;
static UINT8  tem_shop= 0;
#endif

#ifdef  SUPPORT_SHOP_DEMO_MODE_IN_MAINMENU
static void _MAINAPP_SHOP_mode(void)
{
   	 UINT32 dIndex;
 	APP_Source_Type_t eCurrSourType_temp = APP_SOURCE_MAX;	
	INT32 dObjectID=0;

//-------------------------DEMO_MODE  start-----------------------------------------------------------------------
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
		sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
 	APP_GUIOBJ_Source_GetCurrSource(&eCurrSourType_temp);
	MAINAPP_GetActiveSystemAppIndex(&dIndex);
	
     //       AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO, 0,
        //                    sizeof(APP_SETTING_UserInfo_t), &(g_stUserInfoData));
 if (
		//  (ret != SYSTEM_APP_NO_FOCUSED_GUIOBJ)
 		(APP_MenuMgr_Exist_Scan_Menu())
		|| (SYSAPP_GOBJ_GUIObjectExist(dIndex, APP_GUIOBJ_SOFTWARE_UPGRADE))
		||(APP_MenuMgr_Exist_Main_Menu())
	        ||(APP_MenuMgr_Exist_Factory_Menu())
	         ||(SYSAPP_GOBJ_GUIObjectExist(dIndex, APP_GUIOBJ_SOFTWARE_UPGRADE))
	         ||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_FAVMGR))
  		||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_SUBTITLE))
  		 ||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_SUBTITLEMENU))
  		 ||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_CHNLIST))
  		 ||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_TTX))
  	         ||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV, ATV_GUIOBJ_TTX))
  	         ||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV, APP_GUIOBJ_BANNER))
  	          ||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV,ATV_GUIOBJ_CHNLIST))
  	         ||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV, ATV_GUIOBJ_FAVMGR))
  	         || (SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_MULTIAUD))
  	          || (SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_CI_MENU))
	      //  || (SYSAPP_GOBJ_GUIObjectExist(dIndex, APP_GUIOBJ_MUTE))
	    //   || (g_stUserInfoData.Mute)
	        ||((dIndex < TOTAL_SYS_APP_SIZE)&&(SYSAPP_GOBJ_GUIObjectExist(dIndex, APP_GUIOBJ_SOURCE)))
	        ||((dIndex < TOTAL_SYS_APP_SIZE)&&(SYSAPP_GOBJ_GUIObjectExist(dIndex, APP_GUIOBJ_SLEEP_INFO)))
	         ||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_PVR_PLAYINFO))
	         ||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_PVR_FILEPLAY))
	        ||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_PVR_REC))
	        ||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, APP_GUIOBJ_BANNER))
	        ||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_PROGINFO))
	        
	)
	     
{
	
	ShopDemoCountTimer=0;
}
else  
{
	if (
		(SYSAPP_GOBJ_GetFocusedGUIObject(dIndex, &dObjectID) != SYSTEM_APP_SUCCESS)
		||(!APP_MenuMgr_Exist_Main_Menu())
  	  )
	{
		if(ShopDemoCountTimer<400)
		ShopDemoCountTimer++;	
		else
		ShopDemoCountTimer=10;

		if((tem_shop==0)&&(g_stSetupData.u8shop_mode==1))
		{
			 tem_shop=1;
			if (!SYSAPP_GOBJ_GUIObjectExist(dIndex, APP_GUIOBJ_DEMOD))
			{	
				ShopDemoCountTimer=199;
				SYSAPP_GOBJ_CreateGUIObject_WithPara(dIndex, APP_GUIOBJ_DEMOD, 0);
			}
		}
		else if((tem_shop==1)&&(g_stSetupData.u8shop_mode==0))
		{
			 tem_shop=0;
			if (SYSAPP_GOBJ_GUIObjectExist(dIndex, APP_GUIOBJ_DEMOD))
			{
				ShopDemoCountTimer=399;
				SYSAPP_GOBJ_DestroyGUIObject(dIndex, APP_GUIOBJ_DEMOD);
			}
			//SYSAPP_GOBJ_CreateGUIObject_WithPara(dIndex, APP_GUIOBJ_DEMOD, 1);
		}

		if(tem_shop==1)
		{
		 if(ShopDemoCountTimer<10)
			{
				if (SYSAPP_GOBJ_GUIObjectExist(dIndex, APP_GUIOBJ_DEMOD))
				{
					SYSAPP_GOBJ_DestroyGUIObject(dIndex, APP_GUIOBJ_DEMOD);
				}
			}
		 else
		 	{
			if(ShopDemoCountTimer==199)
			{
				if (!SYSAPP_GOBJ_GUIObjectExist(dIndex, APP_GUIOBJ_DEMOD))
				{
					SYSAPP_GOBJ_CreateGUIObject_WithPara(dIndex, APP_GUIOBJ_DEMOD, 0);
				}
			}
			else if(ShopDemoCountTimer==399)
			{
				//SYSAPP_GOBJ_CreateGUIObject_WithPara(dIndex, APP_GUIOBJ_DEMOD, 0);
				if (SYSAPP_GOBJ_GUIObjectExist(dIndex, APP_GUIOBJ_DEMOD))
				{
					SYSAPP_GOBJ_DestroyGUIObject(dIndex, APP_GUIOBJ_DEMOD);
				}
			}	
		 	}
		 //-------------------for mute icon---------------
		 
		 //---------------------------------------

		}
	}
		
}
		
}
//-----------------------DEMO_MODE  end-------------------------------------------------------------------------		
#endif
#ifdef CONFIG_SUPPORT_SYSTEM_LIFETIME
void MAINAPP_UpdateSystemLifeTimeToFlash(UINT8 isCountTimer)
{
	APP_SETTING_SystemUpgradeSaveFlashDateTable_t FlashDateSystem;
	
	if(g_CheckUpgrade)
	{
		return;
	}

	if(isCountTimer == TRUE)
	{
		currtime = GL_GetRtc32()/1000;// s
		APP_SystemLifeTimeDataWriteRead(FALSE,(UINT8 *)&FlashDateSystem);//Read flash time
		FlashDateSystem.SystemLifeTime += SAVE_SYSTEM_LIFETIME_TIME;//min
		#if 1//def CONFIG_SUPPORT_SYSTEM_PowerBurning	
		//FlashDateSystem.SystemPowerBurning = 1;// SAVE_SYSTEM_LIFETIME_TIME;//min
		#endif
		APP_SystemLifeTimeDataWriteRead(TRUE,(UINT8 *)&FlashDateSystem);//Write time to flash			
		SystemLifeTimerCount = 0;
	}
	else
	{
		usedtime = GL_GetRtc32()/1000 - currtime;
		if(usedtime/60 <= 2*SAVE_SYSTEM_LIFETIME_TIME)//check valid
		{
			APP_SystemLifeTimeDataWriteRead(FALSE,(UINT8 *)&FlashDateSystem);//Read flash time
			FlashDateSystem.SystemLifeTime += usedtime/60;//min
			#if 1//def CONFIG_SUPPORT_SYSTEM_PowerBurning
			//FlashDateSystem.SystemPowerBurning = 1;
			#endif
			APP_SystemLifeTimeDataWriteRead(TRUE,(UINT8 *)&FlashDateSystem);//Write time to flash			
		}
	}
}
#endif
static void _MAINAPP_ResendEventInTransition(void)
{
    UINT32 dActiveSysApp = 0;
	UINT8 u8Count1 = 0;
	UINT8 u8Count2 = 0;

   	switch (MAINAPP_GetActiveSystemAppIndex(&dActiveSysApp))
    {
        case MAIN_APP_IN_TRANSITION:
            mainapp_printf(" In Transition State, wait and retry.\n");
            break;

        case MAIN_APP_SUCCESS:
			for (u8Count1 = 0; u8Count1 < NUM_RESEND_MESSAGE_TYPE; u8Count1++)
			{
				for (u8Count2 = 0; u8Count2 < NUM_RESEND_MESSAGE_OF_TYPE; u8Count2++)
				{
					if (gdResentMessage[u8Count1][u8Count2] > 0)
					{
						MSG_FILTER_DispatchMessage(gdResentMessage[u8Count1][u8Count2], gdResentParam[u8Count1][u8Count2]);
					}
				}
			}
			break;

        default:
			mainapp_printf("[%s] In Invalid State\n",__FUNCTION__);
			for (u8Count1 = 0; u8Count1 < NUM_RESEND_MESSAGE_TYPE; u8Count1++)
			{
				for (u8Count2 = 0; u8Count2 < NUM_RESEND_MESSAGE_OF_TYPE; u8Count2++)
				{
					gdResentMessage[u8Count1][u8Count2] = 0;
					gdResentParam[u8Count1][u8Count2] = 0;
				}
			}
            break;
    }
}

static void _MAINAPP_SetEventInTransition(UINT32 dMessage, UINT32 dParam)
{
	UINT8 u8Count = 0;

	if (((dMessage-DMN_EVENT_DEVICE_DETECT_MIN) > 0)
		&& ((dMessage-DMN_EVENT_DEVICE_DETECT_MIN-1) < NUM_RESEND_MESSAGE_TYPE))
	{
		for (u8Count = 0; u8Count < NUM_RESEND_MESSAGE_OF_TYPE; u8Count++)
		{
			//zhongbaoxing added @20120313. no need to record the exist messages
			if((gdResentMessage[dMessage-DMN_EVENT_DEVICE_DETECT_MIN-1][u8Count] == dMessage)
                &&(gdResentParam[dMessage-DMN_EVENT_DEVICE_DETECT_MIN-1][u8Count] == dParam))
            {
				break;
            }
			if (gdResentMessage[dMessage-DMN_EVENT_DEVICE_DETECT_MIN-1][u8Count] == 0)
			{
				gdResentMessage[dMessage-DMN_EVENT_DEVICE_DETECT_MIN-1][u8Count] = dMessage;
				gdResentParam[dMessage-DMN_EVENT_DEVICE_DETECT_MIN-1][u8Count] = dParam;
				break;
			}
		}
	}
}

static void _MAINAPP_ClearEventInTransition(UINT32 dMessage, UINT32 dParam)
{
	UINT8 u8Count = 0;

	if (((dMessage-DMN_EVENT_DEVICE_DETECT_MIN) > 0)
		&& ((dMessage-DMN_EVENT_DEVICE_DETECT_MIN-1) < NUM_RESEND_MESSAGE_TYPE))
	{
		for (u8Count = 0; u8Count < NUM_RESEND_MESSAGE_OF_TYPE; u8Count++)
		{
			if ((gdResentMessage[dMessage-DMN_EVENT_DEVICE_DETECT_MIN-1][u8Count] == dMessage)
				&&(gdResentParam[dMessage-DMN_EVENT_DEVICE_DETECT_MIN-1][u8Count] == dParam))
			{
				gdResentMessage[dMessage-DMN_EVENT_DEVICE_DETECT_MIN-1][u8Count] = 0;
				gdResentParam[dMessage-DMN_EVENT_DEVICE_DETECT_MIN-1][u8Count] = 0;
				break;
			}
		}
	}
}

/*****************************************************************************
** FUNCTION : _MAINAPP_GUIFeedbackEventHandler
**
** DESCRIPTION :
**				main app feedback event handler
**
** PARAMETERS :
**				dMessage: feadback message
**				dParam: feadback message's paramter
**
** RETURN VALUES:
**				None
*****************************************************************************/
void _MAINAPP_GUIFeedbackEventHandler(UINT32 dMessage, UINT32 dParam)
{
	switch(dMessage)
	{
		default:
			mainapp_printf(" Unknown GUI feedback message received.\n");
			break;
	}

	return;
}

/*****************************************************************************
** FUNCTION : _MAINAPP_UIEventHandler
**
** DESCRIPTION :
**				main app UI event handler
**
** PARAMETERS :
**				dMessage: UI message
**				dParam: UI message's paramter
**
** RETURN VALUES:
**				None
*****************************************************************************/
extern UINT8 gIsFactoryResetting;
int _MAINAPP_UIEventHandler(UINT32 dMessage, UINT32 dParam)
{
    UINT32  dIndex = 0;
    APP_Source_Type_t eSourceType;
    eSourceType = APP_SOURCE_MAX;

    if (MAIN_APP_SUCCESS == MAINAPP_GetActiveSystemAppIndex(&dIndex))
    {
        APP_GUIOBJ_Source_GetCurrSource(&eSourceType);
#if (defined CONFIG_DTV_SUPPORT)
        if (eSourceType == APP_SOURCE_DTV || eSourceType == APP_SOURCE_RADIO)
        {
            if ((SYSAPP_GOBJ_GUIObjectExist(dIndex, DVB_GUIOBJ_SCAN_PROCESS))
				&&(dMessage != UI_EVENT_POWER)
 				&&(dMessage != UI_EVENT_MUTE))
            {
                return MAIN_APP_SUCCESS;
            }
        }
#endif
#if (defined CONFIG_ATV_SUPPORT)
        if (eSourceType == APP_SOURCE_ATV)
        {
            if ((SYSAPP_GOBJ_GUIObjectExist(dIndex, ATV_GUIOBJ_SCAN_PROCESS))
				&&(dMessage != UI_EVENT_POWER)
				&&(dMessage != UI_EVENT_MUTE))
            {
                return MAIN_APP_SUCCESS;
            }
        }
#endif
    }

    if (MAIN_APP_SUCCESS == MAINAPP_GetActiveSystemAppIndex(&dIndex))
    {
 #ifdef CONFIG_SUPPORT_OTA_UPGRADE
	    if (SYS_APP_DVB == dIndex)
	    {
	 		if (SYSAPP_GOBJ_GUIObjectExist(dIndex, DVB_GUIOBJ_OTA_UPGRADE_PROGRESS) && (AL_POWER_STATE_MAX != dParam))
		    {
 		        return MAIN_APP_SUCCESS;
		    }
 	    }
#endif

#ifdef CONFIG_SUPPORT_USB_UPGRADE
        if (SYSAPP_GOBJ_GUIObjectExist(dIndex, APP_GUIOBJ_SOFTWARE_UPGRADE))
        {
            return MAIN_APP_SUCCESS;
        }
#endif
    }

    switch (dMessage)
    {
        case UI_EVENT_POWER:
#ifdef CELLO_BATHROOMTV
			if(AL_POWER_STATE_SLEEP == dParam)
			{
				if(_MAINAPP_CheckIfEnterFakeStandbyStatue()==TRUE)
				{
					return MAIN_APP_SUCCESS;
				}
			}
#endif
#ifdef SUPPORT_WAKEUP_TIMER_IN_STANDBY
            if (APP_WAKEUP_GetBootFlag() == TRUE)
            {
#ifdef AC_ON_AUTO_GET_TIME
                if (g_fBackgroundGetTime == TRUE)
                {
                    g_fBackgroundGetTime = FALSE;
                }
#endif
                g_fRecorderNeedEntryStandby = FALSE;

				/*<   for CI+  Module */
	#ifdef CONFIG_SUPPORT_PVR
				if (MID_RecorderIsCurRecording() == DRV_SUCCESS)
				{
					mainapp_printf("REC  Mode Chang to MID_REC_MODE_MANUAL\n\n\n");
					MID_RecorderModeChange(MID_REC_MODE_MANUAL);
				}
	#endif
				/*<  light backlight */
                APP_WAKEUP_RealPowerUp();

                return MAIN_APP_SUCCESS;
            }
            else
#endif
            {
#ifdef CHECK_USB_FOR_SLEEP_AND_STANDBY
		/*
		 * if the POWER event is from handset, check usb status for schedruler rec
		 */
                if (APP_IsUIMessageType())
                {
                    USB_PvrSt_t USB_ST = USB_MSG_SUCCESS;
                    UINT32 dActiveSysApp = 0;
                    USB_ST = APP_CheckUSBStatus();
                    if (MAINAPP_GetActiveSystemAppIndex(&dActiveSysApp) == MAIN_APP_SUCCESS)
                    {
                        UINT32 u32PopMsgName = POPMSG_PROMPT_NO_PROG;
                    #ifdef CONFIG_MEDIA_SUPPORT
                        UINT32 u32MMPopName = FILE_POP_PLAYBACK_NONE;
                        if (SYS_APP_FILE_PLAYER == dActiveSysApp)
                        {
                            u32MMPopName = MM_PopMsg_GetCurrentPopup();
                        }
                        else
                    #endif
                        if ((SYS_APP_ATV== dActiveSysApp)
                            || (SYS_APP_DVB== dActiveSysApp))
                        {
                            APP_GUIOBJ_PopMsg_GetMsgDialogType(&u32PopMsgName);
                        }

                        if ((u32PopMsgName != POPMSG_CONFIRM_INSERT_USB)
                            && (u32PopMsgName != POPMSG_CONFIRM_INVALID_USB)
                    #ifdef CONFIG_MEDIA_SUPPORT
                            && (u32MMPopName != FILE_POP_PVR_INSERT_USB_CONFIRM)
                            && (u32MMPopName != FILE_POP_PVR_INVALID_USB_CONFIRM)
                    #endif
                            )
                        {
                            switch(USB_ST)
                            {
                                case USB_MSG_SUCCESS:
                                    break;
                                case USB_MSG_FAIL_NO_STORAGE:
                                    SYSAPP_IF_SendGlobalEventWithIndex(dActiveSysApp, APP_GLOBAL_EVENT_PVR_INSERT_USB_CONFIRM|PASS_TO_SYSAPP, 0);
                                    return MAIN_APP_SUCCESS;
                                default:
                                    SYSAPP_IF_SendGlobalEventWithIndex(dActiveSysApp, APP_GLOBAL_EVENT_PVR_INVALID_USB_CONFIRM|PASS_TO_SYSAPP, 0);
                                    return MAIN_APP_SUCCESS;
                            }
                        }
                    }
                }
#endif
                AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
                                sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData));
                if ((g_stFactoryUserData.n_FactSet_BurningMode == 1)&&(!gIsFactoryResetting))
                {
                    return MAIN_APP_SUCCESS;
                }
#ifdef CONFIG_SUPPORT_PVR
				if (MID_RecorderIsCurRecording() == DRV_SUCCESS)
				{
				    MID_REC_MODE MIDRecMode = MID_Recorder_GetRecMode();
					if (MID_REC_MODE_MANUAL == MIDRecMode)
					{
#ifndef SUPPORT_FREE_RECORD_TIME
						//if (!APP_GUIOBJ_DVB_PvrRec_GetDurationTime())
#endif
						{
						    if (APP_GetAskPowerOffInRecording() == TRUE)
                            {
                                //for cultravier spec, mantis 0163493
    							//pop up to ask whether continu recording with panel off when receive POWER KEY
    							SYSAPP_IF_SendGlobalEventWithIndex(dIndex,
    							    APP_GLOGAL_EVENT_PVR_ASKFORSTANDBY|PASS_TO_SYSAPP, 0);
								APP_GUIOBJ_SleepTimer_SetTimeoutPowerOffStatus(FALSE);
    							return MAIN_APP_SUCCESS;
                            }
#ifdef SUPPORT_PVR_RECORD_CONFLICT_WITH_SLEEPTIME
                            else if (APP_GUIOBJ_DVB_PvrPOwer_GetMode() != SCREENDOWN_MODE)
                            {
                                DVBApp_StopRecord();
                            }
#ifdef SUPPORT_PVR_RECORD_PLAY_CURRENTRECORD
                            if (DVBAPP_Pvr_GetRECPlayFlag() == TRUE)
                            {
								DVBApp_StopTimeshift(TRUE, TRUE);
                            }
#endif
                            APP_GUIOBJ_DVB_PvrPOwer_ResetMode();
#endif
						}
					}
					else if((MID_REC_MODE_TIMESHIFT_AFTER_REC == MIDRecMode)
                            && (APP_GetAskPowerOffInRecording() == TRUE))
					{
                        SYSAPP_IF_SendGlobalEventWithIndex(dIndex,
                            APP_GLOGAL_EVENT_PVR_TIMESHIFT_STANDBY|PASS_TO_SYSAPP, 0);
						APP_GUIOBJ_SleepTimer_SetTimeoutPowerOffStatus(FALSE);
                        return MAIN_APP_SUCCESS;
					}
                    else
                    {
						//APP_Video_SetMute(TRUE, FALSE, APP_MUTE_MODE_NO_SIGNALE, APP_SOURCE_MAX);
						//APP_Audio_SetMute(TRUE, FALSE, APP_MUTE_MODE_STATEMAX, APP_SOURCE_MAX);
                        DVBApp_StopTimeshift(TRUE, TRUE);
                    }
				}
				APP_SetAskPowerOffInRecording(TRUE);
                /*< check if enter panel off mode -- recording or recorder timer is less than 2 minute */
                if (APP_WAKEUP_Recorder_CheckPanelOffMode() == TRUE)
                {
                    mainapp_printf(" UI_EVENT_POWER receive and enter panel off mode.\n");
					APP_GUIOBJ_SleepTimer_SetTimeoutPowerOffStatus(FALSE);
                    return MAIN_APP_SUCCESS;
                }
#endif
                mainapp_printf(" UI_EVENT_POWER received.\n");
#ifdef SUPPORT_WAKEUP_TIMER_IN_STANDBY
#ifdef CONFIG_SUPPORT_USB_UPGRADE
                extern bool g_bUpgradeForceStandby;
#endif
			    if((APP_WAKEUP_CheckTimer() == FALSE)
#ifdef CONFIG_SUPPORT_USB_UPGRADE
					|| (g_bUpgradeForceStandby == TRUE)
#endif
				)

#endif
                {
	                //zhongbaoxing added to mute the video for mantis 0166639 @20120224
#ifdef CONFIG_SUPPORT_SUBTITLE
					if (MAIN_APP_SUCCESS == MAINAPP_GetActiveSystemAppIndex(&dIndex))
					{
						if (dIndex == SYS_APP_DVB)
						{
							DVBApp_DataApplicationSwitch(OSD2CTRLMDL_DISABLE|OSD2CTRLMDL_SUB);
						}
					}
#endif
#ifdef CONFIG_SUPPORT_TTX
					if (MAIN_APP_SUCCESS == MAINAPP_GetActiveSystemAppIndex(&dIndex))
	    			{
	    	#ifdef CONFIG_DTV_SUPPORT
					    if (SYS_APP_DVB == dIndex)
					    {
		 					if (SYSAPP_GOBJ_GUIObjectExist(dIndex, DVB_GUIOBJ_TTX))
		 					{
								SYSAPP_GOBJ_DestroyGUIObject(dIndex, DVB_GUIOBJ_TTX);
							}
					    }
		#endif
		#ifdef CONFIG_ATV_SUPPORT
						 if(SYS_APP_ATV == dIndex)
						{
							if (SYSAPP_GOBJ_GUIObjectExist(dIndex, ATV_GUIOBJ_TTX))
		 					{
								SYSAPP_GOBJ_DestroyGUIObject(dIndex, ATV_GUIOBJ_TTX);
							}
						}
		#endif
		    		}
#endif
#ifdef CONFIG_CC_SUPPORT
                  if (MAIN_APP_SUCCESS == MAINAPP_GetActiveSystemAppIndex(&dIndex))
                  {
                      if(dIndex == SYS_APP_ATV)
                      {
                          if(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV, ATV_GUIOBJ_CC))
                          {
                              SYSAPP_GOBJ_DestroyGUIObject(SYS_APP_ATV, ATV_GUIOBJ_CC);
                          }
                      }
                  }
#endif
	                APP_Video_SetMute(TRUE, FALSE, APP_MUTE_MODE_NO_SIGNALE, APP_SOURCE_MAX);
	                APP_Audio_SetMute(TRUE, FALSE, APP_MUTE_MODE_STATEMAX, APP_SOURCE_MAX);

#ifdef SUPPORT_CEC_TV
					AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FEATURE, 0,
						sizeof(APP_SETTING_Feature_t), &(g_stFeatureData));
					if (( APP_SWITCH_ON == g_stFeatureData.Enable_HDMILink)
						&&(APP_SWITCH_ON == g_stFeatureData.HDMI_AutoStandby))
					{
						if (CECTV_GetCECEnable()==CECTV_ENABLE)
						{
							mainapp_printf("[%s]:Waiting for CECTV_SINGLE_ACT_CMD_STANDBY Send Complete !!!!!\n\n", __FUNCTION__);
							CECTV_SendCmd(CECTV_SINGLE_ACT_CMD_STANDBY, 0);
							
							//LED control,beacuse wait need time,so need set led first 
							APP_LED_Flash_Timer_Set(LED_FLASH_TYPE_MAX,0);
							APP_LED_SetLEDBasicLight(0);
							
							UINT8 i=0;
							while(CECTV_GetCECTaskStatus()!=CEC_TASK_Standby)//Waiting for CEC Message Send complete
							{
								i++;
								GL_TaskSleep(10);
								if (i > 50)
								{
									break;
								}
							}
						}
					}
#endif

	                Cmd_Tuner_PowerSaving();

					Cmd_gpio_WriteOnLevel(GPIO_AUDIO_CTL);
					Cmd_gpio_WriteOffLevel(GPIO_12V_EN);


	                /* power down when scan, must stop scan thread first
			                 * Note: when return from AL_CS_Stop(), is it already stop?
			                 */
	                if (AL_CS_IsStop() == al_false)
	                {
	                    AL_CS_Stop();
	                    while(AL_CS_IsStop() == al_false)
	                    {
							GL_TaskSleep(20);
	                    }
						//#ifndef CONFIG_ISDB_SYSTEM
						UINT32 dIndex;
						MAINAPP_GetActiveSystemAppIndex(&dIndex);
						if (SYSAPP_GOBJ_GUIObjectExist(dIndex, APP_GUIOBJ_INITINSTALL))
						{
							SYSAPP_GOBJ_DestroyGUIObject(dIndex, APP_GUIOBJ_INITINSTALL);
#ifdef SUPPORT_LCN
							AL_DB_EDBType_t eNetType = APP_DVB_Playback_GetCurrentNetType();
						    AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO, 0,
						                    sizeof(APP_SETTING_SystemInfo_t), &(g_stSysInfoData));

							#ifdef SUPPORT_DVBS_LCN
							if (g_stSysInfoData.LcnOnOff == LCN_ON && (eNetType == AL_DBTYPE_DVB_T || eNetType == AL_DBTYPE_DVB_C|| eNetType == AL_DBTYPE_DVB_S))
							#else
							if (g_stSysInfoData.LcnOnOff == LCN_ON && (eNetType == AL_DBTYPE_DVB_T || eNetType == AL_DBTYPE_DVB_C))
							#endif
						    {
						        DBLCNConfInfo_t *pLcnConf = AL_LCN_GetLCNConfChannel();
						        LCN_PreJudgeLCN();
						        LCN_QueryLCNConflict(pLcnConf);
						        if (pLcnConf->ConfCount > 0)
						        {
						            LCN_AutoAssignConflictLCN();
						        }
						        LCN_PostJudgeLCN();
						    }
#endif
#ifdef CONFIG_DTV_SUPPORT
							extern void APP_DVB_ChannelOrderChecking(void);
					    	APP_DVB_ChannelOrderChecking();
#endif
						}
						//#endif

					}

                    AL_Power_SetPowerState(dParam);
#ifdef SUPPORT_BLUETOOTH
					if((APP_Audio_GetBlueToothOnOff() == APP_SWITCH_ON) && (APP_Get_BlueTooth_State() == BLUETOOTH_STATE_CONNECTED))
					APP_Console_TO_BlueTooth_Parser(CON_TO_BLUETOOTH_POWER_OFF);
#endif
					
                   /* g_stUserInfoData.PowerStatus = APP_SWITCH_OFF; // power off
		                    AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO,
		                                     ITEM_OFFSET(APP_SETTING_UserInfo_t, PowerStatus),
		                                     sizeof(g_stUserInfoData.PowerStatus), &(g_stUserInfoData.PowerStatus));
		                    AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO,
                                     ITEM_OFFSET(APP_SETTING_UserInfo_t, PowerStatus), sizeof(g_stUserInfoData.PowerStatus));*/			
#ifdef TEAC_SYSTEMINFO_SUPPORT
					AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,0,
						sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
					usedtime = GL_GetRtc32()/1000 - currtime;
    				g_stSetupData.PanelTime += usedtime;
    				AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,
						ITEM_OFFSET(APP_SETTING_Setup_t, PanelTime),
	 					sizeof(g_stSetupData.PanelTime),&(g_stSetupData.PanelTime));
	 				AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
    					sizeof(APP_SETTING_Setup_t));
	 				APP_GUIOBJ_Source_GetCurrSource(&eCurrSourType);
	 				if(eCurrSourType == APP_SOURCE_DVD)
	 				{
	 					usedtime = (GL_GetRtc32() - DVDcurrtime)/1000;
	 					g_stSetupData.DVDTime += usedtime;
	 					AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,
							ITEM_OFFSET(APP_SETTING_Setup_t, DVDTime),
	 						sizeof(g_stSetupData.DVDTime),&(g_stSetupData.DVDTime));
    					AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
    						sizeof(APP_SETTING_Setup_t));
    				}
#endif
#ifdef CONFIG_SUPPORT_SYSTEM_LIFETIME
					MAINAPP_UpdateSystemLifeTimeToFlash(FALSE);
#endif
#ifdef AC_ON_AUTO_GET_TIME
                    if (g_fBackgroundGetTime == TRUE)
                    {
                        /* keep the latest source if get time over */
                        g_fBackgroundGetTime = FALSE;
                        APP_Source_Type_t SourceIndex;
                        SourceIndex = APP_GUIOBJ_Source_GetStandbySource();
                        g_stUserInfoData.SourceIndex = SourceIndex;
                        AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO,
                            ITEM_OFFSET(APP_SETTING_UserInfo_t, SourceIndex),
                            sizeof(g_stUserInfoData.SourceIndex),&(g_stUserInfoData.SourceIndex));
                        AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO,
                            ITEM_OFFSET(APP_SETTING_UserInfo_t, SourceIndex),
                            sizeof(g_stUserInfoData.SourceIndex));
                    }
#endif
					MAINAPP_FinalizeFlow(dParam);
                }
            }
            break;

        case UI_EVENT_MUTE:
            if (MAINAPP_GetActiveSystemAppIndex(&dIndex) == MAIN_APP_SUCCESS)
            {
                SYSAPP_IF_SendGlobalEventWithIndex(dIndex,(dMessage | PASS_TO_SYSAPP), dParam);
            }
            break;

        case UI_EVENT_SOURCE:
            mainapp_printf("[%s]: UI_EVENT_SOURCE!!!!!\n\n", __FUNCTION__);
			break;

#ifdef SUPPORT_USB_UPGRADE_LONG_PRESS_KEYPAD_POWER
        case UI_EVENT_KEYPAD_POWER_UPGRADE:
            if (MAINAPP_GetActiveSystemAppIndex(&dIndex) == MAIN_APP_SUCCESS)
            {
                dMessage = APP_GLOBAL_EVENT_USB_UPGRADE | PASS_TO_SYSAPP;
                SYSAPP_IF_SendGlobalEventWithIndex(dIndex, dMessage, 0); // 1:EraseAll,0:not EraseAll
				g_CheckUpgrade = TRUE;
            }
			break;
#endif
#ifdef CONFIG_SUPPORT_PVR
		case APP_GLOGAL_EVENT_PVR_TIEMRREC_CHECKSTANDBY:
			APP_WAKEUP_CompleteNotify();
			break;
#endif
		case APP_GLOBAL_EVENT_OSD_CAPTURE:
		{
			mainapp_printf("\n\n%s APP_GLOBAL_EVENT_OSD_CAPTURE\n", __FUNCTION__);
			//extern int app_capture_osd_handler(void);
			//app_capture_osd_handler();
			//mainapp_printf("capture OK!  \n");
			char szMountName[20];
			char szTempName[20] = {0};
			char fileNameStringtemp[50] = {0};
			char fileNameString[200] = {0};
			struct stat stStat;

			memset(fileNameStringtemp, 0, 50);
			memset(fileNameString, 0, 200);
			memset(szMountName, 0, sizeof(szMountName));
			MID_PartitionList_GetMountName(0, szMountName);
			if(strcmp(szMountName, szTempName))
			{
				sprintf(fileNameStringtemp,"%s/%s", szMountName,"SPOSD");
				if (stat(fileNameStringtemp, &stStat) < 0)
				{
					if (mkdir(fileNameStringtemp, O_WRONLY | O_CREAT) < 0)
					{
						mainapp_printf("%s mkdir failed\n", __FUNCTION__);
						break;
					}
				}
				static int fn = 0;
				fn++;
				sprintf(fileNameString,"%s/%s%d_%.3d%.2d.bmp", fileNameStringtemp,"OSD_",fn,rand()%1000,rand()%100);
				mainapp_printf("\nfileNameString:%s\n",fileNameString);
				OSD_ChangeIntoBitmap(fileNameString);
				mainapp_printf("\n\n%s capture ok!\n", __FUNCTION__);
			}

			break;
		}
		
		case FW_DB_LOAD_DEFAULT:
#ifdef CONFIG_DVB_SYSTEM_DVBS_SUPPORT
			if (AL_DBTYPE_DVB_S == APP_DVB_Playback_GetCurrentNetType())
			{
				/* wait for DVB-S becomes acticve */
				while(AL_FW_CheckActiveDBModule(AL_DBTYPE_DVB_S) != al_true);
				
				DVBApp_LoaddefaultDB(AL_DBTYPE_DVB_S);
				AL_DB_Sync(AL_DBTYPE_DVB_S, al_true);
			}
#endif
			break;
        default:
            mainapp_printf(" Un-handled UI Eventid: 0x%x.\n", dMessage);
            break;
    }

    return MAIN_APP_SUCCESS;
}

/*****************************************************************************
** FUNCTION : _MAINAPP_DaemonEventHandler
**
** DESCRIPTION :
**				main app daemon event handler
**
** PARAMETERS :
**				dMessage: daemon message
**				dParam: daemon message's paramter
**
** RETURN VALUES:
**				None
*****************************************************************************/
void _MAINAPP_DaemonEventHandler(UINT32 dMessage, UINT32 dParam)
{
#ifdef NET_SUPPORT
#ifdef NET_APP_SUPPORT
    UINT8 i;
    UINT8 bFlag = 0;
    char *pcMountName = NULL;
    int iPtnHandle = 0;
#endif
#endif
    UINT32 dActiveSysApp = 0;
    int sdPtIdx = -1;
	INT32 iRet = AL_FAILURE;
#if defined(CONFIG_SUPPORT_USB_AUTO_UPGRADE) 
	app_data_keyupgrade_status_e updateCI = APP_DATA_KEYUPGRADE_NOT_UPGRADE;
	app_data_keyupgrade_status_e updateHDCP = APP_DATA_KEYUPGRADE_NOT_UPGRADE;
	UINT32 dKeyparam = 0;
	UINT32 dKeyUpgradeEvent = 0;
#endif

#if defined(NET_ET_SUPPORT) || defined (NET_N32_SUPPORT)
    MainAppUIEventHandlerInfo_t *pstEventInfo = &stUIEventHandlerInfo;
#endif

    switch (dMessage)
    {
        case DMN_EVENT_BD_VIDEO_DISC_INSERTED:
        case DMN_EVENT_DVD_VIDEO_DISC_INSERTED:
        case DMN_EVENT_VCD_DISC_INSERTED:
        case DMN_EVENT_TEMPTEST_START_USB_DVD:
            break;

        case DMN_EVENT_USB_HDD_ATTACHED:
        case DMN_EVENT_USB_LOADER_ATTACHED:
        case DMN_EVENT_CARD_DEV_ATTACHED:
        case DMN_EVENT_IDE_HDD_ATTACHED:
        case DMN_EVENT_IDE_LOADER_ATTACHED:
        case DMN_EVENT_NAND_ATTACHED:
#if defined(CONFIG_AUTO_USB_STORE_IRSEQ) && defined(CONFIG_QSD_AUTOIR)
		AUTO_IF_Start(SEQUENTIAL_TEST, 0)
#endif

#ifdef SUPPORT_USB_UPGRADE_LONG_PRESS_KEYPAD_POWER
            if(dMessage == DMN_EVENT_USB_HDD_ATTACHED)
            {
                USB_ATTACHED = TRUE;
            }
#endif
#ifdef CONFIG_STORE_MSG_TO_USB
			{
				UINT8 isEnablePrintToUSB = 1;
				ioctl(kmfdev, KMF_IOC_PRINTOFILEENABLE, &isEnablePrintToUSB);
			}
#endif
			iRet = APP_Data_USB_Upgrade_ChecFileAndBuildTime();

			//mainapp_printf("[L%d] DEV_ATTACHED(%d) received.\n", __LINE__,dMessage);
			switch (MAINAPP_GetActiveSystemAppIndex(&dActiveSysApp))
            {
                case MAIN_APP_IN_TRANSITION:
                    //mainapp_printf("[L%d] In Transition State, wait and retry.\n",__LINE__);
					_MAINAPP_SetEventInTransition(dMessage, dParam);
                    return;

				case MAIN_APP_SUCCESS:
                    //mainapp_printf("[L%d] MAIN_APP_SUCCESS!!!\n",__LINE__);
                    break;

                default:
					mainapp_printf("[%s %d] In Invalid State\n",__FUNCTION__,__LINE__);
                    return;
            }
			_MAINAPP_ClearEventInTransition(dMessage, dParam);

#if defined(CONFIG_SUPPORT_USB_AUTO_UPGRADE) 
	#if defined(CONFIG_CIPLUS_SUPPORT)
			updateCI = APP_Data_CIKey_Upgrade_AutoStart(iRet);
	#endif
			updateHDCP = APP_Data_HDCPKey_Upgrade_AutoStart(iRet);
	
			dKeyparam = (updateHDCP<<4) | updateCI;
			dKeyUpgradeEvent = APP_GLOBAL_EVENT_CIHDCPKEY_RESULT | PASS_TO_SYSAPP;
			SYSAPP_IF_SendGlobalEventWithIndex(dActiveSysApp, dKeyUpgradeEvent, dKeyparam);
#endif


#ifdef NET_SUPPORT
            MID_PartitionList_Mount(sdPtIdx, MntModule_Network);
#ifdef NET_ET_SUPPORT
            _MAINAPP_EtMountHandle(pstEventInfo, dParam);
#endif
#ifdef NET_N32_SUPPORT
            _MAINAPP_N32MountHandle(pstEventInfo, dParam);
#endif
#endif
#ifdef NET_SUPPORT
#ifdef NET_APP_SUPPORT
            pcMountName = (char *)GL_MemAlloc(_SPAL_PARTITION_NAME_MAX_LENGTH);
            memset(pcMountName, 0, _SPAL_PARTITION_NAME_MAX_LENGTH2);
            iPtnHandle = MID_PartitionList_FindItem(dParam);
            if (iPtnHandle == -1)
            {
                mainapp_printf("get ptn handle\n");
            }
            if (MID_PartitionList_GetMountName(iPtnHandle, pcMountName) == NULL)
            {
                mainapp_printf("Get Mount Name\n");
            }
            if (strcmp(pcMountName, stNetConfigStorage.stParName) == 0)
            {
                Net_Cfg_SetDlPath(stNetConfigStorage.stParName);
            }
            if (NULL != pcMountName)
            {
                GL_MemFree(pcMountName);
            }
            if (stNetConfigStorage.u8IsSetNetConfigStorage == 0)
            {
                stNetConfigStorage.u8IsSetNetConfigStorage = 1;
                stNetConfigStorage.u8NetConfigStorage = 0;
                MID_PartitionList_GetMountName((int)stNetConfigStorage.u8NetConfigStorage, stNetConfigStorage.stParName);
                Net_Cfg_SetDlPath(stNetConfigStorage.stParName);
            }
#endif
#endif
#if 0//def CONFIG_SUPPORT_PVR
			int iPtListIdx = -1;
			if (MID_RecorderGetDefaultPartition(&iPtListIdx) != MID_REC_FS_OK)
			{
				/* aps new don't support NTFS fs */
				if (APP_GUIOBJ_DVB_PartitionList_GetFirstFat(&iPtListIdx) == TRUE)
				{
					MID_RecorderSetDefaultPartition(iPtListIdx, MID_REC_PARTITION_SET_FORCE); //set partition 0 for default partition
				}
			}
#endif
		    SYSAPP_IF_SendGlobalEventWithIndex(dActiveSysApp, dMessage, (UINT32)sdPtIdx);

#ifdef CONFIG_SUPPORT_OTA_UPGRADE
			if(TRUE == DVBApp_GetOTAStatus())
			{
				break;
			}
#endif

#if (defined(CONFIG_SUPPORT_USB_AUTO_UPGRADE) && defined(CONFIG_CIPLUS_SUPPORT))
    		if(g_CheckUpgradeCIPLUS == FALSE)
    		{
    			#if 0
    			if (APP_Data_USB_Upgrade_CIPlus_Mergerom_Get_ValidPath() == AL_SUCCESS)
    			{
    				APP_CIPlus_AutoInstall_Process(1); //Auto-Install Mergerom : 1, Auto-Install Product key : 0
    				g_CheckUpgradeCIPLUS = TRUE;
    				break;
    			}
    			else
    			#endif
    			#if 0 // TODO: later do this
    			if (APP_Data_USB_Upgrade_CIPlus_KeyInstall_Get_ValidPath() == AL_SUCCESS)
    			{
    				APP_CIPlus_AutoInstall_Process(0); //Auto-Install Mergerom : 1, Auto-Install Product key : 0
    				g_CheckUpgradeCIPLUS = TRUE;
    				break;
    			}
    			#endif
    		}
#endif

#ifdef SUPPORT_FACTORY_AUTO_TEST
			if (APP_Factory_GetAutoTestOnOff() == TRUE)
			{
				Cmd_SetPanelBacklightPower(1);
#ifndef SUPPORT_FACTORY_BURNING_ADJ_TEST
#if (SYSCONF_BOARD_POWER == 0)
						Backlight_t BacklightSetting;
						BacklightSetting.Backlight_total_Stage = 100; // set total backlight stage = 100
						BacklightSetting.OSD_backlight_index = 100;
						Cmd_SetLcdBackLight(BacklightSetting);// when into	FACTORY Mode BackLight must be max
#endif
#endif
			}
			if ((APP_Factory_GetAutoTestOnOff() == TRUE)&&(iRet == AL_FAILURE))
			{
				extern void Enable_Debug_Message(UINT32 DBGStatus);
				Enable_Debug_Message(1<<MODULEID_UMF);
				//mainapp_printf("Test Start\n");
				Enable_Debug_Message(0);
				if(AcPowerOnNoRomBinFlag== FALSE)
				 AcPowerOnNoRomBinFlag= TRUE;
			}
#endif
#ifdef SUPPORT_FACTORY_BURNING_ADJ_TEST
			extern UINT8 APP_GetBacklightValueByCurrentValue(UINT16 Current_Value);
			UINT16 Current_Value = 0;
			Backlight_t Backlight;
			Backlight.Backlight_total_Stage = 100; // set total backlight stage = 100
			AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
			            sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
			Current_Value = APP_Clone_Check_UDiskFile_GetValue();

			Backlight.OSD_backlight_index = APP_GetBacklightValueByCurrentValue(Current_Value);

			if(Backlight.OSD_backlight_index <= 100)
			{
				printf("\n Cmd_SetLcdBackLight:Backlight_Value==%d\n", Backlight.OSD_backlight_index);
				Cmd_SetLcdBackLight(Backlight);	
				g_stSetupData.HomeMode.Backlight[g_stSetupData.HomeMode.Type]=  Backlight.OSD_backlight_index;
			    AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,
			                     ITEM_OFFSET(APP_SETTING_Setup_t, HomeMode.Backlight),
			                     sizeof(g_stSetupData.HomeMode.Backlight), &(g_stSetupData.HomeMode.Backlight));


				AL_Setting_Store(APP_Data_UserSetting_Handle(),SYS_SET_ID_SETUP,
					ITEM_OFFSET(APP_SETTING_Setup_t,HomeMode.Backlight),
					sizeof(g_stSetupData.HomeMode.Backlight));
			}

#endif

			if(g_CheckUpgrade)
			{
				break;
			}
            if((iRet == AL_SUCCESS)
	#ifdef  SUPPORT_WAKEUP_TIMER_IN_STANDBY
                && (APP_WAKEUP_GetBootFlag() == FALSE)
	#endif
                )
            {
                mainapp_printf("[L%d] APP_Data_USB_Upgrade_ChecFileAndBuildTime() Success!!!\n",__LINE__);

	#ifdef SUPPORT_FACTORY_AUTO_TEST
				extern int APP_Factory_GetAutoTestOnOff(void);
				extern int APP_Factory_GetUpgradeandAutoTestFlag(void);
				extern void APP_Factory_SetUpgradeandAutoTestFlag(Boolean Flag);

				if(AcPowerOnNoRomBinFlag == TRUE)
				{
					mainapp_printf("In AutoTest Factory mode\n");
				}
				else if((APP_Factory_GetAutoTestOnOff() == TRUE)&&(APP_Factory_GetUpgradeandAutoTestFlag() == FALSE))
				{
					dMessage = APP_GLOBAL_EVENT_USB_UPGRADE | PASS_TO_SYSAPP;
					gFACTESTResentMessage.dResendMessage = dMessage;
					gFACTESTResentMessage.dResendMessageParm = 0;
					//SYSAPP_IF_SendGlobalEventWithIndex(dActiveSysApp, dMessage, 0); // 1:EraseAll,0:not EraseAll
				    //APP_Factory_SetAutoTestOnOff(FALSE);
					//APP_Factory_SetUpgradeandAutoTestFlag(TRUE);
					//g_CheckUpgrade = TRUE;
				}
				else if((APP_Factory_GetAutoTestOnOff() == TRUE)&&(APP_Factory_GetUpgradeandAutoTestFlag() == TRUE))
				{
				   mainapp_printf("In AutoTest Factory mode\n");
				}
				else
				{
					#ifndef CONFIG_SUPPORT_USB_AUTO_UPGRADE
					if(FALSE == gUSBUpgradeBinIsSupperBin)
					{
						break;					
					}
					#endif
					dMessage = APP_GLOBAL_EVENT_USB_UPGRADE | PASS_TO_SYSAPP;
					SYSAPP_IF_SendGlobalEventWithIndex(dActiveSysApp, dMessage, 0);
					g_CheckUpgrade = TRUE;
				}
	#else
				#ifndef CONFIG_SUPPORT_USB_AUTO_UPGRADE
				if(FALSE == gUSBUpgradeBinIsSupperBin)
				{
					break;					
				}
				#endif
				dMessage = APP_GLOBAL_EVENT_USB_UPGRADE | PASS_TO_SYSAPP;
				SYSAPP_IF_SendGlobalEventWithIndex(dActiveSysApp, dMessage, 0);
				g_CheckUpgrade = TRUE;
	#endif
            }
            else
            {
				if(dMessage == DMN_EVENT_USB_HDD_ATTACHED)
				{
#ifdef CONFIRM_AUTO_USB
					UINT32 u32PopMsgName = POPMSG_PROMPT_NO_PROG;
					APP_GUIOBJ_PopMsg_GetMsgDialogType(&u32PopMsgName);
					if((u32PopMsgName!=POPMSG_CONFIRM_START_AUTO_USB)
#if (defined(CONFIG_SUPPORT_USB_AUTO_UPGRADE) && defined(CONFIG_CIPLUS_SUPPORT))
					&& (APP_Data_CIKey_Upgrade_Get_ValidPath() == AL_FAILURE)
#endif
					)
					{
						dMessage = APP_GLOBAL_EVENT_USB_AUTOUSB | PASS_TO_SYSAPP;
						SYSAPP_IF_SendGlobalEventWithIndex(dActiveSysApp, dMessage, 1);
					}
#endif


#ifdef N32_GAME_SUPPORT
					UINT8 ret;
					ret = APP_Clone_Game_First_Check();

					if(ret == SC_SUCCESS || ret == SC_ERR_SIZE_NOT_ENOUGH)
					{
						dMessage = APP_GLOBAL_EVENT_GAME_UPGRADE | PASS_TO_SYSAPP;
						SYSAPP_IF_SendGlobalEventWithIndex(dActiveSysApp, dMessage, ret);
					}
#endif
				}
            }
            break;

        case DMN_EVENT_USB_HDD_DETACHED:
        case DMN_EVENT_USB_LOADER_DETACHED:
        case DMN_EVENT_CARD_DEV_DETACHED:
        case DMN_EVENT_IDE_HDD_DETACHED:
        case DMN_EVENT_IDE_LOADER_DETACHED:
        case DMN_EVENT_NAND_DETACHED:
            mainapp_printf(" DEV_DETACHED(%d) received.\n", dMessage);

			switch (MAINAPP_GetActiveSystemAppIndex(&dActiveSysApp))
            {
                case MAIN_APP_IN_TRANSITION:
                    //mainapp_printf("[L%d] In Transition State, wait and retry.\n",__LINE__);
					_MAINAPP_SetEventInTransition(dMessage, dParam);
                    return;

				case MAIN_APP_SUCCESS:
                    //mainapp_printf("[L%d] MAIN_APP_SUCCESS!!!\n",__LINE__);
                    break;

                default:
					mainapp_printf("[%s %d] In Invalid State\n",__FUNCTION__,__LINE__);
                    return;
            }

			_MAINAPP_ClearEventInTransition(dMessage, dParam);

			if(dMessage == DMN_EVENT_USB_HDD_DETACHED)
			{
				g_CheckUpgrade = FALSE;
#ifdef SUPPORT_USB_UPGRADE_LONG_PRESS_KEYPAD_POWER
                USB_ATTACHED = FALSE;
#endif
			}
#ifdef CONFIG_CONFIRM_AUTO_UPGRADE
			UINT32 u32PopMsgName;
		 	APP_GUIOBJ_PopMsg_GetMsgDialogType(&u32PopMsgName);
		 	if(u32PopMsgName == POPMSG_CONFIRM_START_AUTO_UPGRADE)
			{
#ifdef CONFIG_ATV_SUPPORT
				if(SYS_APP_ATV == dActiveSysApp)
				{
					ATVApp_ClosePopup(POPMSG_CONFIRM_START_AUTO_UPGRADE, UI_EVENT_NULL);
				}
#endif
#ifdef CONFIG_DTV_SUPPORT
				if(SYS_APP_DVB== dActiveSysApp)
				{
					DVBApp_ClosePopup(POPMSG_CONFIRM_START_AUTO_UPGRADE, UI_EVENT_NULL);
				}
#endif
			}
#endif
#ifdef NET_ET_SUPPORT
            _MAINAPP_EtUnmountHandle(pstEventInfo, dParam);
#endif
#ifdef NET_N32_SUPPORT
            _MAINAPP_N32UnmountHandle(pstEventInfo, dParam);
#endif

#ifdef NET_SUPPORT
#ifdef NET_APP_SUPPORT
#if defined(CONFIG_DVB_SYSTEM_DVBC_SUPPORT)
            Dvb_GUIOBJ_Networking_PartitionInfoUpdate();
#endif
            pcMountName = (char *)GL_MemAlloc(_SPAL_PARTITION_NAME_MAX_LENGTH);
            memset(pcMountName, 0, _SPAL_PARTITION_NAME_MAX_LENGTH2);
            for (i = 0; i < MID_PartitionList_GetMountedCount(); i++)
            {
                MID_PartitionList_GetMountName(i, pcMountName);
                if (strcmp(pcMountName, stNetConfigStorage.stParName) == 0)
                {
                    bFlag++;
                }
            }
            if (NULL != pcMountName)
            {
                GL_MemFree(pcMountName);
            }
            if (bFlag == 0)
            {
                if (MID_PartitionList_GetMountedCount() == 0)
                {
                    memset(stNetConfigStorage.stParName, 0, sizeof(char)*_SPAL_PARTITION_NAME_MAX_LENGTH);
                    stNetConfigStorage.u8IsSetNetConfigStorage = 0;
                    stNetConfigStorage.u8NetConfigStorage = 0;
                }
                else
                {
                    stNetConfigStorage.u8IsSetNetConfigStorage = 1;
                    stNetConfigStorage.u8NetConfigStorage = 0;
                    MID_PartitionList_GetMountName((int)stNetConfigStorage.u8NetConfigStorage, stNetConfigStorage.stParName);
                }
            }
            Net_Cfg_SetDlPath(stNetConfigStorage.stParName);
#endif
#endif
			SYSAPP_IF_SendGlobalEventWithIndex(dActiveSysApp, dMessage, (UINT32)sdPtIdx);
            break;

        case DMN_EVENT_SCREEN_SAVER_ON:
            mainapp_printf(" DMN_EVENT_SCREEN_SAVER_ON received.\n");
            break;

        case DMN_EVENT_SCREEN_SAVER_OFF:
            mainapp_printf(" DMN_EVENT_SCREEN_SAVER_OFF received.\n");
            break;

#ifdef NET_SUPPORT
        case DMN_EVENT_NET_LINKER_DETACHED:
            mainapp_printf("DMN_EVENT_NET_LINKER_DETACHED received.\n");
#if defined( SAMBA_SUPPORT ) && defined( SUPPORT_ETHERNET )
            DVB_Network_UnMountSmbFs();
#endif
            pstEventInfo->stNetDriveCtrlInfo.eState = NET_DRIVE_STATE_SHUT_DOWN;

            if (MAINAPP_GetActiveSystemAppIndex(&dActiveSysApp) == MAIN_APP_SUCCESS)
            {
                SYSAPP_IF_SendGlobalEventWithIndex(dActiveSysApp, dMessage, dParam);
            }
            break;

        case DMN_EVENT_NET_LINKER_ATTACHED:
            mainapp_printf("DMN_EVENT_NET_LINKER_ATTACHED received.\n");
#if defined( SAMBA_SUPPORT ) && defined( SUPPORT_ETHERNET )
            DVB_Network_MountSmbFs();
#endif
            pstEventInfo->stNetDriveCtrlInfo.eState = NET_DRIVE_STATE_BOOTUP;

            if (MAINAPP_GetActiveSystemAppIndex(&dActiveSysApp) == MAIN_APP_SUCCESS)
            {
                SYSAPP_IF_SendGlobalEventWithIndex(dActiveSysApp, dMessage, dParam);
            }
            break;
#endif
        case DMN_EVENT_MAINAPP_STARTS:
            mainapp_printf(" DMN_EVENT_MAINAPP_STARTS received.\n");
#ifdef NET_SUPPORT
            MAINAPP_NetAPInit();
#endif
            break;

        default:
            mainapp_printf(" Un-handled Daemon Eventid: 0x%x.\n", dMessage);
            break;
    }

    return;
}

/*****************************************************************************
** FUNCTION : MAINAPP_OnTimerUpdate
**
** DESCRIPTION :
**				main app 100ms message handler
**
** PARAMETERS :
**				None
**
** RETURN VALUES:
**				None
*****************************************************************************/

int MAINAPP_OnTimerUpdate(void)
{
    UINT32 dIndex;
/*mark for S2tek */

#ifdef SUPPORT_FACTORY_AUTO_TEST
	if((APP_Factory_GetAutoTestOnOff() == TRUE) && (APP_Factory_GetUpgradeandAutoTestFlag() == FALSE))
	{
		static UINT8 CheckCount = 0;
		//100ms * 50
		CheckCount++;
		CheckCount %= 255;
		if(CheckCount >= 50 && gFACTESTResentMessage.dResendMessage != 0) 
		{
			APP_Factory_SetAutoTestOnOff(FALSE);
			APP_Factory_SetUpgradeandAutoTestFlag(TRUE);
			MSG_FILTER_DispatchMessage(gFACTESTResentMessage.dResendMessage, gFACTESTResentMessage.dResendMessageParm);
			g_CheckUpgrade = TRUE;
			gFACTESTResentMessage.dResendMessage = 0;
			gFACTESTResentMessage.dResendMessageParm= 0;
		}
	}
#endif
    _MAINAPP_ResendEventInTransition();
    AL_Time_Timeout();

#ifdef CONFIG_SUPPORT_PVR
	if((MID_REC_MODE_TIMESHIFT_AFTER_REC == MID_Recorder_GetRecMode())
		&&(PVR_STATE_STOP != APP_GUIOBJ_DVB_PvrPlayInfo_GetRecState())
		&&(PVR_STATE_PLAY != APP_GUIOBJ_DVB_PvrPlayInfo_GetRecState()))
	{
		if(g_bNeedReset == TRUE)
		{
			g_u32TimeAddOffset = 0;;
		}
		if(!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_PVR_PLAYINFO))
		{
			if(dPvrinfoCount++ >=10)
			{
				dPvrinfoCount = 0;
				g_u32TimeAddOffset ++;
			}
		}
	
	}
#endif
    if (APP_SleepTimer_GetDetectFlag())
    {
    	if(!(APP_MenuMgr_Exist_Factory_Menu()))
    	{
	        if (APP_GUIOBJ_SleepTimer_Timeout())
	        {
#ifdef CONFIG_ATV_SUPPORT
	        	if(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV, APP_GUIOBJ_SLEEP_INFO))
	    		{
	    			APP_GUIOBJ_SleepTimer_SetSleepPopConflict(TRUE);
	    		}
#endif
	            MAINAPP_GetActiveSystemAppIndex(&dIndex);
	            SYSAPP_IF_SendGlobalEventWithIndex(dIndex,
	                                               (APP_GLOBAL_EVENT_SLEEP_TIMER_INFO | PASS_TO_SYSAPP), APP_POWERDOWN_SLEEP);
	            APP_SleepTimer_SetDetectFlag(FALSE);
	        }
    	}
    }
#ifdef CELLO_ALARM_TV
	if(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, APP_GUIOBJ_SYSTEM_TIME))
	{
		;
	}
	else
#endif	
    if (APP_AutoPowerOff_Timeout())
    {
    	MAINAPP_GetActiveSystemAppIndex(&dIndex);
    	if (SYSAPP_GOBJ_GUIObjectExist(dIndex, APP_GUIOBJ_SLEEP_INFO) && (APP_GUIOBJ_SleepTimer_GetStandbyPopConflict() == FALSE))
        {
        	APP_GUIOBJ_SleepTimer_SetStandbyPopConflict(TRUE);
        }
		SYSAPP_IF_SendGlobalEventWithIndex(dIndex,
	      									(APP_GLOBAL_EVENT_SLEEP_TIMER_INFO | PASS_TO_SYSAPP), APP_POWERDOWN_AUTOPOWEROFF);
		#if 0
		while(!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, APP_GUIOBJ_SLEEP_INFO))
		{
        	MAINAPP_SendGlobalEvent(UI_EVENT_POWER, AL_POWER_STATE_OFF);
        }
        #endif
    }
#ifdef TIANLE_Board_Time
	BoardCountTimer++;
	BoardCountTimer_M++;
	if(BoardCountTimer >= 36000)
	{
		//AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,0,
		//	sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
    	//g_stSetupData.BoardTime += 3600;
    	//AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,
		//	ITEM_OFFSET(APP_SETTING_Setup_t, BoardTime),
	 	//	sizeof(g_stSetupData.BoardTime),&(g_stSetupData.BoardTime));
    	AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
    		sizeof(APP_SETTING_Setup_t));
    	BoardCountTimer = 0;
		BoardCountTimer_M = 0;
	}
	if(BoardCountTimer_M >= 600)
	{
		AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,0,
			sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
    	g_stSetupData.BoardTime += 60;
    	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,
			ITEM_OFFSET(APP_SETTING_Setup_t, BoardTime),
	 		sizeof(g_stSetupData.BoardTime),&(g_stSetupData.BoardTime));
		BoardCountTimer_M = 0;
	}
#endif
#ifdef TEAC_SYSTEMINFO_SUPPORT
    PanelCountTimer ++;
	if(PanelCountTimer == 36000)
	{
		currtime = GL_GetRtc32()/1000;
		AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,0,
			sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
    	g_stSetupData.PanelTime += 3600;
    	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,
			ITEM_OFFSET(APP_SETTING_Setup_t, PanelTime),
	 		sizeof(g_stSetupData.PanelTime),&(g_stSetupData.PanelTime));
    	AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
    		sizeof(APP_SETTING_Setup_t));
    	PanelCountTimer = 0;
	}

	ePrevSourType = eCurrSourType;
	APP_GUIOBJ_Source_GetCurrSource(&eCurrSourType);
	if((ePrevSourType != APP_SOURCE_DVD)&&(eCurrSourType == APP_SOURCE_DVD))
	{
		DVDcurrtime = GL_GetRtc32();
	}
	else if((ePrevSourType == APP_SOURCE_DVD)&&(eCurrSourType != APP_SOURCE_DVD))
	{
		DVDusedtime = (GL_GetRtc32() - DVDcurrtime)/1000;
		AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,0,
			sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
		g_stSetupData.DVDTime += DVDusedtime;
    	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,
			ITEM_OFFSET(APP_SETTING_Setup_t, DVDTime),
	 		sizeof(g_stSetupData.DVDTime),&(g_stSetupData.DVDTime));
    	AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
    		sizeof(APP_SETTING_Setup_t));
	}
	if(eCurrSourType == APP_SOURCE_DVD )
	{
		DVDCountTimer ++;
		if(DVDCountTimer == 36000)
		{
			DVDcurrtime = GL_GetRtc32();
			AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,0,
				sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
    		g_stSetupData.DVDTime += 3600;
    		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,
				ITEM_OFFSET(APP_SETTING_Setup_t, DVDTime),
	 			sizeof(g_stSetupData.DVDTime),&(g_stSetupData.DVDTime));
    		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
    			sizeof(APP_SETTING_Setup_t));
    		DVDCountTimer = 0;
    	}
	}
#endif
#ifdef CONFIG_SUPPORT_SYSTEM_LIFETIME
    SystemLifeTimerCount ++;
	if(SystemLifeTimerCount >= 600*SAVE_SYSTEM_LIFETIME_TIME)
	{
		MAINAPP_UpdateSystemLifeTimeToFlash(TRUE);
	}
#endif
#ifdef SUPPORT_SHOP_DEMO_MODE
    if (APP_ShopDemo_Timeout())
    {
		APP_GUIOBJ_Picture_SetPictureMode(TV_IDS_String_Picture_dynamic);
		APP_GUIOBJ_Picture_SetColourTemp(TV_IDS_String_Standard);
	    APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_BACKLIGHT, 100);
    }
#endif

#ifdef SUPPORT_SHOP_DEMO_MODE_IN_MAINMENU
	_MAINAPP_SHOP_mode();
#endif

#ifdef CONFIG_SUPPORT_3D_EN
	if(APP_GUIOBJ_3DMenu_Timer())
	{
		MAINAPP_GetActiveSystemAppIndex(&dIndex);
        if ((!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_SCAN_PROCESS))
            && (!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_AUTO_SEARCH))
            && (!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV, ATV_GUIOBJ_SCAN_PROCESS))
            && (!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV, ATV_GUIOBJ_ANALOG_MANUAL_SEARCH))
            && (!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV, ATV_GUIOBJ_AMS_SEARCH))
            && (!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV, ATV_GUIOBJ_AMS_FINE_TUNE))
#ifdef CONFIG_SUPPORT_USB_UPGRADE
            && (!SYSAPP_GOBJ_GUIObjectExist(dIndex, APP_GUIOBJ_SOFTWARE_UPGRADE))
#endif
#if (defined CONFIG_DVB_SYSTEM_DVBT_SUPPORT)
            && (!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_DVB_T_MANUAL_SEARCH))
#endif
#if (defined CONFIG_DVB_SYSTEM_DVBC_SUPPORT)
            && (!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV, ATV_GUIOBJ_DVBC_AUTO))
            && (!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_DVBC_AUTO))
            && (!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_DVB_C_MANUAL_SEARCH))
#endif
#if (defined CONFIG_DVB_SYSTEM_DVBS_SUPPORT)
            && (!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV, ATV_GUIOBJ_DVBS_AUTO))
            && (!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_DVBS_SCAN))
            && (!SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_DVBS_AUTO))
#endif
            )
        {
            APP_GUIOBJ_3DMenu_TimerCountdown(0);
    		SYSAPP_IF_SendGlobalEventWithIndex(dIndex,
    			(APP_GLOBAL_EVENT_3D_TIMER_INFO|PASS_TO_SYSAPP),0);
        }
	}
#endif

#ifdef CONFIG_SUPPORT_PVR
    APP_WAKEUP_Recorder_Timeout();
#ifdef AC_ON_AUTO_GET_TIME
    APP_BackgroundGetTime_Timeout();
#endif
#endif

#ifdef TEAC_ONOFF_TIMER_SUPPORT
#ifdef CELLO_ALARM_TV
	APP_SleepOnTimerCheck();
#endif
    APP_OffTimer_Check();
    APP_OnTimer_Check_WhenPanelOff();
#endif
#ifdef CONFIG_CIPLUS_SUPPORT
	if (g_CIPlus_update.isUpdate)
	{
		if (0 == g_CIPlus_update.updateTime)
		{
			g_CIPlus_update.isUpdate = FALSE;
			MAINAPP_GetActiveSystemAppIndex(&dIndex);
			SYSAPP_IF_SendGlobalEventWithIndex(dIndex,(APP_GLOBAL_EVENT_ASK_PROFILE_ACTION | PASS_TO_SYSAPP), 0);
		}
		g_CIPlus_update.updateTime--;
	}
#endif

#ifdef SUPPORT_CEC_TV
#ifndef SUPPORT_UI_ARC_ITEM_HIGHLIGHT
	//Add for update amp volume
	static UINT8 CheckGetAMPVolumeCount = 0;
	CheckGetAMPVolumeCount++;
	if(CheckGetAMPVolumeCount>=10)//100ms * 10
	{
		CheckGetAMPVolumeCount = 0;
		APP_Process_CECTV_GetCurrentAMPVolume();
	}
#endif
#endif

	return SP_SUCCESS;
}

/*****************************************************************************
** FUNCTION : MAINAPP_OnEvent
**
** DESCRIPTION :
**				main app external event handler
**
** PARAMETERS :
**				dMessage : message
**				dParam : the parameter for dMessage
**
** RETURN VALUES:
**				None
*****************************************************************************/
int MAINAPP_OnEvent(UINT32 dMessage, UINT32 dParam)
{
	if((dMessage < UI_EVENT_NULL)
		|| ((dMessage > APP_GLOBAL_EVENT_MIN) && (dMessage < APP_GLOBAL_EVENT_MAX)))
	{
        //mainapp_printf("==================_MAINAPP_UIEventHandler()=================\n");
		_MAINAPP_UIEventHandler(dMessage, dParam);
	}
	else if ((dMessage > DMN_EVENT_MIN && dMessage < DMN_EVENT_MAX)
		|| (dMessage > DMN_EVENT_DEVICE_DETECT_MIN && dMessage < DMN_EVENT_DEVICE_DETECT_MAX))
	{
        //mainapp_printf("==================_MAINAPP_DaemonEventHandler()=================\n");
		_MAINAPP_DaemonEventHandler(dMessage, dParam);
	}
	else if (dMessage > GUI_RESPOND_MIN && dMessage < GUI_RESPOND_MAX)
	{
        //mainapp_printf("==================_MAINAPP_GUIFeedbackEventHandler()=================\n");
		_MAINAPP_GUIFeedbackEventHandler(dMessage, dParam);
	}

	return SP_SUCCESS;
}

/*****************************************************************************
** FUNCTION : MAINAPP_GetPowerOffState
**
** DESCRIPTION :
**				Get power off state
**
** PARAMETERS :
**				None
**
** RETURN VALUES:
**				g_bPowerOffFlag: power off state
*****************************************************************************/
bool MAINAPP_GetPowerOffState(void)
{
	return g_bPowerOffFlag;
}

/*****************************************************************************
** FUNCTION : MAINAPP_SetPowerOffState
**
** DESCRIPTION :
**				Set power off state
**
** PARAMETERS :
**				PowerOffFlag - power off state
**
** RETURN VALUES:
**				None
*****************************************************************************/
void MAINAPP_SetPowerOffState(bool PowerOffFlag)
{
	g_bPowerOffFlag = PowerOffFlag;
}

#ifdef NET_ET_SUPPORT
static void MAINAPP_NetAPInit(void)
{
    NetDaemonCfg_t stNetDaemon;
    stNetDaemon.pfEventCallBack = NetDaemon_callback;
#ifdef NET_WIFI_SUPPORT
    Net_WifiFlashiInit();
#endif

    if (NET_IF_Initialize(&stNetDaemon) != NET_DAEMON_IF_SUCCESSFUL)
    {
        mainapp_printf("ERROR: init net daemon\n");
    }

    if (APP_NET_Initialize() != APP_NET_IF_SUCCESSFUL)
    {
        mainapp_printf("ERROR: app init net daemon\n");
    }

#ifdef SUPPORT_NET_TESTING
    if (NetTest_Init(1) != 0)
    {
        mainapp_printf("ERROR: Init Net Testing Thread\n");
    }
#endif
}

MainAppUIEventHandlerInfo_t *MAINAPP_GetNetHandlerInfo(void)
{
    return &stUIEventHandlerInfo;
}

static void _MAINAPP_EtMountHandle(MainAppUIEventHandlerInfo_t *pstEventInfo, UINT32 dDevNum)
{
    int iPtnHandle = 0;
    char *pszMountPonit = NULL;

    if (!pstEventInfo)
    {
        mainapp_printf("%s\n", __FUNCTION__);
        return;
    }

    pszMountPonit = GL_MemAlloc(ET_PATH_NAME_MAX_LENTH);
    if (!pszMountPonit)
    {
        mainapp_printf("alloc et new path name\n");
        return;
    }
    memset(pszMountPonit, 0, ET_PATH_NAME_MAX_LENTH);
    do
    {
        iPtnHandle = MID_PartitionList_FindItem(dDevNum);
        if (iPtnHandle == -1)
        {
            mainapp_printf("invalidate ptn handle\n");
            break;
        }

        if (MID_PartitionList_GetMountName(iPtnHandle, pszMountPonit) == NULL)
        {
            mainapp_printf("get ptn mount point error!\n");
            break;
        }
        pstEventInfo->stSysDriveCtrlInfo.iSysCurPtnHnd = iPtnHandle;
    }
    while (0);
    GL_MemFree(pszMountPonit);
}

static void _MAINAPP_EtUnmountHandle(MainAppUIEventHandlerInfo_t *pstEventInfo, UINT32 dDevNum)
{
    int iPtnHandle = 0;
    char *pszMountPonit = NULL;

    if (!pstEventInfo)
    {
        mainapp_printf("%s\n", __FUNCTION__);
        return;
    }

    pszMountPonit = GL_MemAlloc(ET_PATH_NAME_MAX_LENTH);
    if (!pszMountPonit)
    {
        mainapp_printf("alloc et new path name\n");
        return;
    }
    memset(pszMountPonit, 0, ET_PATH_NAME_MAX_LENTH);
    do
    {
        iPtnHandle = MID_PartitionList_FindItem(dDevNum);
        if (iPtnHandle == -1)
        {
            mainapp_printf("invalidate ptn handle\n");
            break;
        }

        if (MID_PartitionList_GetMountName(iPtnHandle, pszMountPonit) == NULL)
        {
            mainapp_printf("get ptn mount point error!\n");
            break;
        }
#if 0
        sdReturnValue = NET_IF_InstOps(NET_DAEMON_INST_ET, NET_DAEMON_INST_OPS_UNMOUNT_CHECK, (UINT32)pszMountPonit);
        if (sdReturnValue < NET_DAEMON_IF_SUCCESSFUL)
        {
            mainapp_printf("boot up check et download\n");
            break;
        }
#endif
        pstEventInfo->stSysDriveCtrlInfo.iSysCurPtnHnd = iPtnHandle;
    }
    while (0);
    GL_MemFree(pszMountPonit);
}
#endif

#ifdef NET_N32_SUPPORT
static void _MAINAPP_N32MountHandle(MainAppUIEventHandlerInfo_t *pstEventInfo, UINT32 dDevNum)
{
    INT32 sdReturnValue = MAIN_APP_SUCCESS;
    int iPtnHandle = 0;
    char *pszMountPonit = NULL;

    if (!pstEventInfo)
    {
        mainapp_printf("%s\n", __FUNCTION__);
        return;
    }

    pszMountPonit = GL_MemAlloc(DLMGR_IF_FILEPATH_MAX_LEN);
    if (!pszMountPonit)
    {
        mainapp_printf("alloc et new path name\n");
        return;
    }
    memset(pszMountPonit, 0, DLMGR_IF_FILEPATH_MAX_LEN);
    do
    {
        iPtnHandle = MID_PartitionList_FindItem(dDevNum);
        if (iPtnHandle == -1)
        {
            mainapp_printf("get ptn handle\n");
            break;
        }

        if (MID_PartitionList_GetMountName(iPtnHandle, pszMountPonit) == NULL)
        {
            mainapp_printf("get ptn mount point error!\n");
            break;
        }
        //add by ztli 2010/08/04 for mantis bug 96324 start {
        if (iPtnHandle == 0)
        {
            Net_Cfg_SetDlPath(pszMountPonit);
        }
        //add by ztli end }
        //sdReturnValue = NET_IF_InstOps(NET_DAEMON_INST_N32,NET_DAEMON_N32_INST_OPS_MOUNT_CHECK,(UINT32)pszMountPonit);
        //if(sdReturnValue < NET_DAEMON_IF_SUCCESSFUL)
        sdReturnValue = APP_NET_InstCtrl(APP_NET_INST_N32, APP_NET_INST_OPS_MOUNT, (UINT32)pszMountPonit);
        if (sdReturnValue < APP_NET_IF_SUCCESSFUL)
        {
            mainapp_printf("boot up check et download\n");
            break;
        }

    }
    while (0);
    GL_MemFree(pszMountPonit);
}

static void _MAINAPP_N32UnmountHandle(MainAppUIEventHandlerInfo_t *pstEventInfo, UINT32 dDevNum)
{
    INT32 sdReturnValue = MAIN_APP_SUCCESS;
    int iPtnHandle = 0;
    char *pszMountPonit = NULL;

    if (!pstEventInfo)
    {
        mainapp_printf("%s\n", __FUNCTION__);
        return;
    }

    pszMountPonit = GL_MemAlloc(DLMGR_IF_FILEPATH_MAX_LEN);
    if (!pszMountPonit)
    {
        mainapp_printf("alloc et new path name\n");
        return;
    }
    memset(pszMountPonit, 0, DLMGR_IF_FILEPATH_MAX_LEN);
    do
    {
        iPtnHandle = MID_PartitionList_FindItem(dDevNum);
        if (iPtnHandle == -1)
        {
            mainapp_printf("invalidate ptn handle\n");
            break;
        }

        if (MID_PartitionList_GetMountName(iPtnHandle, pszMountPonit) == NULL)
        {
            mainapp_printf("get ptn mount point error!\n");
            break;
        }

        //sdReturnValue = NET_IF_InstOps(NET_DAEMON_INST_N32,NET_DAEMON_N32_INST_OPS_UNMOUNT_CHECK,(UINT32)pszMountPonit);
        //if(sdReturnValue < NET_DAEMON_IF_SUCCESSFUL)
        sdReturnValue = APP_NET_InstCtrl(APP_NET_INST_N32, APP_NET_INST_OPS_UNMOUNT, (UINT32)pszMountPonit);
        if (sdReturnValue < APP_NET_IF_SUCCESSFUL)
        {
            mainapp_printf("boot up check et download\n");
            break;
        }
    }
    while (0);
    GL_MemFree(pszMountPonit);
}
#endif


