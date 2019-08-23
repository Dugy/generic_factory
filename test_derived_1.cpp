#include "test_base.hpp"
#include "generic_factory.hpp"
#include "test_sub_derived_1.h"

class TestDerived1 : public TestBase {
	float _value = 0;
	std::shared_ptr<TestSubDerived1> _sub;
public:
	TestDerived1(std::shared_ptr<TestSubDerived1> underlying, float val) : _value(val), _sub(underlying) {}
	std::string type() const override {
		return _sub->name();
	}
	float value() override {
		return _value;
	}
	void correctValue(float correction) override {
		_value += correction;
	}
	void use() override {
		_value++;
	}

	~TestDerived1() override = default;
};

REGISTER_SECONDARY_CHILD_INTO_FACTORY(TestBase, TestSubBase, TestDerived1, TestSubDerived1, float);
