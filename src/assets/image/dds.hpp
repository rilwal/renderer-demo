#pragma once

// Load DDS compressed texture files
// Based on MS Documentation here: https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide#related-topics

#include <cinttypes>
#include <vector>
#include <filesystem>

#include "util.hpp"




constexpr uint32_t g_dds_magic = 0x20534444;

constexpr uint32_t make_four_cc(const char in[4]) {
	return 
		static_cast<uint32_t>(in[3]) << 24 |
		static_cast<uint32_t>(in[2]) << 16 |
		static_cast<uint32_t>(in[1]) << 8 |
		static_cast<uint32_t>(in[0]);
}


enum class DDS_FourCC : uint32_t {
	None = 0,
	DXT1 = make_four_cc("DXT1"),
	DXT2 = make_four_cc("DXT2"),
	DXT3 = make_four_cc("DXT3"),
	DXT4 = make_four_cc("DXT4"),
	DXT5 = make_four_cc("DXT5"),
	DX10 = make_four_cc("DX10")
};


enum class DDS_Format : uint32_t {
	UNKNOWN = 0,
	R32G32B32A32_TYPELESS = 1,
	R32G32B32A32_FLOAT = 2,
	R32G32B32A32_UINT = 3,
	R32G32B32A32_SINT = 4,
	R32G32B32_TYPELESS = 5,
	R32G32B32_FLOAT = 6,
	R32G32B32_UINT = 7,
	R32G32B32_SINT = 8,
	R16G16B16A16_TYPELESS = 9,
	R16G16B16A16_FLOAT = 10,
	R16G16B16A16_UNORM = 11,
	R16G16B16A16_UINT = 12,
	R16G16B16A16_SNORM = 13,
	R16G16B16A16_SINT = 14,
	R32G32_TYPELESS = 15,
	R32G32_FLOAT = 16,
	R32G32_UINT = 17,
	R32G32_SINT = 18,
	R32G8X24_TYPELESS = 19,
	D32_FLOAT_S8X24_UINT = 20,
	R32_FLOAT_X8X24_TYPELESS = 21,
	X32_TYPELESS_G8X24_UINT = 22,
	R10G10B10A2_TYPELESS = 23,
	R10G10B10A2_UNORM = 24,
	R10G10B10A2_UINT = 25,
	R11G11B10_FLOAT = 26,
	R8G8B8A8_TYPELESS = 27,
	R8G8B8A8_UNORM = 28,
	R8G8B8A8_UNORM_SRGB = 29,
	R8G8B8A8_UINT = 30,
	R8G8B8A8_SNORM = 31,
	R8G8B8A8_SINT = 32,
	R16G16_TYPELESS = 33,
	R16G16_FLOAT = 34,
	R16G16_UNORM = 35,
	R16G16_UINT = 36,
	R16G16_SNORM = 37,
	R16G16_SINT = 38,
	R32_TYPELESS = 39,
	D32_FLOAT = 40,
	R32_FLOAT = 41,
	R32_UINT = 42,
	R32_SINT = 43,
	R24G8_TYPELESS = 44,
	D24_UNORM_S8_UINT = 45,
	R24_UNORM_X8_TYPELESS = 46,
	X24_TYPELESS_G8_UINT = 47,
	R8G8_TYPELESS = 48,
	R8G8_UNORM = 49,
	R8G8_UINT = 50,
	R8G8_SNORM = 51,
	R8G8_SINT = 52,
	R16_TYPELESS = 53,
	R16_FLOAT = 54,
	D16_UNORM = 55,
	R16_UNORM = 56,
	R16_UINT = 57,
	R16_SNORM = 58,
	R16_SINT = 59,
	R8_TYPELESS = 60,
	R8_UNORM = 61,
	R8_UINT = 62,
	R8_SNORM = 63,
	R8_SINT = 64,
	A8_UNORM = 65,
	R1_UNORM = 66,
	R9G9B9E5_SHAREDEXP = 67,
	R8G8_B8G8_UNORM = 68,
	G8R8_G8B8_UNORM = 69,
	BC1_TYPELESS = 70,
	BC1_UNORM = 71,
	BC1_UNORM_SRGB = 72,
	BC2_TYPELESS = 73,
	BC2_UNORM = 74,
	BC2_UNORM_SRGB = 75,
	BC3_TYPELESS = 76,
	BC3_UNORM = 77,
	BC3_UNORM_SRGB = 78,
	BC4_TYPELESS = 79,
	BC4_UNORM = 80,
	BC4_SNORM = 81,
	BC5_TYPELESS = 82,
	BC5_UNORM = 83,
	BC5_SNORM = 84,
	B5G6R5_UNORM = 85,
	B5G5R5A1_UNORM = 86,
	B8G8R8A8_UNORM = 87,
	B8G8R8X8_UNORM = 88,
	R10G10B10_XR_BIAS_A2_UNORM = 89,
	B8G8R8A8_TYPELESS = 90,
	B8G8R8A8_UNORM_SRGB = 91,
	B8G8R8X8_TYPELESS = 92,
	B8G8R8X8_UNORM_SRGB = 93,
	BC6H_TYPELESS = 94,
	BC6H_UF16 = 95,
	BC6H_SF16 = 96,
	BC7_TYPELESS = 97,
	BC7_UNORM = 98,
	BC7_UNORM_SRGB = 99,
	AYUV = 100,
	Y410 = 101,
	Y416 = 102,
	NV12 = 103,
	P010 = 104,
	P016 = 105,
	YUV_420_OPAQUE = 106,
	YUY2 = 107,
	Y210 = 108,
	Y216 = 109,
	NV11 = 110,
	AI44 = 111,
	IA44 = 112,
	P8 = 113,
	A8P8 = 114,
	B4G4R4A4_UNORM = 115,
	P208 = 130,
	V208 = 131,
	V408 = 132,
	SAMPLER_FEEDBACK_MIN_MIP_OPAQUE,
	SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE,
	FORCE_UINT = 0xffffffff
};


enum class DDS_Dimension {
	Unknown		= 0,
	Buffer		= 1,
	Texture1D	= 2,
	Texture2D   = 3,
	Texture3D	= 4
};

// Bitset indices (LOG_2 of the standard constants)
enum class DDS_PixelFormatFlags : uint32_t {
	AlphaPixels = 0x0,
	Alpha		= 0x1,
	Fourcc		= 0x2,
	RGB			= 0x5,
	YUV			= 0x8,
	LUMINANCE	= 0x10
};

enum class DDS_Capabilities : uint32_t {
	Complex = 0x3,
	Texture = 0xC,
	MipMap	= 0x16
};

enum class DDS_Capabilities2 : uint32_t {
	Cubemap			 = 0x9,
	CubemapPositiveX = 0xA,
	CubemapNegativeX = 0xB,
	CubemapPositiveY = 0xC,
	CubemapNegativeY = 0xD,
	CubemapPositiveZ = 0xE,
	CubemapNegativeZ = 0xF,
	Volume			 = 0x15
};

// In docs this is call MiscFlag2, but for ease of code readability
// I will call it AlphaMode. 
enum class DDS_AlphaMode : uint32_t {
	Unknown = 0x0,
	Straight = 0x1,
	Premultiplied = 0x2,
	Opaque = 0x3,
	Custom = 0x4 // Alpha channel doesn't represent alpha
};


// In docs this is call MiscFlag, but for ease of code readability
// I will call it CubeMapFlag. 
enum class DDS_CubeMapFlag : uint32_t {
	CubeMap = 0x2
};


#pragma pack (push, 1)
struct DDS_PixelFormat {
	uint32_t size;
	EnumBitset<DDS_PixelFormatFlags> flags;
	Enum<DDS_FourCC> four_cc;
	uint32_t rgb_bitcount;
	uint32_t r_bitmask;
	uint32_t g_bitmask;
	uint32_t b_bitmask;
	uint32_t a_bitmask;
};

struct DDS_Header {
	uint32_t size;
	uint32_t flags;
	uint32_t height;
	uint32_t width;
	uint32_t pitch_or_linear_size;
	uint32_t depth_count;
	uint32_t mipmap_count;
	uint32_t _reserved_1[11]; // UNUSED
	DDS_PixelFormat pixel_format;
	EnumBitset<DDS_Capabilities> capabilities;
	EnumBitset<DDS_Capabilities2> capabilities_2;
	std::bitset<32> capabilities_3; // UNUSED
	std::bitset<32> capabilities_4; // UNUSED
	uint32_t _reserved_2; // UNUSED
};

struct DDS_Header_DX10 {
	DDS_Format format;
	DDS_Dimension dimension;
	EnumBitset<DDS_CubeMapFlag> misc_flag;
	uint32_t array_size;
	DDS_AlphaMode alpha_mode;
	
};
#pragma pack (pop)	


struct Image;


// Just do magic number check
bool is_dds(std::vector<uint8_t> buffer);


// ID is to be used in error output when the data comes from a buffer,
// to help use identify what files etc. are causing issues.
std::unique_ptr<Image> load_dds(std::vector<uint8_t> buffer, std::string id = "");
std::unique_ptr<Image> load_dds(std::filesystem::path path);
