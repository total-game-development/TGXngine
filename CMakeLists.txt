cmake_minimum_required(VERSION 3.16)
project(TGXngine VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------------------------------
# MARK: Optimisation
#
# Both of these have to be settled before the first add_subdirectory below.
# add_compile_options fills in a directory property that a target reads when it
# is created, and the flag variables are read the same way, so anything set after
# a target exists does not reach it. OPTIMISE used to sit at the bottom of this
# file, after every target in it had already been created, and reached none of
# them.
# ------------------------------------------------------------------------------
option(OPTIMISE "Enable compiler optimizations" OFF)

if(OPTIMISE)
	message(STATUS "Compiler optimizations enabled (OPTIMISE=ON)")
	if(MSVC)
		add_compile_options(/O2)
	else()
		add_compile_options(-O3)
	endif()
endif()

# A Debug build runs an order of magnitude under a Release one, which is a poor
# way to judge whether a change to the simulation is fast enough, and painful in
# a match that is paced by a clock. This builds the Debug configuration at
# production speed while keeping its symbols, its assertions and its debug heap.
#
# The cost is that stepping becomes jumpy and some locals are optimised out,
# because the code being stepped through is the optimised code. Plain Debug is
# still there for when a breakpoint has to land exactly where it was put.
option(FAST_DEBUG "Optimise the Debug configuration, keeping its symbols" OFF)

if(FAST_DEBUG)
	message(STATUS "Debug configuration will be optimised (FAST_DEBUG=ON)")

	foreach(flags IN ITEMS CMAKE_CXX_FLAGS_DEBUG CMAKE_C_FLAGS_DEBUG)
		if(MSVC)
			# Rewritten rather than appended to: /Od beats an /O2 that follows
			# it, and the compiler refuses /RTC1 and /O2 together outright.
			string(REGEX REPLACE "/RTC[1csu]+" "" ${flags} "${${flags}}")
			string(REGEX REPLACE "/Ob[0-3]" "" ${flags} "${${flags}}")
			string(REGEX REPLACE "/Od" "" ${flags} "${${flags}}")
			set(${flags} "${${flags}} /O2")
		else()
			string(REGEX REPLACE "-O[0-3sgz]" "" ${flags} "${${flags}}")
			set(${flags} "${${flags}} -O2")
		endif()
	endforeach()

	if(MSVC)
		# The checked iterators are the other half of the cost, and they change
		# the layout of every standard container. Everything here -- the engine,
		# the modules, SFML, fmt, IXWebSocket, gtest -- is built from this one
		# tree and so is told the same thing, which is why this has to be above
		# the vendor subdirectories. A binary that missed it would disagree about
		# the shape of the containers WorldState carries across the module
		# boundary; MSVC records the level in each object file and refuses to
		# link a mismatch, so that reads as a link error rather than as
		# corruption at run time.
		add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)
	endif()
endif()

# ------------------------------------------------------------------------------
# MARK: Vendors
# ------------------------------------------------------------------------------

option(BUILD_TESTING "Build tests for vendor projects" OFF)
option(Random_BuildTests "Build tests for random" OFF)
set(IXWEBSOCKET_ENABLE_PROJECT ON) # TODO: might be unnecessary now

# Off by default: TLS needs OpenSSL or mbedTLS present, and a client that only
# ever speaks ws:// should not fail to configure over a library it never calls.
# With it on, a wss:// address connects; without it, one is refused with a
# message rather than failing silently.
option(TGX_ENABLE_TLS "Build IXWebSocket with TLS, for wss:// transport" OFF)

if(TGX_ENABLE_TLS)
	set(USE_TLS ON CACHE BOOL "" FORCE)
endif()

add_subdirectory(Vendor/SFML) # sfml-graphics sfml-window sfml-system sfml-audio
add_subdirectory(Vendor/IXWebSocket)
add_subdirectory(Vendor/json) # header only
add_subdirectory(Vendor/random) # header only
add_subdirectory(Vendor/unordered_dense) # header only
add_subdirectory(Vendor/fmt)

set(
	GLOBAL_INCLUDES
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/include>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/include/common>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/include/library>
	${SFML_SOURCE_DIR}/include
	${nlohmann_json_SOURCE_DIR}/include
	${ixwebsocket_SOURCE_DIR}/include
	${effolkronium_random_SOURCE_DIR}/include
	${unordered_dense_SOURCE_DIR}/include
)

set(
	VENDOR_LIBRARIES
	sfml-graphics
	sfml-window
	sfml-system
	sfml-audio
	nlohmann_json
	ixwebsocket
	effolkronium_random
	unordered_dense
	fmt
)

# ------------------------------------------------------------------------------
# MARK: Disable warnings
# only for vendor libs that compile sources
# ------------------------------------------------------------------------------

if(MSVC)
	set(DISABLE_WARNINGS $<$<COMPILE_LANGUAGE:CXX>:/w>)
else()
	set(DISABLE_WARNINGS -w)
endif()

foreach(
	tgt
	IN
	ITEMS sfml-graphics sfml-window sfml-system sfml-audio ixwebsocket fmt
)
	if(TARGET ${tgt})
		get_target_property(_ttype ${tgt} TYPE)
		# NOTE: NEVER do this for INTERFACE targets, it propegates to consumers
		if(NOT _ttype STREQUAL "INTERFACE_LIBRARY")
			target_compile_options(${tgt} PRIVATE ${DISABLE_WARNINGS})
		endif()
	endif()
endforeach()

# ------------------------------------------------------------------------------
# MARK: Toolchain specifics
# ------------------------------------------------------------------------------

# nice to have
if(WIN32)
	add_definitions(-DPLATFORM_WINDOWS)
elseif(APPLE)
	add_definitions(-DPLATFORM_APPLE)
elseif(UNIX)
	add_definitions(-DPLATFORM_LINUX)
endif()

add_compile_definitions("BUILD_DEBUG=$<CONFIG:Debug>")

if(MINGW)
	add_compile_options(-fdeclspec)
endif()

if(MSVC)
	add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
	add_compile_options(/permissive- /Zc:__cplusplus)
endif()

if(MSVC)
	add_compile_options(
		/W4
		/WX
		/wd4100 # unused parameter
		/wd4101 # unused local variable
		/wd4505 # unreferenced inline function
	)
else()
	add_compile_options(
		-Wall
		-Wextra
		-Wpedantic
		-Werror
		# FIXME: ideally temporary
		-Wno-unused-variable
		-Wno-unused-parameter
		-Wno-unused-function
		-Wno-unused-private-field
		-Wno-unused-but-set-variable
	)
endif()

# ------------------------------------------------------------------------------
# MARK: Output directories
# main & common: <build>/<config> TODO: get libs in <build>/<Config>
# modules: <build>/<config>/modules
# ------------------------------------------------------------------------------

set(OUT_BIN_ROOT "${CMAKE_BINARY_DIR}/$<IF:$<CONFIG:>,$<CONFIG>,.>")

function(set_outputs target outdir)
	set_target_properties(
		${target}
		PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${outdir}"
			LIBRARY_OUTPUT_DIRECTORY "${outdir}"
			ARCHIVE_OUTPUT_DIRECTORY "${outdir}"
			RUNTIME_OUTPUT_DIRECTORY_DEBUG "${outdir}"
			LIBRARY_OUTPUT_DIRECTORY_DEBUG "${outdir}"
			ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${outdir}"
			RUNTIME_OUTPUT_DIRECTORY_RELEASE "${outdir}"
			LIBRARY_OUTPUT_DIRECTORY_RELEASE "${outdir}"
			ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${outdir}"
			RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${outdir}"
			LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${outdir}"
			ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${outdir}"
			RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${outdir}"
			LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${outdir}"
			ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${outdir}"
	)
endfunction()

# NON-HEADER vendor libs should use the main bin dir
foreach(
	vendor_target
	IN
	ITEMS sfml-graphics sfml-window sfml-system sfml-audio ixwebsocket fmt
)
	if(TARGET ${vendor_target})
		set_outputs(${vendor_target} "${OUT_BIN_ROOT}")
	endif()
endforeach()

# ------------------------------------------------------------------------------
# MARK: Common & Library
# common is a shared lib. library is static. this means common exists once in a
# lib, and library is copied into each lib that uses it
# both should end up in the build dir
# ------------------------------------------------------------------------------

# common
add_library(Common SHARED)
target_compile_definitions(Common PRIVATE COMMON_BUILD)

file(GLOB_RECURSE Common_SOURCES "Source/include/common/*.cpp")
target_sources(Common PRIVATE ${Common_SOURCES})

target_include_directories(Common PUBLIC ${GLOBAL_INCLUDES})
target_link_libraries(Common PRIVATE ${VENDOR_LIBRARIES})

set_outputs(Common "${OUT_BIN_ROOT}")
set_target_properties(Common PROPERTIES OUTPUT_NAME "Common" PREFIX "")

# library
add_library(Library STATIC)
target_compile_definitions(Library PRIVATE COMMON_BUILD)

file(GLOB_RECURSE Library_SOURCES "Source/include/library/*.cpp")
target_sources(Library PRIVATE ${Library_SOURCES})

target_include_directories(Library PUBLIC ${GLOBAL_INCLUDES})
target_link_libraries(Library PRIVATE ${VENDOR_LIBRARIES})

set_outputs(Library "${OUT_BIN_ROOT}")
set_target_properties(Library PROPERTIES OUTPUT_NAME "Library" PREFIX "")

# ------------------------------------------------------------------------------
# MARK: Modules
# build modules into <build>/<config>/modules
# ------------------------------------------------------------------------------

function(add_wos_module name)
	add_library(${name} SHARED)

	file(GLOB_RECURSE ${name}_SOURCES "Source/modules/${name}/*.cpp")
	target_sources(${name} PRIVATE ${${name}_SOURCES})

	target_include_directories(
		${name}
		PUBLIC
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/modules/${name}/src>
			${GLOBAL_INCLUDES}
			# TODO: shouldn't be used by aircrafts.h
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/modules/Buildings>
	)

	set_outputs(${name} "${OUT_BIN_ROOT}/modules")
	set_target_properties(${name} PROPERTIES OUTPUT_NAME "${name}" PREFIX "")

	target_link_libraries(${name} PRIVATE ${VENDOR_LIBRARIES} Common Library)
endfunction()

file(GLOB MODULES "Source/modules/*")
foreach(module ${MODULES})
	if(IS_DIRECTORY ${module})
		get_filename_component(module_name ${module} NAME)
		add_wos_module(${module_name})
	endif()
endforeach()

# ------------------------------------------------------------------------------
# MARK: Main executable
# ------------------------------------------------------------------------------

file(GLOB_RECURSE MAIN_SOURCES "Source/src/*.cpp")

add_executable(TGXngine Source/src/Main.cpp ${MAIN_SOURCES})

# default Visual Studio startup project
set_property(
	DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	PROPERTY VS_STARTUP_PROJECT TGXngine
)

target_include_directories(
	TGXngine
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/src ${GLOBAL_INCLUDES}
)

set_outputs(TGXngine "${OUT_BIN_ROOT}")

set(TGX_LIBS Common Library ${VENDOR_LIBRARIES})
if(UNIX)
	list(APPEND TGX_LIBS dl)
endif()
target_link_libraries(TGXngine PRIVATE ${TGX_LIBS})

# ------------------------------------------------------------------------------
# MARK: Dependencies
# theres probably a propper way to get this working, but for now it just copies
# required libs to the build dir
# ------------------------------------------------------------------------------

if(WIN32)
	find_package(ZLIB)
	find_package(OpenAL)

	# ZLIB DLL Handling
	if(NOT ZLIB_FOUND)
		if(CMAKE_BUILD_TYPE STREQUAL "Debug")
			# If it built as zlibd.dll, use that name here, but we still output
			# it as z.dll if that's what TGXngine expects
			set(
				ZLIB_DLL_SRC
				"${CMAKE_CURRENT_SOURCE_DIR}/Vendor/zlib/build/Debug/zlibd.dll"
			)
		else()
			set(
				ZLIB_DLL_SRC
				"${CMAKE_CURRENT_SOURCE_DIR}/Vendor/zlib/build/Release/zlib.dll"
			)
		endif()

		add_custom_command(
			TARGET TGXngine
			POST_BUILD
			COMMAND
				${CMAKE_COMMAND} -E copy_if_different "${ZLIB_DLL_SRC}"
				"${OUT_BIN_ROOT}/z.dll"
			COMMENT "Copying z.dll to build directory..."
		)
	endif()
endif()

# ------------------------------------------------------------------------------
# MARK: Resources
# ------------------------------------------------------------------------------

add_custom_target(
	copy_resources
	ALL
	COMMAND
		${CMAKE_COMMAND} -E copy_directory_if_different
		"${CMAKE_CURRENT_SOURCE_DIR}/Resources" "${OUT_BIN_ROOT}/Resources"
	COMMENT "Copying assets to the build directory"
)

add_dependencies(TGXngine copy_resources)

# ------------------------------------------------------------------------------
# MARK: Docs
# ------------------------------------------------------------------------------

# TODO: allow generating docs without building project
if(GENERATE_DOCUMENTATION)
	find_package(Doxygen QUIET)
	if(DOXYGEN_FOUND)
		doxygen_add_docs(
			doc
			"Source/"
			ALL
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			COMMENT "Generating documentation with Doxygen"
		)
	else()
		message(STATUS "Doxygen not found, skipping documentation generation")
	endif()
endif()

# ------------------------------------------------------------------------------
# MARK: Google Test
# ------------------------------------------------------------------------------

option(BUILD_UNIT_TESTS "Build unit tests" OFF)

if(BUILD_UNIT_TESTS)
	include(FetchContent)

	FetchContent_Declare(
		googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG v1.17.0
	)
	FetchContent_MakeAvailable(googletest)

	enable_testing()

	file(GLOB_RECURSE TEST_SOURCES "Source/tests/*.cpp")

	# the shell module's language and filesystem are exercised headlessly, so they
	# are compiled into the test binary directly. Shell.cpp is the DLL entry point
	# and pulls in the window, so it stays out.
	file(GLOB SHELL_TESTABLE_SOURCES "Source/modules/Shell/*.cpp")
	list(FILTER SHELL_TESTABLE_SOURCES EXCLUDE REGEX "Shell\.cpp$")

	# The lockstep clock is the piece worth testing away from a socket: it decides
	# when a client may simulate and holds every command until its tick. Client.cpp
	# and Session.cpp stay out -- one owns a socket, the other drives the renderer.
	set(NET_TESTABLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Source/src/Net/Lockstep.cpp")

	add_executable(UnitTests ${TEST_SOURCES} ${SHELL_TESTABLE_SOURCES} ${NET_TESTABLE_SOURCES})

	# The test binary links Common as a shared library, so it has to sit beside
	# the Common.dll this build just produced. Left in the generator's own Debug/
	# directory it loads whatever stale copy is lying next to it, and an inline
	# accessor compiled here then reads a WorldState laid out by an older build.
	set_outputs(UnitTests "${OUT_BIN_ROOT}")

	foreach(gtest_target IN ITEMS gtest gtest_main gmock gmock_main)
		if(TARGET ${gtest_target})
			set_outputs(${gtest_target} "${OUT_BIN_ROOT}")
		endif()
	endforeach()

	target_include_directories(
		UnitTests
		PRIVATE
			${GLOBAL_INCLUDES}
			${CMAKE_CURRENT_SOURCE_DIR}/Source/modules/Shell
			${CMAKE_CURRENT_SOURCE_DIR}/Source/src
	)

	target_link_libraries(
		UnitTests
		PRIVATE gtest gtest_main ${VENDOR_LIBRARIES} Common Library
	)

	# suppress warnings for tests
	if(TARGET gtest)
		if(MSVC)
			target_compile_options(gtest PRIVATE /w)
		else()
			target_compile_options(gtest PRIVATE -w)
		endif()
	endif()

	add_test(NAME AllUnitTests COMMAND UnitTests)
endif()

# ------------------------------------------------------------------------------
# MARK: Debugging & Sanitizers
# ------------------------------------------------------------------------------
option(USE_SANITIZERS "Enable Address/UB Sanitizers" OFF)

if(USE_SANITIZERS)
	message(STATUS "Enabling Sanitizers")

	set(SAN_COMPILE_FLAGS)
	set(SAN_LINK_FLAGS)

	# --------------------------------------------------------------------------
	# GCC
	# --------------------------------------------------------------------------
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		list(
			APPEND
			SAN_COMPILE_FLAGS
			-g3
			-O1
			-fno-omit-frame-pointer
			-fno-optimize-sibling-calls
			# Sanitizers
			-fsanitize=address
			-fsanitize=undefined
			-fsanitize-address-use-after-scope
			# Better diagnostics
			-fstack-protector-strong
			-D_GLIBCXX_ASSERTIONS
			# GCC STL false positives
			-Wno-maybe-uninitialized
			-Wno-error=maybe-uninitialized
		)

		list(APPEND SAN_LINK_FLAGS -fsanitize=address -fsanitize=undefined)

		# --------------------------------------------------------------------------
		# Clang / AppleClang (macOS)
		# --------------------------------------------------------------------------
	elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		list(
			APPEND
			SAN_COMPILE_FLAGS
			-g3
			-O1
			-fno-omit-frame-pointer
			-fno-optimize-sibling-calls
			# Sanitizers
			-fsanitize=address
			-fsanitize=undefined
			-fsanitize-address-use-after-scope
			# Better stack traces
			-fno-inline
			# libc++ runtime debugging
			-D_LIBCPP_ENABLE_ASSERTIONS=1
		)

		list(APPEND SAN_LINK_FLAGS -fsanitize=address -fsanitize=undefined)

		# ----------------------------------------------------------------------
		# Apple Silicon specific improvements
		# ----------------------------------------------------------------------
		if(APPLE)
			message(STATUS "Configuring Apple/macOS sanitizer options")

			# Better symbolication in LLDB
			list(APPEND SAN_COMPILE_FLAGS -glldb)

			# Prevent aggressive dead stripping
			list(APPEND SAN_LINK_FLAGS -Wl,-no_deduplicate)

			# Optional:
			# Detect container overflows in libc++
			list(
				APPEND
				SAN_COMPILE_FLAGS
				-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG
			)
		endif()

		# --------------------------------------------------------------------------
		# MSVC
		# --------------------------------------------------------------------------
	elseif(MSVC)
		list(APPEND SAN_COMPILE_FLAGS /Zi /fsanitize=address)

		list(APPEND SAN_LINK_FLAGS /INCREMENTAL:NO)
	endif()

	# --------------------------------------------------------------------------
	# MAIN TARGET
	# --------------------------------------------------------------------------
	target_compile_options(${PROJECT_NAME} PRIVATE ${SAN_COMPILE_FLAGS})

	target_link_options(${PROJECT_NAME} PRIVATE ${SAN_LINK_FLAGS})

	# --------------------------------------------------------------------------
	# UNIT TESTS
	# --------------------------------------------------------------------------
	if(TARGET UnitTests)
		target_compile_options(UnitTests PRIVATE ${SAN_COMPILE_FLAGS})

		target_link_options(UnitTests PRIVATE ${SAN_LINK_FLAGS})
	endif()

	# --------------------------------------------------------------------------
	# Runtime sanitizer environment hints
	# --------------------------------------------------------------------------
	message(STATUS "")
	message(STATUS "Sanitizers enabled")
	message(STATUS "Recommended runtime:")
	message(STATUS "ASAN_OPTIONS=detect_leaks=1:check_initialization_order=1")
	message(STATUS "UBSAN_OPTIONS=print_stacktrace=1")
	message(STATUS "")
endif()

# ------------------------------------------------------------------------------
# MARK: Toolchain specifics
# OPTIMISE and FAST_DEBUG are declared at the top of this file, where they are
# early enough to reach the targets they are meant to.
# ------------------------------------------------------------------------------
