#include "generic_factory.hpp"
#include "test_sub_derived_1.h"

TestSubDerived1::TestSubDerived1() : _name("SubDer1") {

}

std::string TestSubDerived1::name() const {
	return "A SubTestDerived1 named " + _name;
}

void TestSubDerived1::setName(const std::string& name) {
	_name = name;
}

REGISTER_CHILD_INTO_FACTORY(TestSubBase, TestSubDerived1, "TestSubDerived1");
