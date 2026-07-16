/*
	JSBSim을 조종하기 위한 스틱 러더 제어기

	추적점을 통과하기 위한 조종값을 만드는 제어기가 아닌 추적점을 바라보기 위한 조종값을 만드는것을 목표로함

		-> 추적점을 통과하는 제어기는 해당 점으로 이동하는 방법에는 잘 맞을지 모르겠으나 조준 성능이 떨어져 해당 AIP 프로젝트에서는 사용 불가능

	간략한 개요

		- 내 forwardVector를 법선으로 가지는 가상의 평면을 생성 
		- 적기를 해당 평면에 프로젝션
		- 내 ForwardVector와 평면이 만나는 점에서 프로젝션된 적기의 위치까지의 벡터를 구함 : V
		- 내 UpVector와 V와의 각도를 통하여 Roll Cmd 생성
		- 해당 각도와 Los 등을 이용하여 pitch cmd 생성
		- Rudder Cmd는 RollCMD와 los 값을 이용하여 생성

		자세한건 코드보고 잘 알아서 보시길
*/
#pragma once

#include "Vector3.h"
#include "EulerAngle.h"
#include "Matrix3.h"
#include "Quaternion.h"
//#include <cmath>
#include <vector>
#include "CoordinateConverter.h"

using namespace BT_Geometry;

const double DEG2RAD = 3.14159265358979323846 / 180.0;

struct StickValue
{
	float RollCMD;
	float PitchCMD;
	float RudderCMD;
};

class StickController
{
	int SumCount;
	float MF[20];
	int FilterIndex;
	std::vector<float> ErrorSum;

public:
	StickController();
	float GetLOSErrorSUM(float LOSError);
	/*
		일반적인 기동에서 사용하는 제어기

		MyLocation_FNED : Cartesian 	/ 고도를 양수로 가지는 평면 좌표계(언리얼 엔진과 동일)
		MyRotation_FNED : Radian		/ 평면 좌표계 기준 자세(언리얼. JSBSim 출력값 라디안 버전)
		VP				: Cartesian 	/ 이 비행기가 바라봐야할 위치
	*/
	StickValue GetStick(Vector3 MyLocation_FNED, Vector3 MyRotation_FNED, Vector3 VP);

};