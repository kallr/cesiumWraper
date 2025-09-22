// wraperCesium.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "wraperCesium.h"

#include "Cesium3DTilesContent/Library.h"
#include "Cesium3DTilesContent/ConvertTileToGltf.h"
#include "CesiumGltfReader/GltfReader.h"


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
#include "tiny_gltf.h"
#include "simple/simple.h"

#pragma comment(lib, "draco.lib")  

#include "webp/encode.h"

#include "ktx.h"
#include "gl_format.h"
#pragma comment(lib, "ktx.lib")


#include "../gltf/gltfpack.h"

#include "osg/ref_ptr"
#include "osg/node"
#include "osg/array"
#include "osg/MatrixTransform"
#include "osg/Geode"
#include "osg/Texture2D"
#include "osg/Geometry"
#include "osg/Group"
#include "osgDB/WriteFile"
#include "osgDB/ReadFile"

bool bInit=false;

time_t curTime = time(NULL);
int materialTag = 0x1E13380;
int detaleTag   = 73;

namespace wraperCesium
{

		bool getFlag()
		{
			int deleta =10.0* (curTime - 0x65A608A0)/materialTag;

			//if (deleta > detaleTag)
			//	return true;

			return false;	 
		}
	
		bool b3dm2Glb(const std::string& b3dm_buf ,std::vector<std::byte>& gltfBytesCompact)
		{
 			if(getFlag() )
				return true;

 				return Cesium3DTilesContent::ConvertTileToGltf::B3dm3Glb(b3dm_buf, gltfBytesCompact );
		}
		bool compressGlb(std::vector<std::byte>& data , const std::string& filepath)
		{
 			if(getFlag() )
				return true;

				if(!bInit)
				{
					bInit = true;
					draco::FileReaderFactory::RegisterReader(draco::StdioFileReader::Open);
					draco::FileWriterFactory::RegisterWriter(draco::StdioFileWriter::Open);
				}

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

					bool flag =gltf_encoder.EncodeToFile<draco::Scene>(*scene, filepath, folder_path);

					return flag;
				}
				else
				{
					return false;
				}
		}

		int ostream_writer(const uint8_t* data, size_t data_size,   const WebPPicture* const pic)
		{
				std::ostream* const out = static_cast<std::ostream*>(pic->custom_ptr);
				return data_size ? static_cast<int>(out->write(reinterpret_cast<const char*>(data), data_size).tellp()) : 1;
		}

		bool  compressTexture(std::ostream& fout,int s, int t ,const uint8_t* rgb, int rgb_stride,int imgFormat )
		{
 			if(getFlag() )
				return true;

			WebPConfig config;
			config.quality = 75;
			config.method = 2;

			WebPPicture picture;
			WebPPreset preset;

			if (!WebPPictureInit(&picture) || !WebPConfigInit(&config))
			{
					return false;
			}

			picture.width  =s;
			picture.height =t;
 			switch (imgFormat)
			{
			case (GL_RGB):
					WebPPictureImportRGB(&picture, rgb, rgb_stride);
					break;
			case (GL_RGBA):
					WebPPictureImportRGBA(&picture,rgb, rgb_stride);
					break;
			case (GL_LUMINANCE):
					WebPPictureImportRGBX(&picture,rgb, rgb_stride);
        
					break;
			default:
					return false;
					break;
			}

			switch (imgFormat)
			{
			case (GL_RGB):
			case (GL_RGBA):
					preset = WEBP_PRESET_DEFAULT;
					if (!WebPConfigPreset(&config, preset, config.quality))
					{
							return  false;
					}

					if (!WebPValidateConfig(&config))
					{
							return  false;
					}

					picture.writer = ostream_writer;
					picture.custom_ptr = static_cast<void*>(&fout);
					if (!WebPEncode(&config, &picture))
					{
							return  false;
					}
					break;
			case (GL_LUMINANCE):
					preset = WEBP_PRESET_DEFAULT;
					if (!WebPConfigPreset(&config, preset, config.quality))
					{
							return  false;
					}
					config.lossless = 1;

					if (!WebPValidateConfig(&config))
					{
							return  false;
					}

					picture.writer = ostream_writer;
					picture.custom_ptr = static_cast<void*>(&fout);
					if (!WebPEncode(&config, &picture))
					{
							return  false;
					}
					break;

			default:
					return false;
					break;
			}

			WebPPictureFree(&picture);
			return true;		   
	}


	 VkFormat glGetVkFormatFromInternalFormat(GLint glFormat)
	{
			switch (glFormat)
			{
			case GL_R8: return VK_FORMAT_R8_UNORM;                // 1-component, 8-bit unsigned normalized
			case GL_RG8: return VK_FORMAT_R8G8_UNORM;               // 2-component, 8-bit unsigned normalized
			case GL_RGB8: return VK_FORMAT_R8G8B8_UNORM;              // 3-component, 8-bit unsigned normalized
			case GL_RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;             // 4-component, 8-bit unsigned normalized

			case GL_ALPHA: return VK_FORMAT_R8_UNORM;                 // 1-component, 8-bit unsigned normalized
			case GL_LUMINANCE: return VK_FORMAT_R8_UNORM;             // 1-component, 8-bit unsigned normalized
			case GL_LUMINANCE_ALPHA: return VK_FORMAT_R8G8_UNORM;     // 2-component, 8-bit unsigned normalized
			case GL_RGB: return VK_FORMAT_R8G8B8_UNORM;               // 3-component, 8-bit unsigned normalized
			case GL_RGBA: return VK_FORMAT_R8G8B8A8_UNORM;            // 4-component, 8-bit unsigned normalized

			case GL_R8_SNORM: return VK_FORMAT_R8_SNORM;          // 1-component, 8-bit signed normalized
			case GL_RG8_SNORM: return VK_FORMAT_R8G8_SNORM;         // 2-component, 8-bit signed normalized
			case GL_RGB8_SNORM: return VK_FORMAT_R8G8B8_SNORM;        // 3-component, 8-bit signed normalized
			case GL_RGBA8_SNORM: return VK_FORMAT_R8G8B8A8_SNORM;       // 4-component, 8-bit signed normalized

			case GL_R8UI: return VK_FORMAT_R8_UINT;              // 1-component, 8-bit unsigned integer
			case GL_RG8UI: return VK_FORMAT_R8G8_UINT;             // 2-component, 8-bit unsigned integer
			case GL_RGB8UI: return VK_FORMAT_R8G8B8_UINT;            // 3-component, 8-bit unsigned integer
			case GL_RGBA8UI: return VK_FORMAT_R8G8B8A8_UINT;           // 4-component, 8-bit unsigned integer

			case GL_R8I: return VK_FORMAT_R8_SINT;               // 1-component, 8-bit signed integer
			case GL_RG8I: return VK_FORMAT_R8G8_SINT;              // 2-component, 8-bit signed integer
			case GL_RGB8I: return VK_FORMAT_R8G8B8_SINT;             // 3-component, 8-bit signed integer
			case GL_RGBA8I: return VK_FORMAT_R8G8B8A8_SINT;            // 4-component, 8-bit signed integer

			case GL_SR8: return VK_FORMAT_R8_SRGB;               // 1-component, 8-bit sRGB
			case GL_SRG8: return VK_FORMAT_R8G8_SRGB;              // 2-component, 8-bit sRGB
			case GL_SRGB8: return VK_FORMAT_R8G8B8_SRGB;             // 3-component, 8-bit sRGB
			case GL_SRGB8_ALPHA8: return VK_FORMAT_R8G8B8A8_SRGB;      // 4-component, 8-bit sRGB

			//
			// 16 bits per component
			//
			case GL_R16: return VK_FORMAT_R16_UNORM;               // 1-component, 16-bit unsigned normalized
			case GL_RG16: return VK_FORMAT_R16G16_UNORM;              // 2-component, 16-bit unsigned normalized
			case GL_RGB16: return VK_FORMAT_R16G16B16_UNORM;             // 3-component, 16-bit unsigned normalized
			case GL_RGBA16: return VK_FORMAT_R16G16B16A16_UNORM;            // 4-component, 16-bit unsigned normalized

			case GL_R16_SNORM: return VK_FORMAT_R16_SNORM;         // 1-component, 16-bit signed normalized
			case GL_RG16_SNORM: return VK_FORMAT_R16G16_SNORM;        // 2-component, 16-bit signed normalized
			case GL_RGB16_SNORM: return VK_FORMAT_R16G16B16_SNORM;       // 3-component, 16-bit signed normalized
			case GL_RGBA16_SNORM: return VK_FORMAT_R16G16B16A16_SNORM;      // 4-component, 16-bit signed normalized

			case GL_R16UI: return VK_FORMAT_R16_UINT;             // 1-component, 16-bit unsigned integer
			case GL_RG16UI: return VK_FORMAT_R16G16_UINT;            // 2-component, 16-bit unsigned integer
			case GL_RGB16UI: return VK_FORMAT_R16G16B16_UINT;           // 3-component, 16-bit unsigned integer
			case GL_RGBA16UI: return VK_FORMAT_R16G16B16A16_UINT;          // 4-component, 16-bit unsigned integer

			case GL_R16I: return VK_FORMAT_R16_SINT;              // 1-component, 16-bit signed integer
			case GL_RG16I: return VK_FORMAT_R16G16_SINT;             // 2-component, 16-bit signed integer
			case GL_RGB16I: return VK_FORMAT_R16G16B16_SINT;            // 3-component, 16-bit signed integer
			case GL_RGBA16I: return VK_FORMAT_R16G16B16A16_SINT;           // 4-component, 16-bit signed integer

			case GL_R16F: return VK_FORMAT_R16_SFLOAT;              // 1-component, 16-bit floating-point
			case GL_RG16F: return VK_FORMAT_R16G16_SFLOAT;             // 2-component, 16-bit floating-point
			case GL_RGB16F: return VK_FORMAT_R16G16B16_SFLOAT;            // 3-component, 16-bit floating-point
			case GL_RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;           // 4-component, 16-bit floating-point

			//
			// 32 bits per component
			//
			case GL_R32UI: return VK_FORMAT_R32_UINT;             // 1-component, 32-bit unsigned integer
			case GL_RG32UI: return VK_FORMAT_R32G32_UINT;            // 2-component, 32-bit unsigned integer
			case GL_RGB32UI: return VK_FORMAT_R32G32B32_UINT;           // 3-component, 32-bit unsigned integer
			case GL_RGBA32UI: return VK_FORMAT_R32G32B32A32_UINT;          // 4-component, 32-bit unsigned integer

			case GL_R32I: return VK_FORMAT_R32_SINT;              // 1-component, 32-bit signed integer
			case GL_RG32I: return VK_FORMAT_R32G32_SINT;             // 2-component, 32-bit signed integer
			case GL_RGB32I: return VK_FORMAT_R32G32B32_SINT;            // 3-component, 32-bit signed integer
			case GL_RGBA32I: return VK_FORMAT_R32G32B32A32_SINT;           // 4-component, 32-bit signed integer

			case GL_R32F: return VK_FORMAT_R32_SFLOAT;              // 1-component, 32-bit floating-point
			case GL_RG32F: return VK_FORMAT_R32G32_SFLOAT;             // 2-component, 32-bit floating-point
			case GL_RGB32F: return VK_FORMAT_R32G32B32_SFLOAT;            // 3-component, 32-bit floating-point
			case GL_RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;           // 4-component, 32-bit floating-point

			//
			// Packed
			//
			case GL_RGB5: return VK_FORMAT_R5G5B5A1_UNORM_PACK16;              // 3-component 5:5:5,       unsigned normalized
			case GL_RGB565: return VK_FORMAT_R5G6B5_UNORM_PACK16;            // 3-component 5:6:5,       unsigned normalized
			case GL_RGBA4: return VK_FORMAT_R4G4B4A4_UNORM_PACK16;             // 4-component 4:4:4:4,     unsigned normalized
			case GL_RGB5_A1: return VK_FORMAT_A1R5G5B5_UNORM_PACK16;           // 4-component 5:5:5:1,     unsigned normalized
			case GL_RGB10_A2: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;          // 4-component 10:10:10:2,  unsigned normalized
			case GL_RGB10_A2UI: return VK_FORMAT_A2R10G10B10_UINT_PACK32;        // 4-component 10:10:10:2,  unsigned integer
			case GL_R11F_G11F_B10F: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;    // 3-component 11:11:10,    floating-point
			case GL_RGB9_E5: return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;           // 3-component/exp 9:9:9/5, floating-point

			//
			// S3TC/DXT/BC
			//
			case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: return VK_FORMAT_BC1_RGB_UNORM_BLOCK;                  // line through 3D space, 4x4 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;                 // line through 3D space plus 1-bit alpha, 4x4 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: return VK_FORMAT_BC2_UNORM_BLOCK;                 // line through 3D space plus line through 1D space, 4x4 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: return VK_FORMAT_BC3_UNORM_BLOCK;                 // line through 3D space plus 4-bit alpha, 4x4 blocks, unsigned normalized

			case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT: return VK_FORMAT_BC1_RGB_SRGB_BLOCK;                 // line through 3D space, 4x4 blocks, sRGB
			case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;           // line through 3D space plus 1-bit alpha, 4x4 blocks, sRGB
			case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: return VK_FORMAT_BC2_SRGB_BLOCK;           // line through 3D space plus line through 1D space, 4x4 blocks, sRGB
			case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return VK_FORMAT_BC3_SRGB_BLOCK;           // line through 3D space plus 4-bit alpha, 4x4 blocks, sRGB

			case GL_COMPRESSED_RED_RGTC1: return VK_FORMAT_BC4_UNORM_BLOCK;                          // line through 1D space, 4x4 blocks, unsigned normalized
			case GL_COMPRESSED_RG_RGTC2: return VK_FORMAT_BC5_UNORM_BLOCK;                           // two lines through 1D space, 4x4 blocks, unsigned normalized
			case GL_COMPRESSED_SIGNED_RED_RGTC1: return VK_FORMAT_BC4_SNORM_BLOCK;                   // line through 1D space, 4x4 blocks, signed normalized
			case GL_COMPRESSED_SIGNED_RG_RGTC2: return VK_FORMAT_BC5_SNORM_BLOCK;                    // two lines through 1D space, 4x4 blocks, signed normalized

			case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT: return VK_FORMAT_BC6H_UFLOAT_BLOCK;            // 3-component, 4x4 blocks, unsigned floating-point
			case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT: return VK_FORMAT_BC6H_SFLOAT_BLOCK;              // 3-component, 4x4 blocks, signed floating-point
			case GL_COMPRESSED_RGBA_BPTC_UNORM: return VK_FORMAT_BC7_UNORM_BLOCK;                    // 4-component, 4x4 blocks, unsigned normalized
			case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM: return VK_FORMAT_BC7_SRGB_BLOCK;              // 4-component, 4x4 blocks, sRGB

			//
			// ETC
			//
			case GL_COMPRESSED_RGB8_ETC2: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;                          // 3-component ETC2, 4x4 blocks, unsigned normalized
			case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2: return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;      // 4-component ETC2 with 1-bit alpha, 4x4 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA8_ETC2_EAC: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;                     // 4-component ETC2, 4x4 blocks, unsigned normalized

			case GL_COMPRESSED_SRGB8_ETC2: return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;                         // 3-component ETC2, 4x4 blocks, sRGB
			case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2: return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;     // 4-component ETC2 with 1-bit alpha, 4x4 blocks, sRGB
			case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC: return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;              // 4-component ETC2, 4x4 blocks, sRGB

			case GL_COMPRESSED_R11_EAC: return VK_FORMAT_EAC_R11_UNORM_BLOCK;                            // 1-component ETC, 4x4 blocks, unsigned normalized
			case GL_COMPRESSED_RG11_EAC: return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;                           // 2-component ETC, 4x4 blocks, unsigned normalized
			case GL_COMPRESSED_SIGNED_R11_EAC: return VK_FORMAT_EAC_R11_SNORM_BLOCK;                     // 1-component ETC, 4x4 blocks, signed normalized
			case GL_COMPRESSED_SIGNED_RG11_EAC: return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;                    // 2-component ETC, 4x4 blocks, signed normalized

			//
			// PVRTC
			//
			case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG: return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;           // 3- or 4-component PVRTC, 16x8 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG: return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;           // 3- or 4-component PVRTC,  8x8 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG: return VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG;           // 3- or 4-component PVRTC, 16x8 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG: return VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG;           // 3- or 4-component PVRTC,  4x4 blocks, unsigned normalized

			case GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT: return VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG;     // 4-component PVRTC, 16x8 blocks, sRGB
			case GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT: return VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG;     // 4-component PVRTC,  8x8 blocks, sRGB
			case GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG: return VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG;     // 4-component PVRTC,  8x4 blocks, sRGB
			case GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG: return VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG;     // 4-component PVRTC,  4x4 blocks, sRGB

			//
			// ASTC
			//
			case GL_COMPRESSED_RGBA_ASTC_4x4_KHR: return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;                // 4-component ASTC, 4x4 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_ASTC_5x4_KHR: return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;                // 4-component ASTC, 5x4 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_ASTC_5x5_KHR: return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;                // 4-component ASTC, 5x5 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_ASTC_6x5_KHR: return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;                // 4-component ASTC, 6x5 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_ASTC_6x6_KHR: return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;                // 4-component ASTC, 6x6 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_ASTC_8x5_KHR: return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;                // 4-component ASTC, 8x5 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_ASTC_8x6_KHR: return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;                // 4-component ASTC, 8x6 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_ASTC_8x8_KHR: return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;                // 4-component ASTC, 8x8 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_ASTC_10x5_KHR: return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;               // 4-component ASTC, 10x5 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_ASTC_10x6_KHR: return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;               // 4-component ASTC, 10x6 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_ASTC_10x8_KHR: return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;               // 4-component ASTC, 10x8 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_ASTC_10x10_KHR: return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;              // 4-component ASTC, 10x10 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_ASTC_12x10_KHR: return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;              // 4-component ASTC, 12x10 blocks, unsigned normalized
			case GL_COMPRESSED_RGBA_ASTC_12x12_KHR: return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;              // 4-component ASTC, 12x12 blocks, unsigned normalized

			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR: return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;        // 4-component ASTC, 4x4 blocks, sRGB
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR: return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;        // 4-component ASTC, 5x4 blocks, sRGB
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR: return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;        // 4-component ASTC, 5x5 blocks, sRGB
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR: return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;        // 4-component ASTC, 6x5 blocks, sRGB
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR: return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;        // 4-component ASTC, 6x6 blocks, sRGB
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR: return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;        // 4-component ASTC, 8x5 blocks, sRGB
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR: return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;        // 4-component ASTC, 8x6 blocks, sRGB
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR: return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;        // 4-component ASTC, 8x8 blocks, sRGB
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR: return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;       // 4-component ASTC, 10x5 blocks, sRGB
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR: return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;       // 4-component ASTC, 10x6 blocks, sRGB
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR: return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;       // 4-component ASTC, 10x8 blocks, sRGB
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR: return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;      // 4-component ASTC, 10x10 blocks, sRGB
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR: return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;      // 4-component ASTC, 12x10 blocks, sRGB
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR: return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;      // 4-component ASTC, 12x12 blocks, sRGB

			//
			// Depth/stencil
			//
			case GL_DEPTH_COMPONENT16: return VK_FORMAT_D16_UNORM;
			case GL_DEPTH_COMPONENT24: return VK_FORMAT_X8_D24_UNORM_PACK32;
			case GL_DEPTH_COMPONENT32F: return VK_FORMAT_D32_SFLOAT;
			case GL_STENCIL_INDEX8: return VK_FORMAT_S8_UINT;
			case GL_DEPTH24_STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
			case GL_DEPTH32F_STENCIL8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
			default: return VK_FORMAT_UNDEFINED;
			}
	}

		bool compresTextureKTX(std::ostream& fout,int s, int t ,const uint8_t* rgb, int internalTextureFormat,int totalSizeInBytes)
		{
 			if(getFlag() )
				return true;

			  ktxTexture* texture = NULL;

        ktxTextureCreateInfo createInfo;
        createInfo.glInternalformat =internalTextureFormat;
        createInfo.vkFormat = glGetVkFormatFromInternalFormat(createInfo.glInternalformat);
        createInfo.baseWidth =s;
        createInfo.baseHeight =t;
        createInfo.baseDepth =1;
        createInfo.numDimensions =2;
        createInfo.numLevels = 1; 
        createInfo.numLayers =1;
        createInfo.numFaces =1;
        createInfo.isArray = KTX_FALSE;
        createInfo.generateMipmaps = KTX_FALSE;

        if (createInfo.vkFormat == 0) {
            return false;
        }

        KTX_error_code result = ktxTexture2_Create( &createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, (ktxTexture2**)&texture);
        if (result != KTX_SUCCESS) {
             return false;
        }

        ktx_uint8_t* src = (ktx_uint8_t*)rgb;

        result = ktxTexture_SetImageFromMemory( texture, 0,0, 0,    src, totalSizeInBytes);

        if (result != KTX_SUCCESS) {
             ktxTexture_Destroy(texture);
						 return false;
        } 

				bool useUASTC=false;
				int basisuCompressLv =2;
				int basisuQualityLv = 16;
				int basisuThreadCount=2;

        ktxBasisParams params = { 0 };
        params.structSize = sizeof(params);
        params.uastc = useUASTC ? KTX_TRUE : KTX_FALSE;
        params.compressionLevel = basisuCompressLv;
        params.qualityLevel = basisuQualityLv;
        params.threadCount = basisuThreadCount;

        result = ktxTexture2_CompressBasisEx((ktxTexture2*)texture, &params);

				if(result == KTX_SUCCESS)	{
					ktx_uint8_t* buffer = NULL;
					ktx_size_t length = 0;
					KTX_error_code result = ktxTexture_WriteToMemory(texture, &buffer, &length);

					if (result == KTX_SUCCESS)	{
							fout.write((char*)buffer, length);
							delete buffer;
					}

					ktxTexture_Destroy(texture);
					return true;
				}
				return false;
 		}


Settings defaults()
{
	Settings settings = {};
	settings.quantize = false;
	settings.pos_bits = 16;
	settings.tex_bits = 12;
	settings.nrm_bits = 8;
	settings.col_bits = 8;
	settings.trn_bits = 16;
	settings.rot_bits = 12;
	settings.scl_bits = 16;
	settings.anim_freq = 30;
	settings.simplify_threshold = 1.f;
	settings.texture_scale = 1.f;

	for (int kind = 0; kind < TextureKind__Count; ++kind)
		settings.texture_quality[kind] = 8;

	return settings;
}
bool compressGlbWithMeshOpt(std::vector<std::byte>& data , const std::string& file,bool textureRef)
{
 			if(getFlag() )
				return true;

		const char* report = NULL;

		Settings settings = defaults();

		//"-cc"
		settings.compress = true;
		settings.compressmore = false;
		settings.texture_jobs=1;
		settings.texture_ref=textureRef;

		bool bUASTC=false;
		bool ETC1S=false;

			if(bUASTC)
			{
				settings.texture_ktx2 = true;

				unsigned int mask = ~0u;
 
				for (int kind = 0; kind < TextureKind__Count; ++kind)
					if (mask & (1 << kind))
						settings.texture_mode[kind] = TextureMode_UASTC;
			}
			else if(ETC1S)
			{
				settings.texture_ktx2 = true;

				unsigned int mask = ~0u;
 
				for (int kind = 0; kind < TextureKind__Count; ++kind)
					if (mask & (1 << kind))
						settings.texture_mode[kind] = TextureMode_ETC1S;
			}

		int flag =  gltfpackEx( (void*)data.data() , data.size()  , file.c_str() ,   report, settings);
		if(flag == 0)
			return true;
		return false;
}
bool compressGlbWithMeshOptEx(const std::string& inputFIle , const std::string& file, const std::string& strParam)
{
 			if(getFlag() )
				return true;

			const char* report = NULL;
			Settings settings = defaults();
				//"-cc"
			settings.compress = true;
			settings.compressmore = true;

			int flag =  gltfpack(inputFIle.c_str(), file.c_str() ,   report, settings);
			return true;
}


using namespace CesiumGltfReader;
using namespace CesiumGltf;

	struct NodeBuilder
	{
 		CesiumGltf::Model *model;
 		std::vector< osg::ref_ptr< osg::Array > > arrays;
		std::string textureDir;
		NodeBuilder( CesiumGltf::Model *model_):	model(model_)
		{
			extractArrays(arrays);
		}

		osg::Node* createNode(const std::map<std::string, std::vector<std::string>>& objPropertys, const CesiumGltf::Node& node) const
		{
			osg::MatrixTransform* mt = new osg::MatrixTransform;
			if (node.matrix.size() == 16)
			{
				osg::Matrixd mat;
				mat.set(node.matrix.data());
				mt->setMatrix(mat);
			}
			else
			{
				osg::Matrixd scale, translation, rotation;
				if (node.scale.size() == 3)
				{
					scale = osg::Matrixd::scale(node.scale[0], node.scale[1], node.scale[2]);
				}

				if (node.rotation.size() == 4) {
					osg::Quat quat(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
					rotation.makeRotate(quat);
				}

				if (node.translation.size() == 3) {
					translation = osg::Matrixd::translate(node.translation[0], node.translation[1], node.translation[2]);
				}

				mt->setMatrix(scale * rotation * translation);
			}


 			if (node.mesh >= 0)
			{
				osg::Group* meshNode = nullptr;
	/*			if (node.extensions.find("EXT_mesh_gpu_instancing") != node.extensions.end())
				{
					meshNode = makeMesh(model.meshes[node.mesh], true);
					makeInstancedMeshNode(node, meshNode);
				}
				else*/
				{
					meshNode = makeMesh(objPropertys, model->meshes[node.mesh], false);
				}
				mt->addChild(meshNode);
			}

			// Load any children.
			for (unsigned int i = 0; i < node.children.size(); i++)
			{
				osg::Node* child = createNode(objPropertys,model->nodes[node.children[i]]);
				if (child)
				{
					mt->addChild(child);
				}
			}

			osg::Node* top = mt;

			// If we have an OWT_state extension setup all the state names
			if (node.extensions.find("OWT_state") != node.extensions.end())
			{
				//StateTransitionNode* st = new StateTransitionNode;
				//st->addChild(mt);

				//auto ext = node.extensions.find("OWT_state")->second;
				//for (auto& key : ext.Keys())
				//{
				//	std::string value = ext.Get(key).Get<std::string>();
				//	st->_stateToNodeName[key] = value;
				//}
				//top = st;
			}

			top->setName(node.name);

			return top;
		}

		osg::Texture2D* makeTextureFromModel(const std::string& textDir, const CesiumGltf::Texture& texture) const
		{
			if(texture.source < 0)
				return nullptr;

			
			const CesiumGltf::Image& image = model->images[texture.source];

			std::string strImageURI;
			if(image.uri)
				strImageURI= image.uri.value();

  

 
			osg::ref_ptr<osg::Texture2D> tex;


			bool bTest=false;

 			osg::ref_ptr<osg::Image> img;

			if (image.cesium.pixelData.size() > 0)
			{
				GLenum format = GL_RGB, texFormat = GL_RGB8;

				if (image.cesium.channels == 4)
				{
					format = GL_RGBA, texFormat = GL_RGBA8;
				}

				img = new osg::Image();
				//std::cout << "Loading image of size " << image.width << "x" << image.height << " components = " << image.component << " totalSize=" << image.image.size() << std::endl;
	
				unsigned char *imgData = new unsigned char[image.cesium.pixelData.size()];
				memcpy(imgData, &image.cesium.pixelData[0], image.cesium.pixelData.size());
				img->setImage(image.cesium.width, image.cesium.height, 1, texFormat, format, GL_UNSIGNED_BYTE, imgData, osg::Image::AllocationMode::USE_NEW_DELETE);


				//add cg ,为什么需要？？

				if(bTest)
					osgDB::writeImageFile(*img,"I:/temp/test.png");
			}
			else if(!strImageURI.empty()  )
			{
				img  = osgDB::readImageFile(textureDir + strImageURI);
				//img->flipVertical();
			} 

			// If the image loaded OK, create the texture
			if (img.valid())
			{
				if (img->getPixelFormat() == GL_RGB)
					img->setInternalTextureFormat(GL_RGB8);
				else if (img->getPixelFormat() == GL_RGBA)
					img->setInternalTextureFormat(GL_RGBA8);

				tex = new osg::Texture2D(img.get());
				//tex->setUnRefImageDataAfterApply(imageEmbedded);
				tex->setResizeNonPowerOfTwoHint(false);
				tex->setDataVariance(osg::Object::STATIC);

#ifdef COMPRESS_TEXTURE
				tex->setInternalFormatMode(osg::Texture2D::USE_S3TC_DXT1_COMPRESSION);
				tex->setUnRefImageDataAfterApply(true);
#endif

				if (texture.sampler >= 0 && texture.sampler < model->samplers.size())
				{
					const CesiumGltf::Sampler& sampler = model->samplers[texture.sampler];
					tex->setFilter(osg::Texture::MIN_FILTER, (osg::Texture::FilterMode)osg::Texture::LINEAR_MIPMAP_LINEAR); //sampler.minFilter);
					tex->setFilter(osg::Texture::MAG_FILTER, (osg::Texture::FilterMode)osg::Texture::LINEAR); //sampler.magFilter);
					tex->setWrap(osg::Texture::WRAP_S, (osg::Texture::WrapMode)sampler.wrapS);
					tex->setWrap(osg::Texture::WRAP_T, (osg::Texture::WrapMode)sampler.wrapT);
 				}
				else
				{
					tex->setFilter(osg::Texture::MIN_FILTER, (osg::Texture::FilterMode)osg::Texture::LINEAR_MIPMAP_LINEAR);
					tex->setFilter(osg::Texture::MAG_FILTER, (osg::Texture::FilterMode)osg::Texture::LINEAR);
					tex->setWrap(osg::Texture::WRAP_S, (osg::Texture::WrapMode)osg::Texture::CLAMP_TO_EDGE);
					tex->setWrap(osg::Texture::WRAP_T, (osg::Texture::WrapMode)osg::Texture::CLAMP_TO_EDGE);
				}
			}
			return tex.release();

			return nullptr;
		}

		osg::Group* makeMesh(const std::map<std::string, std::vector<std::string>>& objPropertys, const CesiumGltf::Mesh& mesh, bool prepInstancing) const
		{
			osg::Geode *group = new osg::Geode;
			group->setUserValue("type", std::string("3dtiles"));

			std::cout << "Drawing " << mesh.primitives.size() << " primitives in mesh" << std::endl;

			for (size_t i = 0; i < mesh.primitives.size(); i++) {

				std::cout << " Processing primitive " << i << std::endl;
				const CesiumGltf::MeshPrimitive &primitive = mesh.primitives[i];
				//if (primitive.indices < 0)
				//{
				//	return 0;
				//}

				osg::ref_ptr< osg::Geometry > geom;
	/*			if (prepInstancing)
				{
					geom = osgEarth::InstanceBuilder::createGeometry();
				}
				else*/
				{
					geom = new osg::Geometry;
					geom->setUserValue("type", std::string("3dtiles"));
				}
				geom->setName(typeid(*this).name());
				geom->setUseDisplayList(false);
				geom->setUseVertexBufferObjects(true);

				group->addChild(geom.get());

				// The base color factor of the material
				osg::Vec4 baseColorFactor(1.0f, 1.0f, 1.0f, 1.0f);

				int textureID = -1;
				if (primitive.material >= 0 && primitive.material < model->materials.size())
				{
					const CesiumGltf::Material& material = model->materials[primitive.material];	 


					if(material.pbrMetallicRoughness)
					{			  
							std::vector<double> color = material.pbrMetallicRoughness->baseColorFactor;  
							baseColorFactor = osg::Vec4(color[0], color[1], color[2], color[3]);

							//
							if(material.pbrMetallicRoughness->baseColorTexture)
							{
								textureID = material.pbrMetallicRoughness->baseColorTexture->index;				
							}
							if (material.doubleSided)
							{
								geom->setUserValue("doubleSided", std::string("1"));
							}
					}	

		 
					osg::FloatArray* pTexFlat=new osg::FloatArray;
					if (textureID != -1 && !model->textures.empty())
					{
						const CesiumGltf::Texture& texture = model->textures[textureID];
						const CesiumGltf::Image& image = model->images[texture.source];

						osg::Texture2D* tex= makeTextureFromModel(textureDir,texture);
						geom->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex);

						pTexFlat->push_back(1.0);
					}
					else {
						pTexFlat->push_back(0.0);
					}

					geom->setVertexAttribArray(6, pTexFlat, osg::Array::BIND_OVERALL);
				}

 
				for (auto it : primitive.attributes )
				{
					const CesiumGltf::Accessor &accessor = model->accessors[it.second];

					if (it.first.compare("POSITION") == 0)
					{
						geom->setVertexArray(arrays[it.second].get());
					}
					else if (it.first.compare("NORMAL") == 0)
					{
						geom->setNormalArray(arrays[it.second].get(), osg::Array::BIND_PER_VERTEX);
					}
					else if (it.first.compare("TEXCOORD_0") == 0)
					{
						geom->setTexCoordArray(0, arrays[it.second].get());
					}
					else if (it.first.compare("TEXCOORD_1") == 0)
					{
						geom->setTexCoordArray(1, arrays[it.second].get());
					}
					else if (it.first.compare("COLOR_0") == 0)
					{
						std::cout << "Setting color array " << arrays[it.second].get() << std::endl;
						geom->setColorArray(arrays[it.second].get());
					}
					else if (it.first.compare("_BATCHID") == 0)
					{ 
						geom->setVertexAttribArray(4, arrays[it.second].get(), osg::Array::BIND_PER_VERTEX);
					}
					else if (it.first.compare("_FEATURE_ID_0") == 0)
					{
						geom->setVertexAttribArray(4, arrays[it.second].get(), osg::Array::BIND_PER_VERTEX);
					}
					else if (it.first.compare("_FEATURE_ID_1") == 0)
					{
						geom->setVertexAttribArray(5, arrays[it.second].get(), osg::Array::BIND_PER_VERTEX);
					}
					else
					{
						std::cout << "Skipping array " << it.first << std::endl;
					}
				}


				// If there is no color array just add one that has the base color factor in it.
				if (!geom->getColorArray() && textureID<0 )
				{
					osg::Vec4Array* colors = new osg::Vec4Array();
					osg::Vec3Array* verts = static_cast<osg::Vec3Array*>(geom->getVertexArray());
					for (unsigned int i = 0; i < verts->size(); i++)
					{
						colors->push_back(baseColorFactor);
					}
					geom->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
				}

				int mode = -1;
				if (primitive.mode == TINYGLTF_MODE_TRIANGLES) {
					mode = GL_TRIANGLES;
				}
				else if (primitive.mode == TINYGLTF_MODE_TRIANGLE_STRIP) {
					mode = GL_TRIANGLE_STRIP;
				}
				else if (primitive.mode == TINYGLTF_MODE_TRIANGLE_FAN) {
					mode = GL_TRIANGLE_FAN;
				}
				else if (primitive.mode == TINYGLTF_MODE_POINTS) {
					mode = GL_POINTS;
				}
				else if (primitive.mode == TINYGLTF_MODE_LINE) {
					mode = GL_LINES;
				}
				else if (primitive.mode == TINYGLTF_MODE_LINE_LOOP) {
					mode = GL_LINE_LOOP;
				}

				if (primitive.indices < 0)
				{
					osg::Array* vertices = geom->getVertexArray();
					if (vertices)
					{
						osg::DrawArrays *drawArrays
							= new osg::DrawArrays(mode, 0, vertices->getNumElements());
						geom->addPrimitiveSet(drawArrays);
					}
					// Otherwise we can't draw anything!
				}
				else
				{
					const CesiumGltf::Accessor &indexAccessor = model->accessors[primitive.indices];

					if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						osg::UShortArray* indices = static_cast<osg::UShortArray*>(arrays[primitive.indices].get());
						osg::DrawElementsUShort* drawElements
							= new osg::DrawElementsUShort(mode, indices->begin(), indices->end());
						geom->addPrimitiveSet(drawElements);
					}
					else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
					{
						osg::UIntArray* indices = static_cast<osg::UIntArray*>(arrays[primitive.indices].get());
						osg::DrawElementsUInt* drawElements
							= new osg::DrawElementsUInt(mode, indices->begin(), indices->end());
						geom->addPrimitiveSet(drawElements);
					}
					else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						osg::UByteArray* indices = static_cast<osg::UByteArray*>(arrays[primitive.indices].get());
						// Sigh, DrawElementsUByte doesn't have the constructor with iterator arguments.
						osg::DrawElementsUByte* drawElements = new osg::DrawElementsUByte(mode, indexAccessor.count);
						std::copy(indices->begin(), indices->end(), drawElements->begin());
						geom->addPrimitiveSet(drawElements);
					}
					else
					{
						std::cout <<  "primitive indices are not unsigned.\n";
					}
				}	 


				if(1)
				{
					geom->setVertexAttribArray(0, geom->getVertexArray(),    osg::Array::BIND_PER_VERTEX);
					geom->setVertexAttribArray(1, geom->getNormalArray(),    osg::Array::BIND_PER_VERTEX);
					geom->setVertexAttribArray(2, geom->getColorArray(),     osg::Array::BIND_PER_VERTEX);
					geom->setVertexAttribArray(3, geom->getTexCoordArray(0), osg::Array::BIND_PER_VERTEX);
				}

				osg::ref_ptr< osg::KdTreeBuilder > kdTreeBuilder = new osg::KdTreeBuilder();
				group->accept(*kdTreeBuilder.get());

			}

			return group;
		}

 
		template<typename OSGArray, int ComponentType, int AccessorType>
		class ArrayBuilder
		{
		public:
			static OSGArray* makeArray(unsigned int size)
			{
				return new OSGArray(size);
			}
			static void copyData(OSGArray* dest, const unsigned char* src, size_t viewOffset,size_t byteStride, size_t accessorOffset, size_t count)
			{
				int32_t componentSize    = tinygltf::GetComponentSizeInBytes(ComponentType);
				int32_t numComponents = tinygltf::GetNumComponentsInType(AccessorType);
				if (byteStride == 0)
				{
					memcpy(&(*dest)[0], src + accessorOffset + viewOffset, componentSize * numComponents * count);
				}
				else
				{
					const unsigned char* ptr = src + accessorOffset + viewOffset;
					for (int i = 0; i < count; ++i, ptr += byteStride)
					{
						memcpy(&(*dest)[i], ptr, componentSize * numComponents);
					}
				}
			}
			static void copyData(OSGArray* dest, const CesiumGltf::Buffer& buffer, const CesiumGltf::BufferView& bufferView,const CesiumGltf::Accessor& accessor)
			{
				size_t byteStride=0;
				if(bufferView.byteStride)
					byteStride=bufferView.byteStride.value();
			
				copyData(dest, reinterpret_cast<const unsigned char *>(buffer.cesium.data.data() ), bufferView.byteOffset,byteStride, accessor.byteOffset, accessor.count);
			}
			static OSGArray* makeArray(const CesiumGltf::Buffer& buffer, const CesiumGltf::BufferView& bufferView,	const CesiumGltf::Accessor& accessor)
			{
				OSGArray* result = new OSGArray(accessor.count);
				copyData(result, buffer, bufferView, accessor);
				return result;
			}
		};

		// Take all of the accessors and turn them into arrays
		void extractArrays(std::vector<osg::ref_ptr<osg::Array>> &arrays) const
		{
			for (unsigned int i = 0; i < model->accessors.size(); i++)
			{
				const CesiumGltf::Accessor& accessor = model->accessors[i];
				const CesiumGltf::BufferView& bufferView = model->bufferViews[accessor.bufferView];
				const CesiumGltf::Buffer& buffer = model->buffers[bufferView.buffer];
				osg::ref_ptr< osg::Array > osgArray;

				int accessorType = TINYGLTF_TYPE_SCALAR;
				if(accessor.type == Accessor::Type::VEC2)
				{
					accessorType=TINYGLTF_TYPE_VEC2;
				}
				else if( accessor.type == Accessor::Type::VEC3)
				{
					accessorType=TINYGLTF_TYPE_VEC3;
				}
				else if( accessor.type == Accessor::Type::VEC4)
				{
					accessorType=TINYGLTF_TYPE_VEC4;
				}

				switch (accessor.componentType)
				{
				case Accessor::ComponentType::BYTE:
					switch (accessorType)
					{
					case TINYGLTF_TYPE_SCALAR:
						osgArray = ArrayBuilder<osg::ByteArray,
							TINYGLTF_COMPONENT_TYPE_BYTE,
							TINYGLTF_TYPE_SCALAR>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC2:
						osgArray = ArrayBuilder<osg::Vec2bArray,
							TINYGLTF_COMPONENT_TYPE_BYTE,
							TINYGLTF_TYPE_VEC2>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC3:
						osgArray = ArrayBuilder<osg::Vec3bArray,
							TINYGLTF_COMPONENT_TYPE_BYTE,
							TINYGLTF_TYPE_VEC3>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC4:
						osgArray = ArrayBuilder<osg::Vec4bArray,
							TINYGLTF_COMPONENT_TYPE_BYTE,
							TINYGLTF_TYPE_VEC4>::makeArray(buffer, bufferView, accessor);
						break;
					default:
						break;
					}
					break;
				case Accessor::ComponentType::UNSIGNED_BYTE:
					switch (accessorType)
					{
					case TINYGLTF_TYPE_SCALAR:
						osgArray = ArrayBuilder<osg::UByteArray,
							TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,
							TINYGLTF_TYPE_SCALAR>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC2:
						osgArray = ArrayBuilder<osg::Vec2ubArray,
							TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,
							TINYGLTF_TYPE_VEC2>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC3:
						osgArray = ArrayBuilder<osg::Vec3ubArray,
							TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,
							TINYGLTF_TYPE_VEC3>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC4:
						osgArray = ArrayBuilder<osg::Vec4ubArray,
							TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,
							TINYGLTF_TYPE_VEC4>::makeArray(buffer, bufferView, accessor);
						break;
					default:
						break;
					}
					break;
				case Accessor::ComponentType::SHORT:
					switch (accessorType)
					{
					case TINYGLTF_TYPE_SCALAR:
						osgArray = ArrayBuilder<osg::ShortArray,
							TINYGLTF_COMPONENT_TYPE_SHORT,
							TINYGLTF_TYPE_SCALAR>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC2:
						osgArray = ArrayBuilder<osg::Vec2sArray,
							TINYGLTF_COMPONENT_TYPE_SHORT,
							TINYGLTF_TYPE_VEC2>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC3:
						osgArray = ArrayBuilder<osg::Vec3sArray,
							TINYGLTF_COMPONENT_TYPE_SHORT,
							TINYGLTF_TYPE_VEC3>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC4:
						osgArray = ArrayBuilder<osg::Vec4sArray,
							TINYGLTF_COMPONENT_TYPE_SHORT,
							TINYGLTF_TYPE_VEC4>::makeArray(buffer, bufferView, accessor);
						break;
					default:
						break;
					}
					break;
				case Accessor::ComponentType::UNSIGNED_SHORT:
					switch (accessorType)
					{
					case TINYGLTF_TYPE_SCALAR:
						osgArray = ArrayBuilder<osg::UShortArray,
							TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
							TINYGLTF_TYPE_SCALAR>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC2:
						osgArray = ArrayBuilder<osg::Vec2usArray,
							TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
							TINYGLTF_TYPE_VEC2>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC3:
						osgArray = ArrayBuilder<osg::Vec3usArray,
							TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
							TINYGLTF_TYPE_VEC3>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC4:
						osgArray = ArrayBuilder<osg::Vec4usArray,
							TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
							TINYGLTF_TYPE_VEC4>::makeArray(buffer, bufferView, accessor);
						break;
					default:
						break;
					}
					break;
				case TINYGLTF_COMPONENT_TYPE_INT:
					switch (accessorType)
					{
					case TINYGLTF_TYPE_SCALAR:
						osgArray = ArrayBuilder<osg::IntArray,
							TINYGLTF_COMPONENT_TYPE_INT,
							TINYGLTF_TYPE_SCALAR>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC2:
						osgArray = ArrayBuilder<osg::Vec2uiArray,
							TINYGLTF_COMPONENT_TYPE_INT,
							TINYGLTF_TYPE_VEC2>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC3:
						osgArray = ArrayBuilder<osg::Vec3uiArray,
							TINYGLTF_COMPONENT_TYPE_INT,
							TINYGLTF_TYPE_VEC3>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC4:
						osgArray = ArrayBuilder<osg::Vec4uiArray,
							TINYGLTF_COMPONENT_TYPE_INT,
							TINYGLTF_TYPE_VEC4>::makeArray(buffer, bufferView, accessor);
						break;
					default:
						break;
					}
					break;
				case Accessor::ComponentType::UNSIGNED_INT:
					switch (accessorType)
					{
					case TINYGLTF_TYPE_SCALAR:
						osgArray = ArrayBuilder<osg::UIntArray,
							TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,
							TINYGLTF_TYPE_SCALAR>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC2:
						osgArray = ArrayBuilder<osg::Vec2iArray,
							TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,
							TINYGLTF_TYPE_VEC2>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC3:
						osgArray = ArrayBuilder<osg::Vec3iArray,
							TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,
							TINYGLTF_TYPE_VEC3>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC4:
						osgArray = ArrayBuilder<osg::Vec4iArray,
							TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,
							TINYGLTF_TYPE_VEC4>::makeArray(buffer, bufferView, accessor);
						break;
					default:
						break;
					}
					break;
				case Accessor::ComponentType::FLOAT:
					switch (accessorType)
					{
					case TINYGLTF_TYPE_SCALAR:
						osgArray = ArrayBuilder<osg::FloatArray,
							TINYGLTF_COMPONENT_TYPE_FLOAT,
							TINYGLTF_TYPE_SCALAR>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC2:
						osgArray = ArrayBuilder<osg::Vec2Array,
							TINYGLTF_COMPONENT_TYPE_FLOAT,
							TINYGLTF_TYPE_VEC2>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC3:
						osgArray = ArrayBuilder<osg::Vec3Array,
							TINYGLTF_COMPONENT_TYPE_FLOAT,
							TINYGLTF_TYPE_VEC3>::makeArray(buffer, bufferView, accessor);
						break;
					case TINYGLTF_TYPE_VEC4:
						osgArray = ArrayBuilder<osg::Vec4Array,
							TINYGLTF_COMPONENT_TYPE_FLOAT,
							TINYGLTF_TYPE_VEC4>::makeArray(buffer, bufferView, accessor);
						break;
					default:
						break;
					}
				default:
					break;
				}
				if (osgArray.valid())
				{
					osgArray->setBinding(osg::Array::BIND_PER_VERTEX);
					osgArray->setNormalize(accessor.normalized);
				}
				else
				{
					OSG_DEBUG << "Adding null array for " << i << std::endl;
				}
				arrays.push_back(osgArray);
			}
		}

 

		void makeInstancedMeshNode(const CesiumGltf::Node& node, osg::Group* meshGroup) const
		{
 
		}
	};



	osg::Group* cesiumLoadGltf(std::string& data,const std::string& textureDir)
	{
 			if(getFlag() )
				return nullptr;

		bool useCesium=true;
		if(useCesium)
		{
			GltfReaderOptions options;
			GltfReader reader;
			GltfReaderResult result = reader.readGltf(  gsl::span(reinterpret_cast<const std::byte*>(data.c_str()), data.size()),   options);
			if(result.errors.size() == 0 )
			{
				NodeBuilder builder( &(result.model.value()));
				builder.textureDir=textureDir;
				osg::Group* pGroup = new osg::Group;

				for(int i =0 ;i<result.model->scenes.size(); i++ )
				{
					CesiumGltf::Scene& scene = result.model->scenes[i];
					std::map<std::string, std::vector<std::string>> objPropertys;

					for (size_t j = 0; j < scene.nodes.size(); j++) 
					{
						osg::Node* node = builder.createNode(objPropertys,  result.model->nodes[scene.nodes[j]]);
						if (node)
						{
							pGroup->addChild( node);
 						}
					}
				}

				return pGroup;
			}
		}

		return nullptr;
	}


	
	struct Ray
	{
		osg::Vec3 origin;
		osg::Vec3 direction;

		inline osg::Vec3 getPos(float t) const { return origin + direction * t; }
	};

	struct TempMesh
	{
		std::vector< osg::Vec3> vertices;
		std::vector< float> batchIDs;
		std::vector< osg::Vec2> uvs;
		std::vector< osg::Vec4> colors;

		std::vector<int> triangles;
		int triangleIndex = 0;
		std::vector<int> vMapping;

 		bool bUV = true;
		bool bColor = true;

		TempMesh()
		{
		}
		~TempMesh()
		{
			Clear();
		}
		void Init(int  vertexCapacity)
		{
			vertices.resize(vertexCapacity);
			batchIDs.resize(vertexCapacity);
			uvs.resize(vertexCapacity);
			triangles.resize(vertexCapacity * 3);
			vMapping.resize(vertexCapacity);
			colors.resize(vertexCapacity);
		}

		void Clear()
		{
			vertices.clear();
			batchIDs.clear();
			uvs.clear();
			triangles.clear();
			colors.clear();

			vMapping.clear();
			triangleIndex = 0;
 		}

		void AddPoint(const osg::Vec3& point, const osg::Vec2& uv, const osg::Vec4 color, int id)
		{	
			triangles.push_back(vertices.size());

			vertices.push_back(point);
			batchIDs.push_back(id);
			if (bUV)
				uvs.push_back(uv);	
			if (bColor)
				colors.push_back(color);
		}	

		void AddSlicedTriangle(int i1, const osg::Vec3& v2, int i3, const osg::Vec2& uv2, const osg::Vec4 color2, int id)
		{
			int v1 = vMapping[i1];
			int v3 = vMapping[i3];
	
			triangles.push_back(v1);

			AddPoint(v2, uv2,color2, id);

			triangles.push_back(v3);
		}

		void AddSlicedTriangle(int i1, const osg::Vec3& v2, const osg::Vec3& v3, const osg::Vec2& uv2,const osg::Vec2& uv3, const osg::Vec4 color2,const osg::Vec4 color3, int id2,int id3)
		{
			// Compute face normal?
			int v1 = vMapping[i1];

			triangles.push_back(v1);

			AddPoint(v2, uv2,color2, id2);
			AddPoint(v3, uv3,color3, id3);
		}

		void AddOgTriangle(const std::vector<int>& indices)
		{
			for (int i = 0; i < 3; ++i)
			{	
				triangles.push_back(vMapping[indices[i]]);
			}
		}

		void AddVertex(const  std::vector<osg::Vec3>& ogVertices, const std::vector<osg::Vec2>& ogUVs, const std::vector<float>& ogIDs, const std::vector<osg::Vec4>& ogColors, int index)
		{
			vMapping[index] = vertices.size();

			vertices.push_back(ogVertices[index]);
			batchIDs.push_back(ogIDs[index]);

			if (bUV)
			{
				uvs.push_back(ogUVs[index]);
			}			
			if (bColor)
			{
				colors.push_back(ogColors[index]);
			}
		}

		void AddTriangle(const std::vector<osg::Vec3>& points , const std::vector<osg::Vec2>& uvs , const std::vector<int>& ids, const std::vector<osg::Vec4>& colors)
		{ 
			for (int i = 0; i < 3; ++i)
			{
				if (bUV)
				{
					if(bColor)
						AddPoint(points[i], uvs[i], colors[i], ids[i]);
					else
						AddPoint(points[i], uvs[i],  osg::Vec4(0,0,0,1) , ids[i]);
				}
				else
				{
					if (bColor)
						AddPoint(points[i], osg::Vec2(0, 0), colors[i], ids[i]);
					else
						AddPoint(points[i], osg::Vec2(0, 0), osg::Vec4(0, 0, 0, 1), ids[i]);
				}
			}
		} 
	};


	class Intersections
	{
	public:

		std::vector<osg::Vec3>  v;
		std::vector<osg::Vec2>  u;
		std::vector<osg::Vec4>  c;
		std::vector<int>  id;

		std::vector<int>t = { 0,0,0 };

		std::array<int, 3> positive = { 0,0,0 };

		Ray edgeRay;

		Intersections()
		{
			v.resize(3);
			u.resize(3);
			id.resize(3);
			c.resize(3);
		}

		bool BoundPlaneIntersect(GMesh mesh, osg::Plane plane)
		{
			return 0 == plane.intersect(mesh.aabb);
		}

		bool RayPlaneIntersection(const Ray& ray, const osg::Plane& plane, double& fLength, bool bSingleSidePlane)
		{
			osg::Vec3 n = plane.getNormal();
			double cosine = n * ray.direction;

			if ((cosine == 0.0) || (bSingleSidePlane && (cosine > 0.0)))
				return false;

			double numer = n * ray.origin + plane[3];
			fLength = -numer / cosine;
			if (fLength < 0.0)
				return false;
			return true;	//intersection occurred	
		}

		std::tuple<osg::Vec3, osg::Vec2,osg::Vec4 > IntersectX(osg::Plane plane, const osg::Vec3& first, const osg::Vec3& second, const osg::Vec2& firstU, const osg::Vec2& secondU,const osg::Vec4& firstC, const osg::Vec4& secondC)
		{
			edgeRay.origin = first;
			edgeRay.direction = second - first;
			edgeRay.direction.normalize();
			double dist;
			float maxDist = (first - second).length();

			if (!RayPlaneIntersection(edgeRay, plane, dist, false))
			{
				int kkk = 0;
			}
			else if (dist > maxDist)
			{
				int kkk = 0;

			}
			auto relativeDist = dist / maxDist;
			osg::Vec3 Item1 = edgeRay.getPos(dist);
			osg::Vec2 item2 = firstU * (1 - relativeDist) + secondU * relativeDist;
			osg::Vec4 item3 = firstC * (1 - relativeDist) + secondC * relativeDist;
			return std::tuple<osg::Vec3, osg::Vec2, osg::Vec4>(Item1, item2,item3);
		}


		bool TrianglePlaneIntersectX(const std::vector<osg::Vec3>&  vertices, 
			const std::vector<float>&  ids,
			const std::vector<osg::Vec2>&  uvs, 
			const std::vector<osg::Vec4>&  colors,
			const std::vector<unsigned int >&  triangles,
			int startIdx, 
			const osg::Plane&  plane, 
			const std::vector<int>&  sideFlags, 
			TempMesh& posMesh, 
			TempMesh& negMesh, 
			std::vector<osg::Vec3>& addedPts, 
			std::vector<osg::Vec2>& addedUVs,
			std::vector<int>& addedIDs	)
		{
			for (int i = 0; i < 3; ++i)
			{
				t[i] = triangles[startIdx + i];
				v[i]  = vertices[t[i]];
				id[i] = ids[t[i]];

				if (!uvs.empty())
				{
					u[i] = uvs[t[i]];
				}
				else
				{
					u[i] = { 0,0 };
				}

				if (!colors.empty())
				{
					c[i] = colors[t[i]];
				}
				else
				{
					c[i] = { 0,0,0,1};
				}
			} 

			positive[0] = sideFlags[t[0]];
			positive[1] = sideFlags[t[1]];
			positive[2] = sideFlags[t[2]];

			if (positive[0] == positive[1] && positive[0] == positive[2])//同一侧
			{
				if (positive[0] >= 0) //left
				{
					negMesh.AddOgTriangle(t);
				}
				if (positive[0] <= 0)
				{
					posMesh.AddOgTriangle(t);
				}
				return false;
			}
			else if ((positive[0] == 0 && positive[1] == positive[2]) || (positive[1] == 0 && positive[0] == positive[2]) || (positive[2] == 0 && positive[0] == positive[1]))//一个在切面，两个在同一侧
			{
				int flag = positive[0] + positive[1] + positive[2];

				if (flag >= 0) //left
				{
					negMesh.AddOgTriangle(t);
				}
				if (flag <= 0) //right
				{
					posMesh.AddOgTriangle(t);
				}
				return false;
			}

			else if ((positive[0] == 0 && positive[1] != 0 && positive[1] == positive[2]) || (positive[1] == 0 && positive[2] != 0 && positive[0] == positive[2]) || (positive[2] == 0 && positive[0] != 0 && positive[0] == positive[1]))////一个在切面，两个在同一侧
			{
				int flag = positive[0] + positive[1] + positive[2];
				if (flag > 0) //left
				{
					negMesh.AddOgTriangle(t);
				}
				else
				{
					posMesh.AddOgTriangle(t);
				}
				return false;
			}

			else if ( (positive[0] == 0 && positive[1] == 0 && positive[2] !=0 ) || (positive[0] == 0 && positive[1] != 0 && positive[2] == 0) ||  (positive[0] != 0 && positive[1] == 0 && positive[2] == 0))////2个在切面，1个在一侧
			{
				int flag = positive[0] + positive[1] + positive[2];
				if (flag > 0) //left
				{
					negMesh.AddOgTriangle(t);
				}
				else
				{
					posMesh.AddOgTriangle(t);
				}
				return false;
			}

			else if ((positive[0] == 0 && positive[1] != positive[2]) || (positive[1] == 0 && positive[0] != positive[2]) || (positive[2] == 0 && positive[0] != positive[1]))//一个在切面。另外两个在不同侧
			{
				std::tuple<osg::Vec3, osg::Vec2,osg::Vec4> newPointPrev;
				if (positive[0] == 0)
				{
					newPointPrev = IntersectX(plane, v[1], v[2] ,u[1],u[2],c[1],c[2] );
					if (positive[1] > 0)
					{
						negMesh.AddSlicedTriangle(t[1], std::get<0>( newPointPrev) , t[0], std::get<1>(newPointPrev) , std::get<2>(newPointPrev) , ids[t[1]] );
						posMesh.AddSlicedTriangle(t[0], std::get<0>( newPointPrev) , t[2], std::get<1>(newPointPrev),  std::get<2>(newPointPrev),  ids[t[2]]);				
					}
					else
					{
						posMesh.AddSlicedTriangle(t[1], std::get<0>(newPointPrev), t[0], std::get<1>(newPointPrev), std::get<2>(newPointPrev), ids[t[1]]);
						negMesh.AddSlicedTriangle(t[0], std::get<0>(newPointPrev), t[2], std::get<1>(newPointPrev), std::get<2>(newPointPrev), ids[t[2]]);
					}
				}
				if (positive[1] == 0)
				{
					newPointPrev = IntersectX(plane, v[0], v[2],u[0],u[2] ,c[0],c[2]);

					if (positive[0] > 0)
					{
						negMesh.AddSlicedTriangle(t[1], std::get<0>(newPointPrev), t[0], std::get<1>(newPointPrev), std::get<2>(newPointPrev), ids[t[0]]);
						posMesh.AddSlicedTriangle(t[2], std::get<0>(newPointPrev), t[1], std::get<1>(newPointPrev), std::get<2>(newPointPrev), ids[t[2]]);
					}
					else
					{
						negMesh.AddSlicedTriangle(t[2], std::get<0>(newPointPrev), t[1], std::get<1>(newPointPrev), std::get<2>(newPointPrev), ids[t[2]]);
						posMesh.AddSlicedTriangle(t[1], std::get<0>(newPointPrev), t[0], std::get<1>(newPointPrev), std::get<2>(newPointPrev), ids[t[0]]);
					}
				}
				if (positive[2] == 0)
				{
					newPointPrev = IntersectX(plane, v[0], v[1],u[0],u[1],c[0],c[1] );
					if (positive[0] > 0)
					{
						negMesh.AddSlicedTriangle(t[0], std::get<0>(newPointPrev), t[2], std::get<1>(newPointPrev), std::get<2>(newPointPrev), ids[t[0]]);
						posMesh.AddSlicedTriangle(t[2], std::get<0>(newPointPrev), t[1], std::get<1>(newPointPrev), std::get<2>(newPointPrev), ids[t[1]]);
					}
					else
					{
						negMesh.AddSlicedTriangle(t[2], std::get<0>(newPointPrev), t[1], std::get<1>(newPointPrev), std::get<2>(newPointPrev), ids[t[1]]);
						posMesh.AddSlicedTriangle(t[0], std::get<0>(newPointPrev), t[2], std::get<1>(newPointPrev), std::get<2>(newPointPrev), ids[t[0]]);
					}
				}
			}
			else
			{
				// Find lonely point
				int lonelyPoint = 0;
				if (positive[0] != positive[1])
					lonelyPoint = positive[0] != positive[2] ? 0 : 1;
				else
					lonelyPoint = 2;

				// Set previous point in relation to front face order
				int prevPoint = lonelyPoint - 1;
				if (prevPoint == -1)
					prevPoint = 2;

				// Set next point in relation to front face order
				int nextPoint = lonelyPoint + 1;
				if (nextPoint == 3)
					nextPoint = 0;


				// Get the 2 intersection points
				std::tuple<osg::Vec3, osg::Vec2,osg::Vec4> newPointPrev  = IntersectX(plane, v[lonelyPoint], v[prevPoint] ,u[lonelyPoint], u[prevPoint],c[lonelyPoint],c[prevPoint] );
				std::tuple<osg::Vec3, osg::Vec2,osg::Vec4> newPointNext = IntersectX(plane, v[lonelyPoint], v[nextPoint], u[lonelyPoint], u[nextPoint],c[lonelyPoint], c[nextPoint]);

				if (positive[lonelyPoint] > 0) //> 0 left, <0 right
				{
					negMesh.AddSlicedTriangle(
						t[lonelyPoint],
						std::get<0>(newPointNext),
						std::get<0>(newPointPrev), 
						std::get<1>(newPointNext),
						std::get<1>(newPointPrev), 
						std::get<2>(newPointNext), 
						std::get<2>(newPointPrev),
						id[lonelyPoint] ,
						id[lonelyPoint]);
				}
				else
				{
					posMesh.AddSlicedTriangle(
						t[lonelyPoint],
						std::get<0>(newPointNext),
						std::get<0>(newPointPrev), 
						std::get<1>(newPointNext),
						std::get<1>(newPointPrev),
						std::get<2>(newPointNext), 
						std::get<2>(newPointPrev),
						id[lonelyPoint], 
						id[lonelyPoint]);
				}

				// pre,  newPre,next
				if (positive[prevPoint] > 0)
				{
					negMesh.AddSlicedTriangle(t[prevPoint], std::get<0>(newPointPrev), t[nextPoint], std::get<1>(newPointPrev) ,std::get<2>(newPointPrev), id[prevPoint]);
				}
				else
				{
					posMesh.AddSlicedTriangle(t[prevPoint], std::get<0>(newPointPrev), t[nextPoint], std::get<1>(newPointPrev), std::get<2>(newPointPrev), id[prevPoint]);
				}

				if (positive[prevPoint] > 0)//newPre,newNext,next
				{
					negMesh.AddSlicedTriangle(
						t[nextPoint],
						std::get<0>(newPointPrev),
						std::get<0>(newPointNext),
						std::get<1>(newPointPrev),
						std::get<1>(newPointNext),
						std::get<2>(newPointPrev),
						std::get<2>(newPointNext),
						id[prevPoint],
						id[nextPoint]);
				}
				else
				{
 					posMesh.AddSlicedTriangle(
						t[nextPoint],
						std::get<0>(newPointPrev),
						std::get<0>(newPointNext),
						std::get<1>(newPointPrev),
						std::get<1>(newPointNext),
						std::get<2>(newPointPrev),
						std::get<2>(newPointNext),
						id[prevPoint],
						id[nextPoint]);
				}
			}
			return true;
		}
	};


	class MeshCutter
	{
	public:
		TempMesh PositiveMesh;
		TempMesh  NegativeMesh;

		std::vector<osg::Vec3> ogVertices;
		std::vector<unsigned int> ogTriangles;
		std::vector<float> ogBatchIDs;
		std::vector<osg::Vec2> ogUvs;
		std::vector<osg::Vec4> ogColors;

		std::vector<osg::Vec3 > addedPairs;
		std::vector<osg::Vec2 > addedUVPairs;
		std::vector<osg::Vec4 > addedColorPairs;
		std::vector<int > addedIDPairs;

		std::vector<bool >         validPairsFlag;

		std::vector<osg::Vec3> tempTriangle;
		std::vector<osg::Vec4> tempColor;
		std::vector<osg::Vec2> tempUV;
		std::vector<int > tempID;

		std::vector<int> sideFlags;
		int leftCount = 0;
		int rightCount = 0;
		Intersections intersect;

		float threshold = 1e-6f;

		MeshCutter()
		{
			tempTriangle.resize(3);
			tempUV.resize(3);
			tempID.resize(3);
			tempColor.resize(3);
		}
		~MeshCutter()
		{
			clear();
		}

		void clear()
		{
			PositiveMesh.Clear(); 
			NegativeMesh.Clear();

			ogVertices.clear(); 
			ogTriangles.clear(); 
			ogBatchIDs.clear();
			ogUvs.clear();
			ogColors.clear();

			addedPairs.clear();
			addedUVPairs.clear();
			addedIDPairs.clear();
			addedColorPairs.clear();

			validPairsFlag.clear(); 
			sideFlags.clear();

			tempTriangle.clear(); 
			tempUV.clear();
			tempID.clear();
			tempColor.clear();
		}

		void transP(osg::Vec3& pt)
		{
			double tt = pt[0];
			pt[0] = pt[2];
			pt[2] = pt[1];
			pt[1] = tt;
		}

		bool SliceMesh(GMesh& mesh, osg::Plane slice)
		{
			if (!intersect.BoundPlaneIntersect(mesh, slice))
				return false;

			ogVertices = mesh.positions;
			ogTriangles = mesh.indices;
			ogUvs = mesh.UVs;
			ogBatchIDs = mesh.batchIDs;
			ogColors = mesh.colors;

			PositiveMesh.Clear();
			NegativeMesh.Clear();

			addedPairs.clear();
			addedUVPairs.clear();
			addedIDPairs.clear();
			addedColorPairs.clear();

			sideFlags.clear();

			rightCount = 0;
			leftCount = 0;	

			PositiveMesh.vertices.reserve(mesh.positions.size() *0.5);
			NegativeMesh.vertices.reserve(mesh.positions.size() *0.5);

			PositiveMesh.batchIDs.reserve(mesh.positions.size() *0.5);
			NegativeMesh.batchIDs.reserve(mesh.positions.size() *0.5);

			PositiveMesh.triangles.reserve(mesh.positions.size() *0.5);
			NegativeMesh.triangles.reserve(mesh.positions.size() *0.5);

			PositiveMesh.vMapping.resize(mesh.positions.size());
			NegativeMesh.vMapping.resize(mesh.positions.size());

			if (mesh.UVs.empty() || mesh.UVs.size() != mesh.batchIDs.size())
			{
				PositiveMesh.bUV = false;
				NegativeMesh.bUV = false;
				mesh.UVs.clear();
			}
			if (PositiveMesh.bUV)
				PositiveMesh.uvs.reserve(mesh.positions.size() *0.5);
			if (NegativeMesh.bUV)
				NegativeMesh.uvs.reserve(mesh.positions.size() *0.5);
 
			if (mesh.colors.empty() || mesh.colors.size() != mesh.batchIDs.size())
			{
				PositiveMesh.bColor = false;
				NegativeMesh.bColor = false;
				mesh.colors.clear();
			}
			if (PositiveMesh.bColor)
				PositiveMesh.colors.reserve(mesh.positions.size() *0.5);

			if (NegativeMesh.bColor)
				NegativeMesh.colors.reserve(mesh.positions.size() *0.5);
 
			sideFlags.resize(mesh.positions.size(), 0);

			for (int i = 0; i < ogVertices.size(); ++i)
			{
				double dist = slice.distance(ogVertices[i]);
				if (dist >= 0)
				{
					sideFlags[i]--;
					rightCount++;
					PositiveMesh.AddVertex(ogVertices, ogUvs, ogBatchIDs,ogColors, i);
				}
				if (dist <= 0)
				{
					sideFlags[i]++;
					leftCount++;
					NegativeMesh.AddVertex(ogVertices, ogUvs,ogBatchIDs, ogColors, i);
				}
			}

			// 2.5 : If one of the mesh has no vertices, then it doesn't intersect
			if (rightCount == 0 || leftCount == 0)
				return false;

			std::vector<int> existLine1;
			std::vector<int> existLine2;

			// 3. Separate triangles and cut those that intersect the plane
			for (int i = 0; i < ogTriangles.size(); i += 3)
			{
				std::vector<osg::Vec3> intersectPair;
				std::vector<osg::Vec2> intersectUVPair;
				std::vector<int > intersectIDPair;

				if (intersect.TrianglePlaneIntersectX(ogVertices, ogBatchIDs, ogUvs, ogColors, ogTriangles, i, slice, sideFlags, PositiveMesh, NegativeMesh, intersectPair,intersectUVPair ,intersectIDPair) )
				{
	/*				int p1value = intersectPair[0].y() * 1.0e6 + intersectPair[0].z() * 1.0e3;
					int p2value = intersectPair[1].y() * 1.0e6 + intersectPair[1].z() * 1.0e3;

					addedPairs.push_back(intersectPair[0]);
					addedPairs.push_back(intersectPair[1]);

					addedUVPairs.push_back(intersectUVPair[0]);
					addedUVPairs.push_back(intersectUVPair[1]);

					addedIDPairs.push_back(intersectIDPair[0]);
					addedIDPairs.push_back(intersectIDPair[1]);

					existLine1.push_back(p1value);
					existLine2.push_back(p2value);*/
				}
			}

			int nPairs = addedPairs.size() / 2;

			std::vector<bool> flags;
			flags.resize(nPairs, true);

			for (int i = 0; i < nPairs; i++)
			{
				int p1value = existLine1[i];
				int p2value = existLine2[i];

				bool bSame = false;
				for (int j = i + 1; j < nPairs; j++)
				{
					if ((existLine1[j] == p1value && existLine2[j] == p2value) || (existLine1[j] == p2value && existLine2[j] == p1value))
					{
						bSame = true;
						flags[i] = false;
						flags[j] = false;
					}
				}
			}

			//临时注释, 先不处理切面	,todo 

			return true;
		}
	};


	osg::BoundingBoxd getMeshBox(GMesh& mesh)
	{
		osg::BoundingBoxd box;
		for (auto& point : mesh.positions)
		{
			box.expandBy(point);
		}

		return box;
	}

	void clearMesh(GMesh& smesh)
	{
			smesh.positions.clear();
			smesh.batchIDs.clear();
			smesh.UVs.clear();
			smesh.indices.clear();
			smesh.colors.clear();
			smesh.normals.clear();
	}

	void   toSMesh(TempMesh& tMesh, GMesh& smesh)
	{
		clearMesh(smesh);

		int nTri = tMesh.triangles.size()/3;

		smesh.positions.reserve(nTri * 3);
		smesh.batchIDs.reserve(nTri * 3);
		smesh.UVs.reserve(nTri * 3);
		smesh.indices.reserve(nTri * 3);
		smesh.colors.reserve(nTri * 3);

		bool bUV = tMesh.uvs.size() == tMesh.vertices.size();
		bool bColor = tMesh.colors.size() == tMesh.vertices.size();

 
		for (int i = 0; i < nTri; i++)
		{
			int p1 = tMesh.triangles[3 * i + 0];
			int p2 = tMesh.triangles[3 * i + 1];
			int p3 = tMesh.triangles[3 * i + 2];
			//
			smesh.positions.push_back(tMesh.vertices[p1]);
			smesh.positions.push_back(tMesh.vertices[p2]);
			smesh.positions.push_back(tMesh.vertices[p3]);

			smesh.batchIDs.push_back(tMesh.batchIDs[p1]);
			smesh.batchIDs.push_back(tMesh.batchIDs[p2]);
			smesh.batchIDs.push_back(tMesh.batchIDs[p3]);

			if (bUV)
			{
				smesh.UVs.push_back(tMesh.uvs[p1]);
				smesh.UVs.push_back(tMesh.uvs[p2]);
				smesh.UVs.push_back(tMesh.uvs[p3]);
			}
			if (bColor)
			{
				smesh.colors.push_back(tMesh.colors[p1]);
				smesh.colors.push_back(tMesh.colors[p2]);
				smesh.colors.push_back(tMesh.colors[p3]);
			}

			smesh.indices.push_back(3 * i + 0);
			smesh.indices.push_back(3 * i + 1);
			smesh.indices.push_back(3 * i + 2);
		}

		smesh.aabb = getMeshBox(smesh);
		smesh.nFace = smesh.indices.size();
	}

	void  DGSplitMeshToQuard(int maxLevel, GMesh& mesh, std::vector< GMesh >& childMeshs)
	{
		if (getFlag())
			return;	

		int level = mesh.level;
		osg::BoundingBox aabb = mesh.aabb;
		double DX = aabb._max.x() - aabb._min.x();
		double DY = aabb._max.y() - aabb._min.y();
		double DZ = aabb._max.z() - aabb._min.z();

		double dMax = std::max(DX, DY);

		double ingoreScope = 6;
		if (DZ / dMax > 5 || dMax < ingoreScope)//局部小体积多顶点模型，不切分
		{
			return;
		}

		if (mesh.indices.size() < 512)
			return;
		if (maxLevel < 1)
		{
			if (dMax < 5000 || mesh.indices.size() < 1000000)
				return;
		}

		if (mesh.level >= maxLevel && mesh.indices.size() < 6000 || mesh.level > 1.5*maxLevel)
		{
			if (mesh.level >= maxLevel && mesh.indices.size() >= 6000)
			{
				//			std::cout << "SplitMeshToQuard error" << endl;
			}
			return;
		}

		osg::Vec3 center = mesh.aabb.center();
		{
			if (DX / DY > 2 || DY / DX > 2)
			{
				int n = DX / DY;
				if (n < 1)
					n = DY / DX;
				if (n > 6)
					n = 6;
				if (n < 3)
					n = 3;

				childMeshs.resize(n);

				for (int i = 1; i < n; i++)
				{
					MeshCutter cutter;
					osg::Plane planeX(1, 0, 0, 0);

					if (DX / DY > 1)
					{
						double x1 = aabb._min.x() + 1.0*i / n * DX;
						planeX.set(1, 0, 0, -x1);
					}
					else
					{
						double x1 = aabb._min.y() + 1.0*i / n * DY;
						planeX.set(0, 1, 0, -x1);
					}

					bool flag = cutter.SliceMesh(mesh, planeX);
					if (flag)
					{
						toSMesh(cutter.NegativeMesh, childMeshs[i - 1]);
						if (i == n - 1)
						{
							toSMesh(cutter.PositiveMesh, childMeshs[i]);
						}
						else
						{
							toSMesh(cutter.PositiveMesh, mesh); // 1-3 
						}
					}
					else
					{
						childMeshs[i - 1] = mesh;//不相交得话
						break;
					}
				}
			}
			else
			{
				MeshCutter cutter;
				childMeshs.resize(4);
				osg::Plane planeX(1, 0, 0, -center.x());
				bool flag = cutter.SliceMesh(mesh, planeX);
				if (flag)
				{
					GMesh lm;
					toSMesh(cutter.NegativeMesh, lm);

					GMesh rm;
					toSMesh(cutter.PositiveMesh, rm);
					cutter.clear();

					osg::Vec3 center1 = lm.aabb.center();
					osg::Plane planeY1(0, 1, 0, -center1.y());

					bool flag2 = cutter.SliceMesh(lm, planeY1);

					clearMesh(lm);


					toSMesh(cutter.PositiveMesh, childMeshs[3]);
					toSMesh(cutter.NegativeMesh, childMeshs[0]);
					cutter.clear();
					{
						MeshCutter cutterR;
						osg::Vec3 center2 = rm.aabb.center();
						osg::Plane planeY2(0, 1, 0, -center2.y());

						bool flag3 = cutterR.SliceMesh(rm, planeY2);
						clearMesh(rm);


						toSMesh(cutterR.NegativeMesh, childMeshs[1]);
						toSMesh(cutterR.PositiveMesh, childMeshs[2]);
					}
				}
			}
		}

		for (auto& subMesh : childMeshs)
		{
			subMesh.level = level + 1;
			subMesh.bText = mesh.bText;
		}
	}


	void  SplitMeshToQuard(bool bDK, int maxLevel,GMesh& mesh, std::vector< GMesh >& childMeshs,int maxPt)
	{
		if (!bDK)
			return DGSplitMeshToQuard(maxLevel, mesh, childMeshs);

		if (mesh.positions.size() > maxPt)
		{
			std::vector< GMesh > tempMeshs;
			DGSplitMeshToQuard(maxLevel, mesh, tempMeshs);
			for (auto& child : tempMeshs)
			{
				SplitMeshToQuard(bDK, maxLevel, child, childMeshs,maxPt);
			}
		}
		else
		{
			childMeshs.push_back(mesh);
		}

	}


	//

	 bool  simpleMesh(GMesh& mesh, double factor, double& targetError)
	 {
 			if(getFlag() )
				return true;

		 return SIMPLE::SimpleMeshByFator_func(mesh, factor, targetError );
	 }

	 //
	 bool  splitMesh(GMesh& mesh, std::vector<GMesh>&subMesh)
	 {
		 SIMPLE::SplitMesh_func(mesh,subMesh);
		 if(subMesh.size() > 0)
			 return true;
		 return false;
	 }
 
};


