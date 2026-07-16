#include <map>
#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include "./BehaviorTree/CPPBehaviorTree.h"

using namespace std;

#define MAX_ITEM 51
#define MAX_OTHRES 3
#define D2R 1.745329251994330e-2
#define R2D 57.29577951308232e+0
#define METER_TO_FEET 3.28084
#define FEET_TO_METER 0.3048
#define KNOT_TO_MS 0.5144444
#define KNOT_TO_FT 1.68781

#pragma pack(push, 1)
typedef struct _ControlValue
{
	float RollCMD;
	float PitchCMD;
	float RudderCMD;
    float Throttle;
}ControlValue, *pControlValue;

typedef struct NavigationData {
	uint8_t Header[2];
    uint8_t Counter;
    double  SimTime;
    uint8_t AircraftModel;
    uint8_t AircraftID;

    int32_t Lat;
    int32_t Lon;
    uint32_t Alt;

    int32_t phi;
    int32_t theta;
    int32_t psi;

    int32_t u;
    int32_t v;
    int32_t w;

    int32_t p;
    int32_t q;
    int32_t r;

    int32_t Ax;
    int32_t Ay;
    int32_t Az;

    float AOA;
    float AOS;
    uint16_t KCAS;
    uint16_t KTAS;
    uint16_t GNDS;
    uint16_t MachNum;
    float VV;
    int16_t Nz;
    int16_t Ny;

    uint8_t LonMode;
    float LonCtrlCmd;
    int16_t ElevatorPosition;
    int8_t FlapCtrlCmd;
    int16_t FlapPosition;

    uint8_t LatMode;
    float LatCtrlCmd;
    int16_t AileronPosition; 
    float DirCtrlCmd;
    int16_t RudderPosition; 

    uint8_t SpeedMode;
    float SpeedCtrlCmd1;
    uint32_t Engine1_N1RPM;
    uint32_t Engine1_N2RPM;
    double Engine1_FuelFlow;
    float SpeedCtrlCmd2;
    uint32_t Engine2_N1RPM;
    uint32_t Engine2_N2RPM;
    double Engine2_FuelFlow;
    int8_t SpeedBrakeCtrlCmd;
    uint16_t SpeedBrakePosition;
    double Fuel;
    uint8_t checksum;

} oNavigationData;

typedef struct LPlaneData {
    float LocationX;
    float LocationY;
    float LocationZ;
    float Roll;
    float Pitch;
    float Yaw;
    float Speed;
    int Team;
    float Resv0;
    float Resv1;
    float Resv2;
} oPlaneData;


struct VPandThrottle
{
    public:
        float VPx;
        float VPy;
        float VPz;
        float Throttle;
};

#pragma pack(pop)

//extern "C"
//{
//    void CreateBehaviorTree(int OwnerID, int ForceID);
//    
//    //1대1용으로 사용하던 임시 Step 함수. ACM 교전이 추가되면서 사용안함
//    ControlValue Step(int MyID, int MyForceID, int TargetID, int MyTargetID, oNavigationData &Owner,  oNavigationData &Target);
//    
//    /*
//    2대2를 위해 새로 추가된 Step함수.
//    MyData                  : 내 비행기 정보
//    NumOfOthers             : 내 비행기 말고 다른 비행기들 개수
//    Others                  : 내 비행기 말고 다른 비행기들 정보
//    isLockedOn              : 적기에게 락온 여부
//    MSL_Lunch_Possible      : 현재 미사일 발사 가능 여부. true가 되면 LunchMSL() 함수 호출해줘야함
//    Flare_Lunch_Possible    : 현재 플레어 사용 가능 여부. true가 되면 LunchFlare() 함수 호출해줘야함
//    */
//    ControlValue Step2(oPlaneData& MyData, int NumOfOthers, oPlaneData* Others, bool isLockedOn, bool & MSL_Lunch_Possible, bool & Flare_Lunch_Possible);    
//   
//     /*
//    step2 함수에서 VP, Throttle 값을 반환하는 함수.
//    MyData                  : 내 비행기 정보
//    NumOfOthers             : 내 비행기 말고 다른 비행기들 개수
//    Others                  : 내 비행기 말고 다른 비행기들 정보
//    isLockedOn              : 적기에게 락온 여부
//    MSL_Lunch_Possible      : 현재 미사일 발사 가능 여부. true가 되면 LunchMSL() 함수 호출해줘야함
//    Flare_Lunch_Possible    : 현재 플레어 사용 가능 여부. true가 되면 LunchFlare() 함수 호출해줘야함
//    */
//    VPandThrottle GetVP(oPlaneData& MyData, int NumOfOthers, oPlaneData* Others, bool isLockedOn, bool & MSL_Lunch_Possible, bool & Flare_Lunch_Possible);
//
//    /*
//    비헤비어트리가 가지고 있는 제어기를 실행하는 함수
//    MyData                  : 내 비행기 정보
//    VP_X, VP_Y, VP_Z        : 입력 VP 값
//    */
//    ControlValue GetStick(oPlaneData& MyData, float VP_X, float VP_Y, float VP_Z);
//
//    Vector3 LLAtoCartesian(Vector3 LLA, Vector3 BaseLLA);
//
//
//    //2대2를 위해 비행기 배열을 만들기위한 함수 
//    oPlaneData ChangeData(int ID, int Team, float HP, int OperType, oNavigationData& NaviData); 
//   
//    //2대2에서 아군기의 타겟을 변경시키기 위한 함수 FriendID :아군기 DISID, TargetDIS : 아군기 타겟 설정
//    void SetTarget(int FriendID,int TargetDIS);
//    
//    //2대2에서 아군기의 ACM 모드를 변경시키기 위한 함수 ACM : 0 == EF, ACM : 1 == SF. FriendID :아군기 DISID, ACM : 아군기 ACM 설정  
//    void SetACM_Mode(int FriendID,int ACM); 
//    
//    //비헤비어트리의 DeltaTime을 설정
//    void SetBehaviorTreeDeltaTime(int OwnShipID, double DT);  
//    
//
//    void Reset();
//    void RemoveBT(int OwnerID);
//  
//}

extern "C"
{
    /** Behavior Tree 인스턴스를 생성한다.**/
    /** OwnerID:BT를 소유한 개체ID, ForceID : 피아식별정보 **/
    __declspec(dllexport) void CreateBehaviorTree(int OwnerID, int ForceID);

    /*
    2대2를 위해 새로 추가된 Step함수.
    MyData                  : 내 비행기 정보
    NumOfOthers             : 내 비행기 말고 다른 비행기들 개수
    Others                  : 내 비행기 말고 다른 비행기들 정보
    isLockedOn              : 적기에게 락온 여부
    MSL_Lunch_Possible      : 현재 미사일 발사 가능 여부. true가 되면 LunchMSL() 함수 호출해줘야함
    Flare_Lunch_Possible    : 현재 플레어 사용 가능 여부. true가 되면 LunchFlare() 함수 호출해줘야함
    */
    __declspec(dllexport) ControlValue Step(oPlaneData& MyData, int NumOfOthers, oPlaneData* Others, bool isLockedOn, bool& MSL_Lunch_Possible, bool& Flare_Lunch_Possible);
    __declspec(dllexport) const char* GetAnnotation(int id);

    //2대2를 위해 비행기 배열을 만들기위한 함수 
    __declspec(dllexport) oPlaneData ChangeData(int ID, int Team, float HP, int OperType, oNavigationData& NaviData);

    __declspec(dllexport) ControlValue GetStick(oPlaneData& MyData, float VP_X, float VP_Y, float VP_Z);

    __declspec(dllexport) Vector3 LLAtoCartesian(Vector3 LLA, Vector3 BaseLLA);
    //2대2에서 아군기의 타겟을 변경시키기 위한 함수 FriendID :아군기 DISID, TargetDIS : 아군기 타겟 설정
    __declspec(dllexport) void SetTarget(int FriendID, int TargetDIS);

    //2대2에서 아군기의 ACM 모드를 변경시키기 위한 함수 ACM : 0 == EF, ACM : 1 == SF. FriendID :아군기 DISID, ACM : 아군기 ACM 설정  
    __declspec(dllexport) void SetACM_Mode(int FriendID, int ACM);

    //비헤비어트리의 DeltaTime을 설정
    __declspec(dllexport) void SetBehaviorTreeDeltaTime(int OwnShipID, double DT);

    __declspec(dllexport) Vector3 GetVP(oPlaneData& MyData);

    __declspec(dllexport) void Reset();
    __declspec(dllexport) void RemoveBT(int OwnerID);

}

map<int, shared_ptr<UCPPBehaviorTree>> BTList;

Vector3 LLAtoCartesian(Vector3 LLA, Vector3 BaseLLA)
{
	double eccentricitysquare, N, M;
	eccentricitysquare = 1.0 - pow(6356752.3142, 2) / pow(6378137.0, 2);
	N = 6378137.0 / sqrt(1.0 - eccentricitysquare * pow(sin(BaseLLA.X * PI / 180.0), 2)); // prime vertical radius of curvature
	M = 6378137.0 * (1.0 - eccentricitysquare) / pow(1 - eccentricitysquare * pow(sin(BaseLLA.X * PI / 180.0), 2), 3 / 2);

	double dlat, dlon;
	dlat = LLA.X - BaseLLA.X;
	dlon = LLA.Y - BaseLLA.Y;

	double dN, dE, dD;
	dN = (M + BaseLLA.Z) * dlat * PI / 180.0;
	dE = (N + BaseLLA.Z) * cos(BaseLLA.X * PI / 180.0) * dlon * PI / 180.0;
	dD = (LLA.Z - BaseLLA.Z);
	Vector3 res(dN, dE, dD);
	return res;
}

void CreateBehaviorTree(int OwnerID, int ForceID)
{
    shared_ptr<UCPPBehaviorTree>  BT = make_shared<UCPPBehaviorTree>(); 
    
    BT->ID = OwnerID;
    BT->ForceID = ForceID;
    BT->init();
    BTList.insert(make_pair(BT->ID, BT));
}

void Reset()
{
    BTList.clear();
}

void RemoveBT(int OwnerID)
{
    if(BTList.find(OwnerID) != BTList.end())
    {
        //shared_ptr<UCPPBehaviorTree> bt = BTList.find(OwnerID)->second;
        BTList.erase(OwnerID);
    }
}

//ControlValue Step(int MyID, int MyForceID, int TargetID, int TargetForceID,  oNavigationData &Owner,  oNavigationData &Target)
//{
//    ControlValue value;
//    PlaneInfo my;
//    PlaneInfo others[MAX_OTHRES];
//    BT_Geometry::Vector3 VP;
//    float throttle;
//
//    memset(others, 0, sizeof(PlaneInfo)*MAX_OTHRES);
//
//    my.Location = BT_Geometry::Vector3(Owner.Lat / 1000000.0, Owner.Lon / 1000000.0, (Owner.Alt / 1000.0) * FEET_TO_METER);
//    my.Rotation = BT_Geometry::EulerAngle( Owner.psi/1000.0, Owner.theta/1000.0, Owner.phi/1000.0); //yaw pitch roll (deg)
//    my.Speed    = (Owner.KTAS / 10.0) * 0.51444; //knot to m/s
//    my.Team     = MyForceID;  //force side
//    my.Resv0    = MyID;
//   
//
//    //for(int i =0; i < NumOfTargets; i++)
//    //{
//        others[0].Location = BT_Geometry::Vector3(Target.Lat/1000000.0, Target.Lon/1000000.0, (Target.Alt / 1000.0) * FEET_TO_METER );
//        others[0].Rotation = BT_Geometry::EulerAngle( Target.psi/1000.0, Target.theta/1000.0, Target.phi/1000.0); //yaw pitch roll (deg)
//        others[0].Speed    = (Target.KTAS/ 10.0) * 0.51444; //knot to m/s
//        others[0].Team     = TargetForceID;//force side
//        others[0].Resv0    = TargetID;
//    //}
//
//    auto BT_item = BTList.find((int)my.Resv0);
//
//   // cout << "my:" <<  my.Team << " lat:" << my.Location.X << " lon:" << my.Location.Y << " alt:" << my.Location.Z << endl;
//   // cout << "target:" << others[0].Team  << " lat:" << others[0].Location.X << " lon:" << others[0].Location.Y << " alt:" << others[0].Location.Z << endl;
//
//    if(BT_item != BTList.end())
//    {
//        bool dump1;
//        bool dump2;
//        StickValue v = BT_item->second->Step(my, 1, others, VP, throttle);
//        value.RollCMD = v.RollCMD;
//        value.PitchCMD = v.PitchCMD;
//        value.RudderCMD = v.RudderCMD;
//        value.Throttle = throttle;
//    }
//    
//    return value;
//}

ControlValue Step(oPlaneData& MyData, int NumOfOthers, oPlaneData* Others, bool isLockedOn, bool & MSL_Lunch_Possible, bool & Flare_Lunch_Possible)
{
    ControlValue value;
    PlaneInfo my;
    PlaneInfo others[3];
    BT_Geometry::Vector3 VP;
    float throttle;
    //std::cout <<"LibMain NOO " <<NumOfOthers<<std::endl;
    memset(others, 0, sizeof(PlaneInfo)*3);

    my.Location.X       = MyData.LocationX;     // 여기서는 LLA 좌표임. 비헤비어트리 안에서 카르테시안으로 바꿔서 씀
    my.Location.Y       = MyData.LocationY;
    my.Location.Z       = MyData.LocationZ;
    my.Rotation.Roll    = MyData.Roll; 
    my.Rotation.Pitch   = MyData.Pitch;
    my.Rotation.Yaw     = MyData.Yaw;
    my.Speed            = MyData.Speed; 
    my.Team             = MyData.Team;  
    my.Resv0            = MyData.Resv0; //ID
    my.Resv1            = MyData.Resv1; //HP
    my.Resv2            = MyData.Resv2; //유인기 무인기 0 : AI, 1 : Human
   
   //std::cout <<"LibMain My HP " <<my.Resv1 <<std::endl;

    for(int i =0; i < NumOfOthers; i++)
    {
        others[i].Location.X        = Others[i].LocationX;
        others[i].Location.Y        = Others[i].LocationY;
        others[i].Location.Z        = Others[i].LocationZ;
        others[i].Rotation.Roll     = Others[i].Roll;
        others[i].Rotation.Pitch    = Others[i].Pitch;
        others[i].Rotation.Yaw      = Others[i].Yaw;
        others[i].Speed             = Others[i].Speed; //m/s
        others[i].Team              = Others[i].Team;  //force side
        others[i].Resv0             = Others[i].Resv0;
        others[i].Resv1             = Others[i].Resv1;
        others[i].Resv2             = Others[i].Resv2;
        //std::cout << i <<" LibMain Other ID " << others[i].Resv0<< " " <<others[i].Location.X   << " " <<others[i].Location.Y    <<std::endl;
    }

    auto BT_item = BTList.find((int)my.Resv0);

   // cout << "my:" <<  my.Team << " lat:" << my.Location.X << " lon:" << my.Location.Y << " alt:" << my.Location.Z << endl;
   // cout << "target:" << others[0].Team  << " lat:" << others[0].Location.X << " lon:" << others[0].Location.Y << " alt:" << others[0].Location.Z << endl;

    if(BT_item != BTList.end())
    {
        StickValue v = BT_item->second->Step(my, NumOfOthers, others, VP, throttle);
        value.RollCMD = v.RollCMD;
        value.PitchCMD = v.PitchCMD;
        value.RudderCMD = v.RudderCMD;
        value.Throttle = throttle;
    }
    
    return value;
}

//VPandThrottle GetVP(oPlaneData& MyData, int NumOfOthers, oPlaneData* Others, bool isLockedOn, bool & MSL_Lunch_Possible, bool & Flare_Lunch_Possible)
//{
//    
//    ControlValue value;
//    PlaneInfo my;
//    PlaneInfo others[3];
//    BT_Geometry::Vector3 VP;
//    VPandThrottle VPaThrottle;
//    float throttle;
//    //std::cout <<"LibMain NOO " <<NumOfOthers<<std::endl;
//    memset(others, 0, sizeof(PlaneInfo)*3);
//
//    my.Location.X       = MyData.LocationX;     // 여기서는 LLA 좌표임. 비헤비어트리 안에서 카르테시안으로 바꿔서 씀
//    my.Location.Y       = MyData.LocationY;
//    my.Location.Z       = MyData.LocationZ;
//    my.Rotation.Roll    = MyData.Roll; 
//    my.Rotation.Pitch   = MyData.Pitch;
//    my.Rotation.Yaw     = MyData.Yaw;
//    my.Speed            = MyData.Speed; 
//    my.Team             = MyData.Team;  
//    my.Resv0            = MyData.Resv0; //ID
//    my.Resv1            = MyData.Resv1; //HP
//    my.Resv2            = MyData.Resv2; //유인기 무인기 0 : AI, 1 : Human
//   
//   //std::cout <<"LibMain My HP " <<my.Resv1 <<std::endl;
//
//    for(int i =0; i < NumOfOthers; i++)
//    {
//        others[i].Location.X        = Others[i].LocationX;
//        others[i].Location.Y        = Others[i].LocationY;
//        others[i].Location.Z        = Others[i].LocationZ;
//        others[i].Rotation.Roll     = Others[i].Roll;
//        others[i].Rotation.Pitch    = Others[i].Pitch;
//        others[i].Rotation.Yaw      = Others[i].Yaw;
//        others[i].Speed             = Others[i].Speed; //m/s
//        others[i].Team              = Others[i].Team;  //force side
//        others[i].Resv0             = Others[i].Resv0;
//        others[i].Resv1             = Others[i].Resv1;
//        others[i].Resv2             = Others[i].Resv2;
//        //std::cout << i <<" LibMain Other ID " << others[i].Resv0<< " " <<others[i].Location.X   << " " <<others[i].Location.Y    <<std::endl;
//    }
//
//    auto BT_item = BTList.find((int)my.Resv0);
//
//   // cout << "my:" <<  my.Team << " lat:" << my.Location.X << " lon:" << my.Location.Y << " alt:" << my.Location.Z << endl;
//   // cout << "target:" << others[0].Team  << " lat:" << others[0].Location.X << " lon:" << others[0].Location.Y << " alt:" << others[0].Location.Z << endl;
//
//    
//    if(BT_item != BTList.end())
//    {
//         BT_item->second->Step(my, NumOfOthers, others, VP, throttle);
//        
//    }
//    VPaThrottle.VPx = VP.X;
//    VPaThrottle.VPy = VP.Y;
//    VPaThrottle.VPz = VP.Z;
//    VPaThrottle.Throttle = throttle;
//    return VPaThrottle;
//}

ControlValue GetStick(oPlaneData& MyData, float VP_X, float VP_Y, float VP_Z)
{
    StickValue v;
    ControlValue value;
    PlaneInfo my;
    
    my.Resv0 = MyData.Resv0; //ID
    auto BT_item = BTList.find((int)my.Resv0);

    if(BT_item != BTList.end())
    {
        BT_Geometry::Vector3 Mylocation_Cartesian = LLAtoCartesian(BT_Geometry::Vector3(MyData.LocationX,MyData.LocationY,MyData.LocationZ), Vector3(37.91455691666666, 128.18188127777776, 0));
        BT_Geometry::Vector3 MyRotation = BT_Geometry::Vector3(MyData.Roll*D2R, MyData.Pitch*D2R, MyData.Yaw*D2R);
        v = BT_item->second->Controller.GetStick(Mylocation_Cartesian, MyRotation, BT_Geometry::Vector3(VP_X, VP_Y,VP_Z));
        value.RollCMD = v.RollCMD;
        value.PitchCMD = v.PitchCMD;
        value.RudderCMD = v.RudderCMD;
    
        return value;
    }
    else
    {
        value.PitchCMD = 0;
        value.RollCMD = 0;
        value.RudderCMD = 0;
        value.Throttle = -1;
        return value;
    }
}


 void SetBehaviorTreeDeltaTime(int OwnShipID, double DT)
 {
    auto BT_item = BTList.find(OwnShipID);

    if(BT_item != BTList.end())
    { 
        BT_item->second->SetDeltaTime(DT);
    }
    else
    {
        cout << "wrong Friend ID"<<endl;
    }
 }

 Vector3 GetVP(oPlaneData& MyData)
 {
     PlaneInfo my;
     BT_Geometry::Vector3 VP;

     my.Resv0 = MyData.Resv0; //ID


     auto BT_item = BTList.find((int)my.Resv0);

     if (BT_item != BTList.end())
     {
         VP = BT_item->second->GetVP();
         //std::cout << "VP : " << VP.X << " " << VP.Y << " " << VP.Z << std::endl;
     }
     else
     {
         std::cout << "LibMain GetVP fail" << std::endl;
     }
     return VP;
 }




oPlaneData ChangeData(int ID, int Team, float HP, int OperType, oNavigationData& NaviData)
{
    oPlaneData PlaneData;
    BT_Geometry::Vector3 LLA = BT_Geometry::Vector3(NaviData.Lat/1000000.0, NaviData.Lon/1000000.0, (NaviData.Alt / 1000.0) * FEET_TO_METER );
    BT_Geometry::EulerAngle YPR = BT_Geometry::EulerAngle( NaviData.psi/1000.0, NaviData.theta/1000.0, NaviData.phi/1000.0); //yaw pitch roll (deg)
    PlaneData.LocationX = LLA.X;
    PlaneData.LocationY = LLA.Y;
    PlaneData.LocationZ = LLA.Z;
    PlaneData.Roll = YPR.Roll;
    PlaneData.Pitch = YPR.Pitch;
    PlaneData.Yaw  = YPR.Yaw;
    PlaneData.Speed    = (NaviData.KTAS / 10.0) * 0.51444; //knot to m/s
    PlaneData.Team     = Team;  //force side
    PlaneData.Resv0    = ID;
    PlaneData.Resv1    = HP;
    PlaneData.Resv2    = OperType;

    return PlaneData;
}
