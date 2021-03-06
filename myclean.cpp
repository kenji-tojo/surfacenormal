#include <iostream>
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <opencv2/core/core.hpp>
#include <opencv2/core/types_c.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

namespace fs = std::filesystem;

using namespace cv;
using namespace std;
float fcxcy[3];//SET THE CAMERA PARAMETERS  F CX CY BEFORE USE calplanenormal.
int  WINDOWSIZE=15;//SET SEARCH WINDOWSIZE(SUGGEST 15) BEFORE USE calplanenormal.
float Tthrehold;//SET THE threshold (SUGGEST 0.1-0.2)BEFORE USE calplanenormal.
// Ax+by+cz=D

void cvFitPlane(const CvMat* points, float* plane);
int telldirection(float* abc, int i, int j, float d);

void
CallFitPlane(const Mat& depth,int * points,int i,int j,float *plane12) {
	float f =fcxcy[0];
	float cx=fcxcy[1];
	float cy=fcxcy[2];
	vector<float>X_vector;
	vector<float>Y_vector;
	vector<float>Z_vector;
	for(int num_point=0; num_point<WINDOWSIZE*WINDOWSIZE;num_point++ )
		if (points[num_point]==1) {//search 已经处理了边界,此处不需要再处理了
		int point_i,point_j;
		point_i=floor(num_point/WINDOWSIZE);
		point_j=num_point-(point_i*WINDOWSIZE);
		point_i+=i-int(WINDOWSIZE/2);point_j+=j-int(WINDOWSIZE/2);
		float x = (point_j - cx) * depth.at<float>(point_i, point_j ) * 1.0 / f;
		float y = (point_i - cy) * depth.at<float>(point_i, point_j )* 1.0 / f;
		float z = depth.at<float>(point_i,point_j);
		X_vector.push_back(x);
		Y_vector.push_back(y);
		Z_vector.push_back(z);
		}
    CvMat*points_mat = cvCreateMat(X_vector.size(), 3, CV_32FC1);//定义用来存储需要拟合点的矩阵 
	if(X_vector.size()<3){ plane12[0]=-1;plane12[1]=-1;plane12[2]=-1;plane12[3]=-1;return;}
	for (int ii=0;ii < X_vector.size(); ++ii){
			points_mat->data.fl[ii * 3 + 0] = X_vector[ii];//矩阵的值进行初始化   X的坐标值
			points_mat->data.fl[ii * 3 + 1] = Y_vector[ii];//  Y的坐标值
			points_mat->data.fl[ii * 3 + 2] = Z_vector[ii];//
		}
		// float plane12[4] = { 0 };//定义用来储存平面参数的数组 
		cvFitPlane(points_mat, plane12);//调用方程 
		if( telldirection(plane12,i,j,depth.at<float>(i,j))  ){
		plane12[0]=-plane12[0];
		plane12[1]=-plane12[1];
		plane12[2]=-plane12[2];}
		X_vector.clear();
		Y_vector.clear();
		Z_vector.clear();
		cvReleaseMat(&points_mat);
}

void 
cvFitPlane(const CvMat* points, float* plane){
	// Estimate geometric centroid.
	int nrows = points->rows;
	int ncols = points->cols;
	int type = points->type;
	CvMat* centroid = cvCreateMat(1, ncols, type);
	cvSet(centroid, cvScalar(0));
	for (int c = 0; c<ncols; c++){
		for (int r = 0; r < nrows; r++)
		{
			centroid->data.fl[c] += points->data.fl[ncols*r + c];
		}   
		centroid->data.fl[c] /= nrows;
	}
	// Subtract geometric centroid from each point.
	CvMat* points2 = cvCreateMat(nrows, ncols, type);
	for (int r = 0; r<nrows; r++)
		for (int c = 0; c<ncols; c++)
			points2->data.fl[ncols*r + c] = points->data.fl[ncols*r + c] - centroid->data.fl[c];
	// Evaluate SVD of covariance matrix.
	CvMat* A = cvCreateMat(ncols, ncols, type);
	CvMat* W = cvCreateMat(ncols, ncols, type);
	CvMat* V = cvCreateMat(ncols, ncols, type);
	cvGEMM(points2, points, 1, NULL, 0, A, CV_GEMM_A_T);
	cvSVD(A, W, NULL, V, CV_SVD_V_T);
	// Assign plane coefficients by singular vector corresponding to smallest singular value.
	plane[ncols] = 0;
	for (int c = 0; c<ncols; c++){
		plane[c] = V->data.fl[ncols*(ncols - 1) + c];
		plane[ncols] += plane[c] * centroid->data.fl[c];
	}
	// Release allocated resources.
	cvReleaseMat(&centroid);
	cvReleaseMat(&points2);
	cvReleaseMat(&A);
	cvReleaseMat(&W);
	cvReleaseMat(&V);
}
void
search_plane_neighbor(Mat &img,int i,int j ,float threhold,int* result){
	 int cols =img.cols;
	 int rows =img.rows; 
	 for (int ii=0; ii<WINDOWSIZE*WINDOWSIZE;ii++)
	 result[ii]=0;
	 float center_depth = img.at<float>(i,j);
     for (int idx=0; idx<WINDOWSIZE;idx++)
	  for (int idy=0; idy<WINDOWSIZE;idy++){
		  int rx= i-int(WINDOWSIZE/2)+idx;
		  int ry= j-int(WINDOWSIZE/2)+idy;
		 if(  rx>= rows || ry>=cols )continue;
		 if (rx < 0 || ry < 0) continue; // added
		 if( img.at<float>(rx,ry)==0.0)continue;
		 if( abs(img.at<float>(rx,ry)-center_depth)<=Tthrehold*center_depth )
             result[idx*WINDOWSIZE+idy]=1;
		}
}
int 
telldirection(float * abc,int i,int j,float d){
	float f =fcxcy[0];
	float cx=fcxcy[1];
	float cy=fcxcy[2];
	float x = (j - cx) *d * 1.0 / f;
    float y = (i - cy) *d * 1.0 / f;
    float z = d;
	// Vec3f camera_center=Vec3f(cx,cy,0);
	Vec3f cor = Vec3f(0-x, 0-y, 0-z);
	Vec3f abcline = Vec3f(abc[0],abc[1],abc[2]);
	float corner = cor.dot(abcline);
	//  float corner =(cx-x)*abc[0]+(cy-y) *abc[1]+(0-z)*abc[2];
	if (corner>=0)
	   return 1;
	else return 0;
 
}
Mat 
calplanenormal(Mat  &src){
	 float f =fcxcy[0];
	 float cx=fcxcy[1];
	 float cy=fcxcy[2];
     Mat normals = Mat::zeros(src.size(),CV_32FC3);
	 src.convertTo(src,CV_32FC1);
	 src*=1.0;
	 int cols =src.cols;
	 int rows =src.rows;
    //  int plane_points[WINDOWSIZE*WINDOWSIZE]={0};
	 int * plane_points = new int[WINDOWSIZE*WINDOWSIZE];
	 float * plane12 = new float[4];
	 for (int i=0;i< rows;i++)
				for (int j=0;j< cols;j++){
                    //for kitti and nyud test
					if(src.at<float>(i,j)==0.0)continue;
                    //for:nyud train
                    //  if(src.at<float>(i,j)<=4000.0)continue;   

					search_plane_neighbor(src,i,j,15.0,plane_points);
					CallFitPlane(src,plane_points,i,j,plane12);
					Vec3f d = Vec3f(plane12[0],plane12[1],plane12[2]);
					Vec3f n = normalize(d);
					normals.at<Vec3f>(i, j) = n;
			}
	 Mat res = Mat::zeros(src.size(),CV_32FC3);
     for (int i=0;i<rows;i++)
      for (int j=0;j<cols;j++){
        res.at<Vec3f>(i, j)[0] = -1.0 * normals.at<Vec3f>(i, j)[0];
        res.at<Vec3f>(i, j)[2] = -1.0 * normals.at<Vec3f>(i, j)[1];
        res.at<Vec3f>(i, j)[1] = -1.0 * normals.at<Vec3f>(i, j)[2];
     }

	 delete[] plane12;
	 delete[] plane_points;
	 normals.release();
     for (int i=0;i<rows;i++)
      for (int j=0;j<cols;j++){
		if(!(res.at<Vec3f>(i, j)[0]==0&&res.at<Vec3f>(i, j)[1]==0&&res.at<Vec3f>(i, j)[2]==0)){
			res.at<Vec3f>(i, j)[0] += 1.0 ;
			res.at<Vec3f>(i, j)[2] += 1.0 ;
			res.at<Vec3f>(i, j)[1] += 1.0;
		 }
      }
	 
     //res =res * 127.5;
	 res *= 127.5;
     res.convertTo(res, CV_8UC3);
     cvtColor(res, res, COLOR_BGR2RGB);
	 return res;
}

/* void
demo{
//set parameters here:fcxcy[0]=0;fcxcy[1]=0;fcxcy[2]=0;
Mat src=imread(INPUT_FILE_NAME,CV_LOAD_IMAGE_ANYDEPTH);
Mat res=calplanenormal(src);
imwrite(OUTPUT_NAME,res);
} */

void task(int i, const fs::path& lidar_path,
	const fs::path& normal_path, const fs::path& intr_path) {

	std::vector<string> lidar_data, intr_data, norm_data;
	fs::path lpath = lidar_path / std::to_string(i);
	fs::path npath = normal_path / std::to_string(i);
	fs::create_directories(npath);
	fs::path ipath = intr_path / std::to_string(i);
	
	for (const auto& entry : fs::directory_iterator(lpath)) {
		lidar_data.push_back(entry.path().string());
		norm_data.push_back((npath / entry.path().filename()).string());
	}
	for (const auto& entry : fs::directory_iterator(ipath)) {
		intr_data.push_back(entry.path().string());
	}
	//cout << "n_lidar_data: " << lidar_data.size() << endl;
	//cout << "n_norm_data: " << norm_data.size() << endl;
	//cout << "n_intr_data: " << intr_data.size() << endl;

	vector<float> camera_intrs;
	chrono::duration<double> elapsed;

	for (int j = 0; j < lidar_data.size(); j++) {
		auto start = chrono::high_resolution_clock::now();
		cout << lidar_data[j] << endl;
		camera_intrs.clear();
		ifstream infile;
		string instr;
		infile.open(intr_data[j]);
		std::getline(infile, instr);
		istringstream iss(instr);
		do {
			float val;
			iss >> val;
			camera_intrs.push_back(val);
		} while (iss);
		infile.close();
		cout << camera_intrs[0] << ' ' << camera_intrs[2] << ' ' << camera_intrs[5] << endl;
		fcxcy[0] = camera_intrs[0];
		fcxcy[1] = camera_intrs[2];
		fcxcy[2] = camera_intrs[5];

		Mat img = imread(lidar_data[j], IMREAD_ANYDEPTH);
		cout << img.size();
		Mat res = calplanenormal(img);
		imwrite(norm_data[j], res);
		auto finish = chrono::high_resolution_clock::now();
		elapsed = finish - start;
		cout << "Elapsed time: " << elapsed.count() << endl;
	}
}

int main(int argc, char *argv[]) {

	fcxcy[0] = 721.5377;
	fcxcy[1] = 609.5593;
	fcxcy[2] = 149.854;
	Tthrehold = 0.1;

	if (argc != 3) {
		cout << "usage: " << argv[0] << " lidar_dir intr_dir" << endl;
	}

	//chrono::duration<double> elapsed;

	std::string lidar_dir(argv[1]);
	std::string normal_dir = lidar_dir + "_normal";
	std::string intr_dir(argv[2]);
	fs::path lidar_path(lidar_dir);
	fs::path normal_path(normal_dir);
	fs::path intr_path(intr_dir);
	//std::vector<string> lidar_data, intr_data, norm_data;
	std::vector<std::thread> threads;

	for (int i = 0; i < 6; i++) {
		threads.push_back(std::thread(task, i, lidar_path, normal_path, intr_path));
		//lidar_data.clear();
		//intr_data.clear();
		//norm_data.clear();
		//fs::path lpath = lidar_path / std::to_string(i);
		//fs::path npath = normal_path / std::to_string(i);
		//fs::create_directories(npath);
		//fs::path ipath = intr_path / std::to_string(i);
		//
		//for (const auto& entry : fs::directory_iterator(lpath)) {
		//	lidar_data.push_back(entry.path().string());
		//	norm_data.push_back((npath / entry.path().filename()).string());
		//}
		//for (const auto& entry : fs::directory_iterator(ipath)) {
		//	intr_data.push_back(entry.path().string());
		//}
		//cout << "n_lidar_data: " << lidar_data.size() << endl;
		//cout << "n_norm_data: " << norm_data.size() << endl;
		//cout << "n_intr_data: " << intr_data.size() << endl;

		//vector<float> camera_intrs;
		//for (int j = 0; j < lidar_data.size(); j++) {
		//	auto start = chrono::high_resolution_clock::now();
		//	cout << lidar_data[j] << endl;
		//	camera_intrs.clear();
		//	ifstream infile;
		//	string instr;
		//	infile.open(intr_data[j]);
		//	std::getline(infile, instr);
		//	istringstream iss(instr);
		//	do {
		//		float val;
		//		iss >> val;
		//		camera_intrs.push_back(val);
		//	} while (iss);
		//	infile.close();
		//	cout << camera_intrs[0] << ' ' << camera_intrs[2] << ' ' << camera_intrs[5] << endl;
		//	fcxcy[0] = camera_intrs[0];
		//	fcxcy[1] = camera_intrs[2];
		//	fcxcy[2] = camera_intrs[5];

		//	Mat img = imread(lidar_data[j], IMREAD_ANYDEPTH);
		//	cout << img.size();
		//	Mat res = calplanenormal(img);
		//	imwrite(norm_data[j], res);
		//	auto finish = chrono::high_resolution_clock::now();
		//	elapsed = finish - start;
		//	cout << "Elapsed time: " << elapsed.count() << endl;
		//}
		//for (const auto& entry : fs::directory_iterator(lpath)) {
		//	cout << entry.path().string() << endl;
		//	Mat img = imread(entry.path().string(), IMREAD_ANYDEPTH);
		//	cout << img.size();
		//	Mat res = calplanenormal(img);
		//	imwrite((npath / entry.path().filename()).string(), res);
		//}
	}
	
	for (auto& th : threads) {
		th.join();
	}

	/*
	std::string path = "data";
	fs::path p(path);
	for (const auto& entry : fs::directory_iterator(p))
		std::cout << entry.path() << std::endl;
	*/

	
	//Mat src = imread(argv[0], CV_LOAD_IMAGE_ANYDEPTH);
	//const char* inputs[3] = { "testimg.png", "data/testdir/test2.png", "data/testdir/test3.png" };
	//const char* outputs[3] = { "normal1.png", "normal2.png", "normal3.png" };

	//for (int i = 0; i < 3; i++) {
	//	Mat img = imread(inputs[i], IMREAD_ANYDEPTH);
	//	if (img.empty()) {
	//		cout << "empty image" << endl;
	//	}
	//	auto start = chrono::high_resolution_clock::now();
	//	Mat res = calplanenormal(img);
	//	auto finish = chrono::high_resolution_clock::now();
	//	elapsed = finish - start;
	//	cout << img.size() << endl;
	//	cout << "Elapsed time: " << elapsed.count() << endl;
	//	imwrite(outputs[i], res);
	//}
	
	//Mat res = calplanenormal(src);
	//cout << src.size() << endl;
	//imwrite("normal.png", res);
	
	
	return 0;
}
