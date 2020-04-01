"""This script is the entry point for building, distributing and installing
this module using distutils/setuptools."""
import distutils.command.build
import pathlib
import platform
import subprocess
import os
import sys
import sysconfig
import setuptools
import setuptools.command.build_ext
import setuptools.command.install
import setuptools.command.test

# Check Python requirement
MAJOR = sys.version_info[0]
MINOR = sys.version_info[1]
if not (MAJOR >= 3 and MINOR >= 6):
    raise RuntimeError("Python %d.%d is not supported, "
                       "you need at least Python 3.6." % (MAJOR, MINOR))

# Working directory
WORKING_DIRECTORY = pathlib.Path(__file__).parent.absolute()


def patch_leveldb():
    """Patch leveldb CMAKE to find Snappy libraries"""
    target = WORKING_DIRECTORY.joinpath("third_party", "leveldb",
                                        "CMakeLists.txt")
    code = f"""
set(
  CMAKE_MODULE_PATH
  "{WORKING_DIRECTORY!s}/cmake"
)

find_package(Snappy)
if(SNAPPY_FOUND)
  set(HAVE_SNAPPY 1)
endif(SNAPPY_FOUND)
"""
    patch = False
    with open(target) as stream:
        lines = stream.readlines()

    for ix, line in enumerate(lines):
        if 'snappy_compress' in line:
            lines[ix] = code
            patch = True

    if not patch:
        return

    with open(target, "w") as stream:
        stream.writelines(lines)


def build_dirname(extname=None):
    """Returns the name of the build directory"""
    extname = '' if extname is None else os.sep.join(extname.split(".")[:-1])
    return str(
        pathlib.Path(WORKING_DIRECTORY, "build",
                     "lib.%s-%d.%d" % (sysconfig.get_platform(), MAJOR, MINOR),
                     extname))


def execute(cmd):
    """Executes a command and returns the lines displayed on the standard
    output"""
    process = subprocess.Popen(cmd,
                               shell=True,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
    return process.stdout.read().decode()


# pylint: disable=too-few-public-methods
class CMakeExtension(setuptools.Extension):
    """Python extension to build"""
    def __init__(self, name):
        super(CMakeExtension, self).__init__(name, sources=[])

    # pylint: enable=too-few-public-methods


class BuildExt(setuptools.command.build_ext.build_ext):
    """Build the Python extension using cmake"""

    #: Preferred BOOST root
    BOOST_ROOT = None

    #: Preferred C++ compiler
    CXX_COMPILER = None

    #: Preferred Eigen root
    EIGEN3_INCLUDE_DIR = None

    #: Run CMake to configure this project
    RECONFIGURE = None

    def run(self):
        """A command's raison d'etre: carry out the action"""
        for ext in self.extensions:
            self.build_cmake(ext)
        super().run()

    @staticmethod
    def boost():
        """Get the default boost path in Anaconda's environnement."""
        # Do not search system for Boost & disable the search for boost-cmake
        boost_option = "-DBoost_NO_SYSTEM_PATHS=TRUE " \
            "-DBoost_NO_BOOST_CMAKE=TRUE"
        boost_root = sys.prefix
        if pathlib.Path(boost_root, "include", "boost").exists():
            return "{boost_option} -DBOOST_ROOT={boost_root}".format(
                boost_root=boost_root, boost_option=boost_option).split()
        boost_root = pathlib.Path(sys.prefix, "Library", "include")
        if not boost_root.exists():
            raise RuntimeError(
                "Unable to find the Boost library in the conda distribution "
                "used.")
        return "{boost_option} -DBoost_INCLUDE_DIR={boost_root}".format(
            boost_root=boost_root, boost_option=boost_option).split()

    @staticmethod
    def eigen():
        """Get the default Eigen3 path in Anaconda's environnement."""
        eigen_include_dir = pathlib.Path(sys.prefix, "include", "eigen3")
        if eigen_include_dir.exists():
            return "-DEIGEN3_INCLUDE_DIR=" + str(eigen_include_dir)
        eigen_include_dir = pathlib.Path(sys.prefix, "Library", "include",
                                         "eigen3")
        if not eigen_include_dir.exists():
            eigen_include_dir = eigen_include_dir.parent
        if not eigen_include_dir.exists():
            raise RuntimeError(
                "Unable to find the Eigen3 library in the conda distribution "
                "used.")
        return "-DEIGEN3_INCLUDE_DIR=" + str(eigen_include_dir)

    @staticmethod
    def is_conda():
        """Detect if the Python interpreter is part of a conda distribution."""
        result = pathlib.Path(sys.prefix, 'conda-meta').exists()
        if not result:
            try:
                # pylint: disable=unused-import
                import conda
                # pylint: enable=unused-import
            except ImportError:
                result = False
            else:
                result = True
        return result

    def set_cmake_user_options(self):
        """Sets the options defined by the user."""
        is_conda = self.is_conda()
        result = []

        if self.CXX_COMPILER is not None:
            result.append("-DCMAKE_CXX_COMPILER=" + self.CXX_COMPILER)

        if self.BOOST_ROOT is not None:
            result.append("-DBOOSTROOT=" + self.BOOST_ROOT)
        elif is_conda:
            result += self.boost()

        if self.EIGEN3_INCLUDE_DIR is not None:
            result.append("-DEIGEN3_INCLUDE_DIR=" + self.EIGEN3_INCLUDE_DIR)
        elif is_conda:
            result.append(self.eigen())

        return result

    def build_cmake(self, ext):
        """Execute cmake to build the Python extension"""
        # These dirs will be created in build_py, so if you don't have
        # any python sources to bundle, the dirs will be missing
        build_temp = pathlib.Path(WORKING_DIRECTORY, self.build_temp)
        build_temp.mkdir(parents=True, exist_ok=True)
        extdir = build_dirname(ext.name)

        # patch leveldb
        patch_leveldb()

        cfg = 'Debug' if self.debug else 'Release'

        cmake_args = [
            "-DCMAKE_BUILD_TYPE=" + cfg, "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=" +
            str(extdir), "-DPYTHON_EXECUTABLE=" + sys.executable
        ] + self.set_cmake_user_options()

        build_args = ['--config', cfg]

        if platform.system() != 'Windows':
            build_args += ['--', '-j%d' % os.cpu_count()]
            if platform.system() == 'Darwin':
                cmake_args += ['-DCMAKE_OSX_DEPLOYMENT_TARGET=10.14']
        else:
            cmake_args += [
                '-G', 'Visual Studio 15 2017',
                '-DCMAKE_GENERATOR_PLATFORM=x64',
                '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(
                    cfg.upper(), extdir)
            ]
            build_args += ['--', '/m']
            if self.verbose:
                build_args += ['/verbosity:n']

        if self.verbose:
            build_args.insert(0, "--verbose")

        os.chdir(str(build_temp))

        # Has CMake ever been executed?
        if pathlib.Path(build_temp, "CMakeFiles",
                        "TargetDirectories.txt").exists():
            # The user must force the reconfiguration
            configure = self.RECONFIGURE is not None
        else:
            configure = True

        if configure:
            self.spawn(['cmake', str(WORKING_DIRECTORY)] + cmake_args)
        if not self.dry_run:
            cmake_cmd = ['cmake', '--build', '.', '--target', 'core']
            self.spawn(cmake_cmd + build_args)
        os.chdir(str(WORKING_DIRECTORY))


class Build(distutils.command.build.build):
    """Build everything needed to install"""
    user_options = distutils.command.build.build.user_options
    user_options += [
        ('boost-root=', None, 'Preferred Boost installation prefix'),
        ('reconfigure', None, 'Forces CMake to reconfigure this project'),
        ('cxx-compiler=', None, 'Preferred C++ compiler'),
        ('eigen-root=', None, 'Preferred Eigen3 include directory'),
    ]

    def initialize_options(self):
        """Set default values for all the options that this command supports"""
        super().initialize_options()
        self.boost_root = None
        self.cxx_compiler = None
        self.eigen_root = None
        self.reconfigure = None

    def run(self):
        """A command's raison d'etre: carry out the action"""
        if self.boost_root is not None:
            BuildExt.BOOST_ROOT = self.boost_root
        if self.cxx_compiler is not None:
            BuildExt.CXX_COMPILER = self.cxx_compiler
        if self.eigen_root is not None:
            BuildExt.EIGEN3_INCLUDE_DIR = self.eigen_root
        if self.reconfigure is not None:
            BuildExt.RECONFIGURE = True
        super().run()


def long_description():
    """Reads the README file"""
    with open(pathlib.Path(WORKING_DIRECTORY, "README.md")) as stream:
        return stream.read()


def typehints():
    """Get the list of type information files"""
    pyi = []
    for root, _, files in os.walk(WORKING_DIRECTORY):
        pyi += [
            str(pathlib.Path(root, item).relative_to(WORKING_DIRECTORY))
            for item in files if item.endswith('.pyi')
        ]
    return [(str(pathlib.Path('pyinterp', 'core')), pyi)]


def main():
    """Main function"""
    setuptools.setup(
        author='CNES/CLS',
        author_email='fbriol@gmail.com',
        classifiers=[
            "Development Status :: 3 - Alpha",
            "Topic :: Scientific/Engineering :: Physics",
            "License :: OSI Approved :: BSD License",
            "Natural Language :: English", "Operating System :: POSIX",
            "Operating System :: MacOS",
            "Operating System :: Microsoft :: Windows",
            "Programming Language :: Python :: 3.6",
            "Programming Language :: Python :: 3.7",
            "Programming Language :: Python :: 3.8"
        ],
        cmdclass={
            'build': Build,
            'build_ext': BuildExt,
        },
        data_files=typehints(),
        description='Interpolation of geo-referenced data for Python.',
        ext_modules=[CMakeExtension(name="geohash.core")],
        # install_requires=["numpy", "xarray"],
        license="BSD License",
        long_description=long_description(),
        long_description_content_type='text/markdown',
        name='geohash',
        package_data={
            'geohash': ['py.typed'],
        },
        package_dir={'': 'src'},
        packages=setuptools.find_namespace_packages(where='src',
                                                    exclude=['*core*']),
        platforms=['POSIX', 'MacOS', 'Windows'],
        python_requires='>=3.6',
        # tests_require=["netCDF4", "numpy", "pytest", "xarray>=0.13"],
        url='https://github.com/fbriol/pangeo-geohash',
        # version=revision(),
        zip_safe=False,
    )


if __name__ == "__main__":
    main()
