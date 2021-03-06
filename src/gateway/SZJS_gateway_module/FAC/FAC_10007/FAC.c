#include "fire_alarm.h"
#include "sys_config.h"
//#include <stdio.h>
#include <rtdevice.h>
#include "board.h"

struct rt_device * device_FAC; 


static uint32_t	 addr_station = 0;
static uint32_t  addr_line = 0;
static uint32_t  dev_ID = 0;
static uint8_t	 dev_ID_buf[4] = {0};
static uint8_t	 channel_num = 0;

static uint32_t  cmd_status = 0;
//static uint32_t  dev_status = 0;
static uint8_t *remark = 0;

static uint8_t data_trans[256] = {0};
static uint32_t data_trans_len = 0;

static uint8_t *p_data_meta = NULL;
static uint8_t  data_meta_num = 0;


static uint8_t FAC_head_buf[] = {0xEB,0x00};
static uint8_t FAC_tail_buf[] = {0xEB};


s_com_bus_cfg FAC_com_cfg = {9600, 8, PARITY_NONE, 1};


/*
const s_FAC_config FAC_config = 
    {/////////////////////////////////////////////// FAC_type_JB_TG_JBF_11S
        FAC_type_FC18R,
        "FC18R",
        {9600, 8, PARITY_NONE, 1},
        1000, // alive_period 
        {0, "", 0, 0}, // pre_send_1
        {0, "", 0, 0}, // pre_send_2
        {0, "", 0, "", 0, 0}, // rec_alive_1
        {0, "", 0, "", 0, 0}, // rec_alive_2
        {1, {0xFF ,0xFF ,0xFF ,0x13 ,0x01 ,0x01 ,0x6E ,0x01 ,0x01 ,0x00 ,0x00 ,0x85}, 12, {0xFF ,0xFF ,0xFF ,0x09 ,0x00 ,0x00 ,0x01 ,0x6E ,0x01 ,0x0A ,0x00 ,0x83}, 12, 0}, // send_alive_1
        {0, "", 0, "", 0, 0}, // send_alive_2
        {0, "", 0, "", 0, 0}, // reset_1
        {0, "", 0, "", 0, 0}, // reset_2
        {0, "", 0, "", 0, 0}, // reset_3
        { // alarm
            {1, {0xEB,0x00}, 2}, // head
            {1, {0xEB}, 1}, // tail
            0, 0,  // if_fix_len, fix_len
            { // alarm_alarm
                1,  // if_alarm
                {0x0A}, // buf
                3, // index
                1, // len
                { // sys_addr
                    1, // if_data
                    10, // data_index
                    FAC_data_HEX, // data_type
                    {1,{0}}, // data_HEX: num, index[]
                    {0,{0}}, // data_BCD: num, index[]
                    {0,{0}} // data_ASCII: num, index[]
                },
                { // addr_main
                    1, // if_data 
                    11, // data_index
                    FAC_data_HEX, // data_type
                    {1,{0}}, // data_HEX: num, index[]
                    {0,{0}}, // data_BCD: num, index[]
                    {0,{0}} // data_ASCII: num, index[]
                },
                { // addr_sub
                    1, // if_data
                    12, // data_index
                    FAC_data_HEX, // data_type
                    {1,{0}}, // data_HEX: num, index[]
                    {0,{0}}, // data_BCD: num, index[]
                    {0,{0}} // data_ASCII: num, index[]
                }
            },
            { // alarm_fault
                1,  // if_alarm
                {0x0B}, // buf
                3, // index
                1, // len
                { // sys_addr
                    1, // if_data
                    10, // data_index
                    FAC_data_HEX, // data_type
                    {1,{0}}, // data_HEX: num, index[]
                    {0,{0}}, // data_BCD: num, index[]
                    {0,{0}} // data_ASCII: num, index[]
                },
                { // addr_main
                    1, // if_data
                    11, // data_index
                    FAC_data_HEX, // data_type
                    {1,{0}}, // data_HEX: num, index[]
                    {0,{0}}, // data_BCD: num, index[]
                    {0,{0}} // data_ASCII: num, index[]
                },
                { // addr_sub
                    1, // if_data
                    12, // data_index
                    FAC_data_HEX, // data_type
                    {1,{0}}, // data_HEX: num, index[]
                    {0,{0}}, // data_BCD: num, index[]
                    {0,{0}} // data_ASCII: num, index[]
                }

            }
        }
    };
*/


int FAC_unusual_rx_handler(s_com_bus_cb * cb);

//
//int FAC_test(void)
//{
//	int i = 0;
//	
//    device_FAC = rt_device_find(UITD_UART_COM_BUS);
//    if(device_FAC == RT_NULL)
//    {
//        rt_kprintf("Serial device %s not found!\r\n", UITD_UART_COM_BUS);
//    }
//    else
//    {
//				rt_device_open(device_FAC, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
//    }
//	
//	
//		rt_device_write(device_FAC, 0, FAC_TEST_STRING, sizeof(FAC_TEST_STRING)-1);
//		
//		
//	return 0;	
//}


int FAC_unusual_rx_init(s_com_bus_cb * cb)
{
	
	rt_memcpy(&cb->FAC_config.cfg, &FAC_com_cfg, sizeof(s_com_bus_cfg));
	//cb->FA_listen = 1;
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
		                
		                if (cb->rec_len >= 2)
		                {
		                    if (rt_memcmp(&cb->rec_buf[cb->rec_len-2], 
		                                    FAC_head_buf, 
		                                    sizeof(FAC_head_buf)) == 0)
		                    {
		                        rt_memcpy(cb->parse.buf, FAC_head_buf, sizeof(FAC_head_buf));
		                        cb->parse.len = sizeof(FAC_head_buf);
		                        cb->parse.status = FAC_parse_data;
		                    }
		                    else
		                    {
		                        if (cb->rec_len > COM_BUS_REC_MAX/2)
		                        {
		                            cb->rec_len = 0;
		                        }
		                    }
		                }
		//				if (data_temp == COM_PKT_HEAD_CHAR)
		//				{
		//					cb->parse.status = FAC_parse_head;
		//					cb->parse.len = 0;
		//					cb->parse.valid = 0;
		//				}
		//				else
		//				{
		//					
		//				}
						break;
		//			case FAC_parse_head:
		//				if (data_temp == COM_PKT_HEAD_CHAR)
		//				{
		//					cb->parse.status = FAC_parse_data;
		//				}
		//				else
		//				{
		//					cb->parse.status = FAC_parse_idle;
		//				}
		//				
		//				break;
					case FAC_parse_data:
		                //if (cb->FAC_config.alarm.tail.if_tail)
		                {
		                    cb->parse.buf[cb->parse.len] = data_temp;
		                    cb->parse.len ++;
		                    
		                    if (cb->parse.len >= sizeof(cb->parse.buf))
		                    {
		                        cb->rec_len = 0;
		                        cb->parse.len = 0;
		                        cb->parse.status = FAC_parse_idle;
		                        break;
		                    }
		                    
		                    if (cb->parse.len >= (sizeof(FAC_head_buf) + sizeof(FAC_tail_buf)))
		                    {
		                        if (rt_memcmp(&cb->parse.buf[cb->parse.len - sizeof(FAC_tail_buf)], 
		                                       FAC_tail_buf,
		                                       sizeof(FAC_tail_buf)) == 0)
		                        {
		                            
		                            cb->rec_len = 0;
		                            cb->parse.status = FAC_parse_idle;
		                            cb->parse.valid = 1;
		                            return 0;
		                        }
		                        
		                    }
		                    
		                }
//		                else
//		                {
//		                    if (cb->FAC_config.alarm.if_alarm_fix_len)
//		                    {
//		                        cb->parse.buf[cb->parse.len] = data_temp;
//		                        cb->parse.len ++;
//		                        
//		                        if (cb->parse.len >= cb->FAC_config.alarm.alarm_fix_len)
//		                        {
//		                            cb->rec_len = 0;
//		                            cb->parse.status = FAC_parse_idle;
//		                            cb->parse.valid = 1;
//		                            return 0;
//		                        }
//		                    
//		                    }
//		                
//		                }
		            
		            
		//				if (data_temp == COM_PKT_TAIL_CHAR)
		//				{
		//					cb->parse.status = FAC_parse_tail;
		//					
		//				}
		//				else
		//				{
		//					cb->parse.buf[cb->parse.len] = data_temp;
		//					cb->parse.len ++;
		//					if (cb->parse.len > FAC_PACKET_LEN_MAX)
		//					{
		//						cb->parse.status = FAC_parse_idle;
		//					}
		//				}
						break;
					case FAC_parse_tail:
		//				if (data_temp == COM_PKT_TAIL_CHAR)
		//				{
		//                    // Checking the packet lenght, if too short, discard.
		//                    if (cb->parse.len < FAC_PACKET_LEN_MIN)
		//                    {
		//                        cb->parse.status = FAC_parse_idle;
		//                        break;
		//                    }
		//                    
		//                    // If data body lenght is not equal the actual packet, go on receiving the data.
		//                    ctrl =  (t_COM_ctrl_unit *)cb->parse.buf;
		//                    if (ctrl->data_len != (cb->parse.len - COM_PACKET_NO_DATA_LEN))
		//                    {
		//                        cb->parse.buf[cb->parse.len] = COM_PKT_TAIL_CHAR;
		//                        cb->parse.len ++;
		//                        cb->parse.buf[cb->parse.len] = COM_PKT_TAIL_CHAR;
		//                        cb->parse.len ++;
		//                        if (cb->parse.len > FAC_PACKET_LEN_MAX)
		//                        {
		//                            cb->parse.status = FAC_parse_idle;
		//                            break;
		//                        }
		//                        cb->parse.status = FAC_parse_data;
		//                        break;
		//                    }
		//                    
		//                    // Packet is parsed correctly, exit.
		//					cb->parse.valid = 1;
		//					////rt_kprintf("Parse valid ");
		//					cb->parse.status = FAC_parse_idle;
		//					return 0;
		//				}
		//				else
		//				{
		//					cb->parse.buf[cb->parse.len] = COM_PKT_TAIL_CHAR;
		//					cb->parse.len ++;
		//					cb->parse.buf[cb->parse.len] = data_temp;
		//					cb->parse.len ++;
		//					if (cb->parse.len > FAC_PACKET_LEN_MAX)
		//					{
		//						cb->parse.status = FAC_parse_idle;
		//                        break;
		//					}
		//					cb->parse.status = FAC_parse_data;
		//				}
						
						break;
		            case FAC_parse_end:
		                break;
					default:
						
						break;
				}
        
    
    }
    
    
    
    return -1;		
}



//int FAC_alarm_data_parse(uint8_t *buf, s_FAC_alarm_struct *alarm, s_com_bus_R_alarm *alarm_data)
//{
//    int res = 0;
//    
//    alarm_data->valid = 0;
//    alarm_data->sys_addr = 0;
//    alarm_data->addr_main = 0;
//    alarm_data->addr_sub = 0;
//    
//    res = FAC_alarm_data_element_parse(buf, &alarm->sys_addr, &alarm_data->sys_addr);
//    if (res < 0) return -1;
//    res = FAC_alarm_data_element_parse(buf, &alarm->addr_main, &alarm_data->addr_main);
//    if (res < 0) return -1;
//    res = FAC_alarm_data_element_parse(buf, &alarm->addr_sub, &alarm_data->addr_sub);
//    if (res < 0) return -1;
//    
//    alarm_data->valid = 1;
//    return 0;
//}

uint32_t FC18R_data_trans(uint8_t *src, uint32_t src_len, uint8_t *des)
{
	uint32_t len = 0;
	int i = 0;
	int k = 0;
	
	des[0] = src[0];
	des[1] = src[1];
	 
	k = 1; 
	for (i=2;i<(src_len);)
	{
		if ((src[i] == 0xEC)&&(src[i-1] == 0xEC))	
		{
			des[k] = 0xEB;
			i++;
		}
		else if ((src[i] == 0xED)&&(src[i-1] == 0xEC))
		{
			des[k] = 0xEC;
			i++;
		}
		else
		{
			des[k] = src[i-1];
		}
	  i++;
		k++;
		
	}
	des[k] = src[src_len-1];
	
	len = k+1;
	
/*
	rt_kprintf("trans_data: ");
	for (i=0;i<len;i++)
	{
		rt_kprintf("%02X ", des[i]);	
	}
	rt_kprintf("\n");
*/

	return len;	
}


int FC18R_parse(s_com_bus_cb * cb, uint8_t *data, uint32_t len)
{
		int i = 0;
		//uint32_t data_buf_1 = 0;
		
		addr_station = 0;
		addr_line = 0;
		dev_ID = 0;
		cmd_status = 0;
		
		p_data_meta = &data[4];
		
		data_meta_num = (len - 4 - 3)/19;
		
		for (i=0;i<data_meta_num;i++)
		{
			
			addr_station = 0;
			addr_line = 0;
			dev_ID = 0;
			cmd_status = 0;
			channel_num = 0;
			
			addr_station = p_data_meta[3];	
			addr_line = p_data_meta[4]*256 + p_data_meta[5];
		
			dev_ID = p_data_meta[6];
			dev_ID *= 256;
			dev_ID += p_data_meta[7];
			dev_ID *= 256;
			dev_ID += p_data_meta[8];
			dev_ID *= 256;
			dev_ID += p_data_meta[9];
			
			channel_num = p_data_meta[10];
			
			dev_ID_buf[0] = p_data_meta[9];
			dev_ID_buf[1] = p_data_meta[8];
			dev_ID_buf[2] = p_data_meta[7];
			dev_ID_buf[3] = p_data_meta[6];
			
		
			cmd_status = p_data_meta[11] * 256 + p_data_meta[12];
			
			////remark = &p_data_meta[13];
			
			
			if (dev_ID)
			{
				////rt_kprintf("addr:  %d - %d - 0x%08X\n", addr_station, addr_line, dev_ID);
				////rt_kprintf("status : 0x%04X\n", cmd_status);
				////rt_kprintf("remark : %d-%d-%d %d:%d:%d\n", remark[0], remark[1], remark[2], remark[3], remark[4], remark[5]);
			
			}
			
			
//		  if (cmd_status != 0) // fault_alarm
//		  {
//						cb->alarm_fault.valid = 1;
//						cb->alarm_fault.sys_addr = addr_station;
//						cb->alarm_fault.addr_sub = addr_line%256 + dev_ID_buf[2]*256;
//						cb->alarm_fault.addr_main = dev_ID_buf[0]*256 + dev_ID_buf[1];
//						
//						rt_memcpy(cb->alarm_fault.remark, &data[6], 11);
//						rt_memcpy(&cb->alarm_fault.remark[11], &data[26], 11);
//						
////						rt_memcpy(cb->alarm_fault.remark, remark, 6);
//
////						rt_memcpy(&cb->alarm_fault.remark[6], (uint8_t *)&addr_station, 4);
////						rt_memcpy(&cb->alarm_fault.remark[10], (uint8_t *)&addr_line, 4);
////						rt_memcpy(&cb->alarm_fault.remark[14], (uint8_t *)&dev_ID, 4);
////						rt_memcpy(&cb->alarm_fault.remark[18], (uint8_t *)&cmd_status, 4);
//						
//		  	FA_mq_fault(&cb->alarm_fault, sizeof(s_com_bus_R_alarm));
//		  	FA_mq_fault_2(&cb->alarm_fault, sizeof(s_com_bus_R_alarm));
//		  }			
			
			if (cmd_status == 0) // reset
			{
					// reset alarm
					if ((dev_ID < 100) && (dev_ID > 0))
					{
////						cb->alarm_reset.valid = 1;
						//cb->alarm_reset.sys_addr = addr_station;
						//cb->alarm_reset.addr_sub = dev_ID%100000;
						//cb->alarm_reset.addr_main = addr_line*1000 + dev_ID/100000;
						//rt_memcpy(cb->alarm_reset.remark, remark, 6);
						
////						FA_mq_reset(&cb->alarm_reset, sizeof(s_com_bus_R_alarm));
////						FA_mq_reset_2(&cb->alarm_reset, sizeof(s_com_bus_R_alarm));
					}
					
					
			}
			else if (cmd_status == 1) // fire_alarm
		  {
		  			
						cb->alarm_fire.valid = 1;
						cb->alarm_fire.port = 1; // default is 1.
						cb->alarm_fire.sys_addr = addr_station;
						cb->alarm_fire.addr_sub = addr_line%256 + dev_ID_buf[2]*256;
						cb->alarm_fire.addr_main = dev_ID_buf[0]*256 + dev_ID_buf[1];
						
						fire_alarm_struct_init(&cb->alarm_fire.dev_info);
						cb->alarm_fire.dev_info.port = cb->port;
						cb->alarm_fire.dev_info.controller = addr_station;
						cb->alarm_fire.dev_info.loop = addr_line;
						cb->alarm_fire.dev_info.device_ID = dev_ID;
						cb->alarm_fire.dev_info.device_port = channel_num;
						
						////rt_memcpy(cb->alarm_fire.remark, &data[6], 11);
						////rt_memcpy(&cb->alarm_fire.remark[11], &data[26], 11);
						
//						rt_memcpy(cb->alarm_fire.remark, remark, 6);
						
//						rt_memcpy(&cb->alarm_fire.remark[6], (uint8_t *)&addr_station, 4);
//						rt_memcpy(&cb->alarm_fire.remark[10], (uint8_t *)&addr_line, 4);
//						rt_memcpy(&cb->alarm_fire.remark[14], (uint8_t *)&dev_ID, 4);
//						rt_memcpy(&cb->alarm_fire.remark[18], (uint8_t *)&cmd_status, 4);

		  	FA_mq_fire(&cb->alarm_fire, sizeof(s_com_bus_R_alarm));
		  	FA_mq_fire_2(&cb->alarm_fire, sizeof(s_com_bus_R_alarm));
		  	
		  	SYS_log(SYS_DEBUG_INFO, ("ALARM :  %d - %d - 0x%08X\n", addr_station, addr_line, dev_ID));
		  }
		  else if (cmd_status < 0x0100) // fault_alarm
		  {
						cb->alarm_fault.valid = 1;
						cb->alarm_fault.port = 1; // default is port 1.
						cb->alarm_fault.sys_addr = addr_station;
						cb->alarm_fault.addr_sub = addr_line%256 + dev_ID_buf[2]*256;
						cb->alarm_fault.addr_main = dev_ID_buf[0]*256 + dev_ID_buf[1];

						fire_alarm_struct_init(&cb->alarm_fault.dev_info);
						cb->alarm_fault.dev_info.port = cb->port; 
						cb->alarm_fault.dev_info.controller = addr_station;
						cb->alarm_fault.dev_info.loop = addr_line;
						cb->alarm_fault.dev_info.device_ID = dev_ID;
						cb->alarm_fault.dev_info.device_port = channel_num;

////				rt_kprintf("addr:  %d - %d - 0x%08X\n", addr_station, addr_line, dev_ID);
////				rt_kprintf("status : 0x%04X\n", cmd_status);


////				rt_kprintf("addr:  %d - %04X - %04X - 0x%08X\n", cb->alarm_fault.sys_addr, cb->alarm_fault.addr_sub, cb->alarm_fault.addr_main, dev_ID);
////				rt_kprintf("dev_ID_buf : %d-%d-%d\n", dev_ID_buf[0], dev_ID_buf[1], dev_ID_buf[2]);

						
						////rt_memcpy(cb->alarm_fault.remark, &data[6], 11);
						////rt_memcpy(&cb->alarm_fault.remark[11], &data[26], 11);
						
//						rt_memcpy(cb->alarm_fault.remark, remark, 6);

//						rt_memcpy(&cb->alarm_fault.remark[6], (uint8_t *)&addr_station, 4);
//						rt_memcpy(&cb->alarm_fault.remark[10], (uint8_t *)&addr_line, 4);
//						rt_memcpy(&cb->alarm_fault.remark[14], (uint8_t *)&dev_ID, 4);
//						rt_memcpy(&cb->alarm_fault.remark[18], (uint8_t *)&cmd_status, 4);
						
		  	FA_mq_fault(&cb->alarm_fault, sizeof(s_com_bus_R_alarm));
		  	FA_mq_fault_2(&cb->alarm_fault, sizeof(s_com_bus_R_alarm));
		  	
		  	SYS_log(SYS_DEBUG_INFO, ("FAULT :  %d - %d - 0x%08X\n", addr_station, addr_line, dev_ID));
		  }
			
			p_data_meta += 19;
	  }
		
	return 0;
}



/*
int FC18R_parse(s_com_bus_cb * cb, uint8_t *data, uint32_t len)
{
		int i = 0;
		//uint32_t data_buf_1 = 0;
		
		addr_station = 0;
		addr_line = 0;
		dev_ID = 0;
		
		p_data_meta = &data[4];
		
		data_meta_num = (len - 4 - 3)/19;
		
		//for (i=0;i<data_meta_num;i++)
		{
			addr_station = p_data_meta[3];	
			addr_line = p_data_meta[4]*256 + p_data_meta[5];
		
			dev_ID = p_data_meta[6];
			dev_ID *= 256;
			dev_ID += p_data_meta[7];
			dev_ID *= 256;
			dev_ID += p_data_meta[8];
			dev_ID *= 256;
			dev_ID += p_data_meta[9];
			
			dev_ID_buf[0] = p_data_meta[9];
			dev_ID_buf[1] = p_data_meta[8];
			dev_ID_buf[2] = p_data_meta[7];
			dev_ID_buf[3] = p_data_meta[6];
			
		
			cmd_status = p_data_meta[11] * 256 + p_data_meta[12];
			
			remark = &p_data_meta[13];
			
			if (dev_ID)
			{
				rt_kprintf("addr:  %d - %d - 0x%08X\n", addr_station, addr_line, dev_ID);
				rt_kprintf("status : 0x%04X\n", cmd_status);
				rt_kprintf("remark : %d-%d-%d %d:%d:%d\n", remark[0], remark[1], remark[2], remark[3], remark[4], remark[5]);
			}
			
			
//		  if (cmd_status != 0) // fault_alarm
//		  {
//						cb->alarm_fault.valid = 1;
//						cb->alarm_fault.sys_addr = addr_station;
//						cb->alarm_fault.addr_sub = addr_line%256 + dev_ID_buf[2]*256;
//						cb->alarm_fault.addr_main = dev_ID_buf[0]*256 + dev_ID_buf[1];
//						
//						rt_memcpy(cb->alarm_fault.remark, &data[6], 11);
//						rt_memcpy(&cb->alarm_fault.remark[11], &data[26], 11);
//						
////						rt_memcpy(cb->alarm_fault.remark, remark, 6);
//
////						rt_memcpy(&cb->alarm_fault.remark[6], (uint8_t *)&addr_station, 4);
////						rt_memcpy(&cb->alarm_fault.remark[10], (uint8_t *)&addr_line, 4);
////						rt_memcpy(&cb->alarm_fault.remark[14], (uint8_t *)&dev_ID, 4);
////						rt_memcpy(&cb->alarm_fault.remark[18], (uint8_t *)&cmd_status, 4);
//						
//		  	FA_mq_fault(&cb->alarm_fault, sizeof(s_com_bus_R_alarm));
//		  	FA_mq_fault_2(&cb->alarm_fault, sizeof(s_com_bus_R_alarm));
//		  }			
			
			if (cmd_status != 0) // fault_alarm
		  {
						cb->alarm_fault.valid = 1;
						cb->alarm_fault.sys_addr = addr_station;
						cb->alarm_fault.addr_sub = addr_line%256 + dev_ID_buf[2]*256;
						cb->alarm_fault.addr_main = dev_ID_buf[0]*256 + dev_ID_buf[1];
						
						rt_memcpy(cb->alarm_fault.remark, &data[6], 31);
						//rt_memcpy(&cb->alarm_fault.remark[11], &data[26], 11);
						
//						rt_memcpy(cb->alarm_fault.remark, remark, 6);

//						rt_memcpy(&cb->alarm_fault.remark[6], (uint8_t *)&addr_station, 4);
//						rt_memcpy(&cb->alarm_fault.remark[10], (uint8_t *)&addr_line, 4);
//						rt_memcpy(&cb->alarm_fault.remark[14], (uint8_t *)&dev_ID, 4);
//						rt_memcpy(&cb->alarm_fault.remark[18], (uint8_t *)&cmd_status, 4);
						
		  	FA_mq_fault(&cb->alarm_fault, sizeof(s_com_bus_R_alarm));
		  	FA_mq_fault_2(&cb->alarm_fault, sizeof(s_com_bus_R_alarm));
		  }
			
			p_data_meta += 19;
	  }
		
	return 0;
}
*/

int FAC_unusual_rx_handler(s_com_bus_cb * cb)
{

    int res = 0;
    
    if (cb->parse.valid)
    {
        cb->parse.valid = 0;
        
        data_trans_len = FC18R_data_trans(cb->parse.buf, cb->parse.len, data_trans);
        
        res = FC18R_parse(cb, data_trans, data_trans_len);
        
        
        
//        if ((cb->FAC_config.alarm.alarm.if_alarm) && 
//             (rt_memcmp(&cb->parse.buf[cb->FAC_config.alarm.alarm.index], 
//                        cb->FAC_config.alarm.alarm.buf, 
//                        cb->FAC_config.alarm.alarm.len) == 0))
//        {
//            res = FAC_alarm_data_parse(cb->parse.buf, &cb->FAC_config.alarm.alarm, &cb->alarm_fire);
//            if (res == 0)
//            {
//                rt_kprintf("Alarm_fire parse success: %d, %d, %d\n", 
//                                cb->alarm_fire.sys_addr, 
//                                cb->alarm_fire.addr_main, 
//                                cb->alarm_fire.addr_sub);
//                FA_mq_fire(&cb->alarm_fire, sizeof(s_com_bus_R_alarm));
//                FA_mq_fire_2(&cb->alarm_fire, sizeof(s_com_bus_R_alarm));
//                cb->alarm_fire.valid = 0;
//
//            }
//        }
//        else if ((cb->FAC_config.alarm.fault.if_alarm) && 
//             (rt_memcmp(&cb->parse.buf[cb->FAC_config.alarm.fault.index], 
//                        cb->FAC_config.alarm.fault.buf, 
//                        cb->FAC_config.alarm.fault.len) == 0))
//        {
//            res = FAC_alarm_data_parse(cb->parse.buf, &cb->FAC_config.alarm.fault, &cb->alarm_fault);
//            if (res == 0)
//            {
//                rt_kprintf("Alarm_fault parse success: %d, %d, %d\n", 
//                                cb->alarm_fault.sys_addr, 
//                                cb->alarm_fault.addr_main, 
//                                cb->alarm_fault.addr_sub);
//                FA_mq_fault(&cb->alarm_fault, sizeof(s_com_bus_R_alarm));
//                FA_mq_fault_2(&cb->alarm_fault, sizeof(s_com_bus_R_alarm));
//                cb->alarm_fault.valid = 0;
//            }
//        }
//        else
//        {
//        
//        
//        }
        
        
    }
    
    return 0;
}

int FAC_unusual_server(s_com_bus_cb *cb)
{
//	  // get the g_FA_listen value.
//		FAC_fetch_config(&FA_listen);
		
    if ( (cb->FA_listen == 0) && cb->FAC_config.send_alive_1.if_send_alive)
    {
        if (cb->alive_timer >= cb->alive_period)
        {
            cb->alive_timer = 0;
            
            rt_device_write(cb->dev, 0, cb->FAC_config.send_alive_1.send_alive_buf, cb->FAC_config.send_alive_1.send_alive_len);
            if (cb->FAC_config.send_alive_1.send_alive_delay)
            {
                rt_thread_delay(cb->FAC_config.send_alive_1.send_alive_delay/(1000/RT_TICK_PER_SECOND) + 
                            cb->FAC_config.send_alive_1.send_alive_delay%(1000/RT_TICK_PER_SECOND));
            }
            
            if (cb->FAC_config.send_alive_2.if_send_alive)
            {
                rt_device_write(cb->dev, 0, cb->FAC_config.send_alive_2.send_alive_buf, cb->FAC_config.send_alive_2.send_alive_len);
                if (cb->FAC_config.send_alive_2.send_alive_delay)
                {
                    rt_thread_delay(cb->FAC_config.send_alive_2.send_alive_delay/(1000/RT_TICK_PER_SECOND) + 
                                cb->FAC_config.send_alive_2.send_alive_delay%(1000/RT_TICK_PER_SECOND));
                }
            }
        }
        
    }
    
    

    return 0;
}
