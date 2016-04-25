#ifndef _VAP_COMMON_H_
#define _VAP_COMMON_H_

#define CNUM 10
#define VAP_MAX_BLOB_IN_1_FRAME  255   //单帧团块数量上限
#define MAX_FACE_IN_1_ROI        20    //单个区域人脸数量上限
#define VRG_GRAY 1
#define VRG_COLOR 3
#define VRG_FAIL -1
#define VRG_SUCC 0

#define VRG_MAX(x,y) ( (x ) > (y ) ?  (x ): (y) )
#define VRG_MIN(x,y) ( (x ) < (y ) ?  (x ): (y) )

//团块区域
typedef struct ROI_RECT
{
	int x;   //区域左上角x坐标
	int y;   //区域左上角y坐标
	int width;  //区域宽度
	int height; //区域高度
}s_Rect;

//团块数据
typedef struct ROI_DATA
{
	int ID;         //团块序号
	int type;       //团块类型，包括人体、人脸、车辆、车牌、车标、动物等
	int centroid_X; //质心横坐标
	int centroid_Y; //质心纵坐标
	int area;		//区域面积
	s_Rect RoiRect; //ROI区域外接矩形的起点坐标和宽高
}s_ROI_Data;

//团块列表
typedef struct ROI_TABLE
{	
	int ImgWidth;
	int ImgHeight;
	unsigned char *ImgData;     //图像彩色数据
	unsigned char *MaskData;    //图像蒙板
	unsigned char *BGData;      //背景数据
	int color;                  //颜色. 1:灰度；3：彩色
	int ObjNum;					//团块总数
	s_ROI_Data BlobData[VAP_MAX_BLOB_IN_1_FRAME];//团块数据
}s_ROI_Table;

//人体特征
typedef struct HUMAN_FEATURE
{	
	int color_up[CNUM];		//上半身颜色，color_up[i]表示上半身第i个颜色分量所占的百分比
	int color_down[CNUM];	//下半身颜色，color_down[i]表示下半身第i个颜色分量所占的百分比
}s_Human_Feature;

//人脸列表
typedef struct FACEROI_TABLE
{	
	int FaceNum;             //人脸总数
	s_Rect FaceROI[MAX_FACE_IN_1_ROI];   //人脸ROI区域
}s_FaceROI_Table;

char *VRG_ROIInfo_Version();
void *VRG_ROIInfo_Create(int *in_para);
void VRG_ROIInfo_Release(void **handle);
int VRG_ROIDet_Init(void *handle, int *in_para);
int VRG_ROIDet_Run(void *handle, unsigned char *in_ImageData, unsigned char *in_ImageMask, s_ROI_Table *out_ROI_table);
int VRG_ROIClass_Init(void *handle,char *filename_human, char *filename_car);
int VRG_ROIClass_Run(void *handle, int *in_para, unsigned char *ROIImg);

char *VRG_HumanInfo_Version();
void *VRG_HumanInfo_Create(int *in_para);
void VRG_HumanInfo_Release(void **handle);
int VRG_HumanFeature(void *handle, int *in_para, unsigned char *ROIImg, unsigned char *MaskImg, s_Human_Feature *out_Human_feature);
int VRG_FaceClass_Init(void *handle,char *filename_face);
int VRG_FaceROI_Run(void *handle, int *in_para, unsigned char *ROIImg, s_FaceROI_Table *out_FaceROI_Table);

#endif
