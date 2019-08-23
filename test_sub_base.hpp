#ifndef TEST_SUB_BASE_HPP
#define TEST_SUB_BASE_HPP

#include <string>

class TestSubBase {
public:
	virtual std::string name() const = 0;
	virtual void setName(const std::string&) = 0;
	virtual ~TestSubBase() = default;
};

#endif // TEST_SUB_BASE_HPP
