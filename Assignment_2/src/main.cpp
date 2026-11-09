// C++ include
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

// Utilities for the Assignment
#include "utils.h"

// Image writing library
#define STB_IMAGE_WRITE_IMPLEMENTATION // Do not include this line twice in your project!
#include "stb_image_write.h"

// Shortcut to avoid Eigen:: everywhere, DO NOT USE IN .h
using namespace Eigen;

// ---------------------------------------------------------------------------
// Helper functions (used by more than one exercise so I made them separate)
// ---------------------------------------------------------------------------

// Generic ray-sphere intersection (the version from class, not the special case).
// A point on the ray is p(t) = origin + t * direction.
// A point is on the sphere if |p - center|^2 = radius^2.
// If I plug the ray into the sphere equation I get a quadratic a*t^2 + b*t + c = 0,
// so I just solve it with the quadratic formula.
// Returns true if the ray hits the sphere in front of the origin (t >= 0)
// and puts the closest t into t_out.
bool ray_sphere_intersection(
    const Vector3d &ray_origin, const Vector3d &ray_direction,
    const Vector3d &sphere_center, double sphere_radius,
    double &t_out)
{
    // vector from the sphere center to the ray origin
    const Vector3d oc = ray_origin - sphere_center;

    // coefficients of the quadratic
    const double a = ray_direction.dot(ray_direction);
    const double b = 2.0 * ray_direction.dot(oc);
    const double c = oc.dot(oc) - sphere_radius * sphere_radius;

    // discriminant: negative means the ray completely misses the sphere
    const double disc = b * b - 4.0 * a * c;
    if (disc < 0)
        return false;

    // two solutions = entry point and exit point
    const double sq = std::sqrt(disc);
    const double t0 = (-b - sq) / (2.0 * a);
    const double t1 = (-b + sq) / (2.0 * a);

    // we want the closest hit that is in front of the ray.
    // t0 is always the smaller one, so try it first
    if (t0 >= 0)
    {
        t_out = t0;
        return true;
    }
    // if t0 is behind us but t1 is not, the origin is inside the sphere
    if (t1 >= 0)
    {
        t_out = t1;
        return true;
    }
    // both are behind the ray, so no hit
    return false;
}

// Ray-parallelogram intersection.
// The parallelogram is: origin + u * pu + v * pv, where u and v are between 0 and 1.
// The ray is: ray_origin + t * ray_direction.
// Setting them equal gives:  u*pu + v*pv - t*d = ray_origin - pgram_origin
// which is a 3x3 linear system with unknowns (u, v, t), so I solve it with a matrix.
// The ray hits the parallelogram only if 0 <= u <= 1, 0 <= v <= 1 and t >= 0.
bool ray_parallelogram_intersection(
    const Vector3d &ray_origin, const Vector3d &ray_direction,
    const Vector3d &pgram_origin, const Vector3d &pgram_u, const Vector3d &pgram_v,
    double &t_out)
{
    // columns are pu, pv and -d so that the unknown vector is (u, v, t)
    Matrix3d M;
    M.col(0) = pgram_u;
    M.col(1) = pgram_v;
    M.col(2) = -ray_direction;

    // if the determinant is ~0 the ray is parallel to the plane, so the matrix
    // can't be inverted and there is no single intersection point
    if (std::abs(M.determinant()) < 1e-12)
        return false;

    // right hand side of the system
    const Vector3d rhs = ray_origin - pgram_origin;

    // solve for (u, v, t)
    const Vector3d x = M.inverse() * rhs;
    const double u = x(0), v = x(1), t = x(2);

    // the hit point must be inside the parallelogram (u, v in [0,1])
    // and in front of the ray (t >= 0)
    if (u < 0 || u > 1 || v < 0 || v > 1 || t < 0)
        return false;

    t_out = t;
    return true;
}

// ---------------------------------------------------------------------------
// Ex.1 Part 1: sphere, orthographic camera
// ---------------------------------------------------------------------------
void raytrace_sphere()
{
    std::cout << "Simple ray tracer, one sphere with orthographic projection" << std::endl;

    const std::string filename("sphere_orthographic.png");
    MatrixXd C = MatrixXd::Zero(800, 800); // Store the color
    MatrixXd A = MatrixXd::Zero(800, 800); // Store the alpha mask

    const Vector3d camera_origin(0, 0, 3);
    const Vector3d camera_view_direction(0, 0, -1);

    // The camera is orthographic, pointing in the direction -z and covering the
    // unit square (-1,1) in x and y
    const Vector3d image_origin(-1, 1, 1);
    const Vector3d x_displacement(2.0 / C.cols(), 0, 0);
    const Vector3d y_displacement(0, -2.0 / C.rows(), 0);

    const double sphere_radius = 0.9;
    const Vector3d sphere_center(0, 0, 0);

    // Single light source
    const Vector3d light_position(-1, 1, 1);

    for (unsigned i = 0; i < C.cols(); ++i)
    {
        for (unsigned j = 0; j < C.rows(); ++j)
        {
            // center of the current pixel on the image plane
            const Vector3d pixel_center = image_origin + double(i) * x_displacement + double(j) * y_displacement;

            // Prepare the ray
            // orthographic: every ray goes in the same direction (-z),
            // only the starting point changes from pixel to pixel
            const Vector3d ray_origin = pixel_center;
            const Vector3d ray_direction = camera_view_direction;

            // Intersect with the sphere using the generic function
            double t;
            if (ray_sphere_intersection(ray_origin, ray_direction, sphere_center, sphere_radius, t))
            {
                // The ray hit the sphere, compute the exact intersection point
                // using the ray equation with the t we found
                Vector3d ray_intersection = ray_origin + t * ray_direction;

                // Compute normal at the intersection point
                // for a sphere the normal points from the center to the hit point
                Vector3d ray_normal = (ray_intersection - sphere_center).normalized();

                // Simple diffuse model
                C(i, j) = (light_position - ray_intersection).normalized().transpose() * ray_normal;

                // Clamp to zero
                C(i, j) = std::max(C(i, j), 0.);

                // Disable the alpha mask for this pixel
                A(i, j) = 1;
            }
        }
    }

    // Save to png
    write_matrix_to_png(C, C, C, A, filename);
}

// ---------------------------------------------------------------------------
// Ex.1 Part 2: parallelogram, orthographic camera
// ---------------------------------------------------------------------------
void raytrace_parallelogram()
{
    std::cout << "Simple ray tracer, one parallelogram with orthographic projection" << std::endl;

    const std::string filename("plane_orthographic.png");
    MatrixXd C = MatrixXd::Zero(800, 800); // Store the color
    MatrixXd A = MatrixXd::Zero(800, 800); // Store the alpha mask

    const Vector3d camera_origin(0, 0, 3);
    const Vector3d camera_view_direction(0, 0, -1);

    // The camera is orthographic, pointing in the direction -z and covering the unit square (-1,1) in x and y
    const Vector3d image_origin(-1, 1, 1);
    const Vector3d x_displacement(2.0 / C.cols(), 0, 0);
    const Vector3d y_displacement(0, -2.0 / C.rows(), 0);

    // Parameters of the parallelogram (position of the lower-left corner + two sides)
    const Vector3d pgram_origin(-0.5, -0.5, 0);
    const Vector3d pgram_u(1, 0.4, 0);
    const Vector3d pgram_v(0, 0.7, -10);

    // Single light source
    const Vector3d light_position(-1, 1, 1);

    for (unsigned i = 0; i < C.cols(); ++i)
    {
        for (unsigned j = 0; j < C.rows(); ++j)
        {
            const Vector3d pixel_center = image_origin + double(i) * x_displacement + double(j) * y_displacement;

            // Prepare the ray (same as the sphere, parallel rays along -z)
            const Vector3d ray_origin = pixel_center;
            const Vector3d ray_direction = camera_view_direction;

            // Check if the ray intersects with the parallelogram
            double t;
            if (ray_parallelogram_intersection(ray_origin, ray_direction, pgram_origin, pgram_u, pgram_v, t))
            {
                // The ray hit the parallelogram, compute the exact intersection point
                Vector3d ray_intersection = ray_origin + t * ray_direction;

                // Compute normal at the intersection point
                // the normal of a flat surface is the cross product of its two sides.
                // The sign depends on the order of the cross product, so if the normal
                // points the same way as the ray we flip it so it faces the camera.
                // (otherwise the lighting would come out wrong)
                Vector3d ray_normal = pgram_u.cross(pgram_v).normalized();
                if (ray_normal.dot(ray_direction) > 0)
                    ray_normal = -ray_normal;

                // Simple diffuse model
                C(i, j) = (light_position - ray_intersection).normalized().transpose() * ray_normal;

                // Clamp to zero
                C(i, j) = std::max(C(i, j), 0.);

                // Disable the alpha mask for this pixel
                A(i, j) = 1;
            }
        }
    }

    // Save to png
    write_matrix_to_png(C, C, C, A, filename);
}

// ---------------------------------------------------------------------------
// Ex.1 Part 3: parallelogram, perspective camera
// ---------------------------------------------------------------------------
void raytrace_perspective()
{
    std::cout << "Simple ray tracer, one parallelogram with perspective projection" << std::endl;

    const std::string filename("plane_perspective.png");
    MatrixXd C = MatrixXd::Zero(800, 800); // Store the color
    MatrixXd A = MatrixXd::Zero(800, 800); // Store the alpha mask

    const Vector3d camera_origin(0, 0, 3);

    // The camera is perspective, pointing in the direction -z and covering the unit square (-1,1) in x and y
    const Vector3d image_origin(-1, 1, 1);
    const Vector3d x_displacement(2.0 / C.cols(), 0, 0);
    const Vector3d y_displacement(0, -2.0 / C.rows(), 0);

    // Parameters of the parallelogram (position of the lower-left corner + two sides)
    const Vector3d pgram_origin(-0.5, -0.5, 0);
    const Vector3d pgram_u(1, 0.4, 0);
    const Vector3d pgram_v(0, 0.7, -10);

    // Single light source
    const Vector3d light_position(-1, 1, 1);

    for (unsigned i = 0; i < C.cols(); ++i)
    {
        for (unsigned j = 0; j < C.rows(); ++j)
        {
            const Vector3d pixel_center = image_origin + double(i) * x_displacement + double(j) * y_displacement;

            // Prepare the ray (origin point and direction)
            // perspective: all rays start at the camera and go through the pixel,
            // so now the origin is fixed and the direction changes per pixel
            const Vector3d ray_origin = camera_origin;
            const Vector3d ray_direction = (pixel_center - camera_origin).normalized();

            // Check if the ray intersects with the parallelogram
            double t;
            if (ray_parallelogram_intersection(ray_origin, ray_direction, pgram_origin, pgram_u, pgram_v, t))
            {
                // The ray hit the parallelogram, compute the exact intersection point
                Vector3d ray_intersection = ray_origin + t * ray_direction;

                // Compute normal at the intersection point (flipped to face the ray, same as before)
                Vector3d ray_normal = pgram_u.cross(pgram_v).normalized();
                if (ray_normal.dot(ray_direction) > 0)
                    ray_normal = -ray_normal;

                // Simple diffuse model
                C(i, j) = (light_position - ray_intersection).normalized().transpose() * ray_normal;

                // Clamp to zero
                C(i, j) = std::max(C(i, j), 0.);

                // Disable the alpha mask for this pixel
                A(i, j) = 1;
            }
        }
    }

    // Save to png
    write_matrix_to_png(C, C, C, A, filename);
}

// ---------------------------------------------------------------------------
// Ex.2: shading (ambient + diffuse + specular, in RGB)
// ---------------------------------------------------------------------------
void raytrace_shading()
{
    std::cout << "Simple ray tracer, one sphere with different shading" << std::endl;

    const std::string filename("shading.png");

    // one matrix per color channel now instead of a single grey one
    MatrixXd R = MatrixXd::Zero(800, 800);
    MatrixXd G = MatrixXd::Zero(800, 800);
    MatrixXd B = MatrixXd::Zero(800, 800);
    MatrixXd A = MatrixXd::Zero(800, 800); // Store the alpha mask

    const Vector3d camera_origin(0, 0, 3);

    // The camera is perspective, pointing in the direction -z and covering the unit square (-1,1) in x and y
    const Vector3d image_origin(-1, 1, 1);
    const Vector3d x_displacement(2.0 / A.cols(), 0, 0);
    const Vector3d y_displacement(0, -2.0 / A.rows(), 0);

    // Sphere setup
    const Vector3d sphere_center(0, 0, 0);
    const double sphere_radius = 0.9;

    // Material params
    const Vector3d diffuse_color(1, 0, 1);
    const double specular_exponent = 100;
    const Vector3d specular_color(0., 0, 1);

    // Single light source
    const Vector3d light_position(-1, 1, 1);
    const Vector3d light_intensity(1, 1, 1);
    const double ambient = 0.1;

    for (unsigned i = 0; i < A.cols(); ++i)
    {
        for (unsigned j = 0; j < A.rows(); ++j)
        {
            const Vector3d pixel_center = image_origin + double(i) * x_displacement + double(j) * y_displacement;

            // Perspective ray (same setup as the perspective exercise)
            const Vector3d ray_origin = camera_origin;
            const Vector3d ray_direction = (pixel_center - camera_origin).normalized();

            // Intersect with the sphere using the generic function
            double t;
            if (ray_sphere_intersection(ray_origin, ray_direction, sphere_center, sphere_radius, t))
            {
                // The ray hit the sphere, compute the exact intersection point
                Vector3d ray_intersection = ray_origin + t * ray_direction;

                // Compute normal at the intersection point
                Vector3d ray_normal = (ray_intersection - sphere_center).normalized();

                // Vectors needed for the shading equations (all unit length):
                // l = direction from the point to the light
                // v = direction from the point to the camera (viewer)
                // h = half vector between l and v (Blinn-Phong)
                const Vector3d l = (light_position - ray_intersection).normalized();
                const Vector3d v = (camera_origin - ray_intersection).normalized();
                const Vector3d h = (l + v).normalized();

                // dot products, clamped at 0 so surfaces facing away from the light get no light
                const double n_dot_l = std::max(ray_normal.dot(l), 0.);
                const double n_dot_h = std::max(ray_normal.dot(h), 0.);

                // Diffuse: how much the surface faces the light, times the material color.
                // .array() lets me multiply the vectors component by component (per channel)
                const Vector3d diffuse = (diffuse_color.array() * light_intensity.array()).matrix() * n_dot_l;

                // Specular: shiny highlight. The exponent controls how small/sharp it is
                // (bigger exponent = smaller, sharper highlight)
                const Vector3d specular = (specular_color.array() * light_intensity.array()).matrix() * std::pow(n_dot_h, specular_exponent);

                // Ambient: a small constant so the parts in shadow are not completely black.
                // It is the same value for R, G and B so it shows up as dark grey
                const Vector3d ambient_term = Vector3d::Constant(ambient);

                // Final color = ambient + diffuse + specular
                Vector3d color = ambient_term + diffuse + specular;

                // Clamp each channel to [0,1] and store it
                R(i, j) = std::min(std::max(color(0), 0.), 1.);
                G(i, j) = std::min(std::max(color(1), 0.), 1.);
                B(i, j) = std::min(std::max(color(2), 0.), 1.);

                // Disable the alpha mask for this pixel
                A(i, j) = 1;
            }
        }
    }

    // Save to png
    write_matrix_to_png(R, G, B, A, filename);
}

int main()
{
    raytrace_sphere();
    raytrace_parallelogram();
    raytrace_perspective();
    raytrace_shading();

    return 0;
}
