#include <iostream>
#include <vector>
#include <random>
#include <stdlib.h>
#include <omp.h> 

#include <glm/glm.hpp> 
#include "svpng.inc" 
#include "colors.hpp"
#include "progress_bar.hpp"

using namespace glm;
using namespace std;

const int SAMPLE = 2048;  // SPP

const double BRIGHTNESS = (2.0f * 3.1415926f) * (1.0f / double(SAMPLE));

const int WIDTH = 256;
const int HEIGHT = 256;

// Camera
const double SCREEN_Z = 1.1;
const vec3 EYE_POINT = vec3(0, 0, 4.0);


typedef struct Ray {
    vec3 startPoint = vec3(0, 0, 0); 
    vec3 direction = vec3(0, 0, 0); 
} Ray;

typedef struct Material {
    bool isEmissive = false;
    vec3 normal = vec3(0, 0, 0);
    vec3 color = vec3(0, 0, 0);

    double specularRate = 0.0f;
    double roughness = 1.0f; 

    double refractRate = 0.0f;
    double refractAngle = 1.0f;
    double refractRoughness = 0.0f;
} Material;

typedef struct HitResult {
    bool isHit = false; 
    double distance = 0.0f; 
    vec3 hitPoint = vec3(0, 0, 0);
    Material material;
} HitResult;

class Shape {
public:
    Shape() {}
    virtual HitResult intersect(Ray ray) { return HitResult(); }

};

class Triangle : public Shape {
public:
    Triangle() {}
    Triangle(vec3 P1, vec3 P2, vec3 P3, vec3 C) { 
        p1 = P1;
        p2 = P2; 
        p3 = P3; 
        material.normal = normalize(cross(p2 - p1, p3 - p1)); 
        material.color = C;
    }

    HitResult intersect(Ray ray) { 
        HitResult res;

        vec3 S = ray.startPoint;
        vec3 d = ray.direction;

        vec3 N = material.normal; 
        if (dot(N, d) > 0.0f) N = -N; 
        if (fabs(dot(N, d)) < 0.00001f) return res;

        float t = (dot(N, p1) - dot(S, N)) / dot(d, N);  // distance
        if (t < 0.0005f) return res; 

        
        vec3 P = S + d * t;

        // if P inline Triangle
        vec3 c1 = cross(p2 - p1, P - p1);
        vec3 c2 = cross(p3 - p2, P - p2);
        vec3 c3 = cross(p1 - p3, P - p3);
        vec3 n = material.normal; 
        if (dot(c1, n) < 0 || dot(c2, n) < 0 || dot(c3, n) < 0) return res;

        
        res.isHit = true;
        res.distance = t;
        res.hitPoint = P;
        res.material = material;
        res.material.normal = N; 
        return res; 
    };

public:
    vec3 p1, p2, p3; 
    Material material; 

};

class Sphere : public Shape {
public:
    Sphere() {}
    Sphere(vec3 o, double r, vec3 c) { O = o; R = r; material.color = c; }

    HitResult intersect(Ray ray) {
        HitResult res;

        vec3 S = ray.startPoint;
        vec3 d = ray.direction;

        float OS = length(O - S);
        float SH = dot(O - S, d);
        float OH = sqrt(pow(OS, 2) - pow(SH, 2));

        if (OH > R) return res;


        float PH = sqrt(pow(R, 2) - pow(OH, 2));

        float t1 = length(SH) - PH;
        float t2 = length(SH) + PH;
        float t = (t1 < 0) ? (t2) : (t1); 
        vec3 P = S + t * d;

        if (fabs(t1) < 0.0005f || fabs(t2) < 0.0005f) return res;

        res.isHit = true;
        res.distance = t;
        res.hitPoint = P;
        res.material = material;
        res.material.normal = normalize(P - O);
        return res;
    }

public:
    vec3 O; 
    double R; 
    Material material;

};


void imshow(double* SRC) {
    unsigned char* image = new unsigned char[WIDTH * HEIGHT * 3];
    unsigned char* p = image;
    double* S = SRC;

    FILE* fp;
    fopen_s(&fp, "path_tracing.png", "wb");

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            *p++ = (unsigned char)glm::clamp(pow(*S++, 1.0f / 2.2f) * 255, 0.0, 255.0);
            *p++ = (unsigned char)glm::clamp(pow(*S++, 1.0f / 2.2f) * 255, 0.0, 255.0);
            *p++ = (unsigned char)glm::clamp(pow(*S++, 1.0f / 2.2f) * 255, 0.0, 255.0);
        }
    }

    svpng(fp, WIDTH, HEIGHT, image, 0);
}


HitResult shoot(vector<Shape*>& shapes, Ray ray) {
    HitResult res, r;
    res.distance = DBL_MAX;

    for (auto& shape : shapes) {
        r = shape->intersect(ray);
        if (r.isHit && r.distance < res.distance) res = r;
    }

    return res;
}


std::uniform_real_distribution<> dis(0.0, 1.0);
random_device rd;
mt19937 gen(rd());
double randf() {
    return dis(gen);
}


// uint sphere
vec3 randomVec3() {
    vec3 d;
    do {
        d = 2.0f * vec3(randf(), randf(), randf()) - vec3(1, 1, 1);
    } while (dot(d, d) > 1.0);
    return normalize(d);
}

// normal sphere
vec3 randomDirection(vec3 n) {
    return normalize(randomVec3() + n);
}


vec3 pathTracing(vector<Shape*>& shapes, Ray ray, int depth) {
    if (depth > 8) return vec3(0);

    HitResult res = shoot(shapes, ray);
    if (!res.isHit) return vec3(0);
    if (res.material.isEmissive) return res.material.color;
   
    double r = randf();
    float P = 0.8;  // PDF
    if (r > P) return vec3(0);
    
    Ray randomRay;
    randomRay.startPoint = res.hitPoint;
    randomRay.direction = randomDirection(res.material.normal);
    
    vec3 color = vec3(0);
    float cosine = fabs(dot(-ray.direction, res.material.normal));

    r = randf();
    if (r < res.material.specularRate) {
        vec3 ref = normalize(reflect(ray.direction, res.material.normal));
        randomRay.direction = mix(ref, randomRay.direction, res.material.roughness);
        color = pathTracing(shapes, randomRay, depth + 1) * cosine;
    }
    else if (res.material.specularRate <= r && r <= res.material.refractRate) {
        vec3 ref = normalize(refract(ray.direction, res.material.normal, float(res.material.refractAngle)));
        randomRay.direction = mix(ref, -randomRay.direction, res.material.refractRoughness);
        color = pathTracing(shapes, randomRay, depth + 1) * cosine;
    }
    else {
        vec3 srcColor = res.material.color;
        vec3 ptColor = pathTracing(shapes, randomRay, depth+1) * cosine;
        color = ptColor * srcColor;
    }

    return color / P;
}


int main() {
    vector<Shape*> shapes; 

    Sphere s1 = Sphere(vec3(-0.65, -0.7, 0.0), 0.3, cg::Color::Pink);
    Sphere s2 = Sphere(vec3(0.0, -0.3, 0.0), 0.4, cg::Color::White);
    Sphere s3 = Sphere(vec3(0.65, 0.1, 0.0), 0.3, cg::Color::LavenderBlush);
    s1.material.specularRate = 0.3;
    s1.material.roughness = 0.1;

    s2.material.specularRate = 0.3;
    s2.material.refractRate = 0.95;
    s2.material.refractAngle = 0.1;

    s3.material.specularRate = 0.6;

    shapes.push_back(&s1);
    shapes.push_back(&s2);
    shapes.push_back(&s3);

    shapes.push_back(new Triangle(vec3(-0.15, 0.4, -0.6), vec3(-0.15, -0.95, -0.6), vec3(0.15, 0.4, -0.6), cg::Color::AliceBlue));
    shapes.push_back(new Triangle(vec3(0.15, 0.4, -0.6), vec3(-0.15, -0.95, -0.6), vec3(0.15, -0.95, -0.6), cg::Color::AliceBlue));
    
    // emissive material
    Triangle l1 = Triangle(vec3(0.4, 0.99, 0.4), vec3(-0.4, 0.99, -0.4), vec3(-0.4, 0.99, 0.4), cg::Color::White);
    Triangle l2 = Triangle(vec3(0.4, 0.99, 0.4), vec3(0.4, 0.99, -0.4), vec3(-0.4, 0.99, -0.4), cg::Color::White);
    l1.material.isEmissive = true;
    l2.material.isEmissive = true;
    shapes.push_back(&l1);
    shapes.push_back(&l2);

    // bottom
    shapes.push_back(new Triangle(vec3(1, -1, 1), vec3(-1, -1, -1), vec3(-1, -1, 1), cg::Color::White));
    shapes.push_back(new Triangle(vec3(1, -1, 1), vec3(1, -1, -1), vec3(-1, -1, -1), cg::Color::White));
    // top
    shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(-1, 1, 1), vec3(-1, 1, -1), cg::Color::White));
    shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(-1, 1, -1), vec3(1, 1, -1), cg::Color::White));
    // back
    shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(-1, 1, -1), vec3(-1, -1, -1), cg::Color::Cyan));
    shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(1, 1, -1), vec3(-1, 1, -1), cg::Color::Cyan));
    // left
    shapes.push_back(new Triangle(vec3(-1, -1, -1), vec3(-1, 1, 1), vec3(-1, -1, 1), cg::Color::SpringGreen));
    shapes.push_back(new Triangle(vec3(-1, -1, -1), vec3(-1, 1, -1), vec3(-1, 1, 1), cg::Color::SpringGreen));
    // right
    shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(1, -1, -1), vec3(1, -1, 1), cg::Color::Red));
    shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(1, 1, 1), vec3(1, 1, -1), cg::Color::Red));
    
    
    double* image = new double[WIDTH * HEIGHT * 3];
    memset(image, 0.0, sizeof(double) * WIDTH * HEIGHT * 3);
    
    progressbar bar(SAMPLE);
    omp_set_num_threads(50); 
    #pragma omp parallel for
    for (int k = 0; k < SAMPLE; k++) {
        bar.update();
        double* p = image;
        for (int i = 0; i < HEIGHT; i++) {
            for (int j = 0; j < WIDTH; j++) {
                // pixel cord to plane cord
                double x = 2.0 * double(j) / double(WIDTH) - 1.0;
                double y = 2.0 * double(HEIGHT - i) / double(HEIGHT) - 1.0;

                // AA
                x += (randf() - 0.5f) / double(WIDTH);
                y += (randf() - 0.5f) / double(HEIGHT);

                vec3 coord = vec3(x, y, SCREEN_Z); // projective plane cord
                vec3 direction = normalize(coord - EYE_POINT); // eye direction

                Ray ray;
                ray.startPoint = coord;
                ray.direction = direction;

                HitResult res = shoot(shapes, ray);
                vec3 color = vec3(0, 0, 0);

                if (res.isHit) {
                    if (res.material.isEmissive) {
                        color = res.material.color;
                    }
                    else {
                        Ray randomRay;
                        randomRay.startPoint = res.hitPoint;
                        randomRay.direction = randomDirection(res.material.normal);

                        double r = randf();
                        if (r < res.material.specularRate) {
                            vec3 ref = normalize(reflect(ray.direction, res.material.normal));
                            randomRay.direction = mix(ref, randomRay.direction, res.material.roughness);
                            color = pathTracing(shapes, randomRay, 0);
                        }
                        else if (res.material.specularRate <= r && r <= res.material.refractRate) {
                            vec3 ref = normalize(refract(ray.direction, res.material.normal, float(res.material.refractAngle)));
                            randomRay.direction = mix(ref, -randomRay.direction, res.material.refractRoughness);
                            color = pathTracing(shapes, randomRay, 0);
                        }
                        else {
                            vec3 srcColor = res.material.color;
                            vec3 ptColor = pathTracing(shapes, randomRay, 0);
                            color = ptColor * srcColor;
                        }
                        color *= BRIGHTNESS;
                    }
                }

                *p += color.x; p++;
                *p += color.y; p++;
                *p += color.z; p++;
            }
        }
    }
    
    imshow(image);

    return 0;
}