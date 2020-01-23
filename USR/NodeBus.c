#include "NodeBus.h"
#include "SX1278/SX1278.h"
#include "UART/uart.h"
unsigned char Askbuffer[6];
/**
* 功能：根据ModBus规则计算CRC16
* 参数：
*       _pBuf:待计算数据缓冲区,计算得到的结果存入_pBuf的最后两字节
*       _usLen:待计算数据长度(字节数)
* 返回值：16位校验值
*/
static unsigned short int getModbusCRC16(unsigned char *_pBuf, unsigned short int _usLen)
{
	unsigned short int CRCValue = 0xFFFF;  			//初始化CRC变量各位为1
	unsigned char i,j;

	for(i=0;i<_usLen;++i)
	{
		CRCValue  ^= *(_pBuf+i);       			    //当前数据异或CRC低字节
		for(j=0;j<8;++j)                            //一个字节重复右移8次
		{
			if((CRCValue & 0x01) == 0x01)           //判断右移前最低位是否为1
			{
				 CRCValue = (CRCValue >> 1)^0xA001; //如果为1则右移并异或表达式
			}else 
			{
				CRCValue >>= 1;                     //否则直接右移一位
			}			
		}

	} 

    if(CRCValue&0xFF==0)
    {
        CRCValue |= 0xA5;
    }
    return CRCValue;            
} 

static unsigned char getFrameLength(unsigned char* pbuffer,unsigned char buffer_len)
{
    unsigned char len = buffer_len-1;

    /*数据帧长度最大值不会超过255*/
    while(len)
    {
        if(*(pbuffer+len)!=0)
        {
            break;
        }
        --len;
    }
    return len+1;
}

void sendMasterAsk(unsigned char slave_addr,unsigned char op_code,unsigned char pram)
{
    /*发送数据缓冲区，其大小根绝实际情况设置，本例程为6*/
    unsigned char sendbuffer[6] = {NET_ADDR,slave_addr,op_code,pram,0,0};
    unsigned short int CRC16 = getModbusCRC16(sendbuffer,4);              //计算CRC值

    sendbuffer[4] = CRC16>>8;                                             //赋值CRC高位
    sendbuffer[5] = CRC16;                                                //赋值CRC低位
    
    transmitPackets(sendbuffer,sizeof(sendbuffer));                     //发送数据缓冲区
}
FrameStatus receiveSlaveAck(unsigned char slave_addr,unsigned char op_code,unsigned char pram,DeviceBlock * pdevblock)
{
    /*接收数据缓冲区，其大小根绝实际情况设置，本例程最大值为9*/
    unsigned char receivebuffer[9];
    unsigned char len;
    unsigned char i;

    if(receivePackets(receivebuffer)==1) //收到数据
    {
        if(receivebuffer[0] != NET_ADDR) //不是本网络数据，扔掉
        {
            return FRAME_NETADDR_ERR;
        }

        if(receivebuffer[1] != slave_addr)//不是目标从机发送的数据，扔掉
        {
            return FRAME_SLAVEADDR_ERR;
        }

        len = getFrameLength(receivebuffer,sizeof(receivebuffer));
        if(getModbusCRC16(receivebuffer,len-2) != (receivebuffer[len-2]<<8 | receivebuffer[len-1]))//校验值不对数据无效扔掉
        {
            return FRAME_CRC_ERR;
        }

        /*一切正常开始处理数据*/
#if 0
        if(op_code==OP_W_COILS)           //此时是写线圈(继电器)操作
        {
            (pdevblock+slave_addr)->Coils = receivebuffer[3];//
		}
#endif
			if(op_code==OP_R_SENSOR)    //此时是读寄存器操作
        {
            /**
             * 该函数传入的应该是一个DeviceBlock类型的数组，每个从机对应一个DeviceBlock类型的数组元素
             */
            for(i=0;i<8;++i)
            {

                switch(pram & (0x01<<i))//按位处理数据读取传感器
                {
                    case PRAM_R_TEMPERATURE:(pdevblock+slave_addr)->Temperature = receivebuffer[3];                break;
                    case PRAM_R_HUMIDITY   :(pdevblock+slave_addr)->Humidity = receivebuffer[4];                   break;
					case PRAM_R_BPM        :{
														(pdevblock+slave_addr)->Bpm = receivebuffer[5];
											            (pdevblock+slave_addr)->Bpm = receivebuffer[6];  
												       }  break;

                    default   :   break;
                }                           
            }
        }else 
        {
            /*其他操作码可在此扩充*/
        }
        return FRAME_OK;                 //接收数据成功
    }else
    {
        /**
         * 没收到从机发送过来的数据，此时的状态可能是处于等待消息阶段，
         * 当长时间没有收到数据后，可以命令从机重发或者标注该从机出现问题 
         */
        return FRAME_EMPTY;                                         
    }

    return FRAME_EMPTY;                  //不会执行到这里,添加该语句可以避免警告
}

FrameStatus processMasterAsk(DeviceBlock * pdevblock)//传入向主机发送的DeviceBlock类型结构体指针
{
    //unsigned char Askbuffer[6];//下面有定义了//两个各有作用，接受主机询问帧，为6
    //ask为接受主机缓冲区6bits[在sendmasterAsk函数中改大小]，ack为返回给主机帧9bits[在processMasterAsk中改大小]
    unsigned char Ackbuffer[9] = {NET_ADDR,SLAVE1_ADDR};
    unsigned char len;
    unsigned short int CRC16;
    unsigned char i;

    if(receivePackets(Askbuffer)==1) //收到数据
    {
        if(Askbuffer[0] != NET_ADDR /*可修改网络地址*/ ) //不是本网络数据，扔掉
        {
            return FRAME_NETADDR_ERR;
        }

        if(Askbuffer[1] != SLAVE1_ADDR /*可修改子地址*/ )//不是目标从机发送的数据，扔掉
        {
            return FRAME_SLAVEADDR_ERR;
        }

        len = getFrameLength(Askbuffer,sizeof(Askbuffer));
        if(getModbusCRC16(Askbuffer,len-2) != (Askbuffer[len-2]<<8 | Askbuffer[len-1]))//校验值不对数据无效扔掉
        {
            return FRAME_CRC_ERR;
        }
#if 0
        if(Askbuffer[2]==OP_W_COILS)
        {
            Ackbuffer[2] = OP_W_COILS;
            pdevblock->Coils = Askbuffer[3];
            Ackbuffer[3] = pdevblock->Coils;
            CRC16 = getModbusCRC16(Ackbuffer,4);
            Ackbuffer[4] = CRC16>>8;
            Ackbuffer[5] = CRC16;
        }
#endif		
		 if(Askbuffer[2]==OP_R_SENSOR)
        {
            Ackbuffer[2] = OP_R_SENSOR;
            for(i=0;i<8;++i)
            {
                switch(Askbuffer[3] & (1<<i))
                {
                    case PRAM_R_TEMPERATURE:Ackbuffer[3] = pdevblock->Temperature;
						break;

                    case PRAM_R_HUMIDITY   :Ackbuffer[4] = pdevblock->Humidity; 
						break;

                    case PRAM_R_BPM        :{//加入心跳读取
                                                Ackbuffer[5] = pdevblock->Bpm;
                                                Ackbuffer[6] = pdevblock->Bpm;
                                            }                                     
						break;
                    default                :                                    
						break;
                }
            }

            CRC16 = getModbusCRC16(Ackbuffer,7);
            Ackbuffer[7] = CRC16>>8;
            Ackbuffer[8] = CRC16;

        }
		 else 
        {
            /*其他操作码可在此扩充*/
        }
		
        transmitPackets(Ackbuffer,sizeof(Ackbuffer));
        return FRAME_OK;
    }else 
    {
        return FRAME_EMPTY; 
    }  

    return  FRAME_EMPTY;    //不会执行到这里,添加该语句可以避免警告
}
