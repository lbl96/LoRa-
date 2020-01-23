#include "DELAY/Delay.h"
#include "ADC/ADC.h"
#include "DHT11/DHT11.h"
#include "LED/LED.h"
#include "UART/uart.h"
#include "RELAY/Relay.h"
#include "IIC/IIC.h"
#include "OLED/OLED.h"
#include "SPIx/SPIx.h"
#include "SX1278/SX1278.h"
#include "NodeBus.h"

DeviceBlock DeviceBlock_Structure;
//unsigned char Ackbuffer1[9];
int main(void)
{
	u8 i = 0,j = 0;
	u8 sys=0;
	u16 cache;
	extern unsigned char Askbuffer[6];

	/*初始化各外设*/ 
    initSysTick();
	initUART();
    initDHT11();
	initIIC();
    initOLED();
    initSPIx(SPI2);
    initSX1278();
	//initADC();
	//initLED();
	//initRelay();

	formatScreen(0x00);
	showString(0,2,"Hum :",FONT_16_EN);
	showString(0,4,"Temp:",FONT_16_EN);
    showString(0,6,"Bpm :",FONT_16_EN);
	showString(100,2,"Slave:",FONT_16_EN);

    while (1)
    {
		if(++j==10)
		{
			j = 0;
		 	cache = readDHT11();  //获取温湿度
		 } 
		
		 DeviceBlock_Structure.Temperature = cache&0x00FF;
		 DeviceBlock_Structure.Humidity = cache>>8;
     //DeviceBlock_Structure.Bpm = /*心跳读取函数*/;
		 
		 sys=processMasterAsk(&DeviceBlock_Structure);//处理指令任务
		 
         /*根据设备块参数设置继电器状态*/
#if 0
		 if(DeviceBlock_Structure.Coils&0x01)
		 {
			 setRelay(RELAY1,RELAY_CLOSE);
		 }else  
		 {
			 setRelay(RELAY1,RELAY_OPEN);
		 }
		 
		 if(DeviceBlock_Structure.Coils&0x02)
		 {
			 setRelay(RELAY2,RELAY_CLOSE);
		 }else 
		 {
			 setRelay(RELAY2,RELAY_OPEN);
		 }
#endif

i=0;i++;if(i==40){formatScreen(0x00);}
		 switch(sys){
			 case 0:{
         /*显示传感器数据*/
		showNumber(120,2,Askbuffer[1] ,DEC,3,FONT_16_EN);
        showNumber(40,2,DeviceBlock_Structure.Humidity,DEC,3,FONT_16_EN);
        showNumber(40,4,DeviceBlock_Structure.Temperature,DEC,3,FONT_16_EN);
        showNumber(40,6,DeviceBlock_Structure.Bpm,DEC,3,FONT_16_EN);
#if 0
		toggleLED();
#endif
         Delay_ms(100);		
					} break;
			 case 1:	formatScreen(0x00);showString(0,2,"NET_ADDR_ERROR",FONT_16_EN);formatScreen(0x00);break;
			 case 2:  	formatScreen(0x00);showString(0,4,"SLAVE_ADDR_ERROR",FONT_16_EN);formatScreen(0x00);break;
			 case 3:    formatScreen(0x00);showString(0,6,"CRC_CHECK_ERROR",FONT_16_EN);formatScreen(0x00);break;
			 case 255:  formatScreen(0x00);showString(0,6,"EMPTY:NO_DATA",FONT_16_EN);formatScreen(0x00);break;
			 default :  formatScreen(0x00);showString(0,6,"UNKOWN_CODE",FONT_16_EN);formatScreen(0x00);break;
		 }
}
}
