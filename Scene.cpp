#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <cmath>
#include <map>

#include "Scene.h"
#include "Camera.h"
#include "Color.h"
#include "Mesh.h"
#include "Rotation.h"
#include "Scaling.h"
#include "Translation.h"
#include "Triangle.h"
#include "Vec3.h"
#include "tinyxml2.h"
#include "Helpers.h"

using namespace tinyxml2;
using namespace std;

/*
	Transformations, clipping, culling, rasterization are done here.
	You may define helper functions.
*/

//From Hw1
int colorClamp(double &color){
	if(color > 255)
		return 255;
	if(color < 0)
		return 0;
	return round(color + 0.5);
}

Color colorClamp(Color &color){
	return Color(colorClamp(color.r), 
					colorClamp(color.g), 
					colorClamp(color.b));
}

Matrix4 Scene::getTranslationMatrix(Translation * t) {
	double ret[4][4] =  {{1.,0.,0.,t->tx},
						 {0.,1.,0.,t->ty},
						 {0.,0.,1,t->tz},
						 {0.,0.,0.,1.}};
    return Matrix4(ret);
}

Matrix4 Scene::getScalingMatrix(Scaling * s) {
	double ret[4][4] =  {{s->sx,0,0,0},
						 {0,s->sy,0,0},
						 {0,0,s->sz,0},
						 {0,0,0,1}};
    return Matrix4(ret);
}
Matrix4 Scene::getRotationMatrix(Rotation * r) {
    // Construct an orthonormal basis (ONB) for the given axis of rotation
    Vec3 u = Vec3(r->ux, r->uy, r->uz, -1), v, w;

	// Compute the minimum component of the vector r
	double minVal = min(std::min(abs(r->ux), abs(r->uy)), abs(r->uz));
	
	// Select the corresponding Vec3 value based on the minimum component
	map<double, Vec3> minMap = {
		{abs(r->ux), Vec3(0, -1 * r->uz, r->uy, -1)},
		{abs(r->uy), Vec3(-1 * r->uz, 0, r->ux, -1)},
		{abs(r->uz), Vec3(-1 * r->uy, r->ux, 0, -1)}};

	v = minMap[minVal];
	w = crossProductVec3(u, v);

    // Normalize v and w
    v = normalizeVec3(v);
    w = normalizeVec3(w);

    // Construct matrices to align the given axis of rotation with the X-axis
    double onbMatrix[4][4] = {{u.x,u.y,u.z,0},
                              {v.x,v.y,v.z,0},
                              {w.x,w.y,w.z,0},
                              {0,0,0,1}};

    double onbMatrixInverse[4][4] = {{u.x,v.x,w.x,0},
                                     {u.y,v.y,w.y,0},
                                     {u.z,v.z,w.z,0},
                                     {0,0,0,1}};

    // Construct the rotation matrix for a rotation around the X-axis
    double rotationMatrix[4][4] = {{1,0,0,0},
                                   {0,cos(r->angle * M_PI/180),(-1) * sin(r->angle * M_PI/180),0},
                                   {0,sin(r->angle * M_PI/180),cos(r->angle * M_PI/180),0},
                                   {0,0,0,1}};

    // Multiply the rotation matrix with the ONB matrices to obtain the final result
    Matrix4 res = multiplyMatrixWithMatrix(rotationMatrix, onbMatrix);
    res = multiplyMatrixWithMatrix(onbMatrixInverse, res);
    return res;
}
Matrix4 getCameraTransformation(Camera* camera){
    // Create a translation matrix that translates e to the world origin (0,0,0).
    double translation_matrix[4][4] = {{1, 0, 0, -(camera->pos.x)},
                                       {0, 1, 0, -(camera->pos.y)},
                                       {0, 0, 1, -(camera->pos.z)},
                                       {0, 0, 0, 1}};
                                       
    // Create a rotation matrix that aligns uvw with xyz.
    double rotation_matrix[4][4] = {{camera->u.x, camera->u.y, camera->u.z, 0},
                                    {camera->v.x, camera->v.y, camera->v.z, 0},
                                    {camera->w.x, camera->w.y, camera->w.z, 0},
                                    {0, 0, 0, 1}};
                                    
    // Multiply the rotation and translation matrices to get the camera's transformation matrix.
    return multiplyMatrixWithMatrix(rotation_matrix, translation_matrix);
}

// Visible Function of Liang-Barsky Algorithm 
bool isVisible(double den, double num, double &t_e, double &t_l) {
    if (den > 0) {
        double t = num / den;

        if (t > t_l) 
			return false;
        else if (t > t_e) 
			t_e = t;
    } 
	else if (den < 0) {
        double t = num / den;

        if (t < t_e) 
			return false;
        else if (t < t_l) 
			t_l = t;
    } 
	else if(num > 0){
		return false;
	}

	return true;
}

//Liang-Barsky Algorithm
bool Scene::clipping(Vec4 &vec0, Vec4 &vec1, int nx, int ny){

	Color color_vec0 = *colorsOfVertices[vec0.colorId-1];
	Color color_vec1 = *colorsOfVertices[vec1.colorId-1];

	Vec4 d = subtractVec4(vec1, vec0);
	Color color_diff = (color_vec1- color_vec0)/d.x;
	Vec3 minVec(-0.5, -0.5, 0, -1.);
	Vec3 maxVec(nx-0.5, ny-0.5, 1., -1.);
	double t_e = 0, t_l = 1;

	bool visible = false;

	if (isVisible(d.x, minVec.x - vec0.x, t_e, t_l))  //left
    if (isVisible(-d.x, vec0.x - maxVec.x, t_e, t_l)) //right
    if (isVisible(d.y, minVec.y - vec0.y, t_e, t_l)) //bottom
    if (isVisible(-d.y, vec0.y - maxVec.y, t_e, t_l)) //top
    if (isVisible(d.z, minVec.z - vec0.z, t_e, t_l)) //front
    if (isVisible(-d.z, vec0.z - maxVec.z, t_e, t_l)) {//back
		visible = true;
		if(t_l < 1){
			vec1 = addVec4(vec0, multiplyVec4WithScalar(d, t_l));
			color_vec1 = color_vec0 + color_diff * t_l;
		}							
		if(t_e > 0){
			vec0 = addVec4(vec0, multiplyVec4WithScalar(d, t_e) );
			color_vec0 = color_vec1 + color_diff * t_e;
		}
	}

	return visible;
}

bool backFaceCulling(Vec4 &v1, Vec4 &v2, Vec4 &v3){
	Vec3 v3_v1 = subtractVec3(convertVec3(v3), convertVec3(v1));
	Vec3 v2_v1 = subtractVec3(convertVec3(v2), convertVec3(v1));

	Vec3 normal = normalizeVec3(crossProductVec3(v3_v1, v2_v1));
	if(dotProductVec3(normal, normalizeVec3(convertVec3(v2))) < 0) 
		return true;
	else 
		return false;

}


Matrix4 Scene::getModelingTransform(Mesh & mesh){
    // Initialize the modeling transformation to the identity matrix
    Matrix4 meshModel = getIdentityMatrix();

    // Iterate over the transformations specified by the mesh object
    for (int i = 0; i < mesh.numberOfTransformations; ++i) {
        Matrix4 transMatrix;

		// Adjust the transformation index
		int transformationId = mesh.transformationIds[i] - 1;

        // Determine the type of transformation
        switch (mesh.transformationTypes[i]) {
            case 't':
                transMatrix = getTranslationMatrix(translations[transformationId]);
                break;
				
            case 's':
                transMatrix = getScalingMatrix(scalings[transformationId]);
                break;

            case 'r':
                transMatrix = getRotationMatrix(rotations[transformationId]);
                break;

            default:
                // Invalid transformation
                return NULL;
				break;
        }

        // Apply the transformation to the running total
        meshModel = multiplyMatrixWithMatrix(transMatrix, meshModel);
    }

    // Return the final modeling transformation
    return meshModel;
}

Matrix4 Scene::getOrtographicProjection(Camera *camera){
    double far = camera->far;
    double near = camera->near;
    double top = camera->top;
	double bottom = camera->bottom;
	double left = camera->left;
    double right = camera->right;

    double res[4][4] = {
        {2 / (right - left), 0, 0, -(right + left) / (right - left)},
        {0, 2 / (top - bottom), 0, -(top + bottom) / (top - bottom)},
        {0, 0, -2 / (far - near), -(far + near) / (far - near)},
        {0, 0, 0, 1}};

    return Matrix4(res);
}

Matrix4 Scene::getPerspectiveProjection(Camera *camera) {
    double far = camera->far;
    double near = camera->near;
    double top = camera->top;
	double bottom = camera->bottom;
	double left = camera->left;
    double right = camera->right;

    double res[4][4] = {{2 * near / (right - left ), 0, (right + left) / (right - left),  0},
                           {0, 2 * near / (top - bottom), (top + bottom) / (top - bottom), 0},
                           {0, 0, -(far + near)/ (far - near), -2 * far * near / (far - near)},
                           {0, 0, -1, 0}};

    return Matrix4(res);
}

Matrix4 Scene::getViewportMatrix(Camera *camera) {
	double nx = camera->horRes;
	double ny = camera->verRes;

    double res[4][4] = {
        {nx / 2, 0, 0, (nx - 1) / 2},
        {0, ny / 2, 0, (ny - 1) / 2},
        {0, 0, 0.5, 0.5},
        {0, 0, 0, 1}};

    return Matrix4(res);
}


/*
FROM THE RASTERIZATION SLIDES ON ODTUCLASS
y = y0
d = (y0 – y1) + 0.5(x1 – x0)
c = c0
dc = (c1 – c0) / (x1 – x0) // skip α; directly compute color increment
for x = x0 to x1 do:
	draw(x, y, round(c))
	if d < 0 then: // choose NE
		y = y + 1
		d += (y0 – y1) + (x1 – x0)
	else: // choose E
		d += (y0 – y1)
		c += dc
*/

// Modify midpoint algorithms according to slope
void Scene::lineRasterizer(Vec4 &vec1, Vec4 &vec2)
{
    double dx = vec2.x - vec1.x;
    double dy = vec2.y - vec1.y;

	// -1 < slope < 1 
    if (abs(dy) <= abs(dx)) {

		// -1 < slope < 0 
        if (vec2.x < vec1.x) {
			midpoint1(vec2, vec1);
        }
		// 0 < slope < 1 
		else{
			midpoint1(vec1,vec2);
		}
    }
	// (-inf < slope < -1)  U  (1 < slope < +inf)
    else if (abs(dy) > abs(dx)) {
		// -inf < slope < -1 
        if (vec2.y < vec1.y) {
            midpoint2(vec2, vec1);
        }
		// 1 < slope < +inf
		else{
			midpoint2(vec1, vec2);
		}
    }
}
void Scene::midpoint1(Vec4 &vec1, Vec4 &vec2 ){
	Color c, c1, c2, dc;
	c1 = *colorsOfVertices[vec1.colorId-1];
	c2 = *colorsOfVertices[vec2.colorId-1];
	c = c1;
	double d;
	int y = vec1.y;

	if(vec2.y<vec1.y){       
        d = (vec1.y - vec2.y) + ( -0.5 * (vec2.x - vec1.x));
        dc = (c2 - c1) / (vec2.x - vec1.x);
        for (int x = vec1.x; x <= vec2.x; x++) {
            image[x][y] = colorClamp(c);
           // choose NE
		   if (d > 0){ 
                y--;
                d += (vec1.y - vec2.y) - (vec2.x - vec1.x);
            }
			// choose E
            else{
                d += (vec1.y - vec2.y);
			} 
            c = c + dc;
        }
	}
	else{
        d = (vec1.y - vec2.y) + ( 0.5 * (vec2.x - vec1.x));
        dc = (c2 - c1) / (vec2.x - vec1.x);
        for (int x = vec1.x; x <= vec2.x; x++) {
            image[x][y] = colorClamp(c);
            // choose NE
			if (d < 0){ 
                y ++;
                d += (vec1.y - vec2.y) + (vec2.x - vec1.x);
            }
			// choose E
            else{
 				d += (vec1.y - vec2.y);
			}      
            c = c + dc;
        }
	}

}
void Scene::midpoint2(Vec4 &vec1, Vec4 &vec2){
		Color c, c1, c2, dc;
		c1 = *colorsOfVertices[vec1.colorId-1];
		c2 = *colorsOfVertices[vec2.colorId-1];
		c = c1;
		double d;
        int x = vec1.x;

		if (vec2.x < vec1.x) {
			d = (vec2.x - vec1.x) + (-0.5 * (vec1.y - vec2.y));
			dc = (c2 - c1) / (vec2.y - vec1.y);
			for (int y = vec1.y; y <= vec2.y; y++) {
				image[x][y] = colorClamp(c);
				if (d < 0){
					x --;
					d += (vec2.x - vec1.x) - (vec1.y - vec2.y);
				}
				else{
					d += (vec2.x - vec1.x);
				}	
				c = c + dc;
			}
        }
		else{
			d = (vec2.x - vec1.x) + (0.5 * (vec1.y - vec2.y));
			dc = (c2 - c1) / (vec2.y - vec1.y);
			for (int y = vec1.y; y <= vec2.y; y++) {
				image[x][y] = colorClamp(c);
				if (d > 0){
					x ++;
					d += (vec2.x - vec1.x) + (vec1.y - vec2.y);
				}
				else{
					d += (vec2.x - vec1.x);
				}
				c = c + dc;
			}
		}

}

void Scene::triangleRasterizer(Vec4 &v0, Vec4 &v1, Vec4 &v2, int nx, int ny)
{
    // Compute the bounding box of the triangle
	// Adjust max and min values so,  0 <= values <= camera->HorRes, Camera->verRes
    double xminTemp = std::min({v0.x, v1.x, v2.x}) >= 0 ? std::min({v0.x, v1.x, v2.x}) : 0;
    double yminTemp = std::min({v0.y, v1.y, v2.y}) >= 0 ? std::min({v0.y, v1.y, v2.y}) : 0;
    double xmaxTemp = std::max({v0.x, v1.x, v2.x}) >= 0 ? std::max({v0.x, v1.x, v2.x}) : 0;
    double ymaxTemp = std::max({v0.y, v1.y, v2.y}) >= 0 ? std::max({v0.y, v1.y, v2.y}) : 0;

	double xmin = xminTemp > nx ? nx-1 : xminTemp;
	double ymin = yminTemp > ny ? ny-1 : yminTemp;
	double xmax = xmaxTemp > nx ? nx-1 : xmaxTemp;
	double ymax = ymaxTemp > ny ? ny-1 : ymaxTemp;
	 

    // Get the colors of the vertices
    Color c0(*colorsOfVertices[v0.colorId - 1]);
    Color c1(*colorsOfVertices[v1.colorId - 1]);
    Color c2(*colorsOfVertices[v2.colorId - 1]);

    // Iterate over the bounding box and draw the triangle
    for (int i = xmin; i < xmax; i++)
    {
        for (int j = ymin; j < ymax; j++)
        {
            // Compute the barycentric coordinates of the current point
            double alpha = lineEquation(i, j, v1.x, v1.y, v2.x, v2.y) / lineEquation(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);
            double beta = lineEquation(i, j, v2.x, v2.y, v0.x ,v0.y) / lineEquation(v1.x, v1.y, v2.x, v2.y, v0.x, v0.y);
            double gamma = lineEquation(i, j, v0.x, v0.y, v1.x, v1.y) / lineEquation(v2.x, v2.y, v0.x, v0.y, v1.x, v1.y);

            // Check if the current point is inside the triangle
            if (alpha >= 0 && beta >= 0 && gamma >= 0)
            {
                // Compute the color of the current point
                Color color =(c0 * alpha) + (c1 * beta) + (c2 * gamma);
                image[i][j] = colorClamp(color);
            }
        }
    }
}

void Scene::forwardRenderingPipeline(Camera *camera)
{
	// TODO: Implement this function.
	Matrix4 Mfinal = Matrix4();
	Matrix4 Mviewp = getViewportMatrix(camera);
	Matrix4 Mcam = getCameraTransformation(camera);
	if(camera->projectionType == 0){ //Orthographic
		Matrix4 Morth = getOrtographicProjection(camera);
		Mfinal = multiplyMatrixWithMatrix(Morth, Mcam);
	}
	else{//Perspective
		Matrix4 Mpers = getPerspectiveProjection(camera);
		Mfinal = multiplyMatrixWithMatrix(Mpers, Mcam);
	}
	for(auto &mesh: this->meshes){
		Matrix4 Mtransform = multiplyMatrixWithMatrix(Mfinal, getModelingTransform(*mesh));
		for(auto &triangle: mesh->triangles){
			Vec3* v1 =this->vertices[triangle.getFirstVertexId()-1];
			Vec3* v2 =this->vertices[triangle.getSecondVertexId()-1];
			Vec3* v3 =this->vertices[triangle.getThirdVertexId()-1];

			Vec4 vertex1 = multiplyMatrixWithVec4(Mtransform, Vec4(v1->x, v1->y, v1->z, 1, v1->colorId));
			Vec4 vertex2 = multiplyMatrixWithVec4(Mtransform, Vec4(v2->x, v2->y, v2->z, 1, v2->colorId));
			Vec4 vertex3 = multiplyMatrixWithVec4(Mtransform, Vec4(v3->x, v3->y, v3->z, 1, v3->colorId));

			//Dont compute back-facing polygons
			if(cullingEnabled == 1 && !backFaceCulling(vertex1, vertex2, vertex3))
				continue;

			//Perspective division

			perspectiveDivision(vertex1);
			perspectiveDivision(vertex2);
			perspectiveDivision(vertex3);

			//Viewport Transformation
			vertex1 = multiplyMatrixWithVec4(Mviewp, vertex1);
			vertex2 = multiplyMatrixWithVec4(Mviewp, vertex2);
			vertex3 = multiplyMatrixWithVec4(Mviewp, vertex3);

			//wireframe
			if(!mesh->type){

				//Creating temp variables to pass unmodified versions of vertices to lineRasterizer.
				Vec4 v1 = Vec4(vertex1); Vec4 v11 = Vec4(vertex1);
				Vec4 v2 = Vec4(vertex2); Vec4 v22 = Vec4(vertex2);
				Vec4 v3 = Vec4(vertex3); Vec4 v33 = Vec4(vertex3);

				if(clipping(v1,v2,camera->horRes, camera->verRes)){
					lineRasterizer(v1,v2);
				}
					
				if(clipping(v22,v3,camera->horRes, camera->verRes)){
					lineRasterizer(v22,v3);
				}
					
				if(clipping(v33,v11,camera->horRes, camera->verRes)){
					lineRasterizer(v33,v11);
				}
					
			} 
			//solid
			else{
				triangleRasterizer(vertex1, vertex2, vertex3, camera->horRes, camera->verRes);
			}
		}
	}
}

double Scene::lineEquation(double xp, double yp, double x1, double y1, double x2, double y2){
    return xp * (y1 - y2) + yp * (x2 - x1) + (x1 * y2) - (y1 * x2);
}

/*
	Parses XML file
*/
Scene::Scene(const char *xmlPath)
{
	const char *str;
	XMLDocument xmlDoc;
	XMLElement *pElement;

	xmlDoc.LoadFile(xmlPath);

	XMLNode *pRoot = xmlDoc.FirstChild();

	// read background color
	pElement = pRoot->FirstChildElement("BackgroundColor");
	str = pElement->GetText();
	sscanf(str, "%lf %lf %lf", &backgroundColor.r, &backgroundColor.g, &backgroundColor.b);

	// read culling
	pElement = pRoot->FirstChildElement("Culling");
	if (pElement != NULL) {
		str = pElement->GetText();
		
		if (strcmp(str, "enabled") == 0) {
			cullingEnabled = true;
		}
		else {
			cullingEnabled = false;
		}
	}

	// read cameras
	pElement = pRoot->FirstChildElement("Cameras");
	XMLElement *pCamera = pElement->FirstChildElement("Camera");
	XMLElement *camElement;
	while (pCamera != NULL)
	{
		Camera *cam = new Camera();

		pCamera->QueryIntAttribute("id", &cam->cameraId);

		// read projection type
		str = pCamera->Attribute("type");

		if (strcmp(str, "orthographic") == 0) {
			cam->projectionType = 0;
		}
		else {
			cam->projectionType = 1;
		}

		camElement = pCamera->FirstChildElement("Position");
		str = camElement->GetText();
		sscanf(str, "%lf %lf %lf", &cam->pos.x, &cam->pos.y, &cam->pos.z);

		camElement = pCamera->FirstChildElement("Gaze");
		str = camElement->GetText();
		sscanf(str, "%lf %lf %lf", &cam->gaze.x, &cam->gaze.y, &cam->gaze.z);

		camElement = pCamera->FirstChildElement("Up");
		str = camElement->GetText();
		sscanf(str, "%lf %lf %lf", &cam->v.x, &cam->v.y, &cam->v.z);

		cam->gaze = normalizeVec3(cam->gaze);
		cam->u = crossProductVec3(cam->gaze, cam->v);
		cam->u = normalizeVec3(cam->u);

		cam->w = inverseVec3(cam->gaze);
		cam->v = crossProductVec3(cam->u, cam->gaze);
		cam->v = normalizeVec3(cam->v);

		camElement = pCamera->FirstChildElement("ImagePlane");
		str = camElement->GetText();
		sscanf(str, "%lf %lf %lf %lf %lf %lf %d %d",
			   &cam->left, &cam->right, &cam->bottom, &cam->top,
			   &cam->near, &cam->far, &cam->horRes, &cam->verRes);

		camElement = pCamera->FirstChildElement("OutputName");
		str = camElement->GetText();
		cam->outputFileName = string(str);

		cameras.push_back(cam);

		pCamera = pCamera->NextSiblingElement("Camera");
	}

	// read vertices
	pElement = pRoot->FirstChildElement("Vertices");
	XMLElement *pVertex = pElement->FirstChildElement("Vertex");
	int vertexId = 1;

	while (pVertex != NULL)
	{
		Vec3 *vertex = new Vec3();
		Color *color = new Color();

		vertex->colorId = vertexId;

		str = pVertex->Attribute("position");
		sscanf(str, "%lf %lf %lf", &vertex->x, &vertex->y, &vertex->z);

		str = pVertex->Attribute("color");
		sscanf(str, "%lf %lf %lf", &color->r, &color->g, &color->b);

		vertices.push_back(vertex);
		colorsOfVertices.push_back(color);

		pVertex = pVertex->NextSiblingElement("Vertex");

		vertexId++;
	}

	// read translations
	pElement = pRoot->FirstChildElement("Translations");
	XMLElement *pTranslation = pElement->FirstChildElement("Translation");
	while (pTranslation != NULL)
	{
		Translation *translation = new Translation();

		pTranslation->QueryIntAttribute("id", &translation->translationId);

		str = pTranslation->Attribute("value");
		sscanf(str, "%lf %lf %lf", &translation->tx, &translation->ty, &translation->tz);

		translations.push_back(translation);

		pTranslation = pTranslation->NextSiblingElement("Translation");
	}

	// read scalings
	pElement = pRoot->FirstChildElement("Scalings");
	XMLElement *pScaling = pElement->FirstChildElement("Scaling");
	while (pScaling != NULL)
	{
		Scaling *scaling = new Scaling();

		pScaling->QueryIntAttribute("id", &scaling->scalingId);
		str = pScaling->Attribute("value");
		sscanf(str, "%lf %lf %lf", &scaling->sx, &scaling->sy, &scaling->sz);

		scalings.push_back(scaling);

		pScaling = pScaling->NextSiblingElement("Scaling");
	}

	// read rotations
	pElement = pRoot->FirstChildElement("Rotations");
	XMLElement *pRotation = pElement->FirstChildElement("Rotation");
	while (pRotation != NULL)
	{
		Rotation *rotation = new Rotation();

		pRotation->QueryIntAttribute("id", &rotation->rotationId);
		str = pRotation->Attribute("value");
		sscanf(str, "%lf %lf %lf %lf", &rotation->angle, &rotation->ux, &rotation->uy, &rotation->uz);

		rotations.push_back(rotation);

		pRotation = pRotation->NextSiblingElement("Rotation");
	}

	// read meshes
	pElement = pRoot->FirstChildElement("Meshes");

	XMLElement *pMesh = pElement->FirstChildElement("Mesh");
	XMLElement *meshElement;
	while (pMesh != NULL)
	{
		Mesh *mesh = new Mesh();

		pMesh->QueryIntAttribute("id", &mesh->meshId);

		// read projection type
		str = pMesh->Attribute("type");

		if (strcmp(str, "wireframe") == 0) {
			mesh->type = 0;
		}
		else {
			mesh->type = 1;
		}

		// read mesh transformations
		XMLElement *pTransformations = pMesh->FirstChildElement("Transformations");
		XMLElement *pTransformation = pTransformations->FirstChildElement("Transformation");

		while (pTransformation != NULL)
		{
			char transformationType;
			int transformationId;

			str = pTransformation->GetText();
			sscanf(str, "%c %d", &transformationType, &transformationId);

			mesh->transformationTypes.push_back(transformationType);
			mesh->transformationIds.push_back(transformationId);

			pTransformation = pTransformation->NextSiblingElement("Transformation");
		}

		mesh->numberOfTransformations = mesh->transformationIds.size();

		// read mesh faces
		char *row;
		char *clone_str;
		int v1, v2, v3;
		XMLElement *pFaces = pMesh->FirstChildElement("Faces");
        str = pFaces->GetText();
		clone_str = strdup(str);

		row = strtok(clone_str, "\n");
		while (row != NULL)
		{
			int result = sscanf(row, "%d %d %d", &v1, &v2, &v3);
			
			if (result != EOF) {
				mesh->triangles.push_back(Triangle(v1, v2, v3));
			}
			row = strtok(NULL, "\n");
		}
		mesh->numberOfTriangles = mesh->triangles.size();
		meshes.push_back(mesh);

		pMesh = pMesh->NextSiblingElement("Mesh");
	}
}

/*
	Initializes image with background color
*/
void Scene::initializeImage(Camera *camera)
{
	if (this->image.empty())
	{
		for (int i = 0; i < camera->horRes; i++)
		{
			vector<Color> rowOfColors;

			for (int j = 0; j < camera->verRes; j++)
			{
				rowOfColors.push_back(this->backgroundColor);
			}

			this->image.push_back(rowOfColors);
		}
	}
	else
	{
		for (int i = 0; i < camera->horRes; i++)
		{
			for (int j = 0; j < camera->verRes; j++)
			{
				this->image[i][j].r = this->backgroundColor.r;
				this->image[i][j].g = this->backgroundColor.g;
				this->image[i][j].b = this->backgroundColor.b;
			}
		}
	}
}

/*
	If given value is less than 0, converts value to 0.
	If given value is more than 255, converts value to 255.
	Otherwise returns value itself.
*/
int Scene::makeBetweenZeroAnd255(double value)
{
	if (value >= 255.0)
		return 255;
	if (value <= 0.0)
		return 0;
	return (int)(value);
}

/*
	Writes contents of image (Color**) into a PPM file.
*/
void Scene::writeImageToPPMFile(Camera *camera)
{
	ofstream fout;

	fout.open(camera->outputFileName.c_str());

	fout << "P3" << endl;
	fout << "# " << camera->outputFileName << endl;
	fout << camera->horRes << " " << camera->verRes << endl;
	fout << "255" << endl;

	for (int j = camera->verRes - 1; j >= 0; j--)
	{
		for (int i = 0; i < camera->horRes; i++)
		{
			fout << makeBetweenZeroAnd255(this->image[i][j].r) << " "
				 << makeBetweenZeroAnd255(this->image[i][j].g) << " "
				 << makeBetweenZeroAnd255(this->image[i][j].b) << " ";
		}
		fout << endl;
	}
	fout.close();
}

/*
	Converts PPM image in given path to PNG file, by calling ImageMagick's 'convert' command.
	os_type == 1 		-> Ubuntu
	os_type == 2 		-> Windows
	os_type == other	-> No conversion
*/
void Scene::convertPPMToPNG(string ppmFileName, int osType)
{
	string command;

	// call command on Ubuntu
	if (osType == 1)
	{
		command = "convert " + ppmFileName + " " + ppmFileName + ".png";
		system(command.c_str());
	}

	// call command on Windows
	else if (osType == 2)
	{
		command = "magick convert " + ppmFileName + " " + ppmFileName + ".png";
		system(command.c_str());
	}

	// default action - don't do conversion
	else
	{}

}