from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class MyProjectConan(ConanFile):
    name = "lama_launcher"
    version = "0.1"
    license = "MIT"
    exports_sources = "src/*", "CMakeLists.txt"

    # Settings
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    # Dependencies

    def requirements(self):
        self.requires("nlohmann_json/3.12.0")

    def layout(self):
        self.folders.build = "build"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
