#include "include/trex/hierarchy.hpp"

#include <iostream>
#include <string>

const double PI = 3.141592653589793238463;

struct Shape
{
    virtual double area() const = 0;
    virtual void log(std::ostream& out) const = 0;
    std::string name;
};

struct Square final : Shape
{
    virtual double area() const override { return side * side; }
    virtual void log(std::ostream& out) const override
    {
        out << "Square " << name << ": " << area() << '\n';
    }

    double side;
};

struct Circle final : Shape
{
    virtual double area() const override { return PI * radius * radius; }
    virtual void log(std::ostream& out) const override
    {
        out << "Circle " << name << ": " << area() << '\n';
    }

    double radius;
};

int main()
{
    trex::hierarchy<Shape> h;
    h.register_type<Square>(trex::make_type_info<Square>("sq"));
    h.register_type<Circle>(trex::make_type_info<Circle>("ci"));

    auto c = h.alloc_and_construct("sq");
    c->name = "asd";
    c->log(std::cout);

    c = h.alloc_and_construct("ci");
    c->log(std::cout);

    return 0;
}
