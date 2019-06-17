# XenoToolset
Xenoblade Toolset is collection of modding tools for Xenoblade Engine titles.

## casmExtract
Extracts content from CASM map format into loadable assets for XenoLib project.

**Usage:** casmExtract [options] \<casmhd file\>\
casmhd file can be also drag'n'dropped onto application.\
**Options:**\
**-u**	Exported textures will be converted into PNG format, rather than DDS.\
**-b**	Will generate blue channel for some formats used for normal maps.\
**-h**	Will show this help message.\
**-?**	Same as -h command.

## mdoTextureExtract
Extracts textures from camdo/wimdo/wismt(DRSM) files. You can process multiple files at the same time. Best way is to drag'n'drop files onto app.
For this reason a .config file is placed alongside executable file, since app itself only takes file paths as arguments.
A .config file is in XML format.\
***Please do not create any spaces/tabs/uppercase letters/commas as decimal points within setting field. \
Program must run at least once to generate .config file.***
 
### Settings (.config file):
- ***Generate_Log:***\
        Will generate text log of console output next to application location.
- ***BC5_Generate_Blue:***\
        Will generate blue channel for some formats used for normal maps.
- ***PNG_Output:***\
        Exported textures will be converted into PNG format, rather than DDS.
        
## xenoTextureConvert
Converts MTXT/LBIM into DDS/PNG formats. This app uses multithreading, so you can process multiple files at the same time. Best way is to drag'n'drop files onto app.
For this reason a .config file is placed alongside executable file, since app itself only takes file paths as arguments.
A .config file is in XML format.\
***Please do not create any spaces/tabs/uppercase letters/commas as decimal points within setting field. \
Program must run at least once to generate .config file.***

### Settings (.config file):
- ***Generate_Log:***\
        Will generate text log of console output next to application location.
- ***BC5_Generate_Blue:***\
        Will generate blue channel for some formats used for normal maps.
- ***PNG_Output:***\
        Exported textures will be converted into PNG format, rather than DDS.  
        
## [Latest Release](https://github.com/PredatorCZ/XenoToolset/releases)

## License
This toolset is available under GPL v3 license. (See LICENSE.md)

This toolset uses following libraries:

* XenoLib, Copyright (c) 2017-2019 Lukas Cone
* PugiXml, Copyright (c) 2006-2019 Arseny Kapoulkine
