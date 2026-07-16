// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <iostream>
#include "./behaviortree_cpp_v3/bt_factory.h"
#include "./BT_Content/Task/TaskNodes.h"
#include "./BT_Content/Service/ServiceNodes.h"
#include "./BT_Content/Decorator/DecoratorNodes.h"
#include "../Geometry/Vector3.h"
#include "../Geometry/EulerAngle.h"
#include "../Geometry/Quaternion.h"
#include "./BT_Content/BlackBoard/CPPBlackBoard.h"
#include "./BT_Content/Functions.h"
#include "../Geometry/Controller_CY.h"


#define OriLAT 37.91455691666666
#define OriLOn 128.18188127777776

/*
	Unreal Engien 4 의 비헤비어트리로 만든 RAIP를 C++ 기반의 공짜 비헤비어트리로 구현하기 위한 클래스

	init()				: 트리 xml과 각 노드들을 load하고 블랙보드를 초기화 하는 부분
	RunCPPBT()			: 비헤비어트리를 통하여 추적점 생성
	Step()				: 생성된 추적점을 쫓아가는 스틱값 생성
	PreventLandCrash()	: 지상 충돌 방지 기능 함수
	getBT_Text()		: 비헤비어트리 어너테이션 기능으로 블랙보드에 저장된 비헤비어트리의 결정 과정 String을 불러오는 부분
	SetACM()			: 유무인 복합에서 인간 조종사가 아군기의 ACM(EF/SF)를 수동으로 결정하기 위한 함수
	SetTarget()			: 유무인 복합에서 인간 조종사가 아군기의 Target을 수동을 결정하기 위한 함수
*/
class  UCPPBehaviorTree
{

private:
	double f2m;
	double EQ_R;
	double P_R;
	double fr;
	double Req;
	double d2r;
	double m2f;
	double elev0;
	double aile0;
	double eccen;

private:
	//Lat, Lon, 고도는 meter
	Vector3 LLAtoCartesian(Vector3 LLA, Vector3 BaseLLA);

public:	
	int ID;			//리눅스환경에서 사용하는 변수
	int ForceID;		//리숙스환경에서 사용하는 변수
	// Sets default values for this component's properties
	UCPPBehaviorTree();
	~UCPPBehaviorTree();
	
	BT::BehaviorTreeFactory Factory;	//C++ 비헤비어트리 객체 클래스
	BT::Tree tree;	// C++ 비헤비어트리 트리
	CPPBlackBoard* BB;	// C++ 비헤비어 트리의 기본 블랙보드 방식이 쓰레기 수준이라 따로 블랙보드 클래스를 구현하여 사용
	StickController Controller; // 제어기. 비헤비어트리에서 VP(추적점)을 생성하면 그 VP를 향하여 움직이게 하는 Roll Pitch Yaw 커멘드 값을 생성
public:	
	
	
	//트리 xml과 각 노드들을 load하고 블랙보드를 초기화 하는 부분
	void init();	

	/*
	비헤비어트리를 통하여 추적점 생성
		VP			: Cartesian 좌표계, meter
		Throttle	: 0~1 사이의 쓰로틀값
		AimmingMode : 제어기의 조종 모드를 결정
	*/
	void RunCPPBT(Vector3& VP, float& Throttle, bool& AimmingMode); //서비스 노드 역할, 디시전 트리

	/*
	비헤비어트리에서 생성된 VP를 향하여 비행기가 바라보도록 비행기가 움직이게 하는 스틱값을 생성하는 함수
		MyInfo					: 내 비행기 정보 (위치 자세 속도 팀 정보등)
		NumofOtherPlane			: 전장에서 내 비행기가 아닌 다른 비행기들의 개수
		OthersInfo				: 내 비행기가 아닌 다른 비행기들의 정보 리스트(Array)
		VP						: 디버그용 Ref 변수
		Throttle				: 디버그용 Ref 변수
	*/
	StickValue Step(PlaneInfo MyInfo, int NumofOtherPlane, PlaneInfo* OthersInfo, Vector3 & VP, float & Throttle);

	Vector3 GetVP();

	//비헤비어트리 델타타입 설정 함수
	void SetDeltaTime(double DT);
};
