// ����JB-QG-FT  -- 232  2400 8N1

#include "fire_alarm.h"
#include "sys_config.h"
//#include <stdio.h>
#include <rtdevice.h>
#include "board.h"

//#define FAC_DBG_EN

#define FAC_MSG_FILETER_EN	0


#define FAC_MSG_BUF_NUM		100

struct rt_device * device_FAC; 


//static uint32_t	 addr_station = 0;
static uint32_t sys_addr = 0;
static uint32_t addr_area = 0;
static uint32_t  addr_line = 0;
static uint32_t  dev_ID = 0;
//static uint8_t	 dev_ID_buf[4] = {0};
//static uint8_t	 channel_num = 0;

//static uint32_t  cmd_status = 0;
//static uint32_t  dev_status = 0;
//static uint8_t *remark = 0;

//static uint8_t data_trans[256] = {0};
//static uint32_t data_trans_len = 0;

//static uint8_t *p_data_meta = NULL;
static uint8_t  data_meta_num = 0;
static uint8_t  CmdLen = 0;


#define FAC_MSG_HEAD1		0xAA
#define FAC_MSG_HEAD2		0x55

#define FAC_MSG_CMD_ALARM			0x01
#define FAC_MSG_CMD_FAULT1			0x07
#define FAC_MSG_CMD_FAULT2			0x06
#define FAC_MSG_CMD_RESET			0x02


#define FAC_MSG_TYPE_NONE  		0
#define FAC_MSG_TYPE_ALARM		1
#define FAC_MSG_TYPE_FAULT		2
#define FAC_MSG_TYPE_RESET		3

#define FAC_MSG_LEN_ALARM			6
#define FAC_MSG_LEN_FAULT			6
#define FAC_MSG_LEN_RESET			1


static uint8_t	FAC_msg_type = FAC_MSG_TYPE_NONE;	
static uint32_t	FAC_msg_len = 0;

//static uint8_t FAC_head_buf[] = {0xEB,0x00};
//static uint8_t FAC_tail_buf[] = {0xEB};


s_com_bus_cfg FAC_com_cfg = {2400, 8, PARITY_NONE, 1};

s_com_bus_R_alarm *FAC_msg_buf = NULL;
int FAC_msg_buf_num = 0;


int FAC_unusual_rx_handler(s_com_bus_cb * cb);


int FAC_unusual_rx_init(s_com_bus_cb * cb)
{
	
	rt_memcpy(&cb->FAC_config.cfg, &FAC_com_cfg, sizeof(s_com_bus_cfg));
	//cb->FA_listen = 1;
	
	#if FAC_MSG_FILETER_EN
	FAC_msg_buf = rt_malloc(sizeof(s_com_bus_R_alarm)*FAC_MSG_BUF_NUM);
	if (FAC_msg_buf == NULL)
	{
		SYS_log(SYS_DEBUG_ERROR, ("FAC_msg_buf malloc failed ! \n"));
		while(1)
		{
			rt_thread_delay(5);	
		}
	}
	#endif // FAC_MSG_FILETER_EN
	
	return 0;
}


int FAC_unusual_rx_parser(s_com_bus_cb * cb)
{
    //uint8_t data = 0x00;
    uint8_t data_temp = 0x00;
    int res = 0;
	
    
    while(1)
    {
        res = rt_device_read(cb->dev, 0, &data_temp, 1);
        if (res < 1)
        {
            return -1;
        }
        cb->rec_buf[cb->rec_len] = data_temp;
        cb->rec_len ++;

		
				switch (cb->parse.status)
				{
					case FAC_parse_idle:
										if (data_temp == FAC_MSG_HEAD1)
										{
											
											cb->parse.status = FAC_parse_head;
										}
										else
										{
											cb->rec_len = 0;
											
										}
										break;
					case FAC_parse_head:
										if (cb->rec_len > 5)
										{
											if (cb->rec_buf[0]==FAC_MSG_HEAD1 && 
												cb->rec_buf[1]==FAC_MSG_HEAD2 )
											{
												rt_memcpy(cb->parse.buf, cb->rec_buf, cb->rec_len);
												cb->parse.len = cb->rec_len;
												
												CmdLen = cb->rec_len + cb->rec_buf[5];
												
												cb->parse.status = FAC_parse_data;
											}
											else
											{
												cb->parse.status = FAC_parse_idle;
												cb->rec_len = 0;
											}
										}

										break;

					case FAC_parse_data:		
										
										cb->parse.buf[cb->parse.len] = data_temp;
										cb->parse.len++;
										
										if (cb->parse.len >= CmdLen)
										{
											cb->parse.status = FAC_parse_idle;
											cb->rec_len = 0;
											
											cb->parse.valid = 1;

											#ifdef FAC_DBG_EN
											rt_kprintf("msg valid \n");
											#endif // FAC_DBG_EN

											return 0;
										}
										
										if (cb->rec_len >= (sizeof(cb->rec_buf) - 2))
										{
											cb->parse.len = 0;
											cb->rec_len = 0;
											cb->parse.status = FAC_parse_idle;
										}
																				
										break;

					case FAC_parse_tail:

						break;
		      case FAC_parse_end:
		      	
		        break;
					default:
						
						break;
				}
				
				
    }
    
    return -1;		
}




// return 1:match the buffer , 0: new msg, not match buffer.
int FAC_msg_buf_match(s_com_bus_R_alarm *buf, s_com_bus_R_alarm *msg)
{
	// if match ,return 0. if not match record the msg, FAC_msg_buf_num ++.
	int i = 0;
	int j = 0;
	uint8_t *p_buf = (uint8_t *)buf;
	uint8_t *p_msg = (uint8_t *)msg;
	
	for (i=0;i<FAC_msg_buf_num;i++)
	{
		p_buf = (uint8_t *)&buf[i];
		for (j=0;j<sizeof(s_com_bus_R_alarm);j++)
		{
			if (p_buf[j] != p_msg[j])	
			break;
		}
		if (j != sizeof(s_com_bus_R_alarm))
		{
			continue;	
		}
		else
		{
			break;	
		}
		
	}
	
	if (i != FAC_msg_buf_num) // matched ,return 1;
	{
		SYS_log(SYS_DEBUG_INFO, ("FAC_msg_buf matched %d / %d  \n", i+1, FAC_msg_buf_num));
		return 1;
	}
	else  // not matched , return 0, copy new msg to buf.
	{
		if (FAC_msg_buf_num >= (FAC_MSG_BUF_NUM-1))
		{
			return 0;
		}
		
		p_buf = (uint8_t *)&buf[FAC_msg_buf_num];
		
		rt_memcpy(p_buf, p_msg, sizeof(s_com_bus_R_alarm));
		
		FAC_msg_buf_num ++;
		
		return 0;
	}
	

}


int FAC_msg_buf_clean(s_com_bus_R_alarm *buf)
{
	
	rt_memset(buf, 0x00, sizeof(s_com_bus_R_alarm)*FAC_MSG_BUF_NUM);
	FAC_msg_buf_num = 0;

	SYS_log(SYS_DEBUG_INFO, ("FAC_msg_buf clean !\n"));
  return 0;
}

int FAC_msg_parse(s_com_bus_cb * cb, uint8_t *data, uint32_t len)
{
		int i = 0;
		uint32_t vTemp = 0;
		uint32_t vNum = 0;
		//uint32_t data_buf_1 = 0;
		
		sys_addr = 0;
		addr_area = 0;
		addr_line = 0;
		dev_ID = 0;
		//cmd_status = 0;
		
		//p_data_meta = &data[12];

		#ifdef FAC_DBG_EN
		rt_kprintf("msg cmd : %d \n", data[10]);
		#endif // FAC_DBG_EN
		
		if (data[6] == FAC_MSG_CMD_RESET)
		{
			FAC_msg_type = FAC_MSG_TYPE_RESET;
			
			cb->alarm_reset.valid = 1;
			cb->alarm_reset.port = cb->port;
			cb->alarm_reset.sys_addr = sys_addr;
			
			// clean the FAC_msg_buf.
			#if FAC_MSG_FILETER_EN
			FAC_msg_buf_clean(FAC_msg_buf);
			#endif // FAC_MSG_FILETER_EN
			
			FA_mq_reset(&cb->alarm_reset, sizeof(s_com_bus_R_reset));
			FA_mq_reset_2(&cb->alarm_reset, sizeof(s_com_bus_R_reset));

		  rt_kprintf("Controller reset : %04X.         RESET .\n\n", cb->alarm_reset.sys_addr);
			
		}
		else if ((data[6] == FAC_MSG_CMD_ALARM) || (data[6] == FAC_MSG_CMD_FAULT1) || (data[6] == FAC_MSG_CMD_FAULT2))
		{
		
			if (data[6] == FAC_MSG_CMD_ALARM)
			{
				FAC_msg_type = FAC_MSG_TYPE_ALARM;
			}
			else if( (data[6] == FAC_MSG_CMD_FAULT1) || (data[6] == FAC_MSG_CMD_FAULT2) )
			{
				FAC_msg_type = FAC_MSG_TYPE_FAULT;
			}
				
			//sys_addr = data[4] + data[5]*0x0100;
			addr_area = data[7];
			addr_line = data[8];
			dev_ID = data[9];
			//cmd_status = 0;
			//channel_num = 0;
			
			vTemp = addr_line*1000 + dev_ID;
			
			vNum += vTemp%10;					//1
			vTemp = vTemp/10;						
			vNum += (vTemp%10)*0x10;			//2
			vTemp = vTemp/10;
			vNum += (vTemp%10)*0x100;			//3
			vTemp = vTemp/10;
			vNum += (vTemp%10)*0x1000;			//4
			vTemp = vTemp/10;
			vNum += (vTemp%10)*0x10000;			//5
			vTemp = vTemp/10;
			vNum += (vTemp%10)*0x100000;		//6
			
			
			
			if (FAC_msg_type == FAC_MSG_TYPE_ALARM)
			{
					cb->alarm_fire.valid = 1;
					cb->alarm_fire.sys_addr  = sys_addr;
					cb->alarm_fire.addr_main = addr_area;
					cb->alarm_fire.addr_sub  = vNum;
					
					fire_alarm_struct_init(&cb->alarm_fire.dev_info);
					cb->alarm_fire.dev_info.port = cb->port;
					//cb->alarm_fire.dev_info.controller = addr_area;
					//cb->alarm_fire.dev_info.loop = addr_line;
					cb->alarm_fire.dev_info.device_ID = vNum;
					
					#if FAC_MSG_FILETER_EN
					if (FAC_msg_buf_match(FAC_msg_buf, &cb->alarm_fire) == 0)
					#endif // FAC_MSG_FILETER_EN
					{
					FA_mq_fire(&cb->alarm_fire, sizeof(s_com_bus_R_alarm));
					FA_mq_fire_2(&cb->alarm_fire, sizeof(s_com_bus_R_alarm));
						rt_kprintf("FireAlarm device  %02X--%02X--%02X--%02X    ALARM .\n\n", sys_addr, addr_area, addr_line, dev_ID );
				}

			}
			else if (FAC_msg_type == FAC_MSG_TYPE_FAULT)
			{
					cb->alarm_fault.valid = 1;
					cb->alarm_fault.sys_addr  = sys_addr;
					cb->alarm_fault.addr_main = addr_area;
					cb->alarm_fault.addr_sub  = vNum;
					
					fire_alarm_struct_init(&cb->alarm_fault.dev_info);
					cb->alarm_fault.dev_info.port = cb->port;
					//cb->alarm_fault.dev_info.controller = addr_area;
					//cb->alarm_fault.dev_info.loop = addr_line;
					
					cb->alarm_fault.dev_info.device_ID = vNum;
					
				
					
					#if FAC_MSG_FILETER_EN
					if (FAC_msg_buf_match(FAC_msg_buf, &cb->alarm_fault) == 0)
					#endif // FAC_MSG_FILETER_EN
					{
					FA_mq_fault(&cb->alarm_fault, sizeof(s_com_bus_R_alarm));
					FA_mq_fault_2(&cb->alarm_fault, sizeof(s_com_bus_R_alarm));
						rt_kprintf("FireAlarm device  %02X--%02X--%02X--%02X    FAULT .\n\n", sys_addr, addr_area, addr_line, dev_ID );
					}				
			}
		}
	return 0;
}


int FAC_unusual_rx_handler(s_com_bus_cb * cb)
{

    int res = 0;
    
	
    if (cb->parse.valid)
    {
        cb->parse.valid = 0;
        
        res = FAC_msg_parse(cb, cb->parse.buf, cb->parse.len);
        
    }

    return 0;
}

int FAC_unusual_server(s_com_bus_cb *cb)
{
		
    return 0;
}



