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
// ����ϵͳ��������

#define FD650_BIT_ENABLE  0x01    // ����/�ر�λ
#define FD650_BIT_SLEEP   0x04    // ˯�߿���λ
#define FD650_BIT_7SEG    0x08    // 7�ο���λ
#define FD650_BIT_INTENS1 0x10    // 1������
#define FD650_BIT_INTENS2 0x20    // 2������
#define FD650_BIT_INTENS3 0x30    // 3������
#define FD650_BIT_INTENS4 0x40    // 4������
#define FD650_BIT_INTENS5 0x50    // 5������
#define FD650_BIT_INTENS6 0x60    // 6������
#define FD650_BIT_INTENS7 0x70    // 7������
#define FD650_BIT_INTENS8 0x00    // 8������

#define FD650_SYSOFF  0x0400      // �ر���ʾ���رռ���
#define FD650_SYSON ( FD650_SYSOFF | FD650_BIT_ENABLE ) // ������ʾ������
#define FD650_SLEEPOFF  FD650_SYSOFF  // �ر�˯��
#define FD650_SLEEPON ( FD650_SYSOFF | FD650_BIT_SLEEP )  // ����˯��
#define FD650_7SEG_ON ( FD650_SYSON | FD650_BIT_7SEG )  // �����߶�ģʽ
#define FD650_8SEG_ON ( FD650_SYSON | 0x00 )  // �����˶�ģʽ
#define FD650_SYSON_1 ( FD650_SYSON | FD650_BIT_INTENS1 ) // ������ʾ�����̡�1������
//�Դ�����
#define FD650_SYSON_4 ( FD650_SYSON | FD650_BIT_INTENS4 ) // ������ʾ�����̡�4������
//�Դ�����
#define FD650_SYSON_8 ( FD650_SYSON | FD650_BIT_INTENS8 ) // ������ʾ�����̡�8������


// ��������������
#define FD650_DIG0    0x1400      // �����λ0��ʾ,�����8λ����
#define FD650_DIG1    0x1500      // �����λ1��ʾ,�����8λ����
#define FD650_DIG2    0x1600      // �����λ2��ʾ,�����8λ����
#define FD650_DIG3    0x1700      // �����λ3��ʾ,�����8λ����

#define FD650_DOT     0x0080      // �����С������ʾ

// ��ȡ������������
#define FD650_GET_KEY 0x0700          // ��ȡ����,���ذ�������


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

