BuildType := Debug
HOME := $(subst \,/,$(HOME))
HOME := $(subst C:,,$(HOME))
PROJECTS := $(subst \,/,$(PROJECTS))
PROJECTS := $(subst C:,,$(PROJECTS))
BuildDir := build/$(BuildType)

ifeq ($(findstring Win,$(OS)),Win)
  # uname from gnuwin32 is a 32bit binary so it returns that the system is, too
  BuildOS := Windows
  BuildArch := x86_64
else
  BuildOS := $(shell uname -s)
  BuildArch := $(shell uname -m)
  ifeq ($(BuildArch),amd64)
    BuildArch := x86_64
  endif
endif
Build := $(BuildOS)-$(BuildArch)
Host := $(Build)
HostOS := $(firstword $(subst -, ,$(Host)))
HostArch := $(lastword $(subst -, ,$(Host)))


$(BuildDir)/bin/cmdb $(BuildDir)/bin/cmake: $(BuildDir)/build.ninja
	ninja -C $(BuildDir) cmdb cmake

$(BuildDir)/build.ninja:
	cmake -G Ninja \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DCMAKE_TOOLCHAIN_FILE=$(HOME)/.dotfiles/cmake/toolchains/Toolchain-$(Host).cmake \
		-S /Users/lanza/Projects/cmake \
		-B/Users/lanza/Projects/cmake/$(BuildDir)

$(BuildDir):
	cmake -E make_directory $(BuildDir)
