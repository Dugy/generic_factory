#ifndef TEST_BASE_HPP
#define TEST_BASE_HPP

#include <string>

class TestBase {
public:
	virtual std::string type() const = 0;
	virtual float value() = 0;
	virtual void correctValue(float) = 0;
	virtual void use() = 0;
	virtual ~TestBase() = default;
};

#endif // TEST_BASE_HPP
