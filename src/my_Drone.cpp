#include "main.hpp"
#include <string>
//airsim
#include "vehicles/multirotor/api/MultirotorRpcLibClient.hpp"
#include "api/RpcLibClientBase.hpp"
//opencv
#include "opencv2/opencv.hpp"

#include <iostream>
#include "WINDOWS.h"

#include "timer.h" 
#include <thread>
#include "getImg.h" //���ͼ����
#include "Set_Altitude.h"
#include "rotate.h"


//�����ռ�
using namespace std;
using namespace cv;
using namespace msr::airlib;

typedef ImageCaptureBase::ImageRequest ImageRequest;	//ͼ������
typedef ImageCaptureBase::ImageResponse ImageResponse;	//ͼ����Ӧ
typedef ImageCaptureBase::ImageType ImageType;			//ͼ������

// V ������� ****************************************
//airsim ���
	msr::airlib::MultirotorRpcLibClient client("localhost", 41451, 60000);//����localhost:41451 	
//����������
//GpsData			 GPS_data;			//GPS
BarometerData	 Barometer_data;	//��ѹ��
MagnetometerData Magnetometer_data;	//������
ImuData			 Imu_data;			//IMU
std::mutex g_mutex_img;//������
std::mutex g_mutex_sensor;//���������ݻ�������������ѹ�ơ������ơ�IMU����
std::mutex g_mutex_control_cmd;//��������������
int avoid_obstacle_flag = 0; //���ϱ�־

int flag_mode = 1;//  1��ʾ����1��Ȧ��2��ʾ����2С����

//ͼ��
cv::Mat front_image, down_image, depth_image;

// A ������� ****************************************//

//llf
int num = 2143;
std::string pathName;
int step = 1, back_step = 1, back_border=0;
int interval_num = 0;

// V ������ ****************************************



// A ������ ****************************************//

//�߳�
//�����ɵȺ�ʱ��
HANDLE hTimer_Key_Scan = CreateWaitableTimer(NULL, FALSE, NULL);
HANDLE hTimer_get_img = CreateWaitableTimer(NULL, FALSE, NULL);
HANDLE hTimer_get_sensor = CreateWaitableTimer(NULL, FALSE, NULL);
HANDLE hTimer_move_control = CreateWaitableTimer(NULL, FALSE, NULL);
//�̺߳�������
void Key_Scan(void);
void get_img(void);
void get_sensor(void);//��ȡ����������
void move_control(void);//�ƶ������߳�


static int key_control(int key);//�������ƺ�������

//ȫ�ֱ�־λ
static bool flag_exit = 0;//���δtrue���ʾ�Ƴ�����

int main()
{
	while (RpcLibClientBase::ConnectionState::Connected != client.getConnectionState())
		client.confirmConnection();//���ӳ���

	while (!client.isApiControlEnabled())
		client.enableApiControl(true);//��ȡapi����

	client.armDisarm(true);//�����ɿ�	  
	client.hover();//hoverģʽ
	
	//���ö�ʱ��ʱ��
	INT64 nDueTime = -0 * _SECOND;//��ʱ����Чʱ�䣬����
	SetWaitableTimer(hTimer_Key_Scan, (PLARGE_INTEGER)&nDueTime, 50, NULL, NULL, FALSE);//50��ʾ��ʱ������50ms
	SetWaitableTimer(hTimer_get_img, (PLARGE_INTEGER)&nDueTime, 400, NULL, NULL, FALSE);//50��ʾ��ʱ������50ms
	SetWaitableTimer(hTimer_get_sensor, (PLARGE_INTEGER)&nDueTime, 50, NULL, NULL, FALSE);//50��ʾ��ʱ������50ms
	SetWaitableTimer(hTimer_move_control, (PLARGE_INTEGER)&nDueTime, 50, NULL, NULL, FALSE);//50��ʾ��ʱ������50ms
	
	printf("�̳߳�ʼ��\n");
	std::thread t_Key_Scan(Key_Scan);
	std::thread t_get_img(get_img);
	std::thread t_get_sensor(get_sensor);
	std::thread t_move_control(move_control);

	printf("�̳߳�ʼ�����\n");
	t_Key_Scan.join(); //�������ȴ����߳��˳�
	t_get_img.join();
	t_get_sensor.join();
	t_move_control.join();
	//�رն�ʱ��
	CloseHandle(hTimer_Key_Scan);
	CloseHandle(hTimer_get_img);
	CloseHandle(hTimer_get_sensor);
	CloseHandle(hTimer_move_control);
	
	//�ݻ�����OpenCV����
	cv::destroyAllWindows();
	
	printf("�����߳��˳����������\n");
	return 0;
}


void Key_Scan(void)
{
	int key_value_cv = -1;
	clock_t time_1 = clock();//get time
	
	while (true)
	{
		//printf("Key_Scan��ʱ:%d ms\n", clock() - time_1);
		time_1 = clock();
		//�ȴ���ʱ��ʱ�䵽��
		WaitForSingleObject(hTimer_Key_Scan, INFINITE);
		if (flag_exit)//�˳��߳�
		{
			return;
		}

		//��ʾͼ��
		g_mutex_img.lock();//�����
		if (!front_image.empty())
		{
			cv::imshow("FROWARD", front_image);
		}
		if (!down_image.empty())
		{
			cv::imshow("DOWN", down_image);
		}
		/*if (!depth_image.empty())
		{
			cv::imshow("FROWARD_DEPTH", depth_image);
		}*/
		g_mutex_img.unlock();//�ͷ���

		key_value_cv = cv::waitKeyEx(1);//��ȡ����
		while (-1 != cv::waitKey(1));	//��ʾͼ��Ҫ�ѻ���������󣬲Ż���ʾ1msͼ��
		key_control(key_value_cv);		//ִ�а�������
	
	}
	
}

void get_img(void)
{
	int i = 0;
	clock_t time_1= clock();//get time

	std::vector<ImageRequest> request = { ImageRequest(0, ImageType::Scene) , ImageRequest(0, ImageType::DepthPerspective, true), ImageRequest(3, ImageType::Scene) };
	while (true)
	{

		//�ȴ���ʱ��ʱ�䵽��
		WaitForSingleObject(hTimer_get_img, INFINITE);

		time_1 = clock();   
		if (flag_exit)//�˳��߳�
		{
			return;
		}

		std::vector<ImageResponse>& response = client.simGetImages(request);//��ȡͼ��
		if (response.size() > 0)
		{
			g_mutex_img.lock();//�����
			front_image = cv::imdecode(response.at(0).image_data_uint8, cv::IMREAD_COLOR);	//;ǰ��ͼ
			down_image = cv::imdecode(response.at(2).image_data_uint8, cv::IMREAD_COLOR);	//����ͼ	

			//���ͼ
			depth_image = Mat(response.at(1).height, response.at(1).width, CV_16UC1);
			// ת�����ͼ��ø����������ͼ
			avoid_obstacle_flag = imageResponse2Mat(response.at(1), depth_image);//����ֵΪ����

			g_mutex_img.unlock();//�ͷ���
		}

		//printf("    get_img��ʱ:%d ms\n", clock() - time_1);

// �ָ��� ***************************************************************************************************
		switch (flag_mode)//����1������2
		{
		default:
		case 1:		//����1 ��Ȧ
			//���� ��Ȧ����();
			break;
		case 2:		//��10�ǵ�����
		{
			border_img b_flag;
			int h_flag = 1;

			switch (step)
			{
			case 1:
				//���ߵ�18
				while (h_flag)
					h_flag = setAltitude(20.0, 1);
				h_flag = 1;
				//std::this_thread::sleep_for(std::chrono::duration<double>(3.0f));
				sleep(3);
				step = 2;
				break;
			case 2:
				turn180();
				//std::this_thread::sleep_for(std::chrono::duration<double>(6.0f));
				sleep(7);
				step = 3;
				break;
			case 3:	
				set_control_cmd(true, 1, 1, 0.1f, .0f, .0f, .0f, 0.25f, .0f, .0f);
				b_flag = border_dectection_task2(down_image);
				if (b_flag.bottom == 1)
				{
					//���ߵ�16
					while (h_flag)
						h_flag = setAltitude(16.0, 1);
					h_flag = 1;
					sleep(3);
					step = 4;
					break;
				}
				break;
			case 4:
				set_control_cmd(true, 1, 1, .0f, 0.1f, .0f, .0f, 0.25f, .0f, .0f);
				b_flag = border_dectection_task2(down_image);
				if (b_flag.right == 1)
				{
					step = 1;
					flag_mode = 3;
					break;
				}
				break;
			}
		}
			break;
		case 3:		//С����

			break;
		case 4:		//���ص�0��ͣ��ƺ
			border_img border_flag;
			int h_flag = 1;

			interval_num++;
			if (interval_num >= 20)		//ÿ���һ��ʱ�䶨һ�θ�
			{
				back_step = 1;
				interval_num = 0;
			}
			
			switch (back_step)
			{
			case 1:
				//���ߵ�18
				while (h_flag)
					h_flag = setAltitude(20.0, 1);
				h_flag = 1;
				sleep(3);
				if (back_border == 1)	//������˷��صı߽磬����������
					back_step = 3;
				else
					back_step = 2;
				break;

			case 2:
				set_control_cmd(true, 1, 1, -0.1f, .0f, .0f, .0f, 0.3f, .0f, .0f);
				border_flag = border_dectection_task2(down_image);
				if (border_flag.left == 1)
					set_control_cmd(true, 1, 1, .0f, -0.1f, .0f, .0f, 0.05f, .0f, .0f);

				if(border_flag.right==1)
					set_control_cmd(true, 1, 1, .0f, 0.1f, .0f, .0f, 0.05f, .0f, .0f);

				if (border_flag.top == 1)
					back_step = 3;
				break;

			case 3:
				//TODO	���䵽0��ͣ��ƺ
				break;
			}

			break;
		}
	}	
}

struct control_cmd {
	bool flag_enable = false;//true��ʾ��ָ��ûִ�й�,��Ҫִ��һ��
	int flag_move = 0;// 0��ֹͣ��1��pitch/rollģʽ�ƶ���2��inclination/orientationģʽ�ƶ���3���趨�߶�

	//llf���ӵ�ָ�����
	int flag_set_altitude = 1;		//1��ʾ�߶Ⱦ�����2��ʾ�߶ȴֵ�

	float pitch = .0f, roll = .0f;
	float inclination = .0f, orientation = .0f;
	float duration = .05f;				//����ʱ��
	float set_H = .0f;//�趨�߶�
	float yaw_rate = 0.0f;
};//��ͷ�ļ���

struct control_cmd control_cmdset;
bool flag_H = false;//ָʾ�����Ƿ���ɣ�ture��ʾ�������

//���ÿ�������
//void set_control_cmd(bool flag_enable=true, int flag_move=0,  int flag_set_altitude, float pitch = .0f, float roll = .0f,
//					 float inclination = .0f, float orientation = .0f, float duration=0.05f, 
//					 float set_H = .0f, float yaw_rate = 0.0f)
void set_control_cmd(bool flag_enable, int flag_move, int flag_set_altitude, float pitch, float roll,
					 float inclination, float orientation, float duration,
					 float set_H, float yaw_rate)
{
	g_mutex_control_cmd.lock();//����

	control_cmdset.flag_enable		= flag_enable;
	control_cmdset.flag_move		= flag_move;

	//llf: ���ö��߷�ʽ
	control_cmdset.flag_set_altitude = flag_set_altitude;

	control_cmdset.pitch			= pitch;
	control_cmdset.roll				= roll;
	control_cmdset.inclination		= inclination;
	control_cmdset.orientation		= orientation;
	control_cmdset.duration			= duration;
	control_cmdset.set_H			= set_H;
	control_cmdset.yaw_rate			= yaw_rate;

	g_mutex_control_cmd.unlock();//�ͷ���
}

void move_control(void)//�ƶ������߳�
{
	clock_t time_1= clock();//get time
	
	float throttle = 0.587402f;//�պõ�������ʱ������
	struct control_cmd control_cmdset_temp;
	while (true)
	{
		//�ȴ���ʱ��ʱ�䵽��
		WaitForSingleObject(hTimer_move_control, INFINITE);
		if (flag_exit)//�˳��߳�
		{
			return;
		}

		g_mutex_control_cmd.lock();//����
		control_cmdset_temp.flag_enable		= control_cmdset.flag_enable;
		control_cmdset_temp.flag_move		= control_cmdset.flag_move;

		//llf���ӵ�
		control_cmdset_temp.flag_set_altitude = control_cmdset.flag_set_altitude;

		control_cmdset_temp.pitch			= control_cmdset.pitch;
		control_cmdset_temp.roll			= control_cmdset.roll;
		control_cmdset_temp.inclination		= control_cmdset.inclination;
		control_cmdset_temp.orientation		= control_cmdset.orientation;
		control_cmdset_temp.duration		= control_cmdset.duration;
		control_cmdset_temp.set_H			= control_cmdset.set_H;
		control_cmdset_temp.yaw_rate		= control_cmdset.yaw_rate;

		control_cmdset.flag_enable = false;
		g_mutex_control_cmd.unlock();//�ͷ���
		if (control_cmdset_temp.flag_enable)	//��ָ��δִ�й�
		{
			switch (control_cmdset_temp.flag_move)	//�ƶ�ģʽ
			{
			default:
			case 0:// 0��ֹͣ
				client.hover();//hover
				break;
			case 1://1��pitch/rollģʽ�ƶ�
				client.hover();//hover
				throttle = 0.587402f / cos(control_cmdset_temp.pitch) / cos(control_cmdset_temp.roll);
				client.moveByAngleThrottle(control_cmdset_temp.pitch, control_cmdset_temp.roll, throttle, control_cmdset_temp.yaw_rate, control_cmdset_temp.duration);
				break;
			case 2://2��inclination / orientationģʽ�ƶ�
				break;
			case 3://3���趨�߶�
				client.hover();//hover
				int result_set_H=setAltitude(control_cmdset_temp.set_H, control_cmdset_temp.flag_set_altitude);
				flag_H = result_set_H == 0 ? true : false;
				cout << "flag_H = " << flag_H << endl;
				break;
			}
		}
	}
}

static int key_control(int key)//��������
{
	clock_t time_1;// = clock();//get time
	//float roll = 0.1f;//��x����ʱ�� //��λ�ǻ���
	//float pitch = 0.1f;//��y����ʱ��  
	//float yaw = 0.0f; //��z����ʱ��
	//float duration = 0.05f;//����ʱ��
	//float throttle = 0.587402f;//�պõ�������ʱ������
	//float yaw_rate = 0.1f;

	float roll = 0.0f;//��x����ʱ�� //��λ�ǻ���
	float pitch = 0.0f;//��y����ʱ��  
	float yaw = 0.0f; //��z����ʱ��
	float duration = 0.05f;//����ʱ��
	float throttle = 0.587402f;//�պõ�������ʱ������
	float yaw_rate = 0.0f;

	////llf---���Ա���
	float target_altitude;

	bool flag = false;//Ϊ��ʱִ�п��ƺ���
	switch (key)
	{
	case 27://ESC
		flag_exit = true;//�˳���־λ��λ
		break;
	case 32://�ո�
		client.hover();//hoverģʽ
		printf("hover\n");
		//flag = true;
		break;
	case 'w':
		flag = true;
		throttle += 0.1f;
		break;
	case 's':
		flag = true;
		throttle -= 0.1f;
		break;
	case 'a'://��תʱ���½�...
		//set_control_cmd(true, 1, .0f, .0f, .0f, .0f, 0.05f, .0f, -0.7f);
		set_control_cmd(true, 1, 1, .0f, .0f, .0f, .0f, 0.05f, .0f, -0.7f);	//llf������flag_set_altitude
		break;
	case 'd':
		//set_control_cmd(true, 1, .0f, .0f, .0f, .0f, 0.05f, .0f, 0.7f);
		set_control_cmd(true, 1, 1, .0f, .0f, .0f, .0f, 0.05f, .0f, 0.7f);	//llf������flag_set_altitude
		break;

	//�������Ի�ͷ����ǰ������
	case 'i'://pitch y����ʱ��Ƕ�
		//set_control_cmd(true, 1, -0.1f, .0f, .0f, .0f, 0.05f, .0f, .0f);
		set_control_cmd(true, 1, 1, -0.1f, .0f, .0f, .0f, 0.05f, .0f, .0f);	//llf������flag_set_altitude
		break;
	case 'k'://pitch y����ʱ��Ƕ�
		//set_control_cmd(true, 1, 0.1f, .0f, .0f, .0f, 0.05f, .0f, .0f);
		set_control_cmd(true, 1, 1, 0.1f, .0f, .0f, .0f, 0.05f, .0f, .0f);	//llf������flag_set_altitude
		break;
	case 'j'://roll x����ʱ��Ƕ�
		//set_control_cmd(true, 1, .0f, -0.1f, .0f, .0f, 0.05f, .0f, .0f);
		set_control_cmd(true, 1, 1, .0f, -0.1f, .0f, .0f, 0.05f, .0f, .0f);	//llf������flag_set_altitude
		break;
	case 'l'://roll x����ʱ��Ƕ�
		//set_control_cmd(true, 1, .0f, 0.1f, .0f, .0f, 0.05f, .0f, .0f);
		set_control_cmd(true, 1, 1, .0f, 0.1f, .0f, .0f, 0.05f, .0f, .0f);	//llf������flag_set_altitude
		break;
	case 2490368://�������
		throttle += 0.0003f;
		printf("throttle=%f\n", throttle);
		break;
	case 2621440://�������
		throttle -= 0.0003f;
		printf("throttle=%f\n", throttle);
		break;

	case '4':
		//turn180();
		//flyToCorner();
		flag_mode = 4;
		break;

	case '5':
		target_altitude = client.getBarometerdata().altitude;
		target_altitude -= 0.5;
		controlAltitude(target_altitude, 1);
		break;

	case '6':
		pathName = "E:\\2018��߷����˻��������\\ͼƬ��\\" + to_string(num++) + ".jpg";
		cv::imwrite(pathName, front_image);
		cout << "num = " << num << endl;
		break;

	//llf���ӵĲ������
	case '7':
		target_altitude = client.getBarometerdata().altitude;
		target_altitude += 0.5;
		controlAltitude(target_altitude, 1);
		break;
	case '8':
		int mode, set_altitude_result;
		cout << "please input target_altitude , mode :";
		cin >> target_altitude;
		cin >> mode;
		controlAltitude(target_altitude, mode);
		//set_altitude_result = setAltitude(target_altitude, mode);
		//cout << "set_altitude_result = " << set_altitude_result << endl;
		break;
	case '9':
		cout << "current_altitude = " << client.getBarometerdata().altitude << endl;
		break;
	case '0':
		cout << "������ --- " << land() << endl;
		break;
	default:
		break;
	}

	if (flag)
	{
		flag = false;
		client.moveByAngleThrottle(pitch, roll, throttle, yaw_rate, duration);
	}

	return 0;
}

void get_sensor(void)//��ȡ����������
{
	clock_t time_1 = clock();
	while (true)
	{
		//printf("  get_sensor��ʱ:%d ms\n", clock() - time_1);
		time_1 = clock();
		//�ȴ���ʱ��ʱ�䵽��
		WaitForSingleObject(hTimer_get_sensor, INFINITE);
		if (flag_exit)//�˳��߳�
		{
			return;
		}

		//���´���������
		//g_mutex_sensor.lock();//������������
		//Barometer_data = client.getBarometerdata();
		//Magnetometer_data = client.getMagnetometerdata();
		//Imu_data = client.getImudata();
		//g_mutex_sensor.unlock();//�������������ͷ�	
	}
}






