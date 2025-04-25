import os

from conan import ConanFile
from conan.tools.files import copy
from conan.tools.build import check_min_cppstd
from conan.tools.layout import basic_layout


class HPool(ConanFile):
    name = 'hpool'
    version = '2.1.1'
    license = 'GPL-3.0'
    description = 'Simple pool allocator'
    vendor = 'HyperCPU-Project'
    topics = ('header-only', )
    settings = ['os', 'arch', 'compiler', 'build_type']
    no_copy_source = True
    package_type = 'header-library'

    # for internal usage
    hpool_minimal_cpp_version = 17

    def layout(self):
        basic_layout(self)

    def package(self):
        copy(self, pattern='LICENSE', src=self.source_folder, dst=os.path.join(self.package_folder, 'licenses'))
        copy(
            self,
            pattern='*.hpp',
            src=self.source_folder,
            dst=os.path.join(self.package_folder, 'include')
        )

    def validate(self):
        check_min_cppstd(self, self.hpool_minimal_cpp_version)

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []

        for prop, value in (
            ('cmake_file_name', 'hpool'),
            ('cmake_target_name', 'hpool::hpool'),
            ('pkg_config_name', 'hpool')
        ):
            self.cpp_info.set_property(prop, value)
