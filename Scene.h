#ifndef _SCENE_H_
#define _SCENE_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "Camera.h"
#include "Color.h"
#include "Mesh.h"
#include "Rotation.h"
#include "Scaling.h"
#include "Translation.h"
#include "Triangle.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Matrix4.h"

using namespace std;

class Scene
{
public:
	Color backgroundColor;
	bool cullingEnabled;

	vector< vector<Color> > image;
	vector< Camera* > cameras;
	vector< Vec3* > vertices;
	vector< Color* > colorsOfVertices;
	vector< Scaling* > scalings;
	vector< Rotation* > rotations;
	vector< Translation* > translations;
	vector< Mesh* > meshes;

	Scene(const char *xmlPath);

	void initializeImage(Camera* camera);
	void forwardRenderingPipeline(Camera* camera);
	int makeBetweenZeroAnd255(double value);
	void writeImageToPPMFile(Camera* camera);
	void convertPPMToPNG(string ppmFileName, int osType);
	Matrix4 getModelingTransform(Mesh & mesh);
	Matrix4 getRotationMatrix(Rotation * r);
	Matrix4 getScalingMatrix(Scaling * s);
	Matrix4 getTranslationMatrix(Translation * t);
	Matrix4 getOrtographicProjection(Camera *camera);
	Matrix4 getPerspectiveProjection(Camera *camera);
	Matrix4 getViewportMatrix(Camera *camera);
	void midpoint(Vec4 &v1, Vec4 &v2);
	bool clipping(Vec4 &vec0, Vec4 &vec1, int nx, int ny);
	void rasterTriangle(Vec4 &v0, Vec4 &v1, Vec4 &v2, int nx, int ny);
	double line(double xp, double yp, double x1, double y1, double x2, double y2);
};

#endif
