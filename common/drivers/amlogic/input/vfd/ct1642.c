#include <linux/delay.h>
#include <linux/input/vfd.h>
#include <linux/delay.h>
#include <linux/kernel.h>

#define VFD_DEBUG
#ifdef VFD_DEBUG
#define DBG_VFD(msg)                    msg
#else
#define DBG_VFD(msg)                    
#endif

typedef unsigned char MS_U8;
typedef unsigned short MS_U16;
typedef unsigned long MS_U32;
typedef int MS_S32;
typedef bool MS_BOOL;
#define  CT1642_SCL_SET   \
      vfd_set_clock_pin_value(1);  
#define  CT1642_SCL_CLR  \
      vfd_set_clock_pin_value(0);   
	  
#define CT1642_SCL_D_OUT 

#define  CT1642_SDA_SET   \
      vfd_set_do_pin_value(1);
#define  CT1642_SDA_CLR    \
      vfd_set_do_pin_value(0);   
	  
#define  CT1642_KEY_IN    \
	vfd_get_di_pin_value()
#define  CT1642_SDA_D_OUT
  
#define  CT1642_SDA_D_IN \
	vfd_get_di_pin_value();	

typedef struct tagVFD
{
   char  Character;   
   unsigned char   Value;   
}VFD_CHARACTER_t;
#define a 1<<7
#define b 1<<3
#define c 1<<2
#define d 1<<4
#define e 1<<5
#define f 1<<6
#define g 1<<1
#define h 1<<0
#define NONE 0X00

#define CT1642_REMOTE_KEY_MENU 0
#define CT1642_REMOTE_KEY_ENTER 1
#define CT1642_REMOTE_KEY_UP 2
#define CT1642_REMOTE_KEY_DOWN 3
#define CT1642_REMOTE_KEY_LEFT 4
#define CT1642_REMOTE_KEY_RIGHT 5
#define CT1642_REMOTE_KEY_EXIT 6
#define CT1642_REMOTE_KEY_STANDBY 7

static VFD_CHARACTER_t VFDCharArray[] =
{
	 {'0',a|b|c|d|e|f},{'1',f|e},{'2',a|b|d|g|f},{'3',a|d|e|f|g},
	 {'4',c|e|g|f},{'5',a|e|g|c|d}, {'6',a|b|c|d|e|g},{'7',a|e|f},
	 {'8',a|b|c|d|e|f|g},{'9',a|c|e|g|f},{'A',a|b|c|e|f|g},{'B',a|b|c|d|e|f|g},
	 {'C',a|b|c|d},{'D',a|b|c|d|e|f},{'E',a|b|c|d|g},{'F',a|b|c|g},{'G',a|b|c|d|e|g},
	 {'H',b|c|e|f|g},{'I',b|c},{'J',d|e|f},{'K',NONE},{'L',b|c|d},{'M',a|b|c|e|f},{'N',a|b|c|e|f},
	 {'O',a|b|c|d|e|f},{'P',a|b|c|f|g},{'Q',NONE},{'R',a|b|c|e|f|g},{'S',a|c|d|e|g},{'T',a|b|c},
	 {'U',b|c|d|e|f},{'V',NONE},{'W',NONE},{'X',NONE},{'Y',NONE},{'Z',NONE},{'a',b|d|e|g},
	 {'b',b|c|d|e|g},{'c',a|b|c|d},{'d',b|d|e|f|g},{'e',a|b|c|d|f|g},{'f',a|b|c|g},{'g',a|c|d|e|f|g},
	 {'h',b|c|e|g},{'i',b|c},{'j',d|e|f},{'k',NONE},{'l',f|e},{'m',a|b|c|e|f},{'n',a|b|c|e|f},
	 {'o',b|d|e|g},{'p',a|b|c|f|g},{'q',NONE},{'r',NONE},{'s',a|b|d|e|g},{'t',g|b|c|d},
	 {'u',f|e|d|c|b},{'v',NONE},{'w',NONE},{'x',NONE},{'y',NONE},{'z',NONE}, {'-', g},
	 {' ', NONE},
	 {'\0', 0x00}
};


void MDrv_FrontPnl_Init(void);
void MDrv_FrontPnl_Update(char *U8str,int colon);
int MDrv_CT1642_FP_Get_Key(void);

int (*vfd_set_stb_pin_value)(int value);
int (*vfd_set_clock_pin_value)(int value);
int (*vfd_set_do_pin_value)(int value);
int (*vfd_get_di_pin_value)(void);
static void  ct1642_send_bit_data(unsigned char  v_character, unsigned char v_position,int colon);

/***********************************************************************************************************************
*函数名称：dh_CT1642_key_scan()
*功能说明：按键管理函数
*输入参数：NULL
*返回参数：nKeyPress
*函数功能：该函数负责实现按键扫描，当有键按下时，返回按键值，否则返回STB_KEY_NULL。
***********************************************************************************************************************/
static unsigned char  ct1642_key_scan(void)
{
	unsigned char i;
	unsigned char curKeyPress = 0;				/*当前按键状态值*/
	static unsigned char preKeyPress = 0;		/*前一按键状态值*/
	static short KeyCount = 0;					/*循环按键值*/
	unsigned char const v_KeyCode[8]=
	{
/*0*/	CT1642_REMOTE_KEY_MENU,
/*1*/	CT1642_REMOTE_KEY_ENTER,
/*2*/	CT1642_REMOTE_KEY_UP,
/*3*/	CT1642_REMOTE_KEY_DOWN,
/*4*/	CT1642_REMOTE_KEY_LEFT,
/*5*/	CT1642_REMOTE_KEY_RIGHT,
/*6*/	CT1642_REMOTE_KEY_EXIT,
/*7*/	CT1642_REMOTE_KEY_STANDBY
	};
	//msleep(20);
	 ct1642_send_bit_data(0xff,0x04,0); 
	curKeyPress = CT1642_KEY_IN;				/*读取引脚KEY的电平值*/
	if((1==curKeyPress)&&(0==preKeyPress))		/*有按键按下*/
	{
		for(i=0;i<8;i++)									/*查询按键值*/
		{
			ct1642_send_bit_data(1<<i,0x04,0);
			udelay(1000);
			curKeyPress = CT1642_KEY_IN;				/*读取引脚KEY的电平值*/
			if(1==curKeyPress)
			{
				preKeyPress=1;
				curKeyPress=v_KeyCode[i];
				return curKeyPress;
			}
		}
		return 0xFF;
	}
	else if((1==curKeyPress)&&(1==preKeyPress))	/*有按键长按不放*/
	{
	    KeyCount++;						        /*长按住不放时，做连续按键处理*/
		if(KeyCount == 500)				        /*首次按下延时较长，KeyCount从0加到0x10，再从0x90加到0xaf*/
		{
			KeyCount = 800;
		}
		else if(KeyCount == 1000)				/*以后每次较短，KeyCount从0x90加到0xaf*/
		{
			KeyCount = 800;
			preKeyPress = 0;
		}
		return 0xFF;
	}
	else if((0==curKeyPress)&&(1==preKeyPress))	/*有按键松开*/
	{
		preKeyPress=0;
		KeyCount=0;
		return 0xFF;
	}
	else										/*没有按键按下*/
	{
		return 0xFF;
	}
	return 0xFF;
}

static void MDrv_CT1642_Start( void )
{
    CT1642_SDA_SET;
    CT1642_SDA_D_OUT;
    CT1642_SCL_SET;
    CT1642_SCL_D_OUT;
    //DELAY1;
    CT1642_SDA_CLR;
    //DELAY1;
    CT1642_SCL_CLR;
}

static void MDrv_CT1642_Stop( void )
{
    CT1642_SDA_CLR;
    CT1642_SDA_D_OUT;
    //DELAY1;
    CT1642_SCL_SET;
    //DELAY1;
    CT1642_SDA_SET;
    //DELAY1;
    CT1642_SDA_D_IN;
}
static void  ct1642_send_bit_data(unsigned char  v_character, unsigned char v_position,int colon)
{
	unsigned char BitPosition ;                   /*存储数码管位置编码*/
	unsigned char BitCharacter = v_character;     /*存储数码管显示编码*/
	unsigned char i;
	switch(v_position)
	{
		/*显示第千位数据,BitCharacter|=0x01是点亮  */
		/*电源指示灯D1,有一个缺点是要等系统从Flash */
		/*拷贝到内存当中，才会点亮，而不是一按下电 */
		/*源开关就点亮，对维修可能造成影响         */
		case 0:
		{
			BitPosition=0xef; 	/*1110 1111*/
				BitCharacter |= 0x01;	/*DP:信号灯*/
			break;
		}                        
		case 1: 		/*显示第百位数据,v_lock是信号锁定标志位，为1时信号灯D2点亮,为0时信号灯不亮*/
		{
			BitPosition=0xdf;	/*1101 1111*/
			if(colon)/*control ":" to flash*/
			{
				BitCharacter |= 0x01;
			}
			break;
		}
		case 2:			/*显示第十位数据*/ 	
		{
			BitPosition=0xbf;	/*1011 1111*/
			break;
		}                                        
		case 3:			/*显示第个位数据*/
		{
			BitPosition=0x7f;	/*0111 1111*/
			break;
		}                                        
		case 4:			/*关闭显示，用于键盘扫描*/
		{
			BitPosition=0xff; 	/*1111 1111*/
			break;
		}
		default:		/*默认不显示*/
		{
			BitPosition=0xff;
			BitCharacter=0x00;
			break;
		}                                      
	}
	for(i=0;i<8;i++)	/*发送8位地址*/
	{
		CT1642_SCL_CLR
		if( (BitPosition<<i)&0x80)
		{
			CT1642_SDA_SET
		}
		else
		{
			CT1642_SDA_CLR
		}
		CT1642_SCL_SET
	}
	CT1642_SDA_SET	/*发送两个空位*/
	CT1642_SCL_CLR
	CT1642_SCL_SET
	CT1642_SDA_CLR
	CT1642_SCL_CLR
	CT1642_SCL_SET
	for(i=0;i<8;i++)		/*发送8位数据*/
	{
		CT1642_SCL_CLR
		if( (BitCharacter<<i) & 0x80)
		{
			CT1642_SDA_SET
		}
		else
		{
			CT1642_SDA_CLR
		}
		CT1642_SCL_SET
	}
	CT1642_SCL_SET	/*输出数据*/
	CT1642_SDA_CLR
	CT1642_SDA_SET
	CT1642_SCL_CLR
	CT1642_SDA_CLR
	CT1642_SDA_SET
	return;
}

static MS_U8 MDrv_Led_Get_Code(MS_U8 cTemp)
{
    MS_U8 i, bitmap = 0x00;
    VFD_CHARACTER_t *pSearth;
    pSearth = VFDCharArray;
    while(pSearth->Character!='\0')
    {
        if(pSearth->Character == cTemp)
        {
		//printk("vfd hang here\n");
                 bitmap = pSearth->Value;
             break;
         }
	pSearth++;
    }
    return bitmap;
}

static int ct1642_init(struct vfd_platform_data *pvfd_platform_data)
{
		vfd_set_stb_pin_value = pvfd_platform_data->set_stb_pin_value;				
		vfd_set_clock_pin_value = pvfd_platform_data->set_clock_pin_value;		
		vfd_set_do_pin_value = pvfd_platform_data->set_do_pin_value;
		vfd_get_di_pin_value = pvfd_platform_data->get_di_pin_value;
		
		MDrv_FrontPnl_Init();
		
		return 0;	
}


static int get_ct1642_key_value(void)
{
	
		return ct1642_key_scan();
}


static int set_ct1642_led_value(char *display_code)
{
		int i,j = 0;
		char data,display_char[8];
		int dot = 0;
		for(i = 0; i <= 8; i++)
		{
			data = display_code[i];
			if(data == ':')	{
				//display_char[i] = display_code[i+1];
			  dot++;
			}
			else{				
				display_char[j++] = display_code[i];
				if(data == '\0')break;
			}
		}
		//DBG_VFD(printk("function[%s] line %d display string: %s .\n", __FUNCTION__,__LINE__,display_char));
/*
		printk("function: %s line %d .\n", __FUNCTION__,__LINE__);
		for(i=0;i<j;i++){
		 printk("set led display char[%d] is: %c \n", i,display_char[i]);
		 if(display_char[i] == '\0')	{
		 		printk("end char is: char[%d] \n", i);
		 		break;
		 	}
		}
*/		
		MDrv_FrontPnl_Update(display_char, dot);
		return 0;
}

#ifdef CONFIG_VFD_CT1642
int hardware_init(struct vfd_platform_data *pdev)
{
		int ret;					
		ret = ct1642_init(pdev);					
		return ret;	
}

int get_vfd_key_value(void)
{
		int key_value;
		key_value = get_ct1642_key_value();
		return key_value;
}

int set_vfd_led_value(char *display_code)
{
		int ret;		
		ret = set_ct1642_led_value(display_code);		
		return ret;
}
#endif



void MDrv_FrontPnl_Update(char *U8str, int colon)
{
    	int i;
	MS_U8 data[4] = {0x00, 0x00, 0x00, 0x00};

    	if(!U8str)
   	{
        	return;
   	}
    	for(i = 0; i < 4; i++)
   	{
        	 data[i] = MDrv_Led_Get_Code(U8str[i]);
   	}
    	for(i = 0; i < 4; i++)
	{
		 ct1642_send_bit_data(data[i],3-i,colon);
		 udelay(500);
	}
	ct1642_send_bit_data(0xff,0x04,0);
	udelay(500);
}

// Frontpanel API
void MDrv_FrontPnl_Init(void)
{
//	MDrv_CT1642_Init();
	MDrv_FrontPnl_Update((char *)"----", 0);
}


//------------- END OF FILE ------------------------
