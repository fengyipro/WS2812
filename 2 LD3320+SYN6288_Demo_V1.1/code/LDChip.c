/***********************************************
**  �������ƣ�UNV-LD3320+STC11����ʶ��ģ����������
**	CPU: STC11L08XE
**	����22.1184MHZ
**	�����ʣ�9600 bit/S
**  �޸����ڣ�2018.5.13
**  ˵��������ģʽ�� ��ÿ��ʶ��ʱ����Ҫ˵��Сӥ������� �����ܹ�������һ����ʶ��
/***********************************************/
#include "config.h"

extern void  delay(unsigned long uldata);

uint8 idata ucRegVal;
extern uint8 idata nAsrStatus;

void ProcessInt0(void);

/************************************************************************
���������� 	 ��λLDģ��
��ڲ�����	 none
�� �� ֵ�� 	 none
����˵����	 none
**************************************************************************/
void LD_Reset()
{
	RSTB=1;
	delay(5);
	RSTB=0;
	delay(5);
	RSTB=1;

	delay(5);
	CSB=0;
	delay(5);
	CSB=1;
	delay(5);
}
/************************************************************************
���������� LDģ�������ʼ��
��ڲ����� none
�� �� ֵ�� none
����˵���� �ú���Ϊ�������ã�һ�㲻��Ҫ�޸ģ�
					 ����Ȥ�Ŀͻ��ɶ��տ����ֲ������Ҫ�����޸ġ�
**************************************************************************/
void LD_Init_Common()
{
	LD_ReadReg(0x06);  
	LD_WriteReg(0x17, 0x35); 
	delay(10);
	LD_ReadReg(0x06);  

	LD_WriteReg(0x89, 0x03);  
	delay(5);
	LD_WriteReg(0xCF, 0x43);   
	delay(5);
	LD_WriteReg(0xCB, 0x02);
	
	/*PLL setting*/
	LD_WriteReg(0x11, LD_PLL_11);       

	LD_WriteReg(0x1E,0x00);
	LD_WriteReg(0x19, LD_PLL_ASR_19); 
	LD_WriteReg(0x1B, LD_PLL_ASR_1B);		
  LD_WriteReg(0x1D, LD_PLL_ASR_1D);
	delay(10);
	
    LD_WriteReg(0xCD, 0x04);
//	LD_WriteReg(0x17, 0x4c); 
	delay(5);
	LD_WriteReg(0xB9, 0x00);
	LD_WriteReg(0xCF, 0x4F); 
	LD_WriteReg(0x6F, 0xFF); 
}

/************************************************************************
���������� 	 LDģ�� ASR���ܳ�ʼ��
��ڲ�����	 none
�� �� ֵ�� 	 none
����˵����	 �ú���Ϊ�������ã�һ�㲻��Ҫ�޸ģ�
					 ����Ȥ�Ŀͻ��ɶ��տ����ֲ������Ҫ�����޸ġ�
**************************************************************************/
void LD_Init_ASR()
{
	LD_Init_Common();
	LD_WriteReg(0xBD, 0x00);
	LD_WriteReg(0x17, 0x48);
	delay( 10 );
	LD_WriteReg(0x3C, 0x80);    
	LD_WriteReg(0x3E, 0x07);
	LD_WriteReg(0x38, 0xff);    
	LD_WriteReg(0x3A, 0x07);
  LD_WriteReg(0x40, 0);          
	LD_WriteReg(0x42, 8);
	LD_WriteReg(0x44, 0);    
	LD_WriteReg(0x46, 8); 
	delay( 1 );
}

/************************************************************************
���������� 	�жϴ�������
��ڲ�����	 none
�� �� ֵ�� 	 none
����˵����	��LDģ����յ���Ƶ�ź�ʱ��������ú�����
						�ж�ʶ���Ƿ��н�������û�д������ü�
            ����׼����һ�ε�ʶ��
**************************************************************************/
void ProcessInt0(void)
{
	uint8 nAsrResCount=0;

	EX0=0;
	ucRegVal = LD_ReadReg(0x2B);
	LD_WriteReg(0x29,0) ;
	LD_WriteReg(0x02,0) ;
	if((ucRegVal & 0x10) &&
		LD_ReadReg(0xb2)==0x21 && 
		LD_ReadReg(0xbf)==0x35)			/*ʶ��ɹ�*/
	{	
		nAsrResCount = LD_ReadReg(0xba);
		if(nAsrResCount>0 && nAsrResCount<=8) 
		{
			nAsrStatus=LD_ASR_FOUNDOK;
		}
		else
	    {
			nAsrStatus=LD_ASR_FOUNDZERO;
		}	
	}															 /*û��ʶ����*/
	else
	{	 
		nAsrStatus=LD_ASR_FOUNDZERO;
	}
		
  LD_WriteReg(0x2b, 0);
  LD_WriteReg(0x1C,0);/*д0:ADC������*/

	LD_WriteReg(0x29,0) ;
	LD_WriteReg(0x02,0) ;
	LD_WriteReg(0x2B,  0);
	LD_WriteReg(0xBA, 0);	
	LD_WriteReg(0xBC,0);	
	LD_WriteReg(0x08,1);	 /*���FIFO_DATA*/
	LD_WriteReg(0x08,0);	/*���FIFO_DATA�� �ٴ�д0*/


	EX0=1;
}

/************************************************************************
���������� 	����ASRʶ������
��ڲ�����	none
�� �� ֵ��  asrflag��1->�����ɹ��� 0��>����ʧ��
����˵����	ʶ��˳������:
						1��RunASR()����ʵ����һ��������ASR����ʶ������
						2��LD_AsrStart() ����ʵ����ASR��ʼ��
						3��LD_AsrAddFixed() ����ʵ�������ӹؼ����ﵽLD3320оƬ��
						4��LD_AsrRun()	����������һ��ASR����ʶ������					
						�κ�һ��ASRʶ�����̣�����Ҫ�������˳�򣬴ӳ�ʼ����ʼ��
**************************************************************************/
uint8 RunASR(void)
{
	uint8 i=0;
	uint8 asrflag=0;
	for (i=0; i<5; i++)			//	��ֹ����Ӳ��ԭ����LD3320оƬ����������������һ������5������ASRʶ������
	{
		LD_AsrStart();
		delay(50);
		if (LD_AsrAddFixed()==0)
		{
			LD_Reset();			//	LD3320оƬ�ڲ����ֲ���������������LD3320оƬ
			delay(50);			//	���ӳ�ʼ����ʼ����ASRʶ������
			continue;
		}
		delay(10);
		if (LD_AsrRun() == 0)
		{
			LD_Reset();			//	LD3320оƬ�ڲ����ֲ���������������LD3320оƬ
			delay(50);			//	���ӳ�ʼ����ʼ����ASRʶ������
			continue;
		}
		asrflag=1;
		break;					//	ASR���������ɹ����˳���ǰforѭ������ʼ�ȴ�LD3320�ͳ����ж��ź�
	}

	return asrflag;
}
/************************************************************************
����������  ���LDģ���Ƿ����
��ڲ�����	none
�� �� ֵ�� 	flag��1-> ����
����˵����	none
**************************************************************************/
uint8 LD_Check_ASRBusyFlag_b2()
{
	uint8 j;
	uint8 flag = 0;
	for (j=0; j<10; j++)
	{
		if (LD_ReadReg(0xb2) == 0x21)
		{
			flag = 1;
			break;
		}
		delay(10);		
	}
	return flag;
}
/************************************************************************
���������� 	����ASR
��ڲ�����	none
�� �� ֵ�� 	none
����˵����	none
**************************************************************************/
void LD_AsrStart()
{
	LD_Init_ASR();
}
/************************************************************************
���������� 	����ASR
��ڲ�����	none
�� �� ֵ�� 	1�������ɹ�
����˵����	none
**************************************************************************/
uint8 LD_AsrRun()
{
	EX0=0;
	LD_WriteReg(0x35, MIC_VOL);
	LD_WriteReg(0x1C, 0x09);
	LD_WriteReg(0xBD, 0x20);
	LD_WriteReg(0x08, 0x01);
	delay( 1 );
	LD_WriteReg(0x08, 0x00);
	delay( 1 );

	if(LD_Check_ASRBusyFlag_b2() == 0)
	{
		return 0;
	}
//	LD_WriteReg(0xB6, 0xa); //ʶ��ʱ��	 1S
//	LD_WriteReg(0xB5, 0x1E); //��������ʱ�� 300ms
//	LD_WriteReg(0xB8, 10); //����ʱ��

//	LD_WriteReg(0x1C, 0x07); //����˫ͨ����Ƶ�ź���Ϊ�����ź�
	LD_WriteReg(0x1C, 0x0b); //������˷���Ϊ�����ź�


	LD_WriteReg(0xB2, 0xff);
	delay( 1);	
	LD_WriteReg(0x37, 0x06);
	delay( 1 );
    LD_WriteReg(0x37, 0x06);
		delay( 5 );
	LD_WriteReg(0x29, 0x10);
	
	LD_WriteReg(0xBD, 0x00);
	EX0=1;
	return 1;
}
/************************************************************************
���������� ��LDģ�����ӹؼ���
��ڲ����� none
�� �� ֵ�� flag��1->���ӳɹ�
����˵���� �û��޸�.
					 1���������¸�ʽ����ƴ���ؼ��ʣ�ͬʱע���޸�sRecog ��pCode ����ĳ���
					 �Ͷ�Ӧ����k��ѭ���á�ƴ������ʶ������һһ��Ӧ�ġ�
					 2�������߿���ѧϰ"����ʶ��оƬLD3320�߽��ؼ�.pdf"��
           ���������������մ�����÷������ṩʶ��Ч����
					 3����xiao ying �� Ϊ�������ÿ��ʶ��ʱ�������ȷ�һ�����Сӥ��
**************************************************************************/
uint8 LD_AsrAddFixed()
{
	uint8 k, flag;
	uint8 nAsrAddLength;
	#define DATE_A 5   /*�����ά��ֵ*/
	#define DATE_B 30		/*����һά��ֵ*/
	uint8 code sRecog[DATE_A][DATE_B] = {
																				"da kai deng guang",\
																				"guan bi deng guang",\
																				"hong se mo shi",\
																				"lan se mo shi",\
																				"bai se mo shi"
																			};	/*���ӹؼ��ʣ��û��޸�*/
	uint8 code pCode[DATE_A] = {
															CODE_OPEN_LIGHT,\
															CODE_CLOSE_LIGHT,\
															CODE_RED_MODE,\
															CODE_BLUE_MODE,\
															CODE_WHITE_MODE
														 };	/*����ʶ���룬�û��޸�*/	
	flag = 1;
	for (k=0; k<DATE_A; k++)
	{
			
		if(LD_Check_ASRBusyFlag_b2() == 0)
		{
			flag = 0;
			break;
		}
		
		LD_WriteReg(0xc1, pCode[k] );
		LD_WriteReg(0xc3, 0 );
		LD_WriteReg(0x08, 0x04);
		delay(1);
		LD_WriteReg(0x08, 0x00);
		delay(1);

		for (nAsrAddLength=0; nAsrAddLength<DATE_B; nAsrAddLength++)
		{
			if (sRecog[k][nAsrAddLength] == 0)
				break;
			LD_WriteReg(0x5, sRecog[k][nAsrAddLength]);
		}
		LD_WriteReg(0xb9, nAsrAddLength);
		LD_WriteReg(0xb2, 0xff);
		LD_WriteReg(0x37, 0x04);
	}
    return flag;
}
/************************************************************************
���������� 	��ȡʶ����
��ڲ�����	none
�� �� ֵ�� 	LD_ReadReg(0xc5 )��  ��ȡ�ڲ��Ĵ�������ʶ���롣
����˵����	none
**************************************************************************/
uint8 LD_GetResult()
{		
	return LD_ReadReg(0xc5 );
}



