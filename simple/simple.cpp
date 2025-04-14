#include "simple.h"
#include "cmesh.h"
#include "vcgSimple.h"
#include "mesh_model.h"
#include "../wraperCesium.h"
#include "../threadpool.h"
using namespace vcg;
 
void GeometryToMesh( wraperCesium::GMesh& mesh, osg::Geometry* geometry)
{
	osg::Vec3Array *pVertices = (osg::Vec3Array*)geometry->getVertexArray();
	osg::Vec3Array* inputNormals = (osg::Vec3Array*)geometry->getNormalArray();
	osg::Vec4Array* inputColors = (osg::Vec4Array*)geometry->getColorArray();
	osg::Vec2Array* tex_coords = (osg::Vec2Array*)(geometry->getTexCoordArray(0));
 
	bool bColor = false;
	if (inputColors && (inputColors->getBinding() == 4))
	{
		if(inputColors->size() == pVertices->size() )
			bColor = true;	 
	}
	bool bTC = false;
	if (tex_coords && (tex_coords->getBinding() == 4))
	{
		if (tex_coords->size() == pVertices->size())
			bTC = true;
	}

	int nSet = geometry->getNumPrimitiveSets();
	for (int i = 0; i < nSet; i++)
	{
		osg::PrimitiveSet*  pPrimitiveSet = geometry->getPrimitiveSet(i);
		osg::PrimitiveSet::Type type = pPrimitiveSet->getType();
		if (type == osg::PrimitiveSet::DrawElementsUIntPrimitiveType || type == osg::PrimitiveSet::DrawElementsUShortPrimitiveType || type == osg::PrimitiveSet::DrawArraysPrimitiveType)
		{
			auto mode = pPrimitiveSet->getMode();

			if (mode == GL_TRIANGLES)
			{
				int num = pPrimitiveSet->getNumIndices();
				for (int i = 0; i < num; i++)
				{
					int index = pPrimitiveSet->index(i);
					
					mesh.positions.push_back(pVertices->at(index) );

					if (bColor)
					{
						mesh.colors.push_back(inputColors->at(index));
					}		 
					if (bTC)
					{
						mesh.UVs.push_back(tex_coords->at(index)); 
					}	

					mesh.indices.push_back(i);
				}
			}
			else
			{
				int kk = 0;
			}
		}
		else
		{
			int kk = 0;
		}
	}

	mesh.batchIDs.resize(mesh.positions.size(), 0);
}


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

	bool bID = false;
	//if (mesh.batchIDs.size() == nPtSize)
	//{
	//	bID = true;
	//}

	int idF = 255 * 255;

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
				m.face[i].WT(j).u() = t.x();
				m.face[i].WT(j).v() = t.y();
				m.face[i].WT(j).n() = 0;

				m.face[i].V(j)->T().u() = t.x();
				m.face[i].V(j)->T().v() = t.y();
				m.face[i].V(j)->T().n() = 0;
			}
			if (bID)
			{
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
			if (bText )
			{
				m.face[i].C() = vcg::Color4b(255);
 			}

			m.face[i].N().Import(TriangleNormal(m.face[i]).Normalize());
		}
 	}


}
void fromMesh_func(wraperCesium::GMesh& mesh, CMeshO& m)
{
	int nPtSize = mesh.positions.size();

	int idF = 256 * 256;
	bool bText = false;
	if (mesh.UVs.size() == nPtSize)
		bText = true;
 
	bool bColor = false;
	if (mesh.colors.size() == nPtSize)
		bColor = true;


	std::vector<int> VertexId(m.vert.size());
 
 
	std::vector<osg::Vec3d> positions;
	std::vector<osg::Vec4> colors;
	positions.resize(m.vn);

	if(bColor)
		colors.resize(m.vn); 
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

				if (bText)
				{
					float uu = (*fi).WT(k).u();
					float vv = (*fi).WT(k).v();
					if (k == 0)
					{
						uvs1[order] = { uu, vv };
					}
					if (k == 1)
					{
						uvs2[order] = { uu, vv };
					}
					if (k == 2)
					{
						uvs3[order] = { uu, vv };
					}
 				}

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
				mesh.UVs[3 * j + 0] = uvs1[j];
				mesh.UVs[3 * j + 1] = uvs2[j];
				mesh.UVs[3 * j + 2] = uvs3[j];
			}
		}
	} 
}

////////////////////////////////公用顶点
void fromMesh_funcEx(wraperCesium::GMesh& mesh, CMeshO& m)
{
	int nPtSize = mesh.positions.size();

	int idF = 256 * 256;
	bool bText = false;
	if (mesh.UVs.size() == nPtSize)
		bText = true;
 
	bool bColor = false;
	if (mesh.colors.size() == nPtSize)
		bColor = true;


	std::vector<int> VertexId(m.vert.size());
 
 
	std::vector<osg::Vec3> positions;
	std::vector<osg::Vec4> colors;
	positions.resize(m.vn);

	if(bColor)
		colors.resize(m.vn);

 

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
			numvert++; 
		}
 
		//
		std::vector<osg::Vec2> uvs1;
		if(bText)
			uvs1.resize(numvert);

		mesh.indices.clear();

		int order = 0;
 		for (auto fi = m.face.begin(); fi != m.face.end(); ++fi)
		{
			if(  fi->IsD() )continue;
		 
			for (int k = 0; k < fi->VN(); k++)
			{
				int vInd = VertexId[vcg::tri::Index(m, (*fi).V(k))];
				mesh.indices.push_back(vInd);
				order++;

				if (bText)
				{
					float uu = (*fi).WT(k).u();
					float vv = (*fi).WT(k).v();
					uvs1[vInd] = { uu, vv };	
 				}
			}
 		}	

 
		mesh.positions=positions;
 
		if(bColor)
			mesh.colors=colors;

		if(bText)
			mesh.UVs.swap(uvs1);

 	} 
}

namespace SIMPLE
{
	void  simple_Function(wraperCesium::GMesh* mesh, double factor, bool bKeepBorder)
	{
		int nTri = mesh->indices.size() / 3;

		int nFinal = nTri * factor;

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

		if (factor < 0.02)
			factor = 0.02;

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

		struct threadParam {
			int nCount = 0;
			int iterator = 0;
			std::mutex lock;
			double factor;
			bool bKeepB;
			std::vector< std::pair<int, wraperCesium::GMesh* > > objArray;
		};

		if (threadCount >1 )
		{
			threadParam params;
			params.factor = factor;
			params.bKeepB = bKeepBorder;
			params.nCount = objIDs.size();

 			for (auto& item : objIDs)
			{
				params.objArray.push_back(std::make_pair(item.first, &item.second));
			}

			// 采用多线程执行
			auto gen_dispatch = [](threadParam* param) -> void {
				int count = param->nCount;
				int row;
				while (true)
				{
					{
						std::lock_guard<std::mutex> lck(param->lock);
						row = param->iterator++;
						if (row >= count)
							break;
					}

					simple_Function( param->objArray[row].second,param->factor, param->bKeepB);
				}
			};


			std::vector<std::thread> threads(threadCount);
			for (int ti = 0; ti < threadCount; ti++) {
				threads[ti] = std::move(std::thread(gen_dispatch, &params));
			}
			std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));	 
		}
		else
		{
			for (auto& item : objIDs){
				simple_Function(&item.second, factor, bKeepBorder);
 			}
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
		int mask = 0;
		if (!mesh.UVs.empty())
		{
			mask |= vcg::tri::io::Mask::IOM_VERTTEXCOORD;

			mask |= vcg::tri::io::Mask::IOM_WEDGTEXCOORD;
			mask |= vcg::tri::io::Mask::IOM_FACECOLOR;

			mask |= vcg::tri::io::Mask::IOM_FACECOLOR;
		}
		if (!mesh.colors.empty())
		{
			mask |= vcg::tri::io::Mask::IOM_VERTCOLOR;
		}
		if (!mesh.batchIDs.empty())
		{
			mask |= vcg::tri::io::Mask::IOM_VERTNORMAL;
		}

		MeshModel currentModel(1, "1", "1");
		CMeshO cm;
		currentModel.cm = &cm;
		currentModel.enable(mask);
		toMesh_func(mesh, cm,mask);

		int delvert = tri::Clean<CMeshO>::RemoveDuplicateVertex(cm);


 		bool removeSourceMesh =false;

		currentModel.updateDataMask(MeshModel::MM_FACEFACETOPO);
		std::vector<std::pair<int, CMeshO::FacePointer>> connectedCompVec;
		int numCC = tri::Clean<CMeshO>::ConnectedComponents(cm, connectedCompVec);

		tri::UpdateSelection<CMeshO>::FaceClear(cm);
		tri::UpdateSelection<CMeshO>::VertexClear(cm);

		for (size_t i = 0; i < connectedCompVec.size(); ++i) 
		{
			connectedCompVec[i].second->SetS();
			tri::UpdateSelection<CMeshO>::FaceConnectedFF(cm /*,true*/);
			tri::UpdateSelection<CMeshO>::VertexFromFaceLoose(cm);

			// create a new mesh from the selection
			std::string lable=std::to_string(i+1);
			MeshModel destModel(i+1,lable,lable);
			CMeshO cmi;
			destModel.cm = &cmi;

			destModel.updateDataMask(&currentModel);

			tri::Append<CMeshO, CMeshO>::Mesh(cmi, cm, true);

			// clear selection from source mesh and from newly created mesh
			tri::UpdateSelection<CMeshO>::FaceClear(cm);
			tri::UpdateSelection<CMeshO>::VertexClear(cm);
			tri::UpdateSelection<CMeshO>::FaceClear(cmi);
			tri::UpdateSelection<CMeshO>::VertexClear(cmi);	

			// init new layer
			destModel.updateBoxAndNormals();
			destModel.cm->Tr = cmi.Tr;

			wraperCesium::GMesh subMesh=mesh;
			fromMesh_funcEx(subMesh, cmi);

			subMeshs.push_back(subMesh);
		}
	}
}

