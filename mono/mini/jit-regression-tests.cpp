#include "jit-regression-tests.h"

#ifndef ASSEMBLY_DIR
#error ASSEMBLY_DIR not defined
#endif
#ifndef CONFIG_DIR
#error CONFIG_DIR not defined
#endif
#ifndef COMPILE_OPTS
#error COMPILE_OPTS not defined
#endif

typedef int(*TestMethod) (void);

static char *baseDir;
static char separator[2] = { G_DIR_SEPARATOR, '\0' };
static MonoDomain *domain;

std::map<std::string, TestLoader *> TestLoader::loaders;

TestLoader::TestLoader(const char *assemblyName, const char *className)
{
	this->assemblyName = assemblyName;
	this->className = className;
	this->clazz = NULL;
}

void TestLoader::init()
{
	if (clazz) return;
	char *assemblyPath = g_build_path(separator, baseDir, assemblyName.c_str(), NULL);
	ASSERT_TRUE(assemblyPath != NULL);
	MonoAssembly *assembly = mono_domain_assembly_open(domain, assemblyPath);
	ASSERT_TRUE(assembly != NULL);
	MonoImage *image = mono_assembly_get_image(assembly);
	ASSERT_TRUE(image != NULL);
	this->clazz = mono_class_from_name(image, "", className.c_str());
	ASSERT_TRUE(this->clazz != NULL);
	g_free(assemblyPath);
}
TestLoader *TestLoader::GetLoader(const char *assemblyName, const char *className)
{
	std::string key = std::string(assemblyName) + " " + className;
	TestLoader *loader = loaders[key];
	if (!loader)
	{
		loader = new TestLoader(assemblyName, className);
		loaders[key] = loader;
	}
	return loader;
}

MonoClass *TestLoader::GetMonoClass()
{
	init();
	return clazz;
}

BaseTest::BaseTest(const char *assemblyName, const char *className)
{
	this->loader = TestLoader::GetLoader(assemblyName, className);
}
void BaseTest::RunTest(const char *methodName)
{
	ASSERT_TRUE(strncmp(methodName, "test_", 5) == 0);
	int expected = atoi(&methodName[5]);
	MonoMethod *method = mono_class_get_method_from_name(this->loader->GetMonoClass(), methodName, 0);
	ASSERT_TRUE(method != NULL);
	MonoCompile *compile = mini_method_compile(method, COMPILE_OPTS, domain, JIT_FLAG_RUN_CCTORS, 0, -1);
	ASSERT_TRUE(compile != NULL && compile->exception_type == MONO_EXCEPTION_NONE);
	TestMethod func = (TestMethod) mono_create_ftnptr(domain, (gpointer) compile->native_code);
	ASSERT_TRUE(func != NULL);
	int result = func();
	EXPECT_EQ(expected, result);
	mono_destroy_compile(compile);
}

int main(int argc, char **argv)
{
	baseDir = g_path_get_dirname(argv[0]);

	mono_set_assemblies_path(ASSEMBLY_DIR);
	mono_set_config_dir(CONFIG_DIR);
	mono_config_parse(NULL);
	domain = mono_jit_init("test");
	::testing::InitGoogleTest(&argc, argv);
	int rc = RUN_ALL_TESTS();
	mono_jit_cleanup(domain);
	return rc;
}
