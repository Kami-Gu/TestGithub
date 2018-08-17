/*****************************************************************************
** File: app_guiobj_fm_factory.c:
**
** Description:
**
** Copyright(c) 2013 S2-Tek - All Rights Reserved
**
** Author : Gordon.Hsu
**
** $Id: app_guiobj_fm_factory.c 1069 2010-11-16 10:32:32Z b.yang_c1 $
*****************************************************************************/
#include "gobj_datastruct.h"
#include "app_event.h"
#include "main_app.h"
#include "app_msg_filter.h"
#include "sysapp_if.h"
#include "sysapp_table.h"
#include "app_gui.h"
#include "app_audio.h"
#include "app_video.h"
#include "app_guiobj_cul_fm_factorySetting_new.h"
#include "app_guiobj_fm_Utility.h"
#include "app_guiobj_mainmenu.h"
#include "board_config.h"
#include "app_data_setting.h"
#include "app_clone_data.h"
#include "app_guiobj_source.h"
#include "app_guiobj_channel.h"
#include "mid_dtv_display.h"
#include "mid_tvfe.h"
#include "app_factory_flash_access.h"
#include "app_area_info.h"
#include "app_power_control.h"
#include "app_guiobj_language.h"
#include "app_sysset.h"
#include "app_global.h"
#include "../../../aps/include/middleware/ioctl_interface/audio_ioctl.h"
#include "../../../aps/include/middleware/keyupdate/keyupdate.h"
#include "../../../aps/include/middleware/ioctl_interface/umf_ioctl.h"
#include "../../../aps/include/middleware/ioctl_interface/tuner_demod_ioctl.h"
#include "../../../aps/middleware/common/database/include/db_inner.h"
#include "mid_dtvci.h"

#ifdef CONFIG_ATV_SUPPORT
#include "app_atv_playback.h"
#include "app_atv_event.h"
#include "atv_guiobj_table.h"
#endif
#if defined(CONFIG_DVB_SYSTEM) || defined(CONFIG_AUS_DVB_SYSTEM)
#include "dvb_app.h"
#include "app_dvb_playback.h"
#include "app_dvb_event.h"
#include "dvb_guiobj_table.h"
#include "app_factory.h"
#endif
#ifdef CONFIG_ISDB_SYSTEM
#include "app_sbtvd_playback.h"
#include "app_sbtvd_event.h"
#include "app_guiobj_sbtvd_table.h"
#endif
#ifdef CONFIG_MEDIA_SUPPORT
#include "media_app.h"
#include "app_fileplayer_event.h"
#endif
#include "mid_upgrade.h"
#include "app_dvb_mheg5.h"
#include "mid_partition_list.h"
#include "mid_recorder.h"
#include "app_usb_upgrade.h"
#include "app_dvb_prog_manager.h"
#include "app_guiobj_dtv_pvr_rec.h"
#include "subcustomer_setting.h"
#include "drv_kmf_interface.h"
#include "app_factory_data_new.h"
#ifdef SUPPORT_LED_FLASH
#include "app_led_control.h"
#endif
#include "hdmi_ioctl.h"
#ifdef CELLO_BATHROOMTV
#include "main_app_external.h"
extern BOOLEAN g_bRealStandby;
#endif
#ifdef CONFIG_CTV_UART_FAC_MODE
#include "al_console_CtvRs232.h"
#endif

//#define APP_GUIOBJ_FM_FACTORYSETTING_DEBUG
#ifdef APP_GUIOBJ_FM_FACTORYSETTING_DEBUG
#undef    DEBF
#define	DEBF(fmt, arg...)	UMFDBG(0,fmt, ##arg)
#else
#define	DEBF(fmt, arg...)
#endif
#ifdef SUPPORT_HKC_FACTORY_REMOTE
static bool FAC_REMOTE_Flag = FALSE;
#endif

static INT32 _APP_GUIOBJ_FM_FactorySetting_OnCreate(void** pPrivateData , UINT32 dParameter);
static INT32 _APP_GUIOBJ_FM_FactorySetting_OnRelease(void* pPrivateData);
static INT32 _APP_GUIOBJ_FM_FactorySetting_OnFocused(void* pPrivateData);
static INT32 _APP_GUIOBJ_FM_FactorySetting_OnLoseFocus(void* pPrivateData);
static INT32 _APP_GUIOBJ_FM_FactorySetting_OnTimerUpdate(
	void* pPrivateData, InteractiveData_t *pPostEventData);
static INT32 _APP_GUIOBJ_FM_FactorySetting_OnEvent(
	UINT32 dEventID, UINT32 dParam, void* pPrivateData,
	InteractiveData_t *pPostEventData);
static INT32 _APP_GUIOBJ_FM_FactorySetting_UpdateMenu(void);
static UINT32 _FM_inputsource_mapping2currSource(APP_Source_Type_t eSourceType);
static GUIResult_e _APP_Update_Layer(int NodeId);
static UINT32 _FM_OSDLanguage_mapping2currLanguage(APP_OSDLanguage_t eLanguageType);
BOOL IsOverScanAvalible(void);
extern INT32 OSD_SetDisplay(BOOL ShowOSD);
extern INT32 Subtitle_SetDisplay(BOOL bEnable);
extern bool g_bNeedReopenCurveSetting;

const GUI_Object_Definition_t stAPPGuiObjFmFactorySetting=
{
	GUI_OBJ_CAN_BE_FOCUSED,
	GUI_OBJ_UPDATE_PERIOD,
	_APP_GUIOBJ_FM_FactorySetting_OnCreate,
	_APP_GUIOBJ_FM_FactorySetting_OnRelease,
	_APP_GUIOBJ_FM_FactorySetting_OnFocused,
	_APP_GUIOBJ_FM_FactorySetting_OnLoseFocus,
	_APP_GUIOBJ_FM_FactorySetting_OnTimerUpdate,
	_APP_GUIOBJ_FM_FactorySetting_OnEvent,
};

UINT8 bCURVEType = 0;
UINT16 bCOLORLUTregion = 0;
extern KMFShareData_t *g_pKMFShareData;
static RegionHandle_t g_dRegionHandle;
//static UINT32 g_u32Timer = 0;
static HWND g_fmFactorySetting_data;
static ADCCalibrate_OSDGainOffset_t stAdcWhiteBalance;
UINT8 BurnInModeTaskExistFlag = 0;
GL_Task_t FM_BurninMode;
static bool g_bOpenVersionDir = FALSE;
static String_id_t Source_String[2] = {STRING_LAST, STRING_LAST};

//Gordon 20131120
static  char ***pppStrList=NULL;
static int Current_Node_Count = 0;

//Added for convenience
#define FM_ITEMS_PER_PAGE ( TV_E_IDC_TEXT_TextBox_14 - TV_E_IDC_TEXT_TextBox_01 + 1)
static const UINT8 nItemTitle	= TV_E_IDC_TEXT_FM_Title_Setting_Factory_Setting;
static const UINT8 nItemList	= TV_E_IDC_LIST_FM_Setting_Items;
static const UINT8 nItemStart	= TV_E_IDC_TEXT_TextBox_01;
static const UINT8 nItemEnd		= TV_E_IDC_TEXT_TextBox_14;

static WinControl_t *pWindowControl	= TV_IDM_CUL_FM_Factory_Setting_new_control;
static WinControl_t *pWindow		= &TV_IDM_CUL_FM_Factory_Setting_new_window;

#ifdef CONFIG_SUPPORT_PVR //20131206 Special casr for factory record!
static BOOLEAN b_Isrecording=FALSE;
#endif
static UINT32 u_CurrentAppIndex = 0;
static bool g_bFlipIsChange = FALSE;
static UINT8 u8FlipIndex = 0;

static UINT8 u8AudioOutOffset =0 ;

//static UINT8 u8BackLight = 100;
static UINT8 u8BackLight_Polarity = 0;
static UINT16 u16DutyPWM = 5;
#ifdef CONFIG_SUPPORT_PWM_DUTY_IN_DECIMAL
static UINT8 u8mAGrade = 0;
#else
#if SUPPORT_ADJUST_PWM_400mA
	static UINT8 u8mAGrade = 0;
#elif SUPPORT_ADJUST_PWM_700mA
	static UINT8 u8mAGrade = 1;
#elif SUPPORT_ADJUST_PWM_900mA
	static UINT8 u8mAGrade = 2;
#else
	static UINT8 u8mAGrade = 0;
#endif
#endif
//[CC] Added for Number input
static BOOLEAN b_IsInputNum =FALSE;
static double nInputValue = -1;

static UINT8 **StringList_of_LeftSide = NULL;
static UINT32  nowNodeHeadId = ID_ROOT;
static UINT32  nowNodeId = ID_ROOT;
UINT32  u32NodeHeadId = ID_FM_PicMode_CurveSetting;
static UINT32 string_last = STRING_LAST;
/*
static UINT32 String[] ={
	TV_IDS_String_NULL, TV_IDS_String_NULL, TV_IDS_String_NULL, TV_IDS_String_NULL,
	TV_IDS_String_NULL, TV_IDS_String_NULL, TV_IDS_String_NULL, TV_IDS_String_NULL,
	TV_IDS_String_NULL, TV_IDS_String_NULL, TV_IDS_String_NULL, TV_IDS_String_NULL,
	TV_IDS_String_NULL, TV_IDS_String_NULL,STRING_LAST};
*/
static UINT32 StringRightArrow[2] ={TV_IDS_String_row, STRING_LAST};

static UINT32 StringRight[FM_ITEMS_PER_PAGE][2];

static UINT32 String_null[2]={TV_IDS_String_NULL, STRING_LAST};
static UINT32 pEmptyStrings[FM_ITEMS_PER_PAGE+1];

#define ADCADJUSTGAINOFFSET_MAX	(511)
#define ADCADJUSTGAINOFFSET_MIN	(0)
static VIP_TestPattern VideoPatternMappingTbl[] =
{
	{ VIP_TEST_PATTERN_DISABLE, 0, 0, 0 }, //Disable test pattern
	{ VIP_TEST_PATTERN_FILL_COLOR, 0xFF, 0xFF, 0xFF }, //White pattern
	{ VIP_TEST_PATTERN_FILL_COLOR, 0x00, 0x00, 0x00 }, //Black pattern
	{ VIP_TEST_PATTERN_FILL_COLOR, 0xFF, 0x00, 0x00 }, //Red pattern
	{ VIP_TEST_PATTERN_FILL_COLOR, 0x00, 0xFF, 0x00 }, //Green pattern
	{ VIP_TEST_PATTERN_FILL_COLOR, 0x00, 0x00, 0xFF }, //Blue pattern
};
typedef enum {
	FM_Scale_Mode = 0,
	FM_VTop_Overscan,
	FM_VBottom_Overscan,
	FM_HLeft_Overscan,
	FM_HRight_Overscan,
	FM_Scale_Max,
} fm_OverScan_List_t;

typedef struct {
	APP_Video_AspectRatioType_e eScalerMode;
	UINT16 u16VTOverscan;
	UINT16 u16VBOverscan;
	UINT16 u16HLOverscan;
	UINT16 u16HROverscan;
	UINT16 u16HStart;
	UINT16 u16HEnd;
	UINT16 u16VStart;
	UINT16 u16VEnd;
} APP_FM_OverScan_Setting_t;

typedef struct  
{
	int n_Audio_AVL_Vol_VolumePoint0;
	int n_Audio_AVL_Vol_VolumePoint10;
	int n_Audio_AVL_Vol_VolumePoint20;
	int n_Audio_AVL_Vol_VolumePoint30;
	int n_Audio_AVL_Vol_VolumePoint40;
	int n_Audio_AVL_Vol_VolumePoint50;
	int n_Audio_AVL_Vol_VolumePoint60;
	int n_Audio_AVL_Vol_VolumePoint70;
	int n_Audio_AVL_Vol_VolumePoint80;
	int n_Audio_AVL_Vol_VolumePoint90;
	int n_Audio_AVL_Vol_VolumePoint100;
}FMVolume_t;


APP_FM_OverScan_Setting_t OverScanSettingValue;
static Boolean bOverscanEn = FALSE;
static UINT8 u8aspect_index;
static UINT8 gbIsFMRightKey = FALSE;
static UINT8 gbIsFMLeftKey = FALSE;

static FM_RESETALL_STATUS g_bResetAllFlag = FM_RESETALL_STOP;
static FM_SHIPPING_STATUS g_bShippingFlag = FM_SHIPPING_STOP;

static APP_FM_OsdLang_Page1_t stLangOnOffvalue1;
static APP_FM_OsdLang_Page2_t stLangOnOffvalue2;

UINT8 gIsFactoryResetting = FALSE;

int OSD_NonLine_Value[5] = {0,25,50,75,100};
int OSD_HUE_NonLine_Value[5] = {-50, -25, 0, 25, 50}; //Special range of HUE osd index array.
int CustomerDefine_Nonline_Value[5] = {0};

	//For SounMode_Vol_VolumePoint
static FMVolume_t g_efmPreVolumeValue;

//SounMode_Vol_VolumePoint End
static void Initialize_SounMode_Vol_VolumePoint()
{
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();
	
	g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint0 = g_stSysInfoData.szAudioVolumeTab[sourceindex][0];
	g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint10 = g_stSysInfoData.szAudioVolumeTab[sourceindex][10];
	g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint20 = g_stSysInfoData.szAudioVolumeTab[sourceindex][20];
	g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint30 = g_stSysInfoData.szAudioVolumeTab[sourceindex][30];
	g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint40 = g_stSysInfoData.szAudioVolumeTab[sourceindex][40];
	g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint50 = g_stSysInfoData.szAudioVolumeTab[sourceindex][50];
	g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint60 = g_stSysInfoData.szAudioVolumeTab[sourceindex][60];
	g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint70 = g_stSysInfoData.szAudioVolumeTab[sourceindex][70];
	g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint80 = g_stSysInfoData.szAudioVolumeTab[sourceindex][80];
	g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint90 = g_stSysInfoData.szAudioVolumeTab[sourceindex][90];
	g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint100 = g_stSysInfoData.szAudioVolumeTab[sourceindex][100];

}

static void Update_TypeVersion_Node(char *text_content,UINT32 u32CurrentIndex)
{
	HWND FocusItem_Handle;
	GUI_FUNC_CALL(GEL_GetHandle(pWindowControl, u32CurrentIndex+nItemStart, &FocusItem_Handle));
	sprintf(pppStrList[u32CurrentIndex][0], "%s", text_content);
	GUI_FUNC_CALL( GEL_SetParam(FocusItem_Handle, PARAM_DYNAMIC_STRING, pppStrList[u32CurrentIndex][0]));
	GUI_FUNC_CALL(GEL_SendMsg(FocusItem_Handle, WM_PAINT, 0));
}

static INT32 Factory_OSDLanguage_ISUpdate(void)
{
	al_uint32 Current_OSD_Language = 0;

	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FEATURE, 0,
					sizeof(APP_SETTING_Feature_t), &(g_stFeatureData));
	Current_OSD_Language = g_stFeatureData.OSDLang;

	switch(nowNodeId)
	{
		case ID_FM_SysConf_OSD_German:
			if(Current_OSD_Language == APP_OSDLANG_GERMAN) return FALSE;
			break;
		case ID_FM_SysConf_OSD_English:
			if(Current_OSD_Language == APP_OSDLANG_ENGLISH) return FALSE;
			break;
		case ID_FM_SysConf_OSD_French:
			if(Current_OSD_Language == APP_OSDLANG_FRENCH) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Italian:
			if(Current_OSD_Language == APP_OSDLANG_ITALIAN) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Polish:
			if(Current_OSD_Language == APP_OSDLANG_POLISH) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Spanish:
			if(Current_OSD_Language == APP_OSDLANG_SPAIN) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Netherlands:
			if(Current_OSD_Language == APP_OSDLANG_NEDERLANDS) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Portuguese:
			if(Current_OSD_Language == APP_OSDLANG_NEDERLANDS) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Swedish:
			if(Current_OSD_Language == APP_OSDLANG_SWEDISH) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Finnish:
			if(Current_OSD_Language == APP_OSDLANG_FRENCH) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Greek:
			if(Current_OSD_Language == APP_OSDLANG_GREEK) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Russian:
			if(Current_OSD_Language == APP_OSDLANG_RUSSIA) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Turkey:
			if(Current_OSD_Language == APP_OSDLANG_TURKISH) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Danish:
			if(Current_OSD_Language == APP_OSDLANG_DANISH) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Norwegian:
			if(Current_OSD_Language == APP_OSDLANG_NORWEGIAN) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Hungarian:
			if(Current_OSD_Language == APP_OSDLANG_HUNGARIAN) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Czech:
			if(Current_OSD_Language == APP_OSDLANG_CZECH) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Slovakian:
			if(Current_OSD_Language == APP_OSDLANG_SLOVAKIAN) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Croatian:
			if(Current_OSD_Language == APP_OSDLANG_CROATIAN) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Serbian:
			if(Current_OSD_Language == APP_OSDLANG_SERBIAN) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Arabic:
			if(Current_OSD_Language == APP_OSDLANG_ARABIC) return FALSE;
			break;
		case ID_FM_SysConf_OSD_Persian:
			if(Current_OSD_Language == APP_OSDLANG_PERSIAN) return FALSE;
			break;
		default:
			break;
	}
	return TRUE;
}

static GUIResult_e Enter_Behavior(void)
{
	GUIResult_e dRet = GUI_SUCCESS ;
	UINT32 u32CurrentIndex = 0;
	UINT32 NodeId = ID_ROOT;
	APP_Source_Type_t eSourceType = APP_SOURCE_MAX;
	APP_OSDLanguage_t eLanguageType = APP_OSDLANG_MAX;
	
	APP_GUIOBJ_Source_GetCurrSource(&eSourceType);
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FEATURE, 0,
					sizeof(APP_SETTING_Feature_t), &(g_stFeatureData));
	eLanguageType = g_stFeatureData.OSDLang;

	dRet = GEL_GetParam(g_fmFactorySetting_data, PARAM_CURRENT_INDEX,&u32CurrentIndex);
	//printf("[Gordon] %d nowNodeHeadId=%d \n",__LINE__,nowNodeHeadId);

	UINT32 Ori_Id = nowNodeHeadId;
	if(FactoryNode[nowNodeHeadId].childID==0)
	{
		//printf("[Gordon] %d nowNodeHeadId=%d \n",__LINE__,nowNodeHeadId);
		return dRet;
	}

	UINT32 i;
	nowNodeHeadId = FactoryNode[nowNodeHeadId].childID;
	//printf("[Gordon] %d nowNodeHeadId=%d \n",__LINE__,nowNodeHeadId);
	for(i=0;i<u32CurrentIndex;i++)
	{
		//printf("[Gordon] %d nowNodeHeadId=%d i=%d \n",__LINE__,nowNodeHeadId,i);
		nowNodeHeadId = FactoryNode[nowNodeHeadId].nextID;
	}
	if(FactoryNode[nowNodeHeadId].childID==0)
	{
		nowNodeHeadId = Ori_Id;
		return dRet;
	}

	dRet = _APP_Update_Layer(nowNodeHeadId);

	NodeId = FactoryNode[Ori_Id].childID + u32CurrentIndex;

	u32CurrentIndex = 0;

	if (NodeId == ID_FM_SysConf_Input_NextPage)
	{
		NodeId = FactoryNode[nowNodeHeadId].childID + u32CurrentIndex;
		if (_FM_inputsource_mapping2currSource(eSourceType) == NodeId)
		{
			u32CurrentIndex = 1;
		}
	}
	
	if (NodeId == ID_FM_SysConf_OSD_Nextpage)
	{
		NodeId = FactoryNode[nowNodeHeadId].childID + u32CurrentIndex;
		if (_FM_OSDLanguage_mapping2currLanguage(eLanguageType) == NodeId)
		{
			u32CurrentIndex = 1;
		}
	}
	
	GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_CURRENT_INDEX,&u32CurrentIndex));
	return dRet;
}

static GUIResult_e UpdateNodeFunctionContent(int NodeId,UINT32 u32CurrentIndex,UINT32 dEventID)
{
	GUIResult_e dRet = GUI_SUCCESS ;
	NODE_FUNCTION * pFunc;
	FUNCTION_DATA FuncData;
	HWND FocusItem_Handle;
	dRet = GEL_GetHandle(pWindowControl, u32CurrentIndex+nItemStart, &FocusItem_Handle);
	UINT32 tmp=u32CurrentIndex;

	//printf("[Gordon]%s %d  Current_Node_Count=%d \n",__func__,__LINE__,Current_Node_Count);

	gbIsFMRightKey = (UI_EVENT_RIGHT == dEventID) ? TRUE : FALSE;
	gbIsFMLeftKey = (UI_EVENT_LEFT == dEventID) ? TRUE : FALSE;
	for(u32CurrentIndex=0; (int)u32CurrentIndex< Current_Node_Count; u32CurrentIndex++)
	{
		//printf("[Gordon]%s %d  u32CurrentIndex=%d \n",__func__,__LINE__,u32CurrentIndex);
		if(dEventID!=0)
		u32CurrentIndex = tmp;
		dRet = GEL_GetHandle(pWindowControl, u32CurrentIndex+nItemStart, &FocusItem_Handle);

		UINT32 nCurrNodeId = NodeId+ u32CurrentIndex;//nowNodeHeadId + u32CurrentIndex ;
		nowNodeId = nCurrNodeId;
		pFunc = FactoryNode[nCurrNodeId].pThis;

		if(pFunc != NULL)
		{
			FuncData.nItem = nCurrNodeId;
			FuncData.value = 0;//TV_IDS_String_PresetChannel;

			if(pFunc->type  == MENU_CTRL_TYPE_SLIDER)
			{
				//printf("[Gordon] %d \n",__LINE__);
				//Step1: Update value
				// Do nothing
				if (pFunc->MenuSettingFun)
				{
					// Get Value from UMF
					(pFunc->MenuSettingFun)(FALSE,/* MAIN*/ 0, (FUNCTION_DATA*)&FuncData, FALSE);
				}
				// Step2:  Convert "leave"
				MENU_SLIDER *pChoice = (MENU_SLIDER *)pFunc->leaves;

				if(nowNodeId == ID_FM_Audio_SounMode_Vol_VolumePointStep)
				{
					if(UI_EVENT_LEFT == dEventID || UI_EVENT_RIGHT == dEventID)
					{
						if(FuncData.value == 1)
						{
							FuncData.value = 0;
						}
						else if (FuncData.value == 10 && UI_EVENT_LEFT == dEventID)
						{
							FuncData.value = 11;
						}
					}
				}
				else if(nowNodeId >= ID_FM_Audio_SounMode_Vol_VolumePoint0
					&& nowNodeId <= ID_FM_Audio_SounMode_Vol_VolumePoint100)
				{
					AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
						sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData));
					if(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePointStep != 0)
					{
						pChoice->step = g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePointStep;
					}
					/*zhangjianwei added for mantis 0022931 @20140625 */
					else
					{
						pChoice->step = 1;
					}					
				}

				if(UI_EVENT_RIGHT == dEventID)
				{
					FuncData.value += pChoice->step;
				}
				else if(UI_EVENT_LEFT== dEventID)
				{
					FuncData.value -= pChoice->step;
				}
				else if( nInputValue != -1 /*Invalid*/ )
				{
					FuncData.value = nInputValue;
				}
				
				if(nowNodeId == ID_FM_PicMode_OverScan_VTopOverScan)
				{
					AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
						sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData));

					pChoice->nMax = 949 - g_stFactoryUserData.PictureMode.OverScan.n_PicMode_OverScan_VBottomOverScan;
				}
				else if(nowNodeId == ID_FM_PicMode_OverScan_VBottomOverScan)
				{
					AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
						sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData));

					pChoice->nMax = 949 - g_stFactoryUserData.PictureMode.OverScan.n_PicMode_OverScan_VTopOverScan;
				}
				else if(nowNodeId == ID_FM_PicMode_OverScan_HLeftOverScan)
				{
					AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
						sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData));

					pChoice->nMax = 949 - g_stFactoryUserData.PictureMode.OverScan.n_PicMode_OverScan_HRightOverScan;
				}
				else if(nowNodeId == ID_FM_PicMode_OverScan_HRightOverScan)
				{
					AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
						sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData));

					pChoice->nMax = 949 - g_stFactoryUserData.PictureMode.OverScan.n_PicMode_OverScan_HLeftOverScan;
				}

				if(FuncData.value < pChoice->nMin)
				{
					FuncData.value = pChoice->nMax ;
				}

				if(FuncData.value > pChoice->nMax)
				{
					FuncData.value = pChoice->nMin;

					if(b_IsInputNum)
					{
						b_IsInputNum = FALSE;
						nInputValue = -1;
					}
				}
				/*if(nowNodeId == ID_FM_PicMode_CoLUT_SatMin)
				{
					if (FuncData.value >= APP_Video_GetColorLUTSetting((LUT_FUN_SAT_MAX),bCOLORLUTregion))
					{
						FuncData.value = APP_Video_GetColorLUTSetting((LUT_FUN_SAT_MAX),bCOLORLUTregion);
					}
				}
				else if(nowNodeId == ID_FM_PicMode_CoLUT_SatMax)
				{
					if (FuncData.value <= APP_Video_GetColorLUTSetting((LUT_FUN_SAT_MIN),bCOLORLUTregion))
					{
						FuncData.value = APP_Video_GetColorLUTSetting((LUT_FUN_SAT_MIN),bCOLORLUTregion);
					}
				}
				else if(nowNodeId == ID_FM_PicMode_CoLUT_YMin)
				{
					if (FuncData.value >= APP_Video_GetColorLUTSetting((LUT_FUN_Y_MAX),bCOLORLUTregion))
					{
						FuncData.value = APP_Video_GetColorLUTSetting((LUT_FUN_Y_MAX),bCOLORLUTregion);
					}
				}
				else if(nowNodeId == ID_FM_PicMode_CoLUT_YMax)
				{
					if (FuncData.value <= APP_Video_GetColorLUTSetting((LUT_FUN_Y_MIN),bCOLORLUTregion))
					{
						FuncData.value = APP_Video_GetColorLUTSetting((LUT_FUN_Y_MIN),bCOLORLUTregion);
					}
				}
*/
				//printf("[Gordon] %d \n",__LINE__);
				// Step3: Get string list
				if(pppStrList[u32CurrentIndex])
				{
					//printf("[Gordon]  pppStrList[u32CurrentIndex][0]=%s line=%d \n ",pppStrList[u32CurrentIndex][0],__LINE__);
					if(pppStrList[u32CurrentIndex][0])
					{
						free(pppStrList[u32CurrentIndex][0]);
					}
					pppStrList[u32CurrentIndex][0] = (char *)malloc( sizeof(char ) * 60);

					//printf("[Gordon]  pppStrList[u32CurrentIndex][0]=%s line=%d \n ",pppStrList[u32CurrentIndex][0],__LINE__);
					//printf("[Gordon] line=%d u32CurrentIndex=%d FuncData.value=%d  \n ",__LINE__,u32CurrentIndex,FuncData.value);

					if(pChoice->step == (UINT32)pChoice->step)
					{
						sprintf(pppStrList[u32CurrentIndex][0], "%d", (INT32)FuncData.value);
					}
					else
					{
						sprintf(pppStrList[u32CurrentIndex][0], "%.1f", FuncData.value);
					}
					//printf("[Gordon]  pppStrList[u32CurrentIndex][0]=%s line=%d \n ",pppStrList[u32CurrentIndex][0],__LINE__);
					dRet = GEL_SetParam(FocusItem_Handle,PARAM_DYNAMIC_STRING, pppStrList[u32CurrentIndex][0]);

					//dRet = GEL_SendMsg(FocusItem_Handle, WM_PAINT, 0);
								//printf("[Gordon] %d \n",__LINE__);
				}

				//Step4: Set UMF function
				if((UI_EVENT_LEFT== dEventID) || (UI_EVENT_RIGHT== dEventID) || (UI_EVENT_ENTER== dEventID))
				{
					(pFunc->MenuSettingFun)(TRUE, 0, (FUNCTION_DATA*)&FuncData, FALSE);
				}
				//printf("[Gordon] %d \n",__LINE__);

			}
			else if(pFunc->type  == MENU_CTRL_TYPE_RADIOBUTTON)
			{

				//Step1: Update value
				// Do nothing

				// Step2: Convert "leave"
				UINT32 j;
				FACT_RADIOBTN *pRad;

				if (pFunc->leaves)
				{
					FuncData.nItem = 0;
					FuncData.value = 0;
					if (pFunc->MenuSettingFun)
					{
						// Get Value from UMF
						(pFunc->MenuSettingFun)(FALSE,/* MAIN*/ 0, (FUNCTION_DATA*)&FuncData, FALSE);
					}
					pRad = (FACT_RADIOBTN *)pFunc->leaves;
					//printf("[Gordon]pFunc-> FuncData.value=%d  line=%d \n ", FuncData.value,__LINE__);
					for (j=0; j<pFunc->nItems; j++)
					{
						if (pRad[j].value == FuncData.value)
							break;
					}

					//	printf("[Gordon]pFunc->nItems=%d j=%d  line=%d \n ",pFunc->nItems,j,__LINE__);
					if(UI_EVENT_RIGHT == dEventID)
					{
						if(j==pFunc->nItems-1)
						{
							j=0;
						}
						else
						{
							j++;
						}
					}
					else if(UI_EVENT_LEFT== dEventID)
					{
						if(j==0)
						j=pFunc->nItems-1;
						else
						j--;
					}
					//printf("[Gordon]pFunc->nItems=%d j=%d  line=%d \n ",pFunc->nItems,j,__LINE__);
					/*
					if (j == pFunc->nItems || j+1 > pFunc->nItems-1)
					j = 0;
					else if(j<0)
					j = pFunc->nItems-1; 		*/
					//else
					//	j += 1;
					//printf("[Gordon]FuncData.value=%d  pRad[j].value=%d j=%d  line=%d \n ",FuncData.value,pRad[j].value,j,__LINE__);

					if (FuncData.value != pRad[j].value&& dEventID!=0)
					{

						FuncData.value = pRad[j].value;
						if (pFunc->MenuSettingFun)
						{
							(*pFunc->MenuSettingFun)(TRUE, 0, (FUNCTION_DATA*)&FuncData, FALSE);
						}


					}
					sprintf(pppStrList[u32CurrentIndex][0], "%s", (char *)pRad[j].str);
					//printf("[Gordon] line=%d     tmp_strId= %d \n",__LINE__,tmp_strId[0]);
					dRet = GEL_SetParam(FocusItem_Handle, PARAM_DYNAMIC_STRING, pppStrList[u32CurrentIndex][0]);
					//dRet = GEL_SendMsg(FocusItem_Handle, WM_PAINT, 0);
				}
				// Step3: Get string list
				//Step4: Set UMF function
				// do nothing
			}
			else if(pFunc->type  == MENU_CTRL_TYPE_PUSHBUTTON )
			{
				if (pFunc->MenuSettingFun)
				{
					if( dEventID==UI_EVENT_RIGHT|| dEventID==UI_EVENT_ENTER )
					(*pFunc->MenuSettingFun)(TRUE, 0, &FuncData, FALSE);
					else
					(*pFunc->MenuSettingFun)(FALSE, 0, &FuncData, FALSE);
				}

				FACT_PUSHBTN *pPushBtm = (FACT_PUSHBTN *)pFunc->leaves;

				// Draw Text
				if(pppStrList[u32CurrentIndex][0])
				{
					free(pppStrList[u32CurrentIndex][0]);
				}
				pppStrList[u32CurrentIndex][0] = (char *)malloc( sizeof(char ) * 60);
				sprintf(pppStrList[u32CurrentIndex][0], "%s ", (char *)(pPushBtm[ (INT32)FuncData.value ].str));
				dRet = GEL_SetParam(FocusItem_Handle, PARAM_DYNAMIC_STRING, pppStrList[u32CurrentIndex][0]);
				//dRet = GEL_SendMsg(FocusItem_Handle, WM_PAINT, 0);
				//GUI_FUNC_CALL(GEL_UpdateOSD());
			}
			else if( MENU_CTRL_TYPE_VERSION == pFunc->type )
			{
				FuncData.nItem = u32CurrentIndex;
				if (pFunc->MenuSettingFun)
				{
					if( dEventID==UI_EVENT_RIGHT|| dEventID==UI_EVENT_ENTER)
						(pFunc->MenuSettingFun)(TRUE,/* MAIN*/ 0, &FuncData, FALSE); // Set Value from UMF
					else
						(pFunc->MenuSettingFun)(FALSE,/* MAIN*/ 0, &FuncData, FALSE); // Get Value from UMF
				}
			}
			else if(MENU_CTRL_TYPE_GROUPSTATICSTRINGS == pFunc->type)
			{
				if (pFunc->MenuSettingFun)
				{
					(pFunc->MenuSettingFun)(FALSE,/* MAIN*/ 0, &FuncData, FALSE); // Get Value from UMF
				}
				INT32 i32Index = 0;
				MENU_SLIDER_STATICSTRS *pChoice = (MENU_SLIDER_STATICSTRS *)pFunc->leaves;
				if(UI_EVENT_RIGHT == dEventID)
				{
					FuncData.value++;
				}
				else if(UI_EVENT_LEFT== dEventID)
				{
					FuncData.value--;
				}

				if(FuncData.value < pChoice->nMin)
				{
					FuncData.value = pChoice->nMax ;
				}

				if(FuncData.value > pChoice->nMax)
				{
					FuncData.value = pChoice->nMin;
				}
				i32Index = FuncData.value;
				// Draw Text
				StringRight[u32CurrentIndex][0] = pChoice->str[i32Index];
				StringRight[u32CurrentIndex][1] = STRING_LAST;
				GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle,PARAM_STATIC_STRING, &StringRight[u32CurrentIndex]));
				//GUI_FUNC_CALL(GEL_SendMsg(FocusItem_Handle, WM_PAINT, 0));
				//GUI_FUNC_CALL(GEL_UpdateOSD());
				if (pFunc->MenuSettingFun)
				{
					if((dEventID == UI_EVENT_RIGHT) || (dEventID == UI_EVENT_LEFT))
					{
						(pFunc->MenuSettingFun)(TRUE, 0, (FUNCTION_DATA*)&FuncData, FALSE);
					}
				}
			}
			else if(MENU_CTRL_TYPE_DAYNAMICSTRING == pFunc->type)
			{
				FuncData.nItem = u32CurrentIndex;
				if (pFunc->MenuSettingFun)
				{
					if((dEventID == UI_EVENT_RIGHT) || (dEventID == UI_EVENT_LEFT))
					{
						(pFunc->MenuSettingFun)(TRUE, 0, &FuncData, FALSE);
					}
					else
					{
						(pFunc->MenuSettingFun)(FALSE, 0, &FuncData, FALSE);
					}
				}
			}
		}
		else
		{
			//[CC] Added, Leaf Node behavior
			if(UI_EVENT_RIGHT == dEventID)
			{
				dRet = Enter_Behavior();
			}else{
				GUI_FUNC_CALL(GEL_SendMsg(FocusItem_Handle, WM_PAINT, 0));
			}
		}
		//printf("[Gordon]%s %d  \n",__func__,__LINE__);
		if(dEventID!=0)
		break;
	}
//	printf("[Gordon]%s %d  \n",__func__,__LINE__);
	return dRet;
}

void Fee_LeftSide_List(void)
{
	// Free strings
	if(StringList_of_LeftSide)
	{
		UINT8 i;
		for (i=0; i<=FM_ITEMS_PER_PAGE ; i++) // Added 1 for End symbol
		{
			free(StringList_of_LeftSide[i]);
			StringList_of_LeftSide[i] = NULL;
		}
		
		free(StringList_of_LeftSide);
		StringList_of_LeftSide = NULL;
	}
}

static   GUIResult_e _APP_Update_Layer(int NodeId)
{
 	GUIResult_e dRet = GUI_SUCCESS;
	int i=0;
	int ChildId = FactoryNode[NodeId].childID;

	//printf("[ForDBG] NodeId = %d \n", NodeId);

	if(ChildId==0)
		return dRet;

	// update Title
	//printf("[ForDBG] start set title (NodeId = %d and str id = %d)\n", NodeId, FactoryNode[NodeId].strID);
	HWND Title_Handle;
	dRet = GEL_GetHandle(pWindowControl, nItemTitle, &Title_Handle);
	if( NodeId ==  ID_ROOT) // First nade
	{
		//GUI_FUNC_CALL( GEL_SetParam(Title_Handle, PARAM_STATIC_STRING, &(FactoryNode[ID_ROOT].strID) ) );
		GUI_FUNC_CALL( GEL_SetParam(Title_Handle, PARAM_DYNAMIC_STRING, (void *)FactoryNode[ID_ROOT].szItem ) );
	}else{

		/*
		// Static string Ver.
		if( (FactoryNode[NodeId].strID == TV_IDS_String_NextPage) || (FactoryNode[NodeId].strID == TV_IDS_String_PreviousPage) )
		{
			GUI_FUNC_CALL( GEL_SetParam(Title_Handle, PARAM_STATIC_STRING, &( FactoryNode[ FactoryNode[NodeId].parentID].strID) ) );
		}else{
			GUI_FUNC_CALL( GEL_SetParam(Title_Handle, PARAM_STATIC_STRING, &(FactoryNode[NodeId].strID) ) );
		}
		*/

		//Dynamic String Ver.
		if( 
			( strcmp((void *)STRING_NEXT_PAGE, (void *)FactoryNode[NodeId].szItem) == 0 ) || 
			( strcmp((void *)STRING_PRE_PAGE, (void *)FactoryNode[NodeId].szItem) == 0 ) )
		{
			GUI_FUNC_CALL( GEL_SetParam(Title_Handle, PARAM_DYNAMIC_STRING, (void *)FactoryNode[ FactoryNode[NodeId].parentID].szItem ) );
		}else{
			GUI_FUNC_CALL( GEL_SetParam(Title_Handle, PARAM_DYNAMIC_STRING, (void *)FactoryNode[NodeId].szItem ) );
		}
	}

	dRet = GEL_SendMsg(Title_Handle, WM_PAINT, 0);
	//printf("[CC DBG] set title end!!! \n");

	// Update List
	int NextId =  FactoryNode[ChildId].nextID;
	int count = 1;
	while( (NextId!=0) && (count<FM_ITEMS_PER_PAGE) )
	{
		count ++;
		NextId = FactoryNode[NextId].nextID;
	}
	//printf("[Gordon]%s %d  \n",__func__,__LINE__);

	// New Ver. (Dynamic string) -------------------------------------------------
	// Free strings
	Fee_LeftSide_List();
	
	// Malloc
	StringList_of_LeftSide = (UINT8 **)malloc( sizeof(UINT8 *) * (FM_ITEMS_PER_PAGE+1) ); // Added 1 for End symbol
	for (i=0; i<=FM_ITEMS_PER_PAGE ; i++)
	{
		StringList_of_LeftSide[i] = (UINT8 *)malloc(sizeof(UINT8) * (MAX_TEXT_LENGTH+1));

		// Set all string to End symbol
		memcpy((void *)StringList_of_LeftSide[i], (void *)&string_last, sizeof(string_last));
	}

	// Clear strings
	GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_STATIC_STRING, pEmptyStrings ) );
	
	// Copy Items' strings
	NextId =  ChildId;
	for(i=0; i<count; i++)
	{
		snprintf((char *)StringList_of_LeftSide[i], MAX_TEXT_LENGTH, "%s", FactoryNode[NextId].szItem);
		//printf("[CC DBG] StringList_of_LeftSide[%02d] = %s\n", i, StringList_of_LeftSide[i] );
		NextId = FactoryNode[NextId].nextID;
	}
	//--------------------------------------------------------------------------
	
	for(i=0; i<FM_ITEMS_PER_PAGE; i++)
	{
		HWND Item_Handle;
		dRet = GEL_GetHandle(pWindowControl, nItemStart+i, &Item_Handle);
		dRet = GEL_SetParam(Item_Handle, PARAM_STATIC_STRING,String_null);
		//GUI_FUNC_CALL( GEL_SendMsg(Item_Handle, WM_PAINT, 0) );
	}

	//dRet = GEL_SetParam(g_fmFactorySetting_data, PARAM_STATIC_STRING, StringList_of_LeftSide);
	GUI_FUNC_CALL( GEL_SetParam(g_fmFactorySetting_data, PARAM_DYNAMIC_STRING, (void *)StringList_of_LeftSide) );
	GUI_FUNC_CALL( GEL_SetParam(g_fmFactorySetting_data, PARAM_SETNORMAL, 0) );
	
	//printf("[%s] nItemEnd=%d,nItemStart=%d count=%d==============\n",__FUNCTION__,nItemEnd,nItemStart,count);

	for(i=0; i<=FM_ITEMS_PER_PAGE; i++)
	{
		//printf("[Gordon]%s %d  i=%d \n",__func__,__LINE__,i);
		HWND FocusItem_Handle;
		dRet = GEL_GetHandle(pWindowControl, nItemStart+i, &FocusItem_Handle);
		if(i<count)
		{
			dRet = GEL_SetParam(FocusItem_Handle, PARAM_STATIC_STRING, StringRightArrow);
		}
		else
		{
			dRet = GEL_SetParam(FocusItem_Handle, PARAM_STATIC_STRING,String_null);
		}

		//[CC] Added
		GUI_FUNC_CALL( GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL) );
		if (i<count)
		GUI_FUNC_CALL( GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &i) );

	}
	//printf("[Gordon] NextId=%d line=%d  \n",NextId,__LINE__);

	Current_Node_Count = count;
	//printf("[Gordon]%s %d  Current_Node_Count=%d  \n",__func__,__LINE__,Current_Node_Count);
	UpdateNodeFunctionContent(ChildId,0,0);
	
	//GUI_FUNC_CALL( GEL_ShowMenu(pWindow) );
	//GUI_FUNC_CALL(GEL_UpdateOSD());
	//printf("[Gordon]%s %d  \n",__func__,__LINE__);
	return dRet;
}

static INT32 _APP_GUIOBJ_FM_FactorySetting_UpdateMenu(void)
{
	GUIResult_e dRet = GUI_SUCCESS;
	INT32 i32Index = 0;
	UINT32 NodeId = ID_ROOT;
	HWND hItem;
	HWND hFocusItem;
	State_e nState = NORMAL_STATE;
	APP_Source_Type_t eSourceType = APP_SOURCE_MAX;
	APP_OSDLanguage_t eLanguageType = APP_OSDLANG_MAX;

	APP_GUIOBJ_Source_GetCurrSource(&eSourceType);
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FEATURE, 0,
					sizeof(APP_SETTING_Feature_t), &(g_stFeatureData));
	eLanguageType = g_stFeatureData.OSDLang;

	for(i32Index=nItemStart; i32Index<=nItemEnd; i32Index++)
	{
		dRet = GEL_GetHandle(pWindowControl,i32Index, &hItem);

		// Get state
		GUI_FUNC_CALL( GEL_GetParam(hItem, PARAM_STATE,&nState) );

		if(nState == FOCUS_STATE)
		{
			GUI_FUNC_CALL(GEL_SetAnimEnable(hItem, FALSE));
			GUI_FUNC_CALL(GEL_SetParam(hItem, PARAM_SETNORMAL, NULL));
		}
	}

	if((nowNodeHeadId == ID_FM_SysConf_InputSourceSystem)
		||(nowNodeHeadId == ID_FM_SysConf_Input_NextPage)
		||(nowNodeHeadId == ID_FM_SysConf_Input_PreviousPage)
		)
	{
		for(i32Index=0;(int)i32Index<Current_Node_Count;i32Index++)
		{
			NodeId = FactoryNode[nowNodeHeadId].childID + i32Index;
			if (_FM_inputsource_mapping2currSource(eSourceType) == NodeId)
			{
				dRet = GEL_SetParam(g_fmFactorySetting_data,PARAM_ITEM_DISABLE, (void*)&i32Index);
				dRet = GEL_GetHandle(pWindowControl, i32Index+nItemStart, &hItem);
				dRet = GEL_SetParam(hItem,PARAM_SETDISABLED, NULL);
			}
			else
			{
				dRet = GEL_SetParam(g_fmFactorySetting_data,PARAM_ITEM_ENABLE, (void*)&i32Index);
				dRet = GEL_GetHandle(pWindowControl, i32Index+nItemStart, &hItem);
				dRet = GEL_SetParam(hItem,PARAM_SETNORMAL, NULL);
			}
		}
	}
	
	if((nowNodeHeadId == ID_FM_SysConf_OSDLanguage)
		||(nowNodeHeadId == ID_FM_SysConf_OSD_Nextpage)
		||(nowNodeHeadId == ID_FM_SysConf_OSD_Previouspage)
		)
	{
		for(i32Index=0;(int)i32Index<Current_Node_Count;i32Index++)
		{
			NodeId = FactoryNode[nowNodeHeadId].childID + i32Index;
			if (_FM_OSDLanguage_mapping2currLanguage(eLanguageType) == NodeId)
			{
				dRet = GEL_SetParam(g_fmFactorySetting_data,PARAM_ITEM_DISABLE, (void*)&i32Index);
				dRet = GEL_GetHandle(pWindowControl, i32Index+nItemStart, &hItem);
				dRet = GEL_SetParam(hItem,PARAM_SETDISABLED, NULL);
			}
			else
			{
				dRet = GEL_SetParam(g_fmFactorySetting_data,PARAM_ITEM_ENABLE, (void*)&i32Index);
				dRet = GEL_GetHandle(pWindowControl, i32Index+nItemStart, &hItem);
				dRet = GEL_SetParam(hItem,PARAM_SETNORMAL, NULL);
			}
		}
	}
	
	dRet = GEL_GetParam(g_fmFactorySetting_data, PARAM_CURRENT_INDEX, &i32Index);
	dRet = GEL_SetParam(g_fmFactorySetting_data, PARAM_SETFOCUSED, NULL);
	i32Index +=nItemStart;
	dRet = GEL_GetHandle(pWindowControl, i32Index, &hFocusItem);
	dRet = GEL_SetParam(hFocusItem, PARAM_SETFOCUSED, NULL);

	GUI_FUNC_CALL(GEL_SetAnimEnable(hFocusItem, FALSE)); // S.T. fixed
	APP_GuiMgr_RegionBufferControl(&hFocusItem, 1); // S.T. fixed
	//GUI_FUNC_CALL(GEL_UpdateOSD());
	GUI_FUNC_CALL(GEL_SetAnimEnable(hFocusItem, TRUE)); // S.T. fixed

	GUI_FUNC_CALL( GEL_ShowMenu(pWindow) );// TODO: Replace it by  RegionBuffer
	GUI_FUNC_CALL(GEL_UpdateOSD());

 	return (UINT32)dRet;
}

static INT16 _APP_GUIOBJ_FM_Volume_CalcValueByVolPoint(UINT8 SetVolumeValue)
{
	INT16 CalculateValue[101] = {0,};
	INT16 K;
	INT8 Dectect = 0;
	INT16 PreCalculateValue = 0;
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();
	
	if (SetVolumeValue == 0)
	{
		CalculateValue[0] = g_stSysInfoData.szAudioVolumeTab[sourceindex][0];
	}
	else if (SetVolumeValue == 10)
	{
		CalculateValue[10] = g_stSysInfoData.szAudioVolumeTab[sourceindex][10];
	}
	else if (SetVolumeValue == 20)
	{
		CalculateValue[20] = g_stSysInfoData.szAudioVolumeTab[sourceindex][20];
	}
	else if (SetVolumeValue == 30)
	{
		CalculateValue[30] = g_stSysInfoData.szAudioVolumeTab[sourceindex][30];
	}
	else if (SetVolumeValue == 40)
	{
		CalculateValue[40] = g_stSysInfoData.szAudioVolumeTab[sourceindex][40];
	}
	else if (SetVolumeValue == 50)
	{
		CalculateValue[50] = g_stSysInfoData.szAudioVolumeTab[sourceindex][50];
	}
	else if (SetVolumeValue == 60)
	{
		CalculateValue[60] = g_stSysInfoData.szAudioVolumeTab[sourceindex][60];
	}
	else if (SetVolumeValue == 70)
	{
		CalculateValue[70] = g_stSysInfoData.szAudioVolumeTab[sourceindex][70];
	}
	else if (SetVolumeValue == 80)
	{
		CalculateValue[80] = g_stSysInfoData.szAudioVolumeTab[sourceindex][80];
	}
	else if (SetVolumeValue == 90)
	{
		CalculateValue[90] = g_stSysInfoData.szAudioVolumeTab[sourceindex][90];
	}
	else if (SetVolumeValue == 100)
	{
		CalculateValue[100] = g_stSysInfoData.szAudioVolumeTab[sourceindex][100];
	}
	else //Calculate Other Point Value
	{
		switch (SetVolumeValue / 10)
		{
			case 0:
				if ((g_stSysInfoData.szAudioVolumeTab[sourceindex][0]!= g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint0)
				 || (g_stSysInfoData.szAudioVolumeTab[sourceindex][10]!= g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint10))
				{
					K = (g_stSysInfoData.szAudioVolumeTab[sourceindex][10]	- g_stSysInfoData.szAudioVolumeTab[sourceindex][0]) / 10;
					Dectect = (g_stSysInfoData.szAudioVolumeTab[sourceindex][10]- g_stSysInfoData.szAudioVolumeTab[sourceindex][0] > 0) ? (1) : (-1);
					if (K != 0)
					{
						CalculateValue[SetVolumeValue] = K*(SetVolumeValue - 0)	+ g_stSysInfoData.szAudioVolumeTab[sourceindex][0];
					}
					else
					{
						PreCalculateValue = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue - 1];
						if (PreCalculateValue != g_stSysInfoData.szAudioVolumeTab[sourceindex][10])
						{
							CalculateValue[SetVolumeValue] = Dectect*(SetVolumeValue - 0)+ g_stSysInfoData.szAudioVolumeTab[sourceindex][0];
						}
						else
						{
							CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][10];
						}
					}
				}
				else
				{
					CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue];
				}
				break;

			case 1:
				if ((g_stSysInfoData.szAudioVolumeTab[sourceindex][10] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint10)
				 || (g_stSysInfoData.szAudioVolumeTab[sourceindex][20] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint20))
				{
					K = (g_stSysInfoData.szAudioVolumeTab[sourceindex][20]- g_stSysInfoData.szAudioVolumeTab[sourceindex][10]) / 10;
					Dectect = (g_stSysInfoData.szAudioVolumeTab[sourceindex][20] - g_stSysInfoData.szAudioVolumeTab[sourceindex][10] > 0) ? (1) : (-1);
					if (K != 0)
					{
						CalculateValue[SetVolumeValue] = K*(SetVolumeValue - 10) + g_stSysInfoData.szAudioVolumeTab[sourceindex][10];
					}
					else
					{
						PreCalculateValue = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue - 1];
						if (PreCalculateValue != g_stSysInfoData.szAudioVolumeTab[sourceindex][20])
						{
							CalculateValue[SetVolumeValue] = Dectect*(SetVolumeValue - 10) + g_stSysInfoData.szAudioVolumeTab[sourceindex][10];
						}
						else
						{
							CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][20];
						}
					}
				}
				else
				{
					CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue];
				}
				break;

			case 2:
				if ((g_stSysInfoData.szAudioVolumeTab[sourceindex][20] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint20)
				 || (g_stSysInfoData.szAudioVolumeTab[sourceindex][30] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint30))
				{
					K = (g_stSysInfoData.szAudioVolumeTab[sourceindex][30] - g_stSysInfoData.szAudioVolumeTab[sourceindex][20]) / 10;
					Dectect = (g_stSysInfoData.szAudioVolumeTab[sourceindex][30] - g_stSysInfoData.szAudioVolumeTab[sourceindex][20] > 0) ? (1) : (-1);
					if (K != 0)
					{
						CalculateValue[SetVolumeValue] = K*(SetVolumeValue - 20) + g_stSysInfoData.szAudioVolumeTab[sourceindex][20];
					}
					else
					{
						PreCalculateValue = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue - 1];
						if (PreCalculateValue != g_stSysInfoData.szAudioVolumeTab[sourceindex][30])
						{
							CalculateValue[SetVolumeValue] = Dectect*(SetVolumeValue - 20) + g_stSysInfoData.szAudioVolumeTab[sourceindex][20];
						}
						else
						{
							CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][30];
						}
					}
				}
				else
				{
					CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue];
				}
				break;

			case 3:
				if ((g_stSysInfoData.szAudioVolumeTab[sourceindex][30] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint30)
				 || (g_stSysInfoData.szAudioVolumeTab[sourceindex][40] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint40))
				{
					K = (g_stSysInfoData.szAudioVolumeTab[sourceindex][40] - g_stSysInfoData.szAudioVolumeTab[sourceindex][30]) / 10;
					Dectect = (g_stSysInfoData.szAudioVolumeTab[sourceindex][40] - g_stSysInfoData.szAudioVolumeTab[sourceindex][30] > 0) ? (1) : (-1);
					if (K != 0)
					{
						CalculateValue[SetVolumeValue] = K*(SetVolumeValue - 30) + g_stSysInfoData.szAudioVolumeTab[sourceindex][30];
					}
					else
					{
						PreCalculateValue = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue - 1];
						if (PreCalculateValue != g_stSysInfoData.szAudioVolumeTab[sourceindex][40])
						{
							CalculateValue[SetVolumeValue] = Dectect*(SetVolumeValue - 30) + g_stSysInfoData.szAudioVolumeTab[sourceindex][30];
						}
						else
						{
							CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][40];
						}
					}
				}
				else
				{
					CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue];
				}
				break;

			case 4:
				if ((g_stSysInfoData.szAudioVolumeTab[sourceindex][40] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint40)
				 || (g_stSysInfoData.szAudioVolumeTab[sourceindex][50] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint50))
				{

					K = (g_stSysInfoData.szAudioVolumeTab[sourceindex][50] - g_stSysInfoData.szAudioVolumeTab[sourceindex][40]) / 10;
					Dectect = (g_stSysInfoData.szAudioVolumeTab[sourceindex][50] - g_stSysInfoData.szAudioVolumeTab[sourceindex][40] > 0) ? (1) : (-1);
					if (K != 0)
					{
						CalculateValue[SetVolumeValue] = K*(SetVolumeValue - 40) + g_stSysInfoData.szAudioVolumeTab[sourceindex][40];
					}
					else
					{
						PreCalculateValue = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue - 1];
						if (PreCalculateValue != g_stSysInfoData.szAudioVolumeTab[sourceindex][50])
						{
							CalculateValue[SetVolumeValue] = Dectect*(SetVolumeValue - 40) + g_stSysInfoData.szAudioVolumeTab[sourceindex][40];
						}
						else
						{
							CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][50];
						}
					}
				}
				else
				{
					CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue];
				}
				break;

			case 5:
				if ((g_stSysInfoData.szAudioVolumeTab[sourceindex][50] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint50)
			     || (g_stSysInfoData.szAudioVolumeTab[sourceindex][60] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint60))
				{
					K = (g_stSysInfoData.szAudioVolumeTab[sourceindex][60] - g_stSysInfoData.szAudioVolumeTab[sourceindex][50]) / 10;
					Dectect = (g_stSysInfoData.szAudioVolumeTab[sourceindex][60] - g_stSysInfoData.szAudioVolumeTab[sourceindex][50] > 0) ? (1) : (-1);
					if (K != 0)
					{
						CalculateValue[SetVolumeValue] = K*(SetVolumeValue - 50) + g_stSysInfoData.szAudioVolumeTab[sourceindex][50];
					}
					else
					{
						PreCalculateValue = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue - 1];
						if (PreCalculateValue != g_stSysInfoData.szAudioVolumeTab[sourceindex][60])
						{
							CalculateValue[SetVolumeValue] = Dectect*(SetVolumeValue - 50) + g_stSysInfoData.szAudioVolumeTab[sourceindex][50];
						}
						else
						{
							CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][60];
						}
					}
				}
				else
				{
					CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue];
				}
				break;

			case 6:
				if ((g_stSysInfoData.szAudioVolumeTab[sourceindex][60] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint60)
				 || (g_stSysInfoData.szAudioVolumeTab[sourceindex][70] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint70))
				{
					K = (g_stSysInfoData.szAudioVolumeTab[sourceindex][70] - g_stSysInfoData.szAudioVolumeTab[sourceindex][60]) / 10;
					Dectect = (g_stSysInfoData.szAudioVolumeTab[sourceindex][70] - g_stSysInfoData.szAudioVolumeTab[sourceindex][60] > 0) ? (1) : (-1);
					if (K != 0)
					{
						CalculateValue[SetVolumeValue] = K*(SetVolumeValue - 60) + g_stSysInfoData.szAudioVolumeTab[sourceindex][60];
					}
					else
					{
						PreCalculateValue = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue - 1];
						if (PreCalculateValue != g_stSysInfoData.szAudioVolumeTab[sourceindex][70])
						{
							CalculateValue[SetVolumeValue] = Dectect*(SetVolumeValue - 60) + g_stSysInfoData.szAudioVolumeTab[sourceindex][60];
						}
						else
						{
							CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][70];
						}
					}
				}
				else
				{
					CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue];
				}
				break;

			case 7:
				if ((g_stSysInfoData.szAudioVolumeTab[sourceindex][70] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint70)
				 || (g_stSysInfoData.szAudioVolumeTab[sourceindex][80] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint80))
				{
					K = (g_stSysInfoData.szAudioVolumeTab[sourceindex][80] - g_stSysInfoData.szAudioVolumeTab[sourceindex][70]) / 10;
					Dectect = (g_stSysInfoData.szAudioVolumeTab[sourceindex][80] - g_stSysInfoData.szAudioVolumeTab[sourceindex][70] > 0) ? (1) : (-1);
					if (K != 0)
					{
						CalculateValue[SetVolumeValue] = K*(SetVolumeValue - 70) + g_stSysInfoData.szAudioVolumeTab[sourceindex][70];
					}
					else
					{
						PreCalculateValue = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue - 1];
						if (PreCalculateValue != g_stSysInfoData.szAudioVolumeTab[sourceindex][80])
						{
							CalculateValue[SetVolumeValue] = Dectect*(SetVolumeValue - 70) + g_stSysInfoData.szAudioVolumeTab[sourceindex][70];
						}
						else
						{
							CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][80];
						}
					}
				}
				else
				{
					CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue];
				}
				break;

			case 8:
				if ((g_stSysInfoData.szAudioVolumeTab[sourceindex][80] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint80)
			     || (g_stSysInfoData.szAudioVolumeTab[sourceindex][90] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint90))
				{
					K = (g_stSysInfoData.szAudioVolumeTab[sourceindex][90] - g_stSysInfoData.szAudioVolumeTab[sourceindex][80]) / 10;
					Dectect = (g_stSysInfoData.szAudioVolumeTab[sourceindex][90] - g_stSysInfoData.szAudioVolumeTab[sourceindex][80] > 0) ? (1) : (-1);
					if (K != 0)
					{
						CalculateValue[SetVolumeValue] = K*(SetVolumeValue - 80) + g_stSysInfoData.szAudioVolumeTab[sourceindex][80];
					}
					else
					{
						PreCalculateValue = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue - 1];
						if (PreCalculateValue != g_stSysInfoData.szAudioVolumeTab[sourceindex][90])
						{
							CalculateValue[SetVolumeValue] = Dectect*(SetVolumeValue - 80) + g_stSysInfoData.szAudioVolumeTab[sourceindex][80];
						}
						else
						{
							CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][90];
						}
					}
				}
				else
				{
					CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue];
				}
				break;

			case 9:
				if ((g_stSysInfoData.szAudioVolumeTab[sourceindex][90] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint90)
				 || (g_stSysInfoData.szAudioVolumeTab[sourceindex][100] != g_efmPreVolumeValue.n_Audio_AVL_Vol_VolumePoint100))
				{
					K = (g_stSysInfoData.szAudioVolumeTab[sourceindex][100] - g_stSysInfoData.szAudioVolumeTab[sourceindex][90]) / 10;
					Dectect = (g_stSysInfoData.szAudioVolumeTab[sourceindex][100] - g_stSysInfoData.szAudioVolumeTab[sourceindex][90] > 0) ? (1) : (-1);
					if (K != 0)
					{
						CalculateValue[SetVolumeValue] = K*(SetVolumeValue - 90) + g_stSysInfoData.szAudioVolumeTab[sourceindex][90];
					}
					else
					{
						PreCalculateValue = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue - 1];
						if (PreCalculateValue != g_stSysInfoData.szAudioVolumeTab[sourceindex][100])
						{
							CalculateValue[SetVolumeValue] = Dectect*(SetVolumeValue - 90) + g_stSysInfoData.szAudioVolumeTab[sourceindex][90];
						}
						else
						{
							CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][100];
						}
					}
				}
				else
				{
					CalculateValue[SetVolumeValue] = g_stSysInfoData.szAudioVolumeTab[sourceindex][SetVolumeValue];
				}
				break;

			default:
				break;
		}
	}

	return CalculateValue[SetVolumeValue];
}

static void _APP_GUIOBJ_FM_CurveVolumeValue(void)
{
	UINT8 U8SetVolumeTable = 0;
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();

	for (U8SetVolumeTable = 0; U8SetVolumeTable < 101; U8SetVolumeTable++)
	{
		//APP_Audio_SetVolumeTableByIndex(U8SetVolumeTable, _APP_GUIOBJ_FM_Volume_CalcValueByVolPoint(U8SetVolumeTable));
		g_stSysInfoData.szAudioVolumeTab[sourceindex][U8SetVolumeTable] = _APP_GUIOBJ_FM_Volume_CalcValueByVolPoint(U8SetVolumeTable);
		AL_Setting_Write(
			APP_Data_UserSetting_Handle(), 
			SYS_SET_ID_SYSINFO,
			ITEM_OFFSET(APP_SETTING_SystemInfo_t, szAudioVolumeTab[sourceindex][U8SetVolumeTable]),
			sizeof(g_stSysInfoData.szAudioVolumeTab[sourceindex][U8SetVolumeTable]),
			&(g_stSysInfoData.szAudioVolumeTab[sourceindex][U8SetVolumeTable]));
	}
	/*
	AL_Setting_Store(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_SYSINFO,
		ITEM_OFFSET(APP_SETTING_SystemInfo_t, 
		szAudioVolumeTab),
		sizeof(al_int16) * 101);
	*/
}

void _APP_GUIOBJ_FM_FactorySetting_InitValue(void)
{
	// Read  all
	AL_Setting_Read(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_FACTUSER,
		0,
		sizeof(APP_SETTING_FactoryUser_t), 
		&g_stFactoryUserData );

	AL_Setting_Read(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_PICTURE, 
		0,
		sizeof(APP_SETTING_Picture_t), 
		&g_stPictureData);

	AL_Setting_Read(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_SETUP, 
		0,
		sizeof(APP_SETTING_Setup_t), 
		&g_stSetupData);

	AL_Setting_Read(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_SOUND, 
		0,
		sizeof(APP_SETTING_Sound_t), 
		&g_stSoundData);

	AL_Setting_Read(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_VARIATIONAL, 
		0,
        sizeof(APP_SETTING_Variational_t), 
        &g_stVariationalData);

	AL_Setting_Read(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_SYSINFO, 
		0,
		sizeof(APP_SETTING_SystemInfo_t), 
		&g_stSysInfoData);

	AL_Setting_Read(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_FEATURE, 
		0,
		sizeof(APP_SETTING_Feature_t), 
		&g_stFeatureData);

	Initialize_SounMode_Vol_VolumePoint();
	
}

void _APP_GUIOBJ_FM_FactorySetting_SaveValue(void)
{
	// Write back all
	AL_Setting_Store(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_FACTUSER,
		0,
 		sizeof(APP_SETTING_FactoryUser_t) );

	AL_Setting_Store(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_PICTURE, 
		0,
		sizeof(APP_SETTING_Picture_t) );

	AL_Setting_Store(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_SETUP, 
		0,
		sizeof(APP_SETTING_Setup_t) );

	AL_Setting_Store(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_SOUND, 
		0,
		sizeof(APP_SETTING_Sound_t) );

	AL_Setting_Store(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_VARIATIONAL, 
		0,
 	    sizeof(APP_SETTING_Variational_t) );

	AL_Setting_Store(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_SYSINFO, 
		0,
 		sizeof(APP_SETTING_SystemInfo_t) );

	AL_Setting_Store(
		APP_Data_UserSetting_Handle(), 
		SYS_SET_ID_FEATURE, 
		0,
 		sizeof(APP_SETTING_Feature_t) );
}


/*****************************************************************************
** FUNCTION : Common_GUIOBJ_FM_FactorySetting_OnCreate
**
** DESCRIPTION :
**				Factory Gui Object Create
**
** PARAMETERS :
**				pPrivateData   - User Data
**				dParameter   - Special Parameter
**
** RETURN VALUES:
**				SP_SUCCESS
*****************************************************************************/

static INT32 _APP_GUIOBJ_FM_FactorySetting_OnCreate(void** pPrivateData , UINT32 dParameter)
{
	DEBF("[Common fm factory guiobj] %s is called.\n", __FUNCTION__);
#ifdef CONFIG_SUPPORT_PVR
	b_Isrecording = FALSE;
#endif
	if(dParameter == ID_FM_FactSet_Version)
	{
		//g_bOpenVersionDir = TRUE;
		nowNodeHeadId = ID_FM_FactSet_Version;
	}
	else if(dParameter == ID_FM_PicMode_CurveSetting)
	{
		g_bOpenVersionDir = FALSE;
		nowNodeHeadId = ID_FM_PicMode_CurveSetting;
	}
	else
	{
		g_bOpenVersionDir = FALSE;
		nowNodeHeadId = ID_ROOT;
	}
	pppStrList = NULL;
	GUIResult_e dRet = GUI_SUCCESS;

	g_dRegionHandle = APP_GuiMgr_ActivateRegion(pWindow, OSD_TVPROJECT, FALSE);

	dRet = GEL_CreateMenu( pWindow, g_dRegionHandle);
	dRet = GEL_GetHandle(pWindowControl,
		nItemList,
		&g_fmFactorySetting_data);

	int focus_test = 0;
	dRet = GEL_SetParam(g_fmFactorySetting_data, PARAM_CURRENT_INDEX, &focus_test);
	dRet = GEL_SetParam(g_fmFactorySetting_data, PARAM_SETFOCUSED, NULL);

	//Init string list
	Current_Node_Count = (nItemEnd - nItemStart +1);
	if(pppStrList == NULL)
	{
		pppStrList = (char ***)malloc( sizeof(char **) * (Current_Node_Count+1) );
		int i;
		for(i=0; i<=Current_Node_Count; i++)
		{
			// malloc char **
			pppStrList[i] = (char **)malloc( sizeof(char *) * 2 );

			// malloc char *
			pppStrList[i][0] = (char *)malloc( sizeof(char ) * 60);
			pppStrList[i][1] = (char *)&string_last;
		}
	}

	// Init Empty String
	UINT32 i;
	for(i=0; i<FM_ITEMS_PER_PAGE; i++)
	{
		pEmptyStrings[i] = TV_IDS_String_NULL;
	}
	pEmptyStrings[FM_ITEMS_PER_PAGE] = STRING_LAST;


	// Added for int value
	_APP_GUIOBJ_FM_FactorySetting_InitValue();
	
	g_bFlipIsChange = FALSE;
	//u8FlipIndex = g_stFactoryUserData.Function.PanelSetting.n_FlipIndex ;
	MID_DISP_DTVGetFlip( (MID_DISP_FlipType_t *)(&u8FlipIndex) ); // Mantis 0025944
	
	MAINAPP_GetActiveSystemAppIndex(&u_CurrentAppIndex);
	
	dRet = _APP_Update_Layer(nowNodeHeadId);

	return (INT32)dRet;//SP_SUCCESS;
}

/*****************************************************************************
** FUNCTION : Common_GUIOBJ_FM_FactorySetting_OnFocused
**
** DESCRIPTION :
**				Factory Gui Object Focused
**
** PARAMETERS :
**				pPrivateData   - User Data
**
** RETURN VALUES:
**				SP_SUCCESS
*****************************************************************************/
static INT32 _APP_GUIOBJ_FM_FactorySetting_OnFocused(void* pPrivateData)
{
	DEBF("[Common fm factory guiobj] %s is called.\n", __FUNCTION__);

	_APP_GUIOBJ_FM_FactorySetting_UpdateMenu();

	return SP_SUCCESS;
}

/*****************************************************************************
** FUNCTION : Common_GUIOBJ_FM_FactorySetting_OnLoseFocus
**
** DESCRIPTION :
**				Factory Gui Object Lose Focused
**
** PARAMETERS :
**				pPrivateData   - User Data
**
** RETURN VALUES:
**				SP_SUCCESS
*****************************************************************************/
static INT32 _APP_GUIOBJ_FM_FactorySetting_OnLoseFocus(void* pPrivateData)
{
	DEBF("[Common fm factory guiobj] %s is called.\n", __FUNCTION__);

	return SP_SUCCESS;
}

static INT32 _APP_Fm_Disable_Video_InternalPattern(void)
{
	UINT32	dIndex; 		
	MID_InputInfo_t TimingInfo;
	APP_Source_Type_t eSourType = APP_SOURCE_MAX;
	MID_TVFE_GetCurrentVideoInfo(&TimingInfo, NULL);
	APP_GUIOBJ_Source_GetCurrSource(&eSourType);
	if (eSourType == APP_SOURCE_MEDIA)
	{//Media source, APS need always unmute
#ifdef CONFIG_SUPPORT_ALL_ACTION_SHOW_BLUE_SCREEN
		MID_DISP_DTVSetMuteColor(0, 0, 255);
#else
		MID_DISP_DTVSetMuteColor(0, 0, 0);
#endif
		MID_DISP_DTVSetVideoUnmute();
	}
	else if (TimingInfo.eInputStatus == STATUS_NOSIGNAL)
	{
		if (eSourType == APP_SOURCE_DTV)
		{
			if (MAIN_APP_SUCCESS == MAINAPP_GetActiveSystemAppIndex(&dIndex))
			{
				if(!SYSAPP_GOBJ_GUIObjectExist(dIndex, DVB_GUIOBJ_PVR_FILEPLAY))
				{
#ifdef CONFIG_SUPPORT_ALL_ACTION_SHOW_BLUE_SCREEN
                  	MID_DISP_DTVSetVideoMute(0,0,255);
#else
					MID_DISP_DTVSetVideoMute(0, 0, 0);
#endif
				}
				else
				{
#ifdef CONFIG_SUPPORT_ALL_ACTION_SHOW_BLUE_SCREEN
                    MID_DISP_DTVSetMuteColor(0,0,255);
#else
					MID_DISP_DTVSetMuteColor(0, 0, 0);
#endif
					MID_DISP_DTVSetVideoUnmute();
				}
			}
		}
		else
		{
			AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
				sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
			if (g_stSetupData.BlueScreen == APP_SWITCH_OFF)
			{
				if (eSourType == APP_SOURCE_ATV)
				{
					MID_DISP_DTVSetVideoUnmute();
				}
				else
				{
#ifdef CONFIG_SUPPORT_ALL_ACTION_SHOW_BLUE_SCREEN
                  	MID_DISP_DTVSetVideoMute(0,0,255);
#else
					MID_DISP_DTVSetVideoMute(0, 0, 0);
#endif
				}
			}
			else
			{
#ifdef CONFIG_SUPPORT_ALL_ACTION_SHOW_BLUE_SCREEN
                MID_DISP_DTVSetVideoMute(0,0,255);
#else
				MID_DISP_DTVSetVideoMute(0, 0, 0xFF);
#endif
			}
		}
	}
	else
	{
#ifdef CONFIG_SUPPORT_ALL_ACTION_SHOW_BLUE_SCREEN
		MID_DISP_DTVSetMuteColor(0, 0, 255);
#else
		MID_DISP_DTVSetMuteColor(0, 0, 0);
#endif
		MID_DISP_DTVSetVideoUnmute();
	}
	return SP_SUCCESS;
}

/*****************************************************************************
** FUNCTION : Common_GUIOBJ_FM_FactorySetting_OnRelease
**
** DESCRIPTION :
**				Factory Gui Object Destroy
**
** PARAMETERS :
**				pPrivateData   -  User Data
**
** RETURN VALUES:
**				SP_SUCCESS
*****************************************************************************/
static INT32 _APP_GUIOBJ_FM_FactorySetting_OnRelease(void* pPrivateData)
{
	DEBF("[Common fm factory guiobj] %s is called.\n", __FUNCTION__);

	GUIResult_e dRet = GUI_SUCCESS;
	g_bOpenVersionDir = FALSE;

	// Save values
	_APP_GUIOBJ_FM_FactorySetting_SaveValue();

	if(g_bFlipIsChange == TRUE)
	{
	    //SYSAPP_IF_SendGlobalEventWithIndex(u_CurrentAppIndex, APP_GLOBAL_EVENT_FLIP|PASS_TO_SYSAPP,0);
		g_bFlipIsChange = FALSE;
	}
	else
	{
		dRet = GEL_DestroyMenu(pWindow, TRUE);
		UINT32 StringId = 0;
		APP_GUIOBJ_Language_GetOsdLanguage(&StringId);
		GUI_FUNC_CALL(GEL_SetGlobalLang(APP_GuiMgr_GetCurFontLangIndex((UINT32*)&StringId, OSD_TVPROJECT)));
	}
	//release  memory
	UINT32 i = 0;
	if(pppStrList)
	{
		for(i=0; i<=(UINT32)Current_Node_Count; i++)
		{
			// free char *
			free( pppStrList[i][0] );
			pppStrList[i][0] = NULL;
			pppStrList[i][1] = NULL;

			// free char **
			free(pppStrList[i]);
			pppStrList[i] = NULL;
		}
		// free char ***
		free(pppStrList);
		pppStrList = NULL;
	}
	// Free strings
	Fee_LeftSide_List();
	
	if(!g_stFactoryUserData.n_FactSet_BurningMode)
	{
		/* For VideoPattern function Begin */
		_APP_Fm_Disable_Video_InternalPattern();
		/* For VideoPattern function End */
	}

#ifdef CONFIG_SUPPORT_PVR
	if(b_Isrecording)
	{
		APP_GUIOBJ_DVB_PvrRec_TSRecStop();
		b_Isrecording=FALSE;
	}
#endif

	GUI_FUNC_CALL(GEL_UpdateOSD());

	return (INT32) dRet ;
}

/*****************************************************************************
** FUNCTION : Common_GUIOBJ_FM_FactorySetting_OnTimerUpdate
**
** DESCRIPTION :
**				Factory Gui Object Timer Update
**
** PARAMETERS :
**				pPrivateData   - User Data
**				pPostEventData   -
**
** RETURN VALUES:
**				SP_SUCCESS
*****************************************************************************/
static INT32 _APP_GUIOBJ_FM_FactorySetting_OnTimerUpdate(
	void* pPrivateData, InteractiveData_t *pPostEventData)
{
	DEBF("[Common fm factory guiobj] %s is called.\n", __FUNCTION__);

	if(nowNodeHeadId == ID_FM_PicMode_OverScan)
	{
		int nIndex;
		// Note: Can't Disable all
		int nStartIndex = ID_FM_PicMode_OverScan_VTopOverScan - ID_FM_PicMode_OverScan_ScaleMode;
		int nEndIndex = ID_FM_PicMode_OverScan_HRightOverScan - ID_FM_PicMode_OverScan_ScaleMode;
		HWND FocusItem_Handle;
	#ifdef SUPPORT_OVERSCAN_MOVIE_ONMEDIA//  dchen105  20141210
		APP_Source_Type_t eSrcType = APP_SOURCE_MAX;
		APP_GUIOBJ_Source_GetCurrSource(&eSrcType);
		if ((eSrcType == APP_SOURCE_MEDIA)
			&&((SYSAPP_GOBJ_GUIObjectExist(SYS_APP_FILE_PLAYER, MEDIA_GUIOBJ_MOVIE_PLAYBACK)))
			//||SYSAPP_GOBJ_GUIObjectExist(SYS_APP_FILE_PLAYER, MEDIA_GUIOBJ_PVR_PLAYBACK))
			)
		{
			//Enable items++	
			for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
			{	
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));
			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));
			}
		}
		else
	#endif
	 	{
			if( !IsOverScanAvalible() )
			{
				//Disable items

				for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
				{
					GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

					GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
					GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));
				}
			}
			else
			{
				//Enable items

				for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
				{
					GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

					GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
					GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));
				}
			}
		}
			_APP_GUIOBJ_FM_FactorySetting_UpdateMenu();
	}
#if 0
	g_u32Timer++;
	if (g_u32Timer >= 3)
	{
		pPostEventData->dEventID = GUI_OBJECT_CLOSE;
		return GUI_OBJECT_POST_EVENT;
	}
#endif
	return SP_SUCCESS;
}

static INT32 _APP_GUIOBJ_FM_OSDLanguage_ResetDefault(void)
{
	UINT64 OSD_Support_Language_temp = 0;
	extern const e_OSD_Language_Table OSD_Language_MapTab[];
	extern UINT8 OSD_Language_MapTab_num;
	UINT32 u32TempValue = g_stFeatureData.OSDLang;
	int OSD_Language_Index = 0;
	UINT8 Support_Language_Flag = 0;
	UINT32 StringId = 0;
	UINT8 index = 0;
	UINT8 i;
	/*
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,0,
			sizeof(APP_SETTING_FactoryUser_t), &g_stFactoryUserData);
	*/
	AL_FLASH_SetOSDSupportLanguage(g_stSettingDefault_FactoryUser.SystemConfig.n_SysConf_OSDLanguage);
	OSD_Support_Language_temp = AL_FLASH_GetOSDSupportLanguage();

	stLangOnOffvalue1.u8German 	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_German))>>SUPPORT_OSD_LANGUAGE_German;
	stLangOnOffvalue1.u8English 	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_English))>>SUPPORT_OSD_LANGUAGE_English;
	stLangOnOffvalue1.u8French 	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_French))>>SUPPORT_OSD_LANGUAGE_French;
    stLangOnOffvalue1.u8Italian 	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Italian))>>SUPPORT_OSD_LANGUAGE_Italian;
	stLangOnOffvalue1.u8Polish 	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Polish))>>SUPPORT_OSD_LANGUAGE_Polish;
    stLangOnOffvalue1.u8Spanish 	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Spanish))>>SUPPORT_OSD_LANGUAGE_Spanish;
    stLangOnOffvalue1.u8Netherlands = (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Netherlands))>>SUPPORT_OSD_LANGUAGE_Netherlands;
    stLangOnOffvalue1.u8Portuguese = (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Portuguese))>>SUPPORT_OSD_LANGUAGE_Portuguese;
    stLangOnOffvalue1.u8Swidish 	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Swidish))>>SUPPORT_OSD_LANGUAGE_Swidish;
    stLangOnOffvalue1.u8Finnish 	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Finnish))>>SUPPORT_OSD_LANGUAGE_Finnish;
	stLangOnOffvalue1.u8Greek	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Greek))>>SUPPORT_OSD_LANGUAGE_Greek;

	stLangOnOffvalue2.u8Russian = (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Russian))>>SUPPORT_OSD_LANGUAGE_Russian;
	stLangOnOffvalue2.u8Turkey	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Turkey))>>SUPPORT_OSD_LANGUAGE_Turkey;
	stLangOnOffvalue2.u8Danish		= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Danish))>>SUPPORT_OSD_LANGUAGE_Danish;
	stLangOnOffvalue2.u8Norwegian	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Norwegian))>>SUPPORT_OSD_LANGUAGE_Norwegian;
	stLangOnOffvalue2.u8Hungarian	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Hungarian))>>SUPPORT_OSD_LANGUAGE_Hungarian;
	stLangOnOffvalue2.u8Czech		= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Czech))>>SUPPORT_OSD_LANGUAGE_Czech;
	stLangOnOffvalue2.u8Slovakian	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Slovakian))>>SUPPORT_OSD_LANGUAGE_Slovakian;
	stLangOnOffvalue2.u8Croatian	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Croatian))>>SUPPORT_OSD_LANGUAGE_Croatian;
	stLangOnOffvalue2.u8Serbian 	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Serbian))>>SUPPORT_OSD_LANGUAGE_Serbian;
	stLangOnOffvalue2.u8Arabic		= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Arabic))>>SUPPORT_OSD_LANGUAGE_Arabic;
	stLangOnOffvalue2.u8Persian 	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Persian))>>SUPPORT_OSD_LANGUAGE_Persian;
/*
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FEATURE, 0,
		sizeof(APP_SETTING_Feature_t), &(g_stFeatureData));
*/
	for(i=0;i<OSD_Language_MapTab_num;i++)
	{
		if(OSD_Language_MapTab[i].APP_OSD_Language == u32TempValue)
		{
			OSD_Language_Index = i;
			break;
		}
	}
	Support_Language_Flag =(OSD_Support_Language_temp&(1<<OSD_Language_MapTab[OSD_Language_Index].SupportOSDLanguage))>>OSD_Language_MapTab[OSD_Language_Index].SupportOSDLanguage;
	if(Support_Language_Flag == 0)
	{
		for(index=SUPPORT_OSD_LANGUAGE_German;index<SUPPORT_OSD_LANGUAGE_Max;index++)
		{
			if((OSD_Support_Language_temp&(1<<index))>>index)
			{
				for(i = 0;i < OSD_Language_MapTab_num;i++)
				{
					if(OSD_Language_MapTab[i].SupportOSDLanguage == index)
						break;
				}
				g_stFeatureData.OSDLang = OSD_Language_MapTab[i].APP_OSD_Language;
				APP_GUIOBJ_Language_SetOsdLanguage(g_stFeatureData.OSDLang);
				StringId = APP_GUIOBJ_OSD_Language_MappingTo_StringID(g_stFeatureData.OSDLang);
				GUI_FUNC_CALL(GEL_SetGlobalLang(APP_GuiMgr_GetCurFontLangIndex((UINT32*)&StringId, OSD_TVPROJECT)));
				// Be done in APP_GUIOBJ_Language_SetOsdLanguage()
				/*
				AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FEATURE,
					ITEM_OFFSET(APP_SETTING_Feature_t, OSDLang),
					sizeof(g_stFeatureData.OSDLang),&(g_stFeatureData.OSDLang));
				*/
				/*
				AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FEATURE,
					ITEM_OFFSET(APP_SETTING_Feature_t, OSDLang), sizeof(g_stFeatureData.OSDLang));
				*/
				break;
			}
		}
	}
	/*
	AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
		ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.n_SysConf_OSDLanguage),
		sizeof(g_stFactoryUserData.SystemConfig.n_SysConf_OSDLanguage));
	*/
    return SP_SUCCESS;
}

void APP_Cul_Fm_CheckAndStartBurningMode(void)
{
	
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
	            sizeof(APP_SETTING_Setup_t), &(g_stSetupData));

	if(BurnInModeTaskExistFlag == 0)
	{
#ifdef SUPPORT_LED_FLASH
		APP_LED_Flash_Timer_Set(LED_FLASH_TYPE_RECODER, 800);
#endif
#ifndef SUPPORT_FACTORY_BURNING_ADJ_TEST
#if (SYSCONF_BOARD_POWER == 0)
		Backlight_t BacklightSetting;
		BacklightSetting.Backlight_total_Stage = 100; // set total backlight stage = 100
		BacklightSetting.OSD_backlight_index = 100;
		Cmd_SetLcdBackLight(BacklightSetting);// when into  BurnMode BackLight must be max
#endif
#endif
		APP_Source_Type_t eSourType = APP_SOURCE_MAX;
		APP_GUIOBJ_Source_GetCurrSource(&eSourType);
		APP_Audio_SetMute(TRUE, FALSE,APP_MUTE_MODE_STATEMAX,eSourType);

        #ifdef CONFIG_MEDIA_SUPPORT
        if (APP_SOURCE_MEDIA == eSourType)
        {
            SysApp_MM_BurninMode_Pause_MediaPlay();
        }
        #endif

#ifdef CONFIG_SUPPORT_SUBTITLE
		AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO, 0,
			sizeof(APP_SETTING_UserInfo_t), &(g_stUserInfoData));
		if (g_stUserInfoData.Subtitle == DVB_SWITCH_ON)
		{
			UINT32 u32CurrentSysappIndex = 0;
			MAINAPP_GetActiveSystemAppIndex(&u32CurrentSysappIndex);
			if(u32CurrentSysappIndex == SYS_APP_DVB)
			{
				DVBApp_DataApplicationSwitch(OSD2CTRLMDL_DISABLE|OSD2CTRLMDL_SUB);
			}
		}
#endif
		//GL_TaskSleep(1000);
		OSD_SetDisplay(FALSE);
		MID_TVFE_SetBurnIn(TRUE, 0, TRUE);
	}
	BurnInModeTaskExistFlag = 1;
}


void APP_Cul_Direct_Show_Version_Page(BOOL bShow)
{
	if(bShow)
		_APP_GUIOBJ_FM_FactorySetting_OnCreate(NULL, ID_FM_FactSet_Version);
	else
		_APP_GUIOBJ_FM_FactorySetting_OnRelease(NULL);
}

/******************************************************************************************
* UMF function
******************************************************************************************/
void Factory_Gamma_Type_Setting(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		AL_FLASH_SetGammaType((UINT8) FuncData->value);
		// TODO: UMF function
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetGammaType();
	}
}

void Factory_SelectPQ(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	// Re-init radio table
	UINT32 i = 0;
	int nAvaliablePQ = 0;
	for( i=1 ; i<=sizeof(L_SelectPQ)/sizeof(FACT_RADIOBTN); i++)
	{
		char strName[15];	  //select file name, example=>char name[19]="5PQ_2.bin"
		char strPath[50]={0};	//strPath size must >= 50
		FILE *fp;
		sprintf(strName, "%s", strPQs[i]);

		MID_GetFilePath(strName, strPath);
		//printf("[Init Table] full path = %s\n", strPath);

		if( ( fp = fopen(strPath, "rb") ) != NULL)
		{
			L_SelectPQ[nAvaliablePQ].str = strPQs[i];
			L_SelectPQ[nAvaliablePQ].value = nAvaliablePQ;
			nAvaliablePQ++;

			fclose(fp);
		}
	}
	Fun_SelectPQ.nItems = nAvaliablePQ;

	if(nAvaliablePQ == 0)
	{
		// Not find any PQ file
		L_SelectPQ[0].str = strNone;
		L_SelectPQ[0].value = 0;

		Fun_SelectPQ.nItems = 1;
		return;
	}

	// Set /Get function
	if (bSet)
	{
		AL_FLASH_SetSelectPQ((UINT8) FuncData->value);

		char strName[15];	  //select file name, example=>char name[19]="5PQ_2.bin"
		char strPath[50]={0};	//strPath size must >= 50
		FILE *fp;
		sprintf(strName, "%s", L_SelectPQ[(UINT32)FuncData->value].str);
		//printf("name = %s\n", strName);

		MID_GetFilePath(strName, strPath);
		//printf("[Set Table] full path = %s\n", strPath);

		if( ( fp = fopen(strPath, "rb") ) != NULL)
		{
			if(fread(g_stFactoryUserData.PictureMode.WBAdjust.stColorTemp,sizeof(APP_SETTING_ColorTempTab_t)*APP_VIDEO_PQ_STORE_SOURCE_TYPE_MAX,1,fp) != 1)
			{
				printf("copy bin file fail \n");
			}
			fclose(fp);
			// write back to memeory and flash
		}
		//printf(".... all done\n");

	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetSelectPQ();
	}
}

void Factory_BacklightMode(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	   /* AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
                    sizeof(APP_SETTING_Setup_t), &(g_stSetupData));*/
	Backlight_t BacklightSetting;
	BacklightSetting.Backlight_total_Stage = 100;
	if(bSet)
	{
		//g_stSetupData.HomeMode.Type = FuncData->value;
		AL_FLASH_SetBackLightMode(FuncData->value);
		UpdateNodeFunctionContent(ID_FM_PicMode_ADCAdjust,0,0);
		AL_FLASH_SetBackLight(AL_FLASH_GetBackLight());
        	BacklightSetting.OSD_backlight_index = AL_FLASH_GetBackLight();

		Cmd_SetLcdBackLight(BacklightSetting);
	}
	else
	{
		FuncData->value = AL_FLASH_GetBackLightMode();
		//AL_FLASH_GetBackLightMode();
	}
	
}

void Factory_BackLight(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
    Backlight_t BacklightSetting;
    BacklightSetting.Backlight_total_Stage = 100; // set total backlight stage = 100
	if (bSet)
	{
		DEBF("[FM DBG] %s SET (%d) ...\n", __FUNCTION__, (UINT8) FuncData->value);
		AL_FLASH_SetBackLight((UINT8) FuncData->value);
        BacklightSetting.OSD_backlight_index = FuncData->value;

		Cmd_SetLcdBackLight(BacklightSetting);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetBackLight();

		DEBF("[FM DBG] %s GET (%d) ...\n", __FUNCTION__, (UINT8) FuncData->value);
	}
}

void Factory_DynamicBacklight(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Backlight_t BacklightSetting;
	if(bSet)
	{
		//Set function
		DEBF("[FM DBG] %s SET (%d) ...\n", __FUNCTION__, (UINT8) FuncData->value);
		AL_FLASH_SetDynamicBackLight((UINT8)FuncData->value);
		
		#if 0
		MID_TVFE_SetPictureDynamicBacklight((UINT8)FuncData->value);

		BacklightSetting.Backlight_total_Stage = 100; // set total backlight stage = 100
    	if (FuncData->value == FALSE) //Dynamic Backlight :OFF
    	{
    		//while DB OFF, Set backlight again
     		BacklightSetting.OSD_backlight_index = AL_FLASH_GetBackLight();
        	Cmd_SetLcdBackLight(BacklightSetting);
    	}

		#else
		BacklightSetting.Backlight_total_Stage = 100; // set total backlight stage = 100
		
		if(AL_FLASH_GetDynamicBackLight())
		{
			MID_DISP_DTVSetDynamicBLType(DISP_DYNAMIC_BL_ENERGY_EFFICIENCY);
			MID_TVFE_SetPictureDynamicBacklight(TRUE);
		}
		else
		{
			MID_DISP_DTVSetDynamicBLType(DISP_DYNAMIC_BL_FORMAL);
			MID_TVFE_SetPictureDynamicBacklight(FALSE);
			BacklightSetting.OSD_backlight_index = AL_FLASH_GetBackLight();
			Cmd_SetLcdBackLight(BacklightSetting);
		}
		#endif
	}
	else
	{
		//Get function
		FuncData->value = AL_FLASH_GetDynamicBackLight();
		DEBF("[FM DBG] %s GET (%d) ...\n", __FUNCTION__, (UINT8) FuncData->value);
	}
}

typedef struct
{
	UINT8 u8ChanIdx;
	UINT32 u32ChanFreq;
	UINT8 u8AudioStd;
	UINT8 u8VideoStd;
	UINT8 u8SaveChannel;
} APP_FM_AtvChnSet_t;

static UINT8 g_s8CurChanIdx = 0;
static APP_FM_AtvChnSet_t stAtvChannelSetting;

static INT32 _FM_AtvChnSet_ResetValue(void)
{
	stAtvChannelSetting.u32ChanFreq = 0;
	stAtvChannelSetting.u8AudioStd = 0;
    stAtvChannelSetting.u8VideoStd = 0;
	stAtvChannelSetting.u8SaveChannel = 0;
    return SP_SUCCESS;
}
static APP_OsdColorSystem_t _FM_AtvChnSet_VideoStdOSD2SaveDB(void)
{
	APP_OsdColorSystem_t bColorSystem = APP_OSD_COLOR_SYSTEM_AUTO;
	switch (stAtvChannelSetting.u8VideoStd)
	{
		case 0:
			bColorSystem = APP_OSD_COLOR_SYSTEM_AUTO;
			break;
		case 1:
			bColorSystem = APP_OSD_COLOR_SYSTEM_PAL;
			break;
		case 2:
			bColorSystem = APP_OSD_COLOR_SYSTEM_PALM;
			break;
		case 3:
			bColorSystem = APP_OSD_COLOR_SYSTEM_SECAM;
			break;
		case 4:
			bColorSystem = APP_OSD_COLOR_SYSTEM_NTSC358;
			break;
		default:
			bColorSystem = APP_OSD_COLOR_SYSTEM_AUTO;
			break;
	}
	return bColorSystem;
}
static UINT8 _FM_AtvChnSet_VideoStdDB2OSD(void)
{
	UINT8 bColorSystem = 0;
	switch (g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].ColorSystem)
	{
		case APP_OSD_COLOR_SYSTEM_PAL:
			bColorSystem = 1;
			break;
		case APP_OSD_COLOR_SYSTEM_PALM:
			bColorSystem = 2;
			break;
		case APP_OSD_COLOR_SYSTEM_SECAM:
			bColorSystem = 3;
			break;
		case APP_OSD_COLOR_SYSTEM_NTSC358:
			bColorSystem = 4;
			break;
		default:
			bColorSystem = 0;
			break;
	}
	return bColorSystem;
}

static Ana_SaveScanAudioStd_t _FM_AtvChnSet_AudioStdOSD2SaveDB(void)
{
	Ana_SaveScanAudioStd_t baudiosystem = ANA_SCAN_AUDIO_STD_ERR;
	switch (stAtvChannelSetting.u8AudioStd)
	{
		case 0:
			baudiosystem = ANA_SCAN_AUDIO_STD_ERR;
			break;
		case 1:
			baudiosystem = ANA_SCAN_AUDIO_STD_NICAMBG;
			break;
		case 2:
			baudiosystem = ANA_SCAN_AUDIO_STD_NICAMDK;
			break;
		case 3:
			baudiosystem = ANA_SCAN_AUDIO_STD_NICAMI;
			break;
		case 4:
			baudiosystem = ANA_SCAN_AUDIO_STD_NICAML;
			break;
		case 5:
			baudiosystem = ANA_SCAN_AUDIO_STD_NICAML1;
			break;
		case 6:
			baudiosystem = ANA_SCAN_AUDIO_STD_BTSC;
			break;
		default:
			baudiosystem = ANA_SCAN_AUDIO_STD_ERR;
			break;
	}
	return baudiosystem;
}
static UINT8 _FM_AtvChnSet_AudioStdDB2OSD(void)
{
	UINT8 bsoundSystem = 0;
	switch (g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].SoundSystem)
	{
		case ANA_SCAN_AUDIO_STD_ERR:
			bsoundSystem = 0;
			break;
		case ANA_SCAN_AUDIO_STD_NICAMBG:
			bsoundSystem = 1;
			break;
		case ANA_SCAN_AUDIO_STD_NICAMDK:
			bsoundSystem = 2;
			break;
		case ANA_SCAN_AUDIO_STD_NICAMI:
			bsoundSystem = 3;
			break;
		case ANA_SCAN_AUDIO_STD_NICAML:
			bsoundSystem = 4;
			break;
		case ANA_SCAN_AUDIO_STD_NICAML1:
			bsoundSystem = 5;
			break;
		case ANA_SCAN_AUDIO_STD_BTSC:
			bsoundSystem = 6;
			break;
		default:
			bsoundSystem = 0;
			break;
	}
	return bsoundSystem;
}

void Factory_ATVChannelFrequecy(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stAtvChannelSetting.u32ChanFreq = FuncData->value;
		g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Frequency = FuncData->value;
		
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, ITEM_OFFSET(APP_SETTING_FactoryUser_t,
			SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Frequency),
			sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Frequency),
			&g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Frequency);
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, ITEM_OFFSET(APP_SETTING_FactoryUser_t,
			SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Frequency),
			sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Frequency));
		*/
	}
	else
	{
		if(g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Skip == 1
			|| g_s8CurChanIdx >= APP_FM_PRECHANNEL_NUMBER)
		{
			_FM_AtvChnSet_ResetValue();
		}
		else
		{
			stAtvChannelSetting.u32ChanFreq =
				g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Frequency;
			stAtvChannelSetting.u8AudioStd =
				_FM_AtvChnSet_AudioStdDB2OSD();
			stAtvChannelSetting.u8VideoStd =
				_FM_AtvChnSet_VideoStdDB2OSD();
			stAtvChannelSetting.u8SaveChannel = 0;
		}
		FuncData->value = stAtvChannelSetting.u32ChanFreq;
	}
}

void Factory_ATVAudioStandard(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stAtvChannelSetting.u8AudioStd  = FuncData->value;
	}
	else
	{
		FuncData->value = stAtvChannelSetting.u8AudioStd;
	}
}

void Factory_ATVVideoStandard(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stAtvChannelSetting.u8VideoStd = FuncData->value;
	}
	else
	{
		FuncData->value = stAtvChannelSetting.u8VideoStd;
	}
}
void Factory_ATVSaveCurrentChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stAtvChannelSetting.u8SaveChannel = FuncData->value;
		g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Frequency = stAtvChannelSetting.u32ChanFreq;
		g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].SoundSystem = _FM_AtvChnSet_AudioStdOSD2SaveDB();
		g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].ColorSystem = _FM_AtvChnSet_VideoStdOSD2SaveDB();
		g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Skip = 0;
		g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].AFC = 1;
		g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].ChannelNumber = g_s8CurChanIdx+1;

		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
						ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx]),
						(sizeof(ATVChannelSetting_t)),&(g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx]), sizeof(ATVChannelSetting_t));
		*/

		printf("Frequency[%d]=%d\n",g_s8CurChanIdx,g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Frequency);
		UpdateNodeFunctionContent(ID_FM_SySCon_Pre_ATV_Channel_Freq,0,0);
	}
	else
	{
		stAtvChannelSetting.u8SaveChannel = 0;
		FuncData->value = stAtvChannelSetting.u8SaveChannel;
	}
}
void Factory_ATVViewPreChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if (bSet)
	{
		if(g_s8CurChanIdx <= 0)
			return ;

		INT16 i=0;
		for(i=g_s8CurChanIdx-1; i>=0; i--)
		{
			if(g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[i].Skip == 1)
			{
				continue;
			}
			else
			{
				g_s8CurChanIdx = i;
				break;
			}
		}
		/*
		AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,0,
				sizeof(APP_SETTING_FactoryUser_t), &g_stFactoryUserData);
		*/
		stAtvChannelSetting.u32ChanFreq= g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Frequency;
		stAtvChannelSetting.u8AudioStd = _FM_AtvChnSet_AudioStdDB2OSD();
		stAtvChannelSetting.u8VideoStd= _FM_AtvChnSet_VideoStdDB2OSD();
		stAtvChannelSetting.u8SaveChannel = 0;
		UpdateNodeFunctionContent(ID_FM_SySCon_Pre_ATV_Channel_Freq,0,0);
	}
}
void Factory_ATVViewNextChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if (bSet)
	{
		if(g_s8CurChanIdx >= APP_FM_PRECHANNEL_NUMBER-1)
			return ;

		UINT8 i=0;
		for(i=g_s8CurChanIdx+1; i< APP_FM_PRECHANNEL_NUMBER; i++)
		{
			if(g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[i].Skip == 1)
			{
				continue;
			}
			else
			{
				g_s8CurChanIdx = i;
				break;
			}
		}
		stAtvChannelSetting.u32ChanFreq = g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Frequency;
		stAtvChannelSetting.u8AudioStd = _FM_AtvChnSet_AudioStdDB2OSD();
		stAtvChannelSetting.u8VideoStd= _FM_AtvChnSet_VideoStdDB2OSD();
		stAtvChannelSetting.u8SaveChannel = 0;
		UpdateNodeFunctionContent(ID_FM_SySCon_Pre_ATV_Channel_Freq,0,0);
	}
}
INT32 _FM_AtvChnSet_SaveValue(void)
{
//	if(g_s8CurChanIdx <0) comparison is always false due to limited range of data type [-Werror=type-limits]
//		return SP_ERR_FAILURE;

	g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Frequency = stAtvChannelSetting.u32ChanFreq;
	g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].SoundSystem = _FM_AtvChnSet_AudioStdOSD2SaveDB();
	g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].ColorSystem = _FM_AtvChnSet_VideoStdOSD2SaveDB();
	g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].Skip = 0;
	g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].AFC = 1;
	g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx].ChannelNumber = g_s8CurChanIdx+1;

	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
					ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx]),
					(sizeof(ATVChannelSetting_t)),&(g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx]));
	/*
	AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
		ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx]), sizeof(ATVChannelSetting_t));
	*/
	return SP_SUCCESS;
}

void Factory_ATVAddNewChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		INT16 i=0;
		UINT8 findchannel=0;
		for(i=APP_FM_PRECHANNEL_NUMBER-1; i>=0; i--)
		{
			if(g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[i].Skip == 1)
			{
				continue;
			}
			else
			{
				g_s8CurChanIdx = i;
				findchannel =true;
				break;
			}
		}
		if(findchannel == true)
		{
			g_s8CurChanIdx++;
		}
		else
		{
			g_s8CurChanIdx = 0;
		}
		_FM_AtvChnSet_ResetValue();
		_FM_AtvChnSet_SaveValue();
		UpdateNodeFunctionContent(ID_FM_SySCon_Pre_ATV_Channel_Freq,0,0);

		FuncData->nItem = 1;
	}else{
		FuncData->nItem = 0;

	}
}

typedef struct
{
	UINT32 u32ChanFreq;
	UINT8 u8BoundWidth;
	UINT8 u8SaveChannel;
	UINT8 u8DelChannel;
} APP_FM_DtvChnSet_t;

static APP_FM_DtvChnSet_t stDtvChnSet;
static UINT8 g_u8TotalNum = 0;

void Factory_DVBTChannelFrequecy(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stDtvChnSet.u32ChanFreq = FuncData->value;

		//[CC] Added
		g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].Frequency = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
						ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].Frequency),
						sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].Frequency),
						&(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].Frequency));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
						ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].Frequency),
						sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].Frequency));
		*/
	}
	else
	{
		g_u8TotalNum =  g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.TotalNum;
		stDtvChnSet.u32ChanFreq = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].Frequency;
		stDtvChnSet.u8BoundWidth = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].BoundWidth;
		stDtvChnSet.u8SaveChannel = 0;
		stDtvChnSet.u8DelChannel = 0;
		FuncData->value = stDtvChnSet.u32ChanFreq;
	}
}
void Factory_DVBTChannelBW(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stDtvChnSet.u8BoundWidth  = FuncData->value;

		//[CC] Added
		g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].BoundWidth = FuncData->value;

		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
						ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].BoundWidth),
						sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].BoundWidth),
						&(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].BoundWidth));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].BoundWidth),
						sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].BoundWidth));
		*/
	}
	else
	{
		FuncData->value = stDtvChnSet.u8BoundWidth;
	}
}
static Boolean _FM_DtvChnSet_CheakRepeatChn(UINT32 u32ChnFreq)
{
	UINT8 i = 0;
	Boolean Ret = FALSE;
	UINT32 u32TempChnFreq = 0;

	for(i=0;i<g_u8TotalNum;i++)
	{
		u32TempChnFreq = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[i].Frequency;
		if(u32TempChnFreq == u32ChnFreq)
		{
			if(i == g_s8CurChanIdx)
			{
				continue;
			}
			else
			{
				Ret = TRUE;
				break;
			}
		}
	}
    return Ret;
}
INT32 _FM_DtvChnSet_SaveValue(void)
{
	g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.TotalNum = g_u8TotalNum;
	g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].Frequency = stDtvChnSet.u32ChanFreq;
	g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].BoundWidth = stDtvChnSet.u8BoundWidth;

	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
					ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBTChannelSetting.TotalNum),
					(sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.TotalNum)),
					&(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.TotalNum));
	/*
	AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
		ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBTChannelSetting.TotalNum),
		sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.TotalNum));
	*/
	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
					ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx]),
					(sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx])),
					&(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx]));
	/*
	AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
		ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx]),
		sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx]));
	*/
    return SP_SUCCESS;
}
static INT32 _FM_DtvChnSet_ResetValue(void)
{
	stDtvChnSet.u32ChanFreq = 0;
	stDtvChnSet.u8BoundWidth = 0;
    return SP_SUCCESS;
}

void Factory_DVBTSaveCurrentChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		if(!_FM_DtvChnSet_CheakRepeatChn(stDtvChnSet.u32ChanFreq))
		{
			if((g_u8TotalNum < APP_FM_DVBTPRECHANNEL_NUMBER)
				&&(g_s8CurChanIdx >= g_u8TotalNum))
			{
				g_u8TotalNum++;
			}
			_FM_DtvChnSet_SaveValue();
			stDtvChnSet.u8SaveChannel = 1;
		}
		else
		{
			stDtvChnSet.u8SaveChannel = 2;
			if(g_s8CurChanIdx >= g_u8TotalNum)
			{
				_FM_DtvChnSet_ResetValue();
			}
			else
			{
				stDtvChnSet.u32ChanFreq = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].Frequency;
				stDtvChnSet.u8BoundWidth = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].BoundWidth;
			}
		}

		UpdateNodeFunctionContent(ID_FM_SySConf_Pre_DVBT_ChannelFreq,0,0);

		FuncData->value = stDtvChnSet.u8SaveChannel;
	}
	else
	{
		stDtvChnSet.u8SaveChannel = 0;
		FuncData->value = stDtvChnSet.u8SaveChannel;
	}

}
static UINT32 _FM_DtvChnSet_DeleteChn(UINT32 u32ChnIndex)
{
	UINT8 i = 0;
	UINT8 j = 0;
	DVBT_FM_PreChnValue_t ChnValue[APP_FM_DVBTPRECHANNEL_NUMBER];

	if((g_u8TotalNum <= 0)
		||(u32ChnIndex >= g_u8TotalNum))
	{
		return SP_ERR_FAILURE;
	}
	for(i=0;i<APP_FM_DVBTPRECHANNEL_NUMBER;i++)
	{
		ChnValue[i].Frequency = g_stSettingDefault_FactoryUser.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[i].Frequency;
		ChnValue[i].BoundWidth = g_stSettingDefault_FactoryUser.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[i].BoundWidth;
	}
	for(i=0;i<g_u8TotalNum;i++)
	{
		if(i == u32ChnIndex)
		{
			continue;
		}
		ChnValue[j].Frequency = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[i].Frequency;
		ChnValue[j].BoundWidth = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[i].BoundWidth;
		j++;
	}
	memcpy(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue,ChnValue,sizeof(ChnValue));
	g_u8TotalNum--;
	g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.TotalNum = g_u8TotalNum;

	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
		ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBTChannelSetting),
		sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting),&(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting));
	/*
	AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
		ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBTChannelSetting),
		sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting));
	*/
    return SP_SUCCESS;
}

void Factory_DVBTDelCurrentChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		if(_FM_DtvChnSet_DeleteChn(g_s8CurChanIdx) == SP_SUCCESS)
		{
			if(g_s8CurChanIdx > 0)
			{
				g_s8CurChanIdx--;
			}

			stDtvChnSet.u32ChanFreq = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].Frequency;
			stDtvChnSet.u8BoundWidth = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].BoundWidth;
			stDtvChnSet.u8DelChannel = 0;
		}
		else
		{
			stDtvChnSet.u8DelChannel = 2;
		}

		UpdateNodeFunctionContent(ID_FM_SySConf_Pre_DVBT_ChannelFreq,0,0);

	}
	else
	{
		stDtvChnSet.u8DelChannel = 0;

	}
	FuncData->value = stDtvChnSet.u8DelChannel;
}

void Factory_DVBTViewPreChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if (bSet)
	{
		if(g_u8TotalNum == 0)
			return ;

		if(g_s8CurChanIdx > 0)
		{
			g_s8CurChanIdx --;
		}

		stDtvChnSet.u32ChanFreq= g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].Frequency;
		stDtvChnSet.u8BoundWidth = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].BoundWidth;
		UpdateNodeFunctionContent(ID_FM_SySConf_Pre_DVBT_ChannelFreq,0,0);

	}
}
void Factory_DVBTViewNextChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if (bSet)
	{
		if(g_u8TotalNum == 0)
			return ;

		if(g_s8CurChanIdx < (g_u8TotalNum - 1))
		{
			g_s8CurChanIdx ++;
		}

		stDtvChnSet.u32ChanFreq= g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].Frequency;
		stDtvChnSet.u8BoundWidth = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[g_s8CurChanIdx].BoundWidth;
		UpdateNodeFunctionContent(ID_FM_SySConf_Pre_DVBT_ChannelFreq,0,0);

	}
}
void Factory_DVBTAddNewChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	if (bSet)
	{
		if(g_u8TotalNum >= APP_FM_DVBTPRECHANNEL_NUMBER)
		{
			return;
		}
		_FM_DtvChnSet_ResetValue();
		g_s8CurChanIdx = g_u8TotalNum;

		FuncData->nItem = 1;
		UpdateNodeFunctionContent(ID_FM_SySConf_Pre_DVBT_ChannelFreq,0,0);

	}else{
		FuncData->nItem = 0;
	}
}

typedef struct
{
	UINT32 u32ChanFreq;
	UINT16 u16SymbolRate;
	UINT8 u8Modulation;
	UINT8 u8SaveChannel;
	UINT8 u8DelChannel;
} APP_FM_Dvb_C_ChnSet_t;

static APP_FM_Dvb_C_ChnSet_t stDvb_C_ChnSet;

void Factory_DVBCChannelFrequecy(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stDvb_C_ChnSet.u32ChanFreq = FuncData->value;
		g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Frequency = stDvb_C_ChnSet.u32ChanFreq;

		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Frequency),
			sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Frequency),
			&(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Frequency));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Frequency),
			sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Frequency));
		*/

	}
	else
	{
		g_u8TotalNum =  g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.TotalNum;
		stDvb_C_ChnSet.u32ChanFreq = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Frequency;
		stDvb_C_ChnSet.u16SymbolRate = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].SymbolRate;
		stDvb_C_ChnSet.u8Modulation = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Modulation;
		stDvb_C_ChnSet.u8SaveChannel = 0;
		stDvb_C_ChnSet.u8DelChannel = 0;
		FuncData->value = stDvb_C_ChnSet.u32ChanFreq;
	}
}

void Factory_DVBCChannelSymbolRate(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stDvb_C_ChnSet.u16SymbolRate = FuncData->value;
		g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].SymbolRate = stDvb_C_ChnSet.u16SymbolRate;

		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].SymbolRate),
			sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].SymbolRate),
			&(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].SymbolRate));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].SymbolRate),
			sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].SymbolRate));
		*/
	}
	else
	{
		FuncData->value = stDvb_C_ChnSet.u16SymbolRate;
	}
}

void Factory_DVBCChannelModulation(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stDvb_C_ChnSet.u8Modulation = FuncData->value;
		g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Modulation = stDvb_C_ChnSet.u8Modulation;

		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Modulation),
			sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Modulation),
			&(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Modulation));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Modulation),
			sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Modulation));
		*/
	}
	else
	{
		FuncData->value = stDvb_C_ChnSet.u8Modulation;
	}
}
static Boolean _FM_Dvb_C_ChnSet_CheakRepeatChn(UINT32 u32ChnFreq)
{
	UINT8 i = 0;
	Boolean Ret = FALSE;
	UINT32 u32TempChnFreq = 0;

	for(i=0;i<g_u8TotalNum;i++)
	{
		u32TempChnFreq = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[i].Frequency;
		if(u32TempChnFreq == u32ChnFreq)
		{
			if(i == g_s8CurChanIdx)
			{
				continue;
			}
			else
			{
				Ret = TRUE;
				break;
			}
		}
	}
    return Ret;
}
INT32 _FM_Dvb_C_ChnSet_SaveValue(void)
{
	g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.TotalNum = g_u8TotalNum;
	g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Frequency = stDvb_C_ChnSet.u32ChanFreq;
	g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].SymbolRate = stDvb_C_ChnSet.u16SymbolRate;
	g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Modulation = stDvb_C_ChnSet.u8Modulation;

	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
					ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBCChannelSetting.TotalNum),
					(sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.TotalNum)),
					&(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.TotalNum));
	/*
	AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
		ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBCChannelSetting.TotalNum),
		sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.TotalNum));
	*/
	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
					ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx]),
					(sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx])),
					&(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx]));
	/*
	AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
		ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx]),
		sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx]));
	*/
    return SP_SUCCESS;
}
static INT32 _FM_Dvb_C_ChnSet_ResetValue(void)
{
	stDvb_C_ChnSet.u32ChanFreq = 0;
	stDvb_C_ChnSet.u16SymbolRate = 0;
	stDvb_C_ChnSet.u8Modulation = 2;
    return SP_SUCCESS;
}

void Factory_DVBCSaveCurrentChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		if(!_FM_Dvb_C_ChnSet_CheakRepeatChn(stDvb_C_ChnSet.u32ChanFreq))
		{
			if((g_u8TotalNum < APP_FM_DVBCPRECHANNEL_NUMBER)
				&&(g_s8CurChanIdx >= g_u8TotalNum))
			{
				g_u8TotalNum++;
			}
			_FM_Dvb_C_ChnSet_SaveValue();
			stDvb_C_ChnSet.u8SaveChannel = 1;
		}
		else
		{
			stDvb_C_ChnSet.u8SaveChannel = 2;
			if(g_s8CurChanIdx >= g_u8TotalNum)
			{
				_FM_Dvb_C_ChnSet_ResetValue();
			}
			else
			{
				stDvb_C_ChnSet.u32ChanFreq = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Frequency;
				stDvb_C_ChnSet.u16SymbolRate = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].SymbolRate;
				stDvb_C_ChnSet.u8Modulation = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Modulation;
			}
		}

		UpdateNodeFunctionContent(ID_FM_SysConf_Pre_DVBC_ChannelFreq,0,0);

	}
	else
	{
		stDvb_C_ChnSet.u8SaveChannel = 0;
	}
	FuncData->value = stDvb_C_ChnSet.u8SaveChannel;
}
static UINT32 _FM_Dvb_C_ChnSet_DeleteChn(UINT32 u32ChnIndex)
{
	UINT8 i = 0;

	if((g_u8TotalNum <= 0)
		||(u32ChnIndex >= g_u8TotalNum))
	{
		return SP_ERR_FAILURE;
	}

    {
        AL_RecHandle_t hProg = AL_DB_INVALIDHDL;
        UINT32  uiFreqK = 0;
		if (AL_SUCCESS == AL_DB_GetRecord(AL_DB_REQ_GETFIRST, AL_DBTYPE_DVB_C, AL_RECTYPE_DVBMULTIPLEX, &hProg))
		{
			do
			{
				if (AL_SUCCESS ==AL_DB_QueryDetailFieldByName(hProg, (al_uint8 *)"uiFreqK", (al_void *)&(uiFreqK)))
				{
                    if(uiFreqK == g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[u32ChnIndex].Frequency)
					{
						AL_DB_RemoveRecord(hProg);
						hProg = AL_DB_INVALIDHDL;
						break;
					}
				}
			} while(AL_DB_GetRecord(AL_DB_REQ_GETNEXT, AL_DBTYPE_DVB_C, AL_RECTYPE_DVBMULTIPLEX, &hProg) == AL_SUCCESS);
		}
    }

	for(i=u32ChnIndex;i<g_u8TotalNum;i++)
	{
		g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[i].Frequency
			= g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[i+1].Frequency;
		g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[i].SymbolRate
			= g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[i+1].SymbolRate;
		g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[i].Modulation
			= g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[i+1].Modulation;
	}
	g_u8TotalNum--;
	g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.TotalNum = g_u8TotalNum;
	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
		ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBCChannelSetting),
		sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting),&(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting));
	/*
	AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
		ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.PreChannelSetting.DVBCChannelSetting),
		sizeof(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting));
	*/
    return SP_SUCCESS;
}

void Factory_DVBCDelCurrentChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		if(_FM_Dvb_C_ChnSet_DeleteChn(g_s8CurChanIdx) == SP_SUCCESS)
		{
			if(g_s8CurChanIdx > 0)
			{
				g_s8CurChanIdx--;
			}
			/*
			AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,0,
					sizeof(APP_SETTING_FactoryUser_t), &g_stFactoryUserData);
			*/
			stDvb_C_ChnSet.u32ChanFreq = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Frequency;
			stDvb_C_ChnSet.u16SymbolRate = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].SymbolRate;
			stDvb_C_ChnSet.u8Modulation = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Modulation;
			stDvb_C_ChnSet.u8DelChannel = 0;
		}
		else
		{
			stDvb_C_ChnSet.u8DelChannel = 2;
		}
		UpdateNodeFunctionContent(ID_FM_SysConf_Pre_DVBC_ChannelFreq,0,0);
	}
	else
	{
		stDvb_C_ChnSet.u8DelChannel = 0;
	}
	FuncData->value = stDvb_C_ChnSet.u8DelChannel;
}
void Factory_DVBCViewPreChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if (bSet)
	{
		if(g_u8TotalNum == 0)
			return ;
		/*
		AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,0,
				sizeof(APP_SETTING_FactoryUser_t), &g_stFactoryUserData);
		*/
		if(g_s8CurChanIdx > 0)
		{
			g_s8CurChanIdx --;
		}
		stDvb_C_ChnSet.u32ChanFreq = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Frequency;
		stDvb_C_ChnSet.u16SymbolRate = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].SymbolRate;
		stDvb_C_ChnSet.u8Modulation = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Modulation;
		UpdateNodeFunctionContent(ID_FM_SysConf_Pre_DVBC_ChannelFreq,0,0);

	}
}
void Factory_DVBCViewNextChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if (bSet)
	{
		if(g_u8TotalNum == 0)
			return ;
		/*
		AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,0,
				sizeof(APP_SETTING_FactoryUser_t), &g_stFactoryUserData);
		*/
		if(g_s8CurChanIdx < (g_u8TotalNum - 1))
		{
			g_s8CurChanIdx ++;
		}

		stDvb_C_ChnSet.u32ChanFreq = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Frequency;
		stDvb_C_ChnSet.u16SymbolRate = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].SymbolRate;
		stDvb_C_ChnSet.u8Modulation = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[g_s8CurChanIdx].Modulation;
		UpdateNodeFunctionContent(ID_FM_SysConf_Pre_DVBC_ChannelFreq,0,0);

	}
}
void Factory_DVBCAddNewChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	if (bSet)
	{
		if(g_u8TotalNum >= APP_FM_DVBCPRECHANNEL_NUMBER)
		{
			return;
		}
		_FM_Dvb_C_ChnSet_ResetValue();
		g_s8CurChanIdx = g_u8TotalNum;
		FuncData->nItem = 1;
		UpdateNodeFunctionContent(ID_FM_SysConf_Pre_DVBC_ChannelFreq,0,0);

	}else{
		FuncData->nItem = 0;
	}
}

static UINT8 stInputSource[APP_SOURCE_MAX];
static al_uint32 Support_Input_Source_temp = 0;

static INT32 _FM_InputSource_ResetDefault(void)
{
	UINT8 i = 0;
	APP_Source_Type_t eSourType = APP_SOURCE_MAX;

	APP_GUIOBJ_Source_GetCurrSource(&eSourType);

	Support_Input_Source_temp = g_stSettingDefault_FactoryUser.SystemConfig.n_SysConf_Support_Input_Source;
	memset(stInputSource,0x0,sizeof(stInputSource));
	for(i=0;i<SUPPORT_SOURCE_TYPE_MAX;i++)
	{
		stInputSource[i] = (Support_Input_Source_temp&(1<<i))>>i;
	}
	AL_FLASH_SetSupportInputSource(g_stSettingDefault_FactoryUser.SystemConfig.n_SysConf_Support_Input_Source);
	if (!((Support_Input_Source_temp&(1<<eSourType))>>eSourType))
	{
		APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_DTV);
	}

	return SP_SUCCESS;
}

static INT32 _FM_InputSource_InitValue(void)
{
	memset(stInputSource,0x0,sizeof(stInputSource));
	Support_Input_Source_temp = AL_FLASH_GetSupportInputSource();

	UINT8 i = 0;
	for(i=0;i<SUPPORT_SOURCE_TYPE_MAX;i++)
	{
		stInputSource[i] = (Support_Input_Source_temp&(1<<i))>>i;
	}
	return SP_SUCCESS;
}

static INT32 _FM_InputSource_SaveValue(void)
{
	UINT8 i = 0;
	for(i=0;i<SUPPORT_SOURCE_TYPE_MAX;i++)
	{
		Support_Input_Source_temp = (Support_Input_Source_temp & ~(1<<i))|(stInputSource[i] << i);
	}

	AL_FLASH_SetSupportInputSource(Support_Input_Source_temp);	
    return SP_SUCCESS;
}
void Factory_InputSourceResetDefault(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	if (bSet)
	{
		_FM_InputSource_ResetDefault();
		APP_GOBJ_Source_Get_Support_InputSource_String();
		_FM_InputSource_SaveValue();
		UpdateNodeFunctionContent(ID_FM_SysConf_Input_InputSourceInput,0,0);

		FuncData->value = 1; // Show "Done"
	}
	else
	{
		_FM_InputSource_InitValue();
	}
}
void Factory_InputSourceATVOption(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_ATV - ID_FM_SysConf_Input_ATV] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_ATV - ID_FM_SysConf_Input_ATV];
	}
}
void Factory_InputSourceDTVOption(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_Source_DTV - ID_FM_SysConf_Input_ATV] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_Source_DTV - ID_FM_SysConf_Input_ATV];
	}
}
void Factory_InputSourceAVOption(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_AV - ID_FM_SysConf_Input_ATV] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_AV - ID_FM_SysConf_Input_ATV];
	}
}
void Factory_InputSourceAV2Option(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_AV2 - ID_FM_SysConf_Input_ATV] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_AV2 - ID_FM_SysConf_Input_ATV];
	}
}
void Factory_InputSourceAV3Option(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_AV3 - ID_FM_SysConf_Input_ATV] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_AV3 - ID_FM_SysConf_Input_ATV];
	}
}
void Factory_InputSourceSVideoOption(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_SVideo - ID_FM_SysConf_Input_ATV] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_SVideo - ID_FM_SysConf_Input_ATV];
	}
}
void Factory_InputSourceSVideo2Option(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_SVideo2 - ID_FM_SysConf_Input_ATV] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_SVideo2 - ID_FM_SysConf_Input_ATV];
	}
}
void Factory_InputSourceSVideo3Option(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_SVideo3 - ID_FM_SysConf_Input_ATV] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_SVideo3 - ID_FM_SysConf_Input_ATV];

	}
}

void Factory_InputSourceHDMIOption(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_Source_HDMI - ID_FM_SysConf_Input_ATV] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_Source_HDMI - ID_FM_SysConf_Input_ATV];
	}
}
void Factory_InputSourceHDMI2Option(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_Source_HDMI2 - ID_FM_SysConf_Input_ATV] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_Source_HDMI2 - ID_FM_SysConf_Input_ATV];
	}
}
void Factory_InputSourceHDMI3Option(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_Source_HDMI3 - ID_FM_SysConf_Input_ATV - 1] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_Source_HDMI3 - ID_FM_SysConf_Input_ATV - 1];
	}
}
void Factory_InputSourceMediaOption(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_Media - ID_FM_SysConf_Input_ATV - 1] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_Media - ID_FM_SysConf_Input_ATV - 1];
	}
}
void Factory_InputSourceSCARTOption(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_SCART - ID_FM_SysConf_Input_ATV - 1] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_SCART - ID_FM_SysConf_Input_ATV - 1];
	}
}
void Factory_InputSourceSCART2Option(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_Source_SCART2 - ID_FM_SysConf_Input_ATV - 1] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_Source_SCART2 - ID_FM_SysConf_Input_ATV - 1];
	}
}
void Factory_InputSourceYPbPrOption(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_YPbPr - ID_FM_SysConf_Input_ATV - 1] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_YPbPr - ID_FM_SysConf_Input_ATV - 1];
	}
}
void Factory_InputSourceYPbPr2Option(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_YPbPr2- ID_FM_SysConf_Input_ATV - 1] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_YPbPr2- ID_FM_SysConf_Input_ATV - 1];
	}
}
void Factory_InputSourceYPbPr3Option(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_YPbPr3 - ID_FM_SysConf_Input_ATV - 1] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_YPbPr3 - ID_FM_SysConf_Input_ATV - 1];
	}
}
void Factory_InputSourcePCOption(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_Source_PC - ID_FM_SysConf_Input_ATV - 1] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_Source_PC - ID_FM_SysConf_Input_ATV - 1];
	}
}
void Factory_InputSourceDVDOption(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_DVD - ID_FM_SysConf_Input_ATV - 1] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_DVD - ID_FM_SysConf_Input_ATV - 1];
	}
}
void Factory_InputSourceAndroidOption(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stInputSource[ID_FM_SysConf_Input_Android - ID_FM_SysConf_Input_ATV - 1] = FuncData->value;
	}
	else
	{
		FuncData->value = stInputSource[ID_FM_SysConf_Input_Android - ID_FM_SysConf_Input_ATV - 1];
	}
}

typedef struct
{
	UINT8 u8Ypbpr;
	UINT8 u8Ypbpr2;
	UINT8 u8Ypbpr3;
	UINT8 u8Hdmi;
	UINT8 u8Hdmi2;
	UINT8 u8Hdmi3;
	UINT8 u8Scart;
	UINT8 u8Scart2;
	UINT8 u8Av;
	UINT8 u8Av2;
	UINT8 u8Av3;
} APP_FM_PhysicalInputSetting_t;

static APP_FM_PhysicalInputSetting_t stPhysicalInputSetting;
static UINT16 g_u16SourceConfigTable_Size_New;
static APP_SourceConfig_t g_stSourceConfigTable_New[APP_SOURCE_MAX];

static INT32 _FM_PhysicalInputSetting_NewSourceMappingOldSource(APP_Source_Type_t CurrentSource,APP_Source_Type_t NewSourceTypeMappingOldSource)
{
	UINT8 u8Count_New = 0;
	UINT8 u8Count = 0;

	for (u8Count_New = 0; u8Count_New < g_u16SourceConfigTable_Size_New; u8Count_New++)
	{
		if(g_stSourceConfigTable_New[u8Count_New].eSourceType == CurrentSource)
		{
			for(u8Count = 0; u8Count < g_u16SourceConfigTable_Size; u8Count++)
			{
				if(g_stSourceConfigTable[u8Count].eSourceType == NewSourceTypeMappingOldSource)
				{
					memcpy(&g_stSourceConfigTable_New[u8Count_New].eMidSourceType, &g_stSourceConfigTable_New[u8Count].eMidSourceType, sizeof(InputSrc_t));
					APP_GUIOBJ_Source_SetSourceMapping_Table(g_stSourceConfigTable_New);
					return SP_SUCCESS;
				}
			}
		}
	}
	return SP_ERR_FAILURE;
}


static INT32 _FM_PhysicalInputSetting_SelectPhyInput_Init(UINT32 item)
{
	APP_Source_Type_t CurrentSource = APP_SOURCE_MAX;
	APP_Source_Type_t NewSourceTypeMappingOldSource = APP_SOURCE_MAX;
	switch(item)
	{
		case ID_FM_SysConf_Phy_YPbPr:
			CurrentSource = APP_SOURCE_YPBPR;
			NewSourceTypeMappingOldSource = APP_SOURCE_YPBPR + stPhysicalInputSetting.u8Ypbpr;
			break;
		case ID_FM_SysConf_Phy_YPbPr2:
			CurrentSource = APP_SOURCE_YPBPR1;
			NewSourceTypeMappingOldSource = APP_SOURCE_YPBPR + stPhysicalInputSetting.u8Ypbpr2;
			break;
		case ID_FM_SysConf_Phy_YPbPr3:
			CurrentSource = APP_SOURCE_YPBPR2;
			NewSourceTypeMappingOldSource = APP_SOURCE_YPBPR + stPhysicalInputSetting.u8Ypbpr3;
			break;
		case ID_FM_SysConf_Phy_HDMI:
			CurrentSource = APP_SOURCE_HDMI;
			NewSourceTypeMappingOldSource = APP_SOURCE_HDMI + stPhysicalInputSetting.u8Hdmi;
			break;
		case ID_FM_SysConf_Phy_HDMI2:
			CurrentSource = APP_SOURCE_HDMI1;
			NewSourceTypeMappingOldSource = APP_SOURCE_HDMI + stPhysicalInputSetting.u8Hdmi2;
			break;
		case ID_FM_SysConf_Phy_HDMI3:
			CurrentSource = APP_SOURCE_HDMI2;
			NewSourceTypeMappingOldSource = APP_SOURCE_HDMI + stPhysicalInputSetting.u8Hdmi3;
			break;
		case ID_FM_SysConf_Phy_SCART:
			CurrentSource = APP_SOURCE_SCART;
			NewSourceTypeMappingOldSource = APP_SOURCE_SCART + stPhysicalInputSetting.u8Scart;
			break;
		case ID_FM_SysConf_Phy_SCART2:
			CurrentSource = APP_SOURCE_SCART1;
			NewSourceTypeMappingOldSource = APP_SOURCE_SCART + stPhysicalInputSetting.u8Scart2;
			break;
		case ID_FM_SysConf_Phy_AV:
			CurrentSource = APP_SOURCE_AV;
			NewSourceTypeMappingOldSource = APP_SOURCE_AV + stPhysicalInputSetting.u8Av;
			break;
		case ID_FM_SysConf_Phy_AV2:
			CurrentSource = APP_SOURCE_AV1;
			NewSourceTypeMappingOldSource = APP_SOURCE_AV + stPhysicalInputSetting.u8Av2;
			break;
		case ID_FM_SysConf_Phy_AV3:
			CurrentSource = APP_SOURCE_AV2;
			NewSourceTypeMappingOldSource = APP_SOURCE_AV + stPhysicalInputSetting.u8Av3;
			break;
		default:
			break;
	}
	_FM_PhysicalInputSetting_NewSourceMappingOldSource(CurrentSource,NewSourceTypeMappingOldSource);
	return SP_SUCCESS;
}

static INT32 _FM_PhysicalInputSetting_ResetDefault(void)
{
	UINT32 i= 0;
	stPhysicalInputSetting.u8Ypbpr 	= g_stSettingDefault_FactoryUser.SystemConfig.PhyInputSourceSetting.n_SysConf_Phy_YPbPr;
	stPhysicalInputSetting.u8Ypbpr2 = g_stSettingDefault_FactoryUser.SystemConfig.PhyInputSourceSetting.n_SysConf_Phy_YPbPr2;
	stPhysicalInputSetting.u8Ypbpr3 = g_stSettingDefault_FactoryUser.SystemConfig.PhyInputSourceSetting.n_SysConf_Phy_YPbPr3;
	stPhysicalInputSetting.u8Hdmi	= g_stSettingDefault_FactoryUser.SystemConfig.PhyInputSourceSetting.n_SysConf_Phy_HDMI;
	stPhysicalInputSetting.u8Hdmi2	= g_stSettingDefault_FactoryUser.SystemConfig.PhyInputSourceSetting.n_SysConf_Phy_HDMI2;
	stPhysicalInputSetting.u8Hdmi3 	= g_stSettingDefault_FactoryUser.SystemConfig.PhyInputSourceSetting.n_SysConf_Phy_HDMI3;
	stPhysicalInputSetting.u8Scart	= g_stSettingDefault_FactoryUser.SystemConfig.PhyInputSourceSetting.n_SysConf_Phy_SCART;
	stPhysicalInputSetting.u8Scart2	= g_stSettingDefault_FactoryUser.SystemConfig.PhyInputSourceSetting.n_SysConf_Phy_SCART2;
	stPhysicalInputSetting.u8Av		= g_stSettingDefault_FactoryUser.SystemConfig.PhyInputSourceSetting.n_SysConf_Phy_AV;
	stPhysicalInputSetting.u8Av2 	= g_stSettingDefault_FactoryUser.SystemConfig.PhyInputSourceSetting.n_SysConf_Phy_AV2;
	stPhysicalInputSetting.u8Av3 	= g_stSettingDefault_FactoryUser.SystemConfig.PhyInputSourceSetting.n_SysConf_Phy_AV3;

	for(i=ID_FM_SysConf_Phy_YPbPr; i<= ID_FM_SysConf_Phy_AV3; i++)
		_FM_PhysicalInputSetting_SelectPhyInput_Init(i);

	AL_FLASH_SetPhysicalInputYPP(stPhysicalInputSetting.u8Ypbpr,0);
	AL_FLASH_SetPhysicalInputYPP2(stPhysicalInputSetting.u8Ypbpr2,0);
	AL_FLASH_SetPhysicalInputYPP3(stPhysicalInputSetting.u8Ypbpr3,0);
	AL_FLASH_SetPhysicalInputHDMI(stPhysicalInputSetting.u8Hdmi,0);
	AL_FLASH_SetPhysicalInputHDMI2(stPhysicalInputSetting.u8Hdmi2,0);
	AL_FLASH_SetPhysicalInputHDMI3(stPhysicalInputSetting.u8Hdmi3,0);
	AL_FLASH_SetPhysicalInputScart(stPhysicalInputSetting.u8Scart,0);
	AL_FLASH_SetPhysicalInputScart2(stPhysicalInputSetting.u8Scart2,0);
	AL_FLASH_SetPhysicalInputAV(stPhysicalInputSetting.u8Av,0);
	AL_FLASH_SetPhysicalInputAV2(stPhysicalInputSetting.u8Av2,0);
	AL_FLASH_SetPhysicalInputAV3(stPhysicalInputSetting.u8Av3,0);

   return SP_SUCCESS;
}
void Factory_PhysicalInputSettingYPP(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stPhysicalInputSetting.u8Ypbpr = FuncData->value;
		AL_FLASH_SetPhysicalInputYPP(stPhysicalInputSetting.u8Ypbpr,0);

		UpdateNodeFunctionContent(ID_FM_SysConf_Phy_ResetDefault,0,0);
	}
	else
	{
		FuncData->value = stPhysicalInputSetting.u8Ypbpr;
	}
}
void Factory_PhysicalInputSettingYPP2(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stPhysicalInputSetting.u8Ypbpr2 = FuncData->value;
		AL_FLASH_SetPhysicalInputYPP2(stPhysicalInputSetting.u8Ypbpr2,0);

		UpdateNodeFunctionContent(ID_FM_SysConf_Phy_ResetDefault,0,0);
	}
	else
	{
		FuncData->value = stPhysicalInputSetting.u8Ypbpr2;
	}
}
void Factory_PhysicalInputSettingYPP3(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stPhysicalInputSetting.u8Ypbpr3 = FuncData->value;
		AL_FLASH_SetPhysicalInputYPP3(stPhysicalInputSetting.u8Ypbpr3,0);

		UpdateNodeFunctionContent(ID_FM_SysConf_Phy_ResetDefault,0,0);
	}
	else
	{
		FuncData->value = stPhysicalInputSetting.u8Ypbpr3;
	}
}
void Factory_PhysicalInputSettingHDMI(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stPhysicalInputSetting.u8Hdmi= FuncData->value;
		AL_FLASH_SetPhysicalInputHDMI(stPhysicalInputSetting.u8Hdmi,0);

		UpdateNodeFunctionContent(ID_FM_SysConf_Phy_ResetDefault,0,0);
	}
	else
	{
		FuncData->value = stPhysicalInputSetting.u8Hdmi;
	}
}
void Factory_PhysicalInputSettingHDMI2(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stPhysicalInputSetting.u8Hdmi2= FuncData->value;
		AL_FLASH_SetPhysicalInputHDMI2(stPhysicalInputSetting.u8Hdmi2,0);

		UpdateNodeFunctionContent(ID_FM_SysConf_Phy_ResetDefault,0,0);
	}
	else
	{
		FuncData->value = stPhysicalInputSetting.u8Hdmi2;
	}
}
void Factory_PhysicalInputSettingHDMI3(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stPhysicalInputSetting.u8Hdmi3= FuncData->value;
		AL_FLASH_SetPhysicalInputHDMI3(stPhysicalInputSetting.u8Hdmi3,0);

		UpdateNodeFunctionContent(ID_FM_SysConf_Phy_ResetDefault,0,0);
	}
	else
	{
		FuncData->value = stPhysicalInputSetting.u8Hdmi3;
	}
}
void Factory_PhysicalInputSettingScart(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stPhysicalInputSetting.u8Scart = FuncData->value;
		AL_FLASH_SetPhysicalInputScart(stPhysicalInputSetting.u8Scart,0);

		UpdateNodeFunctionContent(ID_FM_SysConf_Phy_ResetDefault,0,0);
	}
	else
	{
		FuncData->value = stPhysicalInputSetting.u8Scart;
	}
}
void Factory_PhysicalInputSettingScart2(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stPhysicalInputSetting.u8Scart2= FuncData->value;
		AL_FLASH_SetPhysicalInputScart2(stPhysicalInputSetting.u8Scart2,0);

		UpdateNodeFunctionContent(ID_FM_SysConf_Phy_ResetDefault,0,0);
	}
	else
	{
		FuncData->value = stPhysicalInputSetting.u8Scart2;
	}
}
void Factory_PhysicalInputSettingAV(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stPhysicalInputSetting.u8Av= FuncData->value;
		AL_FLASH_SetPhysicalInputAV(stPhysicalInputSetting.u8Av,0);

		UpdateNodeFunctionContent(ID_FM_SysConf_Phy_ResetDefault,0,0);
	}
	else
	{
		FuncData->value = stPhysicalInputSetting.u8Av;
	}
}
void Factory_PhysicalInputSettingAV2(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stPhysicalInputSetting.u8Av2= FuncData->value;
		AL_FLASH_SetPhysicalInputAV2(stPhysicalInputSetting.u8Av2,0);

		UpdateNodeFunctionContent(ID_FM_SysConf_Phy_ResetDefault,0,0);
	}
	else
	{
		FuncData->value = stPhysicalInputSetting.u8Av2;
	}
}
void Factory_PhysicalInputSettingAV3(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stPhysicalInputSetting.u8Av3 = FuncData->value;
		AL_FLASH_SetPhysicalInputAV3(stPhysicalInputSetting.u8Av3,0);

		UpdateNodeFunctionContent(ID_FM_SysConf_Phy_ResetDefault,0,0);
	}
	else
	{
		FuncData->value = stPhysicalInputSetting.u8Av3;
	}
}

void Factory_PhyInputSetting(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if (bSet)
	{
		_FM_PhysicalInputSetting_ResetDefault();
		UpdateNodeFunctionContent(ID_FM_SysConf_Phy_ResetDefault,0,0);

		pValue->value = 1; // Show "Done"
	}
	else
	{
		APP_GUIOBJ_Source_SourceMapping_Table(g_stSourceConfigTable_New,
			&g_u16SourceConfigTable_Size_New);

		stPhysicalInputSetting.u8Ypbpr 	= AL_FLASH_GetPhysicalInputYPP();
		stPhysicalInputSetting.u8Ypbpr2 = AL_FLASH_GetPhysicalInputYPP2();
		stPhysicalInputSetting.u8Ypbpr3 = AL_FLASH_GetPhysicalInputYPP3();
		stPhysicalInputSetting.u8Hdmi	= AL_FLASH_GetPhysicalInputHDMI();
		stPhysicalInputSetting.u8Hdmi2	= AL_FLASH_GetPhysicalInputHDMI2();
		stPhysicalInputSetting.u8Hdmi3 	= AL_FLASH_GetPhysicalInputHDMI3();
		stPhysicalInputSetting.u8Scart	= AL_FLASH_GetPhysicalInputScart();
		stPhysicalInputSetting.u8Scart2	= AL_FLASH_GetPhysicalInputScart2();
		stPhysicalInputSetting.u8Av		= AL_FLASH_GetPhysicalInputAV();
		stPhysicalInputSetting.u8Av2 	= AL_FLASH_GetPhysicalInputAV2();
		stPhysicalInputSetting.u8Av3 	= AL_FLASH_GetPhysicalInputAV3();
	}
}

void Factory_UseCIPLUS12StackOnly(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		AL_FLASH_SetUseCIPLUS12StackOnly((UINT8 )FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)AL_FLASH_GetUseCIPLUS12StackOnly();
	}
}

void Factory_OSDLanguage_ResetDefault(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if (bSet)
	{
		Update_TypeVersion_Node("Start",(ID_FM_SysConf_OSD_ResetDefault - ID_FM_SysConf_OSD_ResetDefault));
		GUI_FUNC_CALL(GEL_UpdateOSD());

		_APP_GUIOBJ_FM_OSDLanguage_ResetDefault();
		int nIndex = 1;
		for( ; nIndex< Current_Node_Count; nIndex++)
			UpdateNodeFunctionContent(ID_FM_SysConf_OSD_ResetDefault, nIndex, 0);
		GUI_FUNC_CALL(GEL_UpdateOSD());

		// For push bottom, show final state
		FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
		FuncData->value = 1;
	}
}

void Factory_Language_Page1(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	al_uint32 OSD_Support_Language_temp = 0;
	if (bSet)
	{
		OSD_Support_Language_temp = AL_FLASH_GetOSDSupportLanguage();

		switch(nowNodeId)
		{
			case ID_FM_SysConf_OSD_German:
				stLangOnOffvalue1.u8German = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_German))|(stLangOnOffvalue1.u8German << SUPPORT_OSD_LANGUAGE_German);
				break;
			case ID_FM_SysConf_OSD_English:
				stLangOnOffvalue1.u8English = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_English))|(stLangOnOffvalue1.u8English << SUPPORT_OSD_LANGUAGE_English);
				break;
			case ID_FM_SysConf_OSD_French:
				stLangOnOffvalue1.u8French = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_French))|(stLangOnOffvalue1.u8French << SUPPORT_OSD_LANGUAGE_French);
				break;
			case ID_FM_SysConf_OSD_Italian:
				stLangOnOffvalue1.u8Italian = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Italian))|(stLangOnOffvalue1.u8Italian << SUPPORT_OSD_LANGUAGE_Italian);
				break;
			case ID_FM_SysConf_OSD_Polish:
				stLangOnOffvalue1.u8Polish = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Polish))|(stLangOnOffvalue1.u8Polish << SUPPORT_OSD_LANGUAGE_Polish);
				break;
			case ID_FM_SysConf_OSD_Spanish:
				stLangOnOffvalue1.u8Spanish = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Spanish))|(stLangOnOffvalue1.u8Spanish << SUPPORT_OSD_LANGUAGE_Spanish);
				break;
			case ID_FM_SysConf_OSD_Netherlands:
				stLangOnOffvalue1.u8Netherlands = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Netherlands))|(stLangOnOffvalue1.u8Netherlands << SUPPORT_OSD_LANGUAGE_Netherlands);
				break;
			case ID_FM_SysConf_OSD_Portuguese:
				stLangOnOffvalue1.u8Portuguese = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Portuguese))|(stLangOnOffvalue1.u8Portuguese << SUPPORT_OSD_LANGUAGE_Portuguese);
				break;
			case ID_FM_SysConf_OSD_Swedish:
				stLangOnOffvalue1.u8Swidish = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Swidish))|(stLangOnOffvalue1.u8Swidish << SUPPORT_OSD_LANGUAGE_Swidish);
				break;
			case ID_FM_SysConf_OSD_Finnish:
				stLangOnOffvalue1.u8Finnish = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Finnish))|(stLangOnOffvalue1.u8Finnish << SUPPORT_OSD_LANGUAGE_Finnish);
				break;
			case ID_FM_SysConf_OSD_Greek:
				stLangOnOffvalue1.u8Greek = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Greek))|(stLangOnOffvalue1.u8Greek << SUPPORT_OSD_LANGUAGE_Greek);
				break;
			default:
				break;
		}

		AL_FLASH_SetOSDSupportLanguage(OSD_Support_Language_temp);
	}
	else
	{
		OSD_Support_Language_temp = AL_FLASH_GetOSDSupportLanguage();

		stLangOnOffvalue1.u8German	    = (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_German))>>SUPPORT_OSD_LANGUAGE_German;
		stLangOnOffvalue1.u8English		= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_English))>>SUPPORT_OSD_LANGUAGE_English;
		stLangOnOffvalue1.u8French		= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_French))>>SUPPORT_OSD_LANGUAGE_French;
		stLangOnOffvalue1.u8Italian		= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Italian))>>SUPPORT_OSD_LANGUAGE_Italian;
		stLangOnOffvalue1.u8Polish		= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Polish))>>SUPPORT_OSD_LANGUAGE_Polish;
		stLangOnOffvalue1.u8Spanish		= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Spanish))>>SUPPORT_OSD_LANGUAGE_Spanish;
		stLangOnOffvalue1.u8Netherlands = (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Netherlands))>>SUPPORT_OSD_LANGUAGE_Netherlands;
		stLangOnOffvalue1.u8Portuguese 	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Portuguese))>>SUPPORT_OSD_LANGUAGE_Portuguese;
		stLangOnOffvalue1.u8Swidish		= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Swidish))>>SUPPORT_OSD_LANGUAGE_Swidish;
		stLangOnOffvalue1.u8Finnish		= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Finnish))>>SUPPORT_OSD_LANGUAGE_Finnish;
		stLangOnOffvalue1.u8Greek		= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Greek))>>SUPPORT_OSD_LANGUAGE_Greek;

		switch(nowNodeId)
		{
			case ID_FM_SysConf_OSD_German:
				FuncData->value = stLangOnOffvalue1.u8German;
				break;
			case ID_FM_SysConf_OSD_English:
				FuncData->value = stLangOnOffvalue1.u8English;
				break;
			case ID_FM_SysConf_OSD_French:
				FuncData->value = stLangOnOffvalue1.u8French;
				break;
			case ID_FM_SysConf_OSD_Italian:
				FuncData->value = stLangOnOffvalue1.u8Italian;
				break;
			case ID_FM_SysConf_OSD_Polish:
				FuncData->value = stLangOnOffvalue1.u8Polish;
				break;
			case ID_FM_SysConf_OSD_Spanish:
				FuncData->value = stLangOnOffvalue1.u8Spanish;
				break;
			case ID_FM_SysConf_OSD_Netherlands:
				FuncData->value = stLangOnOffvalue1.u8Netherlands;
				break;
			case ID_FM_SysConf_OSD_Portuguese:
				FuncData->value = stLangOnOffvalue1.u8Portuguese;
				break;
			case ID_FM_SysConf_OSD_Swedish:
				FuncData->value = stLangOnOffvalue1.u8Swidish;
				break;
			case ID_FM_SysConf_OSD_Finnish:
				FuncData->value = stLangOnOffvalue1.u8Finnish;
				break;
			case ID_FM_SysConf_OSD_Greek:
				FuncData->value = stLangOnOffvalue1.u8Greek;
				break;
			default:
				break;
		}
	}
}

void Factory_Language_Page2(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	al_uint32 OSD_Support_Language_temp = 0;

	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,0,
			sizeof(APP_SETTING_FactoryUser_t), &g_stFactoryUserData);

	if (bSet)
	{
		OSD_Support_Language_temp = AL_FLASH_GetOSDSupportLanguage();

		switch(nowNodeId)
		{
			case ID_FM_SysConf_OSD_Russian:
				stLangOnOffvalue2.u8Russian = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Russian))|(stLangOnOffvalue2.u8Russian << SUPPORT_OSD_LANGUAGE_Russian);
				break;
			case ID_FM_SysConf_OSD_Turkey:
				stLangOnOffvalue2.u8Turkey = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Turkey))|(stLangOnOffvalue2.u8Turkey << SUPPORT_OSD_LANGUAGE_Turkey);
				break;
			case ID_FM_SysConf_OSD_Danish:
				stLangOnOffvalue2.u8Danish = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Danish))|(stLangOnOffvalue2.u8Danish << SUPPORT_OSD_LANGUAGE_Danish);
				break;
			case ID_FM_SysConf_OSD_Norwegian:
				stLangOnOffvalue2.u8Norwegian = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Norwegian))|(stLangOnOffvalue2.u8Norwegian << SUPPORT_OSD_LANGUAGE_Norwegian);
				break;
			case ID_FM_SysConf_OSD_Hungarian:
				stLangOnOffvalue2.u8Hungarian = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Hungarian))|(stLangOnOffvalue2.u8Hungarian<< SUPPORT_OSD_LANGUAGE_Hungarian);
				break;
			case ID_FM_SysConf_OSD_Czech:
				stLangOnOffvalue2.u8Czech = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Czech))|(stLangOnOffvalue2.u8Czech<< SUPPORT_OSD_LANGUAGE_Czech);
				break;
			case ID_FM_SysConf_OSD_Slovakian:
				stLangOnOffvalue2.u8Slovakian = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Slovakian))|(stLangOnOffvalue2.u8Slovakian<< SUPPORT_OSD_LANGUAGE_Slovakian);
				break;
			case ID_FM_SysConf_OSD_Croatian:
				stLangOnOffvalue2.u8Croatian = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Croatian))|(stLangOnOffvalue2.u8Croatian<< SUPPORT_OSD_LANGUAGE_Croatian);
				break;
			case ID_FM_SysConf_OSD_Serbian:
				stLangOnOffvalue2.u8Serbian = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Serbian))|(stLangOnOffvalue2.u8Serbian<< SUPPORT_OSD_LANGUAGE_Serbian);
				break;
			case ID_FM_SysConf_OSD_Arabic:
				stLangOnOffvalue2.u8Arabic = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Arabic))|(stLangOnOffvalue2.u8Arabic<< SUPPORT_OSD_LANGUAGE_Arabic);
				break;
			case ID_FM_SysConf_OSD_Persian:
				stLangOnOffvalue2.u8Persian = FuncData->value;
				OSD_Support_Language_temp = (OSD_Support_Language_temp & ~(1<<SUPPORT_OSD_LANGUAGE_Persian))|(stLangOnOffvalue2.u8Persian<< SUPPORT_OSD_LANGUAGE_Persian);
				break;
			default:
				break;
		}


		AL_FLASH_SetOSDSupportLanguage(OSD_Support_Language_temp);
	}
	else
	{
		OSD_Support_Language_temp = AL_FLASH_GetOSDSupportLanguage();

		stLangOnOffvalue2.u8Russian	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Russian))>>SUPPORT_OSD_LANGUAGE_Russian;
		stLangOnOffvalue2.u8Turkey	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Turkey))>>SUPPORT_OSD_LANGUAGE_Turkey;
		stLangOnOffvalue2.u8Danish		= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Danish))>>SUPPORT_OSD_LANGUAGE_Danish;
		stLangOnOffvalue2.u8Norwegian	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Norwegian))>>SUPPORT_OSD_LANGUAGE_Norwegian;
		stLangOnOffvalue2.u8Hungarian	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Hungarian))>>SUPPORT_OSD_LANGUAGE_Hungarian;
		stLangOnOffvalue2.u8Czech		= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Czech))>>SUPPORT_OSD_LANGUAGE_Czech;
		stLangOnOffvalue2.u8Slovakian	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Slovakian))>>SUPPORT_OSD_LANGUAGE_Slovakian;
		stLangOnOffvalue2.u8Croatian	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Croatian))>>SUPPORT_OSD_LANGUAGE_Croatian;
		stLangOnOffvalue2.u8Serbian 	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Serbian))>>SUPPORT_OSD_LANGUAGE_Serbian;
		stLangOnOffvalue2.u8Arabic		= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Arabic))>>SUPPORT_OSD_LANGUAGE_Arabic;
		stLangOnOffvalue2.u8Persian 	= (OSD_Support_Language_temp&(1<<SUPPORT_OSD_LANGUAGE_Persian))>>SUPPORT_OSD_LANGUAGE_Persian;

		switch(nowNodeId)
		{
			case ID_FM_SysConf_OSD_Russian:
				FuncData->value = stLangOnOffvalue2.u8Russian;
				break;
			case ID_FM_SysConf_OSD_Turkey:
				FuncData->value = stLangOnOffvalue2.u8Turkey;
				break;			case ID_FM_SysConf_OSD_Danish:
				FuncData->value = stLangOnOffvalue2.u8Danish;
				break;
			case ID_FM_SysConf_OSD_Norwegian:
				FuncData->value = stLangOnOffvalue2.u8Norwegian;
				break;
			case ID_FM_SysConf_OSD_Hungarian:
				FuncData->value = stLangOnOffvalue2.u8Hungarian;
				break;
			case ID_FM_SysConf_OSD_Czech:
				FuncData->value = stLangOnOffvalue2.u8Czech;
				break;
			case ID_FM_SysConf_OSD_Slovakian:
				FuncData->value = stLangOnOffvalue2.u8Slovakian;
				break;
			case ID_FM_SysConf_OSD_Croatian:
				FuncData->value = stLangOnOffvalue2.u8Croatian;
				break;
			case ID_FM_SysConf_OSD_Serbian:
				FuncData->value = stLangOnOffvalue2.u8Serbian;
				break;
			case ID_FM_SysConf_OSD_Arabic:
				FuncData->value = stLangOnOffvalue2.u8Arabic;
				break;
			case ID_FM_SysConf_OSD_Persian:
				FuncData->value = stLangOnOffvalue2.u8Persian;
				break;
			default:
				break;
		}
	}
}
#ifdef SUPPORT_HKC_FACTORY_REMOTE
Boolean FM_RESET_FAC_REMOTE_GetStatus(void)
{
    return FAC_REMOTE_Flag;
}

/*****************************************************************************
** FUNCTION : DVBApp_SetOTAStatus
**
** DESCRIPTION :
**				set flag status
**
** PARAMETERS :
**				None
**
** RETURN VALUES:
**				None
*****************************************************************************/
void FM_RESET_FAC_REMOTE_SetStatus(Boolean b_flag)
{
    FAC_REMOTE_Flag = b_flag;
}
#endif
/*1. Factory Setting Start*/
 UINT32 Factory_FactSet_ResetAllEx(void)
{

	UINT32 dSysAppIdx = 0;
	APP_Source_Type_t eSourType = APP_SOURCE_MAX;
	APP_SYSSET_RESET_ID eResetParam = APP_SYSSET_RESET_DATABASE;
	APP_GUIOBJ_Source_GetCurrSource(&eSourType);

	if (MAIN_APP_SUCCESS == MAINAPP_GetActiveSystemAppIndex(&dSysAppIdx))
	{
		#ifdef CONFIG_ATV_SUPPORT
		if (dSysAppIdx == SYS_APP_ATV)
		{
			APP_Audio_SetMute(TRUE, FALSE, APP_MUTE_MODE_STATEMAX, eSourType);
			APP_Video_SetMute(TRUE, FALSE, APP_MUTE_MODE_STATEMAX, eSourType);

			SYSAPP_GOBJ_SendMsgToSingleGUIObject(SYS_APP_ATV, ATV_GUIOBJ_PLAYBACK,
				APP_ATV_INTRA_EVENT_STOP_PLAYBACK, 0);
		}
		#endif
		#ifdef CONFIG_DTV_SUPPORT
		if (dSysAppIdx == SYS_APP_DVB)
		{
			SYSAPP_GOBJ_SendMsgToSingleGUIObject(SYS_APP_DVB, DVB_GUIOBJ_PLAYBACK,
				APP_DVB_INTRA_EVENT_STOP_PLAYBACK, PLAYBACK_STOP_ALL);

			APP_Audio_SetMute(TRUE, FALSE, APP_MUTE_MODE_STATEMAX, eSourType);
			APP_Video_SetMute(TRUE, FALSE, APP_MUTE_MODE_STATEMAX, eSourType);
		}
		#endif
		#ifdef CONFIG_MEDIA_SUPPORT
		if ((dSysAppIdx == SYS_APP_FILE_PLAYER) &&
			(SYSAPP_GOBJ_GUIObjectExist(dSysAppIdx, MEDIA_GUIOBJ_POPMSG)))
		{
	        SYSAPP_GOBJ_DestroyGUIObject(dSysAppIdx, MEDIA_GUIOBJ_POPMSG);
		}
		#endif
	}
	else
	{
		return SP_ERR_FAILURE;
	}
	gIsFactoryResetting = TRUE;
	eResetParam = APP_SYSSET_RESET_DATABASE;
	APP_Sysset_Reset(eResetParam);
    /* destroy mute icon */
#ifdef TIANLE_Board_Time
	UINT32 BoardTime;
	/*Keep Some Setup not changed*/
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
		sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
	BoardTime = g_stSetupData.BoardTime;
#endif
	#ifdef TEAC_SYSTEMINFO_SUPPORT
	UINT32 PanelTime;
	UINT32 DVDTime;
	/*Keep Some Setup not changed*/
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
		sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
	PanelTime = g_stSetupData.PanelTime;
	DVDTime = g_stSetupData.DVDTime;
	#endif
	App_Data_UserSetting_Restore();
#ifdef TIANLE_Board_Time
	/*Keep Some Setup not changed*/
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
		sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
	g_stSetupData.BoardTime = BoardTime;
	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,0,
	 	sizeof(APP_SETTING_Setup_t),&(g_stSetupData));
	AL_Setting_Store(APP_Data_UserSetting_Handle(),SYS_SET_ID_SETUP, 0,
		sizeof(APP_SETTING_Setup_t));
#endif
	#ifdef TEAC_SYSTEMINFO_SUPPORT
	/*Keep Some Setup not changed*/
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
		sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
	g_stSetupData.PanelTime = PanelTime;
	g_stSetupData.DVDTime = DVDTime;
	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,0,
	 	sizeof(APP_SETTING_Setup_t),&(g_stSetupData));
	AL_Setting_Store(APP_Data_UserSetting_Handle(),SYS_SET_ID_SETUP, 0,
		sizeof(APP_SETTING_Setup_t));
	#endif
#ifdef SUPPORT_HIDE_TVSOURCE
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO, 0,
		sizeof(APP_SETTING_UserInfo_t), &(g_stUserInfoData));
	if(g_stUserInfoData.HideTVSource == 1)
	{
		g_stUserInfoData.SourceIndex = APP_SOURCE_AV;
		g_stUserInfoData.LastTVSource = APP_SOURCE_AV;
		g_stUserInfoData.AutoInstalled = 0;
	}
	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO,0,
	 	sizeof(APP_SETTING_UserInfo_t),&(g_stUserInfoData));
	AL_Setting_Store(APP_Data_UserSetting_Handle(),SYS_SET_ID_USERINFO, 0,
		sizeof(APP_SETTING_UserInfo_t));
#endif
	App_Data_UserSetting_FM_Restore();
	App_Data_UserSetting_ResetLangContry_ByFMDefaultValue();
	APP_Video_ResetTVSetting();
#ifdef SUPPORT_HKC_FACTORY_REMOTE	
  if(FM_RESET_FAC_REMOTE_GetStatus() == TRUE)
	{   
	    FM_RESET_FAC_REMOTE_SetStatus(FALSE);
		AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
		sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData));
		if(  g_stFactoryUserData.n_FactSet_FactoryRemote ==1)
		{
		g_stFactoryUserData.n_FactSet_FactoryRemote =0;
		g_stFactoryUserData.Function.n_Funct_PresetChannel=0;
		}					
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,0,
		sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData)); 
		AL_Setting_Store(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,
		sizeof(g_stFactoryUserData));
	}
#endif		
	// --------------------------------------------------------------------
	//Note: Need set some function again
	// 1) Show logo
	MID_TVFE_HideLogo( (BOOL)AL_FLASH_GetPowerShowLogo() );

	// 2) AC auto power on
	if(AL_FLASH_GetACAutoPowerOn()>0)
		MID_TVFE_SetAutoPowerOn(TRUE);
	else if(AL_FLASH_GetACAutoPowerOn()==0)
		MID_TVFE_SetAutoPowerOn(FALSE);

	// 3) Panel Inverce
	//MID_TVFE_SetPanelInverse(g_stFactoryUserData.Function.PanelSetting.n_FlipIndex);

	// 4) Reset panel set
	MID_TVFE_InitLVDS();
	tv_SetPanelIndex(0);	
    GL_TaskSleep(1900);
#if defined(SUPPORT_FACTORY_AUTO_TEST) && defined(CONFIG_CTV_UART_FAC_MODE)
	if(APP_Factory_GetUartFacModeOnOff()==TRUE)
	{
		con_AccuviewRs232_CMD_Response(CTV_RS232_CMD_FAC_RESSET_ALL, 0);
		GL_TaskSleep(50);
	}
#endif
	// --------------------------------------------------------------------
    GL_TaskSleep(50);
	// System reboot
	//APP_Sysset_Reset_System();
	MAINAPP_SendGlobalEvent(UI_EVENT_POWER, AL_POWER_STATE_OFF);
	
	if(APP_Factory_GetAutoTestOnOff() == TRUE)
	{
		extern void Enable_Debug_Message(UINT32 DBGStatus);
		Enable_Debug_Message(1<<MODULEID_UMF);
		printf("\n***RESET:OK***\n");
		GL_TaskSleep(50);
		Enable_Debug_Message(0);
		GL_TaskSleep(50);
	}
	//Direct power on after reboot
	tv_SetRebootAfterPowerOff();

	return SC_SUCCESS;
}
//ID_FM_FactSet_ResetAll,				//08
void Factory_FactSet_ResetAll(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	UINT8 Temp = gIsFactoryResetting;

	if (bSet)
	{
		//Version 2
		if(g_bResetAllFlag != FM_RESETALL_STOP)
		{
			return;
		}
		g_bResetAllFlag = FM_RESETALL_START;
		UpdateNodeFunctionContent(ID_FM_FactSet_Version,0,0);
		if(Factory_FactSet_ResetAllEx() == SP_SUCCESS)
		{
			DEBF("FAC Reset all SUCCESS!!!\n\n");
		}
		else
		{
			DEBF("FAC Reset all FAIL!!!\n\n");
			gIsFactoryResetting = Temp;
			g_bResetAllFlag = FM_RESETALL_FAIL;
			UpdateNodeFunctionContent(ID_FM_FactSet_Version,0,0);
		}
		g_bResetAllFlag = FM_RESETALL_STOP;
		#ifdef CELLO_BATHROOMTV
		g_bRealStandby = TRUE;
		#endif
	}
	else
	{
		pValue->value = g_bResetAllFlag;
	}
}

/*1. Factory Setting end*/

/*1. Version Start*/
/*
typedef enum {
	MID_TVFE_BootVer,
	MID_TVFE_8051Ver,
	MID_TVFE_AudioVer,
	MID_TVFE_KernelVer,
	MID_TVFE_AREAVer,
	MID_TVFE_CusSWVer,
	MID_TVFE_CusHWVer,
	MID_TVFE_FlashVer,
	MID_TVFE_OTAVer,
	MID_TVFE_CIKeyVer,
	MID_TVFE_BuildTimeVer,
	MID_TVFE_ChecksumVer,
	MID_TVFE_VIPTableVer,
} MID_TVFE_VersionItem;

*/

 void Factory_Ver_Debug(void)
{
	printf("\n\nStart Version\n");
	printf("===================================\n");
	printf("[BootRom Ver.]:%s\n",g_pKMFShareData->BootRomVersion);
	printf("[Audio ROM Ver.]:%s\n",g_pKMFShareData->AudioRomVersion);
	printf("[Kernel Ver.]:%s\n",g_pKMFShareData->KernelVersion);
	
	INT8 BuildTime_str[20]={0};
	memset(BuildTime_str,0x0,20);
	MID_TVFE_GetVersionInfo(MID_TVFE_BuildTimeVer, BuildTime_str);
	BuildTime_str[16]=0;//don't display second
	BuildTime_str[17]=0;
	BuildTime_str[18]=0;
	BuildTime_str[19]=0;
	
	char tmp_str[25]={0};
	if(g_stFactoryUserData.SystemConfig.n_SysConf_DefaultArea ==0)
	{
		sprintf(tmp_str,"%s","TaiWan");
	}
	else if(g_stFactoryUserData.SystemConfig.n_SysConf_DefaultArea ==3)
	{
		sprintf(tmp_str,"%s","Middle East");
	}
	else if(g_stFactoryUserData.SystemConfig.n_SysConf_DefaultArea ==2)
	{
		sprintf(tmp_str,"%s","Eurpoe");
	}
	else
	{
		sprintf(tmp_str,"%s","China");
	}
	printf("[AREA Option]:%s\n",tmp_str);
	printf("[Customer HW Ver.]:%s\n",CV_CustomrBoardString[g_stFactoryUserData.SystemConfig.n_SysConf_BoardDefined]);

	memset(&tmp_str, 0, 25*sizeof(char));
	//MID_TVFE_GetVersionInfo(MID_TVFE_CusSWVer, tmp_str);
	printf("[Customer SW Ver.]:%s\n",CUSTOMER_SWVER);

	memset(&tmp_str, 0, 25*sizeof(char));
	MID_TVFE_GetVersionInfo(MID_TVFE_FlashVer, tmp_str);
	printf("[Flash Type]:%s\n",tmp_str);

	memset(&tmp_str, 0, 25*sizeof(char));
	MID_TVFE_GetVersionInfo(MID_TVFE_OTAVer, tmp_str);
	printf("[OTA version]:%s\n",tmp_str);

	memset(&tmp_str, 0, 25*sizeof(char));
#ifdef CONFIG_CIPLUS_SUPPORT
	if(TRUE == MID_DTVCI_IsValidKey())
	{
		UINT8 key_id[8];
		INT32 i;
		MID_DTVCI_GetKeyID(key_id);
		for(i=0;i<8;i++)
		{
			sprintf(tmp_str+(i*2),"%02X",key_id[i]);
		}		
	}
	else
	{
		sprintf(tmp_str, "%s", "InValid Key");
	}

#else
	sprintf(tmp_str,"%s","(None)");
#endif
	printf("[CI Key Host]:%s\n",tmp_str);

	memset(&tmp_str, 0, 25*sizeof(char));
#ifdef CONFIG_CIPLUS_SUPPORT
	if(TRUE == MID_DTVCI_IsValidKey())
	{
		UINT32 SerialNo;
		MID_DTVCI_GetKeySerialNo(&SerialNo);
		sprintf(tmp_str,"%08d",SerialNo);
	}
	else
	{
		sprintf(tmp_str, "%s", "None");
	}
#else
	sprintf(tmp_str,"%s","(None)");
#endif
	printf("[CI Key S.N.]:%s\n",tmp_str);

	char versionInfo[40];
	UINT32 checksum = (UINT32)MID_KEYUPDATE_HDCP_Get_Flash_Checksum();
	sprintf(versionInfo,"%x",checksum);
	printf("[Check Sum]:%s\n",versionInfo);

	printf("[VIP Table Ver.]:%s\n",g_pKMFShareData->VIPTableVersion);
#ifdef SUPPORT_HDMI_ARC
	printf("[ARC]: SUPPORT ARC  \n");
#else
	printf("[ARC]: UNSUPPORT ARC  \n");
#endif
#ifdef CONFIG_HDMI_SUPPORT_MHL
    printf("[MHL]: SUPPORT MHL  \n");
#else
    printf("[MHL]: UNSUPPORT MHL  \n");
#endif
#if ((CONFIG_AUDIO_FEATURE & 0x10) == 0x10)
    printf("[AC3]: SUPPORT AC3  \n");
#else
    printf("[AC3]: UNSUPPORT AC3  \n");
#endif
#ifdef CONFIG_CIPLUS_SUPPORT
    printf("[CI PLUS]: SUPPORT CI PLUS  \n");
#else
    printf("[CI PLUS}: UNSUPPORT CI PLUS  \n");
#endif

	printf("===================================\n");
	printf("End Version\n\n");
	printf("\n\nstart test\n");
	printf("[version]%s[end]\n",BuildTime_str);
	printf("[board]%s[end] \n\n",CV_CustomrBoardString[g_stFactoryUserData.SystemConfig.n_SysConf_BoardDefined]);


}

#ifdef TIANLE_Board_Time
void Factory_Ver_BoardTimer(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	char tmp_str[40]={0};
	UINT32 usedtime;
 	UINT32 usedhour;
 	UINT32 usedmin;
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,0,
		sizeof(APP_SETTING_Setup_t), &g_stSetupData);
	usedtime = g_stSetupData.BoardTime;
	usedhour = usedtime/3600;
	usedmin = usedtime%3600/60;
	sprintf(tmp_str, "%dHour:%dMin", usedhour, usedmin);
	if(bSet == FALSE )
	{
		Update_TypeVersion_Node(tmp_str, pValue->nItem);
	}
	
}
#else
//	ID_FM_Ver_BootRomVer,				//14
void Factory_Ver_BootRomVer(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	#ifdef CELLO_VERSION
	char * cFactoryVersionInfo = "07_15 C32227T2 Wakeup";//for wake up
	
	if(bSet==FALSE)
	{
		Update_TypeVersion_Node(cFactoryVersionInfo, pValue->nItem);
	}
	#else
	if(bSet==FALSE)
	{
		Update_TypeVersion_Node((char * )g_pKMFShareData->BootRomVersion, pValue->nItem);
	}
    #endif
}
#endif
//	ID_FM_Ver_8051ROMVer,				//15
void Factory_Ver_8051ROMVer(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if(bSet==FALSE  )
	{
		Update_TypeVersion_Node((char * ) g_pKMFShareData->_8051RomVersion, pValue->nItem);
	}

}
//	ID_FM_Ver_AudioRomVer,				//16
void Factory_Ver_AudioRomVer(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if(bSet==FALSE)
	{
		Update_TypeVersion_Node((char * ) g_pKMFShareData->AudioRomVersion, pValue->nItem);
	}
}
void Factory_Ver_TunerType(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	char tunertype[20] = {0}; 
	memset(tunertype,0x0,0x20);
	APP_Factory_GetCurrentTunerName(tunertype);
	if(bSet==FALSE)
	{
		Update_TypeVersion_Node(tunertype, pValue->nItem);
	}
}

//	ID_FM_Ver_KernelVer,					//17
void Factory_Ver_KernelVer(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	//printf("[Gordon] %s %d %s \n",__func__,__LINE__,(&tmp_str));
	if(bSet==FALSE )
	{
		Update_TypeVersion_Node((char * ) g_pKMFShareData->KernelVersion, pValue->nItem);
	}

}

//	ID_FM_Ver_PanelInfo,					//17
void Factory_Ver_PanelInfo(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
    UINT8 len = 0;
	UINT8 u8PanelIdx = tv_GetPanelIndexFromRegister();
	char *PanelName = NULL;
	char *PanelConfig;
	int i;
	
	len = strlen(CONFIG_PANEL_TYPE_NAME)+1;
	PanelConfig = (char*)alloca(len);
	if(NULL != PanelConfig)
	{
		memset(PanelConfig,0x0,len);
		memcpy(PanelConfig,CONFIG_PANEL_TYPE_NAME,len-1);
	}
	
	for(i=0;i<CONFIG_PANEL_NUM;i++)
	{
		if(i < CONFIG_PANEL_NUM - 1)
		{
			PanelName = strsep(&PanelConfig," ");
			while(*PanelConfig == ' ')
				PanelConfig++;
		}
		if(i == CONFIG_PANEL_NUM - 1)
		{
			PanelName = strsep(&PanelConfig,"\0");
		}
		if(i==u8PanelIdx)
			break;
	}

	if (!PanelName)
	{
		PanelName = "Don't Found";
	}
	if(bSet==FALSE )
	{
		Update_TypeVersion_Node((char * ) PanelName, pValue->nItem);
	}
}
/*
//	ID_FM_Ver_AREAOption,				//18
void Factory_Ver_AREAOption(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	INT8 tmp_str[15]={0};
	if(g_stFactoryUserData.SystemConfig.n_SysConf_DefaultArea ==0)
	{
		sprintf(tmp_str,"%s","TaiWan");
	}
	else if(g_stFactoryUserData.SystemConfig.n_SysConf_DefaultArea ==3)
	{
		sprintf(tmp_str,"%s","Middle East");
	}
	else if(g_stFactoryUserData.SystemConfig.n_SysConf_DefaultArea ==2)
	{
		sprintf(tmp_str,"%s","Eurpoe");
	}
	else
	{
		sprintf(tmp_str,"%s","China");
	}

	if(bSet==FALSE)
	{
		Update_TypeVersion_Node(tmp_str, pValue->nItem);
	}

}
*/
//	ID_FM_Ver_CustomerHWVer,			//19
 void Factory_Ver_CustomerHWVer(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if(bSet==FALSE)
	{
		Update_TypeVersion_Node(CV_CustomrBoardString[g_stFactoryUserData.SystemConfig.n_SysConf_BoardDefined], pValue->nItem);
	}

}
//	ID_FM_Ver_CustomerSWVer,			//20
void Factory_Ver_CustomerSWVer(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	INT8 tmp_str[6]={0};
	MID_TVFE_GetVersionInfo(MID_TVFE_CusSWVer, tmp_str);
	if(bSet==FALSE)
	{
		#ifdef SUPPORT_SFU_AUTO_TEST
		Update_TypeVersion_Node(tmp_str, pValue->nItem);
		#else
		Update_TypeVersion_Node(CUSTOMER_SWVER, pValue->nItem);
		#endif
	}

}
//	ID_FM_Ver_Flashtype,					//21
void Factory_Ver_Flashtype(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	INT8 tmp_str[25]={0};
	MID_TVFE_GetVersionInfo(MID_TVFE_FlashVer, tmp_str);
	if(bSet==FALSE)
	{
		Update_TypeVersion_Node(tmp_str, pValue->nItem);
	}
}
//	ID_FM_Ver_OTAVersion,				//22
void Factory_Ver_OTAVersion(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	INT8 tmp_str[4]={0};
	MID_TVFE_GetVersionInfo(MID_TVFE_OTAVer, tmp_str);
	if(bSet==FALSE)
	{
		Update_TypeVersion_Node(tmp_str, pValue->nItem);
	}

}
//	ID_FM_Ver_CIKey,					//23
void Factory_Ver_CIKey(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
#ifdef CONFIG_CIPLUS_SUPPORT
	if((TRUE == MID_DTVCI_IsValidKey()) && (TRUE != MID_DTVCI_IsProductKey()))
	{
		Update_TypeVersion_Node("Test Key", pValue->nItem);
	}
	else
	{
		INT8 tmp_str[16]={0};
		if(TRUE == MID_DTVCI_IsValidKey())
		{
			UINT8 key_id[8];
			INT32 i;
			MID_DTVCI_GetKeyID(key_id);
			for(i=0;i<8;i++)
			{
				sprintf(tmp_str+(i*2),"%02X",key_id[i]);
			}
		}
		else
		{
			sprintf(tmp_str, "%s", "InValid Key");
		}
		if(bSet==FALSE)
		{
			Update_TypeVersion_Node(tmp_str, pValue->nItem);
		}
	}
#else
	if(bSet==FALSE)
	{
		Update_TypeVersion_Node("UnSupport CI plus", pValue->nItem);
	}
#endif
}

void Factory_Ver_SerialNum(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	// TODO: Need UMF function
#ifdef CONFIG_CIPLUS_SUPPORT
	INT8 tmp_str[11]={0};
	if(TRUE == MID_DTVCI_IsValidKey())
	{
		UINT32 SerialNo;
		MID_DTVCI_GetKeySerialNo(&SerialNo);
		sprintf(tmp_str,"%08d",SerialNo);
	}
	else
	{
		sprintf(tmp_str, "%s", "None");
	}
	if(bSet==FALSE)
	{
		Update_TypeVersion_Node(tmp_str, pValue->nItem);
	}
#else
	if(bSet==FALSE)
	{
		Update_TypeVersion_Node("None", pValue->nItem);
	}
#endif

}

//#define SHOW_HDCPKEY_SKV

void Factory_Ver_HDCP(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	char tmp_str[40]={0};
	if(MID_KEYUPDATE_HDCP_IsKeyValid() == TRUE)
	{
			#ifdef CONFIG_CTV_UART_FAC_MODE
			UINT8 i = 0;
			UINT8 HDCPKSV[5+1] = {0} ;
			memset((void*)tmp_str,0x0,32);
			memset((void*)HDCPKSV,0x0,6);
			Cmd_Hdmi_ReadHDCPKSV((unsigned char *)HDCPKSV);
			//printf("\n\nksv:\n");
			for(i=0;i<5;i++)
			{
				sprintf(tmp_str+(i*2),"%02X",HDCPKSV[i]);
				//printf("%02x ",HDCPKSV[i]);
			}
			//printf("\nksv end!!\n");			
		#elif defined(CONFIG_APPEND_NAME_TO_CIKEY_HDCPKEY)
			char Name[32]={0};
			MID_KEYUPDATE_HDCP_Get_KeyFileName(Name);
			memset((void*)tmp_str,0x0,32);
			if(0)//(strstr(Name, CONFIG_HDCPKEY_PREFIX) != NULL)
			{
				memcpy((void*)tmp_str,(void*)Name+strlen(CONFIG_HDCPKEY_PREFIX), strlen(Name) - strlen(CONFIG_HDCPKEY_PREFIX)- 4);
			}
			else
			{
				memcpy((void*)tmp_str,(void*)Name, 32);
			}
		#else
			sprintf(tmp_str,"%s","Vaild Key");
		#endif
	}else{
		sprintf(tmp_str,"%s","InValid Key");
	}
		
	if(bSet == FALSE )
	{
		Update_TypeVersion_Node(tmp_str, pValue->nItem);
	}
}

//	ID_FM_Ver_CheckSum,					//25
void Factory_Ver_CheckSum(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	char versionInfo[40];
	UINT32 checksum = (UINT32)MID_KEYUPDATE_HDCP_Get_Flash_Checksum();
	sprintf(versionInfo,"%x",checksum);
	if(bSet==FALSE)
	{
		Update_TypeVersion_Node(versionInfo, pValue->nItem);
	}


}
//	ID_FM_Ver_VIPTableVer,				//26
void Factory_Ver_VIPTableVer(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if(bSet==FALSE )
	{
		Update_TypeVersion_Node((char * ) g_pKMFShareData->VIPTableVersion, pValue->nItem);
	}
}

//	ID_FM_Ver_Builder,				//27
void Factory_Ver_Builder(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	INT8 tmp_str[20]={0};
	memset(tmp_str,0x0,20);
	MID_TVFE_GetVersionInfo(MID_TVFE_Builder, tmp_str);
	if(bSet==FALSE)
	{
		Update_TypeVersion_Node(tmp_str, pValue->nItem);
	}
}

//	ID_FM_Ver_BuildTime,
void Factory_Ver_BuildTime(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
    #ifdef CONFIG_CTV_UART_FAC_MODE
	INT8 tmp_str[20]={0};
	INT8 tmp_str1[20]={0};
	UINT8 i=0;
	UINT8 j=0;
	memset(tmp_str,0x0,20);
	memset(tmp_str1,0x0,20);
	#ifdef	DEFAULT_CUSTOMER_VER
	memcpy(tmp_str, DEFAULT_CUSTOMER_BuildTimeVer, sizeof(DEFAULT_CUSTOMER_BuildTimeVer));
	#else
	MID_TVFE_GetVersionInfo(MID_TVFE_BuildTimeVer, tmp_str);
	#endif
	for (i=0; i<20; i++)
	{
		if((i==4)||(i==7||(i==10)||(i==13)))
		{
			//DEBF("for factory test BuildTime display need remove : -\n");
		}
		else
		{
				
			tmp_str1[j] = tmp_str[i];
			j++;
		}					
	}
	
	tmp_str[16]=0;//don't display second
	tmp_str[17]=0;
	tmp_str[18]=0;
	tmp_str[19]=0;
	if(bSet==FALSE)
	{
		Update_TypeVersion_Node(tmp_str1, pValue->nItem);
	}
	#else
	INT8 tmp_str[20]={0};
	memset(tmp_str,0x0,20);
	MID_TVFE_GetVersionInfo(MID_TVFE_BuildTimeVer, tmp_str);
	tmp_str[16]=0;//don't display second
	tmp_str[17]=0;
	tmp_str[18]=0;
	tmp_str[19]=0;
	if(bSet==FALSE)
	{
		Update_TypeVersion_Node(tmp_str, pValue->nItem);
	}
	#endif
}

/*1. Version End*/


/*2.1 start*/
//	ID_FM_SysConf_BoardDefined,				//27
void Factory_SysConf_BoardDefined(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{

	if(bSet==FALSE)
	{
		Update_TypeVersion_Node(CV_CustomrBoardString[AL_FLASH_GetBoardDefined()], pValue->nItem);
	}

}
/*2.1  end*/

/*2.2 start*/
//	ID_FM_SysConf_DefaultLanguage,			//28
void Factory_SysConf_DefaultLanguage(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	UINT8 i = 0;
	String_id_t StringList[25] = {STRING_LAST};
	String_id_t StringId = STRING_LAST;
	MENU_SLIDER_STATICSTRS *pContent = (MENU_SLIDER_STATICSTRS *)Fun_SysConf_DefaultLanguage.leaves;
	UINT32 OSD_Language_Number = 0;
	OSD_Language_Number = 0;
	APP_GUIOBJ_MainMenu_Get_Support_OSD_Language_String(&OSD_Language_Number,StringList);
	if(pContent->str)
	{
		pContent->str = NULL;
	}
	if(pContent->str == NULL)
	{
		pContent->str = (UINT32 *)malloc((OSD_Language_Number+1)*sizeof(UINT32));
		memcpy(pContent->str,StringList,(OSD_Language_Number+1)*sizeof(UINT32));
	}
	if (bSet)
	{
		if(FuncData->value >= Fun_SysConf_DefaultLanguage.nItems)
		{
			FuncData->value = 0;
		}
		StringId = pContent->str[(UINT32)FuncData->value];
		AL_FLASH_SetDefaultLanguage(APP_GUIOBJ_StringID_MappingTo_OSD_Language(StringId));
	}
	else
	{
		// UMF get function
		/*
		AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,0,
				sizeof(APP_SETTING_FactoryUser_t), &g_stFactoryUserData);
		*/
		StringId = STRING_LAST;
		StringId = APP_GUIOBJ_OSD_Language_MappingTo_StringID(AL_FLASH_GetDefaultLanguage());
		for(i=0;i<OSD_Language_Number;i++)
		{
			if(StringId == (String_id_t)pContent->str[i])
			{
				break;
			}
		}
		if(i < OSD_Language_Number)
		{
			FuncData->value = i;
		}
		else
		{
			FuncData->value = 0;
		}
		Fun_SysConf_DefaultLanguage.nItems = OSD_Language_Number;
		pContent->nMin = 0;
		pContent->nMax = Fun_SysConf_DefaultLanguage.nItems - 1;
		pContent->step = 1;
	}
}
/*2.2  end*/

/*2.3 start*/
//	ID_FM_SysConf_DefaultCountry,			//29
void Factory_SysConf_DefaultCountry(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	MENU_SLIDER_STATICSTRS *pContent = (MENU_SLIDER_STATICSTRS *)Fun_SysConf_DefaultCountry.leaves;
	if (bSet)
	{
		if (FuncData->value >= Fun_SysConf_DefaultCountry.nItems)
		{
			FuncData->value = 0;
		}
		AL_FLASH_SetDefaultCountry((UINT8)(APP_GUIOBJ_Channel_GetAreaBySupportCountryIdx((UINT32) FuncData->value)));
	}
	else
	{
		// UMF get function
		Fun_SysConf_DefaultCountry.nItems = APP_GUIOBJ_Channel_GetCountryNum();
		pContent->str = APP_GUIOBJ_Channel_GetCountryTbl();
		pContent->nMin = 0;
		pContent->nMax = Fun_SysConf_DefaultCountry.nItems - 1;
		pContent->step = 1;
		FuncData->value = (INT32)(APP_GUIOBJ_Channel_GetSupportCountryIdxByArea(AL_FLASH_GetDefaultCountry()));
		if (FuncData->value >= Fun_SysConf_DefaultCountry.nItems)
		{
			FuncData->value = 0;
		}
	}
}
/*2.3  end*/
const APP_Source_Type_t SysConf_DVDAndroidInputSource_MapTab[]=
{
	APP_SOURCE_DVD,
	APP_SOURCE_ANDRO,
	APP_SOURCE_AV,
	APP_SOURCE_AV1,
	APP_SOURCE_AV2,
	APP_SOURCE_YPBPR,
	APP_SOURCE_YPBPR1,
	APP_SOURCE_YPBPR2,
	APP_SOURCE_HDMI,
	APP_SOURCE_HDMI1,
	APP_SOURCE_HDMI2,
};

const UINT16 SysConf_DVDAndroidInputSource_MapTab_Size = sizeof(SysConf_DVDAndroidInputSource_MapTab) / sizeof(APP_Source_Type_t);
/*2.5.1 start*/
//	ID_FM_SysConf_DVD_DVDInputPort,			//29
void Factory_SysConf_DVD_DVDInputPort(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	UINT8 i = 0;
	UINT8 u8Count = 0;
	UINT8 u8TempCount = 0;
	APP_Source_Type_t SourceType = APP_SOURCE_DVD;
	String_id_t StringList[APP_SOURCE_MAX] = {STRING_LAST};
	APP_SourceConfig_t SourceConfigTable_New[APP_SOURCE_MAX];
	UINT16 u16SourceConfigTable_Size_New;
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	MENU_SLIDER_STATICSTRS *pContent = (MENU_SLIDER_STATICSTRS *)Fun_SysConf_DVD_DVDInputPort.leaves;
	APP_GUIOBJ_Source_SourceMapping_Table(SourceConfigTable_New,&u16SourceConfigTable_Size_New);
	if (bSet)
	{
		if (FuncData->value >= SysConf_DVDAndroidInputSource_MapTab_Size)
		{
			FuncData->value = 0;
		}
		for (u8Count = 0; u8Count < u16SourceConfigTable_Size_New; u8Count++)
		{
			if(SourceConfigTable_New[u8Count].eSourceType
				== SysConf_DVDAndroidInputSource_MapTab[(UINT32)FuncData->value])
			{
				for(u8TempCount = 0; u8TempCount < u16SourceConfigTable_Size_New; u8TempCount++)
				{
					if(SourceConfigTable_New[u8TempCount].eSourceType == SourceType)
					{
						memcpy(&SourceConfigTable_New[u8TempCount].eMidSourceType, &SourceConfigTable_New[u8Count].eMidSourceType, sizeof(InputSrc_t));
						APP_GUIOBJ_Source_SetSourceMapping_Table(SourceConfigTable_New);
					}

				}
			}
		}
		g_stFactoryUserData.SystemConfig.DVDAndroidOptio.n_SysConf_DVD_DVDInputPort = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.DVDAndroidOptio.n_SysConf_DVD_DVDInputPort),
			sizeof(g_stFactoryUserData.SystemConfig.DVDAndroidOptio.n_SysConf_DVD_DVDInputPort),
			&(g_stFactoryUserData.SystemConfig.DVDAndroidOptio.n_SysConf_DVD_DVDInputPort));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.DVDAndroidOptio.n_SysConf_DVD_DVDInputPort),
			sizeof(g_stFactoryUserData.SystemConfig.DVDAndroidOptio.n_SysConf_DVD_DVDInputPort));
		*/
	}
	else
	{
		// UMF get function
		/*
		AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,0,
				sizeof(APP_SETTING_FactoryUser_t), &g_stFactoryUserData);
		*/
		FuncData->value = g_stFactoryUserData.SystemConfig.DVDAndroidOptio.n_SysConf_DVD_DVDInputPort;
		if(FuncData->value >= SysConf_DVDAndroidInputSource_MapTab_Size)
		{
			FuncData->value = 0;
		}
		memset(StringList,0x0,sizeof(StringList));
		u8TempCount = 0;
		for(i=0;i<SysConf_DVDAndroidInputSource_MapTab_Size;i++)
		{
			for (u8Count = 0; u8Count < g_u16SourceConfigTable_Size; u8Count++)
			{
				if(g_stSourceConfigTable[u8Count].eSourceType
					== SysConf_DVDAndroidInputSource_MapTab[i])
				{
					StringList[u8TempCount++] = g_stSourceConfigTable[u8Count].dOSDStrID;
				}
			}
		}
		StringList[u8TempCount] = STRING_LAST;
		if(pContent->str)
		{
			pContent->str = NULL;
		}
		if(pContent->str == NULL)
		{
			pContent->str = (UINT32 *)malloc((u8TempCount+1)*sizeof(UINT32));
			memcpy(pContent->str,StringList,(u8TempCount+1)*sizeof(UINT32));
		}
		pContent->nMin = 0;
		pContent->nMax = u8TempCount - 1;
		pContent->step = 1;
	}
}
/*2.5.1  end*/

/*2.5.2 start*/
//	ID_FM_SysConf_DVD_AndroidInputPort,			//29
void Factory_SysConf_DVD_AndroidInputPort(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	UINT8 i = 0;
	UINT8 u8Count = 0;
	UINT8 u8TempCount = 0;
	APP_Source_Type_t SourceType = APP_SOURCE_ANDRO;
	String_id_t StringList[APP_SOURCE_MAX] = {STRING_LAST};
	APP_SourceConfig_t SourceConfigTable_New[APP_SOURCE_MAX];
	UINT16 u16SourceConfigTable_Size_New;
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	MENU_SLIDER_STATICSTRS *pContent = (MENU_SLIDER_STATICSTRS *)Fun_SysConf_DVD_AndroidInputPort.leaves;
	APP_GUIOBJ_Source_SourceMapping_Table(SourceConfigTable_New,&u16SourceConfigTable_Size_New);
	if (bSet)
	{
		if (FuncData->value >= SysConf_DVDAndroidInputSource_MapTab_Size)
		{
			FuncData->value = 0;
		}
		for (i = 0; i < u16SourceConfigTable_Size_New; i++)
		{
			if(SourceConfigTable_New[i].eSourceType
				== SysConf_DVDAndroidInputSource_MapTab[(UINT32)FuncData->value])
			{
				for(u8Count = 0; u8Count < u16SourceConfigTable_Size_New; u8Count++)
				{
					if(SourceConfigTable_New[u8Count].eSourceType == SourceType)
					{
						memcpy(&SourceConfigTable_New[u8Count].eMidSourceType, &SourceConfigTable_New[i].eMidSourceType, sizeof(InputSrc_t));
						APP_GUIOBJ_Source_SetSourceMapping_Table(SourceConfigTable_New);
					}

				}
			}
		}
		g_stFactoryUserData.SystemConfig.DVDAndroidOptio.n_SysConf_DVD_AndroidInputPort = FuncData->value;

		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.DVDAndroidOptio.n_SysConf_DVD_AndroidInputPort),
			sizeof(g_stFactoryUserData.SystemConfig.DVDAndroidOptio.n_SysConf_DVD_AndroidInputPort),
			&(g_stFactoryUserData.SystemConfig.DVDAndroidOptio.n_SysConf_DVD_AndroidInputPort));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.DVDAndroidOptio.n_SysConf_DVD_AndroidInputPort),
			sizeof(g_stFactoryUserData.SystemConfig.DVDAndroidOptio.n_SysConf_DVD_AndroidInputPort));
		*/
	}
	else
	{
		// UMF get function
		AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,0,
				sizeof(APP_SETTING_FactoryUser_t), &g_stFactoryUserData);
		FuncData->value = g_stFactoryUserData.SystemConfig.DVDAndroidOptio.n_SysConf_DVD_AndroidInputPort;
		if(FuncData->value >= SysConf_DVDAndroidInputSource_MapTab_Size)
		{
			FuncData->value = 0;
		}
		memset(StringList,0x0,sizeof(StringList));
		u8TempCount = 0;
		for(i=0;i<SysConf_DVDAndroidInputSource_MapTab_Size;i++)
		{
			for (u8Count = 0; u8Count < g_u16SourceConfigTable_Size; u8Count++)
			{
				if(g_stSourceConfigTable[u8Count].eSourceType
					== SysConf_DVDAndroidInputSource_MapTab[i])
				{
					StringList[u8TempCount++] = g_stSourceConfigTable[u8Count].dOSDStrID;
					continue;
				}
			}
		}
		StringList[u8TempCount] = STRING_LAST;
		if(pContent->str)
		{
			pContent->str = NULL;
		}
		if(pContent->str == NULL)
		{
			pContent->str = (UINT32 *)malloc((u8TempCount+1)*sizeof(UINT32));
			memcpy(pContent->str,StringList,(u8TempCount+1)*sizeof(UINT32));
		}
		pContent->nMin = 0;
		pContent->nMax = u8TempCount - 1;
		pContent->step = 1;
	}
}

/*2.5.2  end*/

/*2.8 2.9 start*/
//	ID_FM_SysConf_PowerShowLogo,			//34
void Factory_SysConf_PowerShowLogo(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		AL_FLASH_SetPowerShowLogo((UINT8) FuncData->value);
		MID_TVFE_HideLogo((BOOL)FuncData->value);
	}
	else
	{
		// UMF get function
		//printf("[DBG] AL_FLASH_GetPowerShowLogo() = %d\n", AL_FLASH_GetPowerShowLogo());
	    FuncData->value = AL_FLASH_GetPowerShowLogo();
		//printf("[DBG] FuncData->value = %f\n", FuncData->value);
	}
}

//	ID_FM_SysConf_ACAutoPowerOn,			//35
void Factory_SysConf_ACAutoPowerOn(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		AL_FLASH_SetACAutoPowerOn((UINT8 )FuncData->value);
		if(FuncData->value>0)
			MID_TVFE_SetAutoPowerOn(TRUE);
		else if(FuncData->value==0)
			MID_TVFE_SetAutoPowerOn(FALSE);
	}
	else
	{
		// UMF get function
		//printf("[DBG] AL_FLASH_GetACAutoPowerOn() = %d\n", AL_FLASH_GetACAutoPowerOn());
		FuncData->value = (double)AL_FLASH_GetACAutoPowerOn();
		//printf("[DBG] FuncData->value = %f\n", FuncData->value);
	}
}

/*2.8 2.9 End*/
/*3. Picture Mode 	Start*/
	/*3.1 ADC Adjust Start*/
//ID_FM_PicMode_ADC_SupportMode,			//117
void Factory_PicMode_ADC_SupportMode(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	#if 0 //Please remove this function
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		g_stFactoryUserData.PictureMode.ADCAdjust.n_PicMode_ADC_SupportMode= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);

	}
	else
	{
		// UMF get function
		FuncData->value =g_stFactoryUserData.PictureMode.ADCAdjust.n_PicMode_ADC_SupportMode;
	}
	#endif
}
//ID_FM_PicMode_ADC_Adjust,				//118
void APP_GUIOBJ_FM_ADCAutoColor(void)
{

	MID_DISP_DTVSetVideoMute(0, 0, 0);
	MID_TVFE_DoAdcWhiteBalance(&stAdcWhiteBalance);
	MID_DISP_DTVSetVideoUnmute();
	AL_FLASH_SetADCRGain(stAdcWhiteBalance.scOSDRGainValue);
	AL_FLASH_SetADCGGain(stAdcWhiteBalance.scOSDGGainValue);
	AL_FLASH_SetADCBGain(stAdcWhiteBalance.scOSDBGainValue);
	AL_FLASH_SetADCROffset(stAdcWhiteBalance.scOSDROffsetValue);
	AL_FLASH_SetADCGOffset(stAdcWhiteBalance.scOSDGOffsetValue);
	AL_FLASH_SetADCBOffset(stAdcWhiteBalance.scOSDBOffsetValue);
}

void Factory_PicMode_ADC_Adjust(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	FuncData->value = 0; // Show "Start"

	input_type_t eInputType = INPUT_TYPE_DISABLE;
	MID_DISP_DTVGetInputType(&eInputType);

	PLF_VideoID_t eResolution;
	MID_TVFE_GetCurrentVideoInfo(NULL, &eResolution);

	int nIndex;
	// Note: Can't Disable all
	int nStartIndex = ID_FM_PicMode_ADC_RGain - ID_FM_PicMode_ADC_Adjust;
	int nEndIndex = ID_FM_PicMode_ADC_BOffset - ID_FM_PicMode_ADC_Adjust;
	HWND FocusItem_Handle;

	if(
		((eInputType != INPUT_TYPE_PC) &&
		(eInputType != INPUT_TYPE_COMPONENT) &&
		(eInputType != INPUT_TYPE_SCART_RGB)) ||
		(eResolution == PLF_VIDEO_TIMING_ID_NO_SIGNAL)||
		(eResolution == PLF_VIDEO_TIMING_ID_UNKNOW_FORMAT)
	)
	{
		//Disable items

		for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));
		}
		FuncData->value = 1; // Show "(NONE)"
		return;
	}else{
		//Enable items

		for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));
		}

	}

	if (bSet)
	{
#ifdef CONFIG_SUPPORT_ALL_ACTION_SHOW_BLUE_SCREEN
        MID_DISP_DTVSetVideoMute(0,0,255);
#else
		MID_DISP_DTVSetVideoMute(0, 0, 0);
#endif
		MID_TVFE_DoAdcWhiteBalance(&stAdcWhiteBalance);
		MID_DISP_DTVSetVideoUnmute();
	}
	else
	{
		// UMF get function
		MID_TVFE_GetAdcWhiteBalanceValue(&stAdcWhiteBalance);
	}
	AL_FLASH_SetADCRGain(stAdcWhiteBalance.scOSDRGainValue);
	AL_FLASH_SetADCGGain(stAdcWhiteBalance.scOSDGGainValue);
	AL_FLASH_SetADCBGain(stAdcWhiteBalance.scOSDBGainValue);
	AL_FLASH_SetADCROffset(stAdcWhiteBalance.scOSDROffsetValue);
	AL_FLASH_SetADCGOffset(stAdcWhiteBalance.scOSDGOffsetValue);
	AL_FLASH_SetADCBOffset(stAdcWhiteBalance.scOSDBOffsetValue);
	if (bSet)
	{
		UpdateNodeFunctionContent(ID_FM_PicMode_ADC_Adjust,0,0);
	}
}
//ID_FM_PicMode_ADC_RGain,				//119
void Factory_PicMode_ADC_RGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Fact_Adc_OSD_st stAdcOSD;
	if (bSet)
	{
		stAdcOSD.wAdcOSDMinValue = ADCADJUSTGAINOFFSET_MIN;
		stAdcOSD.wAdcOSDMaxValue = ADCADJUSTGAINOFFSET_MAX;
		stAdcOSD.wAdcOSDUserValue = (INT16)FuncData->value;
		MID_TVFE_SetAdcWhiteBalanceValue(Factory_ADCAdjust_RGain, stAdcOSD);
		AL_FLASH_SetADCRGain((UINT16)FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)AL_FLASH_GetADCRGain();
	}
}
//ID_FM_PicMode_ADC_GGain,				//120
void Factory_PicMode_ADC_GGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Fact_Adc_OSD_st stAdcOSD;
	if (bSet)
	{
		stAdcOSD.wAdcOSDMinValue = ADCADJUSTGAINOFFSET_MIN;
		stAdcOSD.wAdcOSDMaxValue = ADCADJUSTGAINOFFSET_MAX;
		stAdcOSD.wAdcOSDUserValue = (INT16)FuncData->value;
		MID_TVFE_SetAdcWhiteBalanceValue(Factory_ADCAdjust_GGain, stAdcOSD);
		AL_FLASH_SetADCGGain((UINT16)FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)AL_FLASH_GetADCGGain();
	}
}
//ID_FM_PicMode_ADC_BGain,				//121
void Factory_PicMode_ADC_BGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Fact_Adc_OSD_st stAdcOSD;
	if (bSet)
	{
		stAdcOSD.wAdcOSDMinValue = ADCADJUSTGAINOFFSET_MIN;
		stAdcOSD.wAdcOSDMaxValue = ADCADJUSTGAINOFFSET_MAX;
		stAdcOSD.wAdcOSDUserValue = (INT16)FuncData->value;
		MID_TVFE_SetAdcWhiteBalanceValue(Factory_ADCAdjust_BGain, stAdcOSD);
		AL_FLASH_SetADCBGain((UINT16)FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)AL_FLASH_GetADCBGain();
	}
}
//ID_FM_PicMode_ADC_ROffset,				//122
void Factory_PicMode_ADC_ROffset(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Fact_Adc_OSD_st stAdcOSD;
	if (bSet)
	{
		stAdcOSD.wAdcOSDMinValue = ADCADJUSTGAINOFFSET_MIN;
		stAdcOSD.wAdcOSDMaxValue = ADCADJUSTGAINOFFSET_MAX;
		stAdcOSD.wAdcOSDUserValue = (INT16)FuncData->value;
		MID_TVFE_SetAdcWhiteBalanceValue(Factory_ADCAdjust_ROffset, stAdcOSD);
		AL_FLASH_SetADCROffset((UINT16)FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)AL_FLASH_GetADCROffset();
	}
}
//ID_FM_PicMode_ADC_GOffset,				//123
void Factory_PicMode_ADC_GOffset(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Fact_Adc_OSD_st stAdcOSD;
	if (bSet)
	{
		stAdcOSD.wAdcOSDMinValue = ADCADJUSTGAINOFFSET_MIN;
		stAdcOSD.wAdcOSDMaxValue = ADCADJUSTGAINOFFSET_MAX;
		stAdcOSD.wAdcOSDUserValue = (INT16)FuncData->value;
		MID_TVFE_SetAdcWhiteBalanceValue(Factory_ADCAdjust_GOffset, stAdcOSD);
		AL_FLASH_SetADCGOffset((UINT16)FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)AL_FLASH_GetADCGOffset();
	}
}
//ID_FM_PicMode_ADC_BOffset,				//124
void Factory_PicMode_ADC_BOffset(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Fact_Adc_OSD_st stAdcOSD;
	if (bSet)
	{
		stAdcOSD.wAdcOSDMinValue = ADCADJUSTGAINOFFSET_MIN;
		stAdcOSD.wAdcOSDMaxValue = ADCADJUSTGAINOFFSET_MAX;
		stAdcOSD.wAdcOSDUserValue = (INT16)FuncData->value;
		MID_TVFE_SetAdcWhiteBalanceValue(Factory_ADCAdjust_BOffset, stAdcOSD);
		AL_FLASH_SetADCBOffset((UINT16)FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)AL_FLASH_GetADCBOffset();
	}
}
	/*3.1 ADC Adjust End*/


	/*3.2 W/B Adjust Start*/
//ID_FM_PicMode_WB_Mode,					//126
void Factory_PicMode_WB_Mode(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if (bSet)
	{
	//	AL_FLASH_SetWBMode((UINT8) FuncData->value);
	}
	else
	{
		// UMF get function
	//	FuncData->value = (INT32)AL_FLASH_GetWBMode();
	}
}
//ID_FM_PicMode_WB_Temperature,			//127
void Factory_PicMode_WB_Temperature(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int Mode = (INT32)AL_FLASH_GetWBMode();
	if (bSet)
	{
		AL_FLASH_SetWBMode((UINT8) FuncData->value);
		APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_COLORTEMP,FuncData->value);

		UpdateNodeFunctionContent(ID_FM_PicMode_WB_Temperature,0,0);

	}
	else
	{
		// UMF get function
		FuncData->value = Mode;
	}
}
//ID_FM_PicMode_WB_RGain,				//128
void Factory_PicMode_WB_RGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int Mode = (INT32)AL_FLASH_GetWBMode();
	APP_SETTING_ColorTemp_t stColorTempValue ;
	AL_FLASH_GetWBTemperature(Mode,&stColorTempValue);

	if (bSet)
	{
		MID_TVFE_SetColorTmpRGain((UINT16)FuncData->value);
		stColorTempValue.u16RGain = (UINT16)FuncData->value;

		AL_FLASH_SetWBTemperature(Mode,&stColorTempValue);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)stColorTempValue.u16RGain;
	}
}
//ID_FM_PicMode_WB_GGain,				//129
void Factory_PicMode_WB_GGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	int Mode = (INT32)AL_FLASH_GetWBMode();
	APP_SETTING_ColorTemp_t stColorTempValue ;
	AL_FLASH_GetWBTemperature(Mode,&stColorTempValue);

	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetColorTmpGGain((UINT16)FuncData->value);
		stColorTempValue.u16GGain = (UINT16)FuncData->value;

		AL_FLASH_SetWBTemperature(Mode,&stColorTempValue);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)stColorTempValue.u16GGain;
	}
}
//ID_FM_PicMode_WB_BGain,				//130
void Factory_PicMode_WB_BGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	int Mode = (INT32)AL_FLASH_GetWBMode();
	APP_SETTING_ColorTemp_t stColorTempValue ;
	AL_FLASH_GetWBTemperature(Mode,&stColorTempValue);
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetColorTmpBGain((UINT16)FuncData->value);
		stColorTempValue.u16BGain = (UINT16)FuncData->value;

		AL_FLASH_SetWBTemperature(Mode,&stColorTempValue);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)stColorTempValue.u16BGain;
	}
}
//ID_FM_PicMode_WB_ROffset,				//131
void Factory_PicMode_WB_ROffset(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	int Mode = (INT32)AL_FLASH_GetWBMode();
	APP_SETTING_ColorTemp_t stColorTempValue ;
	AL_FLASH_GetWBTemperature(Mode,&stColorTempValue);

	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetColorTmpROffset((UINT16)FuncData->value);
		stColorTempValue.u16ROffset = (INT16)FuncData->value;

		AL_FLASH_SetWBTemperature(Mode,&stColorTempValue);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)stColorTempValue.u16ROffset;
	}
}
//ID_FM_PicMode_WB_GOffset,				//132
void Factory_PicMode_WB_GOffset(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	int Mode = (INT32)AL_FLASH_GetWBMode();
	APP_SETTING_ColorTemp_t stColorTempValue ;
	AL_FLASH_GetWBTemperature(Mode,&stColorTempValue);

	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetColorTmpGOffset((UINT16)FuncData->value);
		stColorTempValue.u16GOffset = (INT16)FuncData->value;

		AL_FLASH_SetWBTemperature(Mode,&stColorTempValue);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)stColorTempValue.u16GOffset;
	}
}
//ID_FM_PicMode_WB_BOffset,				//133
void Factory_PicMode_WB_BOffset(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	int Mode = (INT32)AL_FLASH_GetWBMode();
	APP_SETTING_ColorTemp_t stColorTempValue ;
	AL_FLASH_GetWBTemperature(Mode,&stColorTempValue);

	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetColorTmpBOffset((UINT16)FuncData->value);
		stColorTempValue.u16BOffset = (INT16)FuncData->value;

		AL_FLASH_SetWBTemperature(Mode,&stColorTempValue);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)stColorTempValue.u16BOffset;
	}
}
//ID_FM_PicMode_WB_GammaTable,			//134
void Factory_PicMode_WB_GammaTable(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int Mode = (INT32)AL_FLASH_GetWBMode();
	if (bSet)
	{
		UINT8 nSwitchState = AL_FLASH_GetGammaType();
		//FuncData->value = FuncData->value - 1;//OSD display 1 ~ 3 ,but gamma index 0 ~ 2
		if(nSwitchState)
		{
			MID_TVFE_SetColorTmpGammaTableIndex(FuncData->value);
		}
		else
		{
			DEBF("[FM DBG] skip gamma table setting!!\n");
		}
		AL_FLASH_SetWBGammaTable(Mode,(UINT8) FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetWBGammaTable(Mode);
	}
}
//ID_FM_PicMode_WB_CopyAll,				//135
void Factory_PicMode_WB_CopyAll(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if (bSet)
	{
		//[CC] write back all data

	}
	else
	{
		// UMF get function
	}
}
	/*3.2 W/B Adjust End*/
	/*3.3 Picture Start*/

//ID_FM_PicMode_Pic_Mode,					//136
void Factory_PicMode_Pic_Mode(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	// Each source
	//APP_Source_Type_t eSourType = 0;
	//APP_GUIOBJ_Source_GetCurrSource(&eSourType);
	/*
	if (bSet)
	{
		AL_FLASH_SetPictureSource((UINT8)FuncData->value);
		// Update all items
		UpdateNodeFunctionContent(ID_FM_PicMode_Pic_Mode,0,0);

	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetPictureSource();
	}
	*/
	APP_Source_Type_t nCurrentSource;
	APP_GUIOBJ_Source_GetCurrSource(&nCurrentSource);
	if( nCurrentSource <= APP_SOURCE_MEDIA)
	{
		FuncData->value = nCurrentSource;
	}else{
		FuncData->value = APP_SOURCE_MEDIA+1;
	}

}
//ID_FM_PicMode_Pic_Picture,				//137
void Factory_PicMode_Pic_Picture(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int currsource 	= (INT32)AL_FLASH_GetPictureSource();
	// 0: ⒁ynamic, 1: Standard, 2: Mid , 3: User
	if (bSet)
	{
		//[CC] write back all data
		AL_FLASH_SetPictureMode(currsource,(UINT8)FuncData->value);
		// Update all items
		APP_Video_SetPictureMode(FuncData->value);
		UpdateNodeFunctionContent(ID_FM_PicMode_Pic_Mode,0,0);

	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetPictureMode(currsource);
	}
}

//"brightness" = 0, "contrast" = 1, "sharpness" = 2, "saturation" = 3, "Hue" = 4

//ID_FM_PicMode_Pic_Brightness,				//138
void Factory_PicMode_Pic_Brightness(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int currsource	= (INT32)AL_FLASH_GetPictureSource();
	int mode 		= (INT32)AL_FLASH_GetPictureMode(currsource);
	if (bSet)
	{
		AL_FLASH_SetPictureBrightness(currsource,mode,(UINT8)FuncData->value);
		APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_BRIGHTNESS,FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetPictureBrightness(currsource,mode);
	}
}
//ID_FM_PicMode_Pic_Contrast,				//139
void Factory_PicMode_Pic_Contrast(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int currsource	= (INT32)AL_FLASH_GetPictureSource();
	int mode 		= (INT32)AL_FLASH_GetPictureMode(currsource);

	if (bSet)
	{
		AL_FLASH_SetPictureContrast(currsource,mode,(UINT8)FuncData->value);
		APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_CONTRAST,FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetPictureContrast(currsource,mode);
	}
}
//ID_FM_PicMode_Pic_Color,					//140
void Factory_PicMode_Pic_Color(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int currsource	= (INT32)AL_FLASH_GetPictureSource();
	int mode 		= (INT32)AL_FLASH_GetPictureMode(currsource);

	if (bSet)
	{
		AL_FLASH_SetPictureColor(currsource,mode,(UINT8)FuncData->value);
		APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_SATURTUION,FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetPictureColor(currsource,mode);
	}
}
//ID_FM_PicMode_Pic_Sharpness,				//141
void Factory_PicMode_Pic_Sharpness(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int currsource	= (INT32)AL_FLASH_GetPictureSource();
	int mode 		= (INT32)AL_FLASH_GetPictureMode(currsource);

	if (bSet)
	{
		AL_FLASH_SetPictureSharpness(currsource,mode,(UINT8)FuncData->value);
		APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_SHARPNESS,FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetPictureSharpness(currsource,mode);
	}
}
//ID_FM_PicMode_Pic_Tint,					//142
void Factory_PicMode_Pic_Tint(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int currsource	= (INT32)AL_FLASH_GetPictureSource();
	int mode 		= (INT32)AL_FLASH_GetPictureMode(currsource);

	if (bSet)
	{
		AL_FLASH_SetPictureTint(currsource,mode,(UINT8)FuncData->value);
		APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_HUE,FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetPictureTint(currsource,mode);
	}
}
//ID_FM_PicMode_Pic_NoiseReduction,			//143
// Note: 20140506: PQ's request. Merge NR and MPEG NR
void Factory_PicMode_Pic_NoiseReduction(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	if (bSet)
	{
		AL_FLASH_SetPictureNR((UINT8) FuncData->value);

		// Noise Reduction
		MID_TVFE_SetPictureNR( (INT8)FuncData->value );

		// MPEG Noise Reductuion
		MID_TVFE_SetPictureMPEGNR((UINT16)FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)AL_FLASH_GetPictureNR();

		// Sync NR and Mpeg NR
	}
}
//ID_FM_PicMode_Pic_ColorMatrix,			//144
void Factory_PicMode_Pic_ColorMatrix(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	if (bSet)
	{
		MID_TVFE_SetPictureColorMatrix((UINT16) FuncData->value);
		AL_FLASH_SetPictureColorMatrix((UINT8) FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetPictureColorMatrix();
	}
}
//ID_FM_PicMode_Pic_DitheringLevel,			//145
void Factory_PicMode_Pic_DitheringLevel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetPictureDitheringLevel((UINT16)FuncData->value);
		AL_FLASH_SetPictureDitheringLevel((UINT8) FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetPictureDitheringLevel();
	}
}
//ID_FM_PicMode_Pic_DitheringAlgorithm,		//146
void Factory_PicMode_Pic_DitheringAlgorithm(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetPictureDitheringAlgorithm((UINT16)FuncData->value);
		AL_FLASH_SetPictureDitheringAlgorithm((UINT8) FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetPictureDitheringAlgorithm();
	}
}
//ID_FM_PicMode_Pic_MPEGNR,				//147
// Note: 20140506: PQ's request. Merge to Factory_PicMode_Pic_NoiseReduction()
/*
void Factory_PicMode_Pic_MPEGNR(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetPictureMPEGNR((UINT16)FuncData->value);
		AL_FLASH_SetPictureMPEGNR((UINT8) FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetPictureMPEGNR();
	}
}
*/
	/*3.3 Picture End*/
/*3.4 OverScan  Start*/
static INT32 _APP_Fm_OverScan_AspectRatio_Set(APP_Video_AspectRatioType_e eAspectType)
{
	Boolean bOverscan = TRUE;
	APP_Source_Type_t eSrcType = APP_SOURCE_MAX;
	PLF_VideoID_t eRetResolution;
	UINT32 u32TempValue = APP_HDMI_AUTO;
	MID_InputInfo_t stTimingInfo;
	Boolean bIsHdmiNeedOverScan = TRUE;

	APP_GUIOBJ_Source_GetCurrSource(&eSrcType);
	MID_TVFE_GetCurrentVideoInfo(&stTimingInfo, &eRetResolution);

	bIsHdmiNeedOverScan = stTimingInfo.bIsHdmiNeedOverScan;

	if ((eAspectType == APP_VIDEO_ASPECT_RATIO_AUTO)
		&& (eSrcType != APP_SOURCE_PC))
	{
	    eAspectType = APP_VIDEO_ASPECT_RATIO_AUTO;
	}

	if ((eSrcType == APP_SOURCE_PC)
#ifndef SUPPORT_OVERSCAN_MOVIE_ONMEDIA	
		|| (eSrcType == APP_SOURCE_MEDIA)
#endif		
		)
	{
	    bOverscan = FALSE;
	}
	else if(eSrcType == APP_SOURCE_HDMI
	    || eSrcType == APP_SOURCE_HDMI1
	    || eSrcType == APP_SOURCE_HDMI2)
	{
		AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
		sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
		u32TempValue = g_stSetupData.HDMIMode;
		if(u32TempValue == APP_HDMI_AUTO) /* auto mode */
		{
			if (bIsHdmiNeedOverScan == TRUE ) /*Video timing*/
			{
				bOverscan = TRUE;
			}
			else
			{
				bOverscan = FALSE;
			}
		}
		else if(u32TempValue == APP_HDMI_PC) /* PC mode */
		{
			bOverscan = FALSE;
		}
		else /* video mode */
		{
			bOverscan = TRUE;
		}
	}
	else
	{
		bOverscan = TRUE;
	}

	if(eAspectType == APP_VIDEO_ASPECT_RATIO_JUSTSCAN)//all source justscan mode not set overscan
	{
		bOverscan = FALSE;
	}

	DISP_Aspect_Ratio_e eMidDispAspect = APP_Video_AspectRatioTypeMappingToMid(eAspectType);
	if (MID_DISP_DTVSetDispAspect(eMidDispAspect, bOverscan) == MIDDISP_SUCCESS)
	{
		APP_Video_AspectSystemSetWrite(APP_Video_AspectRatioTypeMappingToIndex(eAspectType));
		return SP_SUCCESS;
	}

	return SP_ERR_FAILURE;
}

static Boolean _APP_Fm_OverScan_CheckAspect(void)
{
	APP_Source_Type_t eSourType = APP_SOURCE_MAX;
	APP_GUIOBJ_Source_GetCurrSource(&eSourType);
#ifdef CONFIG_ATV_SUPPORT
	if (eSourType == APP_SOURCE_ATV
		|| eSourType == APP_SOURCE_AV
		|| eSourType == APP_SOURCE_AV1
		|| eSourType == APP_SOURCE_AV2
		|| eSourType == APP_SOURCE_SVIDEO
		|| eSourType == APP_SOURCE_SVIDEO1
		|| eSourType == APP_SOURCE_SVIDEO2
		|| eSourType == APP_SOURCE_ANDRO
		|| eSourType == APP_SOURCE_SCART
		|| eSourType == APP_SOURCE_SCART1)
	{
		PLF_VideoID_t eResolution;
		MID_TVFE_GetCurrentVideoInfo(NULL, &eResolution);

		if ((eResolution == PLF_VIDEO_TIMING_ID_NO_SIGNAL) ||
			(eResolution == PLF_VIDEO_TIMING_ID_SNOW525)   ||
			(eResolution == PLF_VIDEO_TIMING_ID_UNKNOW_FORMAT))
		{
			return SP_ERR_FAILURE;
		}
	}
#endif
#ifdef CONFIG_DTV_SUPPORT
	if(eSourType == APP_SOURCE_DTV)
	{
		AL_DB_EDBType_t eNetType = APP_DVB_Playback_GetCurrentNetType();
		AL_DB_ERecordType_t eServiceType = APP_DVB_Playback_GetCurrServiceType(eNetType);

		if((eServiceType == AL_RECTYPE_DVBRADIO && (eSourType == APP_SOURCE_DTV || eSourType == APP_SOURCE_RADIO))
#ifdef CONFIG_SUPPORT_PVR	//if playing pvr file, allow to set pic mode
			&& !SYSAPP_GOBJ_GUIObjectExist(SYS_APP_DVB, DVB_GUIOBJ_PVR_FILEPLAY)
#endif
		   )
		{
			return SP_ERR_FAILURE;
		}
		MID_DISP_MUTE_ST fEn;
		MID_DISP_DTVGetVideoMute(&fEn);
		if (fEn != MID_DISP_UNMUTE)
		{
#ifdef CONFIG_SUPPORT_MHEG5
			al_uint8 bSignalState = al_false;
			AL_DVB_Monitor_GetState(AL_DVB_MONITOR_STATE_SIGNAL, &bSignalState);
			if((bSignalState != AL_DVB_MONITOR_VALUE_TRUE)
				||(!((APP_DVB_Mheg5_GetBootCarouselStatus() == MHEG5_BOOT_CAROUSEL_EXISTED)
				&& APP_Area_SupportDtg())))
			{
				return SP_ERR_FAILURE;
			}
#else
			return SP_ERR_FAILURE;
#endif
		}

		al_bool bHasValidServ = AL_DB_HasVisibleService(eNetType);
		al_uint8 bSignalState = al_false;
		AL_DVB_Monitor_GetState(AL_DVB_MONITOR_STATE_SIGNAL, &bSignalState);
		if ((bHasValidServ && (bSignalState == AL_DVB_MONITOR_VALUE_TRUE) && (eServiceType != AL_RECTYPE_DVBRADIO)))
		{
			return SP_SUCCESS;
		}
		else
		{
			return SP_ERR_FAILURE;
		}
	}
#endif
	return SP_SUCCESS;
}


static INT32 _APP_GUIOBJ_FM_GetAspectValue(Boolean bIsRightKey)
{
	APP_Source_Type_t eSourType = APP_SOURCE_MAX;

	MID_InputInfo_t stTimingInfo;
	Boolean bIsHdmiNeedOverScan = TRUE;

	MID_DISP_DTVGetTimingInfo(&stTimingInfo);
	bIsHdmiNeedOverScan = stTimingInfo.bIsHdmiNeedOverScan;

	APP_GUIOBJ_Source_GetCurrSource(&eSourType);

	if (eSourType == APP_SOURCE_ATV
		|| eSourType == APP_SOURCE_RADIO
		|| eSourType == APP_SOURCE_AV
		|| eSourType == APP_SOURCE_AV1
		|| eSourType == APP_SOURCE_AV2
		|| eSourType == APP_SOURCE_SVIDEO
		|| eSourType == APP_SOURCE_SVIDEO1
		|| eSourType == APP_SOURCE_SVIDEO2
		|| eSourType == APP_SOURCE_ANDRO)
	{
		if (bIsRightKey)
			u8aspect_index = (u8aspect_index + 1) % g_u8TV_Svideo_AV_DVD_Aspect_Size;
		else
			u8aspect_index = (u8aspect_index + g_u8TV_Svideo_AV_DVD_Aspect_Size - 1) % g_u8TV_Svideo_AV_DVD_Aspect_Size;
	}
	else if(eSourType == APP_SOURCE_SCART
		||eSourType == APP_SOURCE_SCART1)
	{
		if (bIsRightKey)
			u8aspect_index = (u8aspect_index + 1) % g_u8TV_Svideo_SCART_Aspect_Size;
		else
			u8aspect_index = (u8aspect_index + g_u8TV_Svideo_SCART_Aspect_Size - 1) % g_u8TV_Svideo_SCART_Aspect_Size;
	}
	else if(eSourType == APP_SOURCE_DTV)
	{
		if (bIsRightKey)
			u8aspect_index = (u8aspect_index + 1) % g_u8DTVAspect_Size;
		else
			u8aspect_index = (u8aspect_index + g_u8DTVAspect_Size - 1) % g_u8DTVAspect_Size;
	}
	else if(eSourType == APP_SOURCE_PC)
	{
		if (bIsRightKey)
			u8aspect_index = (u8aspect_index + 1) % g_u8PCAspect_Size;
		else
			u8aspect_index = (u8aspect_index + g_u8PCAspect_Size - 1) % g_u8PCAspect_Size;
	}
	else if(eSourType == APP_SOURCE_YPBPR
		|| eSourType == APP_SOURCE_YPBPR1
		|| eSourType == APP_SOURCE_DVD
		|| eSourType == APP_SOURCE_YPBPR2)
	{
		if (bIsRightKey)
			u8aspect_index = (u8aspect_index + 1) % g_u8YPbPrAspect_Size;
		else
			u8aspect_index = (u8aspect_index + g_u8YPbPrAspect_Size - 1) % g_u8YPbPrAspect_Size;
	}
	else if(eSourType == APP_SOURCE_HDMI
		||eSourType == APP_SOURCE_HDMI1
		||eSourType == APP_SOURCE_HDMI2)
	{
		if (bIsHdmiNeedOverScan == TRUE )
		{
			if (bIsRightKey)
				u8aspect_index = (u8aspect_index + 1) % g_u8HDMI_Video_Aspect_Size;
			else
				u8aspect_index = (u8aspect_index + g_u8HDMI_Video_Aspect_Size - 1) % g_u8HDMI_Video_Aspect_Size;
		}
		else
		{
			if (bIsRightKey)
				u8aspect_index = (u8aspect_index + 1) % g_u8HDMI_PC_Aspect_Size;
			else
				u8aspect_index = (u8aspect_index + g_u8HDMI_PC_Aspect_Size - 1) % g_u8HDMI_PC_Aspect_Size;
		}
	}
	else if(eSourType == APP_SOURCE_MEDIA)
	{
		if (bIsRightKey)
			u8aspect_index = (u8aspect_index + 1) % g_u8Media_Aspect_Size;
		else
			u8aspect_index = (u8aspect_index + g_u8Media_Aspect_Size - 1) % g_u8Media_Aspect_Size;
	}

	OverScanSettingValue.eScalerMode = APP_Video_AspectRatioIndexMappingToType(u8aspect_index);
	//printf("cRi: [%s] u8aspect_index=%d eScalerMode=%d\n", __FUNCTION__, u8aspect_index, OverScanSettingValue.eScalerMode);

	return SP_SUCCESS;
}

static INT32 _APP_GUIOBJ_FM_GetInputActiveStartValue(PLF_VideoID_t eResolution)
{
	UINT16 usTimingTblIdx;
	Boolean UserFlag;
	input_type_t eInputType = INPUT_TYPE_DISABLE;
	MID_InputInfo_t TimingInfo;
	MID_DISP_DTVGetInputType(&eInputType);
	MID_TVFE_GetCurrentVideoInfo(&TimingInfo, NULL);
	switch (eInputType)
	{
		case INPUT_TYPE_COMPONENT:
		case INPUT_TYPE_SCART_RGB:
	  		MID_DISP_DTVGetExtsInpuwWin(&OverScanSettingValue.u16HStart, &OverScanSettingValue.u16HEnd,
				&OverScanSettingValue.u16VStart, &OverScanSettingValue.u16VEnd);
			break;
		case INPUT_TYPE_PC:
			PCAutoTune_FIFOTable_Search(eResolution, &usTimingTblIdx, &UserFlag);
			OverScanSettingValue.u16HStart = TimingInfo.usTimingTableHStart + g_stSysInfoData.stPC_AutoTune[usTimingTblIdx].i8HpositionOffset;
			OverScanSettingValue.u16VStart = TimingInfo.usTimingTableVStart + g_stSysInfoData.stPC_AutoTune[usTimingTblIdx].i8VpositionOffset;
			break;
		case INPUT_TYPE_ATV:
		case INPUT_TYPE_AV:
		case INPUT_TYPE_SV:
		case INPUT_TYPE_SCART_AUTO:
		case INPUT_TYPE_SCART_CVBS:
		case INPUT_TYPE_SCART_SVIDEO:
			OverScanSettingValue.u16HStart = TimingInfo.usTimingTableHStart;
			OverScanSettingValue.u16VStart = TimingInfo.usTimingTableVStart;
			break;
		default:
			OverScanSettingValue.u16HStart = 0;
			OverScanSettingValue.u16VStart = 0;
			break;

	}
	return SP_SUCCESS;
}

static INT32 _APP_GUIOBJ_FM_SetOverScan_HVStart(PLF_VideoID_t eResolution, UINT16 usStartType)
{
	UINT16 usTimingTblIdx;
	Boolean UserFlag;
	input_type_t eInputType = INPUT_TYPE_DISABLE;
	INT16 sHVStart;
	MID_InputInfo_t TimingInfo;
	MID_DISP_DTVGetInputType(&eInputType);
	MID_TVFE_GetCurrentVideoInfo(&TimingInfo, NULL);
	switch (eInputType)
	{
		case INPUT_TYPE_COMPONENT:
		case INPUT_TYPE_SCART_RGB:
			MID_DISP_DTVSetExtsInpuwWin(OverScanSettingValue.u16HStart, OverScanSettingValue.u16HEnd,
				OverScanSettingValue.u16VStart, OverScanSettingValue.u16VEnd);
			break;
		case INPUT_TYPE_PC:
			PCAutoTune_FIFOTable_Search(eResolution, &usTimingTblIdx, &UserFlag);
			if (usStartType == 1) //HStart
			{
				sHVStart = OverScanSettingValue.u16HStart -TimingInfo.usTimingTableHStart;
				MID_DISP_DTVSetCropWindowHStartOffset(0, sHVStart);
				g_stSysInfoData.stPC_AutoTune[usTimingTblIdx].i8HpositionOffset = sHVStart;
				AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
					ITEM_OFFSET(APP_SETTING_SystemInfo_t, stPC_AutoTune[usTimingTblIdx].i8HpositionOffset),
					sizeof(g_stSysInfoData.stPC_AutoTune[usTimingTblIdx].i8HpositionOffset),
					&(g_stSysInfoData.stPC_AutoTune[usTimingTblIdx].i8HpositionOffset));
				/*
				AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
					ITEM_OFFSET(APP_SETTING_SystemInfo_t, stPC_AutoTune[usTimingTblIdx].i8HpositionOffset),
					sizeof(g_stSysInfoData.stPC_AutoTune[usTimingTblIdx].i8HpositionOffset));
				*/
			}
			else //VStart
			{
				sHVStart = OverScanSettingValue.u16VStart -TimingInfo.usTimingTableVStart;
				MID_DISP_DTVSetCropWindowVStartOffset(0, sHVStart);
				g_stSysInfoData.stPC_AutoTune[usTimingTblIdx].i8VpositionOffset = sHVStart;
				AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
					ITEM_OFFSET(APP_SETTING_SystemInfo_t, stPC_AutoTune[usTimingTblIdx].i8VpositionOffset),
					sizeof(g_stSysInfoData.stPC_AutoTune[usTimingTblIdx].i8VpositionOffset),
					&(g_stSysInfoData.stPC_AutoTune[usTimingTblIdx].i8VpositionOffset));
				/*
				AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
					ITEM_OFFSET(APP_SETTING_SystemInfo_t, stPC_AutoTune[usTimingTblIdx].i8VpositionOffset),
					sizeof(g_stSysInfoData.stPC_AutoTune[usTimingTblIdx].i8VpositionOffset));
				*/
			}
			break;
		case INPUT_TYPE_ATV:
		case INPUT_TYPE_AV:
		case INPUT_TYPE_SV:
		case INPUT_TYPE_SCART_AUTO:
		case INPUT_TYPE_SCART_CVBS:
		case INPUT_TYPE_SCART_SVIDEO:
			if (usStartType == 1) //HStart
			{
				MID_DISP_DTVSetCVD2HActiveStart(OverScanSettingValue.u16HStart);
			}
			else //VStart
			{
				MID_DISP_DTVSetCVD2VActiveStart(OverScanSettingValue.u16VStart);
			}
			break;
		default:
			break;

	}
	return SP_SUCCESS;
}

//ID_FM_PicMode_OverScan_ScaleMode,				//148

BOOL IsOverScanAvalible(void)
{
	input_type_t eInputType = INPUT_TYPE_DISABLE;
	MID_DISP_DTVGetInputType(&eInputType);
	//printf("[DBG] eInputType = 0x%X\n", eInputType );

	PLF_VideoID_t eResolution;
	MID_TVFE_GetCurrentVideoInfo(NULL, &eResolution);
	//printf("[DBG] eResolution = 0x%X\n", eResolution );

	if(
		eInputType == INPUT_TYPE_PC ||
		eInputType == INPUT_TYPE_JPEG ||
#if defined(CONFIG_SUPPORT_NES_GAME) || defined(CONFIG_OSD_GAME_SUPPORT)
        eInputType == INPUT_TYPE_GAME ||
#endif	
		eResolution == PLF_VIDEO_TIMING_ID_NO_SIGNAL ||
		eResolution == PLF_VIDEO_TIMING_ID_UNKNOW_FORMAT ||
		SP_SUCCESS != _APP_Fm_OverScan_CheckAspect()
	)
		return FALSE;
	else
		return TRUE;
}

void Factory_PicMode_OverScan_ScaleMode(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;


	PLF_VideoID_t eResolution;
	MID_TVFE_GetCurrentVideoInfo(NULL, &eResolution);

	int nIndex;
	// Note: Can't Disable all
	int nStartIndex = ID_FM_PicMode_OverScan_VTopOverScan - ID_FM_PicMode_OverScan_ScaleMode;
	int nEndIndex = ID_FM_PicMode_OverScan_HRightOverScan - ID_FM_PicMode_OverScan_ScaleMode;
	HWND FocusItem_Handle;
	BOOL bRatioUnit;

#ifdef SUPPORT_OVERSCAN_MOVIE_ONMEDIA//  dchen105  20141210
	APP_Source_Type_t eSrcType = APP_SOURCE_MAX;
	APP_GUIOBJ_Source_GetCurrSource(&eSrcType);
	if ((eSrcType == APP_SOURCE_MEDIA)
		&&((SYSAPP_GOBJ_GUIObjectExist(SYS_APP_FILE_PLAYER, MEDIA_GUIOBJ_MOVIE_PLAYBACK)))
		//||SYSAPP_GOBJ_GUIObjectExist(SYS_APP_FILE_PLAYER, MEDIA_GUIOBJ_PVR_PLAYBACK))
		)

	{
		//Enable items

		for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));
		}

	}
	else
	#endif
	{
		if( !IsOverScanAvalible() )
		{
			//Disable items

			for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
			{
				GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

				GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
				GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));
			}
		}else{
			//Enable items

			for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
			{
				GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

				GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
				GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));
			}

		}
	}

	if (bSet)
	{
		OverScanSettingValue.eScalerMode = FuncData->value;
		_APP_Fm_OverScan_AspectRatio_Set(OverScanSettingValue.eScalerMode);
		MID_DISP_DTVGetOverscan(
			&bOverscanEn,
			&OverScanSettingValue.u16VTOverscan,
			&OverScanSettingValue.u16VBOverscan,
			&OverScanSettingValue.u16HLOverscan,
			&OverScanSettingValue.u16HROverscan,
			&bRatioUnit);
		_APP_GUIOBJ_FM_GetInputActiveStartValue(eResolution); //Get OverScanSettingValue.u16HStart / OverScanSettingValue.u16VStart value
		// Write back all data
		AL_FLASH_SetOverScanScaleMode((UINT8)OverScanSettingValue.eScalerMode);
		AL_FLASH_SetOverScanVTop(OverScanSettingValue.u16VTOverscan);
		AL_FLASH_SetOverScanVBottom(OverScanSettingValue.u16VBOverscan);
		AL_FLASH_SetOverScanHLeft(OverScanSettingValue.u16HLOverscan);
		AL_FLASH_SetOverScanHRight(OverScanSettingValue.u16HROverscan);
		AL_FLASH_SetOverScanHStart(OverScanSettingValue.u16HStart);
		AL_FLASH_SetOverScanVStart(OverScanSettingValue.u16VStart);

		// Update all items
		UpdateNodeFunctionContent(ID_FM_PicMode_OverScan_ScaleMode,0,0);

	}
	else
	{
		// UMF get function
		if (gbIsFMRightKey == TRUE)
		{
			_APP_GUIOBJ_FM_GetAspectValue(TRUE);
			if (OverScanSettingValue.eScalerMode != 0)
				OverScanSettingValue.eScalerMode--;
			else
				OverScanSettingValue.eScalerMode = (APP_VIDEO_ASPECT_RATIO_MAX-1);
		}
		else if (gbIsFMLeftKey == TRUE)
		{
			_APP_GUIOBJ_FM_GetAspectValue(FALSE);
			if (OverScanSettingValue.eScalerMode < (APP_VIDEO_ASPECT_RATIO_MAX-1))
				OverScanSettingValue.eScalerMode++;
			else
				OverScanSettingValue.eScalerMode = 0;
		}
		else
		{
			APP_Video_AspectSystemSetRead(&u8aspect_index);
			OverScanSettingValue.eScalerMode = APP_Video_AspectRatioIndexMappingToType(u8aspect_index);
		}

		MID_DISP_DTVGetOverscan(
			&bOverscanEn,
			&OverScanSettingValue.u16VTOverscan,
			&OverScanSettingValue.u16VBOverscan,
			&OverScanSettingValue.u16HLOverscan,
			&OverScanSettingValue.u16HROverscan,
			&bRatioUnit);
		_APP_GUIOBJ_FM_GetInputActiveStartValue(eResolution); //Get OverScanSettingValue.u16HStart / OverScanSettingValue.u16VStart value

		AL_FLASH_SetOverScanScaleMode((UINT8)OverScanSettingValue.eScalerMode);
		AL_FLASH_SetOverScanVTop(OverScanSettingValue.u16VTOverscan);
		AL_FLASH_SetOverScanVBottom(OverScanSettingValue.u16VBOverscan);
		AL_FLASH_SetOverScanHLeft(OverScanSettingValue.u16HLOverscan);
		AL_FLASH_SetOverScanHRight(OverScanSettingValue.u16HROverscan);
		AL_FLASH_SetOverScanHStart(OverScanSettingValue.u16HStart);
		AL_FLASH_SetOverScanVStart(OverScanSettingValue.u16VStart);

		FuncData->value = OverScanSettingValue.eScalerMode;
	}
}
//ID_FM_PicMode_OverScan_VTopOverScan,				//149
void Factory_PicMode_OverScan_VTopOverScan(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set funtion
		OverScanSettingValue.u16VTOverscan = FuncData->value;
		MID_DISP_DTVSetOverscan(bOverscanEn,OverScanSettingValue.u16VTOverscan,OverScanSettingValue.u16VBOverscan,\
			OverScanSettingValue.u16HLOverscan,OverScanSettingValue.u16HROverscan);
		//Write back all data
		AL_FLASH_SetOverScanVTop(OverScanSettingValue.u16VTOverscan);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetOverScanVTop();
	}
}
//ID_FM_PicMode_OverScan_VBottomOverScan,				//150
void Factory_PicMode_OverScan_VBottomOverScan(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set funtion
		OverScanSettingValue.u16VBOverscan = FuncData->value;
		MID_DISP_DTVSetOverscan(bOverscanEn,OverScanSettingValue.u16VTOverscan,OverScanSettingValue.u16VBOverscan,\
			OverScanSettingValue.u16HLOverscan,OverScanSettingValue.u16HROverscan);
		//Write back all data
		AL_FLASH_SetOverScanVBottom(OverScanSettingValue.u16VBOverscan);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetOverScanVBottom();
	}
}
//ID_FM_PicMode_OverScan_HLeftOverScan,				//151
void Factory_PicMode_OverScan_HLeftOverScan(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set funtion
		OverScanSettingValue.u16HLOverscan = FuncData->value;
		MID_DISP_DTVSetOverscan(bOverscanEn,OverScanSettingValue.u16VTOverscan,OverScanSettingValue.u16VBOverscan,\
			OverScanSettingValue.u16HLOverscan,OverScanSettingValue.u16HROverscan);
		//Write back all data
		AL_FLASH_SetOverScanHLeft(OverScanSettingValue.u16HLOverscan);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetOverScanHLeft();
	}
}
//ID_FM_PicMode_OverScan_HRightOverScan,				//152
void Factory_PicMode_OverScan_HRightOverScan(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set funtion
		OverScanSettingValue.u16HROverscan = FuncData->value;
		MID_DISP_DTVSetOverscan(bOverscanEn,OverScanSettingValue.u16VTOverscan,OverScanSettingValue.u16VBOverscan,\
			OverScanSettingValue.u16HLOverscan,OverScanSettingValue.u16HROverscan);
		//Write back all data
		AL_FLASH_SetOverScanHRight(OverScanSettingValue.u16HROverscan);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetOverScanHRight();
	}
}

BOOL IsOverScanExtSuouceAvalible(void)
{
	PLF_VideoID_t eResolution;
	MID_TVFE_GetCurrentVideoInfo(NULL, &eResolution);
	//printf("[DBG] eResolution = 0x%X\n", eResolution );

	input_type_t eInputType = INPUT_TYPE_DISABLE;
	MID_DISP_DTVGetInputType(&eInputType);
	//printf("[DBG] eInputType = 0x%X\n", eInputType );

	if(
		(eResolution == PLF_VIDEO_TIMING_ID_NO_SIGNAL) ||
		(eResolution == PLF_VIDEO_TIMING_ID_UNKNOW_FORMAT)
	)
		return FALSE;
	else{
		if
		(
			(eInputType == INPUT_TYPE_COMPONENT) ||
			(eInputType == INPUT_TYPE_SCART_RGB) ||
			(eInputType == INPUT_TYPE_PC) ||
			(eInputType == INPUT_TYPE_ATV) ||
			(eInputType == INPUT_TYPE_AV) ||
			(eInputType == INPUT_TYPE_SV) ||
			(eInputType == INPUT_TYPE_SCART_AUTO) ||
			(eInputType == INPUT_TYPE_SCART_CVBS) ||
			(eInputType == INPUT_TYPE_SCART_SVIDEO)
		)
			return TRUE;
		else
			return FALSE;
	}
}


void Factory_PicMode_OverScan_ExtSuouceHstart(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	PLF_VideoID_t eResolution;
	MID_TVFE_GetCurrentVideoInfo(NULL, &eResolution);

	if (bSet)
	{
		// UMF set function
		OverScanSettingValue.u16HStart= FuncData->value;
		_APP_GUIOBJ_FM_SetOverScan_HVStart(eResolution, 1); // 1: HStart
		//Write back all data
		AL_FLASH_SetOverScanHStart(OverScanSettingValue.u16HStart);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetOverScanHStart();
	}

	int nIndex;
	int nStartIndex = ID_FM_PicMode_OverScan_ExtSuouceHstart - ID_FM_PicMode_OverScan_ScaleMode;
	int nEndIndex = ID_FM_PicMode_OverScan_ExtSuouceVstart - ID_FM_PicMode_OverScan_ScaleMode;
	INT32 i32Index = 0;
	HWND FocusItem_Handle;

	if( !IsOverScanExtSuouceAvalible() )
	{
		//Disable items

		for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));
		}
	}else{
		//Enable items

		for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));
			GUI_FUNC_CALL(GEL_GetParam(g_fmFactorySetting_data, PARAM_CURRENT_INDEX, &i32Index));
			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			if (bSet && (i32Index == nIndex))
			{
				GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETFOCUSED, NULL));
			}
			else
			{
				GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));
			}
		}

	}
}

void Factory_PicMode_OverScan_ExtSuouceVstart(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	PLF_VideoID_t eResolution;
	MID_TVFE_GetCurrentVideoInfo(NULL, &eResolution);

	if (bSet)
	{
		OverScanSettingValue.u16VStart= FuncData->value;
		_APP_GUIOBJ_FM_SetOverScan_HVStart(eResolution, 0); // 0: VStart
		//Write back all data
		AL_FLASH_SetOverScanVStart(OverScanSettingValue.u16VStart);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetOverScanVStart();
	}
}


/*3.4 OverScan  End*/
	/*3.5 Curve Setting Start*/
	/*3.5 Curve Setting End*/
	/*3.6 Gamma TypeSetting Start*/
	/*3.6 Gamma TypeSetting End*/
	/*3.7 ACE_ColorFineTune  Start*/
	/*3.7 ACE_ColorFineTune  End*/
	/*3.8 Select PQ Start*/
	/*3.8 Select PQ End*/
	/*3.9 Dynamic Contrast  start */
//ID_FM_PicMode_DynCon_DynamicContrastlevel,		//160\"0: Off 1: Low2: Medium3: High"

void Factory_DynCon_DynamicContrastlevel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	DynamicContrast_t stDynamic;

	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	AL_FLASH_GetDynamicContrastALL(&stDynamic);

	if (bSet)
	{
		stDynamic.n_PicMode_DynCon_DynamicContrastlevel = FuncData->value;
		AL_FLASH_SetDynamicContrastALL(&stDynamic);

		//Set all relative value
		int nLevel = stDynamic.n_PicMode_DynCon_DynamicContrastlevel;
		MID_TVFE_SetPictureDynamicContrastEnable( nLevel ? 1 : 0);
		MID_TVFE_SetPictureContrastLevel( stDynamic.n_PicMode_DynCon_ContrastLevel[nLevel] );
		MID_TVFE_SetPictureChromaLevel( stDynamic.n_PicMode_DynCon_ChromaLevel[nLevel] );
		MID_TVFE_SetPictureAlphaMode1( stDynamic.n_PicMode_DynCon_Alphamode1[nLevel] );
		MID_TVFE_SetPictureAlphaMode2( stDynamic.n_PicMode_DynCon_Alphamode2[nLevel] );
		MID_TVFE_SetPictureAlphaMode3( stDynamic.n_PicMode_DynCon_Alphamode3[nLevel] );
		MID_TVFE_SetPictureAlphaMode4( stDynamic.n_PicMode_DynCon_Alphamode4[nLevel] );

		// Update all itesms
		UpdateNodeFunctionContent(ID_FM_PicMode_DynCon_DynamicContrastlevel, 0, 0);
	}
	else
	{
		// UMF get function
		FuncData->value = stDynamic.n_PicMode_DynCon_DynamicContrastlevel;
	}

	// Enable/Disable relative items
	int nIndex;
	int nStartIndex = ID_FM_PicMode_DynCon_ContrastLevel - ID_FM_PicMode_DynCon_DynamicContrastlevel;
	int nEndIndex = ID_FM_PicMode_DynCon_Alphamode4 - ID_FM_PicMode_DynCon_DynamicContrastlevel;
	HWND FocusItem_Handle;
	if(stDynamic.n_PicMode_DynCon_DynamicContrastlevel == FALSE)
	{
		//Disable items
		for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));
		}

	}else{

		//Enable items
		for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));
		}
	}

}

void Factory_DynCon_Contrast_Level(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	DynamicContrast_t stDynamic;
	AL_FLASH_GetDynamicContrastALL(&stDynamic);
	int nLevel = stDynamic.n_PicMode_DynCon_DynamicContrastlevel;

	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stDynamic.n_PicMode_DynCon_ContrastLevel[nLevel] = FuncData->value;
		AL_FLASH_SetDynamicContrastALL(&stDynamic);
		MID_TVFE_SetPictureContrastLevel(FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = stDynamic.n_PicMode_DynCon_ContrastLevel[nLevel];
	}

}

void Factory_DynCon_Chroma_Level(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	DynamicContrast_t stDynamic;
	AL_FLASH_GetDynamicContrastALL(&stDynamic);
	int nLevel = stDynamic.n_PicMode_DynCon_DynamicContrastlevel;

	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stDynamic.n_PicMode_DynCon_ChromaLevel[nLevel] = FuncData->value;
		AL_FLASH_SetDynamicContrastALL(&stDynamic);
		MID_TVFE_SetPictureChromaLevel(FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = stDynamic.n_PicMode_DynCon_ChromaLevel[nLevel];
	}

}

void Factory_DynCon_Alpha_Mode_1(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	DynamicContrast_t stDynamic;
	AL_FLASH_GetDynamicContrastALL(&stDynamic);
	int nLevel = stDynamic.n_PicMode_DynCon_DynamicContrastlevel;

	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stDynamic.n_PicMode_DynCon_Alphamode1[nLevel] = FuncData->value;
		AL_FLASH_SetDynamicContrastALL(&stDynamic);
		MID_TVFE_SetPictureAlphaMode1(FuncData->value);

	}
	else
	{
		// UMF get function
		FuncData->value = stDynamic.n_PicMode_DynCon_Alphamode1[nLevel];
	}

}
void Factory_DynCon_Alpha_Mode_2(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	DynamicContrast_t stDynamic;
	AL_FLASH_GetDynamicContrastALL(&stDynamic);
	int nLevel = stDynamic.n_PicMode_DynCon_DynamicContrastlevel;

	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stDynamic.n_PicMode_DynCon_Alphamode2[nLevel] = FuncData->value;
		AL_FLASH_SetDynamicContrastALL(&stDynamic);
		MID_TVFE_SetPictureAlphaMode2(FuncData->value);

	}
	else
	{
		// UMF get function
		FuncData->value = stDynamic.n_PicMode_DynCon_Alphamode2[nLevel];
	}

}



void Factory_DynCon_Alpha_Mode_3(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	DynamicContrast_t stDynamic;
	AL_FLASH_GetDynamicContrastALL(&stDynamic);
	int nLevel = stDynamic.n_PicMode_DynCon_DynamicContrastlevel;


	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stDynamic.n_PicMode_DynCon_Alphamode3[nLevel] = FuncData->value;
		AL_FLASH_SetDynamicContrastALL(&stDynamic);
		MID_TVFE_SetPictureAlphaMode3(FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = stDynamic.n_PicMode_DynCon_Alphamode3[nLevel];
	}

}

void Factory_DynCon_Alpha_Mode_4(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	DynamicContrast_t stDynamic;
	AL_FLASH_GetDynamicContrastALL(&stDynamic);
	int nLevel = stDynamic.n_PicMode_DynCon_DynamicContrastlevel;

	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stDynamic.n_PicMode_DynCon_Alphamode4[nLevel] = FuncData->value;
		AL_FLASH_SetDynamicContrastALL(&stDynamic);
		MID_TVFE_SetPictureAlphaMode4(FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = stDynamic.n_PicMode_DynCon_Alphamode4[nLevel];
	}

}

void Factory_DynCon_Flesh_Tone_Level(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	DynamicContrast_t stDynamic;
	AL_FLASH_GetDynamicContrastALL(&stDynamic);


	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		stDynamic.n_PicMode_DynCon_FleshToneLevel = FuncData->value;
		AL_FLASH_SetDynamicContrastALL(&stDynamic);
		MID_TVFE_SetPictureFleshToneLevel(FuncData->value);

	}
	else
	{
		// UMF get function
		FuncData->value = stDynamic.n_PicMode_DynCon_FleshToneLevel;
	}

}


	/*3.9 Dynamic Contrast  End */
	/*3.10 ColorLUT start */
	/*3.10 ColorLUT End*/
/*3.11 VIP  Start*/
//ID_FM_PicMode_HDMI_RGB_Range,				//174
void Factory_PicMode_HDMI_RGB_Range(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		AL_FLASH_SetHDMIRGBRange((INT16) FuncData->value);
		MID_DISP_DTVSetHdmiRGBRange(FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetHDMIRGBRange();
	}
}
/*3.11 VIP  End*/

/*3.5 Curve Setting Start*/
//ID_FM_PicMode_Curve_Source,						//153
enum{
	CURVE_TYPE_BRIGHTNESS,
	CURVE_TYPE_CONTRAST,
	CURVE_TYPE_SHARPNESS,
	CURVE_TYPE_SATURATION,
	CURVE_TYPE_HUE
};

void Factory_PicMode_Curve_Source(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	// do as  Factory_PicMode_Pic_Mode
	if (bSet)
	{
		if(gbIsFMRightKey || gbIsFMLeftKey)
		{
			if(g_bNeedReopenCurveSetting != TRUE)//Avoid setting app source more times
			{
				g_bNeedReopenCurveSetting = TRUE;
				if(gbIsFMRightKey)
				{
					APP_GUIOBJ_Source_Set_Next_Pre_AppSource(TRUE);
				}
				else if(gbIsFMLeftKey)
				{
					APP_GUIOBJ_Source_Set_Next_Pre_AppSource(FALSE);
				}
			}
		}
	}
	else
	{
		// UMF get function
		GUIResult_e dRet = 0;
		UINT8 u8Count = 0;
		HWND hItem;

		APP_Source_Type_t eAPPSrcType = APP_SOURCE_MAX;
    	APP_GUIOBJ_Source_GetCurrSource(&eAPPSrcType);

		APP_GUIOBJ_Source_SourceMapping_Table(g_stSourceConfigTable_New,&g_u16SourceConfigTable_Size_New);
		for (u8Count = 0; u8Count < g_u16SourceConfigTable_Size_New; u8Count++)
		{
			if(g_stSourceConfigTable_New[u8Count].eSourceType == eAPPSrcType)
			{
				Source_String[0] = g_stSourceConfigTable_New[u8Count].dOSDStrID;
				break;
			}
		}
		dRet = GEL_GetHandle(pWindowControl,
				TV_E_IDC_TEXT_TextBox_01,&hItem);
		dRet = GEL_SetParam(hItem, PARAM_STATIC_STRING, &Source_String);
		dRet = GEL_SendMsg(hItem, WM_PAINT, 0);
		GUI_FUNC_CALL(GEL_UpdateOSD());
		GUI_FUNC_CALL(dRet);
	}
}

static void Set_None_Linear_Curve_Range_Min(int nMin)
{
	L_PicMode_Curve_CurvePoint0[0].nMin=\
	L_PicMode_Curve_CurvePoint25[0].nMin =\
	L_PicMode_Curve_CurvePoint50[0].nMin =\
	L_PicMode_Curve_CurvePoint75[0].nMin =\
	L_PicMode_Curve_CurvePoint100[0].nMin = nMin;
}

static void Set_None_Linear_Curve_Range_Max(int nMax)
{
	L_PicMode_Curve_CurvePoint0[0].nMax =\
	L_PicMode_Curve_CurvePoint25[0].nMax =\
	L_PicMode_Curve_CurvePoint50[0].nMax =\
	L_PicMode_Curve_CurvePoint75[0].nMax =\
	L_PicMode_Curve_CurvePoint100[0].nMax = nMax;
}

static void Update_None_Linear_Curve_Range(UINT32 nCurveType)
{
	switch(nCurveType)
	{
		case CURVE_TYPE_BRIGHTNESS:
			Set_None_Linear_Curve_Range_Min(CURVE_TYPE_BRIGHTNESS_MIN);
			Set_None_Linear_Curve_Range_Max(CURVE_TYPE_BRIGHTNESS_MAX);
			break;

		case CURVE_TYPE_CONTRAST:
			Set_None_Linear_Curve_Range_Min(CURVE_TYPE_CONTRAST_MIN);
			Set_None_Linear_Curve_Range_Max(CURVE_TYPE_CONTRAST_MAX);
			break;

		case CURVE_TYPE_SHARPNESS:
			Set_None_Linear_Curve_Range_Min(CURVE_TYPE_SHARPNESS_MIN);
			Set_None_Linear_Curve_Range_Max(CURVE_TYPE_SHARPNESS_MAX);
			break;

		case CURVE_TYPE_SATURATION:
			Set_None_Linear_Curve_Range_Min(CURVE_TYPE_SATURATION_MIN);
			Set_None_Linear_Curve_Range_Max(CURVE_TYPE_SATURATION_MAX);
			break;

		case CURVE_TYPE_HUE:
			Set_None_Linear_Curve_Range_Min(CURVE_TYPE_HUE_MIN);
			Set_None_Linear_Curve_Range_Max(CURVE_TYPE_HUE_MAX);
			break;

		default:
			break;
	}
}


//ID_FM_PicMode_Curve_CurveType,					//154
void Factory_PicMode_Curve_CurveType(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int currsource = (INT32)AL_FLASH_GetPictureSource();
	int currbrightness = 0;
	int currcontrast = 0;
	int currsharpness = 0;
	int currsaturation = 0;
	int currhue = 0;

	int mode = (INT32)AL_FLASH_GetPictureMode(currsource);

	APP_SETTING_FMCurveSettingTab_t bCurveSetting;
	AL_FLASH_GetCurveSetting(&bCurveSetting);

	if (bSet)
	{
		currbrightness	= AL_FLASH_GetPictureBrightness(currsource,mode);
		currcontrast 	= AL_FLASH_GetPictureContrast(currsource,mode);
		currsharpness 	= AL_FLASH_GetPictureSharpness(currsource,mode);
		currsaturation 	= AL_FLASH_GetPictureColor(currsource,mode);
		currhue 		= AL_FLASH_GetPictureTint(currsource,mode);
		bCURVEType = FuncData->value;
		switch(bCURVEType)
		{
			case CURVE_TYPE_BRIGHTNESS:
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_BRIGHTNESS,currbrightness);
				break;

			case CURVE_TYPE_CONTRAST:
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_CONTRAST,currcontrast);
				break;

			case CURVE_TYPE_SHARPNESS:
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_SHARPNESS,currsharpness);
				break;

			case CURVE_TYPE_SATURATION:
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_SATURTUION,currsaturation);
				break;

			case CURVE_TYPE_HUE:
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_HUE,currhue);
				break;

			default:
				break;
		}

		// Update Max and min range
		Update_None_Linear_Curve_Range(bCURVEType);

		// Update all items
		UpdateNodeFunctionContent(ID_FM_PicMode_Curve_Source,0,0);
	}
	else
	{
		// Update Max and min range
		Update_None_Linear_Curve_Range(bCURVEType);
		// UMF get function
		FuncData->value = bCURVEType;
	}
}

//ID_FM_PicMode_Curve_CurvePoint0,					//155
 void Factory_PicMode_Curve_CurvePoint0(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int currsource = (INT32)AL_FLASH_GetPictureSource();
	int currbrightness = 0;
	int currcontrast = 0;
	int currsharpness = 0;
	int currsaturation = 0;
	int currhue = 0;

	int mode = (INT32)AL_FLASH_GetPictureMode(currsource);

	APP_SETTING_FMCurveSettingTab_t bCurveSetting;
	AL_FLASH_GetCurveSetting(&bCurveSetting);

	if (bSet)
	{
		currbrightness	= AL_FLASH_GetPictureBrightness(currsource,mode);
		currcontrast 	= AL_FLASH_GetPictureContrast(currsource,mode);
		currsharpness 	= AL_FLASH_GetPictureSharpness(currsource,mode);
		currsaturation 	= AL_FLASH_GetPictureColor(currsource,mode);
		currhue 		= AL_FLASH_GetPictureTint(currsource,mode);

		switch(bCURVEType)
		{
			case CURVE_TYPE_BRIGHTNESS:
				bCurveSetting.brightness.CurvePoint0 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_BRIGHTNESS,currbrightness);
				break;
			case CURVE_TYPE_CONTRAST:
				bCurveSetting.contrast.CurvePoint0 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_CONTRAST,currcontrast);
				break;
			case CURVE_TYPE_SHARPNESS:
				bCurveSetting.Sharpness.CurvePoint0 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_SHARPNESS,currsharpness);
				break;
			case CURVE_TYPE_SATURATION:
				bCurveSetting.Saturation.CurvePoint0 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_SATURTUION,currsaturation);
				break;
			case CURVE_TYPE_HUE:
				bCurveSetting.Hue.CurvePoint0 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_HUE,currhue);
				break;
			default:
				break;
		}
		// Update all items
		//UpdateNodeFunctionContent(ID_FM_PicMode_Curve_Source,0,0);
	}
	else
	{
		switch(bCURVEType)
		{
			case CURVE_TYPE_BRIGHTNESS:
				FuncData->value = bCurveSetting.brightness.CurvePoint0;
				break;
			case CURVE_TYPE_CONTRAST:
				FuncData->value = bCurveSetting.contrast.CurvePoint0;
				break;
			case CURVE_TYPE_SHARPNESS:
				FuncData->value = bCurveSetting.Sharpness.CurvePoint0;
				break;
			case CURVE_TYPE_SATURATION:
				FuncData->value = bCurveSetting.Saturation.CurvePoint0;
				break;
			case CURVE_TYPE_HUE:
				FuncData->value = bCurveSetting.Hue.CurvePoint0;
				break;
			default:
				break;
		}

	}
}
//ID_FM_PicMode_Curve_CurvePoint25,				//156
void Factory_PicMode_Curve_CurvePoint25(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int currsource = (INT32)AL_FLASH_GetPictureSource();
	int currbrightness = 0;
	int currcontrast = 0;
	int currsharpness = 0;
	int currsaturation = 0;
	int currhue = 0;

	int mode = (INT32)AL_FLASH_GetPictureMode(currsource);

	APP_SETTING_FMCurveSettingTab_t bCurveSetting;
	AL_FLASH_GetCurveSetting(&bCurveSetting);

	if (bSet)
	{
		currbrightness	= AL_FLASH_GetPictureBrightness(currsource,mode);
		currcontrast 	= AL_FLASH_GetPictureContrast(currsource,mode);
		currsharpness 	= AL_FLASH_GetPictureSharpness(currsource,mode);
		currsaturation 	= AL_FLASH_GetPictureColor(currsource,mode);
		currhue 		= AL_FLASH_GetPictureTint(currsource,mode);

		switch(bCURVEType)
		{
			case CURVE_TYPE_BRIGHTNESS:
				bCurveSetting.brightness.CurvePoint25 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_BRIGHTNESS,currbrightness);
				break;
			case CURVE_TYPE_CONTRAST:
				bCurveSetting.contrast.CurvePoint25 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_CONTRAST,currcontrast);
				break;
			case CURVE_TYPE_SHARPNESS:
				bCurveSetting.Sharpness.CurvePoint25 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_SHARPNESS,currsharpness);
				break;
			case CURVE_TYPE_SATURATION:
				bCurveSetting.Saturation.CurvePoint25 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_SATURTUION,currsaturation);
				break;
			case CURVE_TYPE_HUE:
				bCurveSetting.Hue.CurvePoint25 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_HUE,currhue);
				break;
			default:
				break;
		}
		//UpdateNodeFunctionContent(ID_FM_PicMode_Curve_Source,0,0);
	}
	else
	{
		switch(bCURVEType)
		{
			case CURVE_TYPE_BRIGHTNESS:
				FuncData->value = bCurveSetting.brightness.CurvePoint25;
				break;
			case CURVE_TYPE_CONTRAST:
				FuncData->value = bCurveSetting.contrast.CurvePoint25;
				break;
			case CURVE_TYPE_SHARPNESS:
				FuncData->value = bCurveSetting.Sharpness.CurvePoint25;
				break;
			case CURVE_TYPE_SATURATION:
				FuncData->value = bCurveSetting.Saturation.CurvePoint25;
				break;
			case CURVE_TYPE_HUE:
				FuncData->value = bCurveSetting.Hue.CurvePoint25;
				break;
			default:
				break;
		}

	}
}
//ID_FM_PicMode_Curve_CurvePoint50,				//157
void Factory_PicMode_Curve_CurvePoint50(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int currsource = (INT32)AL_FLASH_GetPictureSource();
	int currbrightness = 0;
	int currcontrast = 0;
	int currsharpness = 0;
	int currsaturation = 0;
	int currhue = 0;

	int mode = (INT32)AL_FLASH_GetPictureMode(currsource);

	APP_SETTING_FMCurveSettingTab_t bCurveSetting;
	AL_FLASH_GetCurveSetting(&bCurveSetting);

	if (bSet)
	{
		currbrightness	= AL_FLASH_GetPictureBrightness(currsource,mode);
		currcontrast 	= AL_FLASH_GetPictureContrast(currsource,mode);
		currsharpness 	= AL_FLASH_GetPictureSharpness(currsource,mode);
		currsaturation 	= AL_FLASH_GetPictureColor(currsource,mode);
		currhue 		= AL_FLASH_GetPictureTint(currsource,mode);


		switch(bCURVEType)
		{
			case CURVE_TYPE_BRIGHTNESS:
				bCurveSetting.brightness.CurvePoint50 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_BRIGHTNESS,currbrightness);
				break;
			case CURVE_TYPE_CONTRAST:
				bCurveSetting.contrast.CurvePoint50 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_CONTRAST,currcontrast);
				break;
			case CURVE_TYPE_SHARPNESS:
				bCurveSetting.Sharpness.CurvePoint50 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_SHARPNESS,currsharpness);
				break;
			case CURVE_TYPE_SATURATION:
				bCurveSetting.Saturation.CurvePoint50 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_SATURTUION,currsaturation);
				break;
			case CURVE_TYPE_HUE:
				bCurveSetting.Hue.CurvePoint50 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_HUE,currhue);
				break;
			default:
				break;
		}

		//UpdateNodeFunctionContent(ID_FM_PicMode_Curve_Source,0,0);
	}
	else
	{
		switch(bCURVEType)
		{
			case CURVE_TYPE_BRIGHTNESS:
				FuncData->value = bCurveSetting.brightness.CurvePoint50;
				break;
			case CURVE_TYPE_CONTRAST:
				FuncData->value = bCurveSetting.contrast.CurvePoint50;
				break;
			case CURVE_TYPE_SHARPNESS:
				FuncData->value = bCurveSetting.Sharpness.CurvePoint50;
				break;
			case CURVE_TYPE_SATURATION:
				FuncData->value = bCurveSetting.Saturation.CurvePoint50;
				break;
			case CURVE_TYPE_HUE:
				FuncData->value = bCurveSetting.Hue.CurvePoint50;
				break;
			default:
				break;
		}

	}
}
//ID_FM_PicMode_Curve_CurvePoint75,				//158
 void Factory_PicMode_Curve_CurvePoint75(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int currsource = (INT32)AL_FLASH_GetPictureSource();
	int currbrightness = 0;
	int currcontrast = 0;
	int currsharpness = 0;
	int currsaturation = 0;
	int currhue = 0;

	int mode = (INT32)AL_FLASH_GetPictureMode(currsource);

	APP_SETTING_FMCurveSettingTab_t bCurveSetting;
	AL_FLASH_GetCurveSetting(&bCurveSetting);

	if (bSet)
	{
		currbrightness	= AL_FLASH_GetPictureBrightness(currsource,mode);
		currcontrast 	= AL_FLASH_GetPictureContrast(currsource,mode);
		currsharpness 	= AL_FLASH_GetPictureSharpness(currsource,mode);
		currsaturation 	= AL_FLASH_GetPictureColor(currsource,mode);
		currhue 		= AL_FLASH_GetPictureTint(currsource,mode);

		switch(bCURVEType)
		{
			case CURVE_TYPE_BRIGHTNESS:
				bCurveSetting.brightness.CurvePoint75 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_BRIGHTNESS,currbrightness);
				break;
			case CURVE_TYPE_CONTRAST:
				bCurveSetting.contrast.CurvePoint75 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_CONTRAST,currcontrast);
				break;
			case CURVE_TYPE_SHARPNESS:
				bCurveSetting.Sharpness.CurvePoint75 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_SHARPNESS,currsharpness);
				break;
			case CURVE_TYPE_SATURATION:
				bCurveSetting.Saturation.CurvePoint75 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_SATURTUION,currsaturation);
				break;
			case CURVE_TYPE_HUE:
				bCurveSetting.Hue.CurvePoint75 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_HUE,currhue);
				break;
			default:
				break;
		}

		//UpdateNodeFunctionContent(ID_FM_PicMode_Curve_Source,0,0);
	}
	else
	{
		switch(bCURVEType)
		{
			case CURVE_TYPE_BRIGHTNESS:
				FuncData->value = bCurveSetting.brightness.CurvePoint75;
				break;
			case CURVE_TYPE_CONTRAST:
				FuncData->value = bCurveSetting.contrast.CurvePoint75;
				break;
			case CURVE_TYPE_SHARPNESS:
				FuncData->value = bCurveSetting.Sharpness.CurvePoint75;
				break;
			case CURVE_TYPE_SATURATION:
				FuncData->value = bCurveSetting.Saturation.CurvePoint75;
				break;
			case CURVE_TYPE_HUE:
				FuncData->value = bCurveSetting.Hue.CurvePoint75;
				break;
			default:
				break;
		}
	}
}
//ID_FM_PicMode_Curve_CurvePoint100,				//159
void Factory_PicMode_Curve_CurvePoint100(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int currsource = (INT32)AL_FLASH_GetPictureSource();
	int currbrightness = 0;
	int currcontrast = 0;
	int currsharpness = 0;
	int currsaturation = 0;
	int currhue = 0;

	int mode = (INT32)AL_FLASH_GetPictureMode(currsource);

	APP_SETTING_FMCurveSettingTab_t bCurveSetting;
	AL_FLASH_GetCurveSetting(&bCurveSetting);

	if (bSet)
	{
		currbrightness	= AL_FLASH_GetPictureBrightness(currsource,mode);
		currcontrast 	= AL_FLASH_GetPictureContrast(currsource,mode);
		currsharpness 	= AL_FLASH_GetPictureSharpness(currsource,mode);
		currsaturation 	= AL_FLASH_GetPictureColor(currsource,mode);
		currhue 		= AL_FLASH_GetPictureTint(currsource,mode);

		switch(bCURVEType)
		{
			case CURVE_TYPE_BRIGHTNESS:
				bCurveSetting.brightness.CurvePoint100 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_BRIGHTNESS,currbrightness);
				break;
			case CURVE_TYPE_CONTRAST:
				bCurveSetting.contrast.CurvePoint100 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_CONTRAST,currcontrast);
				break;
			case CURVE_TYPE_SHARPNESS:
				bCurveSetting.Sharpness.CurvePoint100 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_SHARPNESS,currsharpness);
				break;
			case CURVE_TYPE_SATURATION:
				bCurveSetting.Saturation.CurvePoint100 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_SATURTUION,currsaturation);
				break;
			case CURVE_TYPE_HUE:
				bCurveSetting.Hue.CurvePoint100 = (UINT16) FuncData->value;
				AL_FLASH_SetCurveSetting(&bCurveSetting);
				APP_Video_OSD_PQSet(APP_VIDEO_OSD_ITEM_HUE,currhue);
				break;
			default:
				break;
		}

		//UpdateNodeFunctionContent(ID_FM_PicMode_Curve_Source,0,0);
	}
	else
	{
		switch(bCURVEType)
		{
			case CURVE_TYPE_BRIGHTNESS:
				FuncData->value = bCurveSetting.brightness.CurvePoint100;
				break;
			case CURVE_TYPE_CONTRAST:
				FuncData->value = bCurveSetting.contrast.CurvePoint100;
				break;
			case CURVE_TYPE_SHARPNESS:
				FuncData->value = bCurveSetting.Sharpness.CurvePoint100;
				break;
			case CURVE_TYPE_SATURATION:
				FuncData->value = bCurveSetting.Saturation.CurvePoint100;
				break;
			case CURVE_TYPE_HUE:
				FuncData->value = bCurveSetting.Hue.CurvePoint100;
				break;
			default:
				break;
		}

	}
}

/**/
void Factory_Color_LUT_Region(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int currsrc=0;
	APP_GUIOBJ_Source_GetCurrSource((APP_Source_Type_t *)&currsrc);
	if (bSet)
	{
		bCOLORLUTregion = FuncData->value;
		AL_FLASH_SetColorLUTRegion(currsrc,(UINT8) FuncData->value);

		// Update all items
		UpdateNodeFunctionContent(ID_FM_PicMode_CoLUT_ColorLUTRegion,0,0);

	}
	else
	{
		// UMF get function
		FuncData->value = (UINT32)bCOLORLUTregion;
	}
}

void Factory_Enable(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if(bSet)
	{
		//Set function
		APP_Video_SetColorLUTSetting(LUT_FUN_ENABLE,bCOLORLUTregion,pValue->value,FALSE);
	}
	else
	{
		//Get function
		pValue->value=APP_Video_GetColorLUTSetting(LUT_FUN_ENABLE,bCOLORLUTregion);
	}

	// Enable/Disable relative items
	int nIndex;
	int nStartIndex = ID_FM_PicMode_CoLUT_RotGain - ID_FM_PicMode_CoLUT_ColorLUTRegion;
	int nEndIndex = ID_FM_PicMode_CoLUT_LumaGain - ID_FM_PicMode_CoLUT_ColorLUTRegion;
	HWND FocusItem_Handle;
	if( APP_Video_GetColorLUTSetting(LUT_FUN_ENABLE,bCOLORLUTregion) == FALSE )
	{
		//Disable items
		for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));
		}

	}else{

		//Enable items
		for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));
		}
	}
}

void Factory_SetGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if(bSet)
	{
		//Set function
		APP_Video_SetColorLUTSetting(LUT_FUN_SAT_GAIN,bCOLORLUTregion,pValue->value,FALSE);
	}
	else
	{
		//Get function
		pValue->value=APP_Video_GetColorLUTSetting(LUT_FUN_SAT_GAIN,bCOLORLUTregion);
	}
}

/*void Factory_RotAngle(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	int  currsrc=0;
	int RegionIdx = 0;
	APP_GUIOBJ_Source_GetCurrSource((APP_Source_Type_t *)&currsrc);

	ColorLUT_t stColorLUTValue;
	UINT16 bRotAngle = 0;

	AL_FLASH_GetColorLUTALL(&stColorLUTValue);
	if (bSet)
	{
		stColorLUTValue.n_PicMode_CoLUT_HueGain[bCOLORLUTregion] = FuncData->value;
		AL_FLASH_SetColorLUTALL(&stColorLUTValue);
		for(RegionIdx = 0; RegionIdx<APP_ColorLUT_NUM; RegionIdx++)
		{
			MID_TVFE_SetColorLUT7Axis(RegionIdx,
				stColorLUTValue.n_PicMode_CoLUT_Enable[RegionIdx],
				stColorLUTValue.n_PicMode_CoLUT_HueGain[RegionIdx],
				stColorLUTValue.n_PicMode_CoLUT_SatGain[RegionIdx],
				stColorLUTValue.n_PicMode_CoLUT_YGain[RegionIdx],
				RegionIdx==6? TRUE : FALSE);
		}
	}
	else
	{
		// UMF get function
		bRotAngle = stColorLUTValue.n_PicMode_CoLUT_HueGain[bCOLORLUTregion];
		FuncData->value= (UINT16)bRotAngle;
	}
}*/

void FactoryRotGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if(bSet)
	{
		//Set function
		APP_Video_SetColorLUTSetting(LUT_FUN_HUE_GAIN,bCOLORLUTregion,pValue->value,FALSE);
	}
	else
	{
		//Get function
		pValue->value=APP_Video_GetColorLUTSetting(LUT_FUN_HUE_GAIN,bCOLORLUTregion);
	}
}


void Factory_LumaGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if(bSet)
	{
		//Set function
		APP_Video_SetColorLUTSetting(LUT_FUN_Y_GAIN,bCOLORLUTregion,pValue->value,FALSE);
	}
	else
	{
		//Get function
		pValue->value=APP_Video_GetColorLUTSetting(LUT_FUN_Y_GAIN,bCOLORLUTregion);
	}
}

void Factory_CoLUT_Make(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if(bSet)
	{
		//Set function
		APP_Video_SetColorLUTSetting(LUT_FUN_MAKE,bCOLORLUTregion,pValue->value,FALSE);
	}
	else
	{
		//Get function
		pValue->value=APP_Video_GetColorLUTSetting(LUT_FUN_MAKE,bCOLORLUTregion);
	}
}

void Factory_CoLUT_HueMin(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if(bSet)
	{
		//Set function
		APP_Video_SetColorLUTSetting(LUT_FUN_HUE_MIN,bCOLORLUTregion,pValue->value,FALSE);
	}
	else
	{
		//Get function
		pValue->value=APP_Video_GetColorLUTSetting(LUT_FUN_HUE_MIN,bCOLORLUTregion);
	}
}

void Factory_CoLUT_HueMax(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if(bSet)
	{
		//Set function
		APP_Video_SetColorLUTSetting(LUT_FUN_HUE_MAX,bCOLORLUTregion,pValue->value,FALSE);
	}
	else
	{
		//Get function
		pValue->value=APP_Video_GetColorLUTSetting(LUT_FUN_HUE_MAX,bCOLORLUTregion);
	}
}

void Factory_CoLUT_SatMin(BOOL bSet, BOOL path, FUNCTION_DATA * pValue, BOOL bDefault)
{
	L_CoLUT_SatMin[0].nMax = APP_Video_GetColorLUTSetting(LUT_FUN_SAT_MAX,bCOLORLUTregion);
	if(bSet)
	{
		//Set function
		APP_Video_SetColorLUTSetting(LUT_FUN_SAT_MIN,bCOLORLUTregion,pValue->value,FALSE);
	}
	else
	{
		//Get function
		pValue->value=APP_Video_GetColorLUTSetting(LUT_FUN_SAT_MIN,bCOLORLUTregion);
	}
}

void Factory_CoLUT_SatMax(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	L_CoLUT_SatMax[0].nMin = APP_Video_GetColorLUTSetting(LUT_FUN_SAT_MIN,bCOLORLUTregion);
	if(bSet)
	{
			APP_Video_SetColorLUTSetting(LUT_FUN_SAT_MAX,bCOLORLUTregion,pValue->value,FALSE);
	}
	else
	{
		//Get function
		pValue->value=APP_Video_GetColorLUTSetting(LUT_FUN_SAT_MAX,bCOLORLUTregion);
	}
}

void Factory_CoLUT_YMin(BOOL bSet, BOOL path, FUNCTION_DATA * pValue, BOOL bDefault)
{
	L_CoLUT_YMin[0].nMax= APP_Video_GetColorLUTSetting(LUT_FUN_Y_MAX,bCOLORLUTregion);
	if(bSet)
	{
		//Set function
		APP_Video_SetColorLUTSetting(LUT_FUN_Y_MIN,bCOLORLUTregion,pValue->value,FALSE);
	}
	else
	{
		//Get function
		pValue->value=APP_Video_GetColorLUTSetting(LUT_FUN_Y_MIN,bCOLORLUTregion);
	}
}

void Factory_CoLUT_YMax(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	L_CoLUT_YMax[0].nMin= APP_Video_GetColorLUTSetting(LUT_FUN_Y_MIN,bCOLORLUTregion);
	if(bSet)
	{
		//Set function
		APP_Video_SetColorLUTSetting(LUT_FUN_Y_MAX,bCOLORLUTregion,pValue->value,FALSE);
	}
	else
	{
		//Get function
		pValue->value=APP_Video_GetColorLUTSetting(LUT_FUN_Y_MAX,bCOLORLUTregion);
	}
}

void Factory_CoLUT_SmoothLevel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if(bSet)
	{
		//Set function
		APP_Video_SetColorLUTSetting(LUT_FUN_SMOOTH_LEVEL,bCOLORLUTregion,pValue->value,FALSE);
	}
	else
	{
		//Get function
		pValue->value=APP_Video_GetColorLUTSetting(LUT_FUN_SMOOTH_LEVEL,bCOLORLUTregion);
	}
}

	/*3.6 Gamma TypeSetting Start*/
//	ID_FM_PicMode_GammaTypeSetting,			//44
void Factory_PicMode_GammaSwitch(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		AL_FLASH_SetGammaType((UINT8) FuncData->value);

		int Mode = (INT32)AL_FLASH_GetWBMode();

		if((UINT8)FuncData->value) //enable
		{
			MID_TVFE_SetColorTmpGammaTableIndex( AL_FLASH_GetWBGammaTable(Mode) );
		}
		else // disable
		{
			MID_TVFE_SetColorTmpGammaTableIndex(0xff);
		}
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetGammaType();
	}
}
	/*3.6 Gamma TypeSetting Start*/

/*3. Picture Mode 	End*/



/*4. CVD2 Setting  	Start */
// For CVD2 relative functions use
//ID_FM_CVD2Set_Brightness
void Factory_CVD2Set_Brightness(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	CVD2VideoDecoderSetting_t stCVD2Value ;
	AL_FLASH_GetCVD2PQData(&stCVD2Value);

	if (bSet)
	{
		// Set Function
		stCVD2Value.Brightness = FuncData->value;
		MID_TVFE_SetCvd2Brightness(FuncData->value);
		AL_FLASH_SetCVD2PQData(&stCVD2Value);
	}
	else
	{
		// Get Function
		FuncData->value = stCVD2Value.Brightness;
	}
}

//ID_FM_CVD2Set_Contrast
void Factory_CVD2Set_Contrast(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	CVD2VideoDecoderSetting_t stCVD2Value ;
	AL_FLASH_GetCVD2PQData(&stCVD2Value);

	if (bSet)
	{
		// Set Function
		stCVD2Value.Contrast = FuncData->value;
		MID_TVFE_SetCvd2Contrast(FuncData->value);
		AL_FLASH_SetCVD2PQData(&stCVD2Value);
	}
	else
	{
		// Get Function
		FuncData->value = stCVD2Value.Contrast;
	}
}

//ID_FM_CVD2Set_LDLY
void Factory_CVD2Set_LDLY(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	CVD2VideoDecoderSetting_t stCVD2Value ;
	AL_FLASH_GetCVD2PQData(&stCVD2Value);
	if (bSet)
	{
		// Set Function
		stCVD2Value.Ldly= FuncData->value;
		MID_TVFE_SetCvd2LDLY(FuncData->value);
		AL_FLASH_SetCVD2PQData(&stCVD2Value);
	}
	else
	{
		// Get Function
		FuncData->value = stCVD2Value.Ldly;
	}
}

//ID_FM_CVD2Set_Filter
void Factory_CVD2Set_Filter(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	CVD2VideoDecoderSetting_t stCVD2Value ;
	AL_FLASH_GetCVD2PQData(&stCVD2Value);

	if (bSet)
	{
		// Set Function
		stCVD2Value.Filter= FuncData->value;
		MID_TVFE_SetCvd2Filter(FuncData->value);
		AL_FLASH_SetCVD2PQData(&stCVD2Value);
	}
	else
	{
		// Get Function
		FuncData->value = stCVD2Value.Filter;
	}
}
/*4. CVD2 Setting  	End */


/*5. Audio  	start */
	/*5.1 PowerLimit Start*/
//ID_FM_Audio_PowLim_PLC,
void Factory_Audio_PowLim_PLC(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	if (bSet)
	{
		// Write back all data
		g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_PLC = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PowerLimiter.n_Audio_PowLim_PLC),
			sizeof(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_PLC),
			&(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_PLC));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PowerLimiter.n_Audio_PowLim_PLC),
			sizeof(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_PLC));
		*/
		// UMF set function
		MID_Audio_PLC(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_PLC,\
			g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_SpAttack,\
			g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_HpAttack,\
			g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_LoAttack\
		);
	}
	else
	{
		// UMF get function
		FuncData->value = g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_PLC;
	}

	// Enable/Disable relative items
	int nIndex;
	HWND FocusItem_Handle;
	if(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_PLC == FALSE)
	{
		//Disable items

		// 1) Sp Attack
		nIndex = ID_FM_Audio_PowLim_SpAttack - ID_FM_Audio_PowLim_PLC;
		GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

		GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
		GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));

		// 2) Hp Attack
		nIndex = ID_FM_Audio_PowLim_HpAttack - ID_FM_Audio_PowLim_PLC;
		GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

		GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
		GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));

		// 3) Lo Attack
		nIndex = ID_FM_Audio_PowLim_LoAttack - ID_FM_Audio_PowLim_PLC;
		GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

		GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
		GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));

	}else{

		//Enable items

		// 1) Sp Attack
		nIndex = ID_FM_Audio_PowLim_SpAttack - ID_FM_Audio_PowLim_PLC;
		GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

		GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
		GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));

		// 2) Hp Attack
		nIndex = ID_FM_Audio_PowLim_HpAttack - ID_FM_Audio_PowLim_PLC;
		GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

		GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
		GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));

		// 3) Lo Attack
		nIndex = ID_FM_Audio_PowLim_LoAttack - ID_FM_Audio_PowLim_PLC;
		GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

		GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
		GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));

	}
}

//ID_FM_Audio_PowLim_SpAttack,
void Factory_Audio_PowLim_SpAttack(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// Write back all data
		g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_SpAttack= FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PowerLimiter.n_Audio_PowLim_SpAttack),
			sizeof(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_SpAttack),
			&(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_SpAttack));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PowerLimiter.n_Audio_PowLim_SpAttack),
			sizeof(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_SpAttack));
		*/
		// UMF set function
		MID_Audio_PLC(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_PLC,\
			g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_SpAttack,\
			g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_HpAttack,\
			g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_LoAttack\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_SpAttack ;
	}
}

//ID_FM_Audio_PowLim_HpAttack,
void Factory_Audio_PowLim_HpAttack(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// Write back all data
		g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_HpAttack= FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PowerLimiter.n_Audio_PowLim_HpAttack),
			sizeof(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_HpAttack),
			&(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_HpAttack));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PowerLimiter.n_Audio_PowLim_HpAttack),
			sizeof(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_HpAttack));
		*/
		// UMF set function
		MID_Audio_PLC(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_PLC,\
			g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_SpAttack,\
			g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_HpAttack,\
			g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_LoAttack\
		);
	}
	else
	{
		// UMF get function
		 FuncData->value=g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_HpAttack;
	}
}

//ID_FM_Audio_PowLim_LoAttack,
void Factory_Audio_PowLim_LoAttack(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// Write back all data
		g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_LoAttack= FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PowerLimiter.n_Audio_PowLim_LoAttack),
			sizeof(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_LoAttack),
			&(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_LoAttack));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PowerLimiter.n_Audio_PowLim_LoAttack),
			sizeof(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_LoAttack));
		*/
		// UMF set function
		MID_Audio_PLC(g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_PLC,\
			g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_SpAttack,\
			g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_HpAttack,\
			g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_LoAttack\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.PowerLimiter.n_Audio_PowLim_LoAttack;
	}
}
	/*5.1 PowerLimit End*/

	/*5.2 Audio Effect Start*/

//ID_FM_Audio_Effect_Mode,
void Factory_Audio_Effect_Mode(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		//Write back all data
		g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_Mode= FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.AudioEffect.n_Audio_Effect_Mode),
			sizeof(g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_Mode),
			&(g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_Mode));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.AudioEffect.n_Audio_Effect_Mode),
			sizeof(g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_Mode));
		*/
		// UMF set function
		 MID_Audio_EffectMode(\
		g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_Mode,\
		g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_DirectGain,\
		g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_ReverbGain,\
		g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_AmbianceGain\
		 );
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_Mode ;
	}

	// Enable/Disable relative items
	int nIndex;
	HWND FocusItem_Handle;
	if(g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_Mode == FALSE)
	{
		//Disable items

		// 1) Direct Gain
		nIndex = ID_FM_Audio_Effect_DirectGain - ID_FM_Audio_Effect_Mode;
		GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

		GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
		GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));

		// 2) Reverb Gain
		nIndex = ID_FM_Audio_Effect_ReverbGain - ID_FM_Audio_Effect_Mode;
		GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

		GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
		GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));

		// 3) Ambiance Gain
		nIndex = ID_FM_Audio_Effect_AmbianceGain - ID_FM_Audio_Effect_Mode;
		GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

		GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
		GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));

	}else{

		//Enable items

		// 1) Direct Gain
		nIndex = ID_FM_Audio_Effect_DirectGain - ID_FM_Audio_Effect_Mode;
		GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

		GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
		GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));

		// 2) Reverb Gain
		nIndex = ID_FM_Audio_Effect_ReverbGain - ID_FM_Audio_Effect_Mode;
		GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

		GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
		GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));

		// 3) Ambiance Gain
		nIndex = ID_FM_Audio_Effect_AmbianceGain - ID_FM_Audio_Effect_Mode;
		GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

		GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
		GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));

	}
}

//ID_FM_Audio_Effect_DirectGain,
void Factory_Audio_Effect_DirectGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// Write back all data
		g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_DirectGain= FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.AudioEffect.n_Audio_Effect_DirectGain),
			sizeof(g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_DirectGain),
			&(g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_DirectGain));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.AudioEffect.n_Audio_Effect_DirectGain),
			sizeof(g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_DirectGain));
		*/
		// UMF set function
		MID_Audio_EffectMode(\
			g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_Mode,\
			g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_DirectGain,\
			g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_ReverbGain,\
			g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_AmbianceGain\
		 );

	}
	else
	{
		// UMF get function
		 FuncData->value=g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_DirectGain;
	}
}
//ID_FM_Audio_Effect_ReverbGain,
void Factory_Audio_Effect_ReverbGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{

		// Write back all data
		g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_ReverbGain= FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.AudioEffect.n_Audio_Effect_ReverbGain),
			sizeof(g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_ReverbGain),
			&(g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_ReverbGain));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.AudioEffect.n_Audio_Effect_ReverbGain),
			sizeof(g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_ReverbGain));
		*/
		// UMF set function
		MID_Audio_EffectMode(\
			g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_Mode,\
			g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_DirectGain,\
			g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_ReverbGain,\
			g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_AmbianceGain\
		 );

	}
	else
	{
		// UMF get function
		 FuncData->value=g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_ReverbGain;
	}
}

//ID_FM_Audio_Effect_AmbianceGain,
void Factory_Audio_Effect_AmbianceGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// Write back all data
		g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_AmbianceGain= FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.AudioEffect.n_Audio_Effect_AmbianceGain),
			sizeof(g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_AmbianceGain),
			&(g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_AmbianceGain));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.AudioEffect.n_Audio_Effect_AmbianceGain),
			sizeof(g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_AmbianceGain));
		*/
		
		// UMF set function
		MID_Audio_EffectMode(\
			g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_Mode,\
			g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_DirectGain,\
			g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_ReverbGain,\
			g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_AmbianceGain\
		 );

	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.AudioEffect.n_Audio_Effect_AmbianceGain;
	}
}
	/*5.2 Audio Effect End*/

	/*5.3 SRS Surround     start */
//ID_FM_Audio_SRSS_SRSMode,
void Factory_Audio_SRSS_SRSMode(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// Write back all data
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SRSMode= FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_SRSMode),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SRSMode),
			&(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SRSMode));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_SRSMode),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SRSMode));
		*/
		// UMF set function
		MID_Audio_SRSSurround(\
			g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SRSMode,\
			g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_InputGain,\
			g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_OutputGain,\
			g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SurroundLevel,\
			g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel,\
			g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassLevel,\
			g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel,\
			g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize\
		);
	}
	else
	{
		// UMF get function
		 FuncData->value=g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SRSMode;
	}


	// Enable/Disable relative items
		int nIndex;
		HWND FocusItem_Handle;
		if(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SRSMode == FALSE)
		{
			//Disable items
			for(
				nIndex = ID_FM_Audio_SRSS_InputGain - ID_FM_Audio_SRSS_SRSMode;
				nIndex <= ID_FM_Audio_SRSS_TruBassSpeakerSize- ID_FM_Audio_SRSS_SRSMode;
				nIndex++)
			{
				GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

				GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
				GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));
			}

		}else{

			//Enable items
			for(
				nIndex = ID_FM_Audio_SRSS_InputGain - ID_FM_Audio_SRSS_SRSMode;
				nIndex <= ID_FM_Audio_SRSS_TruBassSpeakerSize- ID_FM_Audio_SRSS_SRSMode;
				nIndex++)
			{
				GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

				GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
				GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));
			}
		}
}

//ID_FM_Audio_SRSS_InputGain,
void Factory_Audio_SRSS_InputGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_InputGain= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_InputGain),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_InputGain),
			&(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_InputGain));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_InputGain),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_InputGain));
		*/
		MID_Audio_SRSSurround(\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SRSMode,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_InputGain,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_OutputGain,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SurroundLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize\
		);
	}
	else
	{
		// UMF get function
		 FuncData->value=g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_InputGain;
	}
}
//ID_FM_Audio_SRSS_OutputGain,
void Factory_Audio_SRSS_OutputGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_OutputGain= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_OutputGain),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_OutputGain),
			&(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_OutputGain));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_OutputGain),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_OutputGain));
		*/
		MID_Audio_SRSSurround(\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SRSMode,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_InputGain,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_OutputGain,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SurroundLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize\
		);
	}
	else
	{
		// UMF get function
		 FuncData->value=g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_OutputGain;
	}
}
//ID_FM_Audio_SRSS_SurroundLevel,
void Factory_Audio_SRSS_SurroundLevel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SurroundLevel= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_SurroundLevel),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SurroundLevel),
			&(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SurroundLevel));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_SurroundLevel),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SurroundLevel));
		*/
		MID_Audio_SRSSurround(\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SRSMode,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_InputGain,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_OutputGain,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SurroundLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SurroundLevel;
	}
}
//ID_FM_Audio_SRSS_DialogClarityLevel,
void Factory_Audio_SRSS_DialogClarityLevel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel),
			&(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel));
		*/
		MID_Audio_SRSSurround(\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SRSMode,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_InputGain,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_OutputGain,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SurroundLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel;
	}
}
//ID_FM_Audio_SRSS_TruBassLevel,
void Factory_Audio_SRSS_TruBassLevel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassLevel= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_TruBassLevel),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassLevel),
			&(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassLevel));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_TruBassLevel),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassLevel));
		*/
		MID_Audio_SRSSurround(\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SRSMode,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_InputGain,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_OutputGain,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SurroundLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassLevel;
	}
}

	//ID_FM_Audio_SRSS_DefinitionLevel,
void Factory_Audio_SRSS_DefinitionLevel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel),
			&(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel));
		*/
		MID_Audio_SRSSurround(\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SRSMode,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_InputGain,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_OutputGain,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SurroundLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel;
	}
}
//ID_FM_Audio_SRSS_TruBassSpeakerSize,
void Factory_Audio_SRSS_TruBassSpeakerSize(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize),
			&(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize),
			sizeof(g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize));
		*/
		MID_Audio_SRSSurround(\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SRSMode,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_InputGain,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_OutputGain,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_SurroundLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DialogClarityLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_DefinitionLevel,\
		g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.SRSSurround.n_Audio_SRSS_TruBassSpeakerSize;
	}
}
	/*5.3 SRS Surround     End */

	/*5.4 SRS TruVolume  Start*/
/*
//ID_FM_Audio_SRST_TruVolumeMode,
void Factory_Audio_SRST_TruVolumeMode(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_TruVolumeMode= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
//MID_Audio_SRSTrueVolume(    UINT8 bSRSTrueVolumeOnOff, UINT8 bProcess, UINT8 bNoiseManager, UINT8 bNoiseThreshold,
   //                                             UINT8 bMode, UINT8 bSpeakerSize, UINT16 bInputGain, UINT8 bOutputGain,
      //                                          UINT8 bByPassGain, UINT8 bReferenceLevel, UINT16 bMaxGain);
		MID_Audio_SRSTrueVolume( \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_TruVolumeMode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Process,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Manager,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Threshold,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Mode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_SpeakerSize,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_InputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_OutputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_BypassGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_ReferenceLevel, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_MaxGain \
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.SRST.n_Audio_SRST_TruVolumeMode;
	}
}
//ID_FM_Audio_SRST_Process,
void Factory_Audio_SRST_Process(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Process= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_SRSTrueVolume( \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_TruVolumeMode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Process,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Manager,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Threshold,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Mode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_SpeakerSize,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_InputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_OutputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_BypassGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_ReferenceLevel, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_MaxGain \
		);
	}
	else
	{
		// UMF get function
		 FuncData->value=g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Process;
	}
}
//ID_FM_Audio_SRST_Noise_Manager,
void Factory_Audio_SRST_Noise_Manager(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Manager= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_SRSTrueVolume( \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_TruVolumeMode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Process,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Manager,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Threshold,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Mode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_SpeakerSize,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_InputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_OutputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_BypassGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_ReferenceLevel, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_MaxGain \
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Manager;
	}
}
//ID_FM_Audio_SRST_Noise_Threshold,
void Factory_Audio_SRST_Noise_Threshold(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Threshold= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_SRSTrueVolume( \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_TruVolumeMode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Process,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Manager,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Threshold,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Mode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_SpeakerSize,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_InputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_OutputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_BypassGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_ReferenceLevel, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_MaxGain \
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Threshold;
	}
}
//ID_FM_Audio_SRST_Mode ,
void Factory_Audio_SRST_Mode(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Mode= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_SRSTrueVolume( \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_TruVolumeMode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Process,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Manager,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Threshold,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Mode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_SpeakerSize,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_InputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_OutputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_BypassGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_ReferenceLevel, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_MaxGain \
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Mode;
	}
}
//ID_FM_Audio_SRST_SpeakerSize,
void Factory_Audio_SRST_SpeakerSize(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_SpeakerSize= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_SRSTrueVolume( \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_TruVolumeMode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Process,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Manager,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Threshold,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Mode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_SpeakerSize,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_InputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_OutputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_BypassGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_ReferenceLevel, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_MaxGain \
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.SRST.n_Audio_SRST_SpeakerSize;
	}
}
//ID_FM_Audio_SRST_InputGain,
void Factory_Audio_SRST_InputGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_InputGain= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_SRSTrueVolume( \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_TruVolumeMode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Process,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Manager,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Threshold,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Mode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_SpeakerSize,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_InputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_OutputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_BypassGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_ReferenceLevel, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_MaxGain \
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.SRST.n_Audio_SRST_InputGain;
	}
}
//ID_FM_Audio_SRST_OutputGain,
void Factory_Audio_SRST_OutputGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_OutputGain= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_SRSTrueVolume( \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_TruVolumeMode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Process,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Manager,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Threshold,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Mode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_SpeakerSize,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_InputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_OutputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_BypassGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_ReferenceLevel, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_MaxGain \
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.SRST.n_Audio_SRST_OutputGain;
	}
}
//ID_FM_Audio_SRST_BypassGain,
void Factory_Audio_SRST_BypassGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_BypassGain= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_SRSTrueVolume( \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_TruVolumeMode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Process,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Manager,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Threshold,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Mode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_SpeakerSize,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_InputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_OutputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_BypassGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_ReferenceLevel, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_MaxGain \
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.SRST.n_Audio_SRST_BypassGain;
	}
}

//ID_FM_Audio_SRST_ReferenceLevel,
void Factory_Audio_SRST_ReferenceLevel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_ReferenceLevel= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_SRSTrueVolume( \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_TruVolumeMode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Process,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Manager,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Threshold,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Mode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_SpeakerSize,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_InputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_OutputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_BypassGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_ReferenceLevel, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_MaxGain \
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.SRST.n_Audio_SRST_ReferenceLevel;
	}
}
//ID_FM_Audio_SRST_MaxGain,
void Factory_Audio_SRST_MaxGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_MaxGain= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_SRSTrueVolume( \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_TruVolumeMode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Process,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Manager,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Noise_Threshold,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_Mode,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_SpeakerSize,\
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_InputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_OutputGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_BypassGain, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_ReferenceLevel, \
		g_stFactoryUserData.Audio.SRST.n_Audio_SRST_MaxGain \
		);
	}
	else
	{
		// UMF get function
		 FuncData->value=g_stFactoryUserData.Audio.SRST.n_Audio_SRST_MaxGain;
	}
}
*/
	/*5.4 SRS TruVolume  End*/

	/*5.5 AVL    Start*/
//ID_FM_Audio_AVL_AVLMode,					//204
void Factory_Audio_AVL_AVLMode(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AVLMode= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.AVL.n_Audio_AVL_AVLMode),
			sizeof(g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AVLMode),
			&(g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AVLMode));
		MID_Audio_AVL(\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AVLMode,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AttackRate,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_ActiveTime,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_Limiter,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_PullupGain\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AVLMode;
	}


	// Enable/Disable relative items
	int nIndex;
	HWND FocusItem_Handle;
	if(g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AVLMode == FALSE)
	{
		//Disable items
		for(
			nIndex = ID_FM_Audio_AVL_AttackRate - ID_FM_Audio_AVL_AVLMode;
			nIndex <= ID_FM_Audio_AVL_PullupGain- ID_FM_Audio_AVL_AVLMode;
			nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));
		}

	}else{

		//Enable items
		for(
			nIndex = ID_FM_Audio_AVL_AttackRate - ID_FM_Audio_AVL_AVLMode;
			nIndex <= ID_FM_Audio_AVL_PullupGain- ID_FM_Audio_AVL_AVLMode;
			nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));
		}
	}
}
//ID_FM_Audio_AVL_AttackRate,					//205
void Factory_Audio_AVL_AttackRate(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AttackRate= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.AVL.n_Audio_AVL_AttackRate),
			sizeof(g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AttackRate),
			&(g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AttackRate));
		MID_Audio_AVL(\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AVLMode,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AttackRate,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_ActiveTime,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_Limiter,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_PullupGain\
		);
	}
	else
	{
		// UMF get function
		 FuncData->value=g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AttackRate;
	}
}
//ID_FM_Audio_AVL_ActiveTime,					//206
void Factory_Audio_AVL_ActiveTime(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_ActiveTime= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.AVL.n_Audio_AVL_ActiveTime),
			sizeof(g_stFactoryUserData.Audio.AVL.n_Audio_AVL_ActiveTime),
			&(g_stFactoryUserData.Audio.AVL.n_Audio_AVL_ActiveTime));
		MID_Audio_AVL(\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AVLMode,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AttackRate,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_ActiveTime,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_Limiter,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_PullupGain\
		);
	}
	else
	{
		// UMF get function
		 FuncData->value=g_stFactoryUserData.Audio.AVL.n_Audio_AVL_ActiveTime;
	}
}
//ID_FM_Audio_AVL_Limiter,						//207
void Factory_Audio_AVL_Limiter(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_Limiter= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.AVL.n_Audio_AVL_Limiter),
			sizeof(g_stFactoryUserData.Audio.AVL.n_Audio_AVL_Limiter),
			&(g_stFactoryUserData.Audio.AVL.n_Audio_AVL_Limiter));
		MID_Audio_AVL(\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AVLMode,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AttackRate,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_ActiveTime,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_Limiter,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_PullupGain\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.AVL.n_Audio_AVL_Limiter;
	}
}
//ID_FM_Audio_AVL_PullupGain,					//208
void Factory_Audio_AVL_PullupGain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_PullupGain= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.AVL.n_Audio_AVL_PullupGain),
			sizeof(g_stFactoryUserData.Audio.AVL.n_Audio_AVL_PullupGain),
			&(g_stFactoryUserData.Audio.AVL.n_Audio_AVL_PullupGain));
		MID_Audio_AVL(\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AVLMode,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_AttackRate,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_ActiveTime,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_Limiter,\
		g_stFactoryUserData.Audio.AVL.n_Audio_AVL_PullupGain\
		);
	}
	else
	{
		// UMF get function
		 FuncData->value=g_stFactoryUserData.Audio.AVL.n_Audio_AVL_PullupGain;
	}
}
	/*5.5 AVL    End*/

	/*5.6 Sound Mode  Start*/
//ID_FM_Audio_Sound_SoundMode,					//209
void Factory_Audio_Sound_SoundMode(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	APP_Source_Type_t eSourType = APP_SOURCE_MAX;
	APP_StorageSource_Type_t eStorageSourType = APP_STORAGE_SOURCE_MAX;
	INT8 i8Basstemp;
	INT8 i8Trebletemp;

	APP_GUIOBJ_Source_GetCurrSource(&eSourType);
	eStorageSourType = APP_Data_UserSetting_SourceTypeMappingToStorage(eSourType);

	if (bSet)
	{
		// Set data
		g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex = FuncData->value;
		if (FuncData->value >= APP_AUDIO_MODE_MAX)
		{
			//DEBF("SPAL_Audio_ChangeMode() Mode out of range\n");
			return;
		}

		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SOUND,
			ITEM_OFFSET(APP_SETTING_Sound_t, stSoundModeSourceTab[eStorageSourType].SoundModeIndex),
			sizeof(g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex),
			&(g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex));
		// Also need set Bass treble again!!
		if (g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex == APP_AUDIO_MODE_FAVORITE)
		{
			i8Basstemp = g_stSoundData.stSoundModeSourceTab[eStorageSourType].stUserTab.Bass;
			i8Trebletemp = g_stSoundData.stSoundModeSourceTab[eStorageSourType].stUserTab.Treble;
		}
		else
		{
			i8Basstemp = g_stSoundData.stAudioSoundModeSetting[
				g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Bass;
			i8Trebletemp = g_stSoundData.stAudioSoundModeSetting[
				g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Treble;
		}
		APP_Audio_SetAudioMode(g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex,
			i8Basstemp,
			i8Trebletemp,FALSE);

		UpdateNodeFunctionContent(ID_FM_Audio_Sound_SoundMode,0,0);
	}
	else
	{
		// UMF get function
		FuncData->value = g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex;
	}
}
//ID_FM_Audio_Sound_Bass,						//210
void Factory_Audio_Sound_Bass(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	APP_Source_Type_t eSourType = APP_SOURCE_MAX;
	APP_StorageSource_Type_t eStorageSourType = APP_STORAGE_SOURCE_MAX;

	APP_GUIOBJ_Source_GetCurrSource(&eSourType);
	eStorageSourType = APP_Data_UserSetting_SourceTypeMappingToStorage(eSourType);
	if (bSet)
	{
		// UMF set function
		if (g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex == APP_AUDIO_MODE_FAVORITE)
		{
			g_stSoundData.stSoundModeSourceTab[eStorageSourType].stUserTab.Bass= FuncData->value;

			AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SOUND,
				ITEM_OFFSET(APP_SETTING_Sound_t, stSoundModeSourceTab[eStorageSourType].stUserTab.Bass),
				sizeof(g_stSoundData.stSoundModeSourceTab[eStorageSourType].stUserTab.Bass),
				&(g_stSoundData.stSoundModeSourceTab[eStorageSourType].stUserTab.Bass));
		}
		else
		{
			g_stSoundData.stAudioSoundModeSetting[
				g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Bass = FuncData->value;

			AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SOUND,
				ITEM_OFFSET(APP_SETTING_Sound_t, stAudioSoundModeSetting[
					g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Bass),
				sizeof(g_stSoundData.stAudioSoundModeSetting[
					g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Bass),
				&(g_stSoundData.stAudioSoundModeSetting[
					g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Bass));
			/*
			AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_SOUND,
				ITEM_OFFSET(APP_SETTING_Sound_t, stAudioSoundModeSetting[
					g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Bass),
				sizeof(g_stSoundData.stAudioSoundModeSetting[
					g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Bass));
			*/
		}
		//printf("Bass = %d\n ", (INT8 )(FuncData->value - 50));
		MID_Audio_Bass( (INT8 )(FuncData->value - 50) ); // mapping form 0~100 to-50~+50

		//Write back all data
	}
	else
	{
		// UMF get function
		if (g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex == APP_AUDIO_MODE_FAVORITE)
		{
			FuncData->value = g_stSoundData.stSoundModeSourceTab[eStorageSourType].stUserTab.Bass;
		}
		else
		{
			FuncData->value = g_stSoundData.stAudioSoundModeSetting[
				g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Bass;
		}
	}
}
//ID_FM_Audio_Sound_Treble,						//211
void Factory_Audio_Sound_Treble(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	APP_Source_Type_t eSourType = APP_SOURCE_MAX;
	APP_StorageSource_Type_t eStorageSourType = APP_STORAGE_SOURCE_MAX;

	APP_GUIOBJ_Source_GetCurrSource(&eSourType);
	eStorageSourType = APP_Data_UserSetting_SourceTypeMappingToStorage(eSourType);
	if (bSet)
	{
		// UMF set function
		if (g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex == APP_AUDIO_MODE_FAVORITE)
		{
			g_stSoundData.stSoundModeSourceTab[eStorageSourType].stUserTab.Treble= FuncData->value;

			AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SOUND,
				ITEM_OFFSET(APP_SETTING_Sound_t, stSoundModeSourceTab[eStorageSourType].stUserTab.Treble),
				sizeof(g_stSoundData.stSoundModeSourceTab[eStorageSourType].stUserTab.Treble),
				&(g_stSoundData.stSoundModeSourceTab[eStorageSourType].stUserTab.Treble));
			/*
			AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_SOUND,
				ITEM_OFFSET(APP_SETTING_Sound_t, stSoundModeSourceTab[eStorageSourType].stUserTab.Treble),
				sizeof(g_stSoundData.stSoundModeSourceTab[eStorageSourType].stUserTab.Treble));
			*/
		}
		else
		{
			g_stSoundData.stAudioSoundModeSetting[
				g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Treble = FuncData->value;

			AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SOUND,
				ITEM_OFFSET(APP_SETTING_Sound_t, stAudioSoundModeSetting[
					g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Treble),
				sizeof(g_stSoundData.stAudioSoundModeSetting[
					g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Treble),
				&(g_stSoundData.stAudioSoundModeSetting[
					g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Treble));
			/*
			AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_SOUND,
				ITEM_OFFSET(APP_SETTING_Sound_t, stAudioSoundModeSetting[
					g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Treble),
				sizeof(g_stSoundData.stAudioSoundModeSetting[
					g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Treble));
			*/
		}
		//printf("Treble = %d\n ", (INT8 )(FuncData->value - 50));
		MID_Audio_Treble( (INT8 )(FuncData->value - 50) ); // mapping form 0~100 to-50~+50

	}
	else
	{
		// UMF get function
		if (g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex == APP_AUDIO_MODE_FAVORITE)
		{
			FuncData->value = g_stSoundData.stSoundModeSourceTab[eStorageSourType].stUserTab.Treble;
		}
		else
		{
			FuncData->value = g_stSoundData.stAudioSoundModeSetting[
				g_stSoundData.stSoundModeSourceTab[eStorageSourType].SoundModeIndex].Treble;
		}
	}
}

/*5.6.4 Volume Start*/
//ID_FM_Audio_SounMode_Vol_OSDVolumeValue,				//277
void Factory_Audio_SounMode_Vol_OSDVolumeValue(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stVariationalData.Volume= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_VARIATIONAL,
			ITEM_OFFSET(APP_SETTING_Variational_t, Volume),
			sizeof(g_stVariationalData.Volume),&(g_stVariationalData.Volume));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_VARIATIONAL,
			ITEM_OFFSET(APP_SETTING_Variational_t, Volume),
			sizeof(g_stVariationalData.Volume));
		*/
		APP_Audio_SetVolume(g_stVariationalData.Volume);

	}
	else
	{
		// UMF get function
		FuncData->value=g_stVariationalData.Volume;
	}
}

static INT32 _APP_GUIOBJ_FM_Volume_SaveItemValue(UINT8 SaveItem,UINT16 eFmVolumePoint)
{
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();

	if (SaveItem == 0)
	{
		g_stSysInfoData.szAudioVolumeTab[sourceindex][0] = eFmVolumePoint;
	}
	else if (SaveItem == 1)
	{
		g_stSysInfoData.szAudioVolumeTab[sourceindex][10] = eFmVolumePoint;
	}
	else if (SaveItem == 2)
	{
		g_stSysInfoData.szAudioVolumeTab[sourceindex][20] = eFmVolumePoint;
	}
	else if (SaveItem == 3)
	{
		g_stSysInfoData.szAudioVolumeTab[sourceindex][30] = eFmVolumePoint;
	}
	else if (SaveItem == 4)
	{
		g_stSysInfoData.szAudioVolumeTab[sourceindex][40] = eFmVolumePoint;
	}
	else if (SaveItem == 5)
	{
		g_stSysInfoData.szAudioVolumeTab[sourceindex][50] = eFmVolumePoint;
	}
	else if (SaveItem == 6)
	{
		g_stSysInfoData.szAudioVolumeTab[sourceindex][60] = eFmVolumePoint;
	}
	else if (SaveItem == 7)
	{
		g_stSysInfoData.szAudioVolumeTab[sourceindex][70] = eFmVolumePoint;
	}
	else if (SaveItem == 8)
	{
		g_stSysInfoData.szAudioVolumeTab[sourceindex][80] = eFmVolumePoint;
	}
	else if (SaveItem == 9)
	{
		g_stSysInfoData.szAudioVolumeTab[sourceindex][90] = eFmVolumePoint;
	}
	else if (SaveItem == 10)
	{
		g_stSysInfoData.szAudioVolumeTab[sourceindex][100] = eFmVolumePoint;
	}
	else
	{
	 	return SP_SUCCESS;
	}
	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
		ITEM_OFFSET(APP_SETTING_SystemInfo_t, szAudioVolumeTab[sourceindex][SaveItem*10]),
		sizeof(g_stSysInfoData.szAudioVolumeTab[sourceindex][SaveItem*10]),&(g_stSysInfoData.szAudioVolumeTab[sourceindex][SaveItem*10]));
	/*
	AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
		ITEM_OFFSET(APP_SETTING_SystemInfo_t, szAudioVolumeTab[SaveItem*10]),
		sizeof(g_stSysInfoData.szAudioVolumeTab[SaveItem*10]));
	*/
	_APP_GUIOBJ_FM_CurveVolumeValue();
	return SP_SUCCESS;
}

void Factory_Audio_SounMode_Vol_VolumePointStep(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	if (bSet)
	{
		if(FuncData->value == 1)
		{
		    FuncData->value = 0;
		}
		g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePointStep = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePointStep),
				sizeof(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePointStep),
				&(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePointStep));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePointStep),
				sizeof(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePointStep));
		*/
	}
	else
	{
		FuncData->value = g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePointStep;
		if(FuncData->value == 0)
		{
		    FuncData->value = 1;
		}
	}
}

//ID_FM_Audio_SounMode_Vol_VolumePoint0,				//278
void Factory_Audio_SounMode_Vol_VolumePoint0(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();
	if (bSet)
	{
		// UMF set function
		g_stSysInfoData.szAudioVolumeTab[sourceindex][0]= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
			ITEM_OFFSET(APP_SETTING_SystemInfo_t, szAudioVolumeTab[sourceindex][0]),
				sizeof(g_stSysInfoData.szAudioVolumeTab[sourceindex][0]),
				&(g_stSysInfoData.szAudioVolumeTab[sourceindex][0]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint0),
				sizeof(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint0));
		*/
		_APP_GUIOBJ_FM_Volume_SaveItemValue(0,FuncData->value);
		APP_Audio_SetVolume(g_stVariationalData.Volume);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stSysInfoData.szAudioVolumeTab[sourceindex][0];
	}
}
//ID_FM_Audio_SounMode_Vol_VolumePoint10,				//279
void Factory_Audio_SounMode_Vol_VolumePoint10(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();

	if (bSet)
	{
		// UMF set function
		g_stSysInfoData.szAudioVolumeTab[sourceindex][10]= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
			ITEM_OFFSET(APP_SETTING_SystemInfo_t, szAudioVolumeTab[sourceindex][10]),
				sizeof(g_stSysInfoData.szAudioVolumeTab[sourceindex][10]),
				&(g_stSysInfoData.szAudioVolumeTab[sourceindex][10]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint10),
				sizeof(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint10));
		*/
		_APP_GUIOBJ_FM_Volume_SaveItemValue(1,FuncData->value);
		APP_Audio_SetVolume(g_stVariationalData.Volume);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stSysInfoData.szAudioVolumeTab[sourceindex][10];
	}
}
//ID_FM_Audio_SounMode_Vol_VolumePoint20,				//280
void Factory_Audio_SounMode_Vol_VolumePoint20(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();

	if (bSet)
	{
		// UMF set function
		g_stSysInfoData.szAudioVolumeTab[sourceindex][20]= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
			ITEM_OFFSET(APP_SETTING_SystemInfo_t, szAudioVolumeTab[sourceindex][20]),
				sizeof(g_stSysInfoData.szAudioVolumeTab[sourceindex][20]),
				&(g_stSysInfoData.szAudioVolumeTab[sourceindex][20]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint20),
				sizeof(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint20));
		*/
		_APP_GUIOBJ_FM_Volume_SaveItemValue(2,FuncData->value);
		APP_Audio_SetVolume(g_stVariationalData.Volume);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stSysInfoData.szAudioVolumeTab[sourceindex][20];
	}
}
//ID_FM_Audio_SounMode_Vol_VolumePoint30,				//281
void Factory_Audio_SounMode_Vol_VolumePoint30(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();

	if (bSet)
	{
		// UMF set function
		g_stSysInfoData.szAudioVolumeTab[sourceindex][30]= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
			ITEM_OFFSET(APP_SETTING_SystemInfo_t, szAudioVolumeTab[sourceindex][30]),
				sizeof(g_stSysInfoData.szAudioVolumeTab[sourceindex][30]),
				&(g_stSysInfoData.szAudioVolumeTab[sourceindex][30]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint30),
				sizeof(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint30));
		*/
		_APP_GUIOBJ_FM_Volume_SaveItemValue(3,FuncData->value);
		APP_Audio_SetVolume(g_stVariationalData.Volume);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stSysInfoData.szAudioVolumeTab[sourceindex][30];
	}
}
//ID_FM_Audio_SounMode_Vol_VolumePoint40,				//282
void Factory_Audio_SounMode_Vol_VolumePoint40(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();

	if (bSet)
	{
		// UMF set function
		g_stSysInfoData.szAudioVolumeTab[sourceindex][40]= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
			ITEM_OFFSET(APP_SETTING_SystemInfo_t, szAudioVolumeTab[sourceindex][40]),
				sizeof(g_stSysInfoData.szAudioVolumeTab[sourceindex][40]),
				&(g_stSysInfoData.szAudioVolumeTab[sourceindex][40]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint40),
				sizeof(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint40));
		*/

		_APP_GUIOBJ_FM_Volume_SaveItemValue(4,FuncData->value);
		APP_Audio_SetVolume(g_stVariationalData.Volume);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stSysInfoData.szAudioVolumeTab[sourceindex][40];
	}
}
//ID_FM_Audio_SounMode_Vol_VolumePoint50,				//283
void Factory_Audio_SounMode_Vol_VolumePoint50(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();
	if (bSet)
	{
		// UMF set function
		g_stSysInfoData.szAudioVolumeTab[sourceindex][50]= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
			ITEM_OFFSET(APP_SETTING_SystemInfo_t, szAudioVolumeTab[sourceindex][50]),
				sizeof(g_stSysInfoData.szAudioVolumeTab[sourceindex][50]),
				&(g_stSysInfoData.szAudioVolumeTab[sourceindex][50]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint50),
				sizeof(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint50));
		*/
		_APP_GUIOBJ_FM_Volume_SaveItemValue(5,FuncData->value);
		APP_Audio_SetVolume(g_stVariationalData.Volume);
	}
	else
	{
		// UMF get function
		FuncData->value = g_stSysInfoData.szAudioVolumeTab[sourceindex][50];
	}
}
//ID_FM_Audio_SounMode_Vol_VolumePoint60,				//284
void Factory_Audio_SounMode_Vol_VolumePoint60(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();

	if (bSet)
	{
		// UMF set function
		g_stSysInfoData.szAudioVolumeTab[sourceindex][60]= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
			ITEM_OFFSET(APP_SETTING_SystemInfo_t, szAudioVolumeTab[sourceindex][60]),
				sizeof(g_stSysInfoData.szAudioVolumeTab[sourceindex][60]),
				&(g_stSysInfoData.szAudioVolumeTab[sourceindex][60]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint60),
				sizeof(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint60));
		*/
		_APP_GUIOBJ_FM_Volume_SaveItemValue(6,FuncData->value);
		APP_Audio_SetVolume(g_stVariationalData.Volume);
	}
	else
	{
		// UMF get function
		 FuncData->value = g_stSysInfoData.szAudioVolumeTab[sourceindex][60];
	}
}
//ID_FM_Audio_SounMode_Vol_VolumePoint70,				//285
void Factory_Audio_SounMode_Vol_VolumePoint70(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();

	if (bSet)
	{
		// UMF set function
		g_stSysInfoData.szAudioVolumeTab[sourceindex][70]= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
			ITEM_OFFSET(APP_SETTING_SystemInfo_t, szAudioVolumeTab[sourceindex][70]),
				sizeof(g_stSysInfoData.szAudioVolumeTab[sourceindex][70]),
				&(g_stSysInfoData.szAudioVolumeTab[sourceindex][70]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint70),
				sizeof(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint70));
		*/
		_APP_GUIOBJ_FM_Volume_SaveItemValue(7,FuncData->value);
		APP_Audio_SetVolume(g_stVariationalData.Volume);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stSysInfoData.szAudioVolumeTab[sourceindex][70];
	}
}
//ID_FM_Audio_SounMode_Vol_VolumePoint80,				//286
void Factory_Audio_SounMode_Vol_VolumePoint80(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();
	if (bSet)
	{
		// UMF set function
		g_stSysInfoData.szAudioVolumeTab[sourceindex][80]= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
			ITEM_OFFSET(APP_SETTING_SystemInfo_t, szAudioVolumeTab[sourceindex][80]),
				sizeof(g_stSysInfoData.szAudioVolumeTab[sourceindex][80]),
				&(g_stSysInfoData.szAudioVolumeTab[sourceindex][80]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint80),
				sizeof(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint80));
		*/
		_APP_GUIOBJ_FM_Volume_SaveItemValue(8,FuncData->value);
		APP_Audio_SetVolume(g_stVariationalData.Volume);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stSysInfoData.szAudioVolumeTab[sourceindex][80];
	}
}
//ID_FM_Audio_SounMode_Vol_VolumePoint90,				//287
void Factory_Audio_SounMode_Vol_VolumePoint90(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();

	if (bSet)
	{
		// UMF set function
		g_stSysInfoData.szAudioVolumeTab[sourceindex][90]= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
			ITEM_OFFSET(APP_SETTING_SystemInfo_t, szAudioVolumeTab[sourceindex][90]),
				sizeof(g_stSysInfoData.szAudioVolumeTab[sourceindex][90]),
				&(g_stSysInfoData.szAudioVolumeTab[sourceindex][90]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint90),
				sizeof(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint90));
		*/
		_APP_GUIOBJ_FM_Volume_SaveItemValue(9,FuncData->value);
		APP_Audio_SetVolume(g_stVariationalData.Volume);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stSysInfoData.szAudioVolumeTab[sourceindex][90];
	}
}
//ID_FM_Audio_SounMode_Vol_VolumePoint100,				//288
void Factory_Audio_SounMode_Vol_VolumePoint100(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	APP_Source_Volume_Table_e sourceindex = SOURCE_AUDIOVOLUME_TABLE_MAX;
	
	sourceindex = APP_Audio_GetAudioVolumeTableIndex();

	if (bSet)
	{
		// UMF set function
		g_stSysInfoData.szAudioVolumeTab[sourceindex][100]= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SYSINFO,
			ITEM_OFFSET(APP_SETTING_SystemInfo_t, szAudioVolumeTab[sourceindex][100]),
				sizeof(g_stSysInfoData.szAudioVolumeTab[sourceindex][100]),
				&(g_stSysInfoData.szAudioVolumeTab[sourceindex][100]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint100),
				sizeof(g_stFactoryUserData.Audio.SoundMode.volume.n_Audio_AVL_Vol_VolumePoint100));
		*/
		_APP_GUIOBJ_FM_Volume_SaveItemValue(10,FuncData->value);
		APP_Audio_SetVolume(g_stVariationalData.Volume);
	}
	else
	{
		// UMF get function
		FuncData->value = g_stSysInfoData.szAudioVolumeTab[sourceindex][100];
	}
}

		/*5.6.4 Volume End*/
	/*5.6 Sound Mode  End*/
	/*5.7 EQ setting   start*/
/*
//ID_FM_Audio_EQ_Band1120Hz,					//213
void Factory_Audio_EQ_Band1120Hz(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band1120Hz= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		//TVFE_Audio_Status_e MID_Audio_EQ(INT8 bBand1, INT8 bBand2, INT8 bBand3, INT8 bBand4, INT8 bBand5, INT8 bBand6, INT8 bBand7);
		MID_Audio_EQ(\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band1120Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band2300Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band3500Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band412kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band530kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band675kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band710kHz\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band1120Hz;
	}
}
//ID_FM_Audio_EQ_Band2300Hz,					//214
void Factory_Audio_EQ_Band2300Hz(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band2300Hz= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_EQ(\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band1120Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band2300Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band3500Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band412kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band530kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band675kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band710kHz\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band2300Hz;
	}
}
//ID_FM_Audio_EQ_Band3500Hz,					//215
void Factory_Audio_EQ_Band3500Hz(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band3500Hz= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_EQ(\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band1120Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band2300Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band3500Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band412kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band530kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band675kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band710kHz\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band3500Hz;
	}
}
//ID_FM_Audio_EQ_Band412kHz,					//216
void Factory_Audio_EQ_Band412kHz(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band412kHz= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_EQ(\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band1120Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band2300Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band3500Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band412kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band530kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band675kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band710kHz\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band412kHz;
	}
}
//ID_FM_Audio_EQ_Band530kHz,					//217
void Factory_Audio_EQ_Band530kHz(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band530kHz= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_EQ(\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band1120Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band2300Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band3500Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band412kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band530kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band675kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band710kHz\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band530kHz;
	}
}
//ID_FM_Audio_EQ_Band675kHz,					//218
void Factory_Audio_EQ_Band675kHz(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band675kHz= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_EQ(\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band1120Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band2300Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band3500Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band412kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band530kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band675kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band710kHz\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band675kHz;
	}
}
//ID_FM_Audio_EQ_Band710kHz,					//219
void Factory_Audio_EQ_Band710kHz(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band710kHz= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
		MID_Audio_EQ(\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band1120Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band2300Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band3500Hz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band412kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band530kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band675kHz,\
		(INT8)g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band710kHz\
		);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.EQsetting.n_Audio_EQ_Band710kHz;
	}
}
*/
void Factory_Audio_EQ_Band(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function

		//Write back all data
		g_stFactoryUserData.Audio.EQsetting.n_EQ_Band = (FuncData->value) -1; //NOTE: 1~7 -> 0~6
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.EQsetting.n_EQ_Band),
				sizeof(g_stFactoryUserData.Audio.EQsetting.n_EQ_Band),
				&(g_stFactoryUserData.Audio.EQsetting.n_EQ_Band));
		UpdateNodeFunctionContent(ID_FM_Audio_EQ_Band,0,0);

	}
	else
	{
		// UMF get function
		FuncData->value = g_stFactoryUserData.Audio.EQsetting.n_EQ_Band + 1; //NOTE:  0~6 -> 1~7
	}
}


void Factory_Audio_EQ_Gain(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	UINT8 n_EQ_Band = g_stFactoryUserData.Audio.EQsetting.n_EQ_Band;

	if (bSet)
	{

		// Update Value
		g_stFactoryUserData.Audio.EQsetting.n_EQ_Gain[n_EQ_Band] = FuncData->value;

		// UMF set function
		MID_Audio_EQ(
			g_stFactoryUserData.Audio.EQsetting.n_EQ_Gain[0],
			g_stFactoryUserData.Audio.EQsetting.n_EQ_Gain[1],
			g_stFactoryUserData.Audio.EQsetting.n_EQ_Gain[2],
			g_stFactoryUserData.Audio.EQsetting.n_EQ_Gain[3],
			g_stFactoryUserData.Audio.EQsetting.n_EQ_Gain[4],
			g_stFactoryUserData.Audio.EQsetting.n_EQ_Gain[5],
			g_stFactoryUserData.Audio.EQsetting.n_EQ_Gain[6] );

		//Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.EQsetting.n_EQ_Gain[n_EQ_Band]),
				sizeof(g_stFactoryUserData.Audio.EQsetting.n_EQ_Gain[n_EQ_Band]),
				&(g_stFactoryUserData.Audio.EQsetting.n_EQ_Gain[n_EQ_Band]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.EQsetting.n_EQ_Gain[n_EQ_Band]),
				sizeof(g_stFactoryUserData.Audio.EQsetting.n_EQ_Gain[n_EQ_Band]));
		*/
	}
	else
	{
		// UMF get function
		FuncData->value = g_stFactoryUserData.Audio.EQsetting.n_EQ_Gain[n_EQ_Band];
	}
}

void SetAudioFreqAndQ(void)
{
	TVFE_Audio_Equalizer_Init_t nEQ;

	int nIdx = 0;
	for(; nIdx<7; nIdx++)
	{
		nEQ.bEQ_fq[nIdx] 	= g_stFactoryUserData.Audio.EQsetting.n_EQ_Freq[nIdx];
		nEQ.bQfactor[nIdx] 	= g_stFactoryUserData.Audio.EQsetting.n_EQ_q[nIdx];
	}
	TVFE_Audio_Initial_Main_Equalizer(&nEQ);
}
void Factory_Audio_EQ_Freq(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{

	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	UINT8 n_EQ_Band = g_stFactoryUserData.Audio.EQsetting.n_EQ_Band;

	if (bSet)
	{
		// Update Value
		g_stFactoryUserData.Audio.EQsetting.n_EQ_Freq[n_EQ_Band] = FuncData->value;

		// UMF set function
		SetAudioFreqAndQ();

		//Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.EQsetting.n_EQ_Freq[n_EQ_Band]),
				sizeof(g_stFactoryUserData.Audio.EQsetting.n_EQ_Freq[n_EQ_Band]),
				&(g_stFactoryUserData.Audio.EQsetting.n_EQ_Freq[n_EQ_Band]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.EQsetting.n_EQ_Freq[n_EQ_Band]),
				sizeof(g_stFactoryUserData.Audio.EQsetting.n_EQ_Freq[n_EQ_Band]));
		*/
	}
	else
	{
		// UMF get function
		FuncData->value = g_stFactoryUserData.Audio.EQsetting.n_EQ_Freq[n_EQ_Band];
	}

}

// Important !! for duoble to int correct convert!!
inline int double2int(double in)
{
	return in > 0 ? (in + 0.5) : (in - 0.5);
}
void Factory_Audio_EQ_q(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{

	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	UINT8 n_EQ_Band = g_stFactoryUserData.Audio.EQsetting.n_EQ_Band;

	if (bSet)
	{
		// Update Value
		g_stFactoryUserData.Audio.EQsetting.n_EQ_q[n_EQ_Band] = double2int(FuncData->value*(10.0)); //Note 0.1~5.0 -> 1~50

		// UMF set function
		SetAudioFreqAndQ();

		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.EQsetting.n_EQ_q[n_EQ_Band]),
				sizeof(g_stFactoryUserData.Audio.EQsetting.n_EQ_q[n_EQ_Band]),
				&(g_stFactoryUserData.Audio.EQsetting.n_EQ_q[n_EQ_Band]));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.EQsetting.n_EQ_q[n_EQ_Band]),
				sizeof(g_stFactoryUserData.Audio.EQsetting.n_EQ_q[n_EQ_Band]));
		*/
	}
	else
	{
		// UMF get function
		FuncData->value = (double)g_stFactoryUserData.Audio.EQsetting.n_EQ_q[n_EQ_Band]/ (10.0);// Note 1~50 -> 0.1~5.0
	}
}

/*5.7 EQ setting   End*/


/*5.8 PEQ1 start*/
TVFE_Audio_Parametric_Equalizer_Config_t PEQ_Setting;
//ID_FM_Audio_PEQ1_Enable,					//220
static void InitializePEQSetting()
{
	//Enable
	PEQ_Setting.enable[0] = g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_Enable;
	PEQ_Setting.enable[1] = g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_Enable;
	PEQ_Setting.enable[2] = g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_Enable;
	//frequency
	PEQ_Setting.eq_fq[0] =g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_CenterFreq;
	PEQ_Setting.eq_fq[1] =g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_CenterFreq;
	PEQ_Setting.eq_fq[2] =g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_CenterFreq;
	//Gain
	PEQ_Setting.gain[0] = g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_GainStep;
	PEQ_Setting.gain[1] = g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_GainStep;
	PEQ_Setting.gain[2] = g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_GainStep;
	//Q
	PEQ_Setting.Qfactor[0] = g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_q;
	PEQ_Setting.Qfactor[1] = g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_q;
	PEQ_Setting.Qfactor[2] = g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_q;
}
void Factory_Audio_PEQ1_Enable(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// Write back all data
		g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_Enable = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ1.n_Audio_PEQ1_Enable),
				sizeof(g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_Enable),
				&(g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_Enable));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ1.n_Audio_PEQ1_Enable),
				sizeof(g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_Enable));
		*/
		// UMF set function
		InitializePEQSetting();
		PEQ_Setting.enable[0] = FuncData->value;
		TVFE_Audio_Set_Parametric_Equalizer_Config(&PEQ_Setting);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_Enable;
	}

	// Enable/Disable relative items
	int nIndex;
	int nStartIndex = ID_FM_Audio_PEQ1_CenterFreq - ID_FM_Audio_PEQ1_Enable;
	int nEndIndex = ID_FM_Audio_PEQ1_GainStep- ID_FM_Audio_PEQ1_Enable;
	HWND FocusItem_Handle;
	if(g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_Enable == FALSE)
	{
		//Disable items
		for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));
		}

	}else{

		//Enable items
		for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));
		}
	}
}
//ID_FM_Audio_PEQ1_CenterFreq,					//221
void Factory_Audio_PEQ1_CenterFreq(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_CenterFreq= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ1.n_Audio_PEQ1_CenterFreq),
				sizeof(g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_CenterFreq),
				&(g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_CenterFreq));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ1.n_Audio_PEQ1_CenterFreq),
				sizeof(g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_CenterFreq));
		*/
		InitializePEQSetting();
		PEQ_Setting.eq_fq[0] = FuncData->value;
		TVFE_Audio_Set_Parametric_Equalizer_Config(&PEQ_Setting);

	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_CenterFreq;
	}
}

//ID_FM_Audio_PEQ1_q,			//222
void Factory_Audio_PEQ1_q(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_q = double2int( FuncData->value*(10.0) ); // map 0.1~5.0 to 1~50
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ1.n_Audio_PEQ1_q),
				sizeof(g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_q),
				&(g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_q));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ1.n_Audio_PEQ1_q),
				sizeof(g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_q));
		*/
		// UMF set function
		InitializePEQSetting();
		TVFE_Audio_Set_Parametric_Equalizer_Config(&PEQ_Setting);
	}
	else
	{
		// UMF get function
		FuncData->value = (double)g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_q/(10.0); // map 1~50 -> 0.1~5.0
	}
}
//ID_FM_Audio_PEQ1_GainStep,					//223
void Factory_Audio_PEQ1_GainStep(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_GainStep= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ1.n_Audio_PEQ1_GainStep),
				sizeof(g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_GainStep),
				&(g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_GainStep));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ1.n_Audio_PEQ1_GainStep),
				sizeof(g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_GainStep));
		*/
		InitializePEQSetting();
		PEQ_Setting.gain[0] = FuncData->value;
		TVFE_Audio_Set_Parametric_Equalizer_Config(&PEQ_Setting);
	}
	else
	{
		// UMF get function
		FuncData->value= g_stFactoryUserData.Audio.PEQ1.n_Audio_PEQ1_GainStep;
	}
}
	/*5.8 PEQ1 End*/

	/*5.9 PEQ2 start*/
//ID_FM_Audio_PEQ2_Enable,
void Factory_Audio_PEQ2_Enable(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// Write back all data
		g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_Enable = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ2.n_Audio_PEQ2_Enable),
				sizeof(g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_Enable),
				&(g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_Enable));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ2.n_Audio_PEQ2_Enable),
				sizeof(g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_Enable));
		*/

		// UMF set function
		InitializePEQSetting();
		PEQ_Setting.enable[1] = FuncData->value;
		TVFE_Audio_Set_Parametric_Equalizer_Config(&PEQ_Setting);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_Enable;
	}

	// Enable/Disable relative items
	int nIndex;
	int nStartIndex = ID_FM_Audio_PEQ2_CenterFreq - ID_FM_Audio_PEQ2_Enable;
	int nEndIndex = ID_FM_Audio_PEQ2_GainStep - ID_FM_Audio_PEQ2_Enable;
	HWND FocusItem_Handle;
	if(g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_Enable == FALSE)
	{
		//Disable items
		for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));
		}

	}else{

		//Enable items
		for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

			GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));
		}
	}
}
//ID_FM_Audio_PEQ2_CenterFreq,
void Factory_Audio_PEQ2_CenterFreq(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_CenterFreq= FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ2.n_Audio_PEQ2_CenterFreq),
				sizeof(g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_CenterFreq),
				&(g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_CenterFreq));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ2.n_Audio_PEQ2_CenterFreq),
				sizeof(g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_CenterFreq));
		*/
		// UMF set function
		InitializePEQSetting();
		TVFE_Audio_Set_Parametric_Equalizer_Config(&PEQ_Setting);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_CenterFreq;
	}
}
//ID_FM_Audio_PEQ2_q,
void Factory_Audio_PEQ2_q(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_q = double2int( FuncData->value*(10.0) ); // map 0.1~5.0 to 1~50
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ2.n_Audio_PEQ2_q),
				sizeof(g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_q),
				&(g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_q));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ2.n_Audio_PEQ2_q),
				sizeof(g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_q));
		*/
		
		// UMF set function
		InitializePEQSetting();
		TVFE_Audio_Set_Parametric_Equalizer_Config(&PEQ_Setting);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_q / (10.0); // map 1~50 -> 0.1~5.0
	}
}
//ID_FM_Audio_PEQ2_GainStep,
void Factory_Audio_PEQ2_GainStep(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_GainStep= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ2.n_Audio_PEQ2_GainStep),
				sizeof(g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_GainStep),
				&(g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_GainStep));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ2.n_Audio_PEQ2_GainStep),
				sizeof(g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_GainStep));
		*/
		InitializePEQSetting();
		PEQ_Setting.gain[1] = FuncData->value;
		TVFE_Audio_Set_Parametric_Equalizer_Config(&PEQ_Setting);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.PEQ2.n_Audio_PEQ2_GainStep;
	}
}
	/*5.9 PEQ2 End*/

	/*5.10 PEQ3   Start*/
//ID_FM_Audio_PEQ3_Enable,
void Factory_Audio_PEQ3_Enable(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// Write back all data
		g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_Enable = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ3.n_Audio_PEQ3_Enable),
				sizeof(g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_Enable),
				&(g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_Enable));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ3.n_Audio_PEQ3_Enable),
				sizeof(g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_Enable));
		*/
		
		// UMF set function
		InitializePEQSetting();
		PEQ_Setting.enable[2] = FuncData->value;
		TVFE_Audio_Set_Parametric_Equalizer_Config(&PEQ_Setting);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_Enable;
	}



	// Enable/Disable relative items
		int nIndex;
		int nStartIndex = ID_FM_Audio_PEQ3_CenterFreq - ID_FM_Audio_PEQ3_Enable;
		int nEndIndex = ID_FM_Audio_PEQ3_GainStep - ID_FM_Audio_PEQ3_Enable;
		HWND FocusItem_Handle;
		if(g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_Enable == FALSE)
		{
			//Disable items
			for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
			{
				GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));

				GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
				GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));
			}

		}else{

			//Enable items
			for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
			{
				GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));

				GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
				GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));
			}
		}
}
//ID_FM_Audio_PEQ3_CenterFreq,
void Factory_Audio_PEQ3_CenterFreq(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_CenterFreq= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ3.n_Audio_PEQ3_CenterFreq),
				sizeof(g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_CenterFreq),
				&(g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_CenterFreq));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ3.n_Audio_PEQ3_CenterFreq),
				sizeof(g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_CenterFreq));
		*/
		InitializePEQSetting();
		PEQ_Setting.eq_fq[2] = FuncData->value;
		TVFE_Audio_Set_Parametric_Equalizer_Config(&PEQ_Setting);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_CenterFreq;
	}
}
//ID_FM_Audio_PEQ3_q,
void Factory_Audio_PEQ3_q(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_q= double2int( FuncData->value*(10.0) ); // map 0.1~5.0 to 1~50
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ3.n_Audio_PEQ3_q),
				sizeof(g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_q),
				&(g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_q));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ3.n_Audio_PEQ3_q),
				sizeof(g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_q));
		*/

		// UMF set function
		InitializePEQSetting();
		TVFE_Audio_Set_Parametric_Equalizer_Config(&PEQ_Setting);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_q / (10.0); // map 1~50 -> 0.1~5.0
	}
}
//ID_FM_Audio_PEQ3_GainStep,
void Factory_Audio_PEQ3_GainStep(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_GainStep= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ3.n_Audio_PEQ3_GainStep),
				sizeof(g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_GainStep),
				&(g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_GainStep));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.PEQ3.n_Audio_PEQ3_GainStep),
				sizeof(g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_GainStep));
		*/
		InitializePEQSetting();
		PEQ_Setting.gain[2] = FuncData->value;
		TVFE_Audio_Set_Parametric_Equalizer_Config(&PEQ_Setting);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.PEQ3.n_Audio_PEQ3_GainStep;
	}
}	/*5.10 PEQ3   End*/

	/*5.11 Misc  start*/
//ID_FM_Audio_Misc_Speaker,					//232
void Factory_Audio_Misc_Speaker(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.Misc.n_Audio_Misc_Speaker= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.Misc.n_Audio_Misc_Speaker),
				sizeof(g_stFactoryUserData.Audio.Misc.n_Audio_Misc_Speaker),
				&(g_stFactoryUserData.Audio.Misc.n_Audio_Misc_Speaker));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.Misc.n_Audio_Misc_Speaker),
				sizeof(g_stFactoryUserData.Audio.Misc.n_Audio_Misc_Speaker));
		*/
		MID_Audio_SPDIFOut((TVFE_Audio_SPDIF_Output_Config_e)FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.Misc.n_Audio_Misc_Speaker;
	}
}
//ID_FM_Audio_Misc_LipSync,					//233
void Factory_Audio_Misc_LipSync(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Audio.Misc.n_Audio_Misc_LipSync= FuncData->value;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.Misc.n_Audio_Misc_LipSync),
				sizeof(g_stFactoryUserData.Audio.Misc.n_Audio_Misc_LipSync),
				&(g_stFactoryUserData.Audio.Misc.n_Audio_Misc_LipSync));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Audio.Misc.n_Audio_Misc_LipSync),
				sizeof(g_stFactoryUserData.Audio.Misc.n_Audio_Misc_LipSync));
		*/
		MID_Audio_LipSync((INT16 )FuncData->value);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Audio.Misc.n_Audio_Misc_LipSync;
	}
}
	/*5.11 Misc  End*/
	//ID_FM_Audio_AudioOutOffset_Value,					//233
void Factory_Audio_AudioOutOffset_Value(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	
	if (bSet)
	{				
		u8AudioOutOffset = FuncData->value ;  	
		APP_Audio_SetLevelAdjust_For_AudioOutOffset(u8AudioOutOffset);	
	}
	else
	{
		FuncData->value = (UINT32)u8AudioOutOffset;			
	}
}
	/*5.11 Misc  End*/
/*5. Audio  	end */
/*6. Burning Mode Start*/
//ID_FM_FactSet_BurningMode,			//06
void Factory_FactSet_BurningMode(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.n_FactSet_BurningMode= FuncData->value;
		MID_TVFE_SetAutoPowerOn(TRUE); //zhouqp add for burning power on 20140903  20140910
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, n_FactSet_BurningMode),
			sizeof(g_stFactoryUserData.n_FactSet_BurningMode),&(g_stFactoryUserData.n_FactSet_BurningMode));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, n_FactSet_BurningMode),
			sizeof(g_stFactoryUserData.n_FactSet_BurningMode));
		*/
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.n_FactSet_BurningMode;
	}
}
    
#ifdef CONFIG_CTV_UART_FAC_MODE    
void Factory_BurnTime(BOOL bSet, BOOL path, FUNCTION_DATA * pValue, BOOL bDefault)
{
    char TimeStr[32];
    UINT32 tim = 0;
	#ifdef CONFIG_CTV_UART_FAC_MODE
		APP_SETTING_BurnInModeTimeTable_t dataBurnInModeTime;
		APP_BurntTimeDataWriteRead(FALSE,(UINT8 *)&dataBurnInModeTime);
        tim = dataBurnInModeTime.n_BurnInModeTime;
	#else
        AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
                ITEM_OFFSET(APP_SETTING_FactoryUser_t, n_BurnInModeTime),
                sizeof(g_stFactoryUserData.n_BurnInModeTime),&(g_stFactoryUserData.n_BurnInModeTime));
        tim = g_stFactoryUserData.n_BurnInModeTime;
	#endif

        sprintf(TimeStr, "%d hour %d min", tim / 60, tim % 60);


    if (bSet == FALSE)
    {
        Update_TypeVersion_Node(TimeStr, pValue->nItem);
    }
}
//  ID_FM_Ver_CustomerSWVer,            //20
void Factory_Test_Result(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
    char TimeStr[32] = {0};
    if(bSet==FALSE)
    {
	    #ifdef CONFIG_CTV_UART_FAC_MODE
		APP_SETTING_CtvRs232ResultTable_t dataCtvRs232Result;        
        APP_TestResultDataWriteRead(FALSE,(UINT8 *)&dataCtvRs232Result);         
        snprintf(TimeStr,32,"%d", dataCtvRs232Result.n_CtvRs232Result);
		#else
        AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
                sizeof(APP_SETTING_FactoryUser_t), &g_stFactoryUserData);
        snprintf(TimeStr,32,"%d", g_stFactoryUserData.n_CtvRs232Result);
		#endif
        Update_TypeVersion_Node(TimeStr, pValue->nItem);
    }

}

//  ID_FM_Ver_CustomerSWVer,            //20
void Factory_PCB_SN(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
    char TimeStr[32] = {0};
    if(bSet==FALSE)
    {
	    #ifdef CONFIG_CTV_UART_FAC_MODE
		APP_SETTING_PcbaSNTable_t dataPcbaSN;
		APP_PCBSNDataWriteRead(FALSE,(UINT8 *)&dataPcbaSN);
		strncpy(TimeStr,(char*)dataPcbaSN.n_PcbaSN,CTV_RS232_PCBA_LEN+1);
		#else
        AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
                sizeof(APP_SETTING_FactoryUser_t), &g_stFactoryUserData);
        strncpy(TimeStr,(char*)g_stFactoryUserData.n_PcbaSN,CTV_RS232_PCBA_LEN+1);
		#endif
        Update_TypeVersion_Node(TimeStr, pValue->nItem);
    }

}
#endif
    
/*7. Shipping Mode */
#ifdef SUPPORT_HKC_FACTORY_REMOTE
void _FM_FactorySetting_Shipping(void)
#else
void _FM_FactorySetting_Shipping(void)
#endif
{
	#ifdef CONFIG_SUPPORT_AUTO_WHITE_BALANCE
	APP_SETTING_ColorTempTab_t	stColorTemp[APP_VIDEO_PQ_STORE_SOURCE_TYPE_MAX];
	#endif
	
	UINT32 dSysAppIdx = 0;
	APP_Source_Type_t eSourType = APP_SOURCE_MAX;

	APP_GUIOBJ_Source_GetCurrSource(&eSourType);
	if (MAIN_APP_SUCCESS == MAINAPP_GetActiveSystemAppIndex(&dSysAppIdx))
	{
#ifdef CONFIG_ATV_SUPPORT
		if (dSysAppIdx == SYS_APP_ATV)
		{
			APP_Audio_SetMute(TRUE, FALSE, APP_MUTE_MODE_STATEMAX, eSourType);
			APP_Video_SetMute(TRUE, FALSE, APP_MUTE_MODE_STATEMAX, eSourType);

			SYSAPP_GOBJ_SendMsgToSingleGUIObject(SYS_APP_ATV, ATV_GUIOBJ_PLAYBACK,
				APP_ATV_INTRA_EVENT_STOP_PLAYBACK, 0);
		}
#endif
#ifdef CONFIG_DTV_SUPPORT
		if (dSysAppIdx == SYS_APP_DVB)
		{
			SYSAPP_GOBJ_SendMsgToSingleGUIObject(SYS_APP_DVB, DVB_GUIOBJ_PLAYBACK,
				APP_DVB_INTRA_EVENT_STOP_PLAYBACK, PLAYBACK_STOP_ALL);

			APP_Audio_SetMute(TRUE, FALSE, APP_MUTE_MODE_STATEMAX, eSourType);
			APP_Video_SetMute(TRUE, FALSE, APP_MUTE_MODE_STATEMAX, eSourType);
		}
#endif
#ifdef CONFIG_MEDIA_SUPPORT
		if ((dSysAppIdx == SYS_APP_FILE_PLAYER) &&
			(SYSAPP_GOBJ_GUIObjectExist(dSysAppIdx, MEDIA_GUIOBJ_POPMSG)))
		{
	        SYSAPP_GOBJ_DestroyGUIObject(dSysAppIdx, MEDIA_GUIOBJ_POPMSG);
		}
#endif
	}
	else
	{
		return;
	}
	gIsFactoryResetting = TRUE;
    /* destroy mute icon */
    SYSAPP_GOBJ_SendMsgToSingleGUIObject(dSysAppIdx,
    	APP_GUIOBJ_MUTE, APP_INTRA_EVENT_HIDE_MUTE, 0);
#ifdef CONFIG_ATV_SUPPORT
	AL_DB_Reset(AL_DBTYPE_DVB_ATV, al_true);
	APP_ATV_Playback_ResetFirstService();
#endif
#if(defined CONFIG_DVB_SYSTEM_DVBT_SUPPORT)
	AL_FW_SwitchDBModule(AL_DBTYPE_DVB_T);
	AL_DB_Reset(AL_DBTYPE_DVB_T, al_true);
	AL_Event_UnLockChannels(AL_DBTYPE_DVB_T, 0, 0, 0);
	AL_PR_DeleteAllSch(AL_DBTYPE_DVB_T,AL_PR_EVT_REM | AL_PR_SRV_REM);
#endif
#if(defined CONFIG_DVB_SYSTEM_DVBC_SUPPORT)
	AL_FW_SwitchDBModule(AL_DBTYPE_DVB_C);
	AL_DB_Reset(AL_DBTYPE_DVB_C, al_true);
	AL_Event_UnLockChannels(AL_DBTYPE_DVB_C, 0, 0, 0);
	AL_PR_DeleteAllSch(AL_DBTYPE_DVB_C,AL_PR_EVT_REM | AL_PR_SRV_REM);
#endif
#if(defined CONFIG_DVB_SYSTEM_DVBS_SUPPORT)
	AL_FW_SwitchDBModule(AL_DBTYPE_DVB_S);
	AL_DB_Reset(AL_DBTYPE_DVB_S, al_true);
	AL_Event_UnLockChannels(AL_DBTYPE_DVB_S, 0, 0, 0);
	AL_PR_DeleteAllSch(AL_DBTYPE_DVB_S,AL_PR_EVT_REM | AL_PR_SRV_REM);
	DVBApp_LoaddefaultDB(AL_DBTYPE_DVB_S);
	AL_DB_Sync(AL_DBTYPE_DVB_S, al_true);
#endif
#ifdef TIANLE_Board_Time
	UINT32 BoardTime;
	/*Keep Some Setup not changed*/
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
		sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
	BoardTime = g_stSetupData.BoardTime;
#endif
#ifdef TEAC_SYSTEMINFO_SUPPORT
	UINT32 PanelTime;
	UINT32 DVDTime;
	/*Keep Some Setup not changed*/
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
		sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
	PanelTime = g_stSetupData.PanelTime;
	DVDTime = g_stSetupData.DVDTime;
#endif
	App_Data_UserSetting_Restore();
#ifdef TIANLE_Board_Time
	/*Keep Some Setup not changed*/
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
		sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
	g_stSetupData.BoardTime = BoardTime;
	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,0,
	 	sizeof(APP_SETTING_Setup_t),&(g_stSetupData));
	AL_Setting_Store(APP_Data_UserSetting_Handle(),SYS_SET_ID_SETUP, 0,
		sizeof(APP_SETTING_Setup_t));
#endif
#ifdef TEAC_SYSTEMINFO_SUPPORT
	/*Keep Some Setup not changed*/
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP, 0,
		sizeof(APP_SETTING_Setup_t), &(g_stSetupData));
	g_stSetupData.PanelTime = PanelTime;
	g_stSetupData.DVDTime = DVDTime;
	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_SETUP,0,
	 	sizeof(APP_SETTING_Setup_t),&(g_stSetupData));
	AL_Setting_Store(APP_Data_UserSetting_Handle(),SYS_SET_ID_SETUP, 0,
		sizeof(APP_SETTING_Setup_t));
#endif
#ifdef SUPPORT_HIDE_TVSOURCE
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO, 0,
		sizeof(APP_SETTING_UserInfo_t), &(g_stUserInfoData));
	if(g_stUserInfoData.HideTVSource == 1)
	{
		g_stUserInfoData.SourceIndex = APP_SOURCE_AV;
		g_stUserInfoData.LastTVSource = APP_SOURCE_AV;
		g_stUserInfoData.AutoInstalled = 0;
	}
	AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_USERINFO,0,
	 	sizeof(APP_SETTING_UserInfo_t),&(g_stUserInfoData));
	AL_Setting_Store(APP_Data_UserSetting_Handle(),SYS_SET_ID_USERINFO, 0,
		sizeof(APP_SETTING_UserInfo_t));
#endif
#ifdef CONFIG_SUPPORT_AUTO_WHITE_BALANCE
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
			sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData));
	memcpy(stColorTemp,g_stFactoryUserData.PictureMode.WBAdjust.stColorTemp,
		sizeof(APP_SETTING_ColorTempTab_t) * APP_VIDEO_PQ_STORE_SOURCE_TYPE_MAX);
#endif

	App_Data_UserSetting_FM_Restore();

#ifdef CONFIG_SUPPORT_AUTO_WHITE_BALANCE
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
			sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData));
	memcpy(g_stFactoryUserData.PictureMode.WBAdjust.stColorTemp,stColorTemp,
		sizeof(APP_SETTING_ColorTempTab_t) * APP_VIDEO_PQ_STORE_SOURCE_TYPE_MAX);
	
	AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,
		sizeof(APP_SETTING_FactoryUser_t),&g_stFactoryUserData);
	AL_Setting_Store(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,
		sizeof(APP_SETTING_FactoryUser_t));
#endif

	App_Data_UserSetting_ResetLangContry_ByFMDefaultValue();
	APP_Video_ResetTVSetting();
#ifdef SUPPORT_HKC_FACTORY_REMOTE	
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER, 0,
		sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData));
		if(  g_stFactoryUserData.n_FactSet_FactoryRemote ==1)
		{
			g_stFactoryUserData.n_FactSet_FactoryRemote =0;
			g_stFactoryUserData.Function.n_Funct_PresetChannel=0;
		}					
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,0,
		sizeof(APP_SETTING_FactoryUser_t), &(g_stFactoryUserData)); 
		AL_Setting_Store(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,
		sizeof(g_stFactoryUserData));
#endif

    GL_TaskSleep(1900);

// --------------------------------------------------------------------
	//Note: Need set some function again
	// 1) Show logo
	MID_TVFE_HideLogo( (BOOL)AL_FLASH_GetPowerShowLogo() );

	// 2) AC auto power on
	if(AL_FLASH_GetACAutoPowerOn()>0)
		MID_TVFE_SetAutoPowerOn(TRUE);
	else if(AL_FLASH_GetACAutoPowerOn()==0)
		MID_TVFE_SetAutoPowerOn(FALSE);

    {
		g_stFactoryUserData.n_FactSet_BurningMode = 1;
		MID_TVFE_SetAutoPowerOn(TRUE);
		AL_FLASH_SetACAutoPowerOn(AC_AUTO_POWERON_ON);
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, n_FactSet_BurningMode),
			sizeof(g_stFactoryUserData.n_FactSet_BurningMode),&(g_stFactoryUserData.n_FactSet_BurningMode));
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, n_FactSet_BurningMode),
			sizeof(g_stFactoryUserData.n_FactSet_BurningMode));
    }
	// 3) Panel Inverce
	//MID_TVFE_SetPanelInverse(g_stFactoryUserData.Function.PanelSetting.n_FlipIndex);

	// 4) Reset panel set
	MID_TVFE_InitLVDS();
	tv_SetPanelIndex(0);	
#if defined(SUPPORT_FACTORY_AUTO_TEST) && defined(CONFIG_CTV_UART_FAC_MODE)
	if(APP_Factory_GetUartFacModeOnOff()==TRUE)
	{
		con_AccuviewRs232_CMD_Response(CTV_RS232_CMD_FAC_SHIPPING_MODE, 0);		
		GL_TaskSleep(50);
	}
#endif
#if 1
	MAINAPP_SendGlobalEvent(UI_EVENT_POWER, AL_POWER_STATE_OFF);
	printf("\n***RESET:OK***\n");
#else//after import restart system remove by degui.xia
	APP_Sysset_Reset_System();
#endif
    return ;

    /* if no reboot system after reset database(...),
     * how to reset global variable(s) and assure run at default state
     */
#ifdef CONFIG_DTV_SUPPORT
    /* reset global variable(s), add others below */
    APP_DVB_Playback_SetNextServiceType(AL_RECTYPE_DVBTV);
#endif
    GL_TaskSleep(500);
#ifdef CONFIG_ATV_SUPPORT
	if (APP_SOURCE_ATV != eSourType)
	{
		SYSAPP_GOBJ_DestroyAllGUIObject(dSysAppIdx);
		APP_GUIOBJ_Source_SetAppSource(APP_SOURCE_ATV);
	}
	else
	{
		SYSAPP_IF_SendGlobalEventWithIndex(
			SYS_APP_ATV, APP_ATV_GLOBAL_EVENT_ATV_ONRUN | PASS_TO_SYSAPP, FALSE);
	}
#endif
}

void Factory_FactSet_ShippingMode(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if (bSet)
	{
		if(g_bShippingFlag != FM_SHIPPING_STOP)
		{
			return;
		}
		g_bShippingFlag = FM_SHIPPING_START;
		UpdateNodeFunctionContent(ID_FM_FactSet_Version,0,0);
		// UMF set function
		_FM_FactorySetting_Shipping();

		g_bShippingFlag = FM_SHIPPING_STOP;
		#ifdef CELLO_BATHROOMTV
		g_bRealStandby = TRUE;
		#endif

	}
	else
	{
		// UMF get function
		pValue->value = g_bShippingFlag;
	}
}


/* 9.1 Panel Setting   start*/
//		ID_FM_Fun_Panl_PWMFREQ,					//235
void Factory_Fun_Panl_PWMFREQ(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetPWMFreq((UINT32)FuncData->value);
		/*
		MID_TVFE_SetPWMFreq((UINT8)FuncData->value);
		g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_PWMFREQ = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.PanelSetting.n_Fun_Panl_PWMFREQ),
				sizeof(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_PWMFREQ),
				&(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_PWMFREQ));
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.PanelSetting.n_Fun_Panl_PWMFREQ),
				sizeof(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_PWMFREQ));
		*/

		// Set backlight again (Mantis 0022770)
	    Backlight_t BacklightSetting;
	    BacklightSetting.Backlight_total_Stage = 100; // set total backlight stage = 100
        BacklightSetting.OSD_backlight_index = AL_FLASH_GetBackLight();
		Cmd_SetLcdBackLight(BacklightSetting);
		
	}
	else
	{
		// UMF get function
		//FuncData->value=g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_PWMFREQ ;
		UINT32 nFreq = 0;
		MID_TVFE_GetCurrPWMFreq((unsigned int *)&(nFreq));

		FuncData->value = (double)nFreq;
	}

}
/*
//	ID_FM_Fun_Panl_PixelClock,					//236
void Factory_Fun_Panl_PixelClock(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_PixelClock = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_PixelClock ;
	}

}*/
//	ID_FM_Fun_Panl_ColorDepth,					//237
void Factory_Panl_ColorDepth(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetLVDS(1, LVDS_PANEL_BIT, 0, (&FuncData->value));
		g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_ColorDepth = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.PanelSetting.n_Fun_Panl_ColorDepth),
				sizeof(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_ColorDepth),
				&(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_ColorDepth));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.PanelSetting.n_Fun_Panl_ColorDepth),
				sizeof(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_ColorDepth));
		*/
	}
	else
	{
		// UMF get function
		//FuncData->value=g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_ColorDepth ;
		MID_TVFE_SetLVDS(0, LVDS_PANEL_BIT, 0, (&FuncData->value));
	}

}
//	ID_FM_Fun_Panl_LVDSChannelSwap,				//238
void Factory_Panl_LVDSChannelSwap(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetLVDS(1, LVDS_CHANNEL_SWAP_ENABLE, 3, (&FuncData->value));
		g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSChannelSwap = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.PanelSetting.n_Fun_Panl_LVDSChannelSwap),
				sizeof(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSChannelSwap),
				&(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSChannelSwap));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.PanelSetting.n_Fun_Panl_LVDSChannelSwap),
				sizeof(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSChannelSwap));
		*/
	}
	else
	{
		// UMF get function
		//FuncData->value=g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSChannelSwap ;
		MID_TVFE_SetLVDS(0, LVDS_CHANNEL_SWAP_ENABLE, 3, (&FuncData->value));
	}

}
//ID_FM_Fun_Panl_ColorDepth,					//237
void Factory_Panl_LVDSConverter(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetLVDS(1, LVDS_TYPE, 1, (&FuncData->value));
		g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSConverter = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.PanelSetting.n_Fun_Panl_LVDSConverter),
				sizeof(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSConverter),
				&(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSConverter));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.PanelSetting.n_Fun_Panl_LVDSConverter),
				sizeof(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSConverter));
		*/
	}
	else
	{
		// UMF get function
		//FuncData->value=g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSConverter ;
		MID_TVFE_SetLVDS(0, LVDS_TYPE, 1, (&FuncData->value));
	}

}
//	ID_FM_Fun_Panl_LVDSSingleDual,				//240
void Factory_Panl_LVDSSingleDual(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetLVDS(1, LVDS_DUAL_CHANNEL_ENABLE, 2, (&FuncData->value));
		g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSSingleDual = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.PanelSetting.n_Fun_Panl_LVDSSingleDual),
				sizeof(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSSingleDual),
				&(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSSingleDual));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.PanelSetting.n_Fun_Panl_LVDSSingleDual),
				sizeof(g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSSingleDual));
		*/
	}
	else
	{
		// UMF get function
		//FuncData->value=g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_LVDSSingleDual ;
		MID_TVFE_SetLVDS(0, LVDS_DUAL_CHANNEL_ENABLE, 2, (&FuncData->value));
	}

}
/*
//	ID_FM_Fun_Panl_Type,						//241
void Factory_Panl_Type(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_Type = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_Type ;
	}

}

//	ID_FM_Fun_Panl_BitOrder,    //匡虫GUI		//242
void Factory_Panl_BitOrder(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_BitOrder = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER, 0,sizeof(g_stFactoryUserData),&g_stFactoryUserData);
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Function.PanelSetting.n_Fun_Panl_BitOrder ;
	}

}
*/

void Factory_Panl_Index(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{

	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
		if(bSet)
		{
			//UMF function
			g_stFactoryUserData.Function.PanelSetting.n_MultiplePanelIndex = FuncData->value;
			AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
				ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.PanelSetting.n_MultiplePanelIndex),
					sizeof(g_stFactoryUserData.Function.PanelSetting.n_MultiplePanelIndex),
					&(g_stFactoryUserData.Function.PanelSetting.n_MultiplePanelIndex));
			/*
			AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
				ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.PanelSetting.n_MultiplePanelIndex),
					sizeof(g_stFactoryUserData.Function.PanelSetting.n_MultiplePanelIndex));
			*/
		}
		else
		{
			FuncData->value = g_stFactoryUserData.Function.PanelSetting.n_MultiplePanelIndex ;
		}
}

UINT8 Factory_GetFlipIndex(void)
{
	return u8FlipIndex;
}

void Factory_Panl_Flip(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	UINT8 u8DriverFlipIndex;		
	MID_DISP_DTVGetFlip((MID_DISP_FlipType_t *)(&u8DriverFlipIndex));		

	if(bSet)
	{
		//UMF function
		if(FuncData->value != u8DriverFlipIndex)
		{
			MID_DISP_DTVSetFlip((MID_DISP_FlipType_t)(FuncData->value) ); // added for VIP test
			extern INT32 OSD_SetInvert(BOOL InvertOSD);
			OSD_SetInvert((FuncData->value==MID_DISP_FLIP_TYPE_NO_FLIP)?FALSE:TRUE);
		}
	}
	else
	{
		FuncData->value = u8DriverFlipIndex;
	}
}

void Factory_Panl_BackLight(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
    	Backlight_t BacklightSetting;
    	BacklightSetting.Backlight_total_Stage = 100; // set total backlight stage = 100
	if (bSet)
	{
		DEBF("[FM DBG] %s SET (%d) ...\n", __FUNCTION__, (UINT8) FuncData->value);
		AL_FLASH_SetBackLight((UINT8) FuncData->value);
        	BacklightSetting.OSD_backlight_index = FuncData->value;

		Cmd_SetLcdBackLight(BacklightSetting);
	}
	else
	{
		// UMF get function
		FuncData->value = (INT32)AL_FLASH_GetBackLight();

		DEBF("[FM DBG] %s GET (%d) ...\n", __FUNCTION__, (UINT8) FuncData->value);
	}
}

void Factory_Panl_BackLight_Polarity(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Backlight_t BacklightSetting;
	BacklightSetting.Backlight_total_Stage = 100;
	BacklightSetting.OSD_backlight_index = (INT32)AL_FLASH_GetBackLight();
	if (bSet)
	{
		DEBF("[FM DBG] %s SET (%d) ...\n", __FUNCTION__, (UINT8) FuncData->value);
		u8BackLight_Polarity = FuncData->value;
		Cmd_SetPanlBackLightPolarity(u8BackLight_Polarity);
		Cmd_SetLcdBackLight( BacklightSetting);
		#ifdef CONFIG_SUPPORT_PWM_DUTY_IN_DECIMAL
		if (u8BackLight_Polarity == 1)
			Cmd_SetPanlDutyPWM(1000-u16DutyPWM);
		else
			Cmd_SetPanlDutyPWM(u16DutyPWM);
		#else
		if (u8BackLight_Polarity == 1)
			Cmd_SetPanlDutyPWM(100-u16DutyPWM);
		else
			Cmd_SetPanlDutyPWM(u16DutyPWM);
		#endif
		UpdateNodeFunctionContent(ID_FM_Fun_Panl_PWMFREQ,0,0);
	}
	else
	{
		FuncData->value = (UINT32)u8BackLight_Polarity;
		DEBF("[FM DBG] %s GET (%d) ...\n", __FUNCTION__, (UINT8) FuncData->value);
	}
}
void Factory_Panl_Duty_PWM(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Backlight_t BacklightSetting;
	BacklightSetting.Backlight_total_Stage = 100;
	BacklightSetting.OSD_backlight_index = (INT32)AL_FLASH_GetBackLight();
	if (bSet)
	{
		DEBF("[FM DBG] %s SET (%d) ...\n", __FUNCTION__, (UINT8) FuncData->value);
		u16DutyPWM = FuncData->value;
		Cmd_SetPanlBackLightPolarity(u8BackLight_Polarity);
		Cmd_SetLcdBackLight( BacklightSetting);
		#ifdef CONFIG_SUPPORT_PWM_DUTY_IN_DECIMAL
		if (u8BackLight_Polarity == 1)
			Cmd_SetPanlDutyPWM(1000-u16DutyPWM);
		else
			Cmd_SetPanlDutyPWM(u16DutyPWM);
		#else
		if (u8BackLight_Polarity == 1)
			Cmd_SetPanlDutyPWM(100-u16DutyPWM);
		else
			Cmd_SetPanlDutyPWM(u16DutyPWM);
		#endif
		UpdateNodeFunctionContent(ID_FM_Fun_Panl_PWMFREQ,0,0);
	}
	else
	{
		FuncData->value = (UINT32)u16DutyPWM;
		DEBF("[FM DBG] %s GET (%d) ...\n", __FUNCTION__, (UINT8) FuncData->value);
	}
}
void Factory_Panl_mA_Grade(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		DEBF("[FM DBG] %s SET (%d) ...\n", __FUNCTION__, (UINT8) FuncData->value);
		u8mAGrade = FuncData->value;
		UpdateNodeFunctionContent(ID_FM_Fun_Panl_PWMFREQ,0,0);
	}
	else
	{
		FuncData->value = (UINT32)u8mAGrade;
		DEBF("[FM DBG] %s GET (%d) ...\n", __FUNCTION__, (UINT8) FuncData->value);
	}
}
void Factory_Panl_mA_Range(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	#if (HW_ADJ_INVERT == 1)
	if(u8BackLight_Polarity == 1)
	{
		#ifdef CONFIG_SUPPORT_PWM_DUTY_IN_DECIMAL
			FuncData->value = g_400mA_700mARange[u16DutyPWM/5];
		#else
		if(u8mAGrade == 0)
			FuncData->value = g_400mARange[u16DutyPWM];
		else if(u8mAGrade == 1)
			FuncData->value = g_700mARange[u16DutyPWM];
		else
			FuncData->value = g_900mARange[u16DutyPWM];
		#endif
	}
	else
	{
		#ifdef CONFIG_SUPPORT_PWM_DUTY_IN_DECIMAL
			FuncData->value = g_400mA_700mARange[200-(u16DutyPWM/5)];
		#else
		if(u8mAGrade == 0)
			FuncData->value = g_400mARange[100-u16DutyPWM];
		else if(u8mAGrade == 1)
			FuncData->value = g_700mARange[100-u16DutyPWM];
		else
			FuncData->value = g_900mARange[100-u16DutyPWM];
		#endif
	}
	#else
	if(u8BackLight_Polarity == 1)
	{
		#ifdef CONFIG_SUPPORT_PWM_DUTY_IN_DECIMAL
			FuncData->value = g_400mA_700mARange[200-(u16DutyPWM/5)];
		#else
		if(u8mAGrade == 0)
			FuncData->value = g_400mARange[100-u16DutyPWM];
		else if(u8mAGrade == 1)
			FuncData->value = g_700mARange[100-u16DutyPWM];
		else
			FuncData->value = g_900mARange[100-u16DutyPWM];
		#endif
	}
	else
	{
		#ifdef CONFIG_SUPPORT_PWM_DUTY_IN_DECIMAL
			FuncData->value = g_400mA_700mARange[u16DutyPWM/5];
		#else
		if(u8mAGrade == 0)
			FuncData->value = g_400mARange[u16DutyPWM];
		else if(u8mAGrade == 1)
			FuncData->value = g_700mARange[u16DutyPWM];
		else
			FuncData->value = g_900mARange[u16DutyPWM];
		#endif
	}	
	#endif
}
//	printf("[FM DBG] %s SET (%d:%d) ...\n", __FUNCTION__, (UINT8) FuncData->value,u16DutyPWM);
#ifdef SUPPORT_FACTORY_BURNING_ADJ_TEST
static UINT8 _APP_GetBacklightValueByCurrentValue(UINT16 Current_Array[], UINT16 Current_Value)
{
	UINT8 tmp_Backlight_Value = 0;
	UINT8 Backlight_Value = 0;
#ifdef CONFIG_SUPPORT_PWM_DUTY_IN_DECIMAL
	for(tmp_Backlight_Value = 0; tmp_Backlight_Value <= 200; tmp_Backlight_Value ++)
#else
	for(tmp_Backlight_Value = 0; tmp_Backlight_Value <= 100; tmp_Backlight_Value ++)
#endif
	{
		Backlight_Value = tmp_Backlight_Value;
		if(Current_Value > Current_Array[tmp_Backlight_Value])
		{
			continue;
		}
		else
		{
			if(0 == tmp_Backlight_Value)
			{
				break;
			}
			else
			{
				Backlight_Value = ((((Current_Array[tmp_Backlight_Value] - Current_Value) - (Current_Value - Current_Array[tmp_Backlight_Value - 1])) < 0) ? (tmp_Backlight_Value) : (tmp_Backlight_Value - 1));
				break;
			}

		}
	}
#ifdef CONFIG_SUPPORT_PWM_DUTY_IN_DECIMAL
		Backlight_Value = Backlight_Value/2;
#endif
	return Backlight_Value;
}

UINT8 APP_GetBacklightValueByCurrentValue(UINT16 Current_Value)
{
	UINT8 Backlight_Value = 0;


    #ifdef CONFIG_SUPPORT_PWM_DUTY_IN_DECIMAL
	if(718 > Current_Value)
	{
		Backlight_Value = _APP_GetBacklightValueByCurrentValue(g_400mA_700mARange, Current_Value);	//700
	}

	else
	{
	
	    printf("fail: APP_GetBacklightValueByCurrentValue:Current_Value=%d--->Backlight_Value==%d\n", Current_Value, Backlight_Value);
		return Current_Value;
	}
    #else
	if(418 > Current_Value)
	{
		Backlight_Value = _APP_GetBacklightValueByCurrentValue(g_400mARange, Current_Value);	//400
	}
	else if((718 > Current_Value) && (418 <= Current_Value))
	{
		Backlight_Value = _APP_GetBacklightValueByCurrentValue(g_700mARange, Current_Value);	//700
	}
	else if((920 > Current_Value) && (718 <= Current_Value))
	{
		Backlight_Value = _APP_GetBacklightValueByCurrentValue(g_900mARange, Current_Value);	//900
	}
	else
	{
	
	    printf("fail: APP_GetBacklightValueByCurrentValue:Current_Value=%d--->Backlight_Value==%d\n", Current_Value, Backlight_Value);
		return Current_Value;
	}
	#endif
	printf("success: APP_GetBacklightValueByCurrentValue:Current_Value=%d--->Backlight_Value==%d\n", Current_Value, Backlight_Value);
	return Backlight_Value;
}
#endif

/*9.1 Panel Setting End*/
/* 9.3 SSC Adjust     start */
//	ID_FM_Fun_SSC_DpllClkrate,								//243
void Factory_SSC_DramSpectrum(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetSSC(EMICMD_DRAMSSC, FuncData->value);
		g_stFactoryUserData.Function.SSCAdjust.n_Fun_SSC_DramSpectrum= FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.SSCAdjust.n_Fun_SSC_DramSpectrum),
				sizeof(g_stFactoryUserData.Function.SSCAdjust.n_Fun_SSC_DramSpectrum),
				&(g_stFactoryUserData.Function.SSCAdjust.n_Fun_SSC_DramSpectrum));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.SSCAdjust.n_Fun_SSC_DramSpectrum),
				sizeof(g_stFactoryUserData.Function.SSCAdjust.n_Fun_SSC_DramSpectrum));
		*/
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Function.SSCAdjust.n_Fun_SSC_DramSpectrum ;
	}

}
//	Factory_SSC_LVDSSpectrum,								//244
void Factory_SSC_LVDSSpectrum(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		MID_TVFE_SetSSC(EMICMD_LVDSSSC, FuncData->value);
		g_stFactoryUserData.Function.SSCAdjust.n_Fun_SSC_LVDSSpectrum = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.SSCAdjust.n_Fun_SSC_LVDSSpectrum),
				sizeof(g_stFactoryUserData.Function.SSCAdjust.n_Fun_SSC_LVDSSpectrum),
				&(g_stFactoryUserData.Function.SSCAdjust.n_Fun_SSC_LVDSSpectrum));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.SSCAdjust.n_Fun_SSC_LVDSSpectrum),
				sizeof(g_stFactoryUserData.Function.SSCAdjust.n_Fun_SSC_LVDSSpectrum));
		*/
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Function.SSCAdjust.n_Fun_SSC_LVDSSpectrum ;
	}

}
/*9.2 Hotel mode start*/
//ID_FM_Funct_HotelMode,					//74
void Factory_Funct_HotelMode(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{

		g_stFactoryUserData.Function.n_Funct_HotelMode = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.n_Funct_HotelMode),
				sizeof(g_stFactoryUserData.Function.n_Funct_HotelMode),
				&(g_stFactoryUserData.Function.n_Funct_HotelMode));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.n_Funct_HotelMode),
				sizeof(g_stFactoryUserData.Function.n_Funct_HotelMode));
		*/
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.Function.n_Funct_HotelMode;
	}
}

void Factory_Funct_DemodFilter(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	//FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		Cmd_Tuner_Demod_Set_Ddmod_Filter( (UINT8) (pValue->value) );

		g_stFactoryUserData.Function.n_Funct_DemodFilter = (UINT8)(pValue->value);
		AL_Setting_Write(
			APP_Data_UserSetting_Handle(), 
			SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t,Function.n_Funct_DemodFilter),
			sizeof(al_uint8),
			&(g_stFactoryUserData.Function.n_Funct_DemodFilter)
		);
	}
	else
	{
		pValue->value = (double)(g_stFactoryUserData.Function.n_Funct_DemodFilter);
	}
}

/*9.2 Hotel mode end*/
INT32 APP_GUIOBJ_FM_AtvChnSet_LoadSettingData(void)
{
	UINT8 i = 0;
	UINT8 len = 0;
	AL_RecHandle_t hProg;
	AL_ServiceDetail_t stServInfo;

	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,0,
			sizeof(APP_SETTING_FactoryUser_t), &g_stFactoryUserData);
	if(g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[g_s8CurChanIdx-1].Skip == 1)
	{
		printf("error ll.jing 11111\n");
		return SP_ERR_FAILURE;
	}

	AL_DB_Reset(AL_DBTYPE_DVB_ATV, al_true);
	APP_ATV_Playback_ResetFirstService();
	
#ifdef CONFIG_SUPPORT_ATV_SCAN_NTSCM_PALM_N
	UINT8 tvtype = 0;
    if (APP_GUIOBJ_Source_GetCurATVType() == ATV_TYPE_AIR)
    {
        tvtype = 0;
    }
    else
    {
        tvtype = 1;
    }
#endif
	for (i = 0; i < APP_FM_PRECHANNEL_NUMBER; i++)
	{
		if(g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[i].Skip == 1)
		{
			break;
		}
		memset(&stServInfo, 0, sizeof(AL_ServiceDetail_t));

		len = strlen((char *)g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[i].ChannelName);
		len = (len > APP_FM_PRECHANNEL_LEN) ? APP_FM_PRECHANNEL_LEN : len;

		memset((char *)stServInfo.stAnalogServ.szChannelName, 0, APP_FM_PRECHANNEL_LEN);
		memcpy((char *)stServInfo.stAnalogServ.szChannelName, (char *)g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[i].ChannelName, len);
		stServInfo.stAnalogServ.colorSystem = g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[i].ColorSystem;
		stServInfo.stAnalogServ.soundSystem = g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[i].SoundSystem;
		stServInfo.stAnalogServ.u32Freq = g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[i].Frequency;
		stServInfo.stAnalogServ.skip = g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[i].Skip;
		stServInfo.stAnalogServ.AFC = g_stFactoryUserData.SystemConfig.PreChannelSetting.ATVChannelSetting[i].AFC;
		stServInfo.stAnalogServ.inuse = 1;
#ifdef CONFIG_SUPPORT_ATV_SCAN_NTSCM_PALM_N
		stServInfo.stAnalogServ.available = 1;
        stServInfo.stAnalogServ.tvtype = tvtype;
		stServInfo.stAnalogServ.del = 0;
#endif
		AL_DB_AddRecord(AL_DBTYPE_DVB_ATV, AL_RECTYPE_ATVSERVICE, (al_void const *)&stServInfo, AL_DB_INVALIDHDL, &hProg);
	}
	AL_DB_Sync(AL_DBTYPE_DVB_ATV, al_true);
    return SP_SUCCESS;
}
#if defined( CONFIG_DVB_SYSTEM_DVBT_SUPPORT)
INT32 APP_GUIOBJ_FM_DtvChnSet_LoadSettingData(void)
{
	DEBF("[Common fm factory guiobj] %s is called.\n", __FUNCTION__);
	static AL_RecHandle_t hNetHdl = AL_DB_INVALIDHDL;
	AL_NetworkDetail_t stRootNet;
	AL_RecHandle_t hProg = AL_DB_INVALIDHDL;
	AL_RecHandle_t FirsthProg = AL_DB_INVALIDHDL;
	AL_RecHandle_t hTp = AL_DB_INVALIDHDL;
	AL_MultiplexDetail_t ts;
	AL_ServiceDetail_t Program;
	UINT32 u32TotalNum = 0;
	al_bool bHasValidServ = al_false;
	UINT8 i = 0;
	UINT32 u32CurrentSysappIndex = TOTAL_SYS_APP_SIZE;
	MAINAPP_GetActiveSystemAppIndex(&u32CurrentSysappIndex);
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,0,
			sizeof(APP_SETTING_FactoryUser_t), &g_stFactoryUserData);
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_CHANNEL, 0,
		sizeof(APP_SETTING_Channel_t), &(g_stChannelData));
#ifdef CONFIG_QSD
//for chip test using to set to special dtv frequency
	u32TotalNum = 1;
#else
	u32TotalNum = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.TotalNum;
#endif
	AL_DB_Reset(AL_DBTYPE_DVB_T, al_true);

	if(u32TotalNum == 0)
	{
		extern void APP_Sysset_Reset_list(AL_DB_EDBType_t eDBType);
		APP_Sysset_Reset_list(AL_DBTYPE_DVB_T);
		
		SYSAPP_GOBJ_SendMsgToSingleGUIObject(SYS_APP_DVB,
											 DVB_GUIOBJ_PLAYBACK,
											 APP_DVB_INTRA_EVENT_STOP_PLAYBACK,
											 (PLAYBACK_STOP_ALL | PLAYBACK_DESTROY_EPG));
		return SP_ERR_FAILURE;
	}
	AL_DB_GetRecord(AL_DB_REQ_GETFIRST, AL_DBTYPE_DVB_T, AL_RECTYPE_DVBNETWORK, &hNetHdl);
	if(hNetHdl == AL_DB_INVALIDHDL)
	{
		stRootNet.stDVBTNetwork.usNetId = 1;
		memcpy(stRootNet.stDVBTNetwork.szNetName, "Dummy", 6);
		AL_DB_AddRecord(AL_DBTYPE_DVB_T, AL_RECTYPE_DVBNETWORK, &stRootNet, AL_DB_INVALIDHDL, &hNetHdl);
	}
	for(i=0;i<u32TotalNum;i++)
	{
		memset(&ts, 0, sizeof(AL_MultiplexDetail_t));
	#ifdef CONFIG_QSD
	//for chip test using to set to special dtv frequency
		ts.stDVBTMultiplex.uiFreqK = 690000;
		ts.stDVBTMultiplex.ucBandwidth = 8;
	#else
		ts.stDVBTMultiplex.uiFreqK = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[i].Frequency;
		ts.stDVBTMultiplex.ucBandwidth = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[i].BoundWidth;
	#endif

	#ifdef CONFIG_QSD
		#ifdef CONFIG_CHIP_506
			ts.stDVBTMultiplex.ucTp_dvb_type = EDVB_DELIVER_TYPE_T;
		#else
			ts.stDVBTMultiplex.ucTp_dvb_type = EDVB_DELIVER_TYPE_T2;
		#endif
	#else
		if(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[i].DVBDeliveryType == EDVB_DELIVER_TYPE_T2)
		{
			ts.stDVBTMultiplex.ucTp_dvb_type = EDVB_DELIVER_TYPE_T2;
		}
		else
		{
			ts.stDVBTMultiplex.ucTp_dvb_type = EDVB_DELIVER_TYPE_T;
		}
	#endif
		ts.stDVBTMultiplex.ucAreaIndex = g_stChannelData.Country;
		ts.stDVBTMultiplex.ucHierarchyMode = FRONTEND_HIERARCHY_AUTO;
		AL_DB_AddRecord(AL_DBTYPE_DVB_T, AL_RECTYPE_DVBMULTIPLEX, &ts, hNetHdl, &hTp);

		memset(&Program, 0, sizeof(AL_ServiceDetail_t));
		Program.stDVBTServ.eSDTSrvType = AL_RECTYPE_DVBTV;
		Program.stDVBTServ.eTvRadioType= AL_RECTYPE_DVBTV;
		Program.stDVBTServ.szProgName[0] = 0;
		Program.stDVBTServ.usOrigNetId = 0x20BF+i;
		Program.stDVBTServ.usServiceId = 0x01F4;
		Program.stDVBTServ.usTsId = 0xD403+i;
		Program.stDVBTServ.usTsId_pat = Program.stDVBTServ.usTsId;
		Program.stDVBTServ.usProgNo = 0x1+i;
		
		if(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBTChannelSetting.ChnValue[i].DVBDeliveryType == EDVB_DELIVER_TYPE_T2)
		{
			Program.stDVBTServ.usPlpId = 0;
		}
	#if (defined(CONFIG_QSD) && defined(CONFIG_CHIP_531))
		Program.stDVBTServ.usPMTPid= 2000;
		Program.stDVBTServ.stVideoPid.usDataPID = 2001;
		Program.stDVBTServ.stAudioPid[0].usDataPID= 2002;
	#else
		Program.stDVBTServ.usPMTPid= 0x1F55;
		Program.stDVBTServ.stVideoPid.usDataPID = 0x1F40;
		Program.stDVBTServ.stAudioPid[0].usDataPID= 0x1F41;
	#endif
		AL_DB_AddRecord(AL_DBTYPE_DVB_T, AL_RECTYPE_DVBTV,
			&Program, hTp, &hProg);
		if(i == 0)
		{
			FirsthProg = hProg;
		}
	}
	AL_DB_Sync(AL_DBTYPE_DVB_T, al_true);
	bHasValidServ = AL_DB_HasVisibleService(AL_DBTYPE_DVB_T);
	if(!bHasValidServ)
	{
		APP_DVB_Playback_SetCurrentRFTypeByNetType(AL_DBTYPE_DVB_T);
		APP_DVB_Playback_SetCurrServiceType(AL_DBTYPE_DVB_T, AL_RECTYPE_DVBTV);
		APP_DVB_Playback_SetCurrentProgHandle(AL_DBTYPE_DVB_T,AL_RECTYPE_DVBTV,FirsthProg);
		if(u32CurrentSysappIndex == SYS_APP_DVB)
		{
			SYSAPP_IF_SendGlobalEventWithIndex(SYS_APP_DVB, APP_DVB_GLOBAL_EVENT_DVB_ONRUN|PASS_TO_SYSAPP, FirsthProg);
		}
	}
    return SP_SUCCESS;
}
#endif
#if defined( CONFIG_DVB_SYSTEM_DVBC_SUPPORT)
INT32 APP_GUIOBJ_FM_Dvb_C_ChnSet_LoadSettingData(void)
{
	DEBF("[Common fm factory guiobj] %s is called.\n", __FUNCTION__);
	static AL_RecHandle_t hNetHdl = AL_DB_INVALIDHDL;
	AL_NetworkDetail_t stRootNet;
	AL_RecHandle_t hProg = AL_DB_INVALIDHDL;
	//AL_RecHandle_t FirsthProg = AL_DB_INVALIDHDL;
	AL_RecHandle_t hTp = AL_DB_INVALIDHDL;
	AL_MultiplexDetail_t ts;
	AL_ServiceDetail_t Program;
	UINT32 u32TotalNum = 0;
	//UINT8 u8RecordsNum = DVBApp_Get_TotalNumber();
	UINT8 i = 0;
	UINT32 u32CurrentSysappIndex = TOTAL_SYS_APP_SIZE;
	MAINAPP_GetActiveSystemAppIndex(&u32CurrentSysappIndex);
	
	AL_DB_Reset(AL_DBTYPE_DVB_C, al_true);
	
	AL_Setting_Read(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,0,
			sizeof(APP_SETTING_FactoryUser_t), &g_stFactoryUserData);
	u32TotalNum = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.TotalNum;
	if(u32TotalNum == 0)
	{
		extern void APP_Sysset_Reset_list(AL_DB_EDBType_t eDBType);
		APP_Sysset_Reset_list(AL_DBTYPE_DVB_C);
		
		SYSAPP_GOBJ_SendMsgToSingleGUIObject(SYS_APP_DVB,
											 DVB_GUIOBJ_PLAYBACK,
											 APP_DVB_INTRA_EVENT_STOP_PLAYBACK,
											 (PLAYBACK_STOP_ALL | PLAYBACK_DESTROY_EPG));
		return SP_ERR_FAILURE;
	}
	AL_DB_GetRecord(AL_DB_REQ_GETFIRST, AL_DBTYPE_DVB_C, AL_RECTYPE_DVBNETWORK, &hNetHdl);
	if(hNetHdl == AL_DB_INVALIDHDL)
	{
		stRootNet.stDVBCNetwork.usNetId = 1;
		memcpy(stRootNet.stDVBCNetwork.szNetName, "Dummy", 6);
		AL_DB_AddRecord(AL_DBTYPE_DVB_C, AL_RECTYPE_DVBNETWORK, &stRootNet, AL_DB_INVALIDHDL, &hNetHdl);
	}
	for(i=0;i<u32TotalNum;i++)
	{
		memset(&ts, 0, sizeof(AL_MultiplexDetail_t));
		ts.stDVBCMultiplex.uiFreqK = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[i].Frequency;
		ts.stDVBCMultiplex.usSymRateK = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[i].SymbolRate;
		ts.stDVBCMultiplex.ucQamSize = g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[i].Modulation;
		if(g_stFactoryUserData.SystemConfig.PreChannelSetting.DVBCChannelSetting.ChnValue[i].DVBDeliveryType == EDVB_DELIVER_TYPE_C2)
		{
			ts.stDVBCMultiplex.ucTp_dvb_type = EDVB_DELIVER_TYPE_C2;
		}
		else
		{
			ts.stDVBCMultiplex.ucTp_dvb_type = EDVB_DELIVER_TYPE_C;
		}

		AL_DB_AddRecord(AL_DBTYPE_DVB_C, AL_RECTYPE_DVBMULTIPLEX, &ts, hNetHdl, &hTp);
		memset(&Program, 0, sizeof(AL_ServiceDetail_t));
		Program.stDVBCServ.eSDTSrvType = AL_RECTYPE_DVBTV;
		Program.stDVBCServ.eTvRadioType= AL_RECTYPE_DVBTV;
		Program.stDVBCServ.szProgName[0] = 0;
		Program.stDVBCServ.usOrigNetId = 0x2222+i;
		Program.stDVBCServ.usServiceId = 0x2222;
		Program.stDVBCServ.usTsId = 0x2222+i;
		Program.stDVBCServ.usTsId_pat = Program.stDVBCServ.usTsId;
		Program.stDVBCServ.usProgNo = 0x1+i;
		Program.stDVBCServ.usPMTPid= 0x1F55;
		Program.stDVBCServ.stVideoPid.usDataPID = 0x1F40;
		Program.stDVBCServ.stAudioPid[0].usDataPID= 0x1F41;
		AL_DB_AddRecord(AL_DBTYPE_DVB_C, AL_RECTYPE_DVBTV,
			&Program, hTp, &hProg);
		#if 0 //no need change channel to dvbc
		if(i == 0)
		{
			FirsthProg = hProg;
		}
		#endif
	}
	AL_DB_Sync(AL_DBTYPE_DVB_C, al_true);
	#if 0//no need change channel to dvbc
	if(u8RecordsNum == 0)
	{
		APP_DVB_Playback_SetCurrentRFTypeByNetType(AL_DBTYPE_DVB_C);
		APP_DVB_Playback_SetCurrServiceType(AL_DBTYPE_DVB_C, AL_RECTYPE_DVBTV);
		APP_DVB_Playback_SetCurrentProgHandle(AL_DBTYPE_DVB_C,AL_RECTYPE_DVBTV,FirsthProg);
		if(u32CurrentSysappIndex == SYS_APP_DVB)
		{
			SYSAPP_IF_SendGlobalEventWithIndex(SYS_APP_DVB, APP_DVB_GLOBAL_EVENT_DVB_ONRUN|PASS_TO_SYSAPP, FirsthProg);
		}
	}
	#endif
    return SP_SUCCESS;
}
#endif
#ifdef SUPPORT_HKC_FACTORY_REMOTE	
INT32 _FM_Function_PresetChannelSaveValue(void)
#else
static INT32 _FM_Function_PresetChannelSaveValue(void)
#endif
{
	if(g_stFactoryUserData.Function.n_Funct_PresetChannel == 1)
	{
		APP_GUIOBJ_FM_AtvChnSet_LoadSettingData();
#if defined(CONFIG_DVB_SYSTEM) || defined(CONFIG_AUS_DVB_SYSTEM)
		#if defined( CONFIG_DVB_SYSTEM_DVBT_SUPPORT)
        AL_FW_SwitchDBModule(AL_DBTYPE_DVB_T);
		APP_GUIOBJ_FM_DtvChnSet_LoadSettingData();
		#endif
		#if defined( CONFIG_DVB_SYSTEM_DVBC_SUPPORT)
		AL_FW_SwitchDBModule(AL_DBTYPE_DVB_C);//avoid not init dvbc db ,can not sync channel to DB
		APP_GUIOBJ_FM_Dvb_C_ChnSet_LoadSettingData();
		#endif
		AL_FW_SwitchDBModule(APP_DVB_Playback_GetCurrentNetType());
#endif
#ifdef CONFIG_ATV_SUPPORT
		UINT32 u32ActiveSysAPP;
		MAINAPP_GetActiveSystemAppIndex(&u32ActiveSysAPP);
		if (u32ActiveSysAPP == SYS_APP_ATV)
		{ 
			APP_ATV_Playback_SetForcePlay();
			SYSAPP_GOBJ_SendMsgToSingleGUIObject(SYS_APP_ATV, ATV_GUIOBJ_PLAYBACK,
			    APP_ATV_INTRA_EVENT_START_PLAYBACK, TRUE);

		}
#endif
#ifdef CONFIG_DTV_SUPPORT
		MAINAPP_GetActiveSystemAppIndex(&u32ActiveSysAPP);
		if (u32ActiveSysAPP == SYS_APP_DVB)
		{ 
			SYSAPP_GOBJ_SendMsgToSingleGUIObject(SYS_APP_DVB, DVB_GUIOBJ_PLAYBACK,
				APP_DVB_INTRA_EVENT_STOP_PLAYBACK, PLAYBACK_STOP_ALL);

			SYSAPP_IF_SendGlobalEventWithIndex(
				SYS_APP_DVB, APP_DVB_GLOBAL_EVENT_DVB_ONRUN | PASS_TO_SYSAPP, FALSE);
		}
#endif
		g_stFactoryUserData.Function.n_Funct_PresetChannel = 0;
	}

	AL_Setting_Write(APP_Data_UserSetting_Handle(),SYS_SET_ID_FACTUSER,
		ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.n_Funct_PresetChannel),
		sizeof((g_stFactoryUserData.Function.n_Funct_PresetChannel)),&(g_stFactoryUserData.Function.n_Funct_PresetChannel));
	/*
	AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
		ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.n_Funct_PresetChannel),
		sizeof(g_stFactoryUserData.Function.n_Funct_PresetChannel));
	*/
	return SP_SUCCESS;
}

void Factory_Funct_PresetChannel(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		g_stFactoryUserData.Function.n_Funct_PresetChannel = FuncData->value;
		_FM_Function_PresetChannelSaveValue();
	}
	else
	{
		// UMF get function
		FuncData->value= g_stFactoryUserData.Function.n_Funct_PresetChannel;
	}
}


/* 9.3 SSC Adjust     start */
/*10. Update 	start*/

//ID_FM_Funct_FullTSRecord  //81
char *strTSState[] =
{
	"Stopped",
	"Initializing",
	"Recording",
	"No Device",
	"Fail",
	"Disk Full",
};
void Factory_Funct_FullTSRecord(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
#ifdef CONFIG_SUPPORT_PVR
	TS_REC_STATE nTSState = 3;
	if (bSet)
	{
		if(b_Isrecording)
		{
			// Update UI
			Update_TypeVersion_Node((char * )"Stopping...", (ID_FM_Funct_FullTSRecord - ID_FM_Funct_PanelSetting));
			APP_GUIOBJ_DVB_PvrRec_TSRecStop();
			//Get status
			nTSState = APP_GUIOBJ_DVB_PvrRec_TSRecGetState();
			Update_TypeVersion_Node(strTSState[nTSState], (ID_FM_Funct_FullTSRecord - ID_FM_Funct_PanelSetting));
			b_Isrecording = FALSE;
			Subtitle_SetDisplay(true);

		}
		else
		{
			Subtitle_SetDisplay(false);
			nTSState = APP_GUIOBJ_DVB_PvrRec_TSRecStart();
			// Update UI
			Update_TypeVersion_Node(strTSState[nTSState], (ID_FM_Funct_FullTSRecord - ID_FM_Funct_PanelSetting) );
			if(nTSState == TSRecording)
			{
				b_Isrecording = TRUE;
			}
		}
	}
	else
	{
		nTSState = APP_GUIOBJ_DVB_PvrRec_TSRecGetState();
		Update_TypeVersion_Node(strTSState[nTSState], (ID_FM_Funct_FullTSRecord - ID_FM_Funct_PanelSetting));
	}
#else

	Update_TypeVersion_Node("(NONE)", (ID_FM_Funct_FullTSRecord - ID_FM_Funct_PanelSetting));

#endif

}

//ID_FM_Funct_VideoPattern,					//82
void Factory_VideoPattern(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Function.n_Funct_VideoPattern = FuncData->value;

		if (VideoPatternMappingTbl[(UINT32)FuncData->value].wPatternType == VIP_TEST_PATTERN_DISABLE)
		{
			_APP_Fm_Disable_Video_InternalPattern();
			Subtitle_SetDisplay(true);
		}
		else
		{
			Subtitle_SetDisplay(false);
			MID_TVFE_SetInternalPattern(VideoPatternMappingTbl[(UINT32)FuncData->value]);
			MID_TVFE_SetColorTmpGammaTableIndex(0xff);
		}
		//Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.n_Funct_VideoPattern),
				sizeof(g_stFactoryUserData.Function.n_Funct_VideoPattern),
				&(g_stFactoryUserData.Function.n_Funct_VideoPattern));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.n_Funct_VideoPattern),
				sizeof(g_stFactoryUserData.Function.n_Funct_VideoPattern));
		*/
	}
	else
	{
		// UMF get function
		FuncData->value = g_stFactoryUserData.Function.n_Funct_VideoPattern;
	}
}
#ifdef SUPPORT_DELETE_SAME_SERVICES
void Factory_Delete_SameServices(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.Function.n_Funct_DeleteSameServices = FuncData->value;

		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.n_Funct_DeleteSameServices),
				sizeof(g_stFactoryUserData.Function.n_Funct_DeleteSameServices),
				&(g_stFactoryUserData.Function.n_Funct_DeleteSameServices));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, Function.n_Funct_DeleteSameServices),
				sizeof(g_stFactoryUserData.Function.n_Funct_DeleteSameServices));
		*/
	}
	else
	{
		FuncData->value = g_stFactoryUserData.Function.n_Funct_DeleteSameServices;
	}
}
#endif

//ID_FM_Update_USBUpdate,					//83
void Factory_Update_USBUpdate(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	if (bSet)
	{
		app_data_upgrade_status_e ret;
		ret = App_Data_USB_Upgrade_CheckFile(BUILD_NAME);
		if (ret == APP_DATA_CHECK_OK)
		{
			UINT32 dMessage;
			UINT32 dActiveSysApp;
			Upgrade_attr_type_e ReserveBOOT;
			Upgrade_attr_type_e ReserveNvm;

			ReserveBOOT = Upgrade_attr_type_FALSE;
			ReserveNvm = Upgrade_attr_type_FALSE;
			Mid_CmnUpgradeSetAttri(ReserveBOOT, ReserveNvm, Upgrade_attr_type_MAX);

			if (MAINAPP_GetActiveSystemAppIndex(&dActiveSysApp) == MAIN_APP_SUCCESS)
			{
				printf("dActiveSysApp        is %d\n",dActiveSysApp);
				dMessage = APP_GLOBAL_EVENT_USB_UPGRADE | PASS_TO_SYSAPP;
				SYSAPP_IF_SendGlobalEventWithIndex(dActiveSysApp, dMessage, 1);
			}
		}
	}
}

//ID_FM_Erase_HDCP
void Factory_Erase_HDCP(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		int ret = MID_EraseHDCPKey();

		if(ret == MID_DTV_SUCCESS)
		{
			FuncData->value = 1;
		}
		else
		{
			FuncData->value = 2;
		}
	}
	else
	{
		FuncData->value = 0;
	}
}


//ID_FM_UpdateHDCPUpdate,					//84
void Factory_UpdateHDCPUpdate(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		if (APP_Data_HDCP_Upgrade_Get_ValidPath() == AL_SUCCESS)
		{
			AL_Return_t ret = AL_FAILURE;
			ret = APP_Data_HDCP_Upgrade_Start();
			//DEBF("FM_UPDATE_HDCP_UPDATE Get ValidPath, ret=%d\e[0m\n", ret);

			if(ret == AL_SUCCESS)
			{
				FuncData->value = 1;
			}
			else
			{
				FuncData->value = 2;
			}
		}
	}
	else
	{
		FuncData->value = 0;
	}
}

//ID_FM_Erase_CI_Key
void Factory_Erase_CI_Key(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		//int ret = MID_EraseCIKey();
		if(0)//ret == MID_DTV_SUCCESS)
		{
			FuncData->value = 1;
		}
		else
		{
			FuncData->value = 2;
		}
	}
	else
	{
		FuncData->value = 0;
	}
}


//ID_FM_UpdateCIPlusKeyImport,				//85
void Factory_UpdateCIPlusKeyImport(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
#if (defined(CONFIG_SUPPORT_USB_AUTO_UPGRADE) && defined(CONFIG_CIPLUS_SUPPORT))

		if (APP_Data_CIKey_Upgrade_Get_ValidPath() == AL_SUCCESS)
		{
			AL_Return_t ret = AL_FAILURE;
			
			if(FALSE == MID_DTVCI_IsValidKey())
			{
				ret = APP_Data_CIKey_Upgrade_Start();
				//DEBF("FM_UPDATE_CI_PLUS_KEY_IMPORT Get ValidPath, ret=%d\e[0m\n", ret);

				if(ret == AL_SUCCESS)
				{
					FuncData->value = 1;
				}
				else
				{
					FuncData->value = 2;
				}
			}
			else
			{
				FuncData->value = 2;
			}
		}
#endif
	}
	else
	{
		FuncData->value = 0;
	}


}
	//ID_FM_Update6M20Update,					//86
void Factory_Update6M20Update(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		if (APP_Data_HDCP_Upgrade_Get_ValidPath() == AL_SUCCESS)
		{
			AL_Return_t ret = AL_FAILURE;
			ret = APP_Data_HDCP_Upgrade_Start();
			//DEBF("FM_UPDATE_HDCP_UPDATE Get ValidPath, ret=%d\e[0m\n", ret);

			if(ret == AL_SUCCESS)
			{
				FuncData->value = 1;
			}
			else
			{
				FuncData->value = 2;
			}
		}
	}
	else
	{
		FuncData->value = 0;
	}


}
/*10. Update 	end*/

/*12  start*/
//	ID_FM_FactSet_UARTDebug,			//11
void Factory_FactSet_UARTDebug(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	UINT32 mode = 0;
	if (bSet)
	{
		// UMF set function
		mode = FuncData->value;
		g_stFactoryUserData.n_FactSet_UARTDebug = mode;
		// Write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, n_FactSet_UARTDebug),
				sizeof(g_stFactoryUserData.n_FactSet_UARTDebug),
				&(g_stFactoryUserData.n_FactSet_UARTDebug));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, n_FactSet_UARTDebug),
				sizeof(g_stFactoryUserData.n_FactSet_UARTDebug));
		*/
		
		MID_TVFE_SetDebugModeOn(mode);
	}
	else
	{
		// UMF get function
		MID_TVFE_GetDebugMode(&mode);
		FuncData->value = mode;
		g_stFactoryUserData.n_FactSet_UARTDebug = mode;
	}
}

/*12  end*/
/*13. Factory Remote  	Start*/
//ID_FM_FactSet_FactoryRemote,			//12
void Factory_FactSet_FactoryRemote(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.n_FactSet_FactoryRemote= FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, n_FactSet_FactoryRemote),
				sizeof(g_stFactoryUserData.n_FactSet_FactoryRemote),
				&(g_stFactoryUserData.n_FactSet_FactoryRemote));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, n_FactSet_FactoryRemote),
				sizeof(g_stFactoryUserData.n_FactSet_FactoryRemote));
		*/
	
	}
	else
	{
		// UMF get function
		FuncData->value=g_stFactoryUserData.n_FactSet_FactoryRemote;
	}
}
/*13. Factory Remote  	End*/

//ID_FM_SysConf_6m20_TransformSupport,	//97
void Factory_6m206m30_TransformSupport(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;

	int nIndex = ID_FM_SysConf_6m20_6m206m30Option - ID_FM_SysConf_6m20_TransformSupport;
	HWND FocusItem_Handle;
	GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );

	if (bSet)
	{
		// Set function
		// Reserved for future implement...

		//Write back all data
		g_stFactoryUserData.SystemConfig.F6m206m30Option.n_SysConf_6m20_TransformSupport = FuncData->value;
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.F6m206m30Option.n_SysConf_6m20_TransformSupport),
				sizeof(g_stFactoryUserData.SystemConfig.F6m206m30Option.n_SysConf_6m20_TransformSupport),
				&(g_stFactoryUserData.SystemConfig.F6m206m30Option.n_SysConf_6m20_TransformSupport));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.F6m206m30Option.n_SysConf_6m20_TransformSupport),
				sizeof(g_stFactoryUserData.SystemConfig.F6m206m30Option.n_SysConf_6m20_TransformSupport));
		*/

		//Disable 6m20/6m30 item
		if(g_stFactoryUserData.SystemConfig.F6m206m30Option.n_SysConf_6m20_TransformSupport == FALSE)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));
		}else{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_ENABLE, &nIndex));
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETNORMAL, NULL));
		}

	}
	else
	{
		// UMF get function
		FuncData->value = g_stFactoryUserData.SystemConfig.F6m206m30Option.n_SysConf_6m20_TransformSupport;

		//Disable 6m20/6m30 item
		if(g_stFactoryUserData.SystemConfig.F6m206m30Option.n_SysConf_6m20_TransformSupport == FALSE)
		{
			GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));
			GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));
		}
	}
}

//ID_FM_SysConf_6m20_6m206m30Option,	//98
void Factory_6m206m30_Option(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	if (bSet)
	{
		// UMF set function
		g_stFactoryUserData.SystemConfig.F6m206m30Option.n_SysConf_6m20_6m206m30Option = FuncData->value;
		//[CC] write back all data
		AL_Setting_Write(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.F6m206m30Option.n_SysConf_6m20_6m206m30Option),
				sizeof(g_stFactoryUserData.SystemConfig.F6m206m30Option.n_SysConf_6m20_6m206m30Option),
				&(g_stFactoryUserData.SystemConfig.F6m206m30Option.n_SysConf_6m20_6m206m30Option));
		/*
		AL_Setting_Store(APP_Data_UserSetting_Handle(), SYS_SET_ID_FACTUSER,
			ITEM_OFFSET(APP_SETTING_FactoryUser_t, SystemConfig.F6m206m30Option.n_SysConf_6m20_6m206m30Option),
				sizeof(g_stFactoryUserData.SystemConfig.F6m206m30Option.n_SysConf_6m20_6m206m30Option));
		*/

	}
	else
	{
		// UMF get function
		FuncData->value = g_stFactoryUserData.SystemConfig.F6m206m30Option.n_SysConf_6m20_6m206m30Option;
	}
}
void Factory_ExportSWImage(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Clone_t dRet = SC_SUCCESS;

	//Disable some items
#ifndef CONFIG_DVB_SYSTEM_DVBS_SUPPORT
	int nIndex;
	int nStartIndex = ID_FM_Export_Satellite_DB - ID_FM_Export_SWImage;
	int nEndIndex = ID_FM_Import_Satellite_DB - ID_FM_Export_SWImage;
	HWND FocusItem_Handle;
	for( nIndex = nStartIndex; nIndex <= nEndIndex; nIndex++)
	{
		GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_ITEM_DISABLE, &nIndex));
		GUI_FUNC_CALL( GEL_GetHandle(pWindowControl, nIndex+nItemStart, &FocusItem_Handle) );
		GUI_FUNC_CALL(GEL_SetParam(FocusItem_Handle, PARAM_SETDISABLED, NULL));
	}
#endif

	if (bSet)
	{		// UMF set function
		dRet = APP_Clone_Backup_FlashData_To_USBBackup(BACKUP_ROMBIN_DATA);
		if(dRet == SC_SUCCESS)
		{
			FuncData->value = 1;
		}
		else
		{
			FuncData->value = 2;
		}
	}
	else
	{
		// UMF get function
		FuncData->value = 0;
	}
}
void Factory_ExportSysconfig(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Clone_t dRet = SC_SUCCESS;
	if (bSet)
	{		// UMF set function
		dRet = APP_Clone_Backup_SystemData_To_USBBackup(CV_FM_DATA);
		if(dRet == SC_SUCCESS)
		{
			FuncData->value = 1;
		}
		else
		{
			FuncData->value = 2;
		}
	}
	else
	{
		// UMF get function
		FuncData->value = 0;
	}
}
void Factory_ImportSysconfig(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Clone_t dRet = SC_SUCCESS;
	if (bSet)
	{		// UMF set function
		dRet = APP_Clone_Move_SystemData_From_USB_To_FlashBackup(CV_FM_DATA);
		if(dRet == SC_SUCCESS)
		{
			FuncData->value = 1;
		}
		else
		{
			FuncData->value = 2;
		}
	}
	else
	{
		// UMF get function
		FuncData->value = 0;
	}
}

void Factory_ImportNewLogo(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Clone_t dRet = SC_SUCCESS;
	if (bSet)
	{		// UMF set function
		Update_TypeVersion_Node("-->", (ID_FM_Import_New_Logo - ID_FM_Export_SWImage));
		dRet = APP_Clone_Revert_BinFile_to_System(UPGRADE_LOGO_ID);

		if(dRet == SC_SUCCESS)
		{
			FuncData->value = 1;
		}
		else
		{
			FuncData->value = 2;
		}
	}
	else
	{
		// UMF get function
		FuncData->value = 0;
	}
}

#ifdef SUPPORT_EXPORTIMPORT_DVBS_CVS_FILE
static Clone_t _Factory_PCChannelSortingExport(void)
{
	Clone_t result= SC_ERR_FAILURE;
	if (APP_Factory_PCSortCheckDBType() == TRUE)
	{ 
		if (SC_SUCCESS == APP_Clone_Backup_FlashData_To_USBBackup(BACKUP_DATABASE_S_DATA))
		{
			if (APP_Factory_PCSortOpenCSV(TRUE) == TRUE)
			{
				if (APP_Factory_ExportDBSWriteCSV() == TRUE)
				{
					result = SC_SUCCESS;
				}
			}	
			APP_Factory_PCSortCloseCSV();
		}
	}
	return result;
}

static Clone_t _Factory_PCChannelSortingImport(void)
{
	Clone_t result = SC_ERR_FAILURE;
	if (APP_Factory_PCSortCheckDBType() == TRUE)
	{
		AL_DB_Sync(AL_DBTYPE_DVB_S, al_true);
		if (SC_SUCCESS == APP_Clone_BinFile_From_USB_To_Flash(UPGRADE_DATABASE_S_ID, NULL))
		{
			AL_FW_ReLoadDBModule(AL_DBTYPE_DVB_S);
			if (APP_Factory_PCSortOpenCSV(FALSE) == TRUE)
			{
				if (APP_Factory_ImportDBSChangeDBbyCSV() == TRUE)
				{
					result = SC_SUCCESS;
				}
			}			
			APP_Factory_PCSortCloseCSV();
			SYSAPP_IF_SendCriticalGlobalEventWithIndex(
				SYS_APP_DVB, APP_DVB_GLOBAL_EVENT_DVB_ONRUN | PASS_TO_SYSAPP, FALSE);
		}
		AL_DB_Sync(AL_DBTYPE_DVB_S, al_false);
	}
	return result;
}
#endif
void Factory_ExportSatelliteDB(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Clone_t dRet = SC_SUCCESS;
	if (bSet)
	{		// UMF set function

		Update_TypeVersion_Node("Wait", (ID_FM_Export_Satellite_DB - ID_FM_Export_SWImage));
		GUI_FUNC_CALL(GEL_UpdateOSD());

#ifndef SUPPORT_EXPORTIMPORT_DVBS_CVS_FILE
		dRet = APP_Clone_Backup_FlashData_To_USBBackup(BACKUP_DATABASE_S_DATA);
#else
		dRet = _Factory_PCChannelSortingExport();
#endif
		if(dRet == SC_SUCCESS)
		{
			FuncData->value = 1;
		}
		else
		{
			FuncData->value = 2;
		}
	}
	else
	{
		// UMF get function
		FuncData->value = 0;
	}
}

void Factory_ImportSatelliteDB(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Clone_t dRet = SC_SUCCESS;
	if (bSet)
	{		
		// UMF set function
		Update_TypeVersion_Node("Wait", (ID_FM_Import_Satellite_DB - ID_FM_Export_SWImage));
		GUI_FUNC_CALL(GEL_UpdateOSD());

#ifndef SUPPORT_EXPORTIMPORT_DVBS_CVS_FILE
		dRet = APP_Clone_BinFile_From_USB_To_Flash(UPGRADE_DATABASE_S_ID, NULL);
#else
		dRet = _Factory_PCChannelSortingImport();
#endif
		if(dRet == SC_SUCCESS)
		{
			FuncData->value = 1;
#ifndef SUPPORT_EXPORTIMPORT_DVBS_CVS_FILE
			Update_TypeVersion_Node("OK", (ID_FM_Import_Satellite_DB - ID_FM_Export_SWImage));
			GUI_FUNC_CALL(GEL_UpdateOSD());
			// System reboot
			GL_TaskSleep(1000);
			APP_Sysset_Reset_System();
#endif
		}
		else
		{
			FuncData->value = 2;
		}
	}
	else
	{
		// UMF get function
		FuncData->value = 0;
	}
}
void APP_ImportDTVATVDB(void)
{
	AL_DB_EDBType_t eDBType = AL_DBTYPE_DVB_T;
	APP_Clone_BinFile_From_USB_To_Flash(UPGRADE_DATABASE_ATV_ID, NULL);
	APP_Clone_BinFile_From_USB_To_Flash(UPGRADE_DATABASE_T_ID, NULL);
#ifdef CONFIG_DVB_SYSTEM_DVBC_SUPPORT
	APP_Clone_BinFile_From_USB_To_Flash(UPGRADE_DATABASE_C_ID, NULL);
#endif
#ifdef CONFIG_DVB_SYSTEM_DVBS_SUPPORT
    APP_Clone_BinFile_From_USB_To_Flash(UPGRADE_DATABASE_S_ID, NULL);
#endif
	eDBType=APP_DVB_Playback_GetCurrentNetType();

	AL_FW_SwitchDBModule(eDBType);
	AL_RecHandle_t hTpHdl = AL_DB_INVALIDHDL;
	AL_DB_GetRecord(AL_DB_REQ_GETFIRST, eDBType, AL_RECTYPE_DVBTV, &hTpHdl);
	APP_DVB_Playback_SetCurrentProgHandle(eDBType, AL_RECTYPE_DVBTV, hTpHdl);
	GL_TaskSleep(1000);

#ifdef CONFIG_ATV_SUPPORT
	UINT32 u32ActiveSysAPP;
	MAINAPP_GetActiveSystemAppIndex(&u32ActiveSysAPP);
	if (u32ActiveSysAPP == SYS_APP_ATV)
	{ 
		APP_ATV_Playback_SetForcePlay();
		SYSAPP_GOBJ_SendMsgToSingleGUIObject(SYS_APP_ATV, ATV_GUIOBJ_PLAYBACK,
			APP_ATV_INTRA_EVENT_START_PLAYBACK, TRUE);

	}
#endif
#ifdef CONFIG_DTV_SUPPORT
	MAINAPP_GetActiveSystemAppIndex(&u32ActiveSysAPP);
	if (u32ActiveSysAPP == SYS_APP_DVB)
	{ 
		SYSAPP_GOBJ_SendMsgToSingleGUIObject(SYS_APP_DVB, DVB_GUIOBJ_PLAYBACK,
			APP_DVB_INTRA_EVENT_STOP_PLAYBACK, PLAYBACK_STOP_ALL);

		SYSAPP_IF_SendGlobalEventWithIndex(
			SYS_APP_DVB, APP_DVB_GLOBAL_EVENT_DVB_ONRUN | PASS_TO_SYSAPP, FALSE);
	}
#endif
}

void Factory_ExportDTVATVDB(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	Clone_t dRet = SC_SUCCESS;
	if (bSet)
	{		// UMF set function

		Update_TypeVersion_Node("Wait", (ID_FM_Export_DTV_ATV_DB - ID_FM_Export_SWImage));
		GUI_FUNC_CALL(GEL_UpdateOSD());

		dRet = APP_Clone_Backup_FlashData_To_USBBackup(BACKUP_DATABASE_ATV_DATA);
		dRet = APP_Clone_Backup_FlashData_To_USBBackup(BACKUP_DATABASE_T_DATA);
		
#ifdef CONFIG_DVB_SYSTEM_DVBC_SUPPORT
		dRet = APP_Clone_Backup_FlashData_To_USBBackup(BACKUP_DATABASE_C_DATA);
#endif
#ifdef CONFIG_DVB_SYSTEM_DVBS_SUPPORT
		dRet = APP_Clone_Backup_FlashData_To_USBBackup(BACKUP_DATABASE_S_DATA);
#endif

		if(dRet == SC_SUCCESS)
		{
			FuncData->value = 1;
		}
		else
		{
			FuncData->value = 2;
		}
	}
	else
	{
		// UMF get function
		FuncData->value = 0;
	}
}

void Factory_ImportDTVATVDB(BOOL bSet, BOOL path, FUNCTION_DATA *pValue, BOOL bDefault)
{
	FUNCTION_DATA * FuncData = (FUNCTION_DATA *)pValue;
	//Clone_t dRet = SC_SUCCESS;
	if (bSet)
	{
	
		if(1)//(dRet == SC_SUCCESS)
		{
			APP_ImportDTVATVDB();
			//FuncData->value = 1;
			Update_TypeVersion_Node("Wait", (ID_FM_Import_DTV_ATV_DB - ID_FM_Export_SWImage));
			GUI_FUNC_CALL(GEL_UpdateOSD());	
			// System reboot
			GL_TaskSleep(1000);
			APP_Sysset_Reset_System();
		}
		else
		{
			FuncData->value = 2;
		}
	}
	else
	{
		// UMF get function
		FuncData->value = 0;
	}
}

#ifdef SUPPORT_IMPORT_SATELLITE_DB_FROM_USB_IN_MAINMENU  //dchen105_2014_10 
void  _APP_GUIOBJ_MENU_ImExData_OnEvent(UINT32 iItem)
{

	Clone_t dRet = 0;
     if(iItem==0)
     	{
		dRet = APP_Clone_BinFile_From_USB_To_Flash(UPGRADE_DATABASE_S_ID, NULL);
		CDBIActiveDBModule(CDBI_DBTYPE_DVB_S, TRUE);

		if (dRet == SC_SUCCESS)
		{
			GL_TaskSleep(1000);
			APP_Sysset_Reset_System();
		}
     	}
}
#endif
static UINT32 _FM_inputsource_mapping2currSource(APP_Source_Type_t eSourceType)
{
	 typedef struct
    {
        APP_Source_Type_t eSourceType;
        UINT32 FACTORY_MODE_NODE_ID;
    }OSDItemMappingSource;

    OSDItemMappingSource Maplist[] =
    {
    	{APP_SOURCE_ATV,			ID_FM_SysConf_Input_ATV	},
    	{APP_SOURCE_DTV,			ID_FM_SysConf_Input_Source_DTV	},
//        {APP_SOURCE_RADIO,			ID_FM_SysConf_Input_CADTV		},
        {APP_SOURCE_SCART,			ID_FM_SysConf_Input_SCART		},
        {APP_SOURCE_SCART1,			ID_FM_SysConf_Input_Source_SCART2},
        {APP_SOURCE_AV,				ID_FM_SysConf_Input_AV			},
        {APP_SOURCE_AV1,			ID_FM_SysConf_Input_AV2			},
        {APP_SOURCE_AV2,			ID_FM_SysConf_Input_AV3			},
        {APP_SOURCE_SVIDEO,			ID_FM_SysConf_Input_SVideo		},
        {APP_SOURCE_SVIDEO1,		ID_FM_SysConf_Input_SVideo2		},
        {APP_SOURCE_SVIDEO2,		ID_FM_SysConf_Input_SVideo3		},
        {APP_SOURCE_YPBPR,			ID_FM_SysConf_Input_YPbPr		},
        {APP_SOURCE_YPBPR1,			ID_FM_SysConf_Input_YPbPr2		},
        {APP_SOURCE_YPBPR2,			ID_FM_SysConf_Input_YPbPr3		},
        {APP_SOURCE_HDMI,			ID_FM_SysConf_Input_Source_HDMI	},
        {APP_SOURCE_HDMI1,			ID_FM_SysConf_Input_Source_HDMI2},
		{APP_SOURCE_HDMI2,			ID_FM_SysConf_Input_Source_HDMI3},
        {APP_SOURCE_PC,				ID_FM_SysConf_Input_Source_PC	},
        {APP_SOURCE_MEDIA,			ID_FM_SysConf_Input_Media		},
        {APP_SOURCE_DVD,			ID_FM_SysConf_Input_DVD			},
        {APP_SOURCE_ANDRO,			ID_FM_SysConf_Input_Android		}
	};

    UINT32 i = 0;

    for(i=0; i<(sizeof(Maplist)/sizeof(Maplist[0])); i++)
	{
		if (eSourceType == Maplist[i].eSourceType)
		{
			return Maplist[i].FACTORY_MODE_NODE_ID;
		}
	}
	return 0;
}

static UINT32 _FM_OSDLanguage_mapping2currLanguage(APP_OSDLanguage_t eLanguageType)
{
	 typedef struct
    {
        APP_OSDLanguage_t eLanguageType;
        UINT32 FACTORY_MODE_NODE_ID;
    }OSDItemMappingLanguage;

    OSDItemMappingLanguage Maplist[] =
    {
    	{APP_OSDLANG_GERMAN,			ID_FM_SysConf_OSD_German	},
    	{APP_OSDLANG_ENGLISH,			ID_FM_SysConf_OSD_English	},
        {APP_OSDLANG_FRENCH,			ID_FM_SysConf_OSD_French	},
        {APP_OSDLANG_ITALIAN,			ID_FM_SysConf_OSD_Italian	},
        {APP_OSDLANG_POLISH,			ID_FM_SysConf_OSD_Polish	},
        {APP_OSDLANG_SPAIN,				ID_FM_SysConf_OSD_Spanish	},
        {APP_OSDLANG_NEDERLANDS,		ID_FM_SysConf_OSD_Netherlands},
        {APP_OSDLANG_PORTUGUESE,		ID_FM_SysConf_OSD_Portuguese},
        {APP_OSDLANG_SWEDISH,			ID_FM_SysConf_OSD_Swedish	},
        {APP_OSDLANG_SUOMI,				ID_FM_SysConf_OSD_Finnish	},
        {APP_OSDLANG_GREEK,				ID_FM_SysConf_OSD_Greek		},
        {APP_OSDLANG_RUSSIA,			ID_FM_SysConf_OSD_Russian	},
        {APP_OSDLANG_TURKISH,			ID_FM_SysConf_OSD_Turkey	},
        {APP_OSDLANG_DANISH,			ID_FM_SysConf_OSD_Danish	},
        {APP_OSDLANG_NORWEGIAN,			ID_FM_SysConf_OSD_Norwegian },
		{APP_OSDLANG_HUNGARIAN,			ID_FM_SysConf_OSD_Hungarian },
        {APP_OSDLANG_CZECH,				ID_FM_SysConf_OSD_Czech	    },
        {APP_OSDLANG_SLOVAKIAN,			ID_FM_SysConf_OSD_Slovakian	},
        {APP_OSDLANG_CROATIAN,			ID_FM_SysConf_OSD_Croatian	},
        {APP_OSDLANG_SERBIAN,			ID_FM_SysConf_OSD_Serbian	},
		{APP_OSDLANG_ARABIC,			ID_FM_SysConf_OSD_Arabic 	},
		{APP_OSDLANG_PERSIAN,			ID_FM_SysConf_OSD_Persian 	}
	};

    UINT32 i = 0;

    for(i=0; i<(sizeof(Maplist)/sizeof(Maplist[0])); i++)
	{
		if (eLanguageType == Maplist[i].eLanguageType)
		{
			return Maplist[i].FACTORY_MODE_NODE_ID;
		}
	}
	return 0;
}

/*****************************************************************************
** FUNCTION : Common_GUIOBJ_FM_FactorySetting_OnEvent
**
** DESCRIPTION :
**				Factory Gui Object dispose Event
**
** PARAMETERS :
**				dEventID   - Event ID
**				dParam   - The Param of the Event
**				pPrivateData   - User Data
**				pPostEventData   -
**
** RETURN VALUES:
**				SP_SUCCESS
*****************************************************************************/
static INT32 _APP_GUIOBJ_FM_FactorySetting_OnEvent(
	UINT32 dEventID, UINT32 dParam, void* pPrivateData,
	InteractiveData_t *pPostEventData)
{
	DEBF("[Common fm factory guiobj] %s is called.\n", __FUNCTION__);
	UINT32 u32CurrentIndex = 0;
	GUIResult_e dRet = GUI_SUCCESS;
#ifndef SUPPORT_FACTORY_BURNING_ADJ_TEST
#if (SYSCONF_BOARD_POWER == 0)
    Backlight_t BacklightSetting;
#endif
#endif
#ifdef CONFIG_CONFIG_CONFIG_CONFIG_SUPPORT_USB_UPGRADE
	UINT32 u32CurrentAppIndex = 0;
#endif
	APP_Source_Type_t eSourceType = APP_SOURCE_MAX;

	APP_GUIOBJ_Source_GetCurrSource(&eSourceType);

	if(dEventID&PASS_TO_SYSAPP)
	{
		return GUI_OBJECT_EVENT_BYPASS;
	}

	GUI_FUNC_CALL(GEL_GetParam(g_fmFactorySetting_data, PARAM_CURRENT_INDEX,
		&u32CurrentIndex));
#ifdef CONFIG_SUPPORT_PVR
	if(b_Isrecording && APP_GUIOBJ_DVB_PvrRec_TSRecGetState()!= TSRecording) //タ魁 τ魁篈砆い耞杠
	{
	      UpdateNodeFunctionContent(FactoryNode[nowNodeHeadId].childID,u32CurrentIndex,UI_EVENT_LEFT); //讽Ωオ龄ㄓ沧ゎ穦э跑flag のΩstop
	      dRet = GEL_ShowMenu(pWindow);
	      GUI_FUNC_CALL(GEL_UpdateOSD());
	}
	if(b_Isrecording && (dEventID != UI_EVENT_LEFT)) //special case for recording ... no action for any event
	{
		return GUI_OBJECT_NO_POST_ACTION;
	}
#endif

	if(dEventID == UI_EVENT_UP || dEventID == UI_EVENT_DOWN)
	{
		if((nowNodeHeadId == ID_FM_SysConf_InputSourceSystem || nowNodeHeadId == ID_FM_SysConf_Input_PreviousPage)
			&& (nowNodeId == ID_FM_SysConf_Input_InputSourceInput || nowNodeId == ID_FM_SysConf_Input_NextPage))
		{
			UpdateNodeFunctionContent(ID_FM_SysConf_Input_InputSourceInput,0,0);
		}
		
		if(nowNodeHeadId == ID_FM_SysConf_PhysicalInputSetting &&
		   nowNodeId != ID_FM_SysConf_Phy_ResetDefault)
		{
			UpdateNodeFunctionContent(ID_FM_SysConf_Phy_ResetDefault,0,0);
		}
		
		if((nowNodeHeadId == ID_FM_SysConf_OSDLanguage || nowNodeHeadId == ID_FM_SysConf_OSD_Previouspage)
			&& nowNodeId != ID_FM_SysConf_OSD_ResetDefault)
		{
			UpdateNodeFunctionContent(ID_FM_SysConf_OSD_ResetDefault,0,0);
		}
	}
	
	switch (dEventID)
	{
#ifdef CONFIG_MEDIA_SUPPORT
		case FILE_GLOBAL_EVENT_OBJECT_CLOSE...FILE_GLOBAL_EVENT_MEDIA_MAX:
		case DMN_EVENT_SCREEN_SAVER_ON ... DMN_EVENT_MAINAPP_STARTS:
		case DMN_EVENT_DEVICE_DETECT_MIN ... DMN_EVENT_DEVICE_DETECT_MAX:
			return GUI_OBJECT_EVENT_BYPASS;
#endif

		case UI_EVENT_UP:
			if(b_IsInputNum)
			{
				b_IsInputNum = FALSE;
				nInputValue = -1;
			}
			if (nowNodeHeadId == ID_FM_SysConf_InputSourceSystem || nowNodeHeadId == ID_FM_SysConf_Input_NextPage)
			{
				GUI_FUNC_CALL(GEL_SendMsg(g_fmFactorySetting_data, WM_SELECT_PREV, 0));
				GUI_FUNC_CALL(GEL_GetParam(g_fmFactorySetting_data, PARAM_CURRENT_INDEX,
					&u32CurrentIndex));
				nowNodeId = FactoryNode[nowNodeHeadId].childID + u32CurrentIndex;
			}
			else
			{
				GUI_FUNC_CALL(GEL_SendMsg(g_fmFactorySetting_data, WM_SELECT_PREV, 0));
			}
			_APP_GUIOBJ_FM_FactorySetting_UpdateMenu();
			break;
		case UI_EVENT_DOWN:
			if(b_IsInputNum)
			{
				b_IsInputNum = FALSE;
				nInputValue = -1;
			}
			if (nowNodeHeadId == ID_FM_SysConf_InputSourceSystem || nowNodeHeadId == ID_FM_SysConf_Input_NextPage)
			{
				GUI_FUNC_CALL(GEL_SendMsg(g_fmFactorySetting_data, WM_SELECT_NEXT, 0));
				GUI_FUNC_CALL(GEL_GetParam(g_fmFactorySetting_data, PARAM_CURRENT_INDEX,
					&u32CurrentIndex));
				nowNodeId = FactoryNode[nowNodeHeadId].childID + u32CurrentIndex;
			}
			else
			{
				GUI_FUNC_CALL(GEL_SendMsg(g_fmFactorySetting_data, WM_SELECT_NEXT, 0));
			}
			_APP_GUIOBJ_FM_FactorySetting_UpdateMenu();
			break;
		case UI_EVENT_RIGHT:
			if(nowNodeHeadId == ID_FM_SysConf_OSDLanguage || nowNodeHeadId == ID_FM_SysConf_OSD_Nextpage || nowNodeHeadId == ID_FM_SysConf_OSD_Previouspage)
			{
				nowNodeId = FactoryNode[nowNodeHeadId].childID + u32CurrentIndex;
				if(Factory_OSDLanguage_ISUpdate() == TRUE)
				{
					UpdateNodeFunctionContent(FactoryNode[nowNodeHeadId].childID,u32CurrentIndex,UI_EVENT_RIGHT);
				}
				_APP_GUIOBJ_FM_FactorySetting_UpdateMenu();
			}
			else
			{
				if (FactoryNode[nowNodeHeadId].childID + u32CurrentIndex  == ID_FM_SysConf_Input_NextPage
					|| FactoryNode[nowNodeHeadId].childID + u32CurrentIndex  == ID_FM_SysConf_Input_PreviousPage)
				{
					_FM_InputSource_SaveValue();
				}
			//#ifdef CONFIG_SUPPORT_ADJUST_PWM
				if (FactoryNode[nowNodeHeadId].childID + u32CurrentIndex  == ID_FM_Funct_PanelSetting)
				{
					MID_TVFE_GetCurrDutyPWM((short unsigned int *)&(u16DutyPWM));
					MID_TVFE_GetCurrPolarity((unsigned char *)&(u8BackLight_Polarity));
					//u8BackLight = AL_FLASH_GetBackLight();
					#if (HW_ADJ_INVERT == 1)
					#ifdef CONFIG_SUPPORT_PWM_DUTY_IN_DECIMAL
					if(u8BackLight_Polarity == 1)
						u16DutyPWM = 1000 - u16DutyPWM;
					#else
					if(u8BackLight_Polarity == 1)
						u16DutyPWM = 100 - u16DutyPWM;
					#endif
					#endif

				}
			//#endif
				if (FactoryNode[nowNodeHeadId].childID + u32CurrentIndex  == ID_FM_FactSet_Audio)
				{
					APP_GUIOBJ_Source_GetCurrSource(&eSourceType);	  //channel match
					u8AudioOutOffset = APP_GUIOBJ_Source_GetAudioOutOffsetValue(eSourceType);				
				}
				UpdateNodeFunctionContent(FactoryNode[nowNodeHeadId].childID, u32CurrentIndex, UI_EVENT_RIGHT);
				_APP_GUIOBJ_FM_FactorySetting_UpdateMenu();
			}
			if(nowNodeId == ID_FM_FactSet_BurningMode)
			{
				if(g_stFactoryUserData.n_FactSet_BurningMode == 1)
				{   
					BurnInModeTaskExistFlag = 0;
					pPostEventData->dEventID = GUI_OBJECT_CLOSE;
					OSD_SetDisplay(FALSE);
					MID_TVFE_SetBurnIn(1, 0, 1);
					return GUI_OBJECT_POST_EVENT;
				}
			}
			break;
		case UI_EVENT_LEFT:
			{
				if(nowNodeHeadId == ID_FM_FactSet_ImportExportData)
				{
					break;
				}
#ifdef CONFIG_SUPPORT_PVR
				if(b_Isrecording)
				{
					APP_GUIOBJ_DVB_PvrRec_TSRecStop();
					b_Isrecording=FALSE;
				}
#endif
				if(nowNodeHeadId == ID_FM_SysConf_OSDLanguage || nowNodeHeadId == ID_FM_SysConf_OSD_Nextpage)
				{
					nowNodeId = FactoryNode[nowNodeHeadId].childID + u32CurrentIndex;
					if(Factory_OSDLanguage_ISUpdate() == TRUE)
					{
						UpdateNodeFunctionContent(FactoryNode[nowNodeHeadId].childID,u32CurrentIndex,UI_EVENT_LEFT);
						dRet = GEL_ShowMenu(pWindow);
						GUI_FUNC_CALL(GEL_UpdateOSD());
					}
				}
				else
				{
					UpdateNodeFunctionContent(FactoryNode[nowNodeHeadId].childID,u32CurrentIndex,UI_EVENT_LEFT);
					_APP_GUIOBJ_FM_FactorySetting_UpdateMenu();
				}
				if(nowNodeId == ID_FM_FactSet_BurningMode)
				{
					if(g_stFactoryUserData.n_FactSet_BurningMode == 1)
					{
						pPostEventData->dEventID = GUI_OBJECT_CLOSE;
						OSD_SetDisplay(FALSE);
#ifndef SUPPORT_FACTORY_BURNING_ADJ_TEST
#if (SYSCONF_BOARD_POWER == 0)
						BacklightSetting.Backlight_total_Stage = 100; // set total backlight stage = 100
						BacklightSetting.OSD_backlight_index = 100; 
						Cmd_SetLcdBackLight(BacklightSetting); // when into  BurnMode BackLight must be max
#endif
#endif
						MID_TVFE_SetBurnIn(1, 0, 1);
						return GUI_OBJECT_POST_EVENT;
					}
				}
				break;
			}
		case UI_EVENT_ENTER:
			if (FactoryNode[nowNodeHeadId].childID + u32CurrentIndex  == ID_FM_Funct_PanelSetting)
			{
				MID_TVFE_GetCurrDutyPWM((short unsigned int *)&(u16DutyPWM));
				MID_TVFE_GetCurrPolarity((unsigned char *)&(u8BackLight_Polarity));
				#if (HW_ADJ_INVERT == 1)
				#ifdef CONFIG_SUPPORT_PWM_DUTY_IN_DECIMAL
				if(u8BackLight_Polarity == 1)
					u16DutyPWM = 1000 - u16DutyPWM;
				#else
				if(u8BackLight_Polarity == 1)
					u16DutyPWM = 100 - u16DutyPWM;
				#endif
				#endif
				//u8BackLight = AL_FLASH_GetBackLight();
			}
			if (FactoryNode[nowNodeHeadId].childID + u32CurrentIndex  == ID_FM_FactSet_Audio)
			{
				APP_GUIOBJ_Source_GetCurrSource(&eSourceType);	  //channel match
				u8AudioOutOffset = APP_GUIOBJ_Source_GetAudioOutOffsetValue(eSourceType);				
			}
			//[CC] Added, do as above codes
			dRet = Enter_Behavior();

			_APP_GUIOBJ_FM_FactorySetting_UpdateMenu();

			break;
		case UI_EVENT_RETURN:
		case UI_EVENT_MENU:
			if(!g_bOpenVersionDir)
			{
				if(b_IsInputNum)
				{
					b_IsInputNum = FALSE;
					nInputValue = -1;
				}

				if(nowNodeHeadId!=ID_ROOT)
				{
					if(nowNodeHeadId == ID_FM_Funct_PanelSetting)
					{
						if(g_bFlipIsChange == TRUE)
						{
							//SYSAPP_IF_SendGlobalEventWithIndex(u_CurrentAppIndex, APP_GLOBAL_EVENT_FLIP|PASS_TO_SYSAPP,0);
							g_bFlipIsChange = FALSE;
							break;
						}
					}
					else if(nowNodeHeadId == ID_FM_SysConf_InputSourceSystem || nowNodeHeadId == ID_FM_SysConf_Input_NextPage || nowNodeHeadId == ID_FM_SysConf_Input_PreviousPage
						|| (nowNodeHeadId >= ID_FM_SysConf_Input_ATV && nowNodeHeadId <= ID_FM_SysConf_Input_Source_HDMI2)
						|| (nowNodeHeadId >= ID_FM_SysConf_Input_Source_HDMI3 && nowNodeHeadId <= ID_FM_SysConf_Input_Android))
				    {
				    	if(nowNodeHeadId == ID_FM_SysConf_Input_PreviousPage
							|| (nowNodeHeadId >= ID_FM_SysConf_Input_ATV && nowNodeHeadId <= ID_FM_SysConf_Input_Source_HDMI2))
				    	{
				    		nowNodeHeadId = ID_FM_SysConf_InputSourceSystem;
				    	}
						_FM_InputSource_SaveValue();
				    }
				    else if(nowNodeHeadId == ID_FM_SysConf_OSDLanguage || nowNodeHeadId == ID_FM_SysConf_OSD_Nextpage || nowNodeHeadId == ID_FM_SysConf_OSD_Previouspage
						|| (nowNodeHeadId >= ID_FM_SysConf_OSD_German && nowNodeHeadId <= ID_FM_SysConf_OSD_Greek)
						|| (nowNodeHeadId >= ID_FM_SysConf_OSD_Russian && nowNodeHeadId <= ID_FM_SysConf_OSD_Persian))
				    {
				    	if(nowNodeHeadId == ID_FM_SysConf_OSD_Previouspage
							|| (nowNodeHeadId >= ID_FM_SysConf_OSD_German && nowNodeHeadId <= ID_FM_SysConf_OSD_Greek))
						{
							nowNodeHeadId = ID_FM_SysConf_OSDLanguage;
						}
				    }
					int nChildID = FactoryNode[FactoryNode[nowNodeHeadId].parentID].childID;
					u32CurrentIndex=0;
					for( ; ; nChildID = FactoryNode[nChildID].nextID)
					{
						u32CurrentIndex++;
						if(FactoryNode[nChildID].nextID == FactoryNode[nowNodeHeadId].nextID)
						{
							// Find the node
							break;
						}

						if(FactoryNode[nChildID].nextID == 0)
						{
							// Reached the end of current layer, and not found the node
							u32CurrentIndex = 0;
							break;
						}
					}

					if(u32CurrentIndex != 0)
						u32CurrentIndex--;

					nowNodeHeadId = FactoryNode[nowNodeHeadId].parentID;

					dRet = _APP_Update_Layer(nowNodeHeadId);
					GUI_FUNC_CALL(GEL_SetParam(g_fmFactorySetting_data, PARAM_CURRENT_INDEX,&u32CurrentIndex));
					_APP_GUIOBJ_FM_FactorySetting_UpdateMenu();

					return GUI_OBJECT_NO_POST_ACTION;
				}
				else
				{
					return GUI_OBJECT_EVENT_BYPASS;
				}
				break;
			}
			else
			{
				return GUI_OBJECT_EVENT_BYPASS;
			}
		case UI_EVENT_MultiPanelIndex:
			if(!g_bOpenVersionDir)
			{
				break;
			}
		case UI_EVENT_FAC_RESET:
			return GUI_OBJECT_EVENT_BYPASS;
			break;
		#ifdef CONFIG_CTV_UART_FAC_MODE			
		case UI_EVENT_FAC_SHIPPING_MODE:
			return GUI_OBJECT_EVENT_BYPASS;
			break;
		#endif	
		case UI_EVENT_EXIT:
			// Save value, if need
			if(nowNodeHeadId == ID_FM_Funct_PanelSetting)
			{
				if(g_bFlipIsChange == TRUE)
				{
					//SYSAPP_IF_SendGlobalEventWithIndex(u_CurrentAppIndex, APP_GLOBAL_EVENT_FLIP|PASS_TO_SYSAPP,0);
					g_bFlipIsChange = FALSE;
					break;
				}
			}
			else if(nowNodeHeadId == ID_FM_SysConf_InputSourceSystem || nowNodeHeadId == ID_FM_SysConf_Input_NextPage
				||(nowNodeHeadId >= ID_FM_SysConf_Input_ATV && nowNodeHeadId <= ID_FM_SysConf_Input_Android))
		    {
				_FM_InputSource_SaveValue();
		    }
			return GUI_OBJECT_EVENT_BYPASS;
			break;
		case UI_EVENT_SOURCE:
			if(nowNodeHeadId == ID_FM_Funct_PanelSetting)
			{
				if(g_bFlipIsChange == TRUE)
				{
					//SYSAPP_IF_SendGlobalEventWithIndex(u_CurrentAppIndex, APP_GLOBAL_EVENT_FLIP|PASS_TO_SYSAPP,0);
					g_bFlipIsChange = FALSE;
					break;
				}
			}
			return GUI_OBJECT_EVENT_BYPASS;
#ifdef SUPPORT_CEC_TV
		case DMN_EVENT_CECTV_SET_SYSTEMAUDIO_STATUS:
		case DMN_EVENT_CECTV_SET_MUTE_STATUS:
		case DMN_EVENT_CECTV_SET_DEVICE_OSDNAME:
			return GUI_OBJECT_EVENT_BYPASS;
#endif
		//[CC] Added for Number key
		case UI_EVENT_0:
		case UI_EVENT_1:
		case UI_EVENT_2:
		case UI_EVENT_3:
		case UI_EVENT_4:
		case UI_EVENT_5:
		case UI_EVENT_6:
		case UI_EVENT_7:
		case UI_EVENT_8:
		case UI_EVENT_9:
			nowNodeId = FactoryNode[nowNodeHeadId].childID + u32CurrentIndex;
			if((nowNodeId >= ID_FM_PicMode_WB_RGain && nowNodeId <= ID_FM_PicMode_WB_GammaTable)
				||(nowNodeId >= ID_FM_PicMode_Pic_Brightness && nowNodeId <= ID_FM_PicMode_Pic_Tint)
				||(nowNodeId >= ID_FM_PicMode_OverScan_VTopOverScan && nowNodeId <= ID_FM_PicMode_OverScan_ExtSuouceVstart)
				||(nowNodeId >= ID_FM_PicMode_Curve_CurvePoint0 && nowNodeId <= ID_FM_PicMode_Curve_CurvePoint100)
				||(nowNodeId >= ID_FM_PicMode_DynCon_ContrastLevel && nowNodeId <= ID_FM_PicMode_DynCon_Alphamode4)
				||(nowNodeId >= ID_FM_PicMode_CoLUT_RotGain && nowNodeId <= ID_FM_PicMode_CoLUT_LumaGain)
				||(nowNodeId == ID_FM_PicMode_Backlight))
			{
				if(b_IsInputNum)
				{
					nInputValue = nInputValue*10 + (dEventID - UI_EVENT_0);
				}
				else
				{
					b_IsInputNum = TRUE;
					nInputValue = (dEventID - UI_EVENT_0);
				}
				UpdateNodeFunctionContent(FactoryNode[nowNodeHeadId].childID,u32CurrentIndex,UI_EVENT_ENTER);
				_APP_GUIOBJ_FM_FactorySetting_UpdateMenu();
			}
			break;
		default:
			break;
	}
	return (INT32)dRet;//SP_SUCCESS;
}

/*****************************************************************************
** $Rev: 1069 $
**
*****************************************************************************/
