#include "vcgsimple.h"
#include "mesh_model.h"

#include <vcg/complex/algorithms/clean.h>
#include <vcg/complex/algorithms/stat.h>
#include <vcg/complex/algorithms/smooth.h>
#include <vcg/complex/algorithms/hole.h>
#include <vcg/complex/algorithms/refine_loop.h>
#include <vcg/complex/algorithms/bitquad_support.h>
#include <vcg/complex/algorithms/bitquad_creation.h>
#include <vcg/complex/algorithms/clustering.h>
#include <vcg/complex/algorithms/attribute_seam.h>
#include <vcg/complex/algorithms/update/curvature.h>
#include <vcg/complex/algorithms/update/curvature_fitting.h>
#include <vcg/complex/algorithms/pointcloud_normal.h>
#include <vcg/complex/algorithms/isotropic_remeshing.h>
#include <vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric.h>

#include <vcg/space/fitting3.h>
 
using namespace vcg;


void log(const std::string& info)
{
	std::cout << info << std::endl;
}



void QuadricTexSimplification(double& TargetError,CMeshO &m,int  TargetFaceNum, bool Selected, tri::TriEdgeCollapseQuadricTexParameter &pp, CallBackPos *cb)
{
  tri::UpdateNormal<CMeshO>::PerFace(m);

	math::Quadric<double> QZero;
	QZero.SetZero();
  tri::QuadricTexHelper<CMeshO>::QuadricTemp TD3(m.vert,QZero);
  tri::QuadricTexHelper<CMeshO>::TDp3()=&TD3;

  std::vector<std::pair<vcg::TexCoord2<float>,Quadric5<double> > > qv;

  tri::QuadricTexHelper<CMeshO>::Quadric5Temp TD(m.vert,qv);
  tri::QuadricTexHelper<CMeshO>::TDp()=&TD;

 
	
  vcg::LocalOptimization<CMeshO> DeciSession(m,&pp);

	DeciSession.Init<tri::MyTriEdgeCollapseQTex>();

	int faceSize = m.FN()*0.9;

	DeciSession.SetTargetSimplices(faceSize);
	DeciSession.SetTimeBudget(0.1f);
	int nNum = 0;
	while (m.FN() > TargetFaceNum && nNum <30 )
	{
		while (DeciSession.DoOptimization() && m.fn > faceSize)
		{
		};

		faceSize = (m.FN() * 0.9);

		if (faceSize < TargetFaceNum)
			faceSize = TargetFaceNum;

		DeciSession.SetTargetSimplices(faceSize);
		nNum++;
	}

 	TargetError = DeciSession.currMetric;

	DeciSession.Finalize<tri::MyTriEdgeCollapseQTex>();

	
  tri::QuadricTexHelper<CMeshO>::TDp3()=nullptr;
  tri::QuadricTexHelper<CMeshO>::TDp()=nullptr;
}


void QuadricSimplification(double& TargetError,  CMeshO &m, int  TargetFaceNum, bool Selected, tri::TriEdgeCollapseQuadricParameter &pp, CallBackPos *cb)
{
	math::Quadric<double> QZero;
	QZero.SetZero();
	tri::QuadricTemp TD(m.vert, QZero);
	tri::QHelper::TDp() = &TD;

	vcg::LocalOptimization<CMeshO> DeciSession(m, &pp);

	DeciSession.Init<tri::MyTriEdgeCollapse >();

	int faceSize = m.fn*0.9;

	DeciSession.SetTargetSimplices(faceSize);
	DeciSession.SetTimeBudget(0.1f);

	TargetError = DeciSession.currMetric;

	int nNum = 0;
	double maxError=1000;
	while (m.fn > TargetFaceNum && nNum <30 && TargetError < maxError )
	{
		while (DeciSession.DoOptimization() && m.fn > faceSize && TargetError<maxError)
		{
			TargetError = DeciSession.currMetric;
		};

		faceSize = (m.fn * 0.9);
		if (faceSize < TargetFaceNum)
			faceSize = TargetFaceNum;

		DeciSession.SetTargetSimplices(faceSize);
		nNum++;
	}

	TargetError = DeciSession.currMetric;

	DeciSession.Finalize<tri::MyTriEdgeCollapse >();

	tri::QHelper::TDp() = nullptr;
}

bool quadric_simplificationTex(double& TargetError, MeshModel& m,double TargetFaceNum,bool bkeepBorder)
{
	if (!tri::Clean<CMeshO>::HasConsistentPerWedgeTexCoord(*(m.cm) ) )
	{
		return false;
 	}

	tri::TriEdgeCollapseQuadricTexParameter pp;

	pp.QualityThr = 0.3;
	pp.ExtraTCoordWeight = 0.5;
	pp.OptimalPlacement = false;
	pp.PreserveBoundary =bkeepBorder;
	pp.BoundaryWeight = 1.0;
	pp.QualityQuadric = false;
	pp.NormalCheck = false;

	pp.PreserveTopology = false;
	pp.ScaleFactor = 1.0;

	QuadricTexSimplification(TargetError,*(m.cm),TargetFaceNum, false, pp, nullptr); 

	return true;
}


bool quadric_simplification(double& TargetError, MeshModel& m,double factor,bool bkeepBorder)
{
	bool bTexture=	m.hasPerVertexTexCoord();

	int delvert = tri::Clean<CMeshO>::RemoveDuplicateVertex(*(m.cm) );
	if (delvert != 0)
	{
		m.updateBoxAndNormals();
	}
	m.clearDataMask(MeshModel::MM_FACEFACETOPO);
	m.clearDataMask(MeshModel::MM_VERTFACETOPO);


	m.updateDataMask(MeshModel::MM_VERTFACETOPO | MeshModel::MM_VERTMARK);
	tri::UpdateFlags<CMeshO>::FaceBorderFromVF(*(m.cm));

	int TargetFaceNum = m.cm->fn* factor;
	if(TargetFaceNum == 0)
		return false;

	//ÊÇ·ñ±ØÒª
	if (TargetFaceNum <= 5)
		TargetFaceNum += 3;
	else if (TargetFaceNum <= 10)
		TargetFaceNum += 5;
	else if (TargetFaceNum <= 20)
		TargetFaceNum += 9;
	else if (TargetFaceNum <= 40)
		TargetFaceNum += 17;
	else if (TargetFaceNum <= 80)
		TargetFaceNum += 33;
	else if (TargetFaceNum <= 160)
		TargetFaceNum += 64;
	else if (TargetFaceNum <= 320)
		TargetFaceNum += 129;
	else if (TargetFaceNum <= 640)
		TargetFaceNum += 257;

	if (bTexture)
	{
		bool flag= quadric_simplificationTex(TargetError,m,TargetFaceNum,bkeepBorder);
		if(flag)
			return true;
	}

	tri::TriEdgeCollapseQuadricParameter pp;

	pp.QualityThr = 0.3;
	//pp.PreserveBoundary = false;
	pp.PreserveBoundary =bkeepBorder;
	pp.BoundaryQuadricWeight = 0.5;
	pp.PreserveTopology = false;
	pp.QualityWeight = false;
	pp.NormalCheck = false;
	pp.OptimalPlacement = false;
	pp.QualityQuadric = false;
	pp.QualityQuadricWeight = 0.001;
	pp.ScaleFactor = 1.0;
	 
	QuadricSimplification(TargetError,  *(m.cm), TargetFaceNum, false, pp, nullptr);

	bool AutoClean = true;
	if (AutoClean)
	{
		int nullFaces = tri::Clean<CMeshO>::RemoveFaceOutOfRangeArea(*m.cm, 0);
		int deldupvert = tri::Clean<CMeshO>::RemoveDuplicateVertex(*m.cm);
		int delvert = tri::Clean<CMeshO>::RemoveUnreferencedVertex(*m.cm);

		m.clearDataMask(MeshModel::MM_FACEFACETOPO);

		tri::Allocator<CMeshO>::CompactVertexVector(*m.cm);
		tri::Allocator<CMeshO>::CompactFaceVector(*m.cm);
	}

	//m.updateBoxAndNormals();
	//tri::UpdateNormal<CMeshO>::NormalizePerFace(*m.cm);
	//tri::UpdateNormal<CMeshO>::PerVertexFromCurrentFaceNormal(*m.cm);
	//tri::UpdateNormal<CMeshO>::NormalizePerVertex(*m.cm);

	return true;
}
