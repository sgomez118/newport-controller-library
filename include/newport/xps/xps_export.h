#ifndef NEWPORT_XPS_XPS_EXPORT_H_
#define NEWPORT_XPS_XPS_EXPORT_H_

// DLL visibility macro — applied to all public classes and free functions.
// When building the shared library, define NEWPORT_XPS_BUILDING_DLL (done
// automatically by CMake via target_compile_definitions).
#ifdef _WIN32
  #ifdef NEWPORT_XPS_BUILDING_DLL
    #define NEWPORT_XPS_API __declspec(dllexport)
  #else
    #define NEWPORT_XPS_API __declspec(dllimport)
  #endif

  // C4251: STL types used as private members of an exported class require
  // dll-interface. Safe to suppress — callers never access these members
  // directly; they go through exported methods only. Both sides must link
  // the same MSVC runtime (enforced by vcpkg's x64-windows triplet).
  #pragma warning(disable: 4251)
#else
  #define NEWPORT_XPS_API __attribute__((visibility("default")))
#endif

#endif  // NEWPORT_XPS_XPS_EXPORT_H_
