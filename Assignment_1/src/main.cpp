////////////////////////////////////////////////////////////////////////////////
#include <algorithm>
#include <complex>
#include <fstream>
#include <iostream>
#include <numeric>
#include <vector>

#include <Eigen/Dense>
// Shortcut to avoid  everywhere, DO NOT USE IN .h
using namespace Eigen;
////////////////////////////////////////////////////////////////////////////////

const std::string root_path = DATA_DIR;

// Computes the determinant of the matrix whose columns are the vector u and v
double inline det(const Vector2d &u, const Vector2d &v) // substract tells us that which side of u the vector v points to (positive means left, negative means right, zero means they’re parallel).
{
    // TODO(done)
    return u .x() * v.y() - v.x() * u.y(); //u * x() is not u.x(). The dot connects the variable to its function. u.x() means “ask u for its x.” Writing u * x() means “multiply u by something called x”, which doesn’t exist.
} // use * insted of , 

// Return true iff [a,b] intersects [c,d]
bool intersect_segment(const Vector2d &a, const Vector2d &b, const Vector2d &c, const Vector2d &d)
{
    // TODO(done)
    double side_c = det(b - a, c - a); //The line a→b is the vector b - a. The vector from a to c is c - a. Then det of those two tells us the side
    double side_d = det( b - a, d - a);

    double side_a = det(d- c,a - c);
    double side_b = det(d - c,b - c );

    return((side_c * side_d < 0) && (side_a * side_b <0)); //"opposite sides" simply means the product is negative * A and B are on opposite sides of line CD AND C and D are on opposite sides of line AB

}

////////////////////////////////////////////////////////////////////////////////

bool is_inside(const std::vector<Vector2d> &poly, const Vector2d &query)
{
    // 1. Compute bounding box and set coordinate of a point outside the polygon
    // TODO
    // Find the maximum x and y coordinates of the polygon.
    // Adding 1 to both coordinates gives us a point outside the polygon.
    double max_x = poly[0].x();
    double  max_y = poly[0].y();

    for(size_t i = 1; i < poly.size_t; i++){
        if(max_x < poly[i].x()){
            max_x = poly[i].(x);
        if(max_y < poly[i].x()){
            max_y = poly[i].y()
        }
    }
    Vector2d outside (max_x + 1 , max_y + 1); //outside is the name of the new variable. We chose it because this point is outside the polygon.
    // 2. Cast a ray from the query point to the 'outside' point, count number of intersections
    // TODO
    return true;
}
}

////////////////////////////////////////////////////////////////////////////////

std::vector<Vector2d> load_xyz(const std::string &filename)
{
    std::vector<Vector2d> points;
    std::ifstream in(filename);
    // TODO(done)
    int n;
    in >> n;
    for(int i = 0;i < n; i++){
        double x,y,z;
        in >> x >> y >> z ;
        points.push_back(Vector2d(x,y));


    }
    return points;
}

void save_xyz(const std::string &filename, const std::vector<Vector2d> &points)
{
    // TODO(done)
    std :: ofstream out (filename);
    out << points.size() << "\n";

    for (size_t i= 0;i< points.size();i++){
        out <<points[i].x() << " " << points[i].y() << " "<< 0 << "\n"; 
    }
}

std::vector<Vector2d> load_obj(const std::string &filename)
{
    std::ifstream in(filename);
    std::vector<Vector2d> points;
    std::vector<Vector2d> poly;
    char key;
    while (in >> key)
    {
        if (key == 'v')
        {
            double x, y, z;
            in >> x >> y >> z;
            points.push_back(Vector2d(x, y));
        }
        else if (key == 'f')
        {
            std::string line;
            std::getline(in, line);
            std::istringstream ss(line);
            int id;
            while (ss >> id)
            {
                poly.push_back(points[id - 1]);
            }
        }
    }
    return poly;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    const std::string points_path = root_path + "/points.xyz";
    const std::string poly_path = root_path + "/polygon.obj";

    std::vector<Vector2d> points = load_xyz(points_path);
    std:: cout << "Loaded" << points.size() << "points\n";

    ////////////////////////////////////////////////////////////////////////////////
    //Point in polygon
    std::vector<Vector2d> poly = load_obj(poly_path);
    std::vector<Vector2d> result;
    for (size_t i = 0; i < points.size(); ++i)
    {
        if (is_inside(poly, points[i]))
        {
            result.push_back(points[i]);
        }
    }
    save_xyz("output.xyz", result);

    return 0;
}
