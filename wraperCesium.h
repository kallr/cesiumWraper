#include <string>
#include <vector>
#include "osg/Vec3"
#include "osg/Vec2"
#include "osg/Vec4"
#include "osg/BoundingBox"
#include "osg/Geometry"
#include <cstddef>


// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the WRAPERCESIUM_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// WRAPERCESIUM_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WRAPERCESIUM_EXPORTS
#define WRAPERCESIUM_API __declspec(dllexport)
#else
#define WRAPERCESIUM_API __declspec(dllimport)
#endif

namespace osg
{
	class Group;
}


 namespace wraperCesium
 {
		class GMesh
		{
		public:
			int primitiveType = 4;
			int level = 0;
			int nFace = 0;
			bool bText=false;
			osg::BoundingBox aabb;
			std::vector<unsigned int> indices;
			std::vector< osg::Vec3> positions;
			std::vector< osg::Vec2> UVs;
			std::vector< osg::Vec4> colors;
			std::vector<float>batchIDs;
		 };
		void clearMesh(GMesh& smesh);

		WRAPERCESIUM_API  bool compressTexture(std::ostream& fout,int s, int t ,const uint8_t* rgb, int rgb_stride,int imgFormat);
		WRAPERCESIUM_API 	bool compresTextureKTX(std::ostream& fout,int s, int t ,const uint8_t* rgb, int internalTextureFormat,int totalSizeInBytes);

		WRAPERCESIUM_API bool b3dm2Glb(const std::string& b3dm_buf ,std::vector<std::byte>& gltfBytesCompact);
		WRAPERCESIUM_API bool compressGlb(std::vector<std::byte>& data , const std::string& file);
		WRAPERCESIUM_API bool compressGlbWithMeshOpt(std::vector<std::byte>& data , const std::string& file,bool textureRef);

		//gltfPack
		WRAPERCESIUM_API bool compressGlbWithMeshOptEx(const std::string& inputFIle , const std::string& file, const std::string& strParam);

		//read gltf
		WRAPERCESIUM_API  osg::Group* cesiumLoadGltf(std::string& data,const std::string& textureDir);


	WRAPERCESIUM_API void  SplitMeshToQuard(bool bDK,int maxLevel, GMesh& mesh, std::vector< GMesh >& childMeshs,int maxPt);

	WRAPERCESIUM_API bool  simpleMesh(GMesh& mesh, double factor, double& targetError);

 	WRAPERCESIUM_API bool  splitMesh(GMesh& mesh,  std::vector<GMesh>&subMesh);

 }
 
