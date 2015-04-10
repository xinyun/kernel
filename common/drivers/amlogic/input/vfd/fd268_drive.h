/* ***************************************************************************************** *
 *	��˾����	:		������΢�������޹�˾��FUZHOU FUDA HISI MICROELECTRONICS CO.,LTD��		
 *	������		��	Ԭ�ı�	                        								
 *	�ļ���		��	FD628_DRIVE.C 														 
 *	��������	��	FD628������ͷ�ļ�����Ҫ��ֲ�޸ĺ͵��õ��ļ�	   	 									
 *	����˵��	��	�������ݵĴ���ӵ�λ��ʼ��FD628�ڴ���ͨ�ŵ�ʱ�������ض�ȡ���ݣ��½����������						 					   
 *	����汾	��	V1B3��2012-10-17��  												
****************************************************************************************** */
#ifndef __FD628_Drive_H__
#define __FD628_Drive_H__

#if 1 //def  FD628_Drive_GLOBALS
#define FD628_Drive_EXT 
#else
#define FD628_Drive_EXT  extern
#endif
//#include 	"../SYSTEM/includes.H"
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
#define		NEGA_LED_NONE 0X00
#define		NEGA_LED_0 0X3F
#define		NEGA_LED_1 0x06
#define		NEGA_LED_2 0x5B
#define		NEGA_LED_3 0x4F
#define		NEGA_LED_4 0x66
#define		NEGA_LED_5 0X6d
#define		NEGA_LED_6 0x7D
#define		NEGA_LED_7 0x07
#define		NEGA_LED_8 0x7f
#define		NEGA_LED_9 0x6F

#define		NEGA_LED_A 0X77 
#define		NEGA_LED_b 0x7c 
#define		NEGA_LED_C 0X39
#define		NEGA_LED_c 0X58
#define		NEGA_LED_d 0x5E
#define		NEGA_LED_E 0X79
#define		NEGA_LED_e 0X7b
#define		NEGA_LED_F 0x71

#define		NEGA_LED_I 0X60
#define		NEGA_LED_L 0X38
#define		NEGA_LED_r 0X72		
#define		NEGA_LED_n 0X54
#define		NEGA_LED_N 0X37
#define		NEGA_LED_O 0X3F
#define		NEGA_LED_P 0XF3
#define		NEGA_LED_S 0X6d
#define		NEGA_LED_y 0X6e
#define		NEGA_LED__ 0x08
/* **************************************API*********************************************** */
/* *************************�û���Ҫ�޸Ĳ���************************** */
typedef unsigned char            BOOLEAN;       /* ����������������*/
typedef unsigned char  INT8U;         /* �޷���8λ��*/
typedef unsigned int  INT32U;         /* �޷���32λ��*/
/* **************�������Ͷ�Ӧ��ֵ����********************** */
#define FD628_KEY10 	0x00000200
#define FD628_KEY9 		0x00000100
#define FD628_KEY8 		0x00000080
#define FD628_KEY7 		0x00000040
#define FD628_KEY6  	0x00000020
#define FD628_KEY5 		0x00000010
#define FD628_KEY4  	0x00000008
#define FD628_KEY3  	0x00000004
#define FD628_KEY2  	0x00000002
#define FD628_KEY1  	0x00000001
#define FD628_KEY_NONE_CODE 0x00

#define FD628_DELAY_KEY_SCAN             //��ʱ10ms
#define FD628_DISP_NORMAL	 (FD628_DISP_ON|FD628_Brightness_8 )
/* *************************�û�����Ҫ�޸Ĳ���************************** */
/* ************** ����FD628�ĺ� ********************** */
#define FD628_4DIG_MODE 					FD628_Command(FD628_4DIG_CMD)						/*����FD628������4λģʽ*/
#define FD628_5DIG_MODE 					FD628_Command(FD628_5DIG_CMD)						/*����FD628������5λģʽ*/
#define FD628_6DIG_MODE 					FD628_Command(FD628_6DIG_CMD)						/*����FD628������6λģʽ*/
#define FD628_7DIG_MODE 					FD628_Command(FD628_7DIG_CMD)						/*����FD628������7λģʽ*/
#define FD628_Disp_Brightness_SET(Status)	FD628_Command(FD628_DISP_STATUE_WRCMD |(Status&0x0f))   	/*����FD628����ʾ��ʽ�����Ⱥ���ʾ���أ�*/ 
/* *************************************************************************************************************************************** *
*	Status˵��	| bit7	| bit6	| bit5	| bit4	| bit3			| bit2	| bit1	| bit0	| 		 Display_EN����ʾʹ��λ��1������ʾ��0���ر���ʾ
*				| 0		| 0		| 0		| 0		| Display_EN	|	brightness[3:0]		|		 brightness����ʾ���ȿ���λ��000��111 �ֱ������1��min����8��max��������
* ************************************************************************************************************************************* */
/* ************** Status����ʹ������ĺ� ��֮���û�Ĺ�ϵ�� ************ */
#define FD628_DISP_ON        					0x08		/*��FD628��ʾ*/
#define FD628_DISP_OFF        				0x00		/*�ر�FD628��ʾ*/

#define FD628_Brightness_1        				0x00		/*����FD628��ʾ���ȵȼ�Ϊ1*/
#define FD628_Brightness_2        				0x01		/*����FD628��ʾ���ȵȼ�Ϊ2*/
#define FD628_Brightness_3        				0x02		/*����FD628��ʾ���ȵȼ�Ϊ3*/
#define FD628_Brightness_4        				0x03		/*����FD628��ʾ���ȵȼ�Ϊ4*/
#define FD628_Brightness_5        				0x04		/*����FD628��ʾ���ȵȼ�Ϊ5*/
#define FD628_Brightness_6        				0x05		/*����FD628��ʾ���ȵȼ�Ϊ6*/
#define FD628_Brightness_7        				0x06		/*����FD628��ʾ���ȵȼ�Ϊ7*/
#define FD628_Brightness_8        				0x07		/*����FD628��ʾ���ȵȼ�Ϊ8*/

#define	FD628_WAIT_KEY_FREE		 		while(FD628_GetKey()!=FD628_KEY_NONE_CODE);		//�ȴ������ͷ�
#define	FD628_WAIT_KEY_PRESS			while(FD628_GetKey()==FD628_KEY_NONE_CODE);		//�ȴ���������	 														 									//����ɨ��ʱ�� 20ms
/* ****************** ���� ************************** */ 
/****************************************************************
 *	����������:					    FD628_Command
 *	����:							���Ϳ�������
 *	����:		             		INT8U ��������
 *	����ֵ:				    	    void
****************************************************************/
//FD628_Drive_EXT		void FD628_Command(INT8U);
/***************************************************************
 *	����������:					    FD628_GetKey
 *	����:										��������ֵ
 *	����:			             	void
 *	����ֵ:					        INT8U ���ذ���ֵ 
 **************************************************************************************************************************************
���صİ���ֵ����  
			| 0			| 0			| 0			| 0			| 0			| 0			| KS10	| KS9		| KS8		| KS7		| KS6		| KS5		| KS4		| KS3		| KS2		| KS1		|
KEY1 	| bit15	| bit14	| bit13	| bit12	| bit11	| bit10	| bit9	| bit8	| bit7	| bit6	| bit5	| bit4	| bit3	| bit2	| bit1	| bit0	| 
KEY2 	| bit31	| bit30	| bit29	| bit28	| bit27	| bit26	| bit25	| bit24	| bit23	| bit22	| bit21	| bit20	| bit19	| bit18	| bit17	| bit16	|
***************************************************************************************************************************************/
//FD628_Drive_EXT		INT32U FD628_GetKey();
/****************************************************************
 *	����������:					    FD628_WrDisp_AddrINC
 *	����:										�Ե�ַ����ģʽ������ʾ����
 *	����:		         				INT8U Addr������ʾ���ݵ���ʼ��ַ�������ַ����ʾ��Ӧ�ı���datasheet
 *													INT8U DataLen ������ʾ���ݵ�λ��
 *	����ֵ:				        	BOOLEAN�������ַ����������1�����ִ�гɹ�����0��
 *  ʹ�÷�����						�Ƚ�����д��FD628_DispData[]����Ӧλ�ã��ٵ���FD628_WrDisp_AddrINC����������
****************************************************************/
//FD628_Drive_EXT		BOOLEAN FD628_WrDisp_AddrINC(INT8U,INT8U)	;
/****************************************************************
 *	����������:				FD628_WrDisp_AddrStatic
 *	����:							�Ե�ַ�̶�ģʽ������ʾ���� ;��ַ��datasheet
 *	����:		          INT8U Addr������ʾ���ݵĵ�ַ��
 *										INT8U DIGData д����ʾ����
 *	����ֵ:				    BOOLEAN�������ַ����������1�����ִ�гɹ�����0��
****************************************************************/
//FD628_Drive_EXT		BOOLEAN FD628_WrDisp_AddrStatic(INT8U,INT8U );
/****************************************************************
 *	����������:				FD628_Init
 *	����:							FD628��ʼ�����û����Ը�����Ҫ�޸���ʾ
 *	����:		          void
 *	����ֵ:				    void
****************************************************************/ 
//FD628_Drive_EXT 	void FD628_Init(void);
//FD628_Drive_EXT		INT8U	FD628_DispData[14]; /* ��ʾ���ݼĴ���,����FD628_WrDisp_AddrINC����ǰ���Ƚ�����д��FD628_DispData[]����Ӧλ�á�*/  
//FD628_Drive_EXT		code  INT8U NEGA_Table[0x10];	/* �����������ֵ�������飬���ζ�Ӧ����ʾ��0,1,2,3,4,5,6,7,8,9,A,b,C,d,E,F  */ 
/* ****************************************************************************************************** */
/* ************************************* *Drive ģ��* ********************************************** */
#ifdef   FD628_Drive_GLOBALS //�ڲ������ͺ���
/* ************************************ *�û���Ҫ�޸Ĳ���* *************************************** */
/* ����ͨ�Žӿڵ�IO����,��ʵ�ʵ�·�й� */
//  	  FD628_STB				;	  //FD628ͨ��ѡͨ 
//  	  FD628_CLK				;	  //FD628ͨ��ʱ�� 
//  	  FD628_DIO				;	 	//FD628ͨ������ 
/* ͨ�Žӿڵ�IO��������ƽ̨IO�����й� */
#if 0
#define		FD628_STB_SET			            					   	/* ��STB����Ϊ�ߵ�ƽ */
#define		FD628_STB_CLR			            					  /* ��STB����Ϊ�͵�ƽ */
#define		FD628_STB_D_OUT						 			    	    /* ����STBΪ������� */
#define		FD628_CLK_SET				             					  /* ��CLK����Ϊ�ߵ�ƽ */
#define		FD628_CLK_CLR				               						/* ��CLK����Ϊ�͵�ƽ */
#define		FD628_CLK_D_OUT					  				  	   		/* ����CLKΪ������� */
#define		FD628_DIO_SET					               						/* ��DIO����Ϊ�ߵ�ƽ */
#define		FD628_DIO_CLR				             						/* ��DIO����Ϊ�͵�ƽ */
#define		FD628_DIO_IN				            							/* ��DIO��Ϊ���뷽��ʱ����ȡ�ĵ�ƽ�ߵ� */
#define		FD628_DIO_D_OUT  										        /* ����DIOΪ������� */
#define		FD628_DIO_D_IN   										        /* ����DIOΪ���뷽�� */
#define 	FD628_DELAY_1us											    	/* ��ʱʱ�� >1us*/
#endif

/* **************************************�û�����Ҫ�޸�*********************************************** */
/* **************д��FD628��ʱ���֣����庬�忴Datasheet��********************** */
#define 	FD628_DELAY_LOW		     	FD628_DELAY_1us                     		        /* ʱ�ӵ͵�ƽʱ�� >500ns*/
#define		FD628_DELAY_HIGH     	 	FD628_DELAY_1us 	   										 				/* ʱ�Ӹߵ�ƽʱ�� >500ns*/
#define  	FD628_DELAY_BUF		 		 	FD628_DELAY_1us	                     				  	/* ͨ�Ž�������һ��ͨ�ſ�ʼ�ļ�� >1us*/
#define  	FD628_DELAY_STB					FD628_DELAY_1us
#endif	
/* ***********************д��FD628��������***************************** */
#define FD628_KEY_RDCMD        					0x42                //������ȡ����
#define FD628_4DIG_CMD        				0x00		/*����FD628������4λģʽ������*/
#define FD628_5DIG_CMD        				0x01		/*����FD628������5λģʽ������*/
#define FD628_6DIG_CMD         				0x02	 	/*����FD628������6λģʽ������*/
#define FD628_7DIG_CMD         				0x03	 	/*����FD628������7λģʽ������*/
#define FD628_DIGADDR_WRCMD  						0xC0								//��ʾ��ַд������
#define FD628_ADDR_INC_DIGWR_CMD       	0x40								//��ַ������ʽ��ʾ����д��
#define FD628_ADDR_STATIC_DIGWR_CMD    	0x44								//��ַ��������ʽ��ʾ����д��	
#define FD628_DISP_STATUE_WRCMD        	0x80								//��ʾ����д������
/* **************************************************************************************************************************** */
#endif
