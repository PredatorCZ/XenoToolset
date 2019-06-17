/*  casmExtract
	Copyright(C) 2019 Lukas Cone

	This program is free software : you can redistribute it and / or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.If not, see <https://www.gnu.org/licenses/>.
*/

#include "XenoLibAPI.h"
#include "../source/MXMD_V1.h"
#include "datas/binreader.hpp"
#include "datas/fileinfo.hpp"
#include "datas/MultiThread.hpp"
#include "datas/esstring.h"
#include "datas/masterprinter.hpp"
#include <tchar.h>

static const char help[] = "Usage: casmExtract [options] <casmhd file>\n\
casmhd file can be also drag'n'dropped onto application.\n\n\
Options:\n\
-u	Exported textures will be converted into PNG format, rather than DDS.\n\
-b	Will generate blue channel for some formats used for normal maps.\n\
-h	Will show this help message.\n\
-?	Same as -h command.";

static const char pressKeyCont[] = "\nPress ENTER to close.";

static TextureConversionParams texParams = {};

bool CreateFile(const TSTRING &fileName, std::ofstream &ofs)
{
	ofs.open(fileName, std::ios_base::out | std::ios_base::binary);

	if (ofs.fail())
	{
		printerror("Couldn't create file: ", << fileName);
		return false;
	}

	return true;
}

struct EmbededHKX
{
	float ufloat[13];
	int offset,
		size,
		unk00[3],
		nameOffset,
		unk01[3];

	ES_FORCEINLINE void SwapEndian() { _ArraySwap<int>(*this); }
};

struct SkyboxModel
{
	float ufloat[13];
	int offset,
		size;

	ES_FORCEINLINE void SwapEndian() { _ArraySwap<int>(*this); }
};

struct TerrainLODModel
{
	float unk00[10];
	int offset,
		size,
		unk01[2];
	float unk02[4];

	ES_FORCEINLINE void SwapEndian() { _ArraySwap<int>(*this); }
};

struct DataFile
{
	int offset,
		size;

	ES_FORCEINLINE void SwapEndian() { _ArraySwap<int>(*this); }
};

struct ObjectTextureFile
{
	int midMapOffset,
		midMapSize,
		nearMapOffset,
		nearMapSize,
		unk;

	ES_FORCEINLINE void SwapEndian() { _ArraySwap<int>(*this); }
};

struct TerrainModel
{
	float unk00[9];
	int unk01,
		unk02;
	float unk03[2];
	int offset,
		size;
	float unk04[4];

	ES_FORCEINLINE void SwapEndian() { _ArraySwap<int>(*this); }
};

struct ObjectModel
{
	float unk00[13];
	int offset,
		size,
		unk01;

	ES_FORCEINLINE void SwapEndian() { _ArraySwap<int>(*this); }
};

struct TGLDEntry
{
	float unk00[6];
	int offset,
		size,
		unk01[6];

	ES_FORCEINLINE void SwapEndian() { _ArraySwap<int>(*this); }
};

struct DMSM
{
	static const int ID = CompileFourCC("MSMD");

	int magic,
		version,
		null00[4],
		terrainModelsCount,
		terrainModelsOffset,
		objectModelsCount,
		objectModelsOffset,
		havokColCount,
		havokColOffset,
		skyboxModelsCount,
		skyboxModelsOffset,
		null01[6],
		mapObjectBuffersCount,
		mapObjectBuffersOffset,
		objectTexturesCount,
		objectTexturesOffset,
		havokNamesOffset,

		Grass_Count,
		Grass_Offset,
		itemcount3,
		itemsoffset3,
		itemcount4,
		itemsoffset4,

		TGLDNamesCount,
		TGLDNamesOffset,
		TGLDInternalOffset,
		TGLDCount,
		TGLDOffset,
		terrainCachedTexturesCount,
		terrainCachedTexturesOffset,
		terrainTexturesCount,
		terrainTexturesOffset,

		bvsc_offset,
		null_offset,

		LCMDOffset,
		LCMDSize,
		EFBCount,
		EFBOffset,

		terrainLODsCount,
		terrainLODsOffset,

		null02,
		itemcount5,
		itemsoffset5,

		mapTerrainBuffersCount,
		mapTerrainBuffersOffset,
		CEMSOffset;

	ES_FORCEINLINE char *GetMe() { return reinterpret_cast<char *>(this); }
	ES_FORCEINLINE EmbededHKX *GetCollisions() { return reinterpret_cast<EmbededHKX *>(GetMe() + havokColOffset); }
	ES_FORCEINLINE const char *GetCollisionName(EmbededHKX *ehkx) { return GetMe() + havokNamesOffset + ehkx->nameOffset; }
	ES_FORCEINLINE SkyboxModel *GetSkyboxModels() { return reinterpret_cast<SkyboxModel *>(GetMe() + skyboxModelsOffset); }
	ES_FORCEINLINE TerrainLODModel *GetTerrainLODs() { return reinterpret_cast<TerrainLODModel *>(GetMe() + terrainLODsOffset); }
	ES_FORCEINLINE DataFile *GetTerrainTextures() { return reinterpret_cast<DataFile *>(GetMe() + terrainTexturesOffset); }
	ES_FORCEINLINE DataFile *GetTerrainCachedTextures() { return reinterpret_cast<DataFile *>(GetMe() + terrainCachedTexturesOffset); }
	ES_FORCEINLINE ObjectTextureFile *GetObjectTextures() { return reinterpret_cast<ObjectTextureFile *>(GetMe() + objectTexturesOffset); }
	ES_FORCEINLINE TerrainModel *GetTerrainModels() { return reinterpret_cast<TerrainModel *>(GetMe() + terrainModelsOffset); }
	ES_FORCEINLINE ObjectModel *GetObjectModels() { return reinterpret_cast<ObjectModel *>(GetMe() + objectModelsOffset); }
	ES_FORCEINLINE DataFile *GetObjectBuffers() { return reinterpret_cast<DataFile *>(GetMe() + mapObjectBuffersOffset); }
	ES_FORCEINLINE DataFile *GetTerrainBuffers() { return reinterpret_cast<DataFile *>(GetMe() + mapTerrainBuffersOffset); }
	ES_FORCEINLINE TGLDEntry *GetTGLD() { return reinterpret_cast<TGLDEntry *>(GetMe() + TGLDOffset); }
	ES_FORCEINLINE int *GetTGLDNameOffsets() { return reinterpret_cast<int *>(GetMe() + TGLDNamesOffset); }
	ES_FORCEINLINE const char *GetTGLDName(int id) { return GetMe() + GetTGLDNameOffsets()[id]; }
	ES_FORCEINLINE DataFile *GetEffectFiles() { return reinterpret_cast<DataFile *>(GetMe() + EFBOffset); }

	ES_FORCEINLINE char *GetCEMS() { return GetMe() + CEMSOffset; }
	ES_FORCEINLINE int CEMSSize() { return bvsc_offset - CEMSOffset; }
	ES_FORCEINLINE char *GetLCMD() { return GetMe() + LCMDOffset; }
	ES_FORCEINLINE char *GetMainTGLD() { return GetMe() + TGLDInternalOffset; }
	ES_FORCEINLINE int GetMainTGLDSize() { return CEMSOffset - TGLDInternalOffset; }

	ES_FORCEINLINE void SwapEndian()
	{
		_ArraySwap<int>(*this);
		EmbededHKX *colls = GetCollisions();

		for (int c = 0; c < havokColCount; c++)
			colls[c].SwapEndian();

		SkyboxModel *skyModels = GetSkyboxModels();

		for (int c = 0; c < skyboxModelsCount; c++)
			skyModels[c].SwapEndian();

		TerrainLODModel *waterModels = GetTerrainLODs();

		for (int c = 0; c < terrainLODsCount; c++)
			waterModels[c].SwapEndian();

		DataFile *cData = GetTerrainTextures();

		for (int c = 0; c < terrainTexturesCount; c++)
			cData[c].SwapEndian();

		cData = GetTerrainCachedTextures();

		for (int c = 0; c < terrainCachedTexturesCount; c++)
			cData[c].SwapEndian();

		ObjectTextureFile *texs = GetObjectTextures();

		for (int c = 0; c < objectTexturesCount; c++)
			texs[c].SwapEndian();

		TerrainModel *mapModels01 = GetTerrainModels();

		for (int c = 0; c < terrainModelsCount; c++)
			mapModels01[c].SwapEndian();

		ObjectModel *mapModels02 = GetObjectModels();

		for (int c = 0; c < objectModelsCount; c++)
			mapModels02[c].SwapEndian();

		cData = GetObjectBuffers();

		for (int c = 0; c < mapObjectBuffersCount; c++)
			cData[c].SwapEndian();

		cData = GetTerrainBuffers();

		for (int c = 0; c < mapTerrainBuffersCount; c++)
			cData[c].SwapEndian();

		TGLDEntry *tgldEntries = GetTGLD();

		for (int c = 0; c < TGLDCount; c++)
			tgldEntries[c].SwapEndian();

		int *nameOffsets = GetTGLDNameOffsets();

		for (int c = 0; c < TGLDNamesCount; c++)
			FByteswapper(nameOffsets[c]);

		cData = GetEffectFiles();

		for (int c = 0; c < EFBCount; c++)
			cData[c].SwapEndian();
	}

};

struct ExternalDataItem
{
	char *buffer;
	int size;
};

struct mtxtQueue
{
	int queue;
	int queueEnd;
	std::vector<ExternalDataItem> *offsets;
	const TCHAR *folder;

	typedef void return_type;

	mtxtQueue() : queue(0) {}

	return_type RetreiveItem()
	{
		TSTRING texName = folder;

		if (queue < 1000)
			texName.push_back('0');
		if (queue < 100)
			texName.push_back('0');
		if (queue < 10)
			texName.push_back('0');

		texName.append(ToTSTRING(queue));

		ConvertMTXT(offsets->at(queue).buffer, offsets->at(queue).size, texName.c_str(), texParams);
	}

	operator bool() { return queue < queueEnd; }
	void operator++(int) { queue++; }
	int NumQueues() const { return queueEnd; }
};

struct TerrainTextureHeader
{
	int numTextures,
		unk00[7];

	struct
	{
		int size,
			offset,
			uncachedID,
			unk00;
	}entries[254];

	ES_FORCEINLINE void SwapEndian() { _ArraySwap<int>(*this); }
};

void ExtractCachedTextures(DataFile *data, DataFile *uncachedData, int count, const TSTRING &outFolder, BinReader *dataFile)
{
	int totalBufferSize = 0;

	for (int i = 0; i < count; i++)
	{
		TerrainTextureHeader cHdr = {};
		DataFile &cData = data[i];
		dataFile->Seek(cData.offset);
		dataFile->Read(cHdr);
		int localTotalSize = 0;

		for (int e = 0; e < cHdr.numTextures; e++)
		{
			if (cHdr.entries[e].uncachedID < 0)
				localTotalSize += cHdr.entries[e].size;
			else
				localTotalSize += uncachedData[cHdr.entries[e].uncachedID].size;
		}

		if (localTotalSize > totalBufferSize)
			totalBufferSize = localTotalSize;
	}

	char *dataBuffer = static_cast<char *>(malloc(totalBufferSize));

	for (int i = 0; i < count; i++)
	{
		TerrainTextureHeader cHdr = {};
		DataFile &cData = data[i];
		dataFile->Seek(cData.offset);
		dataFile->Read(cHdr);

		std::vector<ExternalDataItem> offsets(cHdr.numTextures);
		char *dataIter = dataBuffer;

		for (int e = 0; e < cHdr.numTextures; e++)
		{
			if (cHdr.entries[e].uncachedID < 0)
			{
				const int dataSize = cHdr.entries[e].size;
				dataFile->Seek(cData.offset + cHdr.entries[e].offset);
				dataFile->ReadBuffer(dataIter, dataSize);
				offsets[e].buffer = dataIter;
				offsets[e].size = dataSize;
				dataIter += dataSize;
			}
			else
			{
				const int dataSize = uncachedData[cHdr.entries[e].uncachedID].size;
				dataFile->Seek(uncachedData[cHdr.entries[e].uncachedID].offset);
				dataFile->ReadBuffer(dataIter, dataSize);
				offsets[e].buffer = dataIter;
				offsets[e].size = dataSize;
				dataIter += dataSize;
			}
		}

		TSTRING outFolderTex = outFolder + ToTSTRING(i) + _T("/");
		_tmkdir(outFolderTex.c_str());

		mtxtQueue texQue;
		texQue.offsets = &offsets;
		texQue.folder = outFolderTex.c_str();
		texQue.queueEnd = cHdr.numTextures;

		RunThreadedQueue(texQue);
	}

	free(dataBuffer);
}

void ExtractUncachedTextures(ObjectTextureFile *data, int count, const TSTRING &outFolder, BinReader *dataFile)
{
	int totalBufferSize = 0;

	for (int i = 0; i < count; i++)
		totalBufferSize += data[i].nearMapSize ? data[i].nearMapSize : data[i].midMapSize;

	char *dataBuffer = static_cast<char *>(malloc(totalBufferSize));
	char *dataIter = dataBuffer;
	std::vector<ExternalDataItem> offsets(count);

	for (int i = 0; i < count; i++)
	{
		ObjectTextureFile &cData = data[i];

		if (data[i].nearMapSize)
		{
			dataFile->Seek(cData.nearMapOffset);
			dataFile->ReadBuffer(dataIter, cData.nearMapSize);
			offsets[i].buffer = dataIter;
			offsets[i].size = cData.nearMapSize;
			dataIter += cData.nearMapSize;
		}
		else
		{
			dataFile->Seek(cData.midMapOffset);
			dataFile->ReadBuffer(dataIter, cData.midMapSize);
			offsets[i].buffer = dataIter;
			offsets[i].size = cData.midMapSize;
			dataIter += cData.midMapSize;
		}
	}

	mtxtQueue texQue;
	texQue.offsets = &offsets;
	texQue.folder = outFolder.c_str();
	texQue.queueEnd = count;

	RunThreadedQueue(texQue);

	free(dataBuffer);
}

void ExtractCollision(DMSM *dmsm, const TSTRING &outFolder, BinReader *dataFile)
{
	EmbededHKX *data = dmsm->GetCollisions();
	int biggestSize = 0;

	for (int i = 0; i < dmsm->havokColCount; i++)
		if (data[i].size > biggestSize)
			biggestSize = data[i].size;

	char *dataBuffer = static_cast<char *>(malloc(biggestSize));
	std::ofstream ofs;

	for (int i = 0; i < dmsm->havokColCount; i++)
	{
		dataFile->Seek(data[i].offset);
		dataFile->ReadBuffer(dataBuffer, data[i].size);

		if (CreateFile(outFolder + esStringConvert<TCHAR>(dmsm->GetCollisionName(data + i)) + _T("hkx"), ofs))
			ofs.write(dataBuffer, data[i].size);

		ofs.close();
	}

	free(dataBuffer);
}

struct SkyBoxHeader
{
	int modelsOffset,
		materialsOffset,
		unkOffset0,
		vertexBufferOffset,
		cachedTexturesOffset,
		null00,
		shadersOffset,
		null01[9];

	ES_FORCEINLINE void SwapEndian() { _ArraySwap<int>(*this); }
};

void ExtractSkyboxes(SkyboxModel *data, int count, const TSTRING &outFolder, BinReader *dataFile)
{
	int biggestSize = 0;

	for (int i = 0; i < count; i++)
		if (data[i].size > biggestSize)
			biggestSize = data[i].size;

	char *dataBuffer = static_cast<char *>(malloc(biggestSize));
	std::ofstream ofs;

	for (int i = 0; i < count; i++)
	{
		dataFile->Seek(data[i].offset);
		dataFile->ReadBuffer(dataBuffer, data[i].size);

		MXMDHeader out = {};
		SkyBoxHeader *hdr = reinterpret_cast<SkyBoxHeader *>(dataBuffer);

		hdr->SwapEndian();

		out.magic = CompileFourCC("DMXM");
		out.version = 10040;
		out.modelsOffset = hdr->modelsOffset + 8;
		out.materialsOffset = hdr->materialsOffset + 8;
		out.unkOffset0 = hdr->unkOffset0 + 8;
		out.vertexBufferOffset = hdr->vertexBufferOffset + 8;
		out.cachedTexturesOffset = hdr->cachedTexturesOffset + 8;
		out.shadersOffset = hdr->shadersOffset + 8;
		out.SwapEndian();

		if (CreateFile(outFolder + _T("Skybox") + ToTSTRING(i) + _T(".camdo"), ofs))
		{
			ofs.write(reinterpret_cast<char *>(&out), sizeof(MXMDHeader));
			ofs.write(dataBuffer + sizeof(SkyBoxHeader), data[i].size - sizeof(SkyBoxHeader));
			ofs.close();
		}
	}

	free(dataBuffer);
}

struct TerrainLODHeader
{
	int null[2],
		modelsOffset,
		materialsOffset,
		unkOffset0,
		null00,
		vertexBufferOffset,
		cachedTexturesOffset,
		shadersOffset,
		indicesOffset,
		indicesCount,
		null01[7];

	ES_FORCEINLINE void SwapEndian() { _ArraySwap<int>(*this); }
};

void ExtractTerrainLODs(TerrainLODModel *data, int count, const TSTRING &outFolder, BinReader *dataFile)
{
	int biggestSize = 0;

	for (int i = 0; i < count; i++)
		if (data[i].size > biggestSize)
			biggestSize = data[i].size;

	char *dataBuffer = static_cast<char *>(malloc(biggestSize));
	std::ofstream ofs;

	for (int i = 0; i < count; i++)
	{
		dataFile->Seek(data[i].offset);
		dataFile->ReadBuffer(dataBuffer, data[i].size);

		MXMDHeader out = {};
		TerrainLODHeader *hdr = reinterpret_cast<TerrainLODHeader *>(dataBuffer);

		out.magic = CompileFourCC("MXMD");
		out.version = 10040;
		FByteswapper(out.version);
		out.modelsOffset = hdr->modelsOffset;
		out.materialsOffset = hdr->materialsOffset;
		out.unkOffset0 = hdr->unkOffset0;
		out.vertexBufferOffset = hdr->vertexBufferOffset;
		out.cachedTexturesOffset = hdr->cachedTexturesOffset;
		out.shadersOffset = hdr->shadersOffset;

		if (CreateFile(outFolder + ToTSTRING(i) + _T(".camdo"), ofs))
		{
			ofs.write(reinterpret_cast<char *>(&out), sizeof(MXMDHeader));
			ofs.write(dataBuffer + sizeof(TerrainLODHeader), data[i].size - sizeof(TerrainLODHeader));
			ofs.close();
		}
	}

	free(dataBuffer);
}

struct MapObjectModelHeader
{
	short unk00,
		unk01;
	int null02[2],
		modelsOffset,
		materialsOffset,
		unkOffset0,
		instancesOffset,
		unk03,
		externalTexturesOffset,
		externalTexturesCount,
		externalBufferIDsOffset,
		externalBufferIDsCount,
		unkoffsets01[5],
		shadersOffset,
		textureContainerLookupsOffset,
		textureContainerLookupsCount,
		unkoffsets02[6];

	ES_FORCEINLINE void SwapEndian() { _ArraySwap<int>(*this); }
};

struct MapObjectExternalTexture
{
	short textureID,
		containerID,
		externalTextureID,
		unk;

	ES_FORCEINLINE void SwapEndian() { _ArraySwap<short>(*this); }
};

void ExtractMapObjects(ObjectModel *data, DataFile *buffers, int count, const TSTRING &outFolder, BinReader *dataFile)
{
	int biggestSize = 0;

	for (int i = 0; i < count; i++)
		if (data[i].size > biggestSize)
			biggestSize = data[i].size;

	char *dataBuffer = static_cast<char *>(malloc(biggestSize));
	std::ofstream ofsmt;
	std::ofstream ofs;

	for (int i = 0; i < count; i++)
	{
		dataFile->Seek(data[i].offset);
		dataFile->ReadBuffer(dataBuffer, data[i].size);

		MXMDHeader out = {};
		MapObjectModelHeader *hdr = reinterpret_cast<MapObjectModelHeader *>(dataBuffer);
		hdr->SwapEndian();

		int *indices = reinterpret_cast<int *>(dataBuffer + hdr->externalBufferIDsOffset);
		biggestSize = 0;

		for (int i = 0; i < hdr->externalBufferIDsCount; i++)
		{
			FByteswapper(indices[i]);

			if (buffers[indices[i]].size > biggestSize)
				biggestSize = buffers[indices[i]].size;
		}

		out.magic = CompileFourCC("DMXM");
		out.version = 10040;
		out.modelsOffset = hdr->modelsOffset - 36;
		out.materialsOffset = hdr->materialsOffset - 36;
		out.shadersOffset = hdr->shadersOffset - 36;
		out.externalBufferIDsCount = hdr->externalBufferIDsCount;
		out.externalBufferIDsOffset = hdr->externalBufferIDsOffset - 36;
		out.externalTexturesCount = hdr->externalTexturesCount;
		out.externalTexturesOffset = hdr->externalTexturesOffset - 36;
		out.instancesOffset = hdr->instancesOffset - 36;
		out.unkOffset0 = hdr->unkOffset0 ? hdr->unkOffset0 - 36 : 0;

		out.SwapEndian();

		if (!CreateFile(outFolder + ToTSTRING(i) + _T(".casmt"), ofsmt))
			continue;

		std::vector<DataFile> externalDatas;
		char *masterBuffer = static_cast<char *>(malloc(biggestSize));

		for (int i = 0; i < hdr->externalBufferIDsCount; i++)
		{
			int &cIndex = indices[i];
			DataFile &cBuff = buffers[cIndex];
			bool found = false;

			for (auto &o : externalDatas)
				if (o.size == cIndex)
				{
					cIndex = o.offset;
					FByteswapper(cIndex);
					found = true;
				}

			if (found)
				continue;

			dataFile->Seek(cBuff.offset);
			dataFile->ReadBuffer(masterBuffer, cBuff.size);

			externalDatas.push_back({ static_cast<int>(ofsmt.tellp()), cIndex });

			cIndex = externalDatas.back().offset;
			FByteswapper(cIndex);

			ofsmt.write(masterBuffer, cBuff.size);
		}

		free(masterBuffer);
		ofsmt.close();

		short *containerLookups = reinterpret_cast<short *>(dataBuffer + hdr->textureContainerLookupsOffset);
		MapObjectExternalTexture *textures = reinterpret_cast<MapObjectExternalTexture *>(dataBuffer + hdr->externalTexturesOffset);

		for (int i = 0; i < hdr->externalTexturesCount; i++)
		{
			FByteswapper(textures[i].containerID);
			textures[i].containerID = containerLookups[textures[i].containerID];
		}

		if (CreateFile(outFolder + ToTSTRING(i) + _T(".camdo"), ofs))
		{
			ofs.write(reinterpret_cast<char *>(&out), sizeof(MXMDHeader));
			ofs.write(dataBuffer + sizeof(MapObjectModelHeader), data[i].size - sizeof(MapObjectModelHeader));
			ofs.close();
		}
	}

	free(dataBuffer);
}

struct MapTerrainHeader
{
	short unk00,
		unk01;
	int null02[2],
		modelsOffset,
		materialsOffset,
		unkOffset0,
		unkOffset00,
		externalTexturesOffset,
		externalTexturesCount,
		unkOffset02,
		unkOffset03,
		shadersOffset,
		textureContainerLookupsOffset,
		textureContainerLookupsCount,
		externalBufferIDsOffset,
		null00[7];

	ES_FORCEINLINE void SwapEndian() { _ArraySwap<int>(*this); }
};

void ExtractMapTerrain(TerrainModel *data, DataFile *buffers, int count, const TSTRING &outFolder, BinReader *dataFile)
{
	int biggestSize = 0;

	for (int i = 0; i < count; i++)
		if (data[i].size > biggestSize)
			biggestSize = data[i].size;

	char *dataBuffer = static_cast<char *>(malloc(biggestSize));
	std::ofstream ofsmt;
	std::ofstream ofs;

	for (int i = 0; i < count; i++)
	{
		dataFile->Seek(data[i].offset);
		dataFile->ReadBuffer(dataBuffer, data[i].size);

		MXMDHeader out = {};
		MapTerrainHeader *hdr = reinterpret_cast<MapTerrainHeader *>(dataBuffer);
		hdr->SwapEndian();

		MXMDTerrainBufferLookupHeader_V1 *lookups = reinterpret_cast<MXMDTerrainBufferLookupHeader_V1 *>(dataBuffer + hdr->externalBufferIDsOffset);
		lookups->SwapEndian();
		MXMDTerrainBufferLookup_V1 *bufferLookups = lookups->GetBufferLookups();
		biggestSize = 0;

		for (int i = 0; i < lookups->bufferLookupCount; i++)
		{
			if (buffers[bufferLookups[i].bufferIndex[0]].size > biggestSize)
				biggestSize = buffers[bufferLookups[i].bufferIndex[0]].size;

			if (buffers[bufferLookups[i].bufferIndex[1]].size > biggestSize)
				biggestSize = buffers[bufferLookups[i].bufferIndex[1]].size;
		}

		out.magic = CompileFourCC("DMXM");
		out.version = 10040;
		out.modelsOffset = hdr->modelsOffset - 20;
		out.materialsOffset = hdr->materialsOffset - 20;
		out.shadersOffset = hdr->shadersOffset - 20;
		out.externalBufferIDsOffset = hdr->externalBufferIDsOffset - 20;
		out.externalBufferIDsCount = -1;
		out.externalTexturesCount = hdr->externalTexturesCount;
		out.externalTexturesOffset = hdr->externalTexturesOffset - 20;
		out.unkOffset0 = hdr->unkOffset0 ? hdr->unkOffset0 - 20 : 0;

		out.SwapEndian();

		if (!CreateFile(outFolder + ToTSTRING(i) + _T(".casmt"), ofsmt))
			continue;

		std::vector<DataFile> externalDatas;
		char *masterBuffer = static_cast<char *>(malloc(biggestSize));

		for (int i = 0; i < lookups->bufferLookupCount; i++)
			for (int s = 0; s < 2; s++)
			{
				int &cIndex = bufferLookups[i].bufferIndex[s];
				DataFile &cBuff = buffers[cIndex];
				bool found = false;

				for (auto &o : externalDatas)
					if (o.size == cIndex)
					{
						cIndex = o.offset;
						found = true;
					}

				if (found)
					continue;

				dataFile->Seek(cBuff.offset);
				dataFile->ReadBuffer(masterBuffer, cBuff.size);

				externalDatas.push_back({ static_cast<int>(ofsmt.tellp()), cIndex });
				cIndex = externalDatas.back().offset;
				ofsmt.write(masterBuffer, cBuff.size);
			}

		free(masterBuffer);
		ofsmt.close();
		lookups->RSwapEndian();

		short *containerLookups = reinterpret_cast<short *>(dataBuffer + hdr->textureContainerLookupsOffset);
		MapObjectExternalTexture *textures = reinterpret_cast<MapObjectExternalTexture *>(dataBuffer + hdr->externalTexturesOffset);

		for (int i = 0; i < hdr->externalTexturesCount; i++)
		{
			FByteswapper(textures[i].containerID);
			textures[i].containerID = containerLookups[textures[i].containerID];
		}

		if (CreateFile(outFolder + ToTSTRING(i) + _T(".camdo"), ofs))
		{
			ofs.write(reinterpret_cast<char *>(&out), sizeof(MXMDHeader));
			ofs.write(dataBuffer + sizeof(MapTerrainHeader), data[i].size - sizeof(MapTerrainHeader));
			ofs.close();
		}
	}

	free(dataBuffer);
}

void ExtractTGLD(DMSM *dmsm, const TSTRING &outFolder, BinReader *dataFile)
{
	std::ofstream ofs;

	if (CreateFile(outFolder + _T("main.tgld"), ofs))
		ofs.write(dmsm->GetMainTGLD(), dmsm->GetMainTGLDSize());

	ofs.close();

	int biggestSize = 0;

	TGLDEntry *data = dmsm->GetTGLD();

	for (int i = 0; i < dmsm->TGLDCount; i++)
		if (data[i].size > biggestSize)
			biggestSize = data[i].size;

	char *dataBuffer = static_cast<char *>(malloc(biggestSize));

	for (int i = 0; i < dmsm->TGLDCount; i++)
	{
		dataFile->Seek(data[i].offset);
		dataFile->ReadBuffer(dataBuffer, data[i].size);

		if (CreateFile(outFolder + esStringConvert<TCHAR>(dmsm->GetTGLDName(i)) + _T(".tgld"), ofs))
			ofs.write(dataBuffer, data[i].size);

		ofs.close();
	}

	free(dataBuffer);
}

void ExtractEffects(DataFile *data, int count, const TSTRING &outFolder, BinReader *dataFile)
{
	int biggestSize = 0;

	for (int i = 0; i < count; i++)
		if (data[i].size > biggestSize)
			biggestSize = data[i].size;

	char *dataBuffer = static_cast<char *>(malloc(biggestSize)); 
	std::ofstream ofs;

	for (int i = 0; i < count; i++)
	{
		dataFile->Seek(data[i].offset);
		dataFile->ReadBuffer(dataBuffer, data[i].size);

		if (CreateFile(outFolder + ToTSTRING(i) + _T(".epac"), ofs))
			ofs.write(dataBuffer, data[i].size);

		ofs.close();
	}

	free(dataBuffer);
}

int _tmain(int argc, _TCHAR *argv[])
{
	setlocale(LC_ALL, "");
	printer.AddPrinterFunction(wprintf);

	printline("Xenoblade X CASM Extractor by Lukas Cone in 2019.\n");

	if (argc < 2)
	{
		printer << help << pressKeyCont >> 1;
		getchar();  
		return 1;
	}
	
	const TCHAR *filePath = nullptr;

	for (int a = 1; a < argc; a++)
	{
		if (argv[a][0] == '-')
		{
			switch (argv[a][1])
			{
			case '?':
			case 'h':
			{
				printer << help >> 1;
				break;
			}
			case 'u':
				texParams.uncompress = true;
				break;
			case 'b':
				texParams.allowBC5ZChan = true;
				break;
			default:
				printerror("Unrecognized argument: ", << argv[a]);
				break;
			}
		}
		else if (filePath)
		{
			printerror("Unrecognized argument: ", << argv[a]);
		}
		else
			filePath = argv[a];
	}

	if (!filePath)
	{
		printerror("Missing <casmhd file> argument.");
		return 2;
	}

	BinReader rd(filePath);

	if (!rd.IsValid())
	{
		printerror("Cannot open file: ", << filePath);
		return 3;
	}

	TFileInfo fleInf(filePath);

	TSTRING dataFileName = fleInf.GetPath() + fleInf.GetFileName() + _T(".casmda");
	BinReader dataFile(dataFileName);

	if (!dataFile.IsValid())
	{
		printerror("Cannot open file: ", << dataFileName);
		return 4;
	}

	dataFile.SwapEndian(true);

	const size_t fleSize = rd.GetSize();
	char *masterBuffer = static_cast<char *>(malloc(fleSize));
	rd.ReadBuffer(masterBuffer, fleSize);
	DMSM *dmsm = reinterpret_cast<DMSM *>(masterBuffer);

	if (dmsm->magic != DMSM::ID)
	{
		printerror("Invalid DMSM file.");
		return 5;
	}

	dmsm->SwapEndian();

	printline("Extracting CASM file...");

	TSTRING outFolder = fleInf.GetPath() + fleInf.GetFileName() + _T("/");
	_tmkdir(outFolder.c_str());

	ExtractSkyboxes(dmsm->GetSkyboxModels(), dmsm->skyboxModelsCount, outFolder, &dataFile);

	std::ofstream ofs;

	if (CreateFile(outFolder + fleInf.GetFileName() + _T(".cems"), ofs))
		ofs.write(dmsm->GetCEMS(), dmsm->CEMSSize());

	ofs.close();

	if (CreateFile(outFolder + fleInf.GetFileName() + _T(".lcmd"), ofs))
		ofs.write(dmsm->GetLCMD(), dmsm->LCMDSize);

	ofs.close();

	TSTRING outFolderTGLD = outFolder + _T("TGLD/");
	_tmkdir(outFolderTGLD.c_str());
	ExtractTGLD(dmsm, outFolderTGLD, &dataFile);

	TSTRING outFolderEff = outFolder + _T("effects/");
	_tmkdir(outFolderEff.c_str());
	ExtractEffects(dmsm->GetEffectFiles(), dmsm->EFBCount, outFolderEff, &dataFile);

	TSTRING outFolderLOD = outFolder + _T("terrain/");
	_tmkdir(outFolderLOD.c_str());
	ExtractMapTerrain(dmsm->GetTerrainModels(), dmsm->GetTerrainBuffers(), dmsm->terrainModelsCount, outFolderLOD, &dataFile);

	TSTRING outFoldertex = outFolder + _T("textures/");
	_tmkdir(outFoldertex.c_str());
	ExtractCachedTextures(dmsm->GetTerrainCachedTextures(), dmsm->GetTerrainTextures(), dmsm->terrainCachedTexturesCount, outFoldertex, &dataFile);
	ExtractUncachedTextures(dmsm->GetObjectTextures(), dmsm->objectTexturesCount, outFoldertex, &dataFile);

	TSTRING outFolderObjects = outFolder + _T("objects/");
	_tmkdir(outFolderObjects.c_str());
	ExtractMapObjects(dmsm->GetObjectModels(), dmsm->GetObjectBuffers(), dmsm->objectModelsCount, outFolderObjects, &dataFile);

	TSTRING outFolderCol = outFolder + _T("collision/");
	_tmkdir(outFolderCol.c_str());
	ExtractCollision(dmsm, outFolderCol, &dataFile);

	TSTRING outFolderTerrainLODs = outFolder + _T("terrainLOD/");
	_tmkdir(outFolderTerrainLODs.c_str());
	ExtractTerrainLODs(dmsm->GetTerrainLODs(), dmsm->terrainLODsCount, outFolderTerrainLODs, &dataFile);

	free(masterBuffer);
	printline("Done.");

	return 0;
}