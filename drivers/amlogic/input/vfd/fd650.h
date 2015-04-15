#ifndef __FP__FD650__H
#define __FP__FD650__H

/*** type redefine ***/

typedef unsigned char MS_U8;
typedef unsigned short MS_U16;
typedef unsigned long MS_U32;
typedef int MS_S32;
typedef bool MS_BOOL;


/****************************begin for FD650 define struct***************************** */
#define LEDMAPNUM 40

//key mapping
typedef struct
{
  MS_U8 keyMapData;
  MS_U8 keyMapLevel;

} MS_KEYMAP;

//led mapping
typedef struct _led_bitmap
{
  MS_U8 character;
  MS_U8 bitmap;
} led_bitmap;

static const led_bitmap bcd_decode_tab[LEDMAPNUM] =
{
    {'0', 0x3F}, {'1', 0x06}, {'2', 0x5B}, {'3', 0x4F},
    {'4', 0x66}, {'5', 0x6D}, {'6', 0x7D}, {'7', 0x07},
    {'8', 0x7F}, {'9', 0x6F}, {'a', 0x77}, {'A', 0x77},
    {'b', 0x7C}, {'B', 0x7C}, {'c', 0x58}, {'C', 0x39},
    {'d', 0x5E}, {'D', 0x5E}, {'e', 0x79}, {'E', 0x79},
    {'f', 0x71}, {'F', 0x71}, {'H', 0x76}, {'h', 0x74},
	{'o', 0x5C}, {'t', 0x78},
    {'l', 0x30}, {'L', 0x38}, {'N', 0x37},{'n', 0x37}, 
    {'p', 0x73},{'P', 0x73}, {'O', 0x3F}, {'u', 0x1C}, 
    {'U', 0x3E},{'S', 0x6D}, {'s', 0x6D},{'-', 0x40},
    {' ', 0x00}
};//BCD decode

/****************************end for FD650 define struct***************************** */

/* *************************************hardware related*********************************************** */
#define HIGH 1
#define LOW  0

#define FRONTPNL_START_TIME_MS      3   //((1000 / 50) / LED_NUM)
#define FRONTPNL_PERIOD_TIME_MS     150
#define FP_LED_MAX_NUM 				4
#define DBG_LED(msg)                msg

#define FrontPnl_MSECS_TICK         100 //100 msecs/tick
#define FrontPnl_TICKS_SEC          1    //10 ticks/sec
/* ********************************************************************************************* */
// 设置系统参数命令

#define FD650_BIT_ENABLE  0x01    // 开启/关闭位
#define FD650_BIT_SLEEP   0x04    // 睡眠控制位
#define FD650_BIT_7SEG    0x08    // 7段控制位
#define FD650_BIT_INTENS1 0x10    // 1级亮度
#define FD650_BIT_INTENS2 0x20    // 2级亮度
#define FD650_BIT_INTENS3 0x30    // 3级亮度
#define FD650_BIT_INTENS4 0x40    // 4级亮度
#define FD650_BIT_INTENS5 0x50    // 5级亮度
#define FD650_BIT_INTENS6 0x60    // 6级亮度
#define FD650_BIT_INTENS7 0x70    // 7级亮度
#define FD650_BIT_INTENS8 0x00    // 8级亮度

#define FD650_SYSOFF  0x0400      // 关闭显示、关闭键盘
#define FD650_SYSON ( FD650_SYSOFF | FD650_BIT_ENABLE ) // 开启显示、键盘
#define FD650_SLEEPOFF  FD650_SYSOFF  // 关闭睡眠
#define FD650_SLEEPON ( FD650_SYSOFF | FD650_BIT_SLEEP )  // 开启睡眠
#define FD650_7SEG_ON ( FD650_SYSON | FD650_BIT_7SEG )  // 开启七段模式
#define FD650_8SEG_ON ( FD650_SYSON | 0x00 )  // 开启八段模式
#define FD650_SYSON_1 ( FD650_SYSON | FD650_BIT_INTENS1 ) // 开启显示、键盘、1级亮度
//以此类推
#define FD650_SYSON_4 ( FD650_SYSON | FD650_BIT_INTENS4 ) // 开启显示、键盘、4级亮度
//以此类推
#define FD650_SYSON_8 ( FD650_SYSON | FD650_BIT_INTENS8 ) // 开启显示、键盘、8级亮度


// 加载字数据命令
#define FD650_DIG0    0x1400      // 数码管位0显示,需另加8位数据
#define FD650_DIG1    0x1500      // 数码管位1显示,需另加8位数据
#define FD650_DIG2    0x1600      // 数码管位2显示,需另加8位数据
#define FD650_DIG3    0x1700      // 数码管位3显示,需另加8位数据

#define FD650_DOT     0x0080      // 数码管小数点显示

// 读取按键代码命令
#define FD650_GET_KEY 0x0700          // 获取按键,返回按键代码


#define  FD650_SCL_SET   \
      vfd_set_clock_pin_value(1);  
#define  FD650_SCL_CLR  \
      vfd_set_clock_pin_value(0);   
	  
#define FD650_SCL_D_OUT 

#define  FD650_SDA_SET   \
      vfd_set_do_pin_value(1);
#define  FD650_SDA_CLR    \
      vfd_set_do_pin_value(0);   
	  
#define  FD650_SDA_IN    \
	vfd_get_di_pin_value()
#define  FD650_SDA_D_OUT  
#define  FD650_SDA_D_IN \  
	vfd_get_di_pin_value();
	
#endif

