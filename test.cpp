#include <iostream>
#include <vector>
#include "generic_factory.hpp"
#include "test_base.hpp"
#include "test_sub_base.hpp"

int main()
{
	std::vector<std::string> names = { "TestSubDerived1", "TestSubDerived2", "TestSubDerived2", "TestSubDerived1" };
	std::vector<std::shared_ptr<TestSubBase>> made;
	for (const auto& it : names) {
		made.emplace_back(GenericFactory<TestSubBase>::createChild(it).release());
	}
	std::vector<std::shared_ptr<TestBase>> made2;
	for (const auto& it : made) {
		made2.emplace_back(GenericSecondaryFactory<TestBase, std::shared_ptr<TestSubBase>, float>::createChild(it, 3).release());
	}
	for (const auto& it : made) {
		std::cout << it->name() << std::endl;
	}
	return 0;
}
