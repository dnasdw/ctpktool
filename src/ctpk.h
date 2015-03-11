#ifndef CTPK_H_
#define CTPK_H_

#include "utility.h"

namespace pvrtexture
{
	class CPVRTexture;
}

#include MSC_PUSH_PACKED
struct SCtpkHeader
{
	u32 Signature;
	u16 Version;
	u16 Count;
	u32 TextureOffset;
	u32 TextureSize;
	u32 HashOffset;
	u32 TextureShortInfoOffset;
	u32 Reserved[2];
} GNUC_PACKED;

struct SCtrTextureInfo
{
	u32 FilePathOffset;
	u32 TexDataSize;
	u32 TexDataOffset;
	u32 TexFormat;
	u16 Width;
	u16 Height;
	u8 MipLevel;
	u8 Type;
	u16 CubeDir;
	u32 BitmapSizeOffset;
	u32 SrcFileTime;
} GNUC_PACKED;

struct STextureShortInfo
{
	u8 TextFormat;
	u8 MipLevel;
	u8 Compression;
	u8 CompressionMethod;
} GNUC_PACKED;
#include MSC_POP_PACKED

class CCtpk
{
public:
	enum ETextureFormat
	{
		kTextureFormatRGBA8888 = 0,
		kTextureFormatRGB888 = 1,
		kTextureFormatRGBA5551 = 2,
		kTextureFormatRGB565 = 3,
		kTextureFormatRGBA4444 = 4,
		kTextureFormatLA88 = 5,
		kTextureFormatHL8 = 6,
		kTextureFormatL8 = 7,
		kTextureFormatA8 = 8,
		kTextureFormatLA44 = 9,
		kTextureFormatL4 = 10,
		kTextureFormatA4 = 11,
		kTextureFormatETC1 = 12,
		kTextureFormatETC1_A4 = 13
	};
	CCtpk();
	~CCtpk();
	void SetFileName(const char* a_pFileName);
	void SetDirName(const char* a_pDirName);
	void SetVerbose(bool a_bVerbose);
	bool ExportFile();
	bool ImportFile();
	static bool IsCtpkFile(const char* a_pFileName);
	static const u32 s_uSignature;
	static const int s_nBPP[];
	static const int s_nDecodeTransByte[64];
private:
	static int decode(u8* a_pBuffer, n32 a_nWidth, n32 a_nHeight, n32 a_nFormat, pvrtexture::CPVRTexture** a_pPVRTexture);
	static void encode(u8* a_pData, n32 a_nWidth, n32 a_nHeight, n32 a_nFormat, n32 a_nMipmapLevel, n32 a_nBPP, u8** a_pBuffer);
	const char* m_pFileName;
	String m_sDirName;
	bool m_bVerbose;
	FILE* m_fpCtpk;
};

#endif	// CTPK_H_
