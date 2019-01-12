/*****************************************************************************
** File: app_factory.c:
**
** Description:
**
** Copyright(c) 2008 Sunmedia Technologies - All Rights Reserved
**
** Author : qin.he
**123123123123
**1222222222222222222222222222
** $Id: app_video.c 1069 2010-11-16 10:32:32Z b.yang_c1 $
*****************************************************************************/
#include "gl_task.h"
#include "sysapp_table.h"
#include "app_event.h"
#include "app_data_setting.h"
#include "app_guiobj_source.h"
#include "app_area_info.h"
#include "app_scart.h"
#include "app_guiobj_fm_Src_Ctrl.h"
#include "app_guiobj_mainmenu.h"
#include "app_guiobj_scan_process.h"
#include "app_guiobj_fm_Src_Ctrl.h"
#include "app_guiobj_cul_fm_factorySetting_new.h"
#include "main_app.h"
#ifdef CONFIG_DTV_SUPPORT
#if defined(CONFIG_DVB_SYSTEM) || defined(CONFIG_AUS_DVB_SYSTEM)
#include "dvb_app.h"
#include "app_dvb_ci_mmi.h"
#include "app_dvb_mheg5.h"
#include "app_dvb_playback.h"
#include "dvb_guiobj_table.h"
#endif
#ifdef CONFIG_ISDB_SYSTEM
#include "sbtvd_app.h"
#include "app_sbtvd_ci_mmi.h"
#include "app_sbtvd_mheg5.h"
#include "app_sbtvd_playback.h"
#include "app_guiobj_sbtvd_table.h"
#include "app_scan_api.h"
#endif
#ifdef CONFIG_SUPPORT_ATV_SCAN_NTSCM_PALM_N
#include "app_scan_api.h"
#endif
#endif
#include "al_fw.h"
#include "app_database.h"
//#include "gpio_table.h"
//#include "pin_config.h"
#include "board_config.h"
#include "ir_map.h"
//#include "keypad_mapping_table.h"
#include "sysapp_table.h"
#ifdef CONFIG_ATV_SUPPORT
#include "atv_app.h"
#include "atv_guiobj_table.h"
#include "app_atv_playback.h"
#include "app_guiobj_atv_playback.h"
#endif
#ifdef SUPPORT_FACTORY_AUTO_TEST
#include "mid_tvfe.h"
//#include "Debug_msg.h"
#include "board_config.h"
#endif
#include "app_guiobj_popmsg.h"
//#include "MM_popmsg_gui.h"
#include "app_data.h"
#ifdef SUPPORT_LED_FLASH
#include "app_led_control.h"
#endif
#include "app_audio.h"
#include "app_global.h"
#if defined(CONFIG_DVB_SYSTEM) || defined(CONFIG_AUS_DVB_SYSTEM)
#include "app_dvb_event.h"
#include "dvb_guiobj_table.h"
#include "dvb_app.h"
#include "mid_partition_list.h"
#include "app_clone_data.h"
#include "mid_usb_edit.h"
#endif
#ifdef CONFIG_ATV_SUPPORT
#include "app_atv_event.h"
#endif
#ifdef CONFIG_MEDIA_SUPPORT
#include "app_fileplayer_event.h"
#endif
#include "keypad_mapping_table.h"
#include "app_ir.h"
#include "customize.h"
#include "app_factory_flash_access.h"  //20140910
#include "app_guiobj_channel.h"
#ifdef CELLO_BATHROOMTV
#include "main_app_external.h"
#endif
#include "drv_tuner_external.h"
#include "app_console.h"


static APP_SourceConfig_t g_stSourceConfigTable_New[APP_SOURCE_MAX];
static UINT16 g_u16SourceConfigTable_Size_New;
#ifdef AC_ON_AUTO_GET_TIME
extern Boolean g_fBackgroundGetTime;
#endif

#ifdef SUPPORT_USB_UPGRADE_LONG_PRESS_KEYPAD_POWER
extern UINT8 IsKeypadPowerOnPressRepeat;
extern UINT32 USB_ATTACHED;
#endif
bool g_bNeedReopenWBajust = FALSE;
bool g_bNeedReopenCurveSetting = FALSE;
bool g_bNeedReopenPicture = FALSE;
#if 1//def SUPPORT_FACTORY_AUTO_TEST
extern int APP_Factory_GetAutoTestOnOff(void);
extern void Enable_Debug_Message(UINT32 DBGStatus);
#endif
#ifdef SUPPORT_SHOP_DEMO_MODE_IN_MAINMENU
 extern UINT32 ShopDemoCountTimer ;
#endif
typedef struct {
	tuner_TypeId  eTunerId;
	char	eTunerName[20];
} APP_TunerName_t;

static APP_TunerName_t TunerTable[]=
{
	{TUNER_IdMaxlinearMXL601,	"Maxlinear_MXL601"},
	{TUNER_IdMaxlinearMXL661,	"Maxlinear_MXL661"},
	{TUNER_IdRafaelMicroR840,	"Rafael_R840"},
	{TUNER_IdNxpTDA18275,	"NXP_TDA18275"},
	{TUNER_IdNxpTDA182I5A,	"NXP_TDA18275A"},
	{TUNER_IdRafaelMicroR842,	"Rafael_R842"},
	{TUNER_IdRda5160,	"RDA5160"},
};

#ifdef CONFIG_KEYPAD_SINGLE_REUSE
void APP_Factory_GetSingleKeyPad_IndexEvent(UINT8 currentidx, UINT32 *dMessage)
{
    switch(currentidx)
    {
       case KEYINDEX_UP:
           *dMessage = UI_EVENT_KEYPAD_UP;
           break;
       case KEYINDEX_DOWN:
           *dMessage = UI_EVENT_KEYPAD_DOWN;
           break;
       case KEYINDEX_RIGHT:
           *dMessage = UI_EVENT_KEYPAD_RIGHT;
           break;
       case KEYINDEX_LEFT:
           *dMessage = UI_EVENT_KEYPAD_LEFT;
           break;
       case KEYINDEX_POWER:
           *dMessage = UI_EVENT_KEYPAD_POWER;
           break;
       case KEYINDEX_MENU:
           *dMessage = UI_EVENT_KEYPAD_MENU;
           break;
       case KEYINDEX_SOURCE:
           *dMessage = UI_EVENT_KEYPAD_SOURCE;
           break;
    } 
}
#endif


void APP_Factory_GetCurrentTunerName(char *sTunerName)
{
	tuner_TypeId eTunerID = 0;
	FrontendCmd_t  front;
	UINT8 i = 0; 
		
	if(NULL ==sTunerName)
		return ;
	
	front.cmd = FRONTEND_CMD_GET_TUNER_ID;
	front.param = (void *)&eTunerID;
	ioctl(kmfdev, KMF_IOC_FRONTENDCTRL , &front);

	for(i=0; i<sizeof(TunerTable)/sizeof(APP_TunerName_t); i++)
	{
		if(TunerTable[i].eTunerId == eTunerID)
		{
			memcpy(sTunerName,TunerTable[i].eTunerName,sizeof(TunerTable[i].eTunerName));
			return ;
		}
	}
	
	memcpy(sTunerName,"Not Define Tuner",sizeof("Not Define Tuner"));
	
	return ;
}


int APP_Factory_CheckFactoryModeExitCode(UINT32 dMessage)
{
	static UINT8 u8KeyCount = 0;
	UINT8 ret;
    UINT32 u32CurrentSysappIndex = 0;
	MAINAPP_GetActiveSystemAppIndex(&u32CurrentSysappIndex);

	if(dMessage >= UI_EVENT_NULL
		|| g_arFactoryExitEventTable == NULL
		|| g_u32FactoryExitEventTableSize < 1)
	{
		ret = FALSE;
	}
#ifdef CONFIG_KEYPAD_SINGLE_REUSE
	if(((dMessage >= UI_EVENT_KEYPAD_MIN) && (dMessage <= UI_EVENT_KEYPAD_MAX)) || (dMessage == UI_EVENT_KEYPAD_SHORT_PRESS))
	{
		ret = TRUE;
	}
#else
	if(dMessage == UI_EVENT_KEYPAD_POWER)
	{
		ret = TRUE;
	}
#endif

#ifdef SUPPORT_HKC_FACTORY_REMOTE
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
		sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData));
	if(g_stFactoryUserData.n_FactSet_FactoryRemote == 1)
	{
		if(dMessage == UI_EVENT_HKC_EXIT_BURNINGMODE)
		{
			ret = TRUE;
			u8KeyCount = 0;
		}
	}
#endif
	if(u8KeyCount == g_u32FactoryExitEventTableSize - 1)
	{
		if(dMessage == g_arFactoryExitEventTable[u8KeyCount])
		{
			u8KeyCount = 0;
			ret = TRUE;
		}
	}
	else
	{
		if(dMessage == g_arFactoryExitEventTable[u8KeyCount])
		{
			u8KeyCount ++;
		}
		else
		{
			u8KeyCount = 0;
		}
	}
	if(ret == TRUE)
	{
		if(g_stFactoryUserData.n_FactSet_BurningMode == 1)  //20140910
		{
			if(AL_FLASH_GetACAutoPowerOn() > 0)
				MID_TVFE_SetAutoPowerOn(TRUE);
			else if(AL_FLASH_GetACAutoPowerOn() == 0)
 				MID_TVFE_SetAutoPowerOn(FALSE);
 		}
#ifdef SUPPORT_LED_FLASH
		APP_LED_Flash_Timer_Set(LED_FLASH_TYPE_RECODER, 0);
#endif
		g_stFactoryUserData.n_FactSet_BurningMode = 0;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, n_FactSet_BurningMode),
			sizeof(g_stFactoryUserData.n_FactSet_BurningMode),&(g_stFactoryUserData.n_FactSet_BurningMode));
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, n_FactSet_BurningMode),
			sizeof(g_stFactoryUserData.n_FactSet_BurningMode));
		MID_TVFE_SetBurnIn(0, 0, 0);	
		extern INT32 OSD_SetDisplay(BOOL ShowOSD);
		OSD_SetDisplay(TRUE);
		MID_DISP_DTVSetVideoUnmute();
#ifdef CONFIG_DTV_SUPPORT
		if(u32CurrentSysappIndex == SYS_APP_DVB)
		{
			SYSAPP_IF_SendGlobalEventWithIndex(
				SYS_APP_DVB, APP_DVB_GLOBAL_EVENT_DVB_ONRUN | PASS_TO_SYSAPP, FALSE);
		}
#endif
#ifdef CONFIG_ATV_SUPPORT
		if(u32CurrentSysappIndex == SYS_APP_ATV)
		{
			SYSAPP_IF_SendGlobalEventWithIndex(
				SYS_APP_ATV, APP_ATV_GLOBAL_EVENT_ATV_ONRUN | PASS_TO_SYSAPP, FALSE);
		}
#endif
#ifdef CONFIG_MEDIA_SUPPORT
		if(u32CurrentSysappIndex == SYS_APP_FILE_PLAYER)
		{
            //DRV_TvoutSetMultiWindowBackGroundColor(0x00, 0x00, 0x00);
            SYSAPP_IF_SendGlobalEventWithIndex(
				SYS_APP_FILE_PLAYER, FILE_GLOBAL_EVENT_MEDIA_EXIT_BURNINMODE | PASS_TO_SYSAPP, FALSE);
        }
#endif
		return SP_SUCCESS;
	}
	else
	{
		return SP_ERR_FAILURE;
	}
}

/*****************************************************************************
** FUNCTION : APP_Factory_CheckComponentEvent
**
** DESCRIPTION :
**				check component key.
**
** PARAMETERS :
**				Message:                    event id
**                      pComponentEvent:      Event Id array
**                      dLength:                     the length Event ID array
** RETURN VALUES:
**				check success or failure
*****************************************************************************/
UINT8 APP_Factory_CheckComponentEvent(UINT32 dMessage, UINT32 *pComponentEvent, UINT8 dLength, UINT8 *pEventCount)
{
	if (dMessage >= UI_EVENT_NULL
		|| pComponentEvent == NULL
		|| dLength < 1)
	{
		return FALSE;
	}

	if(*pEventCount == dLength - 1)
	{
		if(dMessage == pComponentEvent[*pEventCount])
		{
			*pEventCount = 0;
			return TRUE;
		}
		else
		{
			if(dMessage == pComponentEvent[0])
			{
				*pEventCount = 1;
			}
			else
			{
				*pEventCount = 0;
			}
		}
	}
	else
	{
		if(dMessage == pComponentEvent[*pEventCount])
		{
			*pEventCount = *pEventCount +1;
		}
		else
		{
			if(dMessage == pComponentEvent[0])
			{
				*pEventCount = 1;
			}
			else
			{
				*pEventCount = 0;
			}
		}
	}
    
	return FALSE;
}

/*****************************************************************************
** FUNCTION : APP_Factory_HotelModePowerOnChan
**
** DESCRIPTION :
**				Set hotel mode power on channel
**
** PARAMETERS :
**				stSourceIndex -   current source
**
** RETURN VALUES:
**				None
*****************************************************************************/
void APP_Factory_HotelModePowerOnChan(void)
{
    AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTHOTEL, 0,
                    sizeof(APP_SETTING_FactoryHotel_t), &(g_stFactoryHotelData));
    al_uint8 u8HotelMode = g_stFactoryHotelData.HotelModeOnOff;
#ifdef CONFIG_DTV_SUPPORT
    AL_DB_EDBType_t DB_Type = AL_DBTYPE_DVB_T;
	AL_DB_ERecordType_t eServiceType = AL_RECTYPE_DVBTV;
#endif
#if defined CONFIG_DTV_SUPPORT || defined CONFIG_ATV_SUPPORT
	al_uint32 Index = 0;
    AL_RecHandle_t hServHdl = AL_DB_INVALIDHDL;
#endif
    APP_Source_Type_t eSourceType = APP_SOURCE_MAX;
#ifdef CONFIG_ISDB_SYSTEM 
	app_scan_table_info_st scan_info;
	app_scan_table_info_st air_scan_info;
#endif
#ifdef CONFIG_SUPPORT_ATV_SCAN_NTSCM_PALM_N
	app_scan_table_info_st scan_info;
	app_scan_table_info_st air_scan_info;
	static UINT32 g_TotalATVChnNo = 0;
#endif
	APP_GUIOBJ_Source_SourceMapping_Table(g_stSourceConfigTable_New,&g_u16SourceConfigTable_Size_New);

    if (u8HotelMode == 1)
    {
        if (g_stFactoryHotelData.PowerOnSource== APP_SOURCE_MAX)
        {
            return;
        }
        else
        {
            eSourceType = g_stFactoryHotelData.PowerOnSource;
            switch (eSourceType)
            {
#ifdef CONFIG_ATV_SUPPORT
                case APP_SOURCE_ATV:
					Index = g_stFactoryHotelData.PowerOnATVChannel;
#ifndef CONFIG_ISDB_SYSTEM
#ifdef CONFIG_SUPPORT_ATV_SCAN_NTSCM_PALM_N
					APP_Scan_API_GetTableInfo(APP_GUIOBJ_Source_GetCurATVType(), &scan_info);
					APP_Scan_API_GetTableInfo(ATV_TYPE_AIR, &air_scan_info);
					g_TotalATVChnNo = scan_info.u32MaxChNo - scan_info.u32MinChNo + 1;
					if (Index>g_TotalATVChnNo)
					{
						Index = 0;

					}

					if (APP_GUIOBJ_Source_GetCurATVType() == ATV_TYPE_AIR)
					{
						APP_Database_GetHandleByIndex(AL_DBTYPE_DVB_ATV, AL_RECTYPE_ATVSERVICE, Index, &hServHdl);
							if (hServHdl != AL_DB_INVALIDHDL)
						{
						    g_stVariationalData.ATV_TVHdl = hServHdl;
						    AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_VARIATIONAL,
						                     ITEM_OFFSET(APP_SETTING_Variational_t, ATV_TVHdl),
						                     sizeof(g_stVariationalData.ATV_TVHdl), &(g_stVariationalData.ATV_TVHdl));
						    AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_VARIATIONAL,
						                     ITEM_OFFSET(APP_SETTING_Variational_t, ATV_TVHdl),
						                     sizeof(g_stVariationalData.ATV_TVHdl));
							}
					}
					else
					{
						APP_Database_GetHandleByIndex(AL_DBTYPE_DVB_ATV, AL_RECTYPE_ATVSERVICE, Index +air_scan_info.u32ScanTableLen, &hServHdl);
						if (hServHdl != AL_DB_INVALIDHDL)
						{
						    g_stVariationalData.CATV_TVHdl = hServHdl;
						    AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_VARIATIONAL,
						                     ITEM_OFFSET(APP_SETTING_Variational_t, CATV_TVHdl),
						                     sizeof(g_stVariationalData.CATV_TVHdl), &(g_stVariationalData.CATV_TVHdl));
						    AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_VARIATIONAL,
						                     ITEM_OFFSET(APP_SETTING_Variational_t, CATV_TVHdl),
						                     sizeof(g_stVariationalData.CATV_TVHdl));
							}
					}	
#else
					if (Index >= 100)
					{
					Index = 0;
					}
					APP_Database_GetHandleByIndex(AL_DBTYPE_DVB_ATV, AL_RECTYPE_ATVSERVICE, Index, &hServHdl);
					if (hServHdl != AL_DB_INVALIDHDL)
					{
					g_stVariationalData.ATV_TVHdl = hServHdl;
					AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_VARIATIONAL,
					                 ITEM_OFFSET(APP_SETTING_Variational_t, ATV_TVHdl),
					                 sizeof(g_stVariationalData.ATV_TVHdl), &(g_stVariationalData.ATV_TVHdl));
					AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_VARIATIONAL,
					                 ITEM_OFFSET(APP_SETTING_Variational_t, ATV_TVHdl),
					                 sizeof(g_stVariationalData.ATV_TVHdl));
					}
#endif
#else
					APP_Scan_API_GetTableInfo(APP_GUIOBJ_Source_GetRFPort(), &scan_info);
					APP_Scan_API_GetTableInfo(RF_INPUT_PORT_AIR, &air_scan_info);
					if ((Index > scan_info.u32MaxChNo) || (Index < scan_info.u32MinChNo))
					{
					Index = scan_info.u32MinChNo;
					}
					if (APP_GUIOBJ_Source_GetRFPort() == RF_INPUT_PORT_AIR)
					{
					APP_Database_GetHandleByIndex(AL_DBTYPE_DVB_ATV, AL_RECTYPE_ATVSERVICE, Index - scan_info.u32MinChNo, &hServHdl);
						if (hServHdl != AL_DB_INVALIDHDL)
					{
					    g_stVariationalData.ATV_TVHdl = hServHdl;
					    AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_VARIATIONAL,
					                     ITEM_OFFSET(APP_SETTING_Variational_t, ATV_TVHdl),
					                     sizeof(g_stVariationalData.ATV_TVHdl), &(g_stVariationalData.ATV_TVHdl));
					    AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_VARIATIONAL,
					                     ITEM_OFFSET(APP_SETTING_Variational_t, ATV_TVHdl),
					                     sizeof(g_stVariationalData.ATV_TVHdl));
						}
					}
					else
					{
					APP_Database_GetHandleByIndex(AL_DBTYPE_DVB_ATV, AL_RECTYPE_ATVSERVICE, Index - scan_info.u32MinChNo+air_scan_info.u32ScanTableLen, &hServHdl);
						if (hServHdl != AL_DB_INVALIDHDL)
					{
					    g_stVariationalData.CATV_TVHdl = hServHdl;
					    AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_VARIATIONAL,
					                     ITEM_OFFSET(APP_SETTING_Variational_t, CATV_TVHdl),
					                     sizeof(g_stVariationalData.CATV_TVHdl), &(g_stVariationalData.CATV_TVHdl));
					    AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_VARIATIONAL,
					                     ITEM_OFFSET(APP_SETTING_Variational_t, CATV_TVHdl),
					                     sizeof(g_stVariationalData.CATV_TVHdl));
						}
					}
#endif
                    break;
#endif	
#ifdef CONFIG_DTV_SUPPORT
                case APP_SOURCE_DTV:
                case APP_SOURCE_RADIO:
					#ifndef CONFIG_ISDB_SYSTEM
					DB_Type = APP_DVB_Playback_GetCurrentNetType();
					#else
					DB_Type = AL_DBTYPE_DVB_SBTVD;
					#endif
					eServiceType = APP_DVB_Playback_GetCurrServiceType(DB_Type);
					if(eServiceType == AL_RECTYPE_DVBTV)
	                    Index = g_stFactoryHotelData.PowerOnDTVChannel;
					else if(eServiceType == AL_RECTYPE_DVBRADIO)
	                    Index = g_stFactoryHotelData.PowerOnRadioChannel;
                    APP_Database_GetHandleByIndex(DB_Type, eServiceType, Index, &hServHdl);
                   	if (hServHdl != AL_DB_INVALIDHDL)
					{
						APP_DVB_Playback_SetCurrentProgHandle(DB_Type, eServiceType, hServHdl);
                   	}
	                break;
#endif
                default:
                    break;
            }
        }
    }
}

/*****************************************************************************
** FUNCTION : APP_Factory_GetHotelModePowerSource
**
** DESCRIPTION :
**				Get hotel mode power on source
**
** PARAMETERS :
**				stSourceIndex -  power off storage source
**
** RETURN VALUES:
**				PrevSource - hotel mode power on source
*****************************************************************************/
APP_Source_Type_t APP_Factory_GetHotelModePowerSource(APP_Source_Type_t stSourceIndex)
{
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTHOTEL, 0,
                    sizeof(APP_SETTING_FactoryHotel_t), &(g_stFactoryHotelData));
    al_uint8 u8HotelLock = g_stFactoryHotelData.HotelModeOnOff;
    APP_Source_Type_t PrevSource = stSourceIndex;

	APP_GUIOBJ_Source_SourceMapping_Table(g_stSourceConfigTable_New,&g_u16SourceConfigTable_Size_New);

    if (u8HotelLock == 1)
    {
        if (g_stFactoryHotelData.PowerOnSource== APP_SOURCE_MAX)
        {
            //return PrevSource;
        }
        else
        {
            PrevSource = g_stFactoryHotelData.PowerOnSource;//g_stSourceConfigTable_New[g_stFactoryHotelData.PowerOnSource-1].eSourceType;
            #ifdef CONFIG_DTV_SUPPORT
			if(PrevSource == APP_SOURCE_RADIO)
			{
				PrevSource = APP_SOURCE_DTV;
				AL_DB_EDBType_t eNetType = APP_DVB_Playback_GetCurrentNetType();
				AL_DB_ERecordType_t eServiceType = AL_RECTYPE_DVBRADIO;
	            APP_DVB_Playback_SetCurrServiceType(eNetType, eServiceType);
			}
			else if(PrevSource == APP_SOURCE_DTV)
			{
				AL_DB_EDBType_t eNetType = APP_DVB_Playback_GetCurrentNetType();
				AL_DB_ERecordType_t eServiceType = AL_RECTYPE_DVBTV;
				APP_DVB_Playback_SetCurrServiceType(eNetType, eServiceType);
			}
		#endif
		#ifdef CONFIG_DVB_SYSTEM
			APP_SCARTIN_SetAutoPlugInStatus(AUTO_PLUGIN_NONE);
		#endif
		}

		#if 0
		if(g_stFactoryHotelData.DefaultVolume > g_stFactoryHotelData.MaxVolume)
			g_stFactoryHotelData.DefaultVolume = g_stFactoryHotelData.MaxVolume;

		//AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO, 0,
						//sizeof(APP_SETTING_UserInfo_t), &(g_stUserInfoData));
		AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_VARIATIONAL, 0,
						sizeof(APP_SETTING_Variational_t), &(g_stVariationalData));

		g_stVariationalData.Volume = g_stFactoryHotelData.DefaultVolume;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_VARIATIONAL,
						 ITEM_OFFSET(APP_SETTING_Variational_t, Volume),
						 sizeof(g_stVariationalData.Volume), &(g_stVariationalData.Volume));
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTHOTEL,
						 ITEM_OFFSET(APP_SETTING_FactoryHotel_t, DefaultVolume),
						 sizeof(g_stFactoryHotelData.DefaultVolume), &(g_stFactoryHotelData.DefaultVolume));

        switch (g_stFactoryHotelData.DefaultVolumeOnOff)
        {
            case 0:
                //off,do nothing
                break;
            case 1:
                AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO, 0,
                                sizeof(APP_SETTING_UserInfo_t), &(g_stUserInfoData));

                g_stVariationalData.Volume = g_stFactoryHotelData.DefaultVolume;
                AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO,
                                 ITEM_OFFSET(APP_SETTING_UserInfo_t, Volume),
                                 sizeof(g_stVariationalData.Volume), &(g_stVariationalData.Volume));
                break;
            default:
                break;
        }
		#endif
    }

    return PrevSource;
}

/*****************************************************************************
** FUNCTION : APP_Factory_SetBacklightOnOff
**
** DESCRIPTION :
**				Set backliht
**
** PARAMETERS :
**				SetOn -
**
** RETURN VALUES:
**				None
*****************************************************************************/
void APP_Factory_SetBacklightOnOff(Boolean SetOn)
{
    APP_Source_Type_t eCurrSource = APP_SOURCE_MAX;
    APP_StorageSource_Type_t eStorageSourType = APP_STORAGE_SOURCE_MAX;

    APP_GUIOBJ_Source_GetCurrSource(&eCurrSource);
    eStorageSourType = APP_Data_UserSetting_SourceTypeMappingToStorage(eCurrSource);

    if ((g_stPictureData.stPictureModeSourceTab[eStorageSourType].PictureModeIndex == APP_VIDEO_PQ_MODE_USER)
        && (g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex == APP_AUDIO_MODE_FAVORITE)
        && (g_stPictureData.PictureMode[eCurrSource].stUserTab.Brightness == 0)
        && (g_stPictureData.PictureMode[eCurrSource].stUserTab.Contrast == 0)
        && (g_stPictureData.PictureMode[eCurrSource].stUserTab.Sharpness == 0)
        && (g_stPictureData.PictureMode[eCurrSource].stUserTab.Saturation == 0)
        && (g_stSoundData.stSoundModeSourceTab[eStorageSourType].stUserTab.Bass == 0)
        && (g_stSoundData.stSoundModeSourceTab[eStorageSourType].stUserTab.Treble == 0)
       )
    {
        if (SetOn)	//backlight from off to on
        {
           // APP_Panel_SetBacklightEnOnOff(TRUE);
        }
        else		//backlight from on to off
        {
           // APP_Panel_SetBacklightEnOnOff(FALSE);
        }
    }

}

#ifdef SUPPORT_FACTORY_AUTO_TEST

typedef enum
{
	    // Data1
    SYSCONF_BOARD_POWER_BIT1         	= 1 << 0,
    SYSCONF_BOARD_POWER_BIT2         	= 1 << 1,
    SYSCONF_BOARD_FACPANEL_BIT1        	= 1 << 2,
    SYSCONF_BOARD_FACPANEL_BIT2        	= 1 << 3,
    SYSCONF_BOARD_FACKEY_BIT1         	= 1 << 4,
    SYSCONF_BOARD_FACKEY_BIT2         	= 1 << 5,
    SYSCONF_BOARD_FACKEY_BIT3    		= 1 << 6,
    
    	// Data2
    SYSCONF_TV_SRC_ATV         			= 1 << 0,
    SYSCONF_TV_SRC_DTV         			= 1 << 1,
    SYSCONF_TV_SRC_DVBT2        		= 1 << 2,
    SYSCONF_TV_SRC_DVBS        			= 1 << 3,
    SYSCONF_TV_FUN_CI         			= 1 << 4,
    SYSCONF_TV_FUN_CIPLUS    			= 1 << 5,
    SYSCONF_TV_FUN_RESET    			= 1 << 6,

        // Data3
    SYSCONF_AV_SRC_AV1       			= 1 << 0,
    SYSCONF_AV_SRC_AV2       			= 1 << 1,
    SYSCONF_AV_SRC_AV3       		    = 1 << 2,
    SYSCONF_AV_SRC_YPBPR1       		= 1 << 3,
    SYSCONF_AV_SRC_YPBPR2       		= 1 << 4,
    SYSCONF_AV_SRC_SCART				= 1 << 5,
    SYSCONF_AV_SRC_PC       			= 1 << 6,
    SYSCONF_AV_SRC_DVD        			= 1 << 7,
    
        // Data4
    SYSCONF_HDMI_SRC_HDMI1        		= 1 << 0,
    SYSCONF_HDMI_SRC_HDMI2        		= 1 << 1,
    SYSCONF_HDMI_SRC_HDMI3         		= 1 << 2,
    SYSCONF_HDMI_SRC_HDMI4     			= 1 << 3,
    SYSCONF_HDMI_FUN_HDCP      			= 1 << 4,
    SYSCONF_HDMI_FUN_CEC          		= 1 << 5,
    SYSCONF_HDMI_FUN_ARC				= 1 << 6,
    SYSCONF_HDMI_FUN_EDID   			= 1 << 7,
    
        // Data5
    SYSCONF_USB_SRC_USB1     			= 1 << 0,
    SYSCONF_USB_SRC_USB2       			= 1 << 1,
    SYSCONF_USB_SRC_USB3    			= 1 << 2,
    SYSCONF_USB_SRC_USB4  				= 1 << 3,
    SYSCONF_BLUETOOTH_BIT1      		= 1 << 4,
    SYSCONF_BLUETOOTH_BIT2         		= 1 << 5,
    SYSCONF_BLUETOOTH_WIFI1         	= 1 << 6,
    SYSCONF_BLUETOOTH_WIFI2         	= 1 << 7,
    
        // Data6
    SYSCONF_FUN_AV_OUT         			= 1 << 0,
    SYSCONF_FUN_CVBS_OUT        		= 1 << 1,
    SYSCONF_FUN_LINE_OUT    			= 1 << 2,
    SYSCONF_FUN_COAX     				= 1 << 3,
    SYSCONF_FUN_SPDIF        			= 1 << 4,
    SYSCONF_FUN_WOFFER        			= 1 << 5,
    SYSCONF_FUN_EARPHONE        		= 1 << 6,
    
            // Data7
    SYSCONF_FUN_2_4G      				= 1 << 0,
    SYSCONF_FUN_LAN        	  			= 1 << 1,
    SYSCONF_FUN_AIRPLAY        			= 1 << 2,//´óÆÁ´«Ð¡ÆÁ
    SYSCONF_FUN_KARAOKE        			= 1 << 3,
    SYSCONF_FUN_ANDROID        			= 1 << 4,//1+1
    SYSCONF_HDMI_FUN_MHL_BIT1        	= 1 << 5,
    SYSCONF_HDMI_FUN_MHL_BIT2        	= 1 << 6,
    SYSCONF_HDMI_FUN_MHL_BIT3       	= 1 << 7,

            // Data8
    SYSCONF_FUN_VCC_PANEL1      		= 1 << 0,
    SYSCONF_FUN_VCC_PANEL2        	  	= 1 << 1,
    SYSCONF_FUN_VCC_CORE1    			= 1 << 2,
    SYSCONF_FUN_VCC_CORE2     			= 1 << 3,
    SYSCONF_FUN_VCC_LED_R        		= 1 << 4,
    SYSCONF_FUN_VCC_LED_G        		= 1 << 5,
    SYSCONF_FUN_VCC_12V        		    = 1 << 6,
    SYSCONF_FUN_VCC_5V_STB       		= 1 << 7,
    
            // Data9
    SYSCONF_FUN_BL_ONOFF      			= 1 << 0,
    SYSCONF_FUN_BL_ADJ        	  		= 1 << 1,
    SYSCONF_FUN_DVD_AUTO    			= 1 << 4,
    SYSCONF_FUN_DVD_STB     			= 1 << 4,
    SYSCONF_FUN_DVD_12V        			= 1 << 6,
    
            // Data10
    SYSCONF_FUN_DVD_5V      			= 1 << 0,
    SYSCONF_FUN_TUNER_3_3V        	  	= 1 << 1,
    SYSCONF_FUN_LNB_13V        	  		= 1 << 2,
    SYSCONF_FUN_TUNER_5V_ANTENNA_5V     = 1 << 3,
    SYSCONF_FUN_LNB_18V    			  	= 1 << 4,
    SYSCONF_FUN_USB_5V        			= 1 << 6,
    SYSCONF_HDMI_FUN_MHL_5V       		= 1 << 7,//1+1    
} SYSCONF_LIST_e;

#define SYSCONF_CMD_LEN			15
void APP_Factory_Send_SysConf(void)
{
	UINT8 bDATA[SYSCONF_CMD_LEN];
	UINT8 bTmpDATA = 0;
	UINT8 i = 0;
	UINT16 temp = 0;
	//UINT8 len = 0;
	//UINT8* boardname;
	
	//len = strlen(MODEL_BOARD_NAME)+1;
	//boardname = (UINT8*)malloc(len);
	//memset(boardname,0x0,len);
	//memcpy(boardname,MODEL_BOARD_NAME,len-1);
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_CHANNEL, 0,
		sizeof(APP_SETTING_Channel_t), &(g_stChannelData));
	extern APP_SETTING_Channel_t g_stSettingDefault_Channel;
	//StartByte
	bDATA[0] = 0xF0|(SYSCONF_CMD_LEN-2);
	
	/****************************************DATA1***************************************/
	bTmpDATA = 0;
	
#if (SYSCONF_BOARD_POWER == 1)
	bTmpDATA |= SYSCONF_BOARD_POWER_BIT1;
#elif (SYSCONF_BOARD_POWER == 2)
	bTmpDATA |= SYSCONF_BOARD_POWER_BIT2;
#endif
    //bTmpDATA |= SYSCONF_BOARD_FACPANEL_BIT1;
    //bTmpDATA |= SYSCONF_BOARD_FACPANEL_BIT2;
    //bTmpDATA |= SYSCONF_BOARD_FACKEY_BIT1;
    //bTmpDATA |= SYSCONF_BOARD_FACKEY_BIT2;
    //bTmpDATA |= SYSCONF_BOARD_FACKEY_BIT3;
	
	bDATA[1] = bTmpDATA;

	/****************************************DATA2***************************************/
	bTmpDATA = 0;	

#if ((defined CONFIG_ATV_SUPPORT)&&((defined DEFAULT_SUPPORT_SOURCE_TYPE_ATV)&&(DEFAULT_SUPPORT_SOURCE_TYPE_ATV == 1)))
    bTmpDATA |= SYSCONF_TV_SRC_ATV; 
#endif
#if	((defined CONFIG_DVB_SYSTEM_DVBT_SUPPORT)&&((defined DEFAULT_SUPPORT_SOURCE_TYPE_DTV)&&(DEFAULT_SUPPORT_SOURCE_TYPE_DTV == 1)))
    bTmpDATA |= SYSCONF_TV_SRC_DTV; 
#endif
#if	((defined CONFIG_DVB_SYSTEM_DVBS_SUPPORT)&&((defined DEFAULT_SUPPORT_SOURCE_TYPE_DTV)&&(DEFAULT_SUPPORT_SOURCE_TYPE_DTV == 1)))
    bTmpDATA |= SYSCONF_TV_SRC_DVBS;
#endif
#if	((defined CONFIG_DVB_SYSTEM_DVBT2_SUPPORT)&&((defined DEFAULT_SUPPORT_SOURCE_TYPE_DTV)&&(DEFAULT_SUPPORT_SOURCE_TYPE_DTV == 1)))
    bTmpDATA |= SYSCONF_TV_SRC_DVBT2;
#endif
#ifdef CONFIG_CI_SUPPORT
    bTmpDATA |= SYSCONF_TV_FUN_CI;    
#endif
#ifdef CONFIG_CIPLUS_SUPPORT
    bTmpDATA |= SYSCONF_TV_FUN_CIPLUS; 
#endif
    bTmpDATA |= SYSCONF_TV_FUN_RESET; //need reset test
	bDATA[2] = bTmpDATA;

	/****************************************DATA3**************************************/
	bTmpDATA = 0;
	
#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_AV)&&(DEFAULT_SUPPORT_SOURCE_TYPE_AV == 1))		
    bTmpDATA |= SYSCONF_AV_SRC_AV1;   
#endif
#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_AV2)&&(DEFAULT_SUPPORT_SOURCE_TYPE_AV2 == 1))
    bTmpDATA |= SYSCONF_AV_SRC_AV2;
#endif
#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_AV3)&&(DEFAULT_SUPPORT_SOURCE_TYPE_AV3 == 1))
    bTmpDATA |= SYSCONF_AV_SRC_AV3;
#endif 
#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_YPBPR)&&(DEFAULT_SUPPORT_SOURCE_TYPE_YPBPR== 1))    
    bTmpDATA |= SYSCONF_AV_SRC_YPBPR1;       			
#endif
#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_YPBPR)&&(DEFAULT_SUPPORT_SOURCE_TYPE_YPBPR2== 1))    
    bTmpDATA |= SYSCONF_AV_SRC_YPBPR2;       			
#endif
#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_SCART)&&(DEFAULT_SUPPORT_SOURCE_TYPE_SCART== 1))
    bTmpDATA |= SYSCONF_AV_SRC_SCART;		
#endif
#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_PC_RGB)&&(DEFAULT_SUPPORT_SOURCE_TYPE_PC_RGB== 1))
    bTmpDATA |= SYSCONF_AV_SRC_PC;   
#endif
#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_DVD)&&(DEFAULT_SUPPORT_SOURCE_TYPE_DVD== 1))
    bTmpDATA |= SYSCONF_AV_SRC_DVD;     
#endif     			

	bDATA[3] = bTmpDATA;

	/****************************************DATA4***************************************/
	bTmpDATA = 0;

#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_HDMI)&&(DEFAULT_SUPPORT_SOURCE_TYPE_HDMI == 1))	
    bTmpDATA |= SYSCONF_HDMI_SRC_HDMI1;        		
#endif
#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_HDMI2)&&(DEFAULT_SUPPORT_SOURCE_TYPE_HDMI2 == 1))
    bTmpDATA |= SYSCONF_HDMI_SRC_HDMI2;        		
#endif
#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_HDMI3)&&(DEFAULT_SUPPORT_SOURCE_TYPE_HDMI3 == 1))
    bTmpDATA |= SYSCONF_HDMI_SRC_HDMI3;         		
#endif
    //bTmpDATA |= SYSCONF_HDMI_SRC_HDMI4;     			
    bTmpDATA |= SYSCONF_HDMI_FUN_HDCP;
#ifdef SUPPORT_CEC_TV
    bTmpDATA |= SYSCONF_HDMI_FUN_CEC;
#endif
#ifdef CONFIG_SUPPORT_ARC
	bTmpDATA |= SYSCONF_HDMI_FUN_ARC;          		
#endif
	bTmpDATA |= SYSCONF_HDMI_FUN_EDID; 

	bDATA[4] = bTmpDATA;

	/****************************************DATA5***************************************/
	bTmpDATA = 0;

#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_USB)&&(DEFAULT_SUPPORT_SOURCE_TYPE_USB == 1))	
    bTmpDATA |= SYSCONF_USB_SRC_USB1;     			
#endif
    //bTmpDATA |= SYSCONF_USB_SRC_USB2;       			
    //bTmpDATA |= SYSCONF_USB_SRC_USB3;    			
    //bTmpDATA |= SYSCONF_USB_SRC_USB4;  				
    //bTmpDATA |= SYSCONF_USB_SRC_USB5;      			
    //bTmpDATA |= SYSCONF_USB_SRC_USB6;        		
    //bTmpDATA |= SYSCONF_USB_SRC_SDCARD; 
	
	bDATA[5] = bTmpDATA;

	/****************************************DATA6***************************************/
	bTmpDATA = 0;
	//if(AREA_AUSTRALIA == g_stChannelData.Country)
	if(AREA_AUSTRALIA == g_stSettingDefault_Channel.Country)
    {
		bTmpDATA |= SYSCONF_FUN_AV_OUT;
	}         
#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_SCART)&&(DEFAULT_SUPPORT_SOURCE_TYPE_SCART == 1))		
    bTmpDATA |= SYSCONF_FUN_CVBS_OUT;        		
#endif
//#ifdef SYSCONF_LINE_OUT
  //  bTmpDATA |= SYSCONF_FUN_LINE_OUT; 
//#endif
    //bTmpDATA |= SYSCONF_FUN_COAX;     				
    bTmpDATA |= SYSCONF_FUN_SPDIF;        			
    //bTmpDATA |= SYSCONF_FUN_WOFFER;        			
    bTmpDATA |= SYSCONF_FUN_EARPHONE;        			

	bDATA[6] = bTmpDATA;

	/****************************************DATA7***************************************/
	bTmpDATA = 0;
	
    //bTmpDATA |= SYSCONF_FUN_2_4G;      				
    //bTmpDATA |= SYSCONF_FUN_LAN;        	  			
    //bTmpDATA |= SYSCONF_FUN_AIRPLAY;        			
    //bTmpDATA |= SYSCONF_FUN_KARAOKE;        			
    //bTmpDATA |= SYSCONF_FUN_ANDROID;       		  	
#ifdef CONFIG_HDMI_SUPPORT_MHL
#if(CONFIG_HDMI_MHL_PORT == 0x0) //000£ºHDMI1 Support _MHL
    bTmpDATA |= SYSCONF_HDMI_FUN_MHL_BIT1; 
#endif
#if(CONFIG_HDMI_MHL_PORT == 0x1)////001£ºHDMI2 Support _MHL
    bTmpDATA |= SYSCONF_HDMI_FUN_MHL_BIT2; 
#endif
#endif
	bDATA[7] = bTmpDATA;

	/****************************************DATA8***************************************/
	bTmpDATA = 0;
#if (SYSCONF_PANEL_VCC == 1)
    bTmpDATA |= SYSCONF_FUN_VCC_PANEL1; 
#elif(SYSCONF_PANEL_VCC == 2)
    bTmpDATA |= SYSCONF_FUN_VCC_PANEL2;
#endif
    //bTmpDATA |= SYSCONF_FUN_VCC_CORE1;    			
    //bTmpDATA |= SYSCONF_FUN_VCC_CORE2;     			
    //bTmpDATA |= SYSCONF_FUN_VCC_DDR;        			
    bTmpDATA |= SYSCONF_FUN_VCC_LED_R;        		
    bTmpDATA |= SYSCONF_FUN_VCC_LED_G;       		  	
    bTmpDATA |= SYSCONF_FUN_VCC_12V;    //    		
    bTmpDATA |= SYSCONF_FUN_VCC_5V_STB;   //    		  	
	bDATA[8] = bTmpDATA;

	/****************************************DATA9***************************************/
	bTmpDATA = 0;
	
    bTmpDATA |= SYSCONF_FUN_BL_ONOFF;      			
    bTmpDATA |= SYSCONF_FUN_BL_ADJ;	
    //bTmpDATA |= SYSCONF_FUN_3_3_SW;        			
    //bTmpDATA |= SYSCONF_FUN_3_3_STB;   
#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_DVD)&&(DEFAULT_SUPPORT_SOURCE_TYPE_DVD== 1)&&(defined SUPPORT_AUTO_DVD))    
   // bTmpDATA |= SYSCONF_FUN_DVD_AUTO; 
#endif
	
	bDATA[9] = bTmpDATA;

	/****************************************DATA10***************************************/
	bTmpDATA = 0;

#if ((defined DEFAULT_SUPPORT_SOURCE_TYPE_DVD)&&(DEFAULT_SUPPORT_SOURCE_TYPE_DVD== 1)&&(defined SUPPORT_AUTO_DVD)) 	
   // bTmpDATA |= SYSCONF_FUN_DVD_STB;      			
#endif
   // bTmpDATA |= SYSCONF_FUN_USB_5V;       	  		
    //bTmpDATA |= SYSCONF_FUN_LED_PWM; 

#if(defined CONFIG_DVB_SYSTEM_DVBT_SUPPORT)
    //bTmpDATA |= SYSCONF_FUN_LNB_13V; 
   // bTmpDATA |= SYSCONF_FUN_LNB_18V; 
#elif defined( SYSCONF_FUN_TUNER_5V_ANTENNA_5V)
   // bTmpDATA |= SYSCONF_FUN_TUNER_5V_ANTENNA_5V;
#endif

#ifdef CONFIG_HDMI_SUPPORT_MHL
    bTmpDATA |= SYSCONF_HDMI_FUN_MHL_5V;       	  		
#endif

	bDATA[10] = bTmpDATA;
	/****************************************DATA11***************************************/
	bDATA[11] = 0;
	bDATA[12] = 0;
	bDATA[13] = 0;
#ifdef PWM_CURRENT
	temp = PWM_CURRENT;
	bDATA[11] = (UINT8)(temp>>8);
	bDATA[12] = (UINT8)(temp&0x00FF);
#endif

#ifdef PWM_VOLTAGE
	bDATA[13] = (UINT8)PWM_VOLTAGE;
#endif
    
	//CheckSum
	bTmpDATA = 0;
	for(i=0; i<(SYSCONF_CMD_LEN-1); i++)
	{
		bTmpDATA += bDATA[i];
	}	
	bDATA[SYSCONF_CMD_LEN-1] = ~bTmpDATA;
	Enable_Debug_Message(1<<MODULEID_UMF);
	printf("\n***0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x*** \n",
			bDATA[0],bDATA[1],bDATA[2],bDATA[3],bDATA[4],bDATA[5],bDATA[6],bDATA[7],
			bDATA[8],bDATA[9],bDATA[10],bDATA[11],bDATA[12],bDATA[13],bDATA[14]);
	Enable_Debug_Message(0);
	
}
/*****************************************************************************
** FUNCTION : APP_Factory_SetAutoTestSourceChange
**
** DESCRIPTION :
**				Set backliht
**
** PARAMETERS :
**				SetOn -
**
** RETURN VALUES:
**				None
*****************************************************************************/
void APP_Factory_SetAutoTestSourceChange(UINT32 dMessage)
{
//#if defined(CONFIG_DVB_SYSTEM_DVBC_SUPPORT)||defined(CONFIG_DVB_SYSTEM_DVBS_SUPPORT)
	UINT32 RFtype = APP_RF_TYPE_DVB_MAX;
#ifndef CONFIG_SUPPORT_ATV_SCAN_NTSCM_PALM_N //for_Colombia ATV&DTV Using the same RFType to swich Air&Cable
	UINT32 StringId = 0;
#endif
    APP_Source_Type_t eSourType = APP_SOURCE_MAX;
	switch(dMessage)
	{
		case UI_EVENT_AUTO_ATV:
#ifdef CONFIG_SUPPORT_ATV_SCAN_NTSCM_PALM_N //for_Colombia ATV&DTV Using the same RFType to swich Air&Cable
			APP_GUIOBJ_Source_GetCurrSource(&eSourType);
			AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_TVINFO, 0,
				sizeof(APP_SETTING_TVInfo_t), &(g_stTVInfoData));

			if(eSourType == APP_SOURCE_ATV)
			{
				APP_GUIOBJ_Channel_SetATVTypeByStringID(TV_IDS_String_Air, eSourType);
			}
			else
			{
				if(g_stTVInfoData.ATV_SourceType != ATV_TYPE_AIR)
				APP_GUIOBJ_Channel_SetATVTypeByStringID(TV_IDS_String_Air, eSourType);

				APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_ATV);
			}
#else
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_ATV);
#endif
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:ATV***\n");
			break;
            
		case UI_EVENT_AUTO_DTV:
		case UI_EVENT_RED:
			APP_GUIOBJ_Source_GetCurrSource(&eSourType);
			AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_CHANNEL, 0,
				sizeof(APP_SETTING_Channel_t), &(g_stChannelData));
			RFtype = g_stChannelData.TV_Connection;
#ifdef CONFIG_SUPPORT_ATV_SCAN_NTSCM_PALM_N //for_Colombia ATV&DTV Using the same RFType to swich Air&Cable
			if(eSourType == APP_SOURCE_DTV)
			{
				APP_GUIOBJ_Channel_SetATVTypeByStringID(TV_IDS_String_Air, eSourType);
				APP_GUIOBJ_Channel_SetRFTypeByStringID(TV_IDS_String_Air);
			}
			else 
			{
				if(RFtype != APP_RF_TYPE_DVB_T)
				{
					APP_GUIOBJ_Channel_SetNeedChangeRFType(APP_RF_TYPE_DVB_T);
				}
				APP_GUIOBJ_Channel_SetATVTypeByStringID(TV_IDS_String_Air, eSourType);
				APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_DTV);
			}
			
#else

			AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_CHANNEL, 0,
				sizeof(APP_SETTING_Channel_t), &(g_stChannelData));
			RFtype = g_stChannelData.TV_Connection;
			APP_GUIOBJ_Source_GetCurrSource(&eSourType);
			if((eSourType == APP_SOURCE_DTV)||((eSourType == APP_SOURCE_RADIO)))
			{
				if(RFtype != APP_RF_TYPE_DVB_T)
				{
					 StringId = APP_GUIOBJ_Channel_GetStringIDByRFType(APP_RF_TYPE_DVB_T);
					 APP_GUIOBJ_Channel_SetRFTypeByStringID(StringId);
				}			 
			}
			else 
			{
				if(RFtype != APP_RF_TYPE_DVB_T)
				{
					APP_GUIOBJ_Channel_SetNeedChangeRFType(APP_RF_TYPE_DVB_T);
				}
				APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_DTV);
			}
#endif
	     //get current ci status			
#ifdef CONFIG_CI_SUPPORT
			APP_DVB_CI_GetCiStatus();
#endif
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:DTV-T***\n");
			break;
#ifdef CONFIG_DVB_SYSTEM_DVBC_SUPPORT		
		    case UI_EVENT_AUTO_DTVC:
			APP_GUIOBJ_Source_GetCurrSource(&eSourType);
#ifdef CONFIG_SUPPORT_ATV_SCAN_NTSCM_PALM_N //for_Colombia ATV&DTV Using the same RFType to swich Air&Cable
				if(eSourType == APP_SOURCE_ATV)
				{
					APP_GUIOBJ_Channel_SetATVTypeByStringID(TV_IDS_String_Cable, eSourType);
				}
				else if(eSourType == APP_SOURCE_DTV)
				{
					APP_GUIOBJ_Channel_SetATVTypeByStringID(TV_IDS_String_Cable, eSourType);
					APP_GUIOBJ_Channel_SetRFTypeByStringID(TV_IDS_String_Cable);
				}
				else 
				{
					if(RFtype != APP_RF_TYPE_DVB_C)
					{
						APP_GUIOBJ_Channel_SetNeedChangeRFType(APP_RF_TYPE_DVB_C);
					}
					
					APP_GUIOBJ_Channel_SetATVTypeByStringID(TV_IDS_String_Cable, eSourType);
					APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_DTV);
				}
#else
			AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_CHANNEL, 0,
				sizeof(APP_SETTING_Channel_t), &(g_stChannelData));
			RFtype = g_stChannelData.TV_Connection;
			APP_GUIOBJ_Source_GetCurrSource(&eSourType);
			if((eSourType == APP_SOURCE_DTV)||((eSourType == APP_SOURCE_RADIO)))
			{
				if(RFtype != APP_RF_TYPE_DVB_C)
				{
					 StringId = APP_GUIOBJ_Channel_GetStringIDByRFType(APP_RF_TYPE_DVB_C);
					 APP_GUIOBJ_Channel_SetRFTypeByStringID(StringId);
				}			 
			}
			else 
			{
				if(RFtype != APP_RF_TYPE_DVB_C)
				{
					APP_GUIOBJ_Channel_SetNeedChangeRFType(APP_RF_TYPE_DVB_C);
				}
				APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_DTV);
			}
#endif
#ifdef CONFIG_CI_SUPPORT
			APP_DVB_CI_GetCiStatus();
#endif
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:DTV-C***\n");
			break;
#endif
#ifdef CONFIG_DVB_SYSTEM_DVBS_SUPPORT		
		case UI_EVENT_AUTO_DTVS:
		case UI_EVENT_GREEN:
			AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_CHANNEL, 0,
				sizeof(APP_SETTING_Channel_t), &(g_stChannelData));
			RFtype = g_stChannelData.TV_Connection;
			APP_GUIOBJ_Source_GetCurrSource(&eSourType);
			if((eSourType == APP_SOURCE_DTV)||((eSourType == APP_SOURCE_RADIO)))
			{
				if(RFtype != APP_RF_TYPE_DVB_S)
				{
					 StringId = APP_GUIOBJ_Channel_GetStringIDByRFType(APP_RF_TYPE_DVB_S);
					 APP_GUIOBJ_Channel_SetRFTypeByStringID(StringId);
				}			 
			}
			else 
			{
				if(RFtype != APP_RF_TYPE_DVB_S)
				{
					APP_GUIOBJ_Channel_SetNeedChangeRFType(APP_RF_TYPE_DVB_S);
				}
				APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_DTV);
			}			//get current ci status
#ifdef CONFIG_CI_SUPPORT
			APP_DVB_CI_GetCiStatus();
#endif
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:DTV-S***\n");
			break;
#endif
		case UI_EVENT_AUTO_AV1:
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_AV);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:AV1***\n");
			break;
            
		case UI_EVENT_AUTO_AV2:
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_AV2);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:AV2***\n");
			break;
            
		case UI_EVENT_AUTO_AV3:
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_AV);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:AV3***\n");
			break;
            
		case UI_EVENT_AUTO_SVIDEO1:
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_SVIDEO);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:SV1***\n");
			break;
            
		case UI_EVENT_AUTO_SVIDEO2:
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_SVIDEO2);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:SV2***\n");
			break;
            
#ifdef CONFIG_DVB_SYSTEM
		case UI_EVENT_AUTO_SCART1:
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_SCART);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:SCART1***\n");
#if 0
			//get current signal type cvbs or RGB
			GL_TaskSleep(2000);
			if(TVFE_TvDecGetScartMode() == TVFE_TvDec_SCART_CVBS
				||TVFE_TvDecGetScartMode() == TVFE_TvDec_SCART_MIX
				)
			{
				Enable_Debug_Message(1<<MODULEID_UMF);
				printf("\n***SCART1 CVBS***\n");
			}
			else if(TVFE_TvDecGetScartMode() == TVFE_TvDec_SCART_RGB)
			{
				Enable_Debug_Message(1<<MODULEID_UMF);
				printf("\n***SCART1 RGB***\n");
			}
			if(APP_SCARTIN_GetAspectRatio() == APP_SCART_ASPECTRATIO_43)
			{
				Enable_Debug_Message(1<<MODULEID_UMF);
				printf("\n***ASPECT 4:3***\n");
			}
			else if(APP_SCARTIN_GetAspectRatio() == APP_SCART_ASPECTRATIO_169)
			{
				Enable_Debug_Message(1<<MODULEID_UMF);
				printf("\n***ASPECT 16:9***\n");
			}
#endif
			break;

		case UI_EVENT_AUTO_SCART2:
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_SCART1);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:SCART2***\n");

#if 0
			//get current signal type cvbs or RGB
			if(TVFE_TvDecGetScartMode() == TVFE_TvDec_SCART_CVBS
				||TVFE_TvDecGetScartMode() == TVFE_TvDec_SCART_MIX
				)
			{
				Enable_Debug_Message(1<<MODULEID_UMF);
				printf("\n***SCART2 CVBS***\n");
			}
			else if(TVFE_TvDecGetScartMode() == TVFE_TvDec_SCART_RGB)
			{
				Enable_Debug_Message(1<<MODULEID_UMF);
				printf("\n***SCART2 RGB***\n");
			}
			if(APP_SCARTIN_GetAspectRatio() == APP_SCART_ASPECTRATIO_43)
			{
				Enable_Debug_Message(1<<MODULEID_UMF);
				printf("\n***ASPECT 4:3***\n");
			}
			else if(APP_SCARTIN_GetAspectRatio() == APP_SCART_ASPECTRATIO_169)
			{
				Enable_Debug_Message(1<<MODULEID_UMF);
				printf("\n***ASPECT 16:9***\n");
			}
#endif
			break;
#endif
		case UI_EVENT_AUTO_YPBPR1:
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_YPBPR);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:YPBPR1***\n");
			break;
            
		case UI_EVENT_AUTO_YPBPR2:
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_YPBPR2);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:YPBPR2***\n");
			break;
            
		case UI_EVENT_AUTO_YPBPR3:
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_YPBPR);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:YPBPR3***\n");
			break;
            
		case UI_EVENT_AUTO_HDMI1:
			APP_GUIOBJ_Source_GetSourceTypeSupportTypeByStrID(TV_IDS_String_HDMI,&eSourType,NULL);
			if(eSourType==APP_SOURCE_MAX)
			{
			  APP_GUIOBJ_Source_GetSourceTypeSupportTypeByStrID(TV_IDS_String_HDMI1,&eSourType,NULL);
			}
			if(eSourType==APP_SOURCE_MAX)
			{
			  eSourType = APP_SOURCE_HDMI;
			}
			APP_GUIOBJ_Source_SetAppSource(eSourType);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:HDMI1***\n");
			break;
            
		case UI_EVENT_AUTO_HDMI2:
			APP_GUIOBJ_Source_GetSourceTypeSupportTypeByStrID(TV_IDS_String_HDMI2,&eSourType,NULL);
			if(eSourType==APP_SOURCE_MAX)
			{
				eSourType = APP_SOURCE_HDMI1;
			}
			APP_GUIOBJ_Source_SetAppSource(eSourType);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:HDMI2***\n");
			break;
            
		case UI_EVENT_AUTO_HDMI3:
			APP_GUIOBJ_Source_GetSourceTypeSupportTypeByStrID(TV_IDS_String_HDMI3,&eSourType,NULL);
			if(eSourType==APP_SOURCE_MAX)
			{
				eSourType = APP_SOURCE_HDMI2;
			}
			APP_GUIOBJ_Source_SetAppSource(eSourType);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:HDMI3***\n");
			break;
#if 0
		case UI_EVENT_AUTO_HDMI4:
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_HDMI);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:HDMI4***\n");
			break;
#endif
		case UI_EVENT_AUTO_PC:
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_PC);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:VGA***\n");
			break;
            
		case UI_EVENT_AUTO_MEDIA:
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_MEDIA);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:USB***\n");
			break;
            
		case UI_EVENT_AUTO_DVD:
			APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_DVD);
			Enable_Debug_Message(1<<MODULEID_UMF);
			printf("\n***FAC:DVD***\n");
			break;
            
		case UI_EVENT_10:
			APP_GUIOBJ_Source_Set_Next_Pre_AppSource(TRUE);
			break;

		default:
			break;
	}
	Enable_Debug_Message(0);
}
#endif
#ifdef CONFIG_ATV_SUPPORT
static INT32 Close_ATVApp_AllMenu(void)
{
	UINT32 u32GuiObj =0;

	for (u32GuiObj =0; u32GuiObj < ATV_GUIOBJ_MAX; u32GuiObj++)
	{
		if (u32GuiObj == ATV_GUIOBJ_PLAYBACK
			|| u32GuiObj == ATV_GUIOBJ_FREEZE
			|| u32GuiObj == ATV_GUIOBJ_POPMSG
			|| u32GuiObj == APP_GUIOBJ_MUTE)
		{
			continue;
		}

		if (SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV, u32GuiObj))
		{
			SYSAPP_GOBJ_DestroyGUIObject(SYS_APP_ATV, u32GuiObj);
		}
	}

    return SP_SUCCESS;
}
#endif
#ifdef CONFIG_DTV_SUPPORT
static INT32 Close_DVBApp_AllMenu(void)
{
    UINT32 u32GuiObj = 0;

    for (u32GuiObj = 0; u32GuiObj < DVB_GUIOBJ_MAX; u32GuiObj++)
    {
        if (u32GuiObj == DVB_GUIOBJ_PLAYBACK
#ifdef CONFIG_SUPPORT_SUBTITLE
            || u32GuiObj == DVB_GUIOBJ_SUBTITLE
#endif
#ifdef CONFIG_SUPPORT_MHEG5
            || u32GuiObj == DVB_GUIOBJ_MHEG5
#endif
            || u32GuiObj == DVB_GUIOBJ_FREEZE
            || u32GuiObj == APP_GUIOBJ_MUTE)
        {
            continue;
        }

        if (SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, u32GuiObj))
        {
            SYSAPP_GOBJ_DestroyGUIObject(SYS_APP_DVB, u32GuiObj);
        }
    }
#ifdef CONFIG_SUPPORT_TTX
    if (SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_TTX))
    {
        SYSAPP_GOBJ_DestroyGUIObject(SYS_APP_DVB, DVB_GUIOBJ_TTX);
    }
#endif

    return SP_SUCCESS;
}
#endif

static INT32 Close_AllMenu(UINT8 bSystemAppIndex)
{
#ifdef CONFIG_ATV_SUPPORT
	if(bSystemAppIndex == SYS_APP_ATV)
	{
		Close_ATVApp_AllMenu();
	}
#endif
#ifdef CONFIG_DTV_SUPPORT
	if(bSystemAppIndex == SYS_APP_DVB)
	{
		Close_DVBApp_AllMenu();
	}
#endif

    return SP_SUCCESS;
}

INT32 APP_Factory_CheckNeedReopenFm(UINT8 bSystemAppIndex)
{
	Boolean ret = FALSE;
	if(g_bNeedReopenWBajust == TRUE)
	{
		g_bNeedReopenWBajust = FALSE;
		Close_AllMenu(bSystemAppIndex);
	}
	else if(g_bNeedReopenPicture == TRUE)
	{
		g_bNeedReopenPicture = FALSE;
		Close_AllMenu(bSystemAppIndex);
	}
	else if(g_bNeedReopenCurveSetting == TRUE)
	{
		g_bNeedReopenCurveSetting = FALSE;
		Close_AllMenu(bSystemAppIndex);
	    if (!SYSAPP_GOBJ_GUIObjectExist(bSystemAppIndex, APP_GUIOBJ_FM_FACTORYSETTING))
	    {
			ret = TRUE;
			SYSAPP_GOBJ_CreateGUIObject_WithPara(bSystemAppIndex, APP_GUIOBJ_FM_FACTORYSETTING, u32NodeHeadId);
	    }
	}
    return ret;
}

int APP_CustomerFuncStart(UINT32 *pdMessage, UINT32 *pdParam)
{
	static UINT8 u8FactoryEventCount = 0;
#ifdef SUPPORT_HOTEL_MODE_OPENHOTEL_WITH_PINCODE
	static UINT8 u8HotelEventCount = 0;
#endif
#ifdef SUPPORT_PVR_ONOFF
    static UINT8 u8PVREventCount = 0;
#endif
#ifdef SUPPORT_EPG_ONOFF
        static UINT8 u8EPGEventCount = 0;
#endif
#ifdef SUPPORT_HIDE_TV_OR_ATV
        static UINT8 u8HideTVSourceCount = 0;
#endif

#ifdef SUPPORT_PASSWORD_OPEN_DEMO_MODE
        static UINT8 u8Demo_PW_Count = 0;
#endif
#ifdef CONFIG_KEYPAD_SINGLE_REUSE
	static UINT32 PreKey = UI_EVENT_NULL;
#endif
#ifdef CONFIG_CTV_UART_FAC_MODE
	static UINT8 u8CleanFacTestEventCount = 0;
#endif
	static UINT8 u8InitializeEventCount = 0;
	static UINT8 u8UpgradeEventCount = 0;
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
			sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData));
    
    if ((*pdMessage & APPLICATION_EXTERNAL_UI_MESSAGE_TYPE) &&
		((*pdMessage & APPLICATION_MESSAGE_MASK) < UI_EVENT_NULL))
    {
		UINT32 dMessage = *pdMessage & APPLICATION_MESSAGE_MASK;
        
#ifdef SUPPORT_FACTORY_AUTO_TEST
        extern int APP_Factory_GetAutoTestOnOff(void);
        if(APP_Factory_GetAutoTestOnOff() == TRUE)
        {
#ifdef CONFIG_CTV_UART_FAC_MODE
            extern void con_AccuviewRs232_IR_Response(AppGlobalEvent_t irEvent);
            if (dMessage >= UI_EVENT_KEYPAD_MIN && dMessage < UI_EVENT_NULL)
            {
                con_AccuviewRs232_IR_Response(dMessage);
            }
#endif
        }
#endif
#if 0
#ifdef CONFIG_DVB_SYSTEM
		AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_CHANNEL, 0,
				sizeof(APP_SETTING_Channel_t), &(g_stChannelData));

		if(AREA_NORWAY == g_stChannelData.Country)
		{
			if(dMessage >= UI_EVENT_0 && dMessage < UI_EVENT_NULL)
			{
				INT32 dFocusedIndex;
				APP_Source_Type_t eSourType = APP_SOURCE_MAX;
				UINT32 bSystemAppIndex = 0;
				al_uint8 bSignalState = al_false;
				INT16 i16ProgState = MID_PLAYBK_PROG_STATE_NORMAL;
				APP_GUIOBJ_Source_GetCurrSource(&eSourType);
				MAINAPP_GetActiveSystemAppIndex(&bSystemAppIndex);
				AL_DVB_Monitor_GetState(AL_DVB_MONITOR_STATE_SIGNAL, &bSignalState);
				AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO, 0,
						sizeof(APP_SETTING_UserInfo_t), &(g_stUserInfoData));
				i16ProgState = APP_DVB_Playback_GetProgStateByHandle(APP_DVB_Playback_GetCurrentNetType(), AL_DB_INVALIDHDL);
				if (SYSTEM_APP_NO_FOCUSED_GUIOBJ != SYSAPP_GOBJ_GetFocusedGUIObject(bSystemAppIndex, &dFocusedIndex)
					&&g_stUserInfoData.Mute != TRUE)
				{
				    if(((APP_SOURCE_DTV == eSourType || APP_SOURCE_RADIO == eSourType)
				    	&& (MID_PLAYBK_PROG_STATE_SCRAMBLED == i16ProgState || MID_PLAYBK_PROG_STATE_LOCKED == i16ProgState
				    	|| DVBApp_CheckServiceNotAvailable() == TRUE))
				    	||(eSourType == APP_SOURCE_ATV && APP_GUIOBJ_ATV_Playback_GetProgramState())
				    	||(eSourType == APP_SOURCE_MEDIA))
				    {
				    	APP_Audio_SetMuteSpeaker(FALSE);
				    	TVFE_Audio_Play_Audio_Effect((UINT32)MCF_BufAlloc(MEM_ANCHOR_AUDIO_UI),4589);
				    }
				    else
				    {
				    	TVFE_Audio_Play_Audio_Effect((UINT32)MCF_BufAlloc(MEM_ANCHOR_AUDIO_UI),4589);
				    }
				}
			}
		}
#endif
#endif
	//reset no signal standby timer when ir event action
	if(dMessage >= UI_EVENT_0 && dMessage < UI_EVENT_NULL)
	{
		APP_Source_Type_t eSourType = APP_SOURCE_MAX;
		APP_GUIOBJ_Source_GetCurrSource(&eSourType);
#ifdef CONFIG_DTV_SUPPORT
		 if(APP_SOURCE_DTV == eSourType || APP_SOURCE_RADIO == eSourType)
		 {
			al_uint8 bMonitorState = al_false;
			al_uint8 bSignalState = al_false;
			AL_DVB_Monitor_GetState(AL_DVB_MONITOR_STATE_MONITOR, &bMonitorState);
			AL_DVB_Monitor_GetState(AL_DVB_MONITOR_STATE_SIGNAL, &bSignalState);
			
			if (bMonitorState == al_true && bSignalState != AL_DVB_MONITOR_VALUE_TRUE)//No signal
			{
				DVBApp_NoSignal_StandbyTimerReset();
			}
		 }
		 else 
#endif
		 if(eSourType != APP_SOURCE_MEDIA && eSourType != APP_SOURCE_DVD 
		 	&& eSourType != APP_SOURCE_ANDRO)
		 {
#ifdef CONFIG_ATV_SUPPORT
			if(ATVApp_GetSignalState() == ATV_NO_SIGNAL)//No signal
			{
		 		ATVAPP_NoSignal_StandbyTimerReset();
			}
#endif
		 }
	}
		
#ifdef AC_ON_AUTO_GET_TIME
        if (g_fBackgroundGetTime == TRUE)
        {
            // if back ground on state, not to respond power key
			dMessage = UI_EVENT_NULL;
        }
#endif

	#ifdef SUPPORT_LED_FLASH
		if ((dMessage == UI_EVENT_MultiPanelIndex) &&(LED_FLASH_TIMER_REMOTE > 0))
		{
			APP_LED_Flash_Timer_Set(LED_FLASH_TYPE_REMOTE, 0);
			//MID_GPIO_SetGPIOOffLevel(GPIO_LED_G_ON_PIN);/*mark for S2tek new Arch*/
		}
		#ifdef SUPPORT_LED_FLASH_WHEN_IR_PRESS
		else if(dMessage!=UI_EVENT_POWER)
		{
			APP_LED_Flash_Timer_Set_FullSetting(LED_FLASH_TYPE_REMOTE, 0,1);
		}
		#endif
	#endif

		UINT32 bSystemAppIndex = 0;
		UINT32 eType;
		if((dMessage >= UI_EVENT_0)&&(dMessage <= UI_EVENT_9))
		{
			MAINAPP_GetActiveSystemAppIndex(&bSystemAppIndex);
			APP_GUIOBJ_PopMsg_GetMsgType(&eType);
#ifdef CONFIG_DTV_SUPPORT
			if (bSystemAppIndex == SYS_APP_DVB)
			{
				if ((SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_POPMSG)&&(eType == POPMSG_TYPE_PWD))
					||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_CHANGEPINCODE)))
						return 0;
			}
#endif
#ifdef CONFIG_ATV_SUPPORT
			if(bSystemAppIndex == SYS_APP_ATV)
			{
				if ((SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV, ATV_GUIOBJ_POPMSG)&&(eType == POPMSG_TYPE_PWD))
					||(SYSAPP_GOBJ_GUIObjectExist(SYS_APP_ATV, ATV_GUIOBJ_CHANGEPINCODE)))
						return 0;
			}
#endif
#ifdef CONFIG_MEDIA_SUPPORT
			if(bSystemAppIndex == SYS_APP_FILE_PLAYER)
			{
				if ((SYSAPP_GOBJ_GUIObjectExist(SYS_APP_FILE_PLAYER, MEDIA_GUIOBJ_POPMSG_MAINMENU)&&(eType == POPMSG_TYPE_PWD))	)
		        	return 0;
			}
#endif
		}
		
#ifdef SUPPORT_SHOP_DEMO_MODE_IN_MAINMENU
		if( (dMessage >= UI_EVENT_0 && dMessage < UI_EVENT_NULL) && (dMessage != UI_EVENT_POWER))//&&(dMessage != UI_EVENT_MUTE))
		{
		       if(ShopDemoCountTimer!=199)
			ShopDemoCountTimer=0;
		}
#endif
		
		if (APP_Factory_CheckComponentEvent(dMessage, g_arFactoryEventTable, g_u32FactoryEventTableSize, &u8FactoryEventCount))
		{
			dMessage = UI_EVENT_FACTORY | PASS_TO_SYSAPP;
		}
#ifdef SUPPORT_HOTEL_MODE_OPENHOTEL_WITH_PINCODE
	 	else if (APP_Factory_CheckComponentEvent(dMessage, g_arFactoryHotelEventTable, g_u32FactoryHotelEventTableSize, &u8HotelEventCount))
		{
			dMessage = UI_EVENT_FAC_HOTEL | PASS_TO_SYSAPP;
		}
#endif
#ifdef SUPPORT_PVR_ONOFF
        else if (APP_Factory_CheckComponentEvent(dMessage, g_arPVROnOffEventTable, g_u32PVROnOffEventTableSize, &u8PVREventCount))
        {
            dMessage = UI_EVENT_PVR_ONOFF | PASS_TO_SYSAPP;
        }
#endif
#ifdef SUPPORT_EPG_ONOFF
        else if (APP_Factory_CheckComponentEvent(dMessage, g_arEPGOnOffEventTable, g_u32EPGOnOffEventTableSize, &u8EPGEventCount))
        {
            dMessage = UI_EVENT_EPG_ONOFF | PASS_TO_SYSAPP;
        }
#endif
#ifdef SUPPORT_HIDE_TV_OR_ATV
	else if (APP_Factory_CheckComponentEvent(dMessage, g_arHideTVSourceTable, g_arHideTVSourceTableSize, &u8HideTVSourceCount))
        {
            dMessage = UI_EVENT_HIDETVSOURCE | PASS_TO_SYSAPP;
        }
#endif

#ifdef SUPPORT_PASSWORD_OPEN_DEMO_MODE
	else if (APP_Factory_CheckComponentEvent(dMessage, g_arDemo_PW_Table, g_arDemo_PW_TableSize, &u8Demo_PW_Count))
        {
            dMessage = UI_EVENT_SHOP| PASS_TO_SYSAPP;
        }
#endif

		else if (APP_Factory_CheckComponentEvent(dMessage, g_arFactoryInitializeEventTable, g_u32FactoryInitializeEventTableSize, &u8InitializeEventCount))
		{
			dMessage = UI_EVENT_INITIAL | PASS_TO_SYSAPP;
		}
		else if (dMessage == UI_EVENT_MultiPanelIndex)
		{
			dMessage = UI_EVENT_MultiPanelIndex | PASS_TO_SYSAPP;
		}
		else if ((g_stFactoryUserData.n_FactSet_BurningMode == 1) && (APP_Factory_CheckFactoryModeExitCode(dMessage) == SP_SUCCESS))
		{
			dMessage = UI_EVENT_NULL;
		}
		else if(APP_Factory_CheckComponentEvent(dMessage, g_arFactoryUpgradeEventTable, g_u32FactoryUpgradeEventTableSize, &u8UpgradeEventCount))
		{
			dMessage = UI_EVENT_CHECK_UPGRADE | PASS_TO_SYSAPP;
		}
#ifdef CONFIG_CTV_UART_FAC_MODE       
		else if(APP_Factory_CheckComponentEvent(dMessage, g_arFactoryCleanFacTestEventTable, g_u32FactoryCleanFacTestEventTableSize, &u8CleanFacTestEventCount))	
		{
			dMessage = UI_EVENT_CLEAN_FAC_TEST | PASS_TO_SYSAPP;
		}
#endif
		else
		{
			if (pdParam)
			{
				if ((dMessage >= UI_EVENT_KEYPAD_MIN) && (dMessage <= UI_EVENT_KEYPAD_MAX))
				{
					Keypad_EventConvert(&dMessage);                    
#ifdef SUPPORT_FACTORY_AUTO_TEST
					//extern int APP_Factory_GetAutoTestOnOff(void);
					if(APP_Factory_GetAutoTestOnOff() == TRUE)
					{
						UINT32	dIndex = 0;
						MAINAPP_GetActiveSystemAppIndex(&dIndex);
						if(!SYSAPP_GOBJ_GUIObjectExist(dIndex,APP_GUIOBJ_BANNER))
						{
							#ifdef CONFIG_DTV_SUPPORT
							if(dIndex == SYS_APP_DVB)
							{
								Close_DVBApp_AllMenu();
							}
							#endif
							#ifdef CONFIG_ATV_SUPPORT
							if(dIndex == SYS_APP_ATV)
							{
								Close_ATVApp_AllMenu();
							}
							#endif
							#ifdef CONFIG_MEDIA_SUPPORT
							if(dIndex == SYS_APP_FILE_PLAYER)
							{
								extern INT32 APP_MenuMgr_Exist_Main_Menu(void);
								extern INT32 APP_MenuMgr_Exit_Main_Menu(void);
								if (APP_MenuMgr_Exist_Main_Menu())
								{
									if (SYSAPP_GOBJ_GUIObjectExist(SYS_APP_FILE_PLAYER, MEDIA_GUIOBJ_POPMSG_MAINMENU))
									{
										SYSAPP_GOBJ_DestroyGUIObject(SYS_APP_FILE_PLAYER, MEDIA_GUIOBJ_POPMSG_MAINMENU);
									}
									APP_MenuMgr_Exit_Main_Menu();
									SYSAPP_GOBJ_DestroyGUIObject(SYS_APP_FILE_PLAYER, MEDIA_GUIOBJ_NULL);
								}
							}
							#endif
							SYSAPP_IF_SendGlobalEventWithIndex(dIndex,
								dMessage, 0);
						}
						else
							SYSAPP_GOBJ_SendMsgToSingleGUIObject(dIndex, APP_GUIOBJ_BANNER, dMessage,0);
					}
#endif
				}
#ifdef CONFIG_KEYPAD_SINGLE_REUSE
                else if((dMessage == UI_EVENT_KEYPAD_SHORT_PRESS))
                {
					UINT32	dIndex = 0;
					MAINAPP_GetActiveSystemAppIndex(&dIndex);
					if(dIndex == SYS_APP_DVB)
					{
				        if (SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_PROGINFO))
				        {
				            SYSAPP_GOBJ_DestroyGUIObject(SYS_APP_DVB, DVB_GUIOBJ_PROGINFO);
				        }
					}
                    if(!(SYSAPP_GOBJ_GUIObjectExist(dIndex,APP_GUIOBJ_KEYPADMENU)))
                    {
						if ((dIndex == SYS_APP_ATV) && (ATVApp_IsPopupExist(PLAYBACK_POPUP_MSG_NO_SIGNAL)))
		                {
		                    ATVApp_ClosePopup(PLAYBACK_POPUP_MSG_NO_SIGNAL, UI_EVENT_NULL);
		                }
						else if(dIndex == SYS_APP_DVB)
						{
							DVBApp_ResetPopup_Channel();
						}
                        SYSAPP_IF_SendGlobalEventWithIndex(dIndex,
                            APP_GLOBAL_EVENT_KEYPADMENU_OPEN | PASS_TO_SYSAPP, dMessage);
                    }
                    else
                    {
                        SYSAPP_GOBJ_SendMsgToSingleGUIObject(dIndex,APP_GUIOBJ_KEYPADMENU,dMessage,0);
                    }
                }
#endif

				else
				{
				#ifdef CELLO_BATHROOMTV
					if(_MAINAPP_GetFakeStandbyStatue()==TRUE)
					{
						if(dMessage!=UI_EVENT_FAKESTANDBY)
						{
							dMessage=UI_EVENT_NULL;
						}
					}
				#endif
					UINT32 dIndex = 0;
					while(dIndex < dIR_map_size)
					{
						if (stIR_map[dIndex].wCID == (*pdParam&0xFFFF))
						{
							stIR_map[dIndex].pfIRConvert(&dMessage);
							break;
						}
						dIndex++;
					}
				}
			}
		}

#ifdef CONFIG_KEYPAD_SINGLE_REUSE
		if ((*pdParam&KEYPAD_EVENT_REPEAT_TYPE) == 0)
		{
			PreKey = UI_EVENT_NULL;
		}
#endif
        
		if ((APP_RepeatEventCheck(dMessage&(~PASS_TO_SYSAPP)) == FALSE) &&
			((*pdParam & (KEYPAD_EVENT_REPEAT_TYPE|IR_EVENT_REPEAT_TYPE)) != 0))
		{
 #ifdef CONFIG_KEYPAD_SINGLE_REUSE
		   	if ((((dMessage&(~PASS_TO_SYSAPP)) == UI_EVENT_SOURCE)) && ((*pdParam&KEYPAD_EVENT_REPEAT_TYPE) != 0))
		   	{

			}
			else if (PreKey != (dMessage&(~PASS_TO_SYSAPP)) && ((*pdParam&KEYPAD_EVENT_REPEAT_TYPE) != 0))
			{
				PreKey = (dMessage&(~PASS_TO_SYSAPP));
			}
			else
#endif
		   	{
		   		printf("\n[L%d] ============convert to UI_EVENT_NULL==========>pdParam: 0x%x\n",__LINE__,*pdParam);
				dMessage = UI_EVENT_NULL;
			}
		}
#ifdef SUPPORT_USB_UPGRADE_LONG_PRESS_KEYPAD_POWER
        else if(((*pdParam & KEYPAD_EVENT_REPEAT_TYPE) != 0) && (IsKeypadPowerOnPressRepeat == TRUE)
                && (USB_ATTACHED == TRUE))
        {
            printf("\n[L%d] ============convert to UI_EVENT_KEYPAD_POWER_UPGRADE==========>\n",__LINE__);
            dMessage = UI_EVENT_KEYPAD_POWER_UPGRADE;
        }
#endif
		*pdMessage = (*pdMessage & APPLICATION_MESSAGE_TYPE_MASK) | dMessage;
    }

    if (g_stFactoryUserData.n_FactSet_BurningMode == 1)
    {
        extern UINT8 gIsFactoryResetting;
        UINT32  dIndex;
		if(TRUE == APP_Factory_GetAutoTestOnOff() && (!gIsFactoryResetting))
		{
				g_stFactoryUserData.n_FactSet_BurningMode = 0;
				if(AL_FLASH_GetACAutoPowerOn() > 0)
				{
					MID_TVFE_SetAutoPowerOn(TRUE);
				}
				else if(AL_FLASH_GetACAutoPowerOn() == 0)
				{
					MID_TVFE_SetAutoPowerOn(FALSE);
				}
				AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
				ITEM_OFFSET(APP_SETTING_FactoryUser_t, n_FactSet_BurningMode),
				sizeof(g_stFactoryUserData.n_FactSet_BurningMode),&(g_stFactoryUserData.n_FactSet_BurningMode));
				AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
				ITEM_OFFSET(APP_SETTING_FactoryUser_t, n_FactSet_BurningMode),
				sizeof(g_stFactoryUserData.n_FactSet_BurningMode));
		}
        else if (MAIN_APP_SUCCESS == MAINAPP_GetActiveSystemAppIndex(&dIndex)
			&&(!gIsFactoryResetting))
		{
		

			MID_TVFE_SetAutoPowerOn(TRUE);	//zhouqp add for burning power on 20140903 

        	APP_Cul_Fm_CheckAndStartBurningMode();
		}
    }

    return 0;
}

int APP_CustomerFuncEnd(UINT32 *pdMessage, UINT32 *pdParam)
{
	UINT32 dMessage;

    if (*pdMessage & APPLICATION_EXTERNAL_UI_MESSAGE_TYPE)
	{
		dMessage = *pdMessage & APPLICATION_MESSAGE_MASK & (~PASS_TO_SYSAPP);
		if (dMessage <= UI_EVENT_NULL)
	    {
		#ifdef SUPPORT_LED_FLASH
        #ifdef AC_ON_AUTO_GET_TIME
            if (g_fBackgroundGetTime == TRUE)
            {
                return 0;
            }
        #endif
		#endif
	    }
    }

    return 0;
}

#ifdef SUPPORT_FACTORY_AUTO_TEST
static Boolean FactoryAutoTestStartFlag = FALSE;
static Boolean SwUpgradeandAutoTestFlag = FALSE;
static UINT8 g_u8PoweronEnterVersion = 0;

void APP_SetPoweronEnterVersion(UINT8 value)
{
	g_u8PoweronEnterVersion = value;
}

UINT8 APP_GetPoweronEnterVersion(void)
{
	return g_u8PoweronEnterVersion;
}

void APP_Factory_SetAutoTestOnOff(Boolean SetOn)
{
	FactoryAutoTestStartFlag = SetOn;
}

int APP_Factory_GetAutoTestOnOff(void)
{
	return FactoryAutoTestStartFlag;
}
void APP_Factory_SetUpgradeandAutoTestFlag(Boolean Flag)
{
	SwUpgradeandAutoTestFlag = Flag;
}

int APP_Factory_GetUpgradeandAutoTestFlag(void)
{
	return SwUpgradeandAutoTestFlag;
}
#endif
#if defined(CONFIG_CTV_UART_FAC_MODE)
static Boolean FactoryUartModeAutoTestStartFlag = FALSE;
void APP_Factory_SetUartFacModeOnOff(Boolean SetOn)
{
	FactoryUartModeAutoTestStartFlag = SetOn;
}
int APP_Factory_GetUartFacModeOnOff(void)
{
	return FactoryUartModeAutoTestStartFlag;
}
#endif

/*PC DB-Sort begin*/
struct stDBSCSVNODE
{
	int key;
	UINT16 usProgNo;
	UINT32 usProgHandle;	
	struct stDBSCSVNODE* next;
};

typedef enum _eDBSFINDTYPE
{
	DBSFIND = 0,
	DBSFINDDIFF,
	DBSNOTFIND,
	DBSFINDMAX,

}eDBSFINDTYPE;

#define DBS_FILE_NAME	      "DBS.CSV"
#define DBS_CSV_TITLE         "Channel No.,Service Id,Ts Id,Network Id, Handle,Service Type,Porg Name\n"
#define DBS_FILE_LEN 		  128
#define DBS_BUFFER_SIZE       256

#define CSV_DBS_DEBUG
#ifdef CSV_DBS_DEBUG
#define	CSV_DBS_DEBF(fmt, arg...)	UMFDBG(0,fmt, ##arg)
#else
#define CSV_DBS_DEBF(fmt, arg...) ((void) 0)
#endif
#define	CSV_DBS_DEBFERR(fmt, arg...)	UMFDBG(0,fmt, ##arg)

static UINT32 g_u32keyNum = 0;
static USB_File_Handler_t DBCSVFD = -1;
static char * DBCSVNAME = NULL;
static struct stDBSCSVNODE pstDBHead;


static Boolean _APP_Factory_ImportDBSAddNode(struct stDBSCSVNODE* head, struct stDBSCSVNODE* Node)
{
	struct stDBSCSVNODE *temp;
	struct stDBSCSVNODE *temp1;

	if (Node == NULL || head == NULL)
	{
		CSV_DBS_DEBFERR("[error]%s		%d\n",__FUNCTION__,__LINE__);
		return FALSE;
	}

	temp = malloc(sizeof(struct stDBSCSVNODE));
	if (temp == NULL)
	{
		CSV_DBS_DEBFERR("[error]%s		%d\n",__FUNCTION__,__LINE__);
		return FALSE;
	}
	
	temp->key          = g_u32keyNum++; 
	temp->usProgHandle = Node->usProgHandle;
	temp->usProgNo     = Node->usProgNo;
	temp->next         = NULL;

	if (head->next == NULL)
	{
		head->next = temp;
		return TRUE;
	}
	
	temp1 = head->next;
	while (temp1->next != NULL)
	{
		temp1 = temp1->next;
	}
	temp1->next = temp;
	return TRUE;

}

static Boolean _APP_Factory_ImportDBSDelAllNode(struct stDBSCSVNODE* head)
{
	struct stDBSCSVNODE *temp;
	struct stDBSCSVNODE *Node;
	int i = 0;//test by kai.wang
	
	if (head == NULL)
	{
		CSV_DBS_DEBFERR("[error]%s		%d\n",__FUNCTION__,__LINE__);
		return FALSE;
	}
	
	Node = head->next;
	temp = head;
	while (Node != NULL)
	{
		i++;		
		temp->next = Node->next;
		free(Node);
		Node = temp->next;
	}
	head->next = NULL;
	CSV_DBS_DEBF("[debug delete node]total is %d\n",i);
	return TRUE;
}

static Boolean _APP_Factory_ImportDBSReadCSV(FILE *fp, char* buffer, int len, struct stDBSCSVNODE* node)
{
	INT8 	usProgNo[10];								
	INT8	usProgHandle[10];								

	if (buffer == NULL || node == NULL || fp == NULL)
	{
		CSV_DBS_DEBFERR("[error]%s		%d\n",__FUNCTION__,__LINE__);
		return FALSE;
	}
							
	memset(buffer, 0x0, len);
	memset(usProgNo, 0x0, 10);
	memset(usProgHandle, 0x0, 10);
	
	if (fgets(buffer, len, fp) == NULL)
	{
		CSV_DBS_DEBF("[get file EOF]%s		%d\n",__FUNCTION__,__LINE__);
		return FALSE;
	}
	
	sscanf(buffer,"%[^,],%*[^,],%*[^,],%*[^,],%[^,],%*[^,],%*[^,]", usProgNo,
		usProgHandle);
	
	node->usProgNo = atoi(usProgNo);
	node->usProgHandle = atoi(usProgHandle);

	return TRUE;	
}

static Boolean _APP_Factory_ImportDBSConList(void)
{
	FILE * fp = NULL;
	char* buffer;
	struct stDBSCSVNODE Node;

	fp = fdopen(DBCSVFD, "r+");
	if (fp == NULL)
	{
		CSV_DBS_DEBFERR("[error]%s		%d\n",__FUNCTION__,__LINE__);
		return FALSE;
	}

	buffer = malloc(DBS_BUFFER_SIZE+1);
	if (buffer == NULL)
	{
		CSV_DBS_DEBFERR("[error]%s		%d\n",__FUNCTION__,__LINE__);
		return FALSE;
	}
	
	fgets(buffer, strlen(DBS_CSV_TITLE), fp);
	CSV_DBS_DEBF("[debug]buffer:%s\n",buffer);

	memset(&pstDBHead, 0x0, sizeof(struct stDBSCSVNODE));
	pstDBHead.next = NULL;
	g_u32keyNum = 0;
	
	while(_APP_Factory_ImportDBSReadCSV(fp, buffer, DBS_BUFFER_SIZE, &Node) == TRUE)
	{
		if (_APP_Factory_ImportDBSAddNode(&pstDBHead, &Node) == FALSE)
		{
			CSV_DBS_DEBFERR("[error]%s		%d\n",__FUNCTION__,__LINE__);
			break;
		}
	}

	if (buffer != NULL)
	{
		free(buffer);
		buffer = NULL;
	}
	fclose(fp);
	fp = NULL;
	return TRUE;
}


eDBSFINDTYPE _APP_Factory_ImportDBFindList(struct stDBSCSVNODE * head,struct stDBSCSVNODE * Cop, UINT16* usProgNo)
{
	struct stDBSCSVNODE* Node = NULL;
	struct stDBSCSVNODE* temp = NULL;

	eDBSFINDTYPE Find = DBSNOTFIND;
	
	if (head == NULL)
	{
		CSV_DBS_DEBFERR("[error]%s		%d\n",__FUNCTION__,__LINE__);
		return Find;
	}

	Node = head->next;
	temp = head;
	*usProgNo = 0;

	while (Node != NULL)
	{
		if (Cop->usProgHandle == Node->usProgHandle)
		{
			Find = DBSFIND;
			if (Cop->usProgNo != Node->usProgNo)
			{
				Find = DBSFINDDIFF;
				*usProgNo = Node->usProgNo;
			}

			temp->next = Node->next;
			free(Node);
			Node = NULL;
			break;
		}
		temp = Node;
		Node = Node->next;
	}
	
	return Find;
}

static void _APP_Factory_ImportDBS_UpdatePR(void)
{
    AL_Return_t ret;
    AL_SCH_sched_id_t ref_id = 0 ;
    AL_SCH_sched_id_t ret_id;
    AL_PR_details_t details;
    Boolean bDeleteFlag = FALSE;
    AL_ServiceDetail_t stServInfor;
    //if the delete attribute of a PR handle is true, delete this PR

	ret = AL_PR_Get(AL_DBTYPE_DVB_S, AL_PR_REC_FIRST, AL_PR_EVT_REM | AL_PR_SRV_REM, ref_id, &ret_id);
    if (ret != AL_SUCCESS)
    {
    	CSV_DBS_DEBFERR("[error]%s		%d\n",__FUNCTION__,__LINE__);
        return;
    }

    do
    {
        if (bDeleteFlag == TRUE)
        {
            /*if delete ID x before get next ID by x, it will not success.
            so get next first then delete it*/
			CSV_DBS_DEBF("[delete:%d]%s		%d\n",ref_id,__FUNCTION__,__LINE__);
            AL_PR_Delete(ref_id);
            bDeleteFlag = FALSE;
        }
        memset(&details, 0, sizeof(AL_PR_details_t));
		CSV_DBS_DEBF("[ret_id:%d]%s		%d\n",ret_id,__FUNCTION__,__LINE__);
        if (AL_PR_GetById(ret_id, &details) != AL_SUCCESS)
        {
            break;
        }

        memset(&stServInfor, 0, sizeof(AL_ServiceDetail_t));
        if(AL_DB_QueryDetail(details.rem_details.handle, (al_void *)&stServInfor)!= AL_SUCCESS)
        {
            continue;
        }
        else
        {
			CSV_DBS_DEBF("[debug]%s		%d\n",__FUNCTION__,__LINE__);
			CSV_DBS_DEBF("handle:0x%x\n",details.rem_details.handle);
			CSV_DBS_DEBF("prog: %d,%s\n",stServInfor.stDVBSServ.usProgNo,stServInfor.stDVBSServ.szProgName);
    		if(1 == stServInfor.stDVBSServ.stProgAttrib.delete)
    		{
    			CSV_DBS_DEBF("[debug]%s		%d\n",__FUNCTION__,__LINE__);
                /*if delete ID x before get next ID by x, it will not success.
                so get next first then delete it*/
                bDeleteFlag = TRUE;
    		}
        }

		ref_id = ret_id;

	} while (AL_PR_Get(AL_DBTYPE_DVB_S, AL_PR_REC_NEXT, AL_PR_EVT_REM | AL_PR_SRV_REM, ref_id, &ret_id) == AL_SUCCESS);

    if (bDeleteFlag == TRUE)
    {
        //delete the last one that handle mached
		CSV_DBS_DEBF("[delete:%d]%s		%d\n",ref_id,__FUNCTION__,__LINE__);
        AL_PR_Delete(ref_id);
        bDeleteFlag = FALSE;
    }
	
    return;

}

/* Function: a.Obtain records from the file and build a record list.
 *           b.find need modify record.
 *           c.update channl list.
 * Param: 
 *		TRUE, OK
 *		FALSE,Failed
 */
Boolean APP_Factory_ImportDBSChangeDBbyCSV(void)
{
	int i;
	int DVBSChannelNumber;
	UINT16 usProgNo = 0;
	AL_RecHandle_t ChannelHandle;
	AL_RecHandle_t _hCurrProgHdl;
	AL_ServiceDetail_t stServInfo;
	Boolean IsDelCurProg = FALSE;
	eDBSFINDTYPE ret;

	struct stDBSCSVNODE Node;

	if (_APP_Factory_ImportDBSConList() == FALSE)
	{
		CSV_DBS_DEBFERR("[error]%s		%d\n",__FUNCTION__,__LINE__);
		return FALSE;
	}
	
	/*get record from system */
	 _hCurrProgHdl = APP_DVB_Playback_GetCurrentProgHandle(AL_DBTYPE_DVB_S,AL_RECTYPE_DVBTV|AL_RECTYPE_DVBRADIO|AL_RECTYPE_DVBDATA);

	APP_DVB_Playback_SetCurrentRFType(APP_RF_TYPE_DVB_S);
	AL_FW_SwitchDBModule(AL_DBTYPE_DVB_S);
	APP_Database_InitChnList(APP_DB_CHNLISTTYPE_DIGITAL,AL_DB_INVALIDHDL,ProgListType_Normal);
	DVBSChannelNumber = APP_Database_GetChnListSize();
	CSV_DBS_DEBF("[debug]%s		%d\n",__FUNCTION__,__LINE__);
	CSV_DBS_DEBF("DVBSChannelNumber is %d\n\n",DVBSChannelNumber);

	
	for (i = 0; i < DVBSChannelNumber; i++)
	{
		ChannelHandle = APP_Database_GetHdlByIdx(i);

		memset(&stServInfo, 0, sizeof(AL_ServiceDetail_t));

		AL_DB_QueryDetail(ChannelHandle, (al_void *)&stServInfo);
		Node.usProgHandle = ChannelHandle;
		Node.usProgNo    = stServInfo.stDVBSServ.usProgNo;
		Node.next        = NULL;
		Node.key         = 0;

		/* find need modify record */
		ret = _APP_Factory_ImportDBFindList(&pstDBHead, &Node, &usProgNo);
		switch (ret)
		{
			case DBSFINDDIFF:
				CSV_DBS_DEBF("0x%x has changed\n",Node.usProgHandle);
				CSV_DBS_DEBF("Node.usProgNo:%d -> usProgNo:%d\n",Node.usProgNo, usProgNo);
				AL_DB_UpdateDetailFieldByName(ChannelHandle, (UINT8*)"usProgNo", &usProgNo);
				break;
			
			case DBSNOTFIND:
				/*delete record*/
				CSV_DBS_DEBF("ProgNumber = %d,ProgHandle = 0x%x,don't find,need delete\n",Node.usProgNo,Node.usProgHandle);
				CSV_DBS_DEBF("ChannelHandle = 0x%x, _hCurrProgHdl = 0x%x\n",ChannelHandle,_hCurrProgHdl);
				CSV_DBS_DEBF("prog: %d,%s\n",stServInfo.stDVBSServ.usProgNo,stServInfo.stDVBSServ.szProgName);

				stServInfo.stDVBSServ.stProgAttrib.delete = 1;
				AL_DB_UpdateDetailField(ChannelHandle, FIELD_OFFSET(SDBServInfo_t, stProgAttrib),
					FIELD_SIZEOF(SDBServInfo_t, stProgAttrib), &stServInfo.stDVBSServ.stProgAttrib);
				AL_Event_UnLockChannels(AL_DBTYPE_DVB_S, stServInfo.stDVBSServ.usOrigNetId, stServInfo.stDVBSServ.usTsId, stServInfo.stDVBSServ.usServiceId);
				if (ChannelHandle == _hCurrProgHdl)
				{
					IsDelCurProg = TRUE;
				}
				break;
				
			case DBSFIND:
			default:
				break;
		}		
	}
	
	AL_DB_SortRecords(AL_DBTYPE_DVB_S, AL_RECTYPE_DVBSERVICE, AL_DB_SORTBY_NO, al_true);

	APP_Database_UninitChnList();
	APP_Database_InitChnList(APP_DB_CHNLISTTYPE_DIGITAL,AL_DB_INVALIDHDL,ProgListType_Normal);

	if (IsDelCurProg == TRUE)
	{
		CSV_DBS_DEBF("Current TP program (handle=0x%x) is removed!!!\n",_hCurrProgHdl);
		ChannelHandle = APP_Database_GetHdlByIdx(0);
		SYSAPP_IF_SendGlobalEventWithIndex(SYS_APP_DVB, APP_DVB_GLOBAL_EVENT_ZAPPING | PASS_TO_SYSAPP, ChannelHandle);
	}
	
//#ifdef CONFIG_SUPPORT_PVR
   _APP_Factory_ImportDBS_UpdatePR();
//#endif

	if (_APP_Factory_ImportDBSDelAllNode(&pstDBHead) == FALSE)
	{
		CSV_DBS_DEBFERR("[error]%s		%d\n",__FUNCTION__,__LINE__);
		return FALSE;
	}
	return TRUE;
}

/* Function: a.build channel list
 *           b.get record of channel list
 *           c.writer file.CSV
 * Param: 
 *		TRUE, OK
 *		FALSE,Failed
 */
Boolean  APP_Factory_ExportDBSWriteCSV(void)
{
	char tempString[DBS_BUFFER_SIZE]={0};
	int Data_Len;
	int i;
	int len;
	int DVBSChannelNumber;
	AL_RecHandle_t ChannelHandle;
	AL_ServiceDetail_t stServInfo;
	AL_DB_EDBType_t dbType;
	AL_DB_ERecordType_t recType;

	if (DBCSVFD < 0)
	{
		CSV_DBS_DEBFERR("[error]%s		%d\n",__FUNCTION__,__LINE__);
		return FALSE;
	}
	APP_DVB_Playback_SetCurrentRFType(APP_RF_TYPE_DVB_S);
	AL_FW_SwitchDBModule(AL_DBTYPE_DVB_S);
	APP_Database_InitChnList(APP_DB_CHNLISTTYPE_DIGITAL,AL_DB_INVALIDHDL,ProgListType_Normal);
	DVBSChannelNumber = APP_Database_GetChnListSize();

	memset(tempString, 0x0, sizeof(char)*DBS_BUFFER_SIZE);

	const unsigned char BOM[] = {0xEF, 0xBB, 0xBF};
	
	if (write(DBCSVFD, BOM, sizeof(BOM)) < (int)sizeof(BOM))
	{
		CSV_DBS_DEBFERR("[error]%s() %d\n", __FUNCTION__, __LINE__);
		return FALSE;
	}
	
	memcpy((char*)tempString, DBS_CSV_TITLE, strlen(DBS_CSV_TITLE));
	Data_Len = strlen(tempString);
	if (Data_Len == 0)
	{
		CSV_DBS_DEBFERR("[error]%s() %d, Source Data_Len = 0\n", __FUNCTION__, __LINE__);
		return FALSE;
	}
	
	if (write(DBCSVFD, tempString, Data_Len) < (int)Data_Len)
	{
		CSV_DBS_DEBFERR("[error]%s() %d\n", __FUNCTION__, __LINE__);
		return FALSE;
	}
	
	for (i=0; i < DVBSChannelNumber; i++)
	{
		ChannelHandle=APP_Database_GetHdlByIdx(i);
		memset(&stServInfo, 0, sizeof(AL_ServiceDetail_t));
		AL_DB_QueryDetail(ChannelHandle, (al_void *)&stServInfo);
		AL_DB_GetRecordType(ChannelHandle,&dbType,&recType);
		
		sprintf((char*)tempString, "%d,%d,%d,%d,%d,%s,",
			stServInfo.stDVBSServ.usProgNo,
			stServInfo.stDVBSServ.usServiceId,
			stServInfo.stDVBSServ.usTsId,
			stServInfo.stDVBSServ.usOrigNetId,
			ChannelHandle,
			(recType==AL_RECTYPE_DVBTV) ? "DTV":((recType==AL_RECTYPE_DVBRADIO) ?"Radio":"Data"));	
			
			len = strlen((char*)tempString);
			memcpy((char*)tempString+len, stServInfo.stDVBSServ.szProgName, sizeof(stServInfo.stDVBSServ.szProgName));
			strcat(tempString,"\n");
			CSV_DBS_DEBF("%s\n",tempString);//test

		Data_Len = strlen(tempString);
		if (Data_Len == 0)
		{
			CSV_DBS_DEBFERR("[error]%s() %d, Source Data_Len = 0\n", __FUNCTION__, __LINE__);
			return FALSE;
		}

		if (write(DBCSVFD, tempString, Data_Len) < (int)Data_Len)
		{
			CSV_DBS_DEBFERR("[error]%s() %d\n", __FUNCTION__, __LINE__);
			return FALSE;
		}
	}
	return TRUE;	
}

/* Function: check db type
 * Param: 
 *		TRUE, Current type is DB-S
 * 
 */
Boolean APP_Factory_PCSortCheckDBType(void)
{
	if (APP_DVB_Playback_GetCurrentNetType() == AL_DBTYPE_DVB_S)
	{
		return TRUE;
	}
	
	CSV_DBS_DEBFERR("[error: please change to DB-S]%s() %d\n", __FUNCTION__, __LINE__);
	return FALSE;
}

/* Function: Open file.CSV
 * Param: 
 *		TRUE,Remove old file.CSV
 *      FALSE, just open
 * Return: 
 *		TRUE,OK.
 *		FALSE:Failed.
 */
Boolean APP_Factory_PCSortOpenCSV(Boolean IsReMove)
{
    int open_config;
	int bPartitionCnt;
	int validdevice=0;
	int FileHander=0;
	char u8UsbName[15];
	
	if (DBCSVNAME != NULL)
	{
		free(DBCSVNAME);
		DBCSVNAME = NULL;
	}

	DBCSVNAME = malloc(sizeof(char)*DBS_FILE_LEN);
	if (DBCSVNAME == NULL)
	{
		CSV_DBS_DEBFERR("[error]%d, can not find usb device!\n", __LINE__);
		return FALSE;
	}

	bPartitionCnt = MID_PartitionList_GetMountedCount();
	if(bPartitionCnt <= 0)
	{
		CSV_DBS_DEBFERR("[error]%d, can not find usb device!\n", __LINE__);
		return FALSE;
	}

	for (validdevice = 0; validdevice < bPartitionCnt; validdevice ++)
	{
		memset(u8UsbName, 0, sizeof(char)*15);
		MID_PartitionList_GetMountName(validdevice, u8UsbName);
		FileHander = open(u8UsbName, O_RDONLY);
		if(FileHander >= 0)
		{
			close(FileHander);
			CSV_DBS_DEBF("[debug]%d, find USB device [%s]\n", __LINE__,u8UsbName);
			break;
		}
		close(FileHander);
	}
	memset(DBCSVNAME, 0, sizeof(char)*DBS_FILE_LEN);
	sprintf(DBCSVNAME, "%s/%s/%s",u8UsbName, STORAGE_FOLDER_NAME, DBS_FILE_NAME);

	if(SC_SUCCESS == APP_Clone_Check_USBFile_IsExsit(DBCSVNAME) && IsReMove == TRUE)
	{
		CSV_DBS_DEBF("[debug remove]%s	%d\n",__FUNCTION__,__LINE__);	
		remove(DBCSVNAME);
	}

    open_config = O_RDWR | O_CREAT;
  	DBCSVFD = open(DBCSVNAME, open_config);
    if (DBCSVFD < 0)
    {
        CSV_DBS_DEBFERR("[error]%s() %d, call open Fail\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

	return TRUE;
}

/* Function: close file.CSV
 * Return: 
 *		TRUE,OK.
 *		FALSE:Failed.
 */
Boolean APP_Factory_PCSortCloseCSV(void)
{

	sync();
	
	if (DBCSVNAME != NULL)
	{
		free(DBCSVNAME);
		DBCSVNAME = NULL;
	}

	if (DBCSVFD < 0)
	{
		return FALSE;
	}
	close(DBCSVFD);
	DBCSVFD = -1;

	return TRUE;
}

/*PC DB-Sort end*/

