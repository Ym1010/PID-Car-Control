/*************************  (C) COPYRIGHT 2016  ******************************
 * 文件名  ：ANO_Data_Transfer.cpp
 * 描述    ：数据发送函数
**************************************************************************************/
#include "ANO_Data_Transfer.h"
#include "ANO_Data.h"
#include "ANO_IMU.h"
#include "ANO_RC.h"
#include "ANO_Drv_MPU6050.h"
#include "ANO_Config.h"
#include "ANO_RC.h"
#include "ANO_Param.h"
#include "ANO_CTRL.h"

//数据拆分宏定义，在发送大于1字节的数据类型时，比如int16、float等，需要把数据拆分成单独字节进行发送
#define BYTE0(dwTemp)       ( *( (u8 *)(&dwTemp)		) )
#define BYTE1(dwTemp)       ( *( (u8 *)(&dwTemp) + 1) )
#define BYTE2(dwTemp)       ( *( (u8 *)(&dwTemp) + 2) )
#define BYTE3(dwTemp)       ( *( (u8 *)(&dwTemp) + 3) )

dt_flag_t f;					            //需要发送数据的标志
u8 data_to_send[50];			//发送数据缓存
static void ANO_DT_Send_Msg(u8 id, u8 data);

/*
* Data_Exchange函数处理各种数据发送请求，比如想实现每6ms发送一次传感器数据至上位机，即在此函数内实现(cnt = 6/2 = 3)
* 此函数应由用户每2ms调用一次
*/
void ANO_DT_Data_Exchange(void) //2ms一次
{
	static u8 cnt = 0;
	static u8 senser_cnt 	= 10;
	static u8 senser2_cnt = 50;
	static u8 user_cnt 	  = 10;
	static u8 status_cnt 	= 15;
	static u8 rcdata_cnt 	= 20;
	static u8 motopwm_cnt	= 20;
	static u8 power_cnt		=	50;
	static u8 speed_cnt   = 50;
	static u8 location_cnt   = 200;
	
	// 对所有的发送数据进行计数
	if((cnt % senser_cnt) == (senser_cnt-1))
		f.send_senser = 1;
	if((cnt % senser2_cnt) == (senser2_cnt-1))
		f.send_senser2 = 1;	
	if((cnt % user_cnt) == (user_cnt-2))
		f.send_user = 1;
	if((cnt % status_cnt) == (status_cnt-1))
		f.send_status = 1;	
	if((cnt % rcdata_cnt) == (rcdata_cnt-1))
		f.send_rcdata = 1;	
	if((cnt % motopwm_cnt) == (motopwm_cnt-2))
		f.send_motopwm = 1;	
	if((cnt % power_cnt) == (power_cnt-2))
		f.send_power = 1;		
	if((cnt % speed_cnt) == (speed_cnt-3))
		f.send_speed = 1;		
	if((cnt % location_cnt) == (location_cnt-3))
		f.send_location += 1;
	// 归零
	if(++cnt>200) 
		cnt = 0;
	
	if(f.msg_id)
	{
		ANO_DT_Send_Msg(f.msg_id,f.msg_data);
		f.msg_id = 0;
	}
	else if(f.send_version)
	{
		f.send_version = 0;
		ANO_DT_Send_Version(1,ANO_Param.hardware,ANO_Param.software,400,0);
	}
	else if(f.send_status)
	{
		f.send_status = 0;
		ANO_DT_Send_Status(imu_data.rol,imu_data.pit,imu_data.yaw,0,0,fly_ready);	
	}	
	else if(f.send_speed)
	{
		f.send_speed = 0;
		//ANO_DT_Send_Speed(airframe_x_sp,airframe_y_sp,wz_speed);
	}
	else if(f.send_user)
	{
		f.send_user = 0;
		ANO_DT_Send_User();
	}
	else if(f.send_senser)
	{
		f.send_senser = 0;
		ANO_DT_Send_Senser(sensor.Acc.x,sensor.Acc.y,sensor.Acc.z, sensor.Gyro.x,sensor.Gyro.y,sensor.Gyro.z, 0, 0, 0);
	}	
	else if(f.send_senser2)
	{
		f.send_senser2 = 0;
		//ANO_DT_Send_Senser2(baroAlt,ultra_distance/10);
	}	
	else if(f.send_rcdata)
	{
		f.send_rcdata = 0;
		// 发送遥控器参数
		ANO_DT_Send_RCData(RX_CH[2],RX_CH[3],RX_CH[0],RX_CH[1],RX_CH[4],RX_CH[5],0,0,0,0);
	}	
	else if(f.send_motopwm)
	{
		f.send_motopwm = 0;
		ANO_DT_Send_MotoPWM(motor[0],motor[1],motor[2],motor[3],0,0,0,0);
	}	
	else if(f.send_power)
	{
		f.send_power = 0;
		ANO_DT_Send_Power(123, 456);
	}
	else if(f.send_pid1)
	{
		f.send_pid1 = 0;
		ANO_DT_Send_PID(1,ANO_Param.PID_rol_s.kp,ANO_Param.PID_rol_s.ki,ANO_Param.PID_rol_s.kd,
						  ANO_Param.PID_pit_s.kp,ANO_Param.PID_pit_s.ki,ANO_Param.PID_pit_s.kd,
						  ANO_Param.PID_yaw_s.kp,ANO_Param.PID_yaw_s.ki,ANO_Param.PID_yaw_s.kd);
	}	
	else if(f.send_pid2)
	{
		f.send_pid2 = 0;
		ANO_DT_Send_PID(2,ANO_Param.PID_rol.kp,ANO_Param.PID_rol.ki,ANO_Param.PID_rol.kd,
						  ANO_Param.PID_pit.kp,ANO_Param.PID_pit.ki,ANO_Param.PID_pit.kd,
						  ANO_Param.PID_yaw.kp,ANO_Param.PID_yaw.ki,ANO_Param.PID_yaw.kd);
	}
	else if(f.send_pid3)
	{
		f.send_pid3 = 0;
		ANO_DT_Send_PID(3,ANO_Param.PID_hs.kp,ANO_Param.PID_hs.ki,ANO_Param.PID_hs.kd,
											0,					0,					0,
											0,					0,					0);
	}
	else if(f.send_pid4)
	{
		f.send_pid4 = 0;
		ANO_DT_Send_PID(4,0,0,0,0,0,0,0,0,0);
	}
	else if(f.send_pid5)
	{
		f.send_pid5 = 0;
		ANO_DT_Send_PID(5,0,0,0,0,0,0,0,0,0);
	}
	else if(f.send_pid6)
	{
		f.send_pid6 = 0;
		ANO_DT_Send_PID(6,0,0,0,0,0,0,0,0,0);
	}
	else if(f.send_location == 2)
	{
		//ANO_DT_Send_Location(0,10,test_lon *10000000,test_lat  *10000000,backhome_angle);
	}
	
	// 通过USB发送该数据
	Usb_Hid_Send();					
}

// Send_Data函数是协议中所有发送数据功能使用到的发送函数
// 移植时，用户应根据自身应用的情况，根据使用的通信方式，实现此函数
// 默认都开启了
void ANO_DT_Send_Data(u8 *dataToSend , u8 length)
{
#ifdef ANO_DT_USE_USB_HID
	Usb_Hid_Adddata(dataToSend,length);
#endif
#ifdef ANO_DT_USE_NRF24l01
	ANO_NRF_TxPacket(dataToSend,length);
#endif
#ifdef ANO_DT_USE_WIFI
	ANO_UART3_Put_Buf(data_to_send, length);
#endif
}

static void ANO_DT_Send_Check(u8 head, u8 check_sum)
{
	data_to_send[0]=0xAA;
	data_to_send[1]=0xAA;
	data_to_send[2]=0xEF;
	data_to_send[3]=2;
	data_to_send[4]=head;
	data_to_send[5]=check_sum;
	
	u8 sum = 0;
	for(u8 i = 0;i < 6; i++)
		sum += data_to_send[i];
	data_to_send[6] = sum;

	ANO_DT_Send_Data(data_to_send, 7);
}

static void ANO_DT_Send_Msg(u8 id, u8 data)
{
	data_to_send[0]=0xAA;
	data_to_send[1]=0xAA;
	data_to_send[2]=0xEE;
	data_to_send[3]=2;
	data_to_send[4]=id;
	data_to_send[5]=data;
	
	u8 sum = 0;
	for(u8 i = 0;i < 6; i++)
		sum += data_to_send[i];
	data_to_send[6] = sum;

	ANO_DT_Send_Data(data_to_send, 7);
}

/*
* Data_Receive_Prepare函数是协议预解析，根据协议的格式，将收到的数据进行一次格式性解析，格式正确的话再进行数据解析
* 移植时，此函数应由用户根据自身使用的通信方式自行调用，比如串口每收到一字节数据，则调用此函数一次
* 此函数解析出符合格式的数据帧后，会自行调用数据解析函数
*/
void ANO_DT_Data_Receive_Prepare(u8 data)
{
	static u8 RxBuffer[50];
	static u8 _data_len = 0,_data_cnt = 0;
	static u8 state = 0;
	
	if(state==0&&data==0xAA)
	{
		state=1;
		RxBuffer[0]=data;
	}
	else if(state==1&&data==0xAF)
	{
		state=2;
		RxBuffer[1]=data;
	}
	else if(state==2&&data<0XF1)
	{
		state=3;
		RxBuffer[2]=data;
	}
	else if(state==3&&data<50)
	{
		state = 4;
		RxBuffer[3]=data;
		_data_len = data;
		_data_cnt = 0;
	}
	else if(state==4&&_data_len>0)
	{
		_data_len--;
		RxBuffer[4+_data_cnt++]=data;
		if(_data_len==0)
			state = 5;
	}
	else if(state==5)
	{
		state = 0;
		RxBuffer[4+_data_cnt]=data;
		ANO_DT_Data_Receive_Anl(RxBuffer,_data_cnt+5);
	}
	else
		state = 0;
}

/*
* Data_Receive_Anl函数是协议数据解析函数，函数参数是符合协议格式的一个数据帧，该函数会首先对协议数据进行校验
* 校验通过后对数据进行解析，实现相应功能
* 此函数可以不用用户自行调用，由函数Data_Receive_Prepare自动调用
*/
extern u16 save_pid_en;

void ANO_DT_Data_Receive_Anl(u8 *data_buf,u8 num)
{
	u8 sum = 0;
	for(u8 i = 0;i < (num-1); i++)
		sum += *(data_buf+i);
	if(!(sum == *(data_buf+num-1)))		
		return;		                            //判断sum
	if(!(*(data_buf) == 0xAA && *(data_buf+1) == 0xAF))		
		return;		//判断帧头
	
	if(*(data_buf+2) == 0X01)
	{
		if(*(data_buf+4) == 0X01)
		{
			sensor.acc_CALIBRATE = 1;
			//sensor.Cali_3d = 1;
		}
		else if(*(data_buf+4) == 0X02)
		{
			sensor.gyr_CALIBRATE = 1;
		}
		else if(*(data_buf+4) == 0X03)
		{
			sensor.acc_CALIBRATE = 1;		
			sensor.gyr_CALIBRATE = 1;			
		}
		else if(*(data_buf+4) == 0XA0)
		{
			fly_ready = 0;			
		}
		else if(*(data_buf+4) == 0XA1)
		{
			fly_ready = 1;			
		}
		else;
	}
	
	if(*(data_buf+2) == 0X02)
	{
		if(*(data_buf+4) == 0X01)
		{
			f.send_pid1 = 1;
			f.send_pid2 = 1;
			f.send_pid3 = 1;
			f.send_pid4 = 1;
			f.send_pid5 = 1;
			f.send_pid6 = 1;
		}
		if(*(data_buf+4) == 0X02){ }
		if(*(data_buf+4) == 0XA0)		//读取版本信息
			f.send_version = 1;
		if(*(data_buf+4) == 0XA1)		//恢复默认参数
			ANO_Param_Init();//Para_ResetToFactorySetup();
	}

	if(*(data_buf+2) == 0X03)
	{
		flag.NS = 2;
		
		// 读入遥控器的控制参数
		RX_CH[THR] = (vs16)(*(data_buf+4)<<8)|*(data_buf+5) ;
		RX_CH[YAW] = (vs16)(*(data_buf+6)<<8)|*(data_buf+7) ;
		RX_CH[ROL] = (vs16)(*(data_buf+8)<<8)|*(data_buf+9) ;
		RX_CH[PIT] = (vs16)(*(data_buf+10)<<8)|*(data_buf+11) ;
		RX_CH[AUX1] = (vs16)(*(data_buf+12)<<8)|*(data_buf+13) ;
		RX_CH[AUX2] = (vs16)(*(data_buf+14)<<8)|*(data_buf+15) ;
		RX_CH[AUX3] = (vs16)(*(data_buf+16)<<8)|*(data_buf+17) ;
		RX_CH[AUX4] = (vs16)(*(data_buf+18)<<8)|*(data_buf+19) ;
	}
	
	// 读入PID控制参数
	if(*(data_buf+2) == 0X10)								//PID1
    {
        ANO_Param.PID_rol_s.kp = ( (vs16)(*(data_buf+4)<<8)|*(data_buf+5) );
        ANO_Param.PID_rol_s.ki = ( (vs16)(*(data_buf+6)<<8)|*(data_buf+7) );
        ANO_Param.PID_rol_s.kd = ( (vs16)(*(data_buf+8)<<8)|*(data_buf+9) );
        ANO_Param.PID_pit_s.kp = ( (vs16)(*(data_buf+10)<<8)|*(data_buf+11) );
        ANO_Param.PID_pit_s.ki = ( (vs16)(*(data_buf+12)<<8)|*(data_buf+13) );
        ANO_Param.PID_pit_s.kd = ( (vs16)(*(data_buf+14)<<8)|*(data_buf+15) );
        ANO_Param.PID_yaw_s.kp 	= ( (vs16)(*(data_buf+16)<<8)|*(data_buf+17) );
        ANO_Param.PID_yaw_s.ki 	= ( (vs16)(*(data_buf+18)<<8)|*(data_buf+19) );
        ANO_Param.PID_yaw_s.kd 	= ( (vs16)(*(data_buf+20)<<8)|*(data_buf+21) );
		
        ANO_DT_Send_Check(*(data_buf+2),sum);
				//Param_SavePID(); // 保存PID参数
				save_pid_en = 1;
    }
    if(*(data_buf+2)==0X11)								//PID2
    {
        ANO_Param.PID_rol.kp = ( (vs16)(*(data_buf+4)<<8)|*(data_buf+5) );
        ANO_Param.PID_rol.ki = ( (vs16)(*(data_buf+6)<<8)|*(data_buf+7) );
        ANO_Param.PID_rol.kd = ( (vs16)(*(data_buf+8)<<8)|*(data_buf+9) );
        ANO_Param.PID_pit.kp = ( (vs16)(*(data_buf+10)<<8)|*(data_buf+11) );
        ANO_Param.PID_pit.ki = ( (vs16)(*(data_buf+12)<<8)|*(data_buf+13) );
        ANO_Param.PID_pit.kd = ( (vs16)(*(data_buf+14)<<8)|*(data_buf+15) );
        ANO_Param.PID_yaw.kp 	= ( (vs16)(*(data_buf+16)<<8)|*(data_buf+17) );
        ANO_Param.PID_yaw.ki 	= ( (vs16)(*(data_buf+18)<<8)|*(data_buf+19) );
        ANO_Param.PID_yaw.kd 	= ( (vs16)(*(data_buf+20)<<8)|*(data_buf+21) );
		
        ANO_DT_Send_Check(*(data_buf+2),sum);
				save_pid_en = 1;
    }
    if(*(data_buf+2)==0X12)								//PID3
    {	
        ANO_Param.PID_hs.kp  = ( (vs16)(*(data_buf+4)<<8)|*(data_buf+5) );
        ANO_Param.PID_hs.ki  = ( (vs16)(*(data_buf+6)<<8)|*(data_buf+7) );
        ANO_Param.PID_hs.kd  = ( (vs16)(*(data_buf+8)<<8)|*(data_buf+9) );
//			
//        pid_setup.groups.hc_height.kp = 0.001*( (vs16)(*(data_buf+10)<<8)|*(data_buf+11) );
//        pid_setup.groups.hc_height.ki = 0.001*( (vs16)(*(data_buf+12)<<8)|*(data_buf+13) );
//        pid_setup.groups.hc_height.kd = 0.001*( (vs16)(*(data_buf+14)<<8)|*(data_buf+15) );
//			
//        pid_setup.groups.ctrl3.kp 	= 0.001*( (vs16)(*(data_buf+16)<<8)|*(data_buf+17) );
//        pid_setup.groups.ctrl3.ki 	= 0.001*( (vs16)(*(data_buf+18)<<8)|*(data_buf+19) );
//        pid_setup.groups.ctrl3.kd 	= 0.001*( (vs16)(*(data_buf+20)<<8)|*(data_buf+21) );
        ANO_DT_Send_Check(*(data_buf+2),sum);
				save_pid_en = 1;
    }
	if(*(data_buf+2)==0X13)								//PID4
	{
//		    pid_setup.groups.ctrl4.kp  = 0.001*( (vs16)(*(data_buf+4)<<8)|*(data_buf+5) );
//        pid_setup.groups.ctrl4.ki  = 0.001*( (vs16)(*(data_buf+6)<<8)|*(data_buf+7) );
//        pid_setup.groups.ctrl4.kd  = 0.001*( (vs16)(*(data_buf+8)<<8)|*(data_buf+9) );
			
//         pid_setup.groups.hc_height.kp = 0.001*( (vs16)(*(data_buf+10)<<8)|*(data_buf+11) );
//         pid_setup.groups.hc_height.ki = 0.001*( (vs16)(*(data_buf+12)<<8)|*(data_buf+13) );
//         pid_setup.groups.hc_height.kd = 0.001*( (vs16)(*(data_buf+14)<<8)|*(data_buf+15) );
// 			
//         pid_setup.groups.ctrl3.kp 	= 0.001*( (vs16)(*(data_buf+16)<<8)|*(data_buf+17) );
//         pid_setup.groups.ctrl3.ki 	= 0.001*( (vs16)(*(data_buf+18)<<8)|*(data_buf+19) );
//         pid_setup.groups.ctrl3.kd 	= 0.001*( (vs16)(*(data_buf+20)<<8)|*(data_buf+21) );
				ANO_DT_Send_Check(*(data_buf+2),sum);
				save_pid_en = 1;
	}
	if(*(data_buf+2)==0X14)								//PID5
	{
		ANO_DT_Send_Check(*(data_buf+2),sum);
	}
	if(*(data_buf+2)==0X15)								//PID6
	{
		ANO_DT_Send_Check(*(data_buf+2),sum);
	}
}

void ANO_DT_Send_Version(u8 hardware_type, u16 hardware_ver,u16 software_ver,u16 protocol_ver,u16 bootloader_ver)
{
	u8 _cnt=0;
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0x00;
	data_to_send[_cnt++]=0;
	
	data_to_send[_cnt++]=hardware_type;
	data_to_send[_cnt++]=BYTE1(hardware_ver);
	data_to_send[_cnt++]=BYTE0(hardware_ver);
	data_to_send[_cnt++]=BYTE1(software_ver);
	data_to_send[_cnt++]=BYTE0(software_ver);
	data_to_send[_cnt++]=BYTE1(protocol_ver);
	data_to_send[_cnt++]=BYTE0(protocol_ver);
	data_to_send[_cnt++]=BYTE1(bootloader_ver);
	data_to_send[_cnt++]=BYTE0(bootloader_ver);
	
	data_to_send[3] = _cnt-4;
	
	u8 sum = 0;
	for(u8 i=0;i<_cnt;i++)
		sum += data_to_send[i];
	data_to_send[_cnt++]=sum;
	
	ANO_DT_Send_Data(data_to_send, _cnt);
}

void ANO_DT_Send_Speed(float x_s,float y_s,float z_s)
{
	u8 _cnt=0;
	vs16 _temp;
	
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0x0B;
	data_to_send[_cnt++]=0;
	
	_temp = (int)(0.1f *x_s);
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);
	_temp = (int)(0.1f *y_s);
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);
	_temp = (int)(0.1f *z_s);
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);
	
	
	data_to_send[3] = _cnt-4;
	
	u8 sum = 0;
	for(u8 i=0;i<_cnt;i++)
		sum += data_to_send[i];
	data_to_send[_cnt++]=sum;
	
	ANO_DT_Send_Data(data_to_send, _cnt);
}

void ANO_DT_Send_Location(u8 state,u8 sat_num,s32 lon,s32 lat,float back_home_angle)
{
	u8 _cnt=0;
	vs16 _temp;
	vs32 _temp2;
	
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0x04;
	data_to_send[_cnt++]=0;
	
	data_to_send[_cnt++]=state;
	data_to_send[_cnt++]=sat_num;
	
	_temp2 = lon;//经度
	data_to_send[_cnt++]=BYTE3(_temp2);
	data_to_send[_cnt++]=BYTE2(_temp2);	
	data_to_send[_cnt++]=BYTE1(_temp2);
	data_to_send[_cnt++]=BYTE0(_temp2);
	
	_temp2 = lat;//纬度
	data_to_send[_cnt++]=BYTE3(_temp2);
	data_to_send[_cnt++]=BYTE2(_temp2);	
	data_to_send[_cnt++]=BYTE1(_temp2);
	data_to_send[_cnt++]=BYTE0(_temp2);
	
	
	_temp = (s16)(100 *back_home_angle);
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);
	
	
	data_to_send[3] = _cnt-4;
	
	u8 sum = 0;
	for(u8 i = 0;i < _cnt;i++)
		sum += data_to_send[i];
	data_to_send[_cnt++] = sum;
	
	ANO_DT_Send_Data(data_to_send, _cnt);
}

void ANO_DT_Send_Status(float angle_rol, float angle_pit, float angle_yaw, s32 alt, u8 fly_model, u8 armed)
{
	u8 _cnt = 0;
	vs16 _temp;
	vs32 _temp2 = alt;
	
	data_to_send[_cnt++] = 0xAA;
	data_to_send[_cnt++] = 0xAA;
	data_to_send[_cnt++] = 0x01;
	data_to_send[_cnt++] = 0;
	
	_temp = (int)(angle_rol*100);
	data_to_send[_cnt++] = BYTE1(_temp);
	data_to_send[_cnt++] = BYTE0(_temp);
	_temp = (int)(angle_pit*100);
	data_to_send[_cnt++] = BYTE1(_temp);
	data_to_send[_cnt++] = BYTE0(_temp);
	_temp = (int)(angle_yaw*100);
	data_to_send[_cnt++] = BYTE1(_temp);
	data_to_send[_cnt++] = BYTE0(_temp);
	
	data_to_send[_cnt++] = BYTE3(_temp2);
	data_to_send[_cnt++] = BYTE2(_temp2);
	data_to_send[_cnt++] = BYTE1(_temp2);
	data_to_send[_cnt++] = BYTE0(_temp2);
	
	data_to_send[_cnt++] = fly_model;
	
	data_to_send[_cnt++] = armed;
	
	data_to_send[3] = _cnt-4;
	
	u8 sum = 0;
	for(u8 i=0;i<_cnt;i++)
		sum += data_to_send[i];
	data_to_send[_cnt++]=sum;
	
	ANO_DT_Send_Data(data_to_send, _cnt);
}
void ANO_DT_Send_Senser(s16 a_x,s16 a_y,s16 a_z,s16 g_x,s16 g_y,s16 g_z,s16 m_x,s16 m_y,s16 m_z)
{
	u8 _cnt=0;
	vs16 _temp;
	
	data_to_send[_cnt++] = 0xAA;
	data_to_send[_cnt++] = 0xAA;
	data_to_send[_cnt++] = 0x02;
	data_to_send[_cnt++] = 0;
	
	_temp = a_x;
	data_to_send[_cnt++] = BYTE1(_temp);
	data_to_send[_cnt++] = BYTE0(_temp);
	_temp = a_y;
	data_to_send[_cnt++] = BYTE1(_temp);
	data_to_send[_cnt++] = BYTE0(_temp);
	_temp = a_z;	
	data_to_send[_cnt++] = BYTE1(_temp);
	data_to_send[_cnt++] = BYTE0(_temp);
	
	_temp = g_x;	
	data_to_send[_cnt++] = BYTE1(_temp);
	data_to_send[_cnt++] = BYTE0(_temp);
	_temp = g_y;	
	data_to_send[_cnt++] = BYTE1(_temp);
	data_to_send[_cnt++] = BYTE0(_temp);
	_temp = g_z;	
	data_to_send[_cnt++] = BYTE1(_temp);
	data_to_send[_cnt++] = BYTE0(_temp);
	
	_temp = m_x;	
	data_to_send[_cnt++] = BYTE1(_temp);
	data_to_send[_cnt++] = BYTE0(_temp);
	_temp = m_y;	
	data_to_send[_cnt++] = BYTE1(_temp);
	data_to_send[_cnt++] = BYTE0(_temp);
	_temp = m_z;	
	data_to_send[_cnt++] = BYTE1(_temp);
	data_to_send[_cnt++] = BYTE0(_temp);
/////////////////////////////////////////
	_temp = 0;	
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);	
	
	data_to_send[3] = _cnt-4;
	
	u8 sum = 0;
	for(u8 i=0;i<_cnt;i++)
		sum += data_to_send[i];
	data_to_send[_cnt++] = sum;
	
	ANO_DT_Send_Data(data_to_send, _cnt);
}

void ANO_DT_Send_Senser2(s32 bar_alt,u16 csb_alt)
{
	u8 _cnt=0;
	
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0x07;
	data_to_send[_cnt++]=0;
	
	data_to_send[_cnt++]=BYTE3(bar_alt);
	data_to_send[_cnt++]=BYTE2(bar_alt);
	data_to_send[_cnt++]=BYTE1(bar_alt);
	data_to_send[_cnt++]=BYTE0(bar_alt);

	data_to_send[_cnt++]=BYTE1(csb_alt);
	data_to_send[_cnt++]=BYTE0(csb_alt);
	
	data_to_send[3] = _cnt-4;
	
	u8 sum = 0;
	for(u8 i=0;i<_cnt;i++)
		sum += data_to_send[i];
	data_to_send[_cnt++] = sum;
	
	ANO_DT_Send_Data(data_to_send, _cnt);
}
void ANO_DT_Send_RCData(u16 thr,u16 yaw,u16 rol,u16 pit,u16 aux1,u16 aux2,u16 aux3,u16 aux4,u16 aux5,u16 aux6)
{
	u8 _cnt=0;
	
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0x03;
	data_to_send[_cnt++]=0;
	data_to_send[_cnt++]=BYTE1(thr);
	data_to_send[_cnt++]=BYTE0(thr);
	data_to_send[_cnt++]=BYTE1(yaw);
	data_to_send[_cnt++]=BYTE0(yaw);
	data_to_send[_cnt++]=BYTE1(rol);
	data_to_send[_cnt++]=BYTE0(rol);
	data_to_send[_cnt++]=BYTE1(pit);
	data_to_send[_cnt++]=BYTE0(pit);
	data_to_send[_cnt++]=BYTE1(aux1);
	data_to_send[_cnt++]=BYTE0(aux1);
	data_to_send[_cnt++]=BYTE1(aux2);
	data_to_send[_cnt++]=BYTE0(aux2);
	data_to_send[_cnt++]=BYTE1(aux3);
	data_to_send[_cnt++]=BYTE0(aux3);
	data_to_send[_cnt++]=BYTE1(aux4);
	data_to_send[_cnt++]=BYTE0(aux4);
	data_to_send[_cnt++]=BYTE1(aux5);
	data_to_send[_cnt++]=BYTE0(aux5);
	data_to_send[_cnt++]=BYTE1(aux6);
	data_to_send[_cnt++]=BYTE0(aux6);

	data_to_send[3] = _cnt-4;
	
	u8 sum = 0;
	for(u8 i=0;i<_cnt;i++)
		sum += data_to_send[i];
	
	data_to_send[_cnt++]=sum;
	
	ANO_DT_Send_Data(data_to_send, _cnt);
}

void ANO_DT_Send_Power(u16 votage, u16 current)
{
	u8 _cnt=0;
	u16 temp;
	
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0x05;
	data_to_send[_cnt++]=0;
	
	temp = votage;
	data_to_send[_cnt++]=BYTE1(temp);
	data_to_send[_cnt++]=BYTE0(temp);
	temp = current;
	data_to_send[_cnt++]=BYTE1(temp);
	data_to_send[_cnt++]=BYTE0(temp);
	
	data_to_send[3] = _cnt-4;
	
	u8 sum = 0;
	for(u8 i=0;i<_cnt;i++)
		sum += data_to_send[i];
	
	data_to_send[_cnt++]=sum;
	
	ANO_DT_Send_Data(data_to_send, _cnt);
}

void ANO_DT_Send_MotoPWM(u16 m_1,u16 m_2,u16 m_3,u16 m_4,u16 m_5,u16 m_6,u16 m_7,u16 m_8)
{
	u8 _cnt=0;
	
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0x06;
	data_to_send[_cnt++]=0;
	
	data_to_send[_cnt++]=BYTE1(m_1);
	data_to_send[_cnt++]=BYTE0(m_1);
	data_to_send[_cnt++]=BYTE1(m_2);
	data_to_send[_cnt++]=BYTE0(m_2);
	data_to_send[_cnt++]=BYTE1(m_3);
	data_to_send[_cnt++]=BYTE0(m_3);
	data_to_send[_cnt++]=BYTE1(m_4);
	data_to_send[_cnt++]=BYTE0(m_4);
	data_to_send[_cnt++]=BYTE1(m_5);
	data_to_send[_cnt++]=BYTE0(m_5);
	data_to_send[_cnt++]=BYTE1(m_6);
	data_to_send[_cnt++]=BYTE0(m_6);
	data_to_send[_cnt++]=BYTE1(m_7);
	data_to_send[_cnt++]=BYTE0(m_7);
	data_to_send[_cnt++]=BYTE1(m_8);
	data_to_send[_cnt++]=BYTE0(m_8);
	
	data_to_send[3] = _cnt-4;
	
	u8 sum = 0;
	for(u8 i=0;i<_cnt;i++)
		sum += data_to_send[i];
	
	data_to_send[_cnt++]=sum;
	
	ANO_DT_Send_Data(data_to_send, _cnt);
}
void ANO_DT_Send_PID(u8 group,s16 p1_p,s16 p1_i,s16 p1_d,s16 p2_p,s16 p2_i,s16 p2_d,s16 p3_p,s16 p3_i,s16 p3_d)
{
	u8 _cnt=0;
	vs16 _temp;
	
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0x10+group-1;
	data_to_send[_cnt++]=0;
	
	
	_temp = p1_p ;
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);
	_temp = p1_i ;
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);
	_temp = p1_d ;
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);
	_temp = p2_p ;
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);
	_temp = p2_i ;
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);
	_temp = p2_d ;
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);
	_temp = p3_p ;
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);
	_temp = p3_i ;
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);
	_temp = p3_d ;
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);
	
	data_to_send[3] = _cnt-4;
	
	u8 sum = 0;
	for(u8 i=0;i<_cnt;i++)
		sum += data_to_send[i];
	
	data_to_send[_cnt++]=sum;

	ANO_DT_Send_Data(data_to_send, _cnt);
}

#include "ANO_CTRL.h"
extern PID_val_t val_1_rol;

void ANO_DT_Send_User()
{
	u8 _cnt=0;
	vs16 _temp;
	
	data_to_send[_cnt++]=0xAA; 
	data_to_send[_cnt++]=0xAA;
	data_to_send[_cnt++]=0xf1; //用户数据
	data_to_send[_cnt++]=0;
	
	
	_temp = (s16)ctrl_1.exp_rol;           //1
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);

	_temp = (s16)ctrl_1.fb_rol;            //2
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);	
	
	_temp = (s16)val_1_rol.err_i;          //3
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);	

	_temp = (s16)ctrl_1.out_rol;          //4
	data_to_send[_cnt++]=BYTE1(_temp);
	data_to_send[_cnt++]=BYTE0(_temp);	
	
	data_to_send[3] = _cnt-4;
	
	u8 sum = 0;
	for(u8 i = 0;i < _cnt;i++)
		sum += data_to_send[i];
	
	data_to_send[_cnt++] = sum;

	ANO_DT_Send_Data(data_to_send, _cnt);
}

/******************* (C) COPYRIGHT 2016 ANO TECH *****END OF FILE************/

