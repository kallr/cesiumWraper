
#include <vector>
namespace wraperCesium
{
	class GMesh;
}
namespace SIMPLE
{
	/**
	 * 
	 */
	bool SimpleMeshByFator_func(wraperCesium::GMesh& mesh, double factor, double& targetError);

	void SplitMesh_func(wraperCesium::GMesh& mesh, std::vector<wraperCesium::GMesh>& subMesh);

}

