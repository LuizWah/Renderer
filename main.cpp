#include <iostream>
#include "gl.h"

Model *model = NULL;
const int width  = 800;
const int height = 800;

//camera
Vec3f cameraPos(-1, 1, 2);//x = 0 does not create shit,z = 0 crashes    
Vec3f cameraTarget(0, 0, 0);     
Vec3f cameraUp(0, 1, 0);
Vec3f light_dir(-8, 3, 2);


struct Shader : public IShader{
    mat<2,3,float> varying_uv;
    mat<3,3,float> varying_nm;
    mat<4,3,float> varying_tri;

    matrix MatrixProjection;
    matrix viewMatrix;

    virtual Vec4f vertex(int iface, int nthvert) {
        
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        //varying_nm.set_col(nthvert, proj<3>((Projection*ModelView).invert_transpose()*embed<4>(model->normal(iface, nthvert), 0.f)));
        matrix Projected;
        matrix invert_transposed;
        Vec4f embed_normal; 
        Vec4f result;

        matrix_matrix_mult(MatrixProjection, viewMatrix, Projected);
        std::cout << "done";
        invert_transpose(Projected, invert_transposed);
        std::cout << "done";
        embed_normal = embed<4>(model->normal(iface, nthvert), 0.f);
        std::cout << "done";
        matrixVec4fMulti(embed_normal, result, invert_transposed);
        std::cout << "done";
        std::cout << "\n";

        Vec3f a;
        Vec4fToVec3f(&result, &a);
        varying_nm.set_col(nthvert, a);


        
        //Vec4f gl_Vertex = Projection*ModelView*embed<4>(model->vert(iface, nthvert));
        Vec4f embed_vert;
        embed_vert = embed<4>(model->vert(iface, nthvert));
        Vec4f gl_Vertex;
        matrixVec4fMulti(embed_vert, gl_Vertex, Projected);
        std::cout << "gl vertex " << gl_Vertex << " |";
        varying_tri.set_col(nthvert, gl_Vertex);

        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color) {
        Vec3f bn = (varying_nm*bar).normalize();
        Vec2f uv = varying_uv*bar;

        float diff = std::max(0.f, bn*light_dir);
        color = model->diffuse(uv)*diff;
        return false;
    }




};


int main(int argc, char **argv) {
    light_dir.normalize();


    if (2 == argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    } 

    //image creation
    TGAImage render(width, height, TGAImage::RGB);
    //buffer creation and population
    zbuffer Depthbuffer(width,height);

    //rotate to right orientation
    matrix rotationMatrix = createRotationMatrixZ();
    Vec3f rotatedUp;
    matrixVectorMulti(cameraUp, rotatedUp, rotationMatrix); 
    matrix viewMatrix = createViewMatrix(cameraPos, cameraTarget, rotatedUp);

    //perspective
    float zfar = 1000.f; 
    float znear = 0.1f;
    float fov = 60.f;
    float AspectRatio = (float) width / (float) height;


    //perspective projection calcs
    matrix MatrixProjection;
    MatrixProjection = ProjectionMatrixS(zfar, znear, fov, AspectRatio);
    
    //shader obj
    Shader shader;
    shader.MatrixProjection = MatrixProjection;
    shader.viewMatrix = viewMatrix;


    //main loop
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i); 

        Vec4f ProjectedVertices[3];

        Vec4f coords[3];
        float OGdepth[3];
        for (int j = 0; j < 3; j++) {
            
            coords[j] = shader.vertex(i,j);

       
            //change from coords to shader.varying_tri
            //change func to receive 4x3 matrices or make varying_tri in other format;
            WorldToScreen(coords, viewMatrix, MatrixProjection, ProjectedVertices, OGdepth, j, height, width); 
            
            
        }


        Vec3f vertices[3];
        Vec4fToVec3f(ProjectedVertices, vertices);

        TGAColor color;
        triangle_filled_depth(vertices, Depthbuffer.buffer, color, &render, shader, OGdepth);
      
    }

    //render.flip_vertically(); // Origin at the bottom left

    render.write_tga_file("wwwwwwwwwwwwwwwwww.tga");
    delete model;

    for (int i = 0; i < width; i++) {
        delete Depthbuffer.buffer[i];
    }
    delete Depthbuffer.buffer;

    return 0;
}