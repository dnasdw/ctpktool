#include "ctpk.h"
#include <png.h>
#include <PVRTextureUtilities.h>

const u32 CCtpk::s_uSignature = CONVERT_ENDIAN('CTPK');
const int CCtpk::s_nBPP[] = { 32, 24, 16, 16, 16, 16, 16, 8, 8, 8, 4, 4, 4, 8 };
const int CCtpk::s_nDecodeTransByte[64] =
{
	 0,  1,  4,  5, 16, 17, 20, 21,
	 2,  3,  6,  7, 18, 19, 22, 23,
	 8,  9, 12, 13, 24, 25, 28, 29,
	10, 11, 14, 15, 26, 27, 30, 31,
	32, 33, 36, 37, 48, 49, 52, 53,
	34, 35, 38, 39, 50, 51, 54, 55,
	40, 41, 44, 45, 56, 57, 60, 61,
	42, 43, 46, 47, 58, 59, 62, 63
};

CCtpk::CCtpk()
	: m_pFileName(nullptr)
	, m_bVerbose(false)
	, m_fpCtpk(nullptr)
{
}

CCtpk::~CCtpk()
{
}

void CCtpk::SetFileName(const char* a_pFileName)
{
	m_pFileName = a_pFileName;
}

void CCtpk::SetDirName(const char* a_pDirName)
{
	m_sDirName = FSAToUnicode(a_pDirName);
}

void CCtpk::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

bool CCtpk::ExportFile()
{
	bool bResult = true;
	m_fpCtpk = FFopen(m_pFileName, "rb");
	if (m_fpCtpk == nullptr)
	{
		return false;
	}
	FFseek(m_fpCtpk, 0, SEEK_END);
	n64 nFileSize = FFtell(m_fpCtpk);
	FFseek(m_fpCtpk, 0, SEEK_SET);
	u8* pBin = new u8[static_cast<size_t>(nFileSize)];
	fread(pBin, 1, static_cast<size_t>(nFileSize), m_fpCtpk);
	fclose(m_fpCtpk);
	SCtpkHeader* pCtpkHeader = reinterpret_cast<SCtpkHeader*>(pBin);
	SCtrTextureInfo* pCtrTextureInfo = reinterpret_cast<SCtrTextureInfo*>(pBin + sizeof(*pCtpkHeader));
	STextureShortInfo* pTextureShortInfo = reinterpret_cast<STextureShortInfo*>(pBin + pCtpkHeader->TextureShortInfoOffset);
	FMakeDir(m_sDirName.c_str());
	for (n32 i = 0; i < pCtpkHeader->Count; i++)
	{
		if (pTextureShortInfo[i].TextFormat != 0xFF && pCtrTextureInfo[i].TexFormat != pTextureShortInfo[i].TextFormat)
		{
			printf("ERROR: format is not equivalent\n\n");
			bResult = false;
			break;
		}
		if (pCtrTextureInfo[i].TexFormat < kTextureFormatRGBA8888 || pCtrTextureInfo[i].TexFormat > kTextureFormatETC1_A4)
		{
			printf("ERROR: unknown format %d\n\n", pCtrTextureInfo[i].TexFormat);
			bResult = false;
			break;
		}
		n32 nCheckSize = 0;
		for (n32 l = 0; l < pCtrTextureInfo[i].MipLevel; l++)
		{
			n32 nMipmapHeight = pCtrTextureInfo[i].Height >> l;
			n32 nMipmapWidth = pCtrTextureInfo[i].Width >> l;
			nCheckSize += nMipmapHeight * nMipmapWidth * s_nBPP[pCtrTextureInfo[i].TexFormat] / 8;
		}
		if (pCtrTextureInfo[i].TexDataSize != nCheckSize && m_bVerbose)
		{
			printf("INFO: width: %X, height: %X, checksize: %X, size: %X, bpp: %d, format: %0X\n", pCtrTextureInfo[i].Width, pCtrTextureInfo[i].Height, nCheckSize, pCtrTextureInfo[i].TexDataSize, pCtrTextureInfo[i].TexDataSize * 8 / pCtrTextureInfo[i].Width / pCtrTextureInfo[i].Height, pCtrTextureInfo[i].TexFormat);
		}
		pvrtexture::CPVRTexture* pPVRTexture = nullptr;
		if (decode(pBin + pCtpkHeader->TextureOffset + pCtrTextureInfo[i].TexDataOffset, pCtrTextureInfo[i].Width, pCtrTextureInfo[i].Height, pCtrTextureInfo[i].TexFormat, &pPVRTexture) == 0)
		{
			String sFilePath = FSAToUnicode(reinterpret_cast<char*>(pBin + pCtrTextureInfo[i].FilePathOffset));
			remove(sFilePath.begin(), sFilePath.end(), STR(':'));
			vector<String> sSplitPath = FSSplitOf<String>(sFilePath, STR("/\\"));
			String sDirPath = m_sDirName;
			for (int j = 0; j < static_cast<int>(sSplitPath.size()) - 1; j++)
			{
				sDirPath += STR("/");
				sDirPath += sSplitPath[j];
				FMakeDir(sDirPath.c_str());
			}
			sFilePath = sDirPath + STR("/") + sSplitPath[sSplitPath.size() - 1] + STR(".png");
			FILE* fp = FFopenUnicode(sFilePath.c_str(), STR("wb"));
			if (fp == nullptr)
			{
				delete pPVRTexture;
				bResult = false;
				break;
			}
			if (m_bVerbose)
			{
				FPrintf(STR("save: %s\n"), sFilePath.c_str());
			}
			png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)nullptr, nullptr, nullptr);
			if (png_ptr == nullptr)
			{
				fclose(fp);
				delete pPVRTexture;
				printf("ERROR: png_create_write_struct error\n\n");
				bResult = false;
				break;
			}
			png_infop info_ptr = png_create_info_struct(png_ptr);
			if (info_ptr == nullptr)
			{
				png_destroy_write_struct(&png_ptr, (png_infopp)nullptr);
				fclose(fp);
				delete pPVRTexture;
				printf("ERROR: png_create_info_struct error\n\n");
				bResult = false;
				break;
			}
			if (setjmp(png_jmpbuf(png_ptr)) != 0)
			{
				png_destroy_write_struct(&png_ptr, &info_ptr);
				fclose(fp);
				delete pPVRTexture;
				printf("ERROR: setjmp error\n\n");
				bResult = false;
				break;
			}
			png_init_io(png_ptr, fp);
			png_set_IHDR(png_ptr, info_ptr, pCtrTextureInfo[i].Width, pCtrTextureInfo[i].Height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
			u8* pData = static_cast<u8*>(pPVRTexture->getDataPtr());
			png_bytepp pRowPointers = new png_bytep[pCtrTextureInfo[i].Height];
			for (n32 j = 0; j < pCtrTextureInfo[i].Height; j++)
			{
				pRowPointers[j] = pData + j * pCtrTextureInfo[i].Width * 4;
			}
			png_set_rows(png_ptr, info_ptr, pRowPointers);
			png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);
			png_destroy_write_struct(&png_ptr, &info_ptr);
			delete[] pRowPointers;
			fclose(fp);
			delete pPVRTexture;
		}
		else
		{
			printf("ERROR: decode error\n\n");
			bResult = false;
			break;
		}
	}
	delete[] pBin;
	return bResult;
}

bool CCtpk::ImportFile()
{
	bool bResult = true;
	m_fpCtpk = FFopen(m_pFileName, "rb");
	if (m_fpCtpk == nullptr)
	{
		return false;
	}
	FFseek(m_fpCtpk, 0, SEEK_END);
	n64 nFileSize = FFtell(m_fpCtpk);
	FFseek(m_fpCtpk, 0, SEEK_SET);
	u8* pBin = new u8[static_cast<size_t>(nFileSize)];
	fread(pBin, 1, static_cast<size_t>(nFileSize), m_fpCtpk);
	fclose(m_fpCtpk);
	SCtpkHeader* pCtpkHeader = reinterpret_cast<SCtpkHeader*>(pBin);
	SCtrTextureInfo* pCtrTextureInfo = reinterpret_cast<SCtrTextureInfo*>(pBin + sizeof(*pCtpkHeader));
	STextureShortInfo* pTextureShortInfo = reinterpret_cast<STextureShortInfo*>(pBin + pCtpkHeader->TextureShortInfoOffset);
	for (n32 i = 0; i < pCtpkHeader->Count; i++)
	{
		if (pTextureShortInfo[i].TextFormat != 0xFF && pCtrTextureInfo[i].TexFormat != pTextureShortInfo[i].TextFormat)
		{
			printf("ERROR: format is not equivalent\n\n");
			bResult = false;
			break;
		}
		if (pCtrTextureInfo[i].TexFormat < kTextureFormatRGBA8888 || pCtrTextureInfo[i].TexFormat > kTextureFormatETC1_A4)
		{
			printf("ERROR: unknown format %d\n\n", pCtrTextureInfo[i].TexFormat);
			bResult = false;
			break;
		}
		n32 nCheckSize = 0;
		for (n32 l = 0; l < pCtrTextureInfo[i].MipLevel; l++)
		{
			n32 nMipmapHeight = pCtrTextureInfo[i].Height >> l;
			n32 nMipmapWidth = pCtrTextureInfo[i].Width >> l;
			nCheckSize += nMipmapHeight * nMipmapWidth * s_nBPP[pCtrTextureInfo[i].TexFormat] / 8;
		}
		if (pCtrTextureInfo[i].TexDataSize != nCheckSize && m_bVerbose)
		{
			printf("INFO: width: %X, height: %X, checksize: %X, size: %X, bpp: %d, format: %0X\n", pCtrTextureInfo[i].Width, pCtrTextureInfo[i].Height, nCheckSize, pCtrTextureInfo[i].TexDataSize, pCtrTextureInfo[i].TexDataSize * 8 / pCtrTextureInfo[i].Width / pCtrTextureInfo[i].Height, pCtrTextureInfo[i].TexFormat);
		}
		String sFilePath = FSAToUnicode(reinterpret_cast<char*>(pBin + pCtrTextureInfo[i].FilePathOffset));
		remove(sFilePath.begin(), sFilePath.end(), STR(':'));
		sFilePath = m_sDirName + STR("/") + sFilePath + STR(".png");
		FILE* fp = FFopenUnicode(sFilePath.c_str(), STR("rb"));
		if (fp == nullptr)
		{
			bResult = false;
			break;
		}
		if (m_bVerbose)
		{
			FPrintf(STR("load: %s\n"), sFilePath.c_str());
		}
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)nullptr, nullptr, nullptr);
		if (png_ptr == nullptr)
		{
			fclose(fp);
			printf("ERROR: png_create_read_struct error\n\n");
			bResult = false;
			break;
		}
		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (info_ptr == nullptr)
		{
			png_destroy_read_struct(&png_ptr, (png_infopp)nullptr, (png_infopp)nullptr);
			fclose(fp);
			printf("ERROR: png_create_info_struct error\n\n");
			bResult = false;
			break;
		}
		png_infop end_info = png_create_info_struct(png_ptr);
		if (end_info == nullptr)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
			fclose(fp);
			printf("ERROR: png_create_info_struct error\n\n");
			bResult = false;
			break;
		}
		if (setjmp(png_jmpbuf(png_ptr)) != 0)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
			fclose(fp);
			printf("ERROR: setjmp error\n\n");
			bResult = false;
			break;
		}
		png_init_io(png_ptr, fp);
		png_read_info(png_ptr, info_ptr);
		n32 nPngWidth = png_get_image_width(png_ptr, info_ptr);
		if (nPngWidth != pCtrTextureInfo[i].Width)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
			fclose(fp);
			printf("ERROR: nPngWidth != Width\n\n");
			bResult = false;
			break;
		}
		n32 nPngHeight = png_get_image_height(png_ptr, info_ptr);
		if (nPngHeight != pCtrTextureInfo[i].Height)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
			fclose(fp);
			printf("ERROR: nPngHeight != Height\n\n");
			bResult = false;
			break;
		}
		n32 nBitDepth = png_get_bit_depth(png_ptr, info_ptr);
		if (nBitDepth != 8)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
			fclose(fp);
			printf("ERROR: nBitDepth != 8\n\n");
			bResult = false;
			break;
		}
		n32 nColorType = png_get_color_type(png_ptr, info_ptr);
		if (nColorType != PNG_COLOR_TYPE_RGB_ALPHA)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
			fclose(fp);
			printf("ERROR: nColorType != PNG_COLOR_TYPE_RGB_ALPHA\n\n");
			bResult = false;
			break;
		}
		u8* pData = new unsigned char[nPngWidth * nPngHeight * 4];
		png_bytepp pRowPointers = new png_bytep[nPngHeight];
		for (n32 j = 0; j < nPngHeight; j++)
		{
			pRowPointers[j] = pData + j * nPngWidth * 4;
		}
		png_read_image(png_ptr, pRowPointers);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		delete[] pRowPointers;
		fclose(fp);
		pvrtexture::CPVRTexture* pPVRTexture = nullptr;
		bool bSame = decode(pBin + pCtpkHeader->TextureOffset + pCtrTextureInfo[i].TexDataOffset, pCtrTextureInfo[i].Width, pCtrTextureInfo[i].Height, pCtrTextureInfo[i].TexFormat, &pPVRTexture) == 0 && memcmp(pPVRTexture->getDataPtr(), pData, pCtrTextureInfo[i].Width * pCtrTextureInfo[i].Height * 4) == 0;
		delete pPVRTexture;
		if (!bSame)
		{
			u8* pBuffer = nullptr;
			encode(pData, nPngWidth, nPngHeight, pCtrTextureInfo[i].TexFormat, pCtrTextureInfo[i].MipLevel, s_nBPP[pCtrTextureInfo[i].TexFormat], &pBuffer);
			memcpy(pBin + pCtpkHeader->TextureOffset + pCtrTextureInfo[i].TexDataOffset, pBuffer, pCtrTextureInfo[i].TexDataSize);
			delete[] pBuffer;
		}
		delete[] pData;
	}
	if (bResult)
	{
		m_fpCtpk = FFopen(m_pFileName, "wb");
		if (m_fpCtpk != nullptr)
		{
			fwrite(pBin, 1, static_cast<size_t>(nFileSize), m_fpCtpk);
			fclose(m_fpCtpk);
		}
		else
		{
			bResult = false;
		}
	}
	delete[] pBin;
	return bResult;
}

bool CCtpk::IsCtpkFile(const char* a_pFileName)
{
	FILE* fp = FFopen(a_pFileName, "rb");
	if (fp == nullptr)
	{
		return false;
	}
	SCtpkHeader ctpkHeader;
	fread(&ctpkHeader, sizeof(ctpkHeader), 1, fp);
	fclose(fp);
	return ctpkHeader.Signature == s_uSignature;
}

int CCtpk::decode(u8* a_pBuffer, n32 a_nWidth, n32 a_nHeight, n32 a_nFormat, pvrtexture::CPVRTexture** a_pPVRTexture)
{
	u8* pRGBA = nullptr;
	u8* pAlpha = nullptr;
	switch (a_nFormat)
	{
	case kTextureFormatRGBA8888:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight * 4];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					for (n32 k = 0; k < 4; k++)
					{
						pTemp[(i * 64 + j) * 4 + k] = a_pBuffer[(i * 64 + s_nDecodeTransByte[j]) * 4 + 3 - k];
					}
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 4];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					for (n32 k = 0; k < 4; k++)
					{
						pRGBA[(i * a_nWidth + j) * 4 + k] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 4 + k];
					}
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatRGB888:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight * 3];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					for (n32 k = 0; k < 3; k++)
					{
						pTemp[(i * 64 + j) * 3 + k] = a_pBuffer[(i * 64 + s_nDecodeTransByte[j]) * 3 + 2 - k];
					}
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 3];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					for (n32 k = 0; k < 3; k++)
					{
						pRGBA[(i * a_nWidth + j) * 3 + k] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 3 + k];
					}
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatRGBA5551:
	case kTextureFormatRGB565:
	case kTextureFormatRGBA4444:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight * 2];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					for (n32 k = 0; k < 2; k++)
					{
						pTemp[(i * 64 + j) * 2 + k] = a_pBuffer[(i * 64 + s_nDecodeTransByte[j]) * 2 + k];
					}
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 2];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					for (n32 k = 0; k < 2; k++)
					{
						pRGBA[(i * a_nWidth + j) * 2 + k] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 2 + k];
					}
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatLA88:
	case kTextureFormatHL8:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight * 2];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					for (n32 k = 0; k < 2; k++)
					{
						pTemp[(i * 64 + j) * 2 + k] = a_pBuffer[(i * 64 + s_nDecodeTransByte[j]) * 2 + 1 - k];
					}
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight * 2];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					for (n32 k = 0; k < 2; k++)
					{
						pRGBA[(i * a_nWidth + j) * 2 + k] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 2 + k];
					}
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatL8:
	case kTextureFormatA8:
	case kTextureFormatLA44:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j++)
				{
					pTemp[i * 64 + j] = a_pBuffer[i * 64 + s_nDecodeTransByte[j]];
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					pRGBA[i * a_nWidth + j] = pTemp[((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8];
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatL4:
	case kTextureFormatA4:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 64; i++)
			{
				for (n32 j = 0; j < 64; j += 2)
				{
					pTemp[i * 64 + j] = (a_pBuffer[i * 32 + s_nDecodeTransByte[j] / 2] & 0xF) * 0x11;
					pTemp[i * 64 + j + 1] = (a_pBuffer[i * 32 + s_nDecodeTransByte[j] / 2] >> 4 & 0xF) * 0x11;
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					pRGBA[i * a_nWidth + j] = pTemp[((i / 8) * (a_nWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8];
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatETC1:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight / 2];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 2 / 8; i++)
			{
				for (n32 j = 0; j < 8; j++)
				{
					pTemp[i * 8 + j] = a_pBuffer[i * 8 + 7 - j];
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight / 2];
			for (n32 i = 0; i < a_nHeight; i += 4)
			{
				for (n32 j = 0; j < a_nWidth; j += 4)
				{
					memcpy(pRGBA + ((i / 4) * (a_nWidth / 4) + j / 4) * 8, pTemp + (((i / 8) * (a_nWidth / 8) + j / 8) * 4 + (i % 8 / 4 * 2 + j % 8 / 4)) * 8, 8);
				}
			}
			delete[] pTemp;
		}
		break;
	case kTextureFormatETC1_A4:
		{
			u8* pTemp = new u8[a_nWidth * a_nHeight / 2];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 2 / 8; i++)
			{
				for (n32 j = 0; j < 8; j++)
				{
					pTemp[i * 8 + j] = a_pBuffer[8 + i * 16 + 7 - j];
				}
			}
			pRGBA = new u8[a_nWidth * a_nHeight / 2];
			for (n32 i = 0; i < a_nHeight; i += 4)
			{
				for (n32 j = 0; j < a_nWidth; j += 4)
				{
					memcpy(pRGBA + ((i / 4) * (a_nWidth / 4) + j / 4) * 8, pTemp + (((i / 8) * (a_nWidth / 8) + j / 8) * 4 + (i % 8 / 4 * 2 + j % 8 / 4)) * 8, 8);
				}
			}
			delete[] pTemp;
			pTemp = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nWidth * a_nHeight / 16; i++)
			{
				for (n32 j = 0; j < 4; j++)
				{
					pTemp[i * 16 + j] = (a_pBuffer[i * 16 + j * 2] & 0x0F) * 0x11;
					pTemp[i * 16 + j + 4] = (a_pBuffer[i * 16 + j * 2] >> 4 & 0x0F) * 0x11;
					pTemp[i * 16 + j + 8] = (a_pBuffer[i * 16 + j * 2 + 1] & 0x0F) * 0x11;
					pTemp[i * 16 + j + 12] = (a_pBuffer[i * 16 + j * 2 + 1] >> 4 & 0x0F) * 0x11;
				}
			}
			pAlpha = new u8[a_nWidth * a_nHeight];
			for (n32 i = 0; i < a_nHeight; i++)
			{
				for (n32 j = 0; j < a_nWidth; j++)
				{
					pAlpha[i * a_nWidth + j] = pTemp[(((i / 8) * (a_nWidth / 8) + j / 8) * 4 + i % 8 / 4 * 2 + j % 8 / 4) * 16 + i % 4 * 4 + j % 4];
				}
			}
			delete[] pTemp;
		}
		break;
	}
	PVRTextureHeaderV3 pvrTextureHeaderV3;
	MetaDataBlock metaDataBlock;
	switch (a_nFormat)
	{
	case kTextureFormatRGBA8888:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 8, 8, 8, 8).PixelTypeID;
		break;
	case kTextureFormatRGB888:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 0, 8, 8, 8, 0).PixelTypeID;
		break;
	case kTextureFormatRGBA5551:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 5, 5, 5, 1).PixelTypeID;
		break;
	case kTextureFormatRGB565:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 0, 5, 6, 5, 0).PixelTypeID;
		break;
	case kTextureFormatRGBA4444:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 4, 4, 4, 4).PixelTypeID;
		break;
	case kTextureFormatLA88:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 8, 8, 0, 0).PixelTypeID;
		break;
	case kTextureFormatHL8:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('r', 'g', 0, 0, 8, 8, 0, 0).PixelTypeID;
		break;
	case kTextureFormatL8:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('l', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatA8:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatLA44:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 4, 4, 0, 0).PixelTypeID;
		break;
	case kTextureFormatL4:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('l', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatA4:
		pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatETC1:
		pvrTextureHeaderV3.u64PixelFormat = ePVRTPF_ETC1;
		break;
	case kTextureFormatETC1_A4:
		pvrTextureHeaderV3.u64PixelFormat = ePVRTPF_ETC1;
		break;
	}
	pvrTextureHeaderV3.u32Height = a_nHeight;
	pvrTextureHeaderV3.u32Width = a_nWidth;
	metaDataBlock.DevFOURCC = PVRTEX3_IDENT;
	metaDataBlock.u32Key = ePVRTMetaDataTextureOrientation;
	metaDataBlock.u32DataSize = 3;
	metaDataBlock.Data = new PVRTuint8[metaDataBlock.u32DataSize];
	metaDataBlock.Data[0] = ePVRTOrientRight;
	metaDataBlock.Data[1] = ePVRTOrientUp;
	metaDataBlock.Data[2] = ePVRTOrientIn;
	pvrtexture::CPVRTextureHeader pvrTextureHeader(pvrTextureHeaderV3, 1, &metaDataBlock);
	*a_pPVRTexture = new pvrtexture::CPVRTexture(pvrTextureHeader, pRGBA);
	delete[] pRGBA;
	pvrtexture::Transcode(**a_pPVRTexture, pvrtexture::PVRStandard8PixelType, ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB);
	if (a_nFormat == kTextureFormatETC1_A4)
	{
		u8* data = static_cast<u8*>((*a_pPVRTexture)->getDataPtr());
		for (n32 i = 0; i < a_nHeight; i++)
		{
			for (n32 j = 0; j < a_nWidth; j++)
			{
				data[(i * a_nWidth + j) * 4 + 3] = pAlpha[i * a_nWidth + j];
			}
		}
		delete[] pAlpha;
	}
	return 0;
}

void CCtpk::encode(u8* a_pData, n32 a_nWidth, n32 a_nHeight, n32 a_nFormat, n32 a_nMipmapLevel, n32 a_nBPP, u8** a_pBuffer)
{
	PVRTextureHeaderV3 pvrTextureHeaderV3;
	pvrTextureHeaderV3.u64PixelFormat = pvrtexture::PVRStandard8PixelType.PixelTypeID;
	pvrTextureHeaderV3.u32Height = a_nHeight;
	pvrTextureHeaderV3.u32Width = a_nWidth;
	MetaDataBlock metaDataBlock;
	metaDataBlock.DevFOURCC = PVRTEX3_IDENT;
	metaDataBlock.u32Key = ePVRTMetaDataTextureOrientation;
	metaDataBlock.u32DataSize = 3;
	metaDataBlock.Data = new PVRTuint8[metaDataBlock.u32DataSize];
	metaDataBlock.Data[0] = ePVRTOrientRight;
	metaDataBlock.Data[1] = ePVRTOrientUp;
	metaDataBlock.Data[2] = ePVRTOrientIn;
	pvrtexture::CPVRTextureHeader pvrTextureHeader(pvrTextureHeaderV3, 1, &metaDataBlock);
	pvrtexture::CPVRTexture* pPVRTexture = nullptr;
	pvrtexture::CPVRTexture* pPVRTextureAlpha = nullptr;
	if (a_nFormat != kTextureFormatETC1_A4)
	{
		pPVRTexture = new pvrtexture::CPVRTexture(pvrTextureHeader, a_pData);
	}
	else
	{
		u8* pRGBAData = new u8[a_nWidth * a_nHeight * 4];
		memcpy(pRGBAData, a_pData, a_nWidth * a_nHeight * 4);
		u8* pAlphaData = new u8[a_nWidth * a_nHeight * 4];
		memcpy(pAlphaData, a_pData, a_nWidth * a_nHeight * 4);
		for (n32 i = 0; i < a_nWidth * a_nHeight; i++)
		{
			pRGBAData[i * 4 + 3] = 0xFF;
			pAlphaData[i * 4] = 0;
			pAlphaData[i * 4 + 1] = 0;
			pAlphaData[i * 4 + 2] = 0;
		}
		pPVRTexture = new pvrtexture::CPVRTexture(pvrTextureHeader, pRGBAData);
		pPVRTextureAlpha = new pvrtexture::CPVRTexture(pvrTextureHeader, pAlphaData);
		delete[] pRGBAData;
		delete[] pAlphaData;
	}
	if (a_nMipmapLevel != 1)
	{
		pvrtexture::GenerateMIPMaps(*pPVRTexture, pvrtexture::eResizeNearest, a_nMipmapLevel);
		if (a_nFormat == kTextureFormatETC1_A4)
		{
			pvrtexture::GenerateMIPMaps(*pPVRTextureAlpha, pvrtexture::eResizeNearest, a_nMipmapLevel);
		}
	}
	pvrtexture::uint64 uPixelFormat = 0;
	pvrtexture::ECompressorQuality eCompressorQuality = pvrtexture::ePVRTCBest;
	switch (a_nFormat)
	{
	case kTextureFormatRGBA8888:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 8, 8, 8, 8).PixelTypeID;
		break;
	case kTextureFormatRGB888:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 0, 8, 8, 8, 0).PixelTypeID;
		break;
	case kTextureFormatRGBA5551:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 5, 5, 5, 1).PixelTypeID;
		break;
	case kTextureFormatRGB565:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 0, 5, 6, 5, 0).PixelTypeID;
		break;
	case kTextureFormatRGBA4444:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 'b', 'a', 4, 4, 4, 4).PixelTypeID;
		break;
	case kTextureFormatLA88:
		uPixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 8, 8, 0, 0).PixelTypeID;
		break;
	case kTextureFormatHL8:
		uPixelFormat = pvrtexture::PixelType('r', 'g', 0, 0, 8, 8, 0, 0).PixelTypeID;
		break;
	case kTextureFormatL8:
		uPixelFormat = pvrtexture::PixelType('l', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatA8:
		uPixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatLA44:
		uPixelFormat = pvrtexture::PixelType('l', 'a', 0, 0, 4, 4, 0, 0).PixelTypeID;
		break;
	case kTextureFormatL4:
		uPixelFormat = pvrtexture::PixelType('l', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatA4:
		uPixelFormat = pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID;
		break;
	case kTextureFormatETC1:
		uPixelFormat = ePVRTPF_ETC1;
		eCompressorQuality = pvrtexture::eETCSlowPerceptual;
		break;
	case kTextureFormatETC1_A4:
		uPixelFormat = ePVRTPF_ETC1;
		eCompressorQuality = pvrtexture::eETCSlowPerceptual;
		break;
	}
	pvrtexture::Transcode(*pPVRTexture, uPixelFormat, ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, eCompressorQuality);
	if (a_nFormat == kTextureFormatETC1_A4)
	{
		pvrtexture::Transcode(*pPVRTextureAlpha, pvrtexture::PixelType('a', 0, 0, 0, 8, 0, 0, 0).PixelTypeID, ePVRTVarTypeUnsignedByteNorm, ePVRTCSpacelRGB, pvrtexture::ePVRTCBest);
	}
	n32 nTotalSize = 0;
	n32 nCurrentSize = 0;
	for (n32 l = 0; l < a_nMipmapLevel; l++)
	{
		nTotalSize += (a_nWidth >> l) * (a_nHeight >> l) * a_nBPP / 8;
	}
	*a_pBuffer = new u8[nTotalSize];
	for (n32 l = 0; l < a_nMipmapLevel; l++)
	{
		n32 mipmapWidth = a_nWidth >> l;
		n32 mipmapHeight = a_nHeight >> l;
		u8* pRGBA = static_cast<u8*>(pPVRTexture->getDataPtr(l));
		u8* pAlpha = nullptr;
		if (a_nFormat == kTextureFormatETC1_A4)
		{
			pAlpha = static_cast<u8*>(pPVRTextureAlpha->getDataPtr(l));
		}
		switch (a_nFormat)
		{
		case kTextureFormatRGBA8888:
			{
				u8* pTemp = new u8[mipmapWidth * mipmapHeight * 4];
				for (n32 i = 0; i < mipmapHeight; i++)
				{
					for (n32 j = 0; j < mipmapWidth; j++)
					{
						for (n32 k = 0; k < 4; k++)
						{
							pTemp[(((i / 8) * (mipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 4 + k] = pRGBA[(i * mipmapWidth + j) * 4 + k];
						}
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < mipmapWidth * mipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						for (n32 k = 0; k < 4; k++)
						{
							pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 4 + 3 - k] = pTemp[(i * 64 + j) * 4 + k];
						}
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatRGB888:
			{
				u8* pTemp = new u8[mipmapWidth * mipmapHeight * 3];
				for (n32 i = 0; i < mipmapHeight; i++)
				{
					for (n32 j = 0; j < mipmapWidth; j++)
					{
						for (n32 k = 0; k < 3; k++)
						{
							pTemp[(((i / 8) * (mipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 3 + k] = pRGBA[(i * mipmapWidth + j) * 3 + k];
						}
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < mipmapWidth * mipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						for (n32 k = 0; k < 3; k++)
						{
							pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 3 + 2 - k] = pTemp[(i * 64 + j) * 3 + k];
						}
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatRGBA5551:
		case kTextureFormatRGB565:
		case kTextureFormatRGBA4444:
			{
				u8* pTemp = new u8[mipmapWidth * mipmapHeight * 2];
				for (n32 i = 0; i < mipmapHeight; i++)
				{
					for (n32 j = 0; j < mipmapWidth; j++)
					{
						for (n32 k = 0; k < 2; k++)
						{
							pTemp[(((i / 8) * (mipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 2 + k] = pRGBA[(i * mipmapWidth + j) * 2 + k];
						}
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < mipmapWidth * mipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						for (n32 k = 0; k < 2; k++)
						{
							pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 2 + k] = pTemp[(i * 64 + j) * 2 + k];
						}
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatLA88:
		case kTextureFormatHL8:
			{
				u8* pTemp = new u8[mipmapWidth * mipmapHeight * 2];
				for (n32 i = 0; i < mipmapHeight; i++)
				{
					for (n32 j = 0; j < mipmapWidth; j++)
					{
						for (n32 k = 0; k < 2; k++)
						{
							pTemp[(((i / 8) * (mipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8) * 2 + k] = pRGBA[(i * mipmapWidth + j) * 2 + k];
						}
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < mipmapWidth * mipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						for (n32 k = 0; k < 2; k++)
						{
							pMipmapBuffer[(i * 64 + s_nDecodeTransByte[j]) * 2 + 1 - k] = pTemp[(i * 64 + j) * 2 + k];
						}
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatL8:
		case kTextureFormatA8:
		case kTextureFormatLA44:
			{
				u8* pTemp = new u8[mipmapWidth * mipmapHeight];
				for (n32 i = 0; i < mipmapHeight; i++)
				{
					for (n32 j = 0; j < mipmapWidth; j++)
					{
						pTemp[((i / 8) * (mipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8] = pRGBA[i * mipmapWidth + j];
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < mipmapWidth * mipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j++)
					{
						pMipmapBuffer[i * 64 + s_nDecodeTransByte[j]] = pTemp[i * 64 + j];
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatL4:
		case kTextureFormatA4:
			{
				u8* pTemp = new u8[mipmapWidth * mipmapHeight];
				for (n32 i = 0; i < mipmapHeight; i++)
				{
					for (n32 j = 0; j < mipmapWidth; j++)
					{
						pTemp[((i / 8) * (mipmapWidth / 8) + j / 8) * 64 + i % 8 * 8 + j % 8] = pRGBA[i * mipmapWidth + j];
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < mipmapWidth * mipmapHeight / 64; i++)
				{
					for (n32 j = 0; j < 64; j += 2)
					{
						pMipmapBuffer[i * 32 + s_nDecodeTransByte[j] / 2] = ((pTemp[i * 64 + j] / 0x11) & 0x0F) | ((pTemp[i * 64 + j + 1] / 0x11) << 4 & 0xF0);
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatETC1:
			{
				u8* pTemp = new u8[mipmapWidth * mipmapHeight / 2];
				for (n32 i = 0; i < mipmapHeight; i += 4)
				{
					for (n32 j = 0; j < mipmapWidth; j += 4)
					{
						memcpy(pTemp + (((i / 8) * (mipmapWidth / 8) + j / 8) * 4 + (i % 8 / 4 * 2 + j % 8 / 4)) * 8, pRGBA + ((i / 4) * (mipmapWidth / 4) + j / 4) * 8, 8);
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < mipmapWidth * mipmapHeight / 2 / 8; i++)
				{
					for (n32 j = 0; j < 8; j++)
					{
						pMipmapBuffer[i * 8 + 7 - j] = pTemp[i * 8 + j];
					}
				}
				delete[] pTemp;
			}
			break;
		case kTextureFormatETC1_A4:
			{
				u8* pTemp = new u8[mipmapWidth * mipmapHeight / 2];
				for (n32 i = 0; i < mipmapHeight; i += 4)
				{
					for (n32 j = 0; j < mipmapWidth; j += 4)
					{
						memcpy(pTemp + (((i / 8) * (mipmapWidth / 8) + j / 8) * 4 + (i % 8 / 4 * 2 + j % 8 / 4)) * 8, pRGBA + ((i / 4) * (mipmapWidth / 4) + j / 4) * 8, 8);
					}
				}
				u8* pMipmapBuffer = *a_pBuffer + nCurrentSize;
				for (n32 i = 0; i < mipmapWidth * mipmapHeight / 2 / 8; i++)
				{
					for (n32 j = 0; j < 8; j++)
					{
						pMipmapBuffer[8 + i * 16 + 7 - j] = pTemp[i * 8 + j];
					}
				}
				delete[] pTemp;
				pTemp = new u8[mipmapWidth * mipmapHeight];
				for (n32 i = 0; i < mipmapHeight; i++)
				{
					for (n32 j = 0; j < mipmapWidth; j++)
					{
						pTemp[(((i / 8) * (mipmapWidth / 8) + j / 8) * 4 + i % 8 / 4 * 2 + j % 8 / 4) * 16 + i % 4 * 4 + j % 4] = pAlpha[i * mipmapWidth + j];
					}
				}
				for (n32 i = 0; i < mipmapWidth * mipmapHeight / 16; i++)
				{
					for (n32 j = 0; j < 4; j++)
					{
						pMipmapBuffer[i * 16 + j * 2] = ((pTemp[i * 16 + j] / 0x11) & 0x0F) | ((pTemp[i * 16 + j + 4] / 0x11) << 4 & 0xF0);
						pMipmapBuffer[i * 16 + j * 2 + 1] = ((pTemp[i * 16 + j + 8] / 0x11) & 0x0F) | ((pTemp[i * 16 + j + 12] / 0x11) << 4 & 0xF0);
					}
				}
				delete[] pTemp;
			}
			break;
		}
		nCurrentSize += mipmapWidth * mipmapHeight * a_nBPP / 8;
	}
	delete pPVRTexture;
	if (a_nFormat == kTextureFormatETC1_A4)
	{
		delete pPVRTextureAlpha;
	}
}
