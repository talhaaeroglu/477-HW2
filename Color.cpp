#include "Color.h"
#include <iostream>
#include <iomanip>

using namespace std;

Color::Color() {}

Color::Color(double r, double g, double b)
{
    this->r = r;
    this->g = g;
    this->b = b;
}

Color::Color(const Color &other)
{
    this->r = other.r;
    this->g = other.g;
    this->b = other.b;
}

ostream& operator<<(ostream& os, const Color& c)
{
    os << fixed << setprecision(0) << "rgb(" << c.r << ", " << c.g << ", " << c.b << ")";
    return os;
}

Color Color::operator/(double num)
{
  return Color(r/num, g/num, b/num);
}

Color Color::operator*(double num)
{
  return Color(r*num, g*num, b*num);
}

Color Color::operator+(Color c){
  return Color (r+c.r, g+c.g, b+c.b);
}

Color Color::operator-(Color c){
  return Color(r-c.r, g-c.g, b-c.b);
}
