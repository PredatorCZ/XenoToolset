/*  mdoTextureExtract
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

#include <thread>
#include "MXMD.h"
#include "DRSM.h"
#include "datas/SettingsManager.hpp"
#include "datas/fileinfo.hpp"
#include "datas/MultiThread.hpp"
#include "pugixml.hpp"

static struct mdoTex : SettingsManager
{
	DECLARE_REFLECTOR;

	bool Generate_Log = false;
	bool PNG_Output = false;
	bool BC5_Generate_Blue = true;
}settings;

REFLECTOR_START_WNAMES(mdoTex, PNG_Output, BC5_Generate_Blue, Generate_Log);

static const char help[] = "\nExtracts textures from camdo/wimdo/wismt(DRSM) files.\n\
Settings (.config file):\n\
  PNG_Output: \n\
        Exported textures will be converted into PNG format, rather than DDS.\n\
  BC5_Generate_Blue:\n\
        Will generate blue channel for some formats used for normal maps.\n\
  Generate_Log: \n\
        Will generate text log of console output next to application location.\n\t";

static const char pressKeyCont[] = "\nPress ENTER to close.";

struct TextureQueue
{
	int queue;
	int queueEnd;
	const DRSM *caller;
	const TCHAR *folderPath;

	typedef int return_type;

	TextureQueue() : queue(0) {}

	return_type RetreiveItem()
	{
		int result = caller->ExtractTexture(folderPath, queue, { settings.PNG_Output, settings.BC5_Generate_Blue });
		return result;
	}

	operator bool() { return queue < queueEnd; }
	void operator++(int) { queue++; }
	int NumQueues() const { return queueEnd; }
};

int _tmain(int argc, _TCHAR *argv[])
{
	setlocale(LC_ALL, "");
	printer.AddPrinterFunction(wprintf);

	printline("Xenoblade Model Texture Extractor by Lukas Cone in 2019.\nSimply drag'n'drop files into application or use as mdoTextureExtract file1 file2 ...\n");

	TFileInfo configInfo(*argv);
	const TSTRING configName = configInfo.GetPath() + configInfo.GetFileName() + _T(".config");

	settings.FromXML(configName);

	pugi::xml_document doc = {};
	pugi::xml_node mainNode(settings.ToXML(doc));
	mainNode.prepend_child(pugi::xml_node_type::node_comment).set_value(help);

	doc.save_file(configName.c_str(), "\t", pugi::format_write_bom | pugi::format_indent);

	if (argc < 2)
	{
		printerror("Insufficient argument count, expected at aleast 1.\n");
		printer << help << pressKeyCont >> 1;
		getchar();
		return 1;
	}

	if (argv[1][1] == '?' || argv[1][1] == 'h')
	{
		printer << help << pressKeyCont >> 1;
		getchar();
		return 0;
	}

	if (settings.Generate_Log)
		settings.CreateLog(configInfo.GetPath() + configInfo.GetFileName());

	for (int f = 1; f < argc; f++)
	{
		printline("Processing file: ", << argv[f]);

		TFileInfo texInfo(argv[f]);
		TSTRING texFolder = texInfo.GetPath() + texInfo.GetFileName() + _T("/");
		MXMD modFile;

		if (!modFile.Load(argv[f]))
		{
			printline("MXMD detected.");

			MXMDTextures::Ptr textures = modFile.GetTextures();

			if (!textures)
				continue;

			_tmkdir(texFolder.c_str());

			textures->ExtractAllTextures(texFolder.c_str(), { settings.PNG_Output, settings.BC5_Generate_Blue });
			continue;
		}

		DRSM streamFile;

		if (!streamFile.Load(argv[f]))
		{
			printline("DRSM detected.");

			_tmkdir(texFolder.c_str());

			TextureQueue texQue;
			texQue.caller = &streamFile;
			texQue.queueEnd = streamFile.GetNumTextures();
			texQue.folderPath = texFolder.c_str();

			RunThreadedQueue(texQue);
			continue;
		}

	}


	return 0;
}