from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps


class MyProjectConan(ConanFile):
    name = "lama_launcher"
    version = "0.1"

    # Settings
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    # Dependencies
    requires = ["nlohmann_json/3.12.0"]

    def layout(self):
        self.folders.build = "build"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
