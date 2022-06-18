#include "bmp.h" //	Simple .bmp library
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// debug
// #include <chrono>
// #include <thread>

using namespace std;

#define Baseline 30.0
#define Focal_Length 100
#define Image_Width 35.0
#define Image_Height 35.0
#define Resolution_Row 512
#define Resolution_Col 512
#define View_Grid_Row 9
#define View_Grid_Col 9

// extra define
#define VIEWPOINT_LIMIT 120

struct Point3d
{
	double x;
	double y;
	double z;
	Point3d(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
};

struct Point2d
{
	double x;
	double y;
	Point2d(double x_, double y_) : x(x_), y(y_) {}
};

/* new functions */

int getImagePlaneIndex(int s, int t)
{
	return t * View_Grid_Col + s;
}

Point3d bilinearInterpolation(int c, int r, double alpha, double beta, vector<Bitmap> viewImageList, int tl, int tr, int bl, int br)
{

	Bitmap tlBitmap = viewImageList[tl];
	Bitmap trBitmap = viewImageList[tr];
	Bitmap blBitmap = viewImageList[bl];
	Bitmap brBitmap = viewImageList[br];
	// get rgb
	Color tlColor, trColor, blColor, brColor;
	tlBitmap.getColor(c, r, tlColor.R, tlColor.G, tlColor.B);
	trBitmap.getColor(c, r, trColor.R, trColor.G, trColor.B);
	blBitmap.getColor(c, r, blColor.R, blColor.G, blColor.B);
	brBitmap.getColor(c, r, brColor.R, brColor.G, brColor.B);

	// calculate new rgb by formula
	Point3d pTarget(0, 0, 0);
	double p1, p2;
	p1 = (1.0 - alpha) * tlColor.R + alpha * trColor.R;
	p2 = (1.0 - alpha) * blColor.R + alpha * brColor.R;
	pTarget.x = (1 - beta) * p1 + beta * p2;

	p1 = (1.0 - alpha) * tlColor.G + alpha * trColor.G;
	p2 = (1.0 - alpha) * blColor.G + alpha * brColor.G;
	pTarget.y = (1 - beta) * p1 + beta * p2;

	p1 = (1.0 - alpha) * tlColor.B + alpha * trColor.B;
	p2 = (1.0 - alpha) * blColor.B + alpha * brColor.B;
	pTarget.z = (1 - beta) * p1 + beta * p2;

	return pTarget;
}

int main(int argc, char **argv)
{
	if (argc < 5 || argc > 6)
	{
		cout << "Arguments prompt: viewSynthesis.exe <LF_dir> <X Y Z> OR: viewSynthesis.exe <LF_dir> <X Y Z> <focal_length>" << endl;
		return 0;
	}
	string LFDir = argv[1];
	double Vx = stod(argv[2]), Vy = stod(argv[3]), Vz = stod(argv[4]); // view point position
	double targetFocalLen = 100;									   // default focal length
	if (argc == 6)
	{
		targetFocalLen = stod(argv[5]); // focal len
	}

	vector<Bitmap> viewImageList;
	//! loading light field views
	for (int i = 0; i < View_Grid_Col * View_Grid_Row; i++)
	{
		char name[128];
		sprintf(name, "/cam%03d.bmp", i); // change i to get the correct the file names
		string filePath = LFDir + name;
		Bitmap view_i(filePath.c_str());
		viewImageList.push_back(view_i);
	}

	Bitmap targetView(Resolution_Col, Resolution_Row);
	cout << "Synthesizing image from viewpoint: (" << Vx << "," << Vy << "," << Vz << ") with focal length: " << targetFocalLen << endl;
	//! resample pixels of the target view one by one

	for (int r = 0; r < Resolution_Row; r++)
	{
		for (int c = 0; c < Resolution_Col; c++)
		{
			Point3d rayRGB(0, 0, 0);
			//! resample the pixel value of this ray: TODO
			Point3d viewPoint(Vx, Vy, Vz);

			// convert pixel index (u, v) to image coordinate (x, y): (x, y) = (u * scale - 35/2, v * scale - 35/2)
			// find target pixel
			Point2d subImagePlane(c * Image_Width / Resolution_Col - Image_Width / 2, r * Image_Height / Resolution_Row - Image_Height / 2);

			// get 4 neighbor viewpoints: top left tl, top right tr, bottom left bl, and bottom right br
			double x = viewPoint.x;
			double y = viewPoint.y;

			// convert to (s, t)
			x = (x + VIEWPOINT_LIMIT) / Baseline;
			y = View_Grid_Row - 1 - (y + VIEWPOINT_LIMIT) / Baseline;

			// get 4 neighbor viewpoints' image plane index
			int tl, tr, bl, br;
			tl = getImagePlaneIndex(floor(x), ceil(y));
			tr = getImagePlaneIndex(ceil(x), ceil(y));
			bl = getImagePlaneIndex(floor(x), floor(y));
			br = getImagePlaneIndex(ceil(x), floor(y));

			double alpha = x - floor(x);
			double beta = y - floor(y);

			// calculate the target rgb by bilinearly interpolating using current view point
			rayRGB = bilinearInterpolation(c, r, alpha, beta, viewImageList, tl, tr, bl, br);

			//! record the resampled pixel value
			targetView.setColor(c, r, (unsigned char)rayRGB.x, (unsigned char)rayRGB.y, (unsigned char)rayRGB.z);
		}
	}
	string savePath = "newView.bmp";
	targetView.save(savePath.c_str());
	cout << "Result saved!" << endl;

	// // debug: compare 2 bmp files
	// std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	// Bitmap newView("newView.bmp");
	// Bitmap ans("LF_views/cam031.bmp");
	// // Bitmap ans("newView2.bmp");
	// printf("%d", ans.getHeight());
	// unsigned char r, g, b, ar, ag, ab;
	// for (int y = 0; y < newView.getHeight(); y++)
	// {
	// 	for (int x = 0; x < newView.getWidth(); x++)
	// 	{
	// 		newView.getColor(x, y, r, g, b);
	// 		ans.getColor(x, y, ar, ag, ab);
	// 		if (r != ar || g != ag || b != ab)
	// 		{
	// 			printf("%d, %d\n", x, y);
	// 			printf("%d %d %d", r, g, b);
	// 			printf(" vs %d %d %d\n", ar, ag, ab);
	// 		}
	// 	}
	// }
	// printf("correct\n");

	return 0;
}
