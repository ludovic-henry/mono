#ifndef JIT_REGRESSION_TESTS_H
#define JIT_REGRESSION_TESTS_H

#include "gtest/gtest.h"

#include <mono/metadata/mono-config.h>
#include <mono/metadata/assembly.h>
#include <mono/mini/jit.h>
extern "C" {
#include <mono/mini/mini.h>
}
#include <glib.h>

class TestLoader
{
private:
	static std::map<std::string, TestLoader *> loaders;

	std::string assemblyName;
	std::string className;
	MonoClass *clazz;

	TestLoader(const char *assemblyName, const char *className);
	void init();
public:
	static TestLoader *GetLoader(const char *assemblyName, const char *className);
	MonoClass *GetMonoClass();
};

class BaseTest : public ::testing::Test {
private:
	TestLoader *loader;
	int optimizations;
public:
	BaseTest(const char *assemblyName, const char *className, int optimizations);
protected:
	void RunTest(const char *methodName);
};

#define CONCAT0(a, b) a ## b
#define CONCAT(a, b) CONCAT0(a, b)
#define DEFINE_TEST_CLASS(test_class_name, assembly_name, class_name, optimizations) \
class test_class_name : public BaseTest { \
public: \
	test_class_name() : BaseTest(assembly_name, class_name, optimizations) {} \
};

#define DEFINE_TEST(test_class_name, test_name, method_name) \
TEST_F(test_class_name, test_name) { \
	RunTest(method_name); \
}

#endif
