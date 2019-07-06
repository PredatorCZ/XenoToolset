/*  xenoTex
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
#include "XenoLibAPI.h"
#include "datas/SettingsManager.hpp"
#include "datas/fileinfo.hpp"
#include "datas/MultiThread.hpp"
#include "datas/binreader.hpp"
#include "pugixml.hpp"

#ifndef _MSC_VER
#define _tmain main
#define _TCHAR char
#endif

static struct xenoTex : SettingsManager
{
	DECLARE_REFLECTOR;
	
	bool Generate_Log = false;
	bool PNG_Output = false;
	bool BC5_Generate_Blue = true;
}settings;

REFLECTOR_START_WNAMES(xenoTex, PNG_Output, BC5_Generate_Blue, Generate_Log);

static const char help[] = "\nConverts MTXT/LBIM into DDS/PNG formats.\n\
Settings (.config file):\n\
  PNG_Output: \n\
        Exported textures will be converted into PNG format, rather than DDS.\n\
  BC5_Generate_Blue:\n\
        Will generate blue channel for some formats used for normal maps.\n\
  Generate_Log: \n\
        Will generate text log of console output next to application location.\n\t";

static const char pressKeyCont[] = "\nPress ENTER to close.";

struct TexQueueTraits
{
	int queue;
	int queueEnd;
	TCHAR **files;
	typedef void return_type;

	return_type RetreiveItem();

	operator bool() { return queue < queueEnd; }
	void operator++(int) { queue++; }
	int NumQueues() const { return queueEnd - 1; }
};

TexQueueTraits::return_type TexQueueTraits::RetreiveItem()
{
	const TCHAR *curFile = files[queue];
	TFileInfo fleInfo(curFile);
	BinReader rd(curFile);

	if (!rd.IsValid())
	{
		printerror("Couldn't load file: ", << curFile);
		return;
	}

	printline("Loading file: ", << curFile);

	rd.Seek(-4, std::ios_base::seekdir::_S_cur);

	int magic;
	const int fileSize = static_cast<int>(rd.GetSize());
	rd.Read(magic);
	rd.Seek(0);

	switch (magic)
	{
	case CompileFourCC("MTXT"):
	{
		printline("MTXT detected.");

		char *buffer = static_cast<char *>(malloc(fileSize));
		rd.ReadBuffer(buffer, fileSize);

		ConvertMTXT(buffer, fileSize, (fleInfo.GetPath() + fleInfo.GetFileName()).c_str(), { settings.PNG_Output, settings.BC5_Generate_Blue });
		break;
	}

	case CompileFourCC("LBIM"):
	{
		printline("LBIM detected.");

		char *buffer = static_cast<char *>(malloc(fileSize));
		rd.ReadBuffer(buffer, fileSize);

		ConvertLBIM(buffer, fileSize, (fleInfo.GetPath() + fleInfo.GetFileName()).c_str(), { settings.PNG_Output, settings.BC5_Generate_Blue });
		break;
	}
	default:
	{
		printerror("Invalid file format.");
		break;
	}
	}
}

int _tmain(int argc, _TCHAR *argv[])
{
	setlocale(LC_ALL, "");
	#ifdef UNICODE
	printer.AddPrinterFunction(wprintf);
	#else
	printer.AddPrinterFunction(reinterpret_cast<void*>(printf));
	#endif
	
	printline("Xenoblade Texture Converter by Lukas Cone in 2019.\nSimply drag'n'drop files into application or use as xenoTextureConvert file1 file2 ...\n");

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

	printer.PrintThreadID(true);

	TexQueueTraits texQue;
	texQue.files = argv;
	texQue.queue = 1;
	texQue.queueEnd = argc;

	RunThreadedQueue(texQue);

	return 0;
}