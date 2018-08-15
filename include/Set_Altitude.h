#pragma once
#ifndef _SET_ALLTITUDE_H
#define _SET_ALLTITUDE_H
#include <iostream>
#include "Drone_data.h"
#include "PID.h"

extern msr::airlib::MultirotorRpcLibClient client;
extern struct control_cmd control_cmdset;
extern void set_control_cmd(bool, int, int, float, float, float, float, float, float, float);

//�Զ���ָ��ķ�װ
//target_altitude---Ŀ��߶ȣ�mode---���߷�ʽ��1��ʾ������2��ʾ�ֵ�
void controlAltitude(float target_altitude, int mode)
{
	set_control_cmd(true, 3, mode, .0f, .0f, .0f, .0f, .05f, target_altitude, 0.0f);
}

//����ϸ�ڵ�ʵ��
/*
����ֵ��0---����Ŀ��߶ȣ������˳�    1---û�ж���Ŀ��߶ȣ��쳣�˳�
target_attlitude---Ŀ��߶�
mode---���߷�ʽ��1��ʾ����   2��ʾ�ֵ�
*/
int setAltitude(float target_altitude, int mode)
{
	int result_set_attlitude = 1;
	float allowable_error, pre_error;	//�������ǰһ�ε��������
	float current_error;	//cirrent_error=target_attlitude-current_attlitude
	double kp = 0.1;
	PID PID_temp;

	if (mode == 1)
	{
		allowable_error = 0.2;
	}
	else
	{
		allowable_error = 2.0;
	}

	//���ִ��20�Σ������쳣�˳�
	current_error= target_altitude - Barometer_data.altitude;	//����ǰ�����
	pre_error = current_error;

	while(true)
	{
		if (abs(current_error) < allowable_error)
		{
			result_set_attlitude = 0;
			break;
		}

		//����������PID����
		if (abs(current_error) < (abs(pre_error)))
		{
			kp -= 0.004;
		}

		//���Ƹ߶�
		double throttle_increment = PID_temp.PIDZ(target_altitude, allowable_error, kp);	//�õ���throttle_increment�Ǽ��ٶ�
		//if (throttle_increment > allowable_error*0.1 || throttle_increment < (-allowable_error*0.1))
		//{
			client.moveByAngleThrottle(0.0f, 0.0f, 0.5875 + throttle_increment, 0.0f, 0.5f);
		//}

		pre_error = current_error;
		current_error = target_altitude - Barometer_data.altitude;
	}

	return result_set_attlitude;
}

#endif _SET_ALLTITUDE_H
