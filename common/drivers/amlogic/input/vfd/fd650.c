//-------------------------------------------------------------------------------------------------
// Include Files
//-------------------------------------------------------------------------------------------------
#include <linux/input/vfd.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include "fd650.h"

#define VFD_DEBUG
#ifdef VFD_DEBUG
#define DBG_VFD(msg)                    msg
#else
#define DBG_VFD(msg)
#endif

void MDrv_FrontPnl_Init(void);
void MDrv_FrontPnl_Update(MS_U8 *U8str,MS_BOOL colon);
static int MDrv_FD650_Read(void);
MS_BOOL MDrv_FrontPnl_EnableLED(MS_BOOL  eEnableLED);

MS_U16 curShowLed4 = 0xffff ;
int (*vfd_set_stb_pin_value)(int value);
int (*vfd_set_clock_pin_value)(int value);
int (*vfd_set_do_pin_value)(int value);
int (*vfd_get_di_pin_value)(void);

#define CLK_DELAY_TIME       2

//-------------------------------------------------------------------------------------------------
// Local Variables
//-------------------------------------------------------------------------------------------------



void Delay(int iCount)
{
	udelay(iCount);
}

#define   DELAY1    Delay(5)

static int fd650_init(struct vfd_platform_data *pvfd_platform_data)
{
	printk("[JD]: %s\n", __func__);
	vfd_set_stb_pin_value = pvfd_platform_data->set_stb_pin_value;
	vfd_set_clock_pin_value = pvfd_platform_data->set_clock_pin_value;
	vfd_set_do_pin_value = pvfd_platform_data->set_do_pin_value;
	vfd_get_di_pin_value = pvfd_platform_data->get_di_pin_value;

	MDrv_FrontPnl_Init();
	return 0;
}
static int get_fd650_key_value(void)
{

	printk("[JD]: %s\n", __func__);
	return MDrv_FD650_Read();
}
static int set_fd650_led_value(char *display_code)
{
	int i,j = 0;
	char data,display_char[8];
	int dot = 0;
	printk("[JD]: %s\n", __func__);
	for(i = 0; i <= 8; i++)
	{
		data = display_code[i];
		if(data == ':')	{
		  dot++;
		}
		else{
			display_char[j++] = display_code[i];
			if(data == '\0')break;
		}
	}
	DBG_VFD(printk("function[%s] line %d display string: %s .\n", __FUNCTION__,__LINE__,display_char));

	MDrv_FrontPnl_Update((MS_U8*)display_char, dot);
	return 0;
}

#ifdef CONFIG_VFD_FD650
int hardware_init(struct vfd_platform_data *pdev)
{
		int ret;
		printk("[JD]: %s\n", __func__);
		ret = fd650_init(pdev);
		return ret;
}

int get_vfd_key_value(void)
{
		int key_value;
		printk("[JD]: %s\n", __func__);
		key_value = get_fd650_key_value();
		return key_value;
}

int set_vfd_led_value(char *display_code)
{
		int ret;
		printk("[JD]: %s=%s\n", __func__,display_code);
		ret = set_fd650_led_value(display_code);
		return ret;
}
#endif

static MS_U8 MDrv_Led_Get_Code(MS_U8 cTemp)
{
    MS_U8 i, bitmap = 0x00;

	printk("[JD]: %s\n", __func__);
    for(i = 0; i < LEDMAPNUM; i++)
    {
        if(bcd_decode_tab[i].character == cTemp)
        {
                 bitmap = bcd_decode_tab[i].bitmap;
             break;
         }
    }
    return bitmap;
}

// FD650 inner function
static void MDrv_FD650_Start( void )
{
    FD650_SDA_SET;
    FD650_SDA_D_OUT;
    FD650_SCL_SET;
    FD650_SCL_D_OUT;
	DELAY1;
	FD650_SDA_CLR;
    DELAY1;
    FD650_SCL_CLR;
}

static void MDrv_FD650_Stop( void )
{
    FD650_SDA_CLR;
    FD650_SDA_D_OUT;
    DELAY1;
    FD650_SCL_SET;
    DELAY1;
    FD650_SDA_SET;
    DELAY1;
    FD650_SDA_D_IN;
}

static void MDrv_FD650_WrByte( MS_U8 dat )
{
    MS_U8 i;
    FD650_SDA_D_OUT;
    for( i = 0; i != 8; i++ )
    {
        if( dat & 0x80 )
        {
            FD650_SDA_SET;
        }
        else
        {
            FD650_SDA_CLR;
        }
        DELAY1;
        FD650_SCL_SET;
        dat <<= 1;
        DELAY1;  // choose delay
        FD650_SCL_CLR;
    }
    FD650_SDA_D_IN;
    FD650_SDA_SET;
    DELAY1;
    FD650_SCL_SET;
    DELAY1;
    FD650_SCL_CLR;
}

static int MDrv_FD650_RdByte( void )
{
    int dat,i;
    FD650_SDA_SET;
    FD650_SDA_D_IN;
    dat = 0;
    for( i = 0; i != 8; i++ )
    {
        DELAY1;  //choose delay
        FD650_SCL_SET;
        DELAY1;  // choose delay
        dat = dat << 1;
        if( FD650_SDA_IN )dat++;
		DELAY1;
		FD650_SCL_CLR;
    }

    FD650_SDA_SET;
    DELAY1;
    FD650_SCL_SET;
    DELAY1;
    FD650_SCL_CLR;
    return dat;
}

static void MDrv_FD650_Write( MS_U16 cmd ) //write cmd
{
	printk("[JD]: %s\n", __func__);
    MDrv_FD650_Start();
    MDrv_FD650_WrByte(((MS_U8)(cmd>>7)&0x3E)|0x40);
    MDrv_FD650_WrByte((MS_U8)cmd);
    Delay(CLK_DELAY_TIME);
    MDrv_FD650_Stop();
    return;
}

static int MDrv_FD650_Read( void )
{
    int keycode = 0;
    MS_U32 value=0;
	printk("[JD]: %s\n", __func__);
    MDrv_FD650_Start();
    value = ((FD650_GET_KEY>>7)&0x3E)|0x01|0x40 ;
    MDrv_FD650_WrByte((MS_U8)value);
    keycode=MDrv_FD650_RdByte();
	Delay(CLK_DELAY_TIME);
    MDrv_FD650_Stop();

    return keycode;
}
MS_U8 led_data4 = 0;
MS_BOOL lock;
void Led_Show_lockflg(bool lockflg)
{
	lock = lockflg;
	led_data4 = 0;
    printk("[JD]: %s=%d,%x\n", __func__,lockflg,curShowLed4);
	if(lockflg)
	{
		led_data4 |= FD650_DOT ;
	}
	else
	{
		led_data4 &= 0x7f ;
	}

	if(curShowLed4 != 0xffff)
	{
		MDrv_FD650_Write(curShowLed4 | led_data4);
	}
}

// FD650 public
void MDrv_FD650_Init(void)
{
      printk("[JD]: %s\n", __func__);
      MDrv_FrontPnl_EnableLED(1);
      MDrv_FD650_Write(FD650_SYSON_8);
}

void MDrv_FrontPnl_Update(MS_U8 *u8str, MS_BOOL Colon_flag)
{
    int i;
    MS_U8 data[4] = {0x00, 0x00, 0x00, 0x00};

    if(!u8str)
    {
        return;
    }

    for(i = 0; i < FP_LED_MAX_NUM; i++)
    {
         data[i] = MDrv_Led_Get_Code(u8str[i]);
    }

	MDrv_FD650_Write(FD650_SYSON_8);// 开启显示和键盘，8段显示方式

	//发显示数据
	MDrv_FD650_Write( FD650_DIG0 | data[0] ); //点亮第一个数码管

	if(Colon_flag)
		MDrv_FD650_Write( FD650_DIG1 | data[1] | FD650_DOT ); //点亮第二个数码管
	else
	MDrv_FD650_Write( FD650_DIG1 | data[1] );

	MDrv_FD650_Write( FD650_DIG2 | data[2]); //点亮第三个数码管
	MDrv_FD650_Write( FD650_DIG3 | data[3] | led_data4); //点亮第四个数码管
	curShowLed4 = FD650_DIG3 | data[3] ;
	//MDrv_FD650_Write(FD650_DIG3 | led_data4);
}

MS_BOOL MDrv_FrontPnl_EnableLED(MS_BOOL  eEnableLED)
{
	return 0;

//	printk("[JD]: %s\n", __func__);
    if(eEnableLED)
    {
         FD650_SCL_SET
    }
    else
    {
         FD650_SCL_CLR
    }
    return true;
}

void MDrv_FrontPnl_Init(void)
{
	printk("[JD]: %s\n", __func__);
	MDrv_FD650_Init();
	MDrv_FrontPnl_Update((MS_U8*)"boot", 0);
}

void MDrv_FrontPnl_Suspend(void)
{
#if 1
	MDrv_FD650_Write(FD650_SYSOFF);
#else
	MDrv_FrontPnl_Update((MS_U8*)" OFF", 0);
#endif
}

