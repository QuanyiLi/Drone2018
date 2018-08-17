#include "PID.h"
#include "Drone_data.h"

//����Ҫʹ�õ�ȫ�ֱ���
extern msr::airlib::MultirotorRpcLibClient client;

//��ͬ�߶�PID�õĲ�һ�����ڱȽϸߵĵط����εõ��Ĳ������ڵ͵�ʹ��ʱ������
//��ʹ������ڶ�����̫���ڵ͵���ڵõ��Ĳ������ڸߵ���ʹ��ûʲô���⣬����
//����һ�㡣
//double kp = -0.01, ki = 0.0, kd = 0.0;// -445�߶��µ�OK��-450�߶�ʱ�������������̫���ƫ��
//double kp = -0.01, ki = 0.0, kd = 0.0;//OK

double kp = -0.002, ki = -0.0, kd = 0.00001;//-450�߶��µġ�OK -0.00002;
double vkp = -0.01, vki = 0, vkd = 0;

double PID::PIDY(double error, double y_max, double tolerance)
{
	double control_pitch;

	if (error < -tolerance || error > tolerance)
	{
		control_pitch = pid_y.getOutput(error, 0.3);//300ms
		CLIP3(-y_max, control_pitch, y_max);
	}
	else
	{
		control_pitch = 0.0;
	}

	printf("control_pitch;%f\n", control_pitch);
	return control_pitch;
}

double PID::PIDX(double error, double x_max, double tolerance) {
	double control_roll;

	if (error < -tolerance || error > tolerance)
	{
		control_roll = pid_x.getOutput(error, 0.3);//300ms
		CLIP3(-x_max, control_roll, x_max);
	}
	else
	{
		control_roll = 0.0;
	}
	
	printf("control_roll;%f\n", control_roll);
	return control_roll;
}

double PID::PIDXY(double error, double v_max, bool is_X) {
	double targetv, control;

	return control;
}

double PID::PIDZ(double reference, double tolerance,double kp) {
  double upd, control_stuff;
  //double kp =  0.1;

  ////��GPS���ݶ���
  //control_stuff = GPS_data.altitude;

  //����ѹ�ƶ���
  //control_stuff = Barometer_data.altitude;
  control_stuff = client.getBarometerdata().altitude;

  if (control_stuff < reference) {
    upd = kp * (reference - control_stuff);
  } else if (control_stuff > reference + tolerance) {
    upd = kp * (reference + tolerance - control_stuff);
  } else {
    upd = 0;
  }
  CLIP3(-0.3f, upd, 0.3f);
  return upd;
}

void PID::PIDReset() {

	pid_vx.reset();
	pid_vy.reset();
	pid_x.reset();
	pid_y.reset();

	pid_vx.setParam(vkp, vki, vkd, 2);
	pid_vy.setParam(vkp, vki, vkd, 2);

	pid_x.setParam(kp, ki, kd, 2);
	pid_y.setParam(kp, ki, kd, 2);
}

double PID::PIDdis(double reference, double tolerance, uint16_t distance )//referenceΪ��ǰ��Ŀ��ľ��룬toleranceΪ������distanceΪ��ǰ���ͼ����
{
	double fb, control_stuff;
	//double kp = 1.453e-5;
	double kp = 3.453e-5;
	control_stuff = distance;
	if (distance > 50000)
		return -0.25;//�������̫Զ����һ���̶��Ƕ���ǰ��

	if (control_stuff < reference) {
		fb = kp * (reference - control_stuff);
	}
	else if (control_stuff > reference + tolerance) {
		fb = kp * (reference + tolerance - control_stuff);
	}
	else {
		fb = 0;
	}
	//CLIP3(-0.436, fb, 0.436);
	CLIP3(-0.2, fb, 0.2);
	return fb;
}


//GPS
//double kp_g = -1282.2, ki_g = -0.0, kd_g = 0.0;//
//double kp_g = -1000, ki_g = -0.0, kd_g = -350.0;//50ms����
//double kp_g = -500, ki_g = -0.0, kd_g = -350.0;//�ֵ�
//double kp_g = -150, ki_g = -0.0, kd_g = -250.0;//�����ã�������
double kp_g = -150, ki_g = -0.0, kd_g = -250;//�����ã�������
double vkp_g = -0.01, vki_g = 0, vkd_g = 0;
void PID_GPS::PIDReset()
{
	pid_vx.reset();
	pid_vy.reset();
	pid_x.reset();
	pid_y.reset();

	pid_vx.setParam(vkp_g, vki_g, vkd_g, 2);
	pid_vy.setParam(vkp_g, vki_g, vkd_g, 2);

	pid_x.setParam(kp_g, ki_g, kd_g, 2);
	pid_y.setParam(kp_g, ki_g, kd_g, 2);
}

double PID_GPS::PIDX(double error, double x_max, double tolerance)
{
	double control_roll;

	if (error < -tolerance || error > tolerance)
	{
		control_roll = pid_x.getOutput(error, 0.300);//300ms
		CLIP3(-x_max, control_roll, x_max);
	}
	else
	{
		control_roll = 0.0;
	}

	//printf("control_roll;%f\n", control_roll);
	return control_roll;
}

double PID_GPS::PIDY(double error, double y_max, double tolerance)
{
	double control_pitch;

	if (error < -tolerance || error > tolerance)
	{
		control_pitch = pid_y.getOutput(error, 0.300);//300ms
		CLIP3(-y_max, control_pitch, y_max);
	}
	else
	{
		control_pitch = 0.0;
	}

	//printf("control_pitch;%f\n", control_pitch);
	return control_pitch;
}

//С����PID
double kp_t = -0.002, ki_t = -0.0, kd_t = 0.00001;//
double vkp_t, vki_t, vkd_t;

double PID_TREE::PIDX(double error, double x_max, double tolerance)
{
	double control_roll;

	if (error < -tolerance || error > tolerance)
	{
		control_roll = pid_x.getOutput(error, 0.300);//300ms
		CLIP3(-x_max, control_roll, x_max);
	}
	else
	{
		control_roll = 0.0;
	}

	//printf("control_roll;%f\n", control_roll);
	return control_roll;
}
void PID_TREE::PIDReset()
{

	pid_vx.reset();
	pid_vy.reset();
	pid_x.reset();
	pid_y.reset();

	pid_vx.setParam(vkp_t, vki_t, vkd_t, 2);
	pid_vy.setParam(vkp_t, vki_t, vkd_t, 2);

	pid_x.setParam(kp_t, ki_t, kd_t, 2);
	pid_y.setParam(kp_t, ki_t, kd_t, 2);

}

