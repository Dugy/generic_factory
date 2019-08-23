TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        test.cpp \
        test_derived_1.cpp \
        test_derived_2.cpp \
        test_sub_derived_1.cpp \
        test_sub_derived_2.cpp

HEADERS += \
	generic_factory.hpp \
	test_base.hpp \
	test_sub_base.hpp \
	test_sub_derived_1.h \
	test_sub_derived_2.h
