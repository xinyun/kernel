/* ***************************************************************************************** *
 *	��˾����	:		������΢�������޹�˾��FUZHOU FUDA HISI MICROELECTRONICS CO.,LTD��		
 *	������		��	Ԭ�ı�	                        								
 *	�ļ���		��	FD628_DRIVE.C 														 
 *	��������	��	ʵ��FD628������		   	 									
 *	����˵��	��	�������ݵĴ���ӵ�λ��ʼ��FD628�ڴ���ͨ�ŵ�ʱ�������ض�ȡ���ݣ��½����������						 					   
 *	����汾	��	V1B3��2012-10-17��  												
****************************************************************************************** */
#define FD628_Drive_GLOBALS

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>

#include <mach/am_regs.h>

#include <linux/io.h>
#include <plat/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <linux/device.h>
#include <linux/spi/flash.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include <mach/mod_gate.h>

#include <mach/gpio.h>
//#include <mach/gpio_data.h>
#include <linux/input/vfd.h>
#include <linux/delay.h>
#include <linux/kernel.h>

#include	"fd268_drive.h"	

#ifdef CONFIG_VFD_FD628SW
INT8U FD628_DispData[14]={0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00};
/* ��ʾ���ݼĴ���,����FD628_WrDisp_AddrINC����ǰ���Ƚ�����д��FD628_DispData[]����Ӧλ�á�*/  
INT8U NEGA_Table[0x10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7c,0x39,0x5E,0x79,0x71};
									    /*  0    1    2    3    4    5    6    7    8    9    A    b    C    d    E		F */ 

static void FD628_Start( void );
static void FD628_Stop( void );
static void FD628_WrByte( INT8U dat );
//static INT8U  FD628_RdByte( void );
static void FD628_Command(INT8U CMD);
//static INT32U FD628_GetKey(void);
static BOOLEAN  FD628_WrDisp_AddrINC(INT8U Addr,INT8U DataLen );
//static BOOLEAN FD628_WrDisp_AddrStatic(INT8U Addr,INT8U DIGData );

#if 0
static void init_gpio(void)
{
	static pinmux_item_t smc_pins[] = {
	    {disable i2C spi  /*GPIOX_29 30 31 gpio*/
	        .reg = PINMUX_REG(8),
	        .clrmask = 0x7f << 16
	    },
	    PINMUX_END_ITEM
	};

	static pinmux_set_t smc_pinmux_set = {
	    .chip_select = NULL,
	    .pinmux = &smc_pins[0]
	};
	pinmux_set(&smc_pinmux_set);
    

}
#endif
/****************************************************************
 *	����������:			    set_stb_gpio
 *	����:					����STB��Ϊ����������
 *	������					void
 *	����ֵ:					void
****************************************************************/
static INT8U set_stb_gpio(INT8U value)
{
	if(value>0)
	aml_clr_reg32_mask(P_AO_GPIO_O_EN_N,1<<6);
	return 1;
}
/****************************************************************
 *	����������:			    set_stb_gpio
 *	����:					����STB��Ϊ����������
 *	������					void
 *	����ֵ:					void
****************************************************************/
/*static void set_stb_out(INT8U value)
{
	if(value>0)
	aml_clr_reg32_mask(P_AO_GPIO_O_EN_N,1<<6);
}

static INT8U get_stb_in(void)
{
    return (gpio_in_get(PAD_GPIOX_31)>0)?1:0;
}*/
/****************************************************************
 *	����������:			    set_clk_gpio
 *	����:					����CLK��Ϊ����������
 *	������					void
 *	����ֵ:					void
****************************************************************/
/*static INT8U set_clk_gpio(INT8U value)
{
    gpio_set_status(PAD_GPIOX_29,(value>0)?(gpio_status_out):(gpio_status_in));
	return 1;
}
static void set_clk_out(INT8U value)
{
	if(value>0)
	aml_clr_reg32_mask(P_AO_GPIO_O_EN_N,1<<6);
}

static INT8U get_clk_in(void)
{
    return (gpio_in_get(PAD_GPIOX_29)>0)?1:0;
}*/
/****************************************************************
 *	����������:			    set_clk_gpio
 *	����:					����CLK��Ϊ����������
 *	������					void
 *	����ֵ:					void
****************************************************************/
static INT8U set_dio_gpio(INT8U value)
{
	if(value>0)
	aml_clr_reg32_mask(P_AO_GPIO_O_EN_N,1<<5);
	else
	aml_set_reg32_mask(P_AO_GPIO_O_EN_N,1<<5);
	return 1;
}
#if 0
static void set_dio_out(INT8U value)
{
	gpio_out(PAD_GPIOX_30, (value>0)?(1):(0));
}
static INT8U get_dio_in(void)
{
    return (gpio_in_get(PAD_GPIOX_30)>0)?1:0;
}
#endif

void MDrv_FrontPnl_Init(void);

int (*vfd_set_stb_pin_value)(int value);
int (*vfd_set_clock_pin_value)(int value);
int (*vfd_set_do_pin_value)(int value);
int (*vfd_get_di_pin_value)(void);


static int fd628_init(struct vfd_platform_data *pvfd_platform_data)
{
		vfd_set_stb_pin_value = pvfd_platform_data->set_stb_pin_value;				
		vfd_set_clock_pin_value = pvfd_platform_data->set_clock_pin_value;		
		vfd_set_do_pin_value = pvfd_platform_data->set_do_pin_value;
		vfd_get_di_pin_value = pvfd_platform_data->get_di_pin_value;
		
		MDrv_FrontPnl_Init();
		
		return 0;	
}

static int set_FD628_led_value(char *display_code)
{
    int i = 0;
    char data;
    int dot = 0;	
    
    for(i = 0; i < 4; i++)
    {
        data = display_code[i];
        if(data >= '0' && data <= '9')
            FD628_DispData[2*i] = NEGA_Table[data-'0'];
        else if(data >= 'a' && data <= 'f')
            FD628_DispData[2*i] = NEGA_Table[data-'a'+10];
        else if(data >= 'A' && data <= 'F')
            FD628_DispData[2*i] = NEGA_Table[data-'A'+10];
        else if(data == 'l' || data == 'L')
            FD628_DispData[2*i] = 0x38;
        else
            FD628_DispData[2*i] = NEGA_LED_NONE;
    }
    
    FD628_DispData[8] = 0;
    for(i=4 ;i< 11; i++)
    {
        data = display_code[i];
        if(data == '1')
            dot = 0X01<<(i-4);
        else
            dot = 0;
        
        FD628_DispData[8] |= dot;
    }	
    
    printk("spink test ****set_FD628_led_value*******************\n");
    FD628_WrDisp_AddrINC(0x00,14);
    return 0;
}

#if 1 //def CONFIG_VFD_SM1628
int hardware_init(struct vfd_platform_data *pdev)
{
		int ret;					
		ret = fd628_init(pdev);					
		return ret;	
}

int get_vfd_key_value(void)
{
		int key_value;
		key_value = 0;//get_sm1628_key_value();
		return key_value;
}

int set_vfd_led_value(char *display_code)
{
		int ret;		
		ret = set_FD628_led_value(display_code);//FD628_WrDisp_AddrINC(0x00,14);//set_sm1628_led_value(display_code);		
		return ret;
}
#endif

#if 1//0
/* ͨ�Žӿڵ�IO��������ƽ̨IO�����й� */
#define FD628_STB_SET                                   vfd_set_stb_pin_value(1)          /* ��STB����Ϊ�ߵ�ƽ */
#define FD628_STB_CLR                                   vfd_set_stb_pin_value(0)          /* ��STB����Ϊ�͵�ƽ */
#define FD628_STB_D_OUT                                 set_stb_gpio(1)         /* ����STBΪ������� */
#define FD628_CLK_SET                                   vfd_set_clock_pin_value(1)          /* ��CLK����Ϊ�ߵ�ƽ */
#define FD628_CLK_CLR                                   vfd_set_clock_pin_value(0)          /* ��CLK����Ϊ�͵�ƽ */
#define FD628_CLK_D_OUT                                 set_stb_gpio(1)         /* ����CLKΪ������� */
#define FD628_DIO_SET                                   vfd_set_do_pin_value(1)          /* ��DIO����Ϊ�ߵ�ƽ */
#define FD628_DIO_CLR                                   vfd_set_do_pin_value(0)          /* ��DIO����Ϊ�͵�ƽ */
#define FD628_DIO_IN                                    vfd_get_di_pin_value()            /* ��DIO��Ϊ���뷽��ʱ����ȡ�ĵ�ƽ�ߵ� */
#define FD628_DIO_D_OUT                                 set_dio_gpio(1)  		/* ����DIOΪ������� */
#define FD628_DIO_D_IN                                  set_dio_gpio(0)         /* ����DIOΪ���뷽�� */
#define FD628_DELAY_1us                                 udelay(10)               /* ��ʱʱ�� >1us*/

#endif

/****************************************************************
 *	����������:			FD628_Start
 *	����:						FD628ͨ�ŵ���ʼ׼��
 *	������					void
 *	����ֵ:					void
****************************************************************/
static void FD628_Start( void )
{	
	FD628_STB_CLR;  				  /* ����STBΪ�͵�ƽ */
	FD628_STB_D_OUT;				  /* ����STBΪ������� */	
	FD628_CLK_D_OUT;				  /* ����CLKΪ������� */	
	FD628_DELAY_STB;	
}
/****************************************************************
 *	����������:			FD628_Stop
 *	����:						FD628ͨ�ŵĽ���׼��
 *	������					void
 *	����ֵ:					void
****************************************************************/
static void FD628_Stop( void )
{  		
	FD628_CLK_SET;						  /* ����CLKΪ�ߵ�ƽ */
	FD628_DELAY_STB;
	FD628_STB_SET;  					  /* ����STBΪ�ߵ�ƽ */
	FD628_DIO_SET;						  /* ����DIOΪ�ߵ�ƽ */
	FD628_DIO_D_IN;						  /* ����DIOΪ���뷽�� */
	FD628_DELAY_BUF;					  /* ͨ�Ž�������һ��ͨ�ſ�ʼ�ļ�� */
}
/****************************************************************
 *	����������:			FD628_WrByte
 *	����:						��FD628д��һ���ֽڵ�����
 *	������					INT8U  ���͵�����
 *	����ֵ:					void
 *	ע��:						���ݴӵ�λ����λ����  
****************************************************************/
static void FD628_WrByte( INT8U dat )
{
	INT8U i;				        			/* ��λ���Ʊ��� */
	FD628_DIO_D_OUT;		        	/* ����DIOΪ������� */
	for( i = 0; i != 8; i++ )	    /* ���8 bit������ */        
	{		
		FD628_CLK_CLR;					  	/* ����CLKΪ�͵�ƽ */
		if( dat & 0x01 ) 						/* ���ݴӵ�λ����λ��� */
		{
		    FD628_DIO_SET;		    	/* ����DIOΪ�ߵ�ƽ */
		}
		else 
		{
		    FD628_DIO_CLR;					/* ����DIOΪ�͵�ƽ */
		}
  	FD628_DELAY_LOW;						/* ʱ�ӵ͵�ƽʱ�� */	
		FD628_CLK_SET;							/* ����SCLΪ�ߵ�ƽ ������д��*/
		dat >>= 1;									/* �����������һλ�����ݴӵ͵��ߵ���� */
		FD628_DELAY_HIGH;          	/* ʱ�Ӹߵ�ƽʱ�� */
	}	
}
#if 0	
/****************************************************************
 *	����������:			FD628_RdByte
 *	����:						��FD628��һ���ֽڵ�����
 *	������					void
 *	����ֵ:					INT8U ����������
 *	ע��:						���ݴӵ�λ����λ����  
****************************************************************/
static INT8U  FD628_RdByte( void )
{
	INT8U i	,dat = 0;				 			/* ��λ���Ʊ���i;��ȡ�����ݴ����dat */
	FD628_DIO_SET;		            /* ����DIOΪ�ߵ�ƽ */
	FD628_DIO_D_IN;		       		  /* ����DIOΪ������� */
	for( i = 0; i != 8; i++ )	    /* ���8 bit������ */        
	{		
		FD628_CLK_CLR;					  	/* ����CLKΪ�͵�ƽ */
 	  FD628_DELAY_LOW;						/* ʱ�ӵ͵�ƽʱ�� */
		dat >>= 1;					 				/* ������������һλ�����ݴӵ͵��ߵĶ��� */
		if( FD628_DIO_IN ) dat|=0X80;		/* ����1 bitֵ */
		FD628_CLK_SET;							/* ����CLKΪ�ߵ�ƽ */
		FD628_DELAY_HIGH;          	/* ʱ�Ӹߵ�ƽʱ�� */
	}		
	return dat;						 				/* ���ؽ��յ������� */
}	
#endif
/****************************************FD628��������*********************************************/
/****************************************************************
 *	����������:					    FD628_Command
 *	����:										���Ϳ�������
 *	����:		             		INT8U ��������
 *	����ֵ:				    	    void
****************************************************************/
static void FD628_Command(INT8U CMD)
{
	FD628_Start();
	FD628_WrByte(CMD);
	FD628_Stop();
}
/****************************************************************
 *	����������:					    FD628_GetKey
 *	����:										��������ֵ
 *	����:			             	void
 *	����ֵ:					        INT32U ���ذ���ֵ 
 **************************************************************************************************************************************
���صİ���ֵ����  
				| 0			| 0			| 0			| 0			| 0			| 0			| KS10	| KS9		| KS8		| KS7		| KS6		| KS5		| KS4		| KS3		| KS2		| KS1		|
KEYI1 	| bit15	| bit14	| bit13	| bit12	| bit11	| bit10	| bit9	| bit8	| bit7	| bit6	| bit5	| bit4	| bit3	| bit2	| bit1	| bit0	| 
KEYI2 	| bit31	| bit30	| bit29	| bit28	| bit27	| bit26	| bit25	| bit24	| bit23	| bit22	| bit21	| bit20	| bit19	| bit18	| bit17	| bit16	|
***************************************************************************************************************************************/
#if 0
static INT32U FD628_GetKey(void)
{
	INT8U i,KeyDataTemp;
	INT32U FD628_KeyData=0;
	FD628_Start();
	FD628_WrByte(FD628_KEY_RDCMD);
	for(i=0;i!=5;i++)
	{
		KeyDataTemp=FD628_RdByte();					   /*��5�ֽڵİ�����ֵת����2�ֽڵ���ֵ*/
		if(KeyDataTemp&0x01)	 FD628_KeyData|=(0x00000001<<i*2);
		if(KeyDataTemp&0x02)	 FD628_KeyData|=(0x00010000<<i*2);
		if(KeyDataTemp&0x08)	 FD628_KeyData|=(0x00000002<<i*2);
		if(KeyDataTemp&0x10)	 FD628_KeyData|=(0x00020000<<i*2);
	}
	FD628_Stop();
	return(FD628_KeyData);
}
#endif
/****************************************************************
 *	����������:					    FD628_WrDisp_AddrINC
 *	����:										�Ե�ַ����ģʽ������ʾ����
 *	����:		         				INT8U Addr������ʾ���ݵ���ʼ��ַ�������ַ����ʾ��Ӧ�ı���datasheet
 *													INT8U DataLen ������ʾ���ݵ�λ��
 *	����ֵ:				        	BOOLEAN�������ַ����������1�����ִ�гɹ�����0��
 *  ʹ�÷�����						�Ƚ�����д��FD628_DispData[]����Ӧλ�ã��ٵ���FD628_WrDisp_AddrINC����������
****************************************************************/
// λ˳��
INT8U realDGInum[14]=
{
    4,1,2,3,6,5,0,7,8,9,10,11,12,13
};
// ��˳��
INT8U realDGInum_bit[7]=
{
    0x10,0x08,0X04,0X02,0X01,0x40,0X20
};

INT8U gongyang_FD628_DispData[14];
/* ************************************************************************************************************************************* 
 *            a
 *         -------
 *        |       |
 *      f |       | b
 *         ---g---		
 *        |       |	c
 *      e |       |	
 *         ---d---   dp
 * *************************************************************************************************************************************** *
 *����		| 0		| 1		| 2		| 3		| 4		| 5		| 6		| 7		| 8		| 9		| A		| b		| C 	| d		| E		| F		|
 *����		|0x3F	|0x06	|0x5B	|0x4F	|0x66	|0x6D	|0x7D	|0x07	|0x7F	|0x6F	|0x77	|0x7c	|0x39	|0x5E	|0x79	|0x71	|
 ************************************************************************************************************************************* */

static void Get_gongyang_date(void)
{
    INT8U i;
    for(i=0;i<14;i++)
    {
        gongyang_FD628_DispData[i]= 0;
    }
    
    for(i =0;i< 5;i++)
    {
        gongyang_FD628_DispData[0]  |= ((FD628_DispData[realDGInum[i*2]] & realDGInum_bit[0])? 0x01:0);
        gongyang_FD628_DispData[2]  |= ((FD628_DispData[realDGInum[i*2]] & realDGInum_bit[1])? 0x01:0);
        gongyang_FD628_DispData[4]  |= ((FD628_DispData[realDGInum[i*2]] & realDGInum_bit[2])? 0x01:0);
        gongyang_FD628_DispData[6]  |= ((FD628_DispData[realDGInum[i*2]] & realDGInum_bit[3])? 0x01:0);
        gongyang_FD628_DispData[8]  |= ((FD628_DispData[realDGInum[i*2]] & realDGInum_bit[4])? 0x01:0);
        gongyang_FD628_DispData[10] |= ((FD628_DispData[realDGInum[i*2]] & realDGInum_bit[5])? 0x01:0);
        gongyang_FD628_DispData[12] |= ((FD628_DispData[realDGInum[i*2]] & realDGInum_bit[6])? 0x01:0);

        if(i<4)
        {
            gongyang_FD628_DispData[0]  <<= 0x01;
            gongyang_FD628_DispData[2]  <<= 0x01;
            gongyang_FD628_DispData[4]  <<= 0x01;
            gongyang_FD628_DispData[6]  <<= 0x01;
            gongyang_FD628_DispData[8]  <<= 0x01;
            gongyang_FD628_DispData[10] <<= 0x01;
            gongyang_FD628_DispData[12] <<= 0x01;
        }
    }

    printk("spink test *************Get_gongyang_date*******************\n");
}



static BOOLEAN  FD628_WrDisp_AddrINC(INT8U Addr,INT8U DataLen )
{
	INT8U i;
	if(DataLen+Addr>14) return(1);
    
    Get_gongyang_date();
    
	FD628_Command(FD628_ADDR_INC_DIGWR_CMD);
	FD628_Start();
	FD628_WrByte(FD628_DIGADDR_WRCMD|Addr);
	for(i=Addr;i!=(Addr+DataLen);i++)
	{
		//FD628_WrByte(FD628_DispData[i]);
		FD628_WrByte(gongyang_FD628_DispData[i]);
	}
    printk("spink test *************FD628_WrDisp_AddrINC*******************\n");
	FD628_Stop();
	return(0);
}
#if 0
/****************************************************************
 *	����������:				FD628_WrDisp_AddrStatic
 *	����:							�Ե�ַ�̶�ģʽ������ʾ���� ;��ַ��datasheet
 *	����:		          INT8U Addr������ʾ���ݵĵ�ַ��
 *										INT8U DIGData д����ʾ����
 *	����ֵ:				    BOOLEAN�������ַ����������1�����ִ�гɹ�����0��
****************************************************************/
static BOOLEAN FD628_WrDisp_AddrStatic(INT8U Addr,INT8U DIGData )
{
	if(Addr>=14) return(1);
	FD628_Command(FD628_ADDR_STATIC_DIGWR_CMD);
	FD628_Start();
	FD628_WrByte(FD628_DIGADDR_WRCMD|Addr);
	FD628_WrByte(DIGData);
	FD628_Stop();
	return(0);
}
#endif


void Led_Show_lockflg(bool SingalStatus)
{
    if (SingalStatus)
    {   
//        FP_Singal_LED_OnOff(1);
    }   
    else
    {   
//        FP_Singal_LED_OnOff(0);
    }   
}






/****************************************************************
 *	����������:				FD628_Init
 *	����:							FD628��ʼ�����û����Ը�����Ҫ�޸���ʾ
 *	����:		          void
 *	����ֵ:				    void
****************************************************************/ 

static void FD628_Init(void )
{
    
    //init_gpio();
    
	FD628_CLK_SET;						  /* ����CLKΪ�ߵ�ƽ */
	FD628_STB_SET;  					  /* ����STBΪ�ߵ�ƽ */
	FD628_DIO_SET;						  /* ����DIOΪ�ߵ�ƽ */
	FD628_STB_D_OUT;				  /* ����STBΪ������� */	
	FD628_CLK_D_OUT;				  /* ����CLKΪ������� */	
	FD628_DIO_D_OUT;						  /* ����DIOΪ���뷽�� */
	FD628_DELAY_BUF;					  /* ͨ�Ž�������һ��ͨ�ſ�ʼ�ļ�� */
	FD628_7DIG_MODE;
	FD628_Disp_Brightness_SET(FD628_DISP_NORMAL);	

    FD628_DispData[0]=NEGA_LED_NONE;  //BIT4
	FD628_DispData[2]=NEGA_LED_NONE;   //BIT3
	FD628_DispData[4]=NEGA_LED_NONE;   //BIT2
	FD628_DispData[6]=NEGA_LED_NONE;   //BIT1
	FD628_DispData[8]=NEGA_LED_NONE;   //BIT0
	
	FD628_WrDisp_AddrINC(0x00,14);

    printk("spink test ****************FD628_Init *******************\n");
}

// Frontpanel API
void MDrv_FrontPnl_Init(void)
{
    printk("jack *******************   MDrv_FrontPnl_Init *******************\n");
    FD628_Init();
	//MDrv_FrontPnl_Update((char *)"8888", 0);
}

#endif

