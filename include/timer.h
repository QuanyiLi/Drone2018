#pragma once

#include "opencv2/opencv.hpp"
#include "WINDOWS.h" 
#include <iostream>

#define _SECOND 10000000//��ʱ1��������ֵ

//�ȴ�ss��
void sleep(int ss)
{
	clock_t start = clock();
	double duration = 0.0;
	while (true)
	{
		duration = double(clock() - start) / CLOCKS_PER_SEC;
		if (duration >= ss)
		{
			break;
		}
	}
}