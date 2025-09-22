#include "simple.h"
#include "cmesh.h"
#include "vcgSimple.h"
#include "mesh_model.h"
#include "../wraperCesium.h"
#include "../threadpool.h"
#include "wrap/io_trimesh/io_mask.h"
#include "vcg/complex/algorithms/clean.h"

using namespace vcg;
 
 

void toMesh_func(wraperCesium::GMesh& mesh, CMeshO& m, int mask)
{
	m.Clear();

	int nPtSize = mesh.positions.size();

	bool bText = false;
	if (mesh.UVs.size() == nPtSize)
	{
		bText = true;
	}

	bool bColor = false;
	if (mesh.colors.size() == nPtSize)
	{
		bColor = true;
	}

	bool bNormal = false;
	if (mesh.normals.size() == nPtSize)
	{
		bNormal = true;
	}

 
	vcg::Point3<float> onePtNor;
	vcg::Point4<unsigned char> onePtColor;
	int num = mesh.indices.size();


	CMeshO::VertexIterator vi = vcg::tri::Allocator<CMeshO>::AddVertices(m, num);

	for (int j = 0; j < num; j++)
	{
		(*vi).P()[0] = mesh.positions[j][0];
		(*vi).P()[1] = mesh.positions[j][1];
		(*vi).P()[2] = mesh.positions[j][2];
		++vi;
	}

	vcg::tri::Allocator<CMeshO>::AddFaces(m, num / 3);

	for (int i = 0; i < num / 3; i++)
	{
		m.face[i].Alloc(3);  
 
		onePtNor = { 1,0,0 };
		for (int j = 0; j < 3; ++j)
		{
			m.face[i].V(j) = &(m.vert[mesh.indices[3*i+j]] );

			if (bText)
			{
				osg::Vec2 t = mesh.UVs[mesh.indices[3 * i + j]];	

				m.face[i].V(j)->T().u() = t.x();
				m.face[i].V(j)->T().v() = t.y();
				m.face[i].V(j)->T().n() = 0;
			}
			if (bNormal)
			{
				osg::Vec3 t = mesh.normals[mesh.indices[3 * i + j]];

				onePtNor[0] = t[0];
				onePtNor[1] = t[1];
				onePtNor[2] = t[2];

				m.face[i].V(j)->N().Import(onePtNor);
			}

			if (bColor)
			{
				osg::Vec4 color = (mesh.colors[mesh.indices[3 * i + j]]);

				onePtColor[0] = color[0] * 255;
				onePtColor[1] = color[1] * 255;
				onePtColor[2] = color[2] * 255;
				onePtColor[3] = color[3] * 255;

 				m.face[i].V(j)->C().Import(onePtColor);	
			}
		 
			m.face[i].ClearF(j);
		}

		if (HasPerFaceNormal(m))
		{
			m.face[i].N().Import(TriangleNormal(m.face[i]).Normalize());
		}
 	}


}
void fromMesh_func(wraperCesium::GMesh& mesh, CMeshO& m)
{
	int nPtSize = mesh.positions.size();


	bool bText = false;
	if (mesh.UVs.size() == nPtSize)
		bText = true;
 
	bool bColor = false;
	if (mesh.colors.size() == nPtSize)
		bColor = true;

	bool bNormal = false;
	if (mesh.normals.size() == nPtSize)
	{
		bNormal = true;
	}

	std::vector<int> VertexId(m.vert.size());
 
 
	std::vector<osg::Vec3d> positions;
	std::vector<osg::Vec4> colors;
	std::vector<osg::Vec2> uvs;
	std::vector<osg::Vec3> normals;

	positions.resize(m.vn);

	if(bColor)
		colors.resize(m.vn);
	if (bText)
		uvs.resize(m.vn);

	if (bNormal)
		normals.resize(m.vn);


	{
		int numvert = 0;
		for (auto vi = m.vert.begin(); vi != m.vert.end(); ++vi)
		{
			if ((*vi).IsD())continue;

			VertexId[vi - m.vert.begin()] = numvert;

			const auto& xx = vi->P();

			float x = xx[0];
			float y = xx[1];
			float z = xx[2];
			positions.at(numvert) = osg::Vec3(x, y, z);
	
			if (bColor)
			{
				const auto& xx = vi->C();

				float x = xx[0]/255.0;
				float y = xx[1]/255.0;
				float z = xx[2]/255.0;
				float w = xx[3]/255.0;
				colors.at(numvert) =osg::Vec4{ x,y,z,w };
			}
			if (bNormal)
			{
				const auto& xx = vi->N();

				float x = xx[0];
				float y = xx[1];
				float z = xx[2];
 				normals.at(numvert) = osg::Vec3{ x,y,z};
			}
			if (bText)
			{
				const auto& xx = vi->T();

				float x = xx.u();
				float y = xx.v();
		 
				uvs.at(numvert) = osg::Vec2{ x,y};
			}
			numvert++; 
		}


		///////face
		std::vector<int> p1s;
		std::vector<int> p2s;
		std::vector<int> p3s;
		p1s.resize(m.fn);
		p2s.resize(m.fn);
		p3s.resize(m.fn);

		//
		std::vector<osg::Vec2> uvs1;
		std::vector<osg::Vec2> uvs2;
		std::vector<osg::Vec2> uvs3;
		uvs1.resize(m.fn);
		uvs2.resize(m.fn);
		uvs3.resize(m.fn);


		int order = 0;
 		for (auto fi = m.face.begin(); fi != m.face.end(); ++fi)
		{
			if(  fi->IsD() )continue;
		 
			for (int k = 0; k < fi->VN(); k++)
			{
				int vInd = VertexId[vcg::tri::Index(m, (*fi).V(k))];

				if ( k == 0 )
					p1s[order]=(vInd);
				else if (k == 1)
					p2s[order] = (vInd);
				else if (k == 2)
					p3s[order] = (vInd);
			}
			order++;
 		}	

		int triangelCount = p1s.size();

		mesh.positions.resize(triangelCount * 3);
		mesh.indices.resize(triangelCount * 3);

		if(bColor)
			mesh.colors.resize(triangelCount * 3);
		if(bText)
			mesh.UVs.resize(triangelCount * 3);
		if (bNormal)
			mesh.normals.resize(triangelCount * 3);

		for (int j = 0; j < triangelCount; j++)
		{
			mesh.indices[3 * j + 0] = (3 * j + 0);
			mesh.indices[3 * j + 1] = (3 * j + 1);
			mesh.indices[3 * j + 2] = (3 * j + 2);

			mesh.positions[3 * j + 0] = (positions[p1s[j]]);
			mesh.positions[3 * j + 1] = (positions[p2s[j]]);
			mesh.positions[3 * j + 2] = (positions[p3s[j]]);

			if (bColor)
			{
				mesh.colors[3 * j + 0]=(colors[p1s[j]]);
				mesh.colors[3 * j + 1]=(colors[p2s[j]]);
				mesh.colors[3 * j + 2]=(colors[p3s[j]]);
			}
			if (bText)
			{
				mesh.UVs[3 * j + 0] = (uvs[p1s[j]]);
				mesh.UVs[3 * j + 1] = (uvs[p2s[j]]);
				mesh.UVs[3 * j + 2] = (uvs[p3s[j]]);
			}
			if (bNormal)
			{
				mesh.normals[3 * j + 0] = (normals[p1s[j]]);
				mesh.normals[3 * j + 1] = (normals[p2s[j]]);
				mesh.normals[3 * j + 2] = (normals[p3s[j]]);
			}
		}
	} 
}

 
namespace SIMPLE
{
	void  simple_Function(wraperCesium::GMesh* mesh, double factor, bool bKeepBorder)
	{
		int mask = 0;
		if (!mesh->UVs.empty())
		{
			mask |= vcg::tri::io::Mask::IOM_VERTTEXCOORD;

			mask |= vcg::tri::io::Mask::IOM_WEDGTEXCOORD;
			mask |= vcg::tri::io::Mask::IOM_FACECOLOR;

			mask |= vcg::tri::io::Mask::IOM_FACECOLOR;
		}
		if (!mesh->colors.empty())
		{
			mask |= vcg::tri::io::Mask::IOM_VERTCOLOR;
		}
		if (!mesh->batchIDs.empty())
		{
			mask |= vcg::tri::io::Mask::IOM_VERTNORMAL;
		}

		MeshModel mm(1, "1", "1");
		CMeshO m;
		mm.cm = &m;
		mm.enable(mask);

		toMesh_func(*mesh, m, mask);

		double error = 0;
		bool flag = quadric_simplification(error, mm, factor, bKeepBorder);
		if (flag)
			fromMesh_func(*mesh, m);
 	}

	int threadCount = 1;

	bool SimpleMeshByFator_func(wraperCesium::GMesh& mesh, double factor, double& targetError)
	{
		if (mesh.primitiveType == 1)return true;


		std::unordered_map<int,wraperCesium::GMesh> objIDs;

		int nTri = mesh.indices.size() / 3;

		for (int i = 0; i < nTri ; i++)
		{
			int p1 = mesh.indices[3 * i + 0];
			int p2 = mesh.indices[3 * i + 1];
			int p3 = mesh.indices[3 * i + 2];
			auto it = objIDs.find(mesh.batchIDs[p1]);
			if (it != objIDs.end())
			{
				int nSize = it->second.indices.size();

				it->second.indices.push_back(nSize+0);
				it->second.indices.push_back(nSize+1);
				it->second.indices.push_back(nSize+2);

				it->second.batchIDs.push_back(mesh.batchIDs[p1]);
				it->second.batchIDs.push_back(mesh.batchIDs[p2]);
				it->second.batchIDs.push_back(mesh.batchIDs[p3]);

				it->second.positions.push_back(mesh.positions[p1]);
				it->second.positions.push_back(mesh.positions[p2]);
				it->second.positions.push_back(mesh.positions[p3]);

				if (!mesh.UVs.empty())
				{
					it->second.UVs.push_back(mesh.UVs[p1]);
					it->second.UVs.push_back(mesh.UVs[p2]);
					it->second.UVs.push_back(mesh.UVs[p3]);
				}
				if (!mesh.colors.empty())
				{
					it->second.colors.push_back(mesh.colors[p1]);
					it->second.colors.push_back(mesh.colors[p2]);
					it->second.colors.push_back(mesh.colors[p3]);
				}
			}
			else
			{
				int nSize = 0;
				wraperCesium::GMesh partMesh;

				partMesh.indices.push_back(nSize + 0);
				partMesh.indices.push_back(nSize + 1);
				partMesh.indices.push_back(nSize + 2);

				partMesh.batchIDs.push_back(mesh.batchIDs[p1]);
				partMesh.batchIDs.push_back(mesh.batchIDs[p2]);
				partMesh.batchIDs.push_back(mesh.batchIDs[p3]);

				partMesh.positions.push_back(mesh.positions[p1]);
				partMesh.positions.push_back(mesh.positions[p2]);
				partMesh.positions.push_back(mesh.positions[p3]);

				if (!mesh.UVs.empty())
				{
					partMesh.UVs.push_back(mesh.UVs[p1]);
					partMesh.UVs.push_back(mesh.UVs[p2]);
					partMesh.UVs.push_back(mesh.UVs[p3]);
				}
				if (!mesh.colors.empty())
				{
					partMesh.colors.push_back(mesh.colors[p1]);
					partMesh.colors.push_back(mesh.colors[p2]);
					partMesh.colors.push_back(mesh.colors[p3]);
				}

				objIDs[mesh.batchIDs[p1]] = partMesh;
			}
		}

		bool bKeepBorder = false;
		if (abs(mesh.batchIDs[0] - 9999999) < 0.1)
			bKeepBorder = true;

		for (auto& item : objIDs) {
			simple_Function(&item.second, factor, bKeepBorder);
		}
 
		//merge
		clearMesh(mesh);
  
		for (auto& item : objIDs)
		{
			item.second.batchIDs.resize(item.second.positions.size(), item.first);

			int nTri = item.second.indices.size() / 3;
			for (int i=0; i< nTri; i++)
			{
				int p1 = item.second.indices[3 * i + 0];
				int p2 = item.second.indices[3 * i + 1];
				int p3 = item.second.indices[3 * i + 2];
	 
				int nSize =mesh.indices.size();
				mesh.indices.push_back(nSize + 0);
				mesh.indices.push_back(nSize + 1);
				mesh.indices.push_back(nSize + 2);

				mesh.positions.push_back(item.second.positions[p1]);
				mesh.positions.push_back(item.second.positions[p2]);
				mesh.positions.push_back(item.second.positions[p3]);

				mesh.batchIDs.push_back(item.first);
				mesh.batchIDs.push_back(item.first);
				mesh.batchIDs.push_back(item.first);


				if (!item.second.UVs.empty())
				{
					mesh.UVs.push_back(item.second.UVs[p1]);
					mesh.UVs.push_back(item.second.UVs[p2]);
					mesh.UVs.push_back(item.second.UVs[p3]);
				}
				if (!item.second.colors.empty())
				{
					mesh.colors.push_back(item.second.colors[p1]);
					mesh.colors.push_back(item.second.colors[p2]);
					mesh.colors.push_back(item.second.colors[p3]);
				}
			}
		}

		return true;
	}  


	//
	void SplitMesh_func(wraperCesium::GMesh& mesh, std::vector<wraperCesium::GMesh>& subMeshs)
	{
	 
	}
}

