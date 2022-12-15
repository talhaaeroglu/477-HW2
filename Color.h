#ifndef __COLOR_H__
#define __COLOR_H__

#include <iostream>

class Color
{
public:
    double r, g, b;

    Color();
    Color(double r, double g, double b);
    Color(const Color &other);
    friend std::ostream& operator<<(std::ostream& os, const Color& c);
    Color operator+(Color c);
    Color operator-(Color c);
    Color operator*(double num);
    Color operator/(double num);
};

#endif