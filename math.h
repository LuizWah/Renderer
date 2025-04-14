#include <algorithm>
#include "geometry.h"
#include <vector>
#include <cmath>
#include <limits>

const float pi = 3.14159f;



Vec3f barycentric(Vec3f a, Vec3f b, Vec3f c, Vec3f P) {
    float u = ((b.y - c.y) * (P.x - c.x) - (b.x - c.x) * (P.y - c.y)) / ((c.y - a.y) * (b.x - c.x) - (c.x - a.x) * (b.y - c.y));
    float v = (P.x - u * a.x + c.x * (u - 1)) / (b.x - c.x);
    float w = 1 - u - v;
    
    if (std::abs(w) > 0) {
        return {u, v, w};
    }
    
    return {-1, -1, -1};    
}


//Matrice and Vec3f operations
struct matrix {
    float m[4][4] = {0};
};

matrix matrix_matrix_mult(matrix& m1, matrix& m2, matrix& result){
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 4; j++){
            for(int k = 0; k < 4; k++){
                result.m[i][j] += m1.m[i][k] + m2.m[k][j];
            }
        }
    }
    return result;
}

matrix transpose(matrix m , matrix &transposed_matrix){
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 4; j++){
            transposed_matrix.m[i][j] = m.m[j][i];
        }
    }

    return transposed_matrix;
}


float det3x3(float (*matrix)[3]){
    float m1 = matrix[0][0] * matrix[1][1] * matrix[2][2] + matrix[0][1] * matrix[1][2] * matrix[2][0] + matrix[1][0] * matrix[2][1] * matrix[0][2];
    float m2 = matrix[0][2] * matrix[1][1] * matrix[2][0] + matrix[0][1] * matrix[1][0] * matrix[2][2] + matrix[1][2] * matrix[2][1] * matrix[0][0];

    return m1 - m2;
}



matrix minor_matrices(matrix m, matrix &minors){
    for(int k = 0; k < 4; k++){
        for(int l = 0; l < 4; l++){
            int current_row = 0;
            int current_collum = 0;
            float matrix3x3[3][3];
            for(int i = 0; i < 4; i++){
                for(int j = 0; j < 4; j++){
                    if(i != k && j != l){
                        matrix3x3[current_row][current_collum] = m.m[i][j];
                        if(current_collum < 2){
                            current_collum +=1 ;
                        }
                        else if(current_row < 2){
                            current_collum = 0;
                            current_row += 1;
                        }
                       
                    }
                }
            }
            
            minors.m[k][l] = det3x3(matrix3x3) * pow(-1,k);
        }
    }
    return minors;
}



matrix Invert4x4(matrix m, matrix &inverted){
    matrix minors;
    minor_matrices(m, minors);

    // for(int i = 0; i < 4; i++){
    //     for(int j = 0; j < 4; j++){
    //         std::cout << minors.m[i][j] << " ";
    //     }
    //     std::cout << "\n";
    // }


    float inverse_determinant4x4 = 1/(m.m[0][0] * minors.m[0][0] - m.m[0][1] * minors.m[0][1] + m.m[0][2] * minors.m[0][2] - m.m[0][3] * minors.m[0][3]);

    transpose(minors, inverted);

    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 4; j++){
            inverted.m[i][j] *= inverse_determinant4x4;
        }
    }
    return inverted;
}

matrix invert_transpose(matrix m, matrix &inverted_transposed){
    Invert4x4(m, inverted_transposed);
    transpose(inverted_transposed, inverted_transposed);

    return inverted_transposed;
}


    
void matrixVec4fMulti(Vec4f &i, Vec4f &o, matrix &m) {
    o[3] = i[2];
    
    o[0] = m.m[0][0] * i[0] + m.m[1][0] * i[1] + m.m[2][0] * i[2] + m.m[3][0] * i[3];
    o[1] = m.m[0][1] * i[0] + m.m[1][1] * i[1] + m.m[2][1] * i[2] + m.m[3][1] * i[3];
    o[2] = m.m[0][2] * i[0] + m.m[1][2] * i[1] + m.m[2][2] * i[2] + m.m[3][2] * i[3];
    
    
    if (o[3] != 0.0f) {
        o[0] /= o[2];
        o[1] /= o[2];
        o[2] /= o[2];
    }
    
}

void matrixVectorMulti(Vec3f &i, Vec3f &o, matrix &m) {
    o.x = m.m[0][0] * i[0] + m.m[1][0] * i[1] + m.m[2][0] * i[2] + m.m[3][0];
    o.y = m.m[0][1] * i[0] + m.m[1][1] * i[1] + m.m[2][1] * i[2] + m.m[3][1];
    o.z = m.m[0][2] * i[0] + m.m[1][2] * i[1] + m.m[2][2] * i[2] + m.m[3][2];
    float w = m.m[0][3] * i[0] + m.m[1][3] * i[1] + m.m[2][3] * i[2] + m.m[3][3];
    
    if (w != 0.0f) {
        o.x /= w;
        o.y /= w;
        o.z /= w;
    }
}




//Vec operations
Vec3f cross(const Vec3f &a, const Vec3f &b) {
    return Vec3f(a.y * b.z - a.z * b.y,
                 a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x);
}

Vec3f Vec4fToVec3f_aux(Vec4f vec4, Vec3f &new_vec3 ){
    new_vec3.x = vec4[0];
    new_vec3.y = vec4[1];
    new_vec3.z = vec4[2];
 
    return new_vec3; 
}

Vec3f* Vec4fToVec3f(Vec4f vec4[], Vec3f vec3[]){
    for(int i = 0; i < 3; i++){
        vec3[i] = Vec4fToVec3f_aux(vec4[i], vec3[i]);
    }
    return vec3;
}