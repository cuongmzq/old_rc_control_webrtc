# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/pi/Documents/cpp/robak

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/pi/Documents/cpp/robak/src

# Include any dependencies generated for this target.
include src/CMakeFiles/video.dir/depend.make

# Include the progress variables for this target.
include src/CMakeFiles/video.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/video.dir/flags.make

src/CMakeFiles/video.dir/video.c.o: src/CMakeFiles/video.dir/flags.make
src/CMakeFiles/video.dir/video.c.o: video.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/Documents/cpp/robak/src/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/CMakeFiles/video.dir/video.c.o"
	cd /home/pi/Documents/cpp/robak/src/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/video.dir/video.c.o   -c /home/pi/Documents/cpp/robak/src/video.c

src/CMakeFiles/video.dir/video.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/video.dir/video.c.i"
	cd /home/pi/Documents/cpp/robak/src/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/Documents/cpp/robak/src/video.c > CMakeFiles/video.dir/video.c.i

src/CMakeFiles/video.dir/video.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/video.dir/video.c.s"
	cd /home/pi/Documents/cpp/robak/src/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/Documents/cpp/robak/src/video.c -o CMakeFiles/video.dir/video.c.s

# Object files for target video
video_OBJECTS = \
"CMakeFiles/video.dir/video.c.o"

# External object files for target video
video_EXTERNAL_OBJECTS =

src/video: src/CMakeFiles/video.dir/video.c.o
src/video: src/CMakeFiles/video.dir/build.make
src/video: src/CMakeFiles/video.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pi/Documents/cpp/robak/src/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable video"
	cd /home/pi/Documents/cpp/robak/src/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/video.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/video.dir/build: src/video

.PHONY : src/CMakeFiles/video.dir/build

src/CMakeFiles/video.dir/clean:
	cd /home/pi/Documents/cpp/robak/src/src && $(CMAKE_COMMAND) -P CMakeFiles/video.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/video.dir/clean

src/CMakeFiles/video.dir/depend:
	cd /home/pi/Documents/cpp/robak/src && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pi/Documents/cpp/robak /home/pi/Documents/cpp/robak/src /home/pi/Documents/cpp/robak/src /home/pi/Documents/cpp/robak/src/src /home/pi/Documents/cpp/robak/src/src/CMakeFiles/video.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/video.dir/depend
