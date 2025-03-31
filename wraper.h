#include <vector>
#include <cstddef>
#include <string>
// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the TRANGLTF_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// TRANGLTF_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WRAPERCESIUM_EXPORTS
#define TRANGLTF_API __declspec(dllexport)
#else
#define TRANGLTF_API __declspec(dllimport)
#endif

// This class is exported from the dll
class TRANGLTF_API CtranGltf {
public:
	CtranGltf(void);
	bool  execFunc(std::vector<std::byte>& data , const std::string& file,double dR);
	// TODO: add your methods here.
};
