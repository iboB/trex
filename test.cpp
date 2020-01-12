// T-Rex
// Copyright (c) 2020 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include "include/trex/hierarchy.hpp"

#include <iostream>
#include <string>

const double PI = 3.141592653589793238463;

struct Shape
{
    Shape(std::string name) : name(std::move(name)) {}
    virtual double area() const = 0;
    virtual void log(std::ostream& out) const = 0;
    std::string name;
};

struct Square final : Shape
{
    using Shape::Shape;
    virtual double area() const override { return side * side; }
    virtual void log(std::ostream& out) const override
    {
        out << "Square " << name << ": " << area() << '\n';
    }

    double side;
};

struct Circle final : Shape
{
    using Shape::Shape;
    virtual double area() const override { return PI * radius * radius; }
    virtual void log(std::ostream& out) const override
    {
        out << "Circle " << name << ": " << area() << '\n';
    }

    double radius;
};

int main()
{
    trex::hierarchy<Shape, std::string> h;
    h.register_type(trex::make_type_info<Square>("sq"));
    h.register_type(trex::make_type_info<Circle>("ci"));

    auto c = h.alloc_and_construct("sq", "asd");
    c->log(std::cout);

    c = h.alloc_and_construct("ci", "cici");
    c->log(std::cout);

    return 0;
}
