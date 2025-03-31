// tranGltf.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "tranGltf.h"

#include <draco/compression/decode.h>
#include <draco/compression/encode.h>
#include <draco/mesh/mesh.h>
#include "draco/io/gltf_encoder.h"
#include "draco/io/gltf_decoder.h"
#include "draco/io/file_utils.h"
#include "draco/scene/scene_utils.h"
#include "draco/io/file_reader_factory.h"
#include "draco/io/file_writer_factory.h"
#include "draco/io/stdio_file_reader.h"
#include "draco/io/stdio_file_writer.h"

#pragma comment(lib, "draco.lib")  

 
// This is the constructor of a class that has been exported.
CtranGltf::CtranGltf()
{
  draco::FileReaderFactory::RegisterReader(draco::StdioFileReader::Open);
  draco::FileWriterFactory::RegisterWriter(draco::StdioFileWriter::Open);
}

bool CtranGltf::execFunc(std::vector<std::byte>& data,  const std::string& filepath,double dR)
{
   //std::vector<unsigned char> out;
   //draco::ReadFileToBuffer(filepath, &out);

  draco::DecoderBuffer dracoBuffer;
  dracoBuffer.Init(reinterpret_cast<char *>(data.data()), data.size());

  draco::GltfDecoder decoder;
  auto maybe_scene = decoder.DecodeFromBufferToScene(&dracoBuffer);
  if (maybe_scene.ok())
  {
    std::unique_ptr<draco::Scene> scene = std::move(maybe_scene).value();

    draco::DracoCompressionOptions options;
	options.compression_level=5;
	options.quantization_position.SetQuantizationBits(20);
    draco::SceneUtils::SetDracoCompressionOptions(&options, scene.get());

    std::string folder_path;
    std::string gltf_file_name;
    draco::SplitPath(filepath, &folder_path, &gltf_file_name);
    draco::GltfEncoder gltf_encoder;

    gltf_encoder.EncodeToFile<draco::Scene>(*scene, filepath, folder_path);

    return true;
  }
  else
  {
	  return false;
  }

}
