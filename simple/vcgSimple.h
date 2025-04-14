
#pragma  once
 
#include <vcg/container/simple_temporary_data.h>
#include <vcg/complex/algorithms/local_optimization.h>
#include <vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric.h>
#include <vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric_tex.h>
#include "cmesh.h"
#include "mesh_model.h"


namespace vcg
{
	namespace tri 
	{
		typedef	SimpleTempData<CMeshO::VertContainer, math::Quadric<double> > QuadricTemp;


		class QHelper
		{
		public:
			QHelper() {}
			static void Init() {}
			static math::Quadric<double> &Qd(CVertexO &v) { return TD()[v]; }
			static math::Quadric<double> &Qd(CVertexO *v) { return TD()[*v]; }
			static CVertexO::ScalarType W(CVertexO * /*v*/) { return 1.0; }
			static CVertexO::ScalarType W(CVertexO & /*v*/) { return 1.0; }
			static void Merge(CVertexO & /*v_dest*/, CVertexO const & /*v_del*/) {}
			static QuadricTemp* &TDp()
			{
				thread_local  static QuadricTemp *td;
				return td;
			}
			static QuadricTemp &TD() { return *TDp(); }
		};

		typedef BasicVertexPair<CVertexO> VertexPair;

		class MyTriEdgeCollapse : public vcg::tri::TriEdgeCollapseQuadric< CMeshO, VertexPair, MyTriEdgeCollapse, QHelper > {
		public:
			typedef  vcg::tri::TriEdgeCollapseQuadric< CMeshO, VertexPair, MyTriEdgeCollapse, QHelper> TECQ;
			inline MyTriEdgeCollapse(const VertexPair &p, int i, BaseParameterClass *pp) :TECQ(p, i, pp) 
			{
			}
		};

		class MyTriEdgeCollapseQTex : public TriEdgeCollapseQuadricTex< CMeshO, VertexPair, MyTriEdgeCollapseQTex, QuadricTexHelper<CMeshO> > {
		public:
			typedef  TriEdgeCollapseQuadricTex< CMeshO, VertexPair, MyTriEdgeCollapseQTex, QuadricTexHelper<CMeshO> > TECQ;
			inline MyTriEdgeCollapseQTex(const VertexPair &p, int i, BaseParameterClass *pp) :TECQ(p, i, pp) {}
		};
	}
}

bool quadric_simplification( double& TargetError, MeshModel & gmesh,double factor,bool bkeepBorder);
