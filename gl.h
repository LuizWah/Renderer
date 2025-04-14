#include "tgaimage.h"
#include "model.h"
#include "math.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0, 255,   0,   0);
const TGAColor blue  = TGAColor(0, 0,   255,   0);

const float rotationAngle = 0.0f;

matrix viewMatrix;
matrix MatrixProjection;


struct zbuffer{
    float** buffer;

    zbuffer(int width, int height){
        buffer = new float*[width];
        for (int i = 0; i < width; i++) {
            buffer[i] = new float[height];
        }

        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                buffer[i][j] = std::numeric_limits<float>::lowest();
            }
        }
    }

};

struct IShader {
    virtual Vec4f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor &color) = 0;
};



float findZ(Vec3f barycentrics, float OGdepth[]) {
    if (barycentrics.x < 0 || barycentrics.y < 0 || barycentrics.z < 0) {
        return -1;
    }
    return barycentrics.x * OGdepth[0] + barycentrics.y * OGdepth[1] + barycentrics.z * OGdepth[2];
}

bool valid_pixel_edge_Vec3f(Vec3f *vertA, Vec3f *vertB, Vec3f *point) {
    Vec2f ab = {vertB->x - vertA->x, vertB->y - vertA->y};
    Vec2f ap = {point->x - vertA->x, point->y - vertA->y};

    if ((ab.x * ap.y - ab.y * ap.x) >= 0) {
        return true;
    }
    return false;
}

void triangle_filled_depth(Vec3f pts[], float *Zbuffer[], TGAColor color, TGAImage *image, IShader &shader, float OGdepth[3]) {
    int x_min = std::min(std::min(pts[0].x, pts[1].x), pts[2].x);
    int x_max = std::max(std::max(pts[0].x, pts[1].x), pts[2].x);
    int y_min = std::min(std::min(pts[0].y, pts[1].y), pts[2].y);
    int y_max = std::max(std::max(pts[0].y, pts[1].y), pts[2].y);

    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            Vec3f point = {(float)x, (float)y, 0};

            bool bari1 = valid_pixel_edge_Vec3f(&pts[0], &pts[1], &point);
            bool bari2 = valid_pixel_edge_Vec3f(&pts[1], &pts[2], &point);
            bool bari3 = valid_pixel_edge_Vec3f(&pts[2], &pts[0], &point);

            bool bari4 = valid_pixel_edge_Vec3f(&pts[0], &pts[1], &pts[2]);
            bool bari5 = valid_pixel_edge_Vec3f(&pts[1], &pts[2], &pts[0]);
            bool bari6 = valid_pixel_edge_Vec3f(&pts[2], &pts[0], &pts[1]);



            if (bari1 == bari4 && bari2 == bari5 && bari3 == bari6) {
                Vec3f barycentrics = barycentric(pts[0], pts[1], pts[2], point);
                
                point.z = findZ(barycentrics, OGdepth);
                if (point.z < -1) {
                    continue;
                }
                if (Zbuffer[x][y] <= point.z) {
                    bool discard = shader.fragment(barycentrics, color);
                    if(!discard){
                        Zbuffer[x][y] = point.z;
                        image->set(x, y, color);
                    }
                }
            }
        }
    }
}


matrix createViewMatrix(Vec3f cameraPos, Vec3f cameraTarget, Vec3f cameraUp) {
    Vec3f zAxis = (cameraPos - cameraTarget).normalize();
    Vec3f xAxis = (cross(cameraUp, zAxis)).normalize();
    Vec3f yAxis = (cross(zAxis, xAxis));

    viewMatrix.m[0][0] = xAxis.x; viewMatrix.m[1][0] = xAxis.y; viewMatrix.m[2][0] = xAxis.z; viewMatrix.m[3][0] = -(xAxis * cameraPos);
    viewMatrix.m[0][1] = yAxis.x; viewMatrix.m[1][1] = yAxis.y; viewMatrix.m[2][1] = yAxis.z; viewMatrix.m[3][1] = -(yAxis * cameraPos);
    viewMatrix.m[0][2] = zAxis.x; viewMatrix.m[1][2] = zAxis.y; viewMatrix.m[2][2] = zAxis.z; viewMatrix.m[3][2] = -(zAxis * cameraPos);
    viewMatrix.m[0][3] = 0.0f;    viewMatrix.m[1][3] = 0.0f;    viewMatrix.m[2][3] = 0.0f;    viewMatrix.m[3][3] = 1.0f;

    return viewMatrix;
}


matrix createRotationMatrixZ(float angleDegrees = rotationAngle) {
    float angleRadians = angleDegrees * M_PI / 180.0f;
    float cosAngle = cos(angleRadians);
    float sinAngle = sin(angleRadians);

    matrix rotationMatrix;
    rotationMatrix.m[0][0] = cosAngle;
    rotationMatrix.m[0][1] = -sinAngle;
    rotationMatrix.m[1][0] = sinAngle;
    rotationMatrix.m[1][1] = cosAngle;
    rotationMatrix.m[2][2] = 1.0f;
    rotationMatrix.m[3][3] = 1.0f;

    return rotationMatrix;
}

matrix ProjectionMatrixS(float zfar, float znear, float fov, float AspectRatio){
    float fovRav = 1.f / (tanf(fov * 0.5f / 180.0f * pi));

    MatrixProjection.m[0][0] = AspectRatio * fovRav;
    MatrixProjection.m[1][1] = fovRav;
    MatrixProjection.m[2][2] = (zfar) / (zfar - znear);
    MatrixProjection.m[3][2] = (-zfar * znear) / (zfar - znear);
    MatrixProjection.m[2][3] = 1.0f;
    MatrixProjection.m[3][3] = 0.0f;

    return MatrixProjection;
}

void WorldToScreen(Vec4f coords[], matrix viewMatrix, matrix MatrixProjection, Vec4f ProjectedVertices[], float OGdepth[], int i, int height, int width){
    Vec4f TranlatedVertices[3];

    matrixVec4fMulti(coords[i], TranlatedVertices[i], viewMatrix);
    OGdepth[i] = TranlatedVertices[i][3];
    std::cout << TranlatedVertices[i][0] << " ";
    std::cout << TranlatedVertices[i][1] << " ";
    std::cout << TranlatedVertices[i][2] << " ";
    std::cout << TranlatedVertices[i][3] << " ";
    //TranlatedVertices[i][2] += 2.f;

    matrixVec4fMulti(TranlatedVertices[i], ProjectedVertices[i], MatrixProjection);
    std::cout << ProjectedVertices[i][0] << " ";
    std::cout << ProjectedVertices[i][1] << " ";
    std::cout << ProjectedVertices[i][2] << " ";
    std::cout << ProjectedVertices[i][3] << " "; 

    //putting coords between screen width
    ProjectedVertices[i][0] += 1.0f;
    ProjectedVertices[i][1] += 1.0f;
    ProjectedVertices[i][0] *= width / 2.0f;
    ProjectedVertices[i][1] *= height / 2.0f;
    std::cout << ProjectedVertices[i][0] << " ";
    std::cout << ProjectedVertices[i][1] << " ";
    std::cout << ProjectedVertices[i][2] << " ";
    std::cout << ProjectedVertices[i][3] << " "; 
    //clamping values that pass the maximun or minimun
    // ProjectedVertices[i][0] = std::clamp(ProjectedVertices[i][0], 0.0f, (float) width);
    // ProjectedVertices[i][1] = std::clamp(ProjectedVertices[i][1], 0.0f, (float) height);
}





void line_with_vec2i(Vec2i vec0, Vec2i vec1, TGAImage &image, TGAColor color){
	bool steep = false;
	bool slopeDown = false;
	if(std::abs(vec0.x - vec1.x) < std::abs(vec0.y - vec1.y)){
		std::swap(vec0.x, vec0.y);
		std::swap(vec1.x, vec1.y);
		steep = true;
	}

	if(vec0.x > vec1.x){
		std::swap(vec0.x, vec1.x);
		std::swap(vec0.y, vec1.y);
	}
	if(vec0.y > vec1.y){
		slopeDown = true;
	}

	int Dx = std::abs(vec1.x - vec0.x);//4
	int Dy = std::abs(vec1.y - vec0.y);//4

	int A = 2*Dy;//8
	int B = A - 2*Dx;//0
	int P = A - Dx;//4

	//image.set(x0,y0, color3); 
	for(int x = vec0.x; x < vec1.x; x++){
		if(P < 0){
			if(steep == true){
				image.set(vec0.y, x + 1, color);
			}
			else{
				image.set(x + 1, vec0.y, color);
			}
			P += A;
		}
		else{
			if(slopeDown == true){
				vec0.y -= 1;
			}
			if(steep == true){
				if(slopeDown == true){
					image.set(vec0.y + 1, x + 1, color);// y0 - x is the one giving problems
					//y0 -= 1;
				}
				else{
					image.set(vec0.y + 1, x + 1, color);
					vec0.y += 1;
				}
			}
			else{
				if(slopeDown == true){
					image.set(x + 1, vec0.y, color);// y0 - x is the one giving problems
					//y0 -= 1;
				}
				else{
					image.set(x + 1, vec0.y, color);
					vec0.y += 1;
				}
			}
			P += B; 
		}	
	}
}