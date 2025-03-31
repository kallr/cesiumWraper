
#include <osg/Node>
#include <osg/Geode>
#include <osg/Vec3d>
#include <osg/Vec2>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/BoundingBox>
#include "osg/Geometry"

namespace wraperCesium
{
	class GMesh;
}
namespace SIMPLE
{
	/**Ä£ÐÍ¼ò»¯
	 * 
	 */
	bool SimpleMeshByFator_func(wraperCesium::GMesh& mesh, double factor, double& targetError);

	void SplitMesh_func(wraperCesium::GMesh& mesh, std::vector<wraperCesium::GMesh>& subMesh);

}

