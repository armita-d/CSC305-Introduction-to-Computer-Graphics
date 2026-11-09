////////////////////////////////////////////////////////////////////////////////
// C++ include
#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <cmath>
#include <cassert>
#include <random>

// Utilities for the Assignment
#include "utils.h"

// Image writing library
#define STB_IMAGE_WRITE_IMPLEMENTATION // Do not include this line twice in your project!
#include "stb_image_write.h"

// Shortcut to avoid Eigen:: everywhere, DO NOT USE IN .h
using namespace Eigen;

////////////////////////////////////////////////////////////////////////////////
// Scene setup, global variables
////////////////////////////////////////////////////////////////////////////////

const std::string filename("raytrace.png");

//Camera settings
const double focal_length = 10;
const double field_of_view = 0.7854; //45 degrees
const double image_z = 5;
const bool is_perspective = true; // Ex 1: set to false to see the orthographic result
const Vector3d camera_position(0, 0, 5);
const double camera_aperture = 0.05;

// Bonus 1: depth of field
const bool enable_dof = false;
const int dof_samples = 64;

// Bonus 2: refraction
const bool enable_refraction = false;
const double refraction_index = 1.5;

//Maximum number of recursive calls
const int max_bounce = 5;

// Small offset to avoid self intersection (shadow acne)
const double EPS = 1e-4;

// Objects
std::vector<Vector3d> sphere_centers;
std::vector<double> sphere_radii;
std::vector<Matrix3d> parallelograms;

//Material for the object, same material for all objects
const Vector4d obj_ambient_color(0.5, 0.1, 0.1, 0);
const Vector4d obj_diffuse_color(0.5, 0.5, 0.5, 0);
const Vector4d obj_specular_color(0.2, 0.2, 0.2, 0);
const double obj_specular_exponent = 256.0;
const Vector4d obj_reflection_color(0.7, 0.7, 0.7, 0);
const Vector4d obj_refraction_color(0.7, 0.7, 0.7, 0);

// Precomputed (or otherwise) gradient vectors at each grid node
const int grid_size = 20;
std::vector<std::vector<Vector2d>> grid;

//Lights
std::vector<Vector3d> light_positions;
std::vector<Vector4d> light_colors;
//Ambient light
const Vector4d ambient_light(0.2, 0.2, 0.2, 0);

//Fills the different arrays
void setup_scene()
{
    grid.resize(grid_size + 1);
    for (int i = 0; i < grid_size + 1; ++i)
    {
        grid[i].resize(grid_size + 1);
        for (int j = 0; j < grid_size + 1; ++j)
            grid[i][j] = Vector2d::Random().normalized();
    }

    //Spheres
    sphere_centers.emplace_back(10, 0, 1);
    sphere_radii.emplace_back(1);

    sphere_centers.emplace_back(7, 0.05, -1);
    sphere_radii.emplace_back(1);

    sphere_centers.emplace_back(4, 0.1, 1);
    sphere_radii.emplace_back(1);

    sphere_centers.emplace_back(1, 0.2, -1);
    sphere_radii.emplace_back(1);

    sphere_centers.emplace_back(-2, 0.4, 1);
    sphere_radii.emplace_back(1);

    sphere_centers.emplace_back(-5, 0.8, -1);
    sphere_radii.emplace_back(1);

    sphere_centers.emplace_back(-8, 1.6, 1);
    sphere_radii.emplace_back(1);

    //parallelograms
    parallelograms.emplace_back();
    parallelograms.back() << -100, -100, 100,
        -1.25, -1.2, 0,
        -100, 100, -100;

    //Lights
    light_positions.emplace_back(8, 8, 0);
    light_colors.emplace_back(16, 16, 16, 0);

    light_positions.emplace_back(6, -8, 0);
    light_colors.emplace_back(16, 16, 16, 0);

    light_positions.emplace_back(4, 8, 0);
    light_colors.emplace_back(16, 16, 16, 0);

    light_positions.emplace_back(2, -8, 0);
    light_colors.emplace_back(16, 16, 16, 0);

    light_positions.emplace_back(0, 8, 0);
    light_colors.emplace_back(16, 16, 16, 0);

    light_positions.emplace_back(-2, -8, 0);
    light_colors.emplace_back(16, 16, 16, 0);

    light_positions.emplace_back(-4, 8, 0);
    light_colors.emplace_back(16, 16, 16, 0);
}

//We need to make this function visible
Vector4d shoot_ray(const Vector3d &ray_origin, const Vector3d &ray_direction, int max_bounce);

////////////////////////////////////////////////////////////////////////////////
// Perlin noise code
////////////////////////////////////////////////////////////////////////////////

// Function to interpolate between a0 and a1
// Weight w should be in the range [0.0, 1.0]
double lerp(double a0, double a1, double w)
{
    assert(w >= 0);
    assert(w <= 1);
    // Linear version (Ex 4, first output):
    // return (1.0 - w) * a0 + w * a1;

    // Cubic version (Ex 4, second output):
    return (a1 - a0) * (3.0 - w * 2.0) * w * w + a0;
}

// Computes the dot product of the distance and gradient vectors.
double dotGridGradient(int ix, int iy, double x, double y)
{
    // Distance vector from the grid node to the point
    const double dx = x - double(ix);
    const double dy = y - double(iy);

    // Dot product with the gradient stored at that node
    return dx * grid[ix][iy](0) + dy * grid[ix][iy](1);
}

// Compute Perlin noise at coordinates x, y
double perlin(double x, double y)
{
    // Determine grid cell coordinates (clamped so x1, y1 stay inside the grid
    // when x or y is exactly grid_size)
    int x0 = int(std::floor(x));
    int y0 = int(std::floor(y));
    x0 = std::min(std::max(x0, 0), grid_size - 1);
    y0 = std::min(std::max(y0, 0), grid_size - 1);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // Determine interpolation weights
    double sx = x - x0;
    double sy = y - y0;

    // Interpolate between grid point gradients
    double n0 = dotGridGradient(x0, y0, x, y);
    double n1 = dotGridGradient(x1, y0, x, y);

    double ix0 = lerp(n0, n1, sx);

    n0 = dotGridGradient(x0, y1, x, y);
    n1 = dotGridGradient(x1, y1, x, y);

    double ix1 = lerp(n0, n1, sx);
    double value = lerp(ix0, ix1, sy);

    return value;
}

Vector4d procedural_texture(const double tu, const double tv)
{
    assert(tu >= 0);
    assert(tv >= 0);

    assert(tu <= 1);
    assert(tv <= 1);

    const double color = (perlin(tu * grid_size, tv * grid_size) + 1) / 2;
    return Vector4d(0, color, 0, 0);

    // Example of checkerboard texture
    // const double color = (int(tu * grid_size) + int(tv * grid_size)) % 2 == 0 ? 0 : 1;
    // return Vector4d(0, color, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////
// Intersection code
////////////////////////////////////////////////////////////////////////////////

//Compute the intersection between a ray and a sphere, return -1 if no intersection
double ray_sphere_intersection(const Vector3d &ray_origin, const Vector3d &ray_direction, int index, Vector3d &p, Vector3d &N)
{
    const Vector3d sphere_center = sphere_centers[index];
    const double sphere_radius = sphere_radii[index];

    // Solve |o + t d - c|^2 = r^2  ->  a t^2 + b t + c = 0
    const Vector3d oc = ray_origin - sphere_center;
    const double a = ray_direction.dot(ray_direction);
    const double b = 2.0 * oc.dot(ray_direction);
    const double c = oc.dot(oc) - sphere_radius * sphere_radius;

    const double disc = b * b - 4.0 * a * c;
    if (disc < 0)
    {
        return -1;
    }

    const double sq = std::sqrt(disc);
    const double t0 = (-b - sq) / (2.0 * a);
    const double t1 = (-b + sq) / (2.0 * a);

    // Smallest root in front of the ray (the larger one is used when the ray starts inside)
    double t = -1;
    if (t0 > 1e-6)
        t = t0;
    else if (t1 > 1e-6)
        t = t1;
    else
        return -1;

    p = ray_origin + t * ray_direction;
    N = (p - sphere_center).normalized();

    return t;
}

//Compute the intersection between a ray and a paralleogram, return -1 if no intersection
double ray_parallelogram_intersection(const Vector3d &ray_origin, const Vector3d &ray_direction, int index, Vector3d &p, Vector3d &N)
{
    const Vector3d pgram_origin = parallelograms[index].col(0);
    const Vector3d A = parallelograms[index].col(1);
    const Vector3d B = parallelograms[index].col(2);
    const Vector3d pgram_u = A - pgram_origin;
    const Vector3d pgram_v = B - pgram_origin;

    // o + t d = origin + u * pgram_u + v * pgram_v
    // -> [pgram_u pgram_v -d] [u v t]^T = o - origin
    Matrix3d M;
    M.col(0) = pgram_u;
    M.col(1) = pgram_v;
    M.col(2) = -ray_direction;

    if (std::abs(M.determinant()) < 1e-12)
    {
        return -1; // ray parallel to the plane
    }

    const Vector3d sol = M.inverse() * (ray_origin - pgram_origin);
    const double u = sol(0);
    const double v = sol(1);
    const double t = sol(2);

    if (u < 0 || u > 1 || v < 0 || v > 1 || t < 1e-6)
    {
        return -1;
    }

    p = ray_origin + t * ray_direction;
    N = pgram_u.cross(pgram_v).normalized();
    // Make the normal face the incoming ray
    if (N.dot(ray_direction) > 0)
        N = -N;

    return t;
}

//Finds the closest intersecting object returns its index
//In case of intersection it writes into p and N (intersection point and normals)
int find_nearest_object(const Vector3d &ray_origin, const Vector3d &ray_direction, Vector3d &p, Vector3d &N)
{
    // Find the object in the scene that intersects the ray first
    // we store the index and the 'closest_t' to their expected values
    int closest_index = -1;
    double closest_t = std::numeric_limits<double>::max(); //closest t is "+ infinity"

    Vector3d tmp_p, tmp_N;
    for (int i = 0; i < sphere_centers.size(); ++i)
    {
        //returns t and writes on tmp_p and tmp_N
        const double t = ray_sphere_intersection(ray_origin, ray_direction, i, tmp_p, tmp_N);
        //We have intersection
        if (t >= 0)
        {
            //The point is before our current closest t
            if (t < closest_t)
            {
                closest_index = i;
                closest_t = t;
                p = tmp_p;
                N = tmp_N;
            }
        }
    }

    for (int i = 0; i < parallelograms.size(); ++i)
    {
        //returns t and writes on tmp_p and tmp_N
        const double t = ray_parallelogram_intersection(ray_origin, ray_direction, i, tmp_p, tmp_N);
        //We have intersection
        if (t >= 0)
        {
            //The point is before our current closest t
            if (t < closest_t)
            {
                closest_index = sphere_centers.size() + i;
                closest_t = t;
                p = tmp_p;
                N = tmp_N;
            }
        }
    }

    return closest_index;
}

////////////////////////////////////////////////////////////////////////////////
// Raytracer code
////////////////////////////////////////////////////////////////////////////////

//Checks if the light is visible
bool is_light_visible(const Vector3d &ray_origin, const Vector3d &ray_direction, const Vector3d &light_position)
{
    Vector3d p, N;
    const int obj = find_nearest_object(ray_origin, ray_direction, p, N);

    if (obj < 0)
    {
        return true; // nothing in the way
    }

    // An object only blocks the light if it is closer than the light itself
    const double dist_to_hit = (p - ray_origin).norm();
    const double dist_to_light = (light_position - ray_origin).norm();
    return dist_to_hit > dist_to_light;
}

Vector4d shoot_ray(const Vector3d &ray_origin, const Vector3d &ray_direction, int max_bounce)
{
    //Intersection point and normal, these are output of find_nearest_object
    Vector3d p, N;

    const int nearest_object = find_nearest_object(ray_origin, ray_direction, p, N);

    if (nearest_object < 0)
    {
        // Return a transparent color
        return Vector4d(0, 0, 0, 0);
    }

    // Normal that faces the incoming ray (used for secondary rays and offsets)
    const Vector3d Nf = (ray_direction.dot(N) < 0) ? N : Vector3d(-N);

    // Ambient light contribution
    const Vector4d ambient_color = obj_ambient_color.array() * ambient_light.array();

    // Punctual lights contribution (direct lighting)
    Vector4d lights_color(0, 0, 0, 0);
    for (int i = 0; i < light_positions.size(); ++i)
    {
        const Vector3d &light_position = light_positions[i];
        const Vector4d &light_color = light_colors[i];

        const Vector3d Li = (light_position - p).normalized();

        // Shadow ray
        if (!is_light_visible(p + EPS * Nf, Li, light_position))
        {
            continue;
        }

        Vector4d diff_color = obj_diffuse_color;

        if (nearest_object == 4)
        {
            //Compute UV coodinates for the point on the sphere
            const double x = p(0) - sphere_centers[nearest_object][0];
            const double y = p(1) - sphere_centers[nearest_object][1];
            const double z = p(2) - sphere_centers[nearest_object][2];
            double tu = acos(z / sphere_radii[nearest_object]) / 3.1415;
            double tv = (3.1415 + atan2(y, x)) / (2 * 3.1415);
            tu = std::min(tu, 1.0);
            tu = std::max(tu, 0.0);

            tv = std::min(tv, 1.0);
            tv = std::max(tv, 0.0);

            diff_color = procedural_texture(tu, tv);
        }

        // Diffuse contribution
        const Vector4d diffuse = diff_color * std::max(Li.dot(N), 0.0);

        // Specular contribution (Blinn-Phong with the half vector)
        const Vector3d V = (-ray_direction).normalized();
        const Vector3d H = (Li + V).normalized();
        const Vector4d specular = obj_specular_color * std::pow(std::max(N.dot(H), 0.0), obj_specular_exponent);

        // Attenuate lights according to the squared distance to the lights
        const Vector3d D = light_position - p;
        lights_color += (diffuse + specular).cwiseProduct(light_color) / D.squaredNorm();
    }

    Vector4d refl_color = obj_reflection_color;
    if (nearest_object == 4)
    {
        refl_color = Vector4d(0.5, 0.5, 0.5, 0);
    }

    // Reflection: r = 2n(n.v) - v, with v pointing back toward the viewer
    Vector4d reflection_color(0, 0, 0, 0);
    if (max_bounce > 0)
    {
        const Vector3d v = -ray_direction;
        const Vector3d r = (2.0 * Nf * Nf.dot(v) - v).normalized();
        const Vector4d bounced = shoot_ray(p + EPS * Nf, r, max_bounce - 1);
        reflection_color = refl_color.cwiseProduct(bounced);
    }

    // Refraction (Snell-Descartes), bonus
    Vector4d refraction_color(0, 0, 0, 0);
    if (enable_refraction && max_bounce > 0)
    {
        const Vector3d d = ray_direction.normalized();
        // If the ray hits the surface from outside, N faces it. From inside, flip.
        const bool entering = d.dot(N) < 0;
        const Vector3d n = entering ? N : Vector3d(-N);
        const double eta = entering ? (1.0 / refraction_index) : refraction_index;

        const double cos_i = -d.dot(n);
        const double k = 1.0 - eta * eta * (1.0 - cos_i * cos_i);

        // k < 0 means total internal reflection: no refracted ray
        if (k >= 0)
        {
            const Vector3d t = (eta * d + (eta * cos_i - std::sqrt(k)) * n).normalized();
            const Vector4d refracted = shoot_ray(p - EPS * n, t, max_bounce - 1);
            refraction_color = obj_refraction_color.cwiseProduct(refracted);
        }
    }

    // Rendering equation
    Vector4d C = ambient_color + lights_color + reflection_color + refraction_color;

    //Set alpha to 1
    C(3) = 1;

    return C;
}

////////////////////////////////////////////////////////////////////////////////

void raytrace_scene()
{
    std::cout << "Simple ray tracer." << std::endl;

    int w = 800;
    int h = 400;
    MatrixXd R = MatrixXd::Zero(w, h);
    MatrixXd G = MatrixXd::Zero(w, h);
    MatrixXd B = MatrixXd::Zero(w, h);
    MatrixXd A = MatrixXd::Zero(w, h); // Store the alpha mask

    // The camera always points in the direction -z
    // The sensor grid is at a distance 'focal_length' from the camera center,
    // and covers an viewing angle given by 'field_of_view'.
    double aspect_ratio = double(w) / double(h);
    double image_y = focal_length * std::tan(field_of_view / 2.0);
    double image_x = image_y * aspect_ratio;

    // The pixel grid through which we shoot rays is at a distance 'focal_length'
    const Vector3d image_origin(-image_x, image_y, -image_z);
    const Vector3d x_displacement(2.0 / w * image_x, 0, 0);
    const Vector3d y_displacement(0, -2.0 / h * image_y, 0);

    std::mt19937 rng(12345);
    std::uniform_real_distribution<double> uni(0.0, 1.0);

    for (unsigned i = 0; i < w; ++i)
    {
        for (unsigned j = 0; j < h; ++j)
        {
            const Vector3d pixel_center = image_origin + (i + 0.5) * x_displacement + (j + 0.5) * y_displacement;

            // Prepare the ray
            Vector3d ray_origin;
            Vector3d ray_direction;

            Vector4d C(0, 0, 0, 0);

            if (is_perspective)
            {
                const int samples = enable_dof ? dof_samples : 1;
                for (int s = 0; s < samples; ++s)
                {
                    ray_origin = camera_position;

                    if (enable_dof)
                    {
                        // Random point on the aperture (disk perpendicular to the view axis)
                        const double r = camera_aperture * std::sqrt(uni(rng));
                        const double theta = 2.0 * 3.14159265358979 * uni(rng);
                        ray_origin += Vector3d(r * std::cos(theta), r * std::sin(theta), 0);
                    }

                    // The pixel grid is the focal plane, so every sample of this pixel
                    // aims at the same point on it
                    ray_direction = (pixel_center - ray_origin).normalized();

                    C += shoot_ray(ray_origin, ray_direction, max_bounce);
                }
                C /= double(samples);
            }
            else
            {
                // Orthographic camera
                ray_origin = camera_position + Vector3d(pixel_center[0], pixel_center[1], 0);
                ray_direction = Vector3d(0, 0, -1);

                C = shoot_ray(ray_origin, ray_direction, max_bounce);
            }

            R(i, j) = C(0);
            G(i, j) = C(1);
            B(i, j) = C(2);
            A(i, j) = C(3);
        }
    }

    // Save to png
    write_matrix_to_png(R, G, B, A, filename);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    setup_scene();

    raytrace_scene();
    return 0;
}
