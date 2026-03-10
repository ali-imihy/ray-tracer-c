#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 512
#define TEX_BUFSIZE 2048

struct Point {
  double x;
  double y;     // point struct
  double z;
};

struct Pair {
  double x;
  double y;
};

struct Color {
  double r, g, b;
};

struct RayType {
  double x, y, z;       // ray struct
  double dx, dy, dz;   
};

struct TriangleType {
  int v1, v2, v3;
  int vn1, vn2, vn3;
  int vt1, vt2, vt3;
  double r, g, b;           // sphere struct
  double sr, sg, sb;   
  double ka, kd, ks;   
  double n; 
  int smooth;
  int texture_idx;
  int t_height, t_width;
  double opacity, refraction;
};

struct SphereType {
  double x, y, z;      
  double radius;
  double r, g, b;           // sphere struct
  double sr, sg, sb;   
  double ka, kd, ks;   
  double n; 
  int texture_idx;
  int t_width, t_height;
  double opacity, refraction;
};

struct LightType {
  double x, y, z, w, i;      // light struct
  double c1, c2, c3;  
  int att;
};

void normalize(double *x, double *y, double *z) {
  double length = sqrt((*x * *x) + (*y * *y) + (*z * *z));
  if (length != 0) {        // normalize function to make life easier
    *x /= length;
    *y /= length;
    *z /= length;
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s input_file\n", argv[0]);
    return 1;
  }
  FILE *fp = fopen(argv[1], "r");
  if (fp == NULL) {
    printf("File does not exist.\n");   //try again
    return 1;
  }
  char output[BUFSIZE];
  strncpy(output, argv[1], BUFSIZE);
  char *dot = strrchr(output, '.');
  if (dot) strcpy(dot, ".ppm");         // same file name just with ppm now
  char buf[BUFSIZE];
  double eyex = 0, eyey = 0, eyez = 0;
  double viewx = 0, viewy = 0, viewz = -1;
  double upx = 0, upy = 1, upz = 0;
  double vfov = 90;
  int width = 0, height = 0;
  double bkgr, bkgg, bkgb, bkgRef;      // whole bunch of variables needed for calculations to get from file
  double mtlr, mtlg, mtlb, mtlsr, mtlsg, mtlsb;
  double ka, kd, ks, n;
  int sphere_count = 0;
  int light_count = 0;
  int parallel = 0;
  double dcr, dcg, dcb, amin, amax, dmin, dmax, adc = 0;    // depth cueing variables
  int depthcueing = 0;    //dc flag
  double opacity = 0;
  double refraction = 0;
  struct SphereType spheres[BUFSIZE] = {0};
  struct LightType lights[BUFSIZE] = {0};   // arrays for lights and spheres
  struct Point vertices[BUFSIZE] = {0};
  struct Point normals[BUFSIZE] = {0};
  struct Pair coords[BUFSIZE] = {0};
  int coords_index = 1;
  int vertices_index = 1;
  int normal_index = 1;
  struct TriangleType triangles[BUFSIZE] = {0};
  int triangle_count = 0;
  char file_name[BUFSIZE] = "";
  struct Color ***textures = malloc(7 * sizeof(struct Color **));
  for (int l = 0; l < 7; l++) {
      textures[l] = malloc(2048 * sizeof(struct Color *));
      for (int x = 0; x < 2048; x++) {
          textures[l][x] = malloc(2048 * sizeof(struct Color));
      }
  }
  int texture_index = -1;
  int texture_height, texture_width;
  while (fscanf(fp, "%s", buf) == 1) {
    if (strcmp(buf, "parallel") == 0) {
      parallel = 1;
    } else if (strcmp(buf, "eye") == 0) {
      fscanf(fp, "%lf %lf %lf", &eyex, &eyey, &eyez);
    } else if (strcmp(buf, "viewdir") == 0) {
      fscanf(fp, "%lf %lf %lf", &viewx, &viewy, &viewz);    // read input from file and assign vars accordingly
    } else if (strcmp(buf, "updir") == 0) {
      fscanf(fp, "%lf %lf %lf", &upx, &upy, &upz);
    } else if (strcmp(buf, "vfov") == 0) {
      fscanf(fp, "%lf", &vfov);
    } else if (strcmp(buf, "imsize") == 0) {
      fscanf(fp, "%d %d", &width, &height);
    } else if (strcmp(buf, "bkgcolor") == 0) {
      fscanf(fp, "%lf %lf %lf %lf", &bkgr, &bkgg, &bkgb, &bkgRef);
    } else if (strcmp(buf, "mtlcolor") == 0) {
      fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf", &mtlr, &mtlg, &mtlb, &mtlsr, &mtlsg, &mtlsb, &ka, &kd, &ks, &n, &opacity, &refraction);
    } else if (strcmp(buf, "texture") == 0) {
      fscanf(fp, "%s", file_name);
      int tex_width, tex_height, max_color;
      char p[10];
      FILE *texture = fopen(file_name, "r");
      fscanf(texture, "%s %d %d %d", p, &tex_width, &tex_height, &max_color);
      texture_height = tex_height;
      texture_width = tex_width;
      int rt, gt, bt;
      for (int e = 0; e < tex_height; e++) {
          for (int t = 0; t < tex_width; t++) {
              fscanf(texture, "%d %d %d", &rt, &gt, &bt);
              textures[texture_index + 1][t][e].r = rt;
              textures[texture_index + 1][t][e].g = gt;
              textures[texture_index + 1][t][e].b = bt;
          }
      }
      texture_index++;
      fclose(texture);
    } else if (strcmp(buf, "sphere") == 0) {
      struct SphereType *sphere = &spheres[sphere_count];
      fscanf(fp, "%lf %lf %lf %lf", &sphere->x, &sphere->y, &sphere->z, &sphere->radius);
      sphere->texture_idx = -1;
      sphere->r = mtlr; 
      sphere->g = mtlg; 
      sphere->b = mtlb;
      sphere->sr = mtlsr;   // assign sphere struct accordingly
      sphere->sg = mtlsg; 
      sphere->sb = mtlsb;
      sphere->ka = ka; 
      sphere->kd = kd; 
      sphere->ks = ks; 
      sphere->n = n;
      sphere->opacity = opacity;
      sphere->refraction = refraction;
      if (texture_index != -1) {
        sphere->texture_idx = texture_index;
        sphere->t_height = texture_height;
        sphere->t_width = texture_width;
      }
      sphere_count++;
    } else if (strcmp(buf, "light") == 0) {
      struct LightType *light = &lights[light_count];
      fscanf(fp, "%lf %lf %lf %lf %lf", &light->x, &light->y, &light->z, &light->w, &light->i);
      light->att = 0;
      light_count++;
    } else if (strcmp(buf, "attlight") == 0) {    // if light attun is enabled check the flag in the struct
      struct LightType *light = &lights[light_count];
      fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf %lf", &light->x, &light->y, &light->z, &light->w, &light->i, 
              &light->c1, &light->c2, &light->c3);
      light->att = 1;
      light_count++;
    } else if (strcmp(buf, "depthcueing") == 0) {
      fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf", &dcr, &dcg, &dcb, &amin, &amax, &dmin, &dmax);
      depthcueing = 1;
    } else if (strcmp(buf, "v") == 0) {
      struct Point *vertex = &vertices[vertices_index];
      fscanf(fp, "%lf %lf %lf", &vertex->x, &vertex->y, &vertex->z);
      vertices_index++;
    } else if (strcmp(buf, "vn") == 0) {
      struct Point *normal = &normals[normal_index];
      fscanf(fp, "%lf %lf %lf", &normal->x, &normal->y, &normal->z);
      normal_index++;
    } else if (strcmp(buf, "vt") == 0) {
      struct Pair *tcoords = &coords[coords_index];
      fscanf(fp, "%lf %lf", &tcoords->x, &tcoords->y);
      coords_index++;
    }
     else if (strcmp(buf, "f") == 0) {
      struct TriangleType *triangle = &triangles[triangle_count];
      triangle->smooth = 0;
      fgets(buf, BUFSIZE, fp);
      if (sscanf(buf, "%d/%d/%d %d/%d/%d %d/%d/%d", &triangle->v1, &triangle->vt1, &triangle->vn1, &triangle->v2, &triangle->vt2, &triangle->vn2, &triangle->v3, &triangle->vt3, &triangle->vn3) == 9) {
        triangle->texture_idx = -1;
        triangle->r = mtlr; 
        triangle->g = mtlg; 
        triangle->b = mtlb;
        triangle->sr = mtlsr;   
        triangle->sg = mtlsg; 
        triangle->sb = mtlsb;
        triangle->ka = ka; 
        triangle->kd = kd; 
        triangle->ks = ks; 
        triangle->n = n;
        triangle->smooth = 1;
        triangle->opacity = opacity;
        triangle->refraction = refraction;
        if (texture_index != -1) {
          triangle->texture_idx = texture_index;
          triangle->t_height = texture_height;
          triangle->t_width = texture_width;
        }
        triangle_count++;
      } else if (sscanf(buf, "%d//%d %d//%d %d//%d", &triangle->v1, &triangle->vn1, &triangle->v2, &triangle->vn2, &triangle->v3, &triangle->vn3) == 6) {
        triangle->texture_idx = -1;
        triangle->r = mtlr; 
        triangle->g = mtlg; 
        triangle->b = mtlb;
        triangle->sr = mtlsr;   
        triangle->sg = mtlsg; 
        triangle->sb = mtlsb;
        triangle->ka = ka; 
        triangle->kd = kd; 
        triangle->ks = ks; 
        triangle->n = n;
        triangle->smooth = 1;
        triangle->opacity = opacity;
        triangle->refraction = refraction;
        triangle_count++;
      } else if (sscanf(buf, "%d/%d %d/%d %d/%d", &triangle->v1, &triangle->vt1, &triangle->v2, &triangle->vt2, &triangle->v3, &triangle->vt3 ) == 6) {
        triangle->texture_idx = -1;
        triangle->r = mtlr; 
        triangle->g = mtlg; 
        triangle->b = mtlb;
        triangle->sr = mtlsr;   
        triangle->sg = mtlsg; 
        triangle->sb = mtlsb;
        triangle->ka = ka; 
        triangle->kd = kd; 
        triangle->ks = ks; 
        triangle->n = n;
        triangle->opacity = opacity;
        triangle->refraction = refraction;
        if (texture_index != -1) {
          triangle->texture_idx = texture_index;
          triangle->t_height = texture_height;
          triangle->t_width = texture_width;
        }
        triangle_count++;
      } else if (sscanf(buf, "%d %d %d", &triangle->v1, &triangle->v2, &triangle->v3) == 3) {
        triangle->texture_idx = -1;
        triangle->r = mtlr; 
        triangle->g = mtlg; 
        triangle->b = mtlb;
        triangle->sr = mtlsr;   
        triangle->sg = mtlsg; 
        triangle->sb = mtlsb;
        triangle->ka = ka; 
        triangle->kd = kd; 
        triangle->ks = ks; 
        triangle->n = n;
        triangle->opacity = opacity;
        triangle->refraction = refraction;
        triangle_count++;
      } else {
        printf("failed to read f\n");
        return 1;
      }      
    }
  }
  fclose(fp);
  if (width < 1 || height < 1 || vfov <= 0) {     // error checking
    printf("Invalid input\n");
    return 1;
  }
  double ux, uy, uz; 
  double vx, vy, vz; 
  double wx, wy, wz;  
  wx = -viewx; wy = -viewy; wz = -viewz;
  ux = viewy * upz - viewz * upy;
  uy = viewz * upx - viewx * upz;
  uz = viewx * upy - viewy * upx;
  normalize(&ux, &uy, &uz);
  vx = uy * viewz - uz * viewy;
  vy = uz * viewx - ux * viewz;
  vz = ux * viewy - uy * viewx;
  normalize(&vx, &vy, &vz);
  double view_distance = 20;
  double window_height = 2 * view_distance * tan(vfov * M_PI / 360.0);    // window calculations
  double window_width = window_height * width / height;
  struct Point ul, ur, ll, lr;
  double nx, ny, nz;
  if (parallel) {
    double halfw = window_width / 2;
    double halfh = window_height / 2;       // if parallel set corners at the eye
    ul.x = eyex - halfw * ux + halfh * vx;
    ul.y = eyey - halfw * uy + halfh * vy;
    ul.z = eyez - halfw * uz + halfh * vz;
  } else {
    double center_x = eyex + view_distance * viewx;
    double center_y = eyey + view_distance * viewy;   // else set at the window 
    double center_z = eyez + view_distance * viewz;
    double halfw = window_width / 2;
    double halfh = window_height / 2;
    ul.x = center_x - halfw * ux + halfh * vx;
    ul.y = center_y - halfw * uy + halfh * vy;    // only need upper left, every other point can be obtained by math
    ul.z = center_z - halfw * uz + halfh * vz;
  }
  FILE* ppm = fopen(output, "w");
  if (ppm == NULL) {
    printf("fopen failed\n");
    return 1;
  }
  double shadow = 1.0;
  double alpha, beta, gamma;
  fprintf(ppm, "P3\n%d %d\n255\n", width, height);      // print to ppm file necessary headers
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {  // For every pixel
        shadow = 1;
        double currx = ul.x + j * window_width / (width - 1);
        double curry = ul.y - i * window_height / (height - 1);
        double currz = ul.z;
        struct RayType ray;
        ray.x = eyex;
        ray.y = eyey;
        ray.z = eyez;
        if (parallel) {
            ray.dx = viewx;
            ray.dy = viewy;
            ray.dz = viewz;
            normalize(&ray.dx, &ray.dy, &ray.dz);
        } else {
            ray.dx = currx - ray.x;
            ray.dy = curry - ray.y;
            ray.dz = currz - ray.z;
            normalize(&ray.dx, &ray.dy, &ray.dz);
        }
        double min_t = INFINITY;
        int closest_object = -1;
        int is_triangle = 0;
        for (int k = 0; k < triangle_count; k++) {
            struct TriangleType *triangle = &triangles[k];
            struct Point *p0 = &vertices[triangle->v1];
            struct Point *p1 = &vertices[triangle->v2];
            struct Point *p2 = &vertices[triangle->v3];
            double e1x = p1->x - p0->x;
            double e1y = p1->y - p0->y;
            double e1z = p1->z - p0->z;
            double e2x = p2->x - p0->x;
            double e2y = p2->y - p0->y;
            double e2z = p2->z - p0->z;
            double nx = e1y * e2z - e1z * e2y;
            double ny = e1z * e2x - e1x * e2z;
            double nz = e1x * e2y - e1y * e2x;
            double D = -(nx * p0->x + ny * p0->y + nz * p0->z);
            double denom = nx * ray.dx + ny * ray.dy + nz * ray.dz;
            if (fabs(denom) > 1e-6) {
                double t = -(nx * ray.x + ny * ray.y + nz * ray.z + D) / denom;
                if (t > 0 && t < min_t) {
                    double interx = ray.x + ray.dx * t;
                    double intery = ray.y + ray.dy * t;
                    double interz = ray.z + ray.dz * t;
                    double epx = interx - p0->x;
                    double epy = intery - p0->y;
                    double epz = interz - p0->z;
                    double d11 = e1x * e1x + e1y * e1y + e1z * e1z;
                    double d22 = e2x * e2x + e2y * e2y + e2z * e2z;
                    double d12 = e1x * e2x + e1y * e2y + e1z * e2z;
                    double d1p = e1x * epx + e1y * epy + e1z * epz;
                    double d2p = e2x * epx + e2y * epy + e2z * epz;
                    double determinant = d11 * d22 - d12 * d12;
                    if (fabs(determinant) > 1e-6) { 
                        beta = (d22 * d1p - d12 * d2p) / determinant;
                        gamma = (d11 * d2p - d12 * d1p) / determinant;
                        alpha = 1.0 - beta - gamma;
                        if (alpha >= 0 && beta >= 0 && gamma >= 0 &&
                            alpha <= 1 && beta <= 1 && gamma <= 1) {
                            min_t = t;
                            is_triangle = 1;
                            closest_object = k;
                        }
                    }
                }
            }
        } 
      for (int k = 0; k < sphere_count; k++) {
        struct SphereType *sphere = &spheres[k];      // for every sphere see if this ray intersects it
        double a = ray.dx * ray.dx + ray.dy * ray.dy + ray.dz * ray.dz;
        double b = 2 * (ray.dx * (ray.x - sphere->x) + ray.dy * (ray.y - sphere->y) + ray.dz * (ray.z - sphere->z));
        double c = pow(ray.x - sphere->x, 2) + pow(ray.y - sphere->y, 2) +
                    pow(ray.z - sphere->z, 2) - pow(sphere->radius, 2);   // quadratic formula shenanigans
        double deter = (b * b) - (4 * a * c);
        if (deter >= 0) {
          double t1 = (-b + sqrt(deter)) / (2 * a);     // only get here if there was a valid intersection
          double t2 = (-b - sqrt(deter)) / (2 * a);
          double t = (t1 < t2 && t1 > 0) ? t1 : t2;
          if (t > 0 && t < min_t) {     // find out if this intersection is the closest and assign that sphere to closest
            min_t = t;
            closest_object = k;
            is_triangle = 0;
          }
        }
      }
      double Ir = bkgr, Ig = bkgg, Ib = bkgb;     // initially set every pixel to the background color
      void *object;
          if (closest_object >= 0) {
            if (is_triangle == 1) {
              object = &triangles[closest_object];
            } else {
              object = &spheres[closest_object];   // if closest sphere exists, time to calculate lighting
            }
            double interx = ray.x + ray.dx * min_t;
            double intery = ray.y + ray.dy * min_t;   // intersection on the sphere
            double interz = ray.z + ray.dz * min_t;
            if (is_triangle == 0) {
              struct SphereType *sphere = (struct SphereType *)object;
              nx = (interx - sphere->x) / sphere->radius;
              ny = (intery - sphere->y) / sphere->radius;      // N vector
              nz = (interz - sphere->z) / sphere->radius;
              
            }
            normalize(&nx, &ny, &nz);
            if (is_triangle == 1) {
              struct TriangleType *triangle = (struct TriangleType *)object;
              if (triangle->texture_idx != -1) {
                struct Pair coord1 = coords[triangle->vt1];
                struct Pair coord2 = coords[triangle->vt2];
                struct Pair coord3 = coords[triangle->vt3];
                double u = alpha * coord1.x + beta * coord2.x + gamma * coord3.x;
                double v = alpha * coord1.y + beta * coord2.y + gamma * coord3.y;
                int i = round(u * (triangle->t_width - 1));
                if (i < 0) {
                  i = 0;
                }
                if (v < 0) {
                  v = 0;
                }
                int j = round(v * (triangle->t_height - 1));
                struct Color color = textures[triangle->texture_idx][i][j];
                triangle->r = color.r / 225.0;
                triangle->g = color.g / 255.0;
                triangle->b = color.b / 255.0;
                Ir = color.r / 255.0;
                Ig = color.g / 255.0;
                Ib = color.b / 255.0;

              } else {
                Ir = triangle->ka * triangle->r;
                Ig = triangle->ka * triangle->g;    // set I to ambient initially
                Ib = triangle->ka * triangle->b;
              }
            } else {
              struct SphereType *sphere = (struct SphereType *)object;
              if (sphere->texture_idx != -1) {
                double fi = acos(nz);
                double theta = atan2(ny, nx);
                double v = fi / M_PI;
                double u = (theta + M_PI) / (2 * M_PI);
                int i = round(u * (sphere->t_width - 1));
                int j = round(v * (sphere->t_height - 1));
                struct Color color = textures[sphere->texture_idx][i][j];
                sphere->r = color.r / 225.0;
                sphere->g = color.g / 255.0;
                sphere->b = color.b / 255.0;
                Ir = color.r / 255.0;
                Ig = color.g / 255.0;
                Ib = color.b / 255.0;
              } else {
                Ir = sphere->ka * sphere->r;
                Ig = sphere->ka * sphere->g;    // set I to ambient initially
                Ib = sphere->ka * sphere->b;
              }
            }
            for (int l = 0; l < light_count; l++) {
              struct LightType *light = &lights[l];
              double Lx, Ly, Lz;
              if (light->w == 0) {
                Lx = -light->x;
                Ly = -light->y;     // L vector is pointing towards the light if directional
                Lz = -light->z;
              } else {
                Lx = light->x - interx;     // if point light L is then calculate from the surface to the point
                Ly = light->y - intery;
                Lz = light->z - interz;
              }
              normalize(&Lx, &Ly, &Lz);   // normalize
              shadow = 1.0;   // shadow flag
              double epsilon = 0.001;     // epsilon because C is weird
              struct RayType shadow_ray;
              shadow_ray.x = interx + epsilon * nx;
              shadow_ray.y = intery + epsilon * ny;   // shadow ray is the intersection times the N vector
              shadow_ray.z = interz + epsilon * nz;
              shadow_ray.dx = Lx;
              shadow_ray.dy = Ly;   // dx is Lx 
              shadow_ray.dz = Lz;
              double light_distance = INFINITY;     // initialize 
              if (light->w != 0) {
                light_distance = sqrt(pow(light->x - interx, 2) +     // if intensity is enabled find the distance
                                    pow(light->y - intery, 2) + pow(light->z - interz, 2));
              }
              for (int k = 0; k < sphere_count; k++) {    // for each sphere check if theres an intersection between the light and original sphere
                if (is_triangle == 0) {
                  if (k == closest_object) continue;  // dont check this sphere
                }
                struct SphereType *shadow_sphere = &spheres[k];
                double a = pow(shadow_ray.dx, 2) + pow(shadow_ray.dy, 2) + pow(shadow_ray.dz, 2);
                double b = 2 * (shadow_ray.dx * (shadow_ray.x - shadow_sphere->x) + 
                              shadow_ray.dy * (shadow_ray.y - shadow_sphere->y) + // quadratic formula :)
                              shadow_ray.dz * (shadow_ray.z - shadow_sphere->z));
                double c = pow(shadow_ray.x - shadow_sphere->x, 2) + pow(shadow_ray.y - shadow_sphere->y, 2) +
                          pow(shadow_ray.z - shadow_sphere->z, 2) - pow(shadow_sphere->radius, 2);
                double deter = b * b - 4 * a * c;
                if (deter >= 0) {
                  double t = (-b - sqrt(deter)) / (2 * a);    // if intersection is found set and directional light is enabled check if the
                  if (t > epsilon && (light->w == 0 || t < light_distance)) { // object is inbetween the distance, if it is its a shadow.
                    shadow = 0.0;   // set shadow flag
                    break;
                  }
                }
              }
              for (int k = 0; k < triangle_count; k++) {
                if (is_triangle == 1) {
                  if (k == closest_object) continue;
                }
                struct TriangleType *triangle = &triangles[k];
                struct Point *p0 = &vertices[triangle->v1];
                struct Point *p1 = &vertices[triangle->v2];
                struct Point *p2 = &vertices[triangle->v3];
                double e1x = p1->x - p0->x;
                double e1y = p1->y - p0->y;
                double e1z = p1->z - p0->z;
                double e2x = p2->x - p0->x;
                double e2y = p2->y - p0->y;
                double e2z = p2->z - p0->z;
                double nx = e1y * e2z - e1z * e2y;
                double ny = e1z * e2x - e1x * e2z;
                double nz = e1x * e2y - e1y * e2x;
                double D = -(nx * p0->x + ny * p0->y + nz * p0->z);
                double denom = nx * shadow_ray.dx + ny * shadow_ray.dy + nz * shadow_ray.dz;
                if (fabs(denom) > 1e-6) {
                  double t = -(nx * shadow_ray.x + ny * shadow_ray.y + nz * shadow_ray.z + D) / denom;
                  if (t > 0) {
                    double shadow_interx = shadow_ray.x + shadow_ray.dx * t;
                    double shadow_intery = shadow_ray.y + shadow_ray.dy * t;
                    double shadow_interz = shadow_ray.z + shadow_ray.dz * t;
                    double epx = shadow_interx - p0->x;
                    double epy = shadow_intery - p0->y;
                    double epz = shadow_interz - p0->z;
                    double d11 = e1x * e1x + e1y * e1y + e1z * e1z;
                    double d22 = e2x * e2x + e2y * e2y + e2z * e2z;
                    double d12 = e1x * e2x + e1y * e2y + e1z * e2z;
                    double d1p = e1x * epx + e1y * epy + e1z * epz;
                    double d2p = e2x * epx + e2y * epy + e2z * epz;
                    double determinant = (d11 * d22 - d12 * d12);
                    if (fabs(determinant) > 1e-6) { 
                        double shadow_beta = (d22 * d1p - d12 * d2p) / determinant;
                        double shadow_gamma = (d11 * d2p - d12 * d1p) / determinant;
                        double shadow_alpha = 1.0 - shadow_beta - shadow_gamma;
                        if (shadow_alpha >= 0 && shadow_beta >= 0 && shadow_gamma >= 0 &&
                            shadow_alpha <= 1 && shadow_beta <= 1 && shadow_gamma <= 1) {
                            if (t > epsilon && (light->w == 0 || t < light_distance)) { // object is inbetween the distance, if it is its a shadow.
                              shadow = 0.0;   // set shadow flag
                              break;
                            }
                        }
                    }
                  }
                }
              }
              if (shadow > 0) {
                  double Vx = eyex - interx;
                  double Vy = eyey - intery;    // if theres no shadow calculate the diffuse and specular terms
                  double Vz = eyez - interz;
                  normalize(&Vx, &Vy, &Vz);
                  double Hx = Lx + Vx;
                  double Hy = Ly + Vy;      // calculate V and H vectors and normalize
                  double Hz = Lz + Vz;
                  normalize(&Hx, &Hy, &Hz);
                  if (is_triangle == 0) {
                    struct SphereType *sphere = (struct SphereType *)object;
                    nx = (interx - sphere->x) / sphere->radius;
                    ny = (intery - sphere->y) / sphere->radius;      // N vector
                    nz = (interz - sphere->z) / sphere->radius;
                  } else {
                    struct TriangleType *triangle = (struct TriangleType *)object;
                    if (triangle->smooth == 1) {
                      struct Point *vn1 = &normals[triangle->vn1];
                      struct Point *vn2 = &normals[triangle->vn2];
                      struct Point *vn3 = &normals[triangle->vn3];
                      nx = (alpha * vn1->x + beta * vn2->x + gamma * vn3->x);
                      ny = (alpha * vn1->y + beta * vn2->y + gamma * vn3->y);
                      nz = (alpha * vn1->z + beta * vn2->z + gamma * vn3->z);
                    }
                    else {
                      struct Point *p0 = &vertices[triangle->v1];
                      struct Point *p1 = &vertices[triangle->v2];
                      struct Point *p2 = &vertices[triangle->v3];
                      double e1x = p1->x - p0->x;
                      double e1y = p1->y - p0->y;
                      double e1z = p1->z - p0->z;
                      double e2x = p2->x - p0->x;
                      double e2y = p2->y - p0->y;
                      double e2z = p2->z - p0->z;
                      nx = e1y * e2z - e1z * e2y;
                      ny = e1z * e2x - e1x * e2z;
                      nz = e1x * e2y - e1y * e2x;    
                    }                
                  }
                  normalize(&nx, &ny, &nz);
                  double ndotl = fmax(0, nx * Lx + ny * Ly + nz * Lz);    // N dot L
                  double ndoth = fmax(0, nx * Hx + ny * Hy + nz * Hz);     // N dot H
                  double intensity = light->i;
                  if (light->att == 1) {    // if light attunment is enabled find the new intensity with c1, c2, and c3
                      double distance = sqrt(pow(interx - light->x, 2) + pow(intery - light->y, 2) + 
                                        pow(interz - light->z, 2));
                      intensity *= 1.0 / (light->c1 + light->c2 * distance + light->c3 * pow(distance, 2));
                  }
                  if (is_triangle == 1) {
                    struct TriangleType *triangle = (struct TriangleType *)object;
                    Ir += intensity * (triangle->kd * triangle->r * ndotl 
                        + triangle->ks * triangle->sr * pow(ndoth, triangle->n));   // tack on diffuse and specular terms onto the ambient
                    Ig += intensity * (triangle->kd * triangle->g * ndotl + 
                          triangle->ks * triangle->sg * pow(ndoth, triangle->n));
                    Ib += intensity * (triangle->kd * triangle->b * ndotl + 
                          triangle->ks * triangle->sb * pow(ndoth, triangle->n));
                  } else {
                    struct SphereType *sphere = (struct SphereType *)object;
                    Ir += intensity * (sphere->kd * sphere->r * ndotl 
                        + sphere->ks * sphere->sr * pow(ndoth, sphere->n));   // tack on diffuse and specular terms onto the ambient
                    Ig += intensity * (sphere->kd * sphere->g * ndotl + 
                          sphere->ks * sphere->sg * pow(ndoth, sphere->n));
                    Ib += intensity * (sphere->kd * sphere->b * ndotl + 
                          sphere->ks * sphere->sb * pow(ndoth, sphere->n));
                  }
              }
            }
            if (depthcueing == 1) {   // if depthcueing is enabled calculate the distance and find values accordingly
                double objdist = sqrt(pow(eyex - interx, 2) + pow(eyey - intery, 2) + 
                                    pow(eyez - interz, 2));
                if (objdist <= dmin) {
                    adc = amax;
                } else if (objdist >= dmax) {
                    adc = amin;
                } else {
                    adc = amin + (amax - amin) * ((dmax - objdist) / (dmax - dmin));    
                }                                   // if its in between distance max and min use the inbetween function
                Ir = adc * Ir + (1 - adc) * dcr;
                Ig = adc * Ig + (1 - adc) * dcg;    // find new I 
                Ib = adc * Ib + (1 - adc) * dcb;
            }
            double Ix = -ray.dx;
            double Iy = -ray.dy;
            double Iz = -ray.dz;
            double ndoti = Ix * nx + Iy * ny + Iz * nz;
            double Rdx = 2.0 * ndoti * nx - Ix;
            double Rdy = 2.0 * ndoti * ny - Iy;
            double Rdz = 2.0 * ndoti * nz - Iz;
            double currOpa;
            double currRef;
            double Rr, Rg, Rb;
            double Rx = interx + 0.001 * nx;
            double Ry = intery + 0.001 * ny;
            double Rz = interz + 0.001 * nz;
            double ks_flag = 0;
            double intersection_flag = 0;
            if (is_triangle == 1) {
              struct TriangleType *triangle = (struct TriangleType *)object;
              currOpa = triangle->opacity;
              currRef = triangle->refraction;
              if (triangle->ks == 0) {
                ks_flag = 1;
              }
            } else {
              struct SphereType *sphere = (struct SphereType *)object;
              currOpa = sphere->opacity;
              currRef = sphere->refraction;
              if (sphere->ks == 0) {
                ks_flag = 1;
              }
            }
            double f0 = pow(((currRef - 1.0) / (currRef + 1.0)), 2);
            if (ks_flag == 1) {
              f0 = 0;
            }
            double fr = f0 + (1 - f0) * pow(1 - ndoti, 5);
            for (int k = 0; k < sphere_count; k++) {    // for each sphere check if theres an intersection between the light and original sphere
                if (is_triangle == 0) {
                  if (k == closest_object) continue;  // dont check this sphere
                }
                struct SphereType *shadow_sphere = &spheres[k];
                double a = pow(Rdx, 2) + pow(Rdy, 2) + pow(Rdz, 2);
                double b = 2 * (Rdx * (Rx - shadow_sphere->x) + 
                              Rdy * (Ry - shadow_sphere->y) + // quadratic formula :)
                              Rdz * (Rz - shadow_sphere->z));
                double c = pow(Rx - shadow_sphere->x, 2) + pow(Ry - shadow_sphere->y, 2) +
                          pow(Rz - shadow_sphere->z, 2) - pow(shadow_sphere->radius, 2);
                double deter = b * b - 4 * a * c;
                if (deter >= 0) {
                  double t = (-b - sqrt(deter)) / (2 * a);    // if intersection is found set and directional light is enabled check if the
                  if (t > 0.001) { // object is inbetween the distance, if it is its a shadow.
                    Rb = shadow_sphere->b;
                    Rg = shadow_sphere->g;
                    Rr = shadow_sphere->r;
                    intersection_flag = 1;
                    break;
                  }
                }
              }
              for (int k = 0; k < triangle_count; k++) {
                if (is_triangle == 1) {
                  if (k == closest_object) continue;
                }
                struct TriangleType *triangle = &triangles[k];
                struct Point *p0 = &vertices[triangle->v1];
                struct Point *p1 = &vertices[triangle->v2];
                struct Point *p2 = &vertices[triangle->v3];
                double e1x = p1->x - p0->x;
                double e1y = p1->y - p0->y;
                double e1z = p1->z - p0->z;
                double e2x = p2->x - p0->x;
                double e2y = p2->y - p0->y;
                double e2z = p2->z - p0->z;
                double nx = e1y * e2z - e1z * e2y;
                double ny = e1z * e2x - e1x * e2z;
                double nz = e1x * e2y - e1y * e2x;
                double D = -(nx * p0->x + ny * p0->y + nz * p0->z);
                double denom = nx * Rdx + ny * Rdy + nz * Rdz;
                if (fabs(denom) > 1e-6) {
                  double t = -(nx * Rx + ny * Ry + nz * Rz + D) / denom;
                  if (t > 0) {
                    double shadow_interx = Rx + Rdx * t;
                    double shadow_intery = Ry + Rdy * t;
                    double shadow_interz = Rz + Rdz * t;
                    double epx = shadow_interx - p0->x;
                    double epy = shadow_intery - p0->y;
                    double epz = shadow_interz - p0->z;
                    double d11 = e1x * e1x + e1y * e1y + e1z * e1z;
                    double d22 = e2x * e2x + e2y * e2y + e2z * e2z;
                    double d12 = e1x * e2x + e1y * e2y + e1z * e2z;
                    double d1p = e1x * epx + e1y * epy + e1z * epz;
                    double d2p = e2x * epx + e2y * epy + e2z * epz;
                    double determinant = (d11 * d22 - d12 * d12);
                    if (fabs(determinant) > 1e-6) { 
                        double shadow_beta = (d22 * d1p - d12 * d2p) / determinant;
                        double shadow_gamma = (d11 * d2p - d12 * d1p) / determinant;
                        double shadow_alpha = 1.0 - shadow_beta - shadow_gamma;
                        if (shadow_alpha >= 0 && shadow_beta >= 0 && shadow_gamma >= 0 &&
                            shadow_alpha <= 1 && shadow_beta <= 1 && shadow_gamma <= 1) {
                            if (t > 0.001) { // object is inbetween the distance, if it is its a shadow.
                              Rb = triangle->b;
                              Rg = triangle->g;
                              Rr = triangle->r;
                              intersection_flag = 1;
                              break;
                            }
                        }
                    }
                  }
                }
            }
            if (intersection_flag) {
              Ir = fmax(0, fmin(1, Ir + Rr * fr / 3.5));
              Ib = fmax(0, fmin(1, Ib + Rb * fr / 3.5));
              Ig = fmax(0, fmin(1, Ig + Rg * fr / 3.5));
            } 
            double Tx, Ty, Tz;
            double eta = bkgRef / currRef;
            double k = 1 - eta * eta * (1 - ndoti * ndoti);
            intersection_flag = 0;
            if (k < 0) {
              Tx = 0;
              Ty = 0;       // internal refraction check
              Tz = 0;
            } else {
              Tx = (-nx) * sqrt(1 - pow(bkgRef / currRef, 10) * (1 - pow(ndoti, 2))) + (bkgRef / currRef) * (ndoti * nx - Ix) * .8;
              Ty = (-ny) * sqrt(1 - pow(bkgRef / currRef, 10) * (1 - pow(ndoti, 2))) + (bkgRef / currRef) * (ndoti * ny - Iy) * .8;
              Tz = (-nz) * sqrt(1 - pow(bkgRef / currRef, 10) * (1 - pow(ndoti, 2))) + (bkgRef / currRef) * (ndoti * nz - Iz) * .8;
            }
            double Tr, Tb, Tg;
            for (int k = 0; k < sphere_count; k++) {    // for each sphere check if theres an intersection
                if (is_triangle == 0) {
                  if (k == closest_object) continue;  // dont check this sphere
                }
                struct SphereType *shadow_sphere = &spheres[k];
                double a = pow(Tx, 2) + pow(Ty, 2) + pow(Tz, 2);
                double b = 2 * (Tx * (Rx - shadow_sphere->x) + 
                              Ty * (Ry - shadow_sphere->y) + // quadratic formula :)
                              Tz * (Rz - shadow_sphere->z));
                double c = pow(Rx - shadow_sphere->x, 2) + pow(Ry - shadow_sphere->y, 2) +
                          pow(Rz - shadow_sphere->z, 2) - pow(shadow_sphere->radius, 2);
                double deter = b * b - 4 * a * c;
                if (deter >= 0) {
                  double t = (-b - sqrt(deter)) / (2 * a);    // if intersection is found set and directional light is enabled check if the
                  if (t > 0.001) { // object is inbetween the distance, if it is its a shadow.
                    Tb = shadow_sphere->b;
                    Tg = shadow_sphere->g;
                    Tr = shadow_sphere->r;
                    intersection_flag = 1;
                    break;
                  }
                }
              }
              for (int k = 0; k < triangle_count; k++) {
                if (is_triangle == 1) {
                  if (k == closest_object) continue;
                }
                struct TriangleType *triangle = &triangles[k];
                struct Point *p0 = &vertices[triangle->v1];
                struct Point *p1 = &vertices[triangle->v2];
                struct Point *p2 = &vertices[triangle->v3];
                double e1x = p1->x - p0->x;
                double e1y = p1->y - p0->y;
                double e1z = p1->z - p0->z;
                double e2x = p2->x - p0->x;
                double e2y = p2->y - p0->y;
                double e2z = p2->z - p0->z;
                double nx = e1y * e2z - e1z * e2y;
                double ny = e1z * e2x - e1x * e2z;
                double nz = e1x * e2y - e1y * e2x;
                double D = -(nx * p0->x + ny * p0->y + nz * p0->z);
                double denom = nx * Tx + ny * Ty + nz * Tz;
                if (fabs(denom) > 1e-6) {
                  double t = -(nx * Rx + ny * Ry + nz * Rz + D) / denom;
                  if (t > 0) {
                    double shadow_interx = Rx + Tx * t;
                    double shadow_intery = Ry + Ty * t;
                    double shadow_interz = Rz + Tz * t;
                    double epx = shadow_interx - p0->x;
                    double epy = shadow_intery - p0->y;
                    double epz = shadow_interz - p0->z;
                    double d11 = e1x * e1x + e1y * e1y + e1z * e1z;
                    double d22 = e2x * e2x + e2y * e2y + e2z * e2z;
                    double d12 = e1x * e2x + e1y * e2y + e1z * e2z;
                    double d1p = e1x * epx + e1y * epy + e1z * epz;
                    double d2p = e2x * epx + e2y * epy + e2z * epz;
                    double determinant = (d11 * d22 - d12 * d12);
                    if (fabs(determinant) > 1e-6) { 
                        double shadow_beta = (d22 * d1p - d12 * d2p) / determinant;
                        double shadow_gamma = (d11 * d2p - d12 * d1p) / determinant;
                        double shadow_alpha = 1.0 - shadow_beta - shadow_gamma;
                        if (shadow_alpha >= 0 && shadow_beta >= 0 && shadow_gamma >= 0 &&
                            shadow_alpha <= 1 && shadow_beta <= 1 && shadow_gamma <= 1) {
                            if (t > 0.001) { // object is inbetween the distance, if it is its a shadow.
                              Tb = triangle->b;
                              Tg = triangle->g;
                              Tr = triangle->r;
                              intersection_flag = 1;
                              break;
                            }
                        }
                    }
                  }
                }
            }
            if (intersection_flag == 1) {
              Ir = fmax(0, fmin(1, Ir + (1 - fr / 5) * (1 - currOpa) * Tr));
              Ib = fmax(0, fmin(1, Ib + (1 - fr / 5) * (1 - currOpa) * Tb));
              Ig = fmax(0, fmin(1, Ig + (1 - fr / 5) * (1 - currOpa) * Tg));
            } else {
              Ir = fmax(0, fmin(1, Ir + (1 - fr / 5) * (1 - currOpa) * bkgr));
              Ib = fmax(0, fmin(1, Ib + (1 - fr / 5) * (1 - currOpa) * bkgb));
              Ig = fmax(0, fmin(1, Ig + (1 - fr / 5) * (1 - currOpa) * bkgg));
            }
          }
          Ir = fmax(0, fmin(1, Ir));
          Ig = fmax(0, fmin(1, Ig));    // cap I between 0 and 1;
          Ib = fmax(0, fmin(1, Ib)); 
          fprintf(ppm, "%lf %lf %lf\n", Ir * 255, Ig * 255, Ib * 255);    // print to ppm file.
      }
  }
  fclose(ppm);    // close, success
  return 0;
}
