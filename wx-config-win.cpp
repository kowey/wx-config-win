////////////////////////////////////////////////////////////////////////////////////////////////////
// Name:        wx-config-win.cpp
// Purpose:     A wx-config implementation for Windows
// Author:      Takeshi Miya
// Created:     2006-03-23
// Copyright:   (c) Takeshi Miya
// Licence:     wxWidgets licence
// $Revision$
// $URL$
// $Date$
// $Id$
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <cstdlib>

#include <map>
#include <fstream>
#include <algorithm>
#include <vector>

// -------------------------------------------------------------------------------------------------

/// if 1, enables output of -mwindows, warnings, optimize and debug flags automatically.
#ifndef WXCONFIG_EASY_MODE
#define WXCONFIG_EASY_MODE 0
#endif

// -------------------------------------------------------------------------------------------------

std::string getSvnRevision()
{
    std::string str = "$Rev$";
    return str.substr(6, str.length()-8);
}

std::string getSvnDate()
{
    std::string str = "$Date$";
    return str.substr(7, 10);
}

// -------------------------------------------------------------------------------------------------

/// Program options
class Options
{
public:
    std::string& operator[](const std::string& key)
    {
        return m_vars[key];
    }

    bool keyExists(const std::string& key)
    {
        return m_vars.count(key) != 0;
    }
protected:
    std::map<std::string,std::string> m_vars;
};

// -------------------------------------------------------------------------------------------------

/// File build.cfg/config.* options
class BuildFileOptions : public Options
{
public:
    BuildFileOptions(const std::string& filepath)
    {
        parse(filepath);
    }

    bool parse(const std::string& filepath)
    {
        std::string line;
        std::ifstream file(filepath.c_str());
        if (file.is_open())
        {
            while (!file.eof() )
            {
                std::getline(file, line);

                // it's a comment line
                if (line.find_first_of('#') != std::string::npos)
                    continue;

                // strip spaces
                line.erase( std::remove(line.begin(), line.end(), ' '), line.end() );

                split(line);
            }
            file.close();

            if (!m_vars.empty())
                return true;
        }
        else
            std::cout << "   *** Error: Unable to open file '" << filepath.c_str() << "'." << std::endl;

        return false;
    }

protected:
    void split(const std::string& line)
    {
        size_t sep = line.find('=');
        if (sep != std::string::npos)
        {
            std::string key = line.substr(0, sep);
            std::string val = line.substr(sep+1, line.size()-sep-1);
            m_vars[key] = val;
        }
    }
};

// -------------------------------------------------------------------------------------------------

/// Command line options
class CmdLineOptions : public Options
{
public:
    CmdLineOptions(int argc, char* argv[])
    {
        parse(argc, argv);
    }

    bool validArgs()
    {
        bool valid = /*keyExists("--compiler") ||*/
                     keyExists("--prefix") ||
                     keyExists("--wxcfg") ||
                     keyExists("--libs") ||
                     keyExists("--cflags") ||
                     keyExists("--cxxflags") ||
                     keyExists("--rcflags") ||
                     keyExists("--list") ||
                     keyExists("--debug") ||
                     keyExists("--unicode") ||
                     keyExists("--static") ||
                     keyExists("--universal") ||
                     keyExists("--release") ||
                     keyExists("--cc") ||
                     keyExists("--cxx") ||
                     keyExists("--ld") ||
                     keyExists("-v");

        if(!valid)
        {
            // TODO: not all flags are being validated
            if(m_vars.size() > 1 && !keyExists("--help"))
                std::cout << "  *** Error: Unrecognised option: '" <<
                m_vars.begin()->first << "'\n" << std::endl;

            std::cout << "Usage: wx-config [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --prefix[=DIR]              Path of the wxWidgets installation (ie. C:\\wxWidgets2.6.3)\n";
            std::cout << "  --wxcfg[=DIR]               Relative path of the build.cfg file (ie. gcc_dll\\mswud)\n";
//          std::cout << "  --list                      Lists all the library configurations. [NOT IMPLEMENTED]\n";
            std::cout << "  --cflags                    Outputs all pre-processor and compiler flags.\n";
//          std::cout << "  --cxxflags                  Same as --cflags.\n";
            std::cout << "  --rcflags                   Outputs all resource compiler flags. [UNTESTED]\n";
            std::cout << "  --libs                      Outputs all linker flags required for a wxWidgets application.\n";
            std::cout << std::endl;
            std::cout << "  --debug                     Uses a debug configuration if found.\n";
            std::cout << "  --unicode                   Uses an unicode configuration if found.\n";
            std::cout << "  --static                    Uses a static configuration if found.\n";
            std::cout << "  --universal                 Uses an universal configuration if found.\n";
//          std::cout << "  --compiler[=mingw,dmc,vc]   Selects the compiler. The default is mingw.\n";
            std::cout << "  --release                   Outputs the wxWidgets release number.\n";
            std::cout << "  --cc                        Outputs the name of the C compiler.\n";
            std::cout << "  --cxx                       Outputs the name of the C++ compiler.\n";
            std::cout << "  --ld                        Outputs the linker command.\n";
            std::cout << "  -v                          Outputs the revision of wx-config.\n";


            std::cout << std::endl;
            std::cout << "  Note that using --prefix is not needed if you have defined the \n";
            std::cout << "  environmental variable WXWIN.\n";
            std::cout << std::endl;
            std::cout << "  Also note that using --wxcfg is not needed if you have defined the \n";
            std::cout << "  environmental variable WXCFG.\n";
            std::cout << std::endl;
            
        }

        return valid;
    }

    bool parse(int argc, char* argv[])
    {
        for(int i=0; i<argc; ++i)
        {
            std::string line;
            line = argv[i];
            split(line);
        }
        parseLibs(argv[argc-1]);

        return true;
    }

    std::vector<std::string> getLibs() const
    {
        return m_libs;
    }
    
protected:

    void split(const std::string& line)
    {
        size_t sep = line.find("=");
        if (sep != std::string::npos)
        {
            std::string key = line.substr(0, sep);
            std::string val = line.substr(sep+1, line.size()-sep-1);
            m_vars[key] = val;
        }
        else
            m_vars[line] = "";
    }
    
    bool libExists(const std::string& lib)
    {
        return std::find(m_libs.begin(), m_libs.end(), lib) != m_libs.end();
    }
    
    void addLib(const std::string& lib)
    {
        // adds the lib if its not present already
        if (!libExists(lib))
            m_libs.push_back(lib);
    }

    void parseLibs(const std::string libs)
    {
        std::string param = libs;

        // if the last parameter doesn't haves a -- switch
        if (param.find("--") == std::string::npos)
        {
            // saves in the vector, comma separated text like "text1,text2,text3,text4"
            while(true)
            {
                size_t comma = param.find(",");
                if (comma != std::string::npos)
                {
                    m_libs.push_back(param.substr(0, comma));
                    param = param.substr(comma+1, param.size()-comma-1);
                }
                else
                {
                    m_libs.push_back(param);
                    break;
                }
            }
        }
        
        // assuming magic keyword 'std' as a lib parameter for non-monolithic
        // magic keyword std: links with xrc,qa,html,adv,core,base_xml,base_net,base
        if (m_libs.empty() || libExists("std"))
        {
            addLib("xrc");
            addLib("qa");
            addLib("html");
            addLib("adv");
            addLib("core");
            addLib("xml");
            addLib("net");
            addLib("base");
        }
    }

    std::vector<std::string> m_libs;
};

// -------------------------------------------------------------------------------------------------

/// Struct to keep programs
struct CompilerPrograms
{
    std::string cc; // C compiler
    std::string cxx; // C++ compiler
    std::string ld; // dynamic libs linker
    std::string lib; // static libs linker
    std::string windres; // resource compiler
};

/// Struct to keep switches
struct CompilerSwitches
{
    std::string includeDirs;   // -I
    std::string resIncludeDirs;   // --include-dir
    std::string libDirs;       // -L
    std::string linkLibs;      // -l
    std::string libPrefix; // lib
    std::string libExtension; // a
    std::string defines;       // -D
    std::string resDefines;    // --define
    std::string genericSwitch; // -
    std::string objectExtension; // o
    bool forceLinkerUseQuotes; // use quotes for filenames in linker command line (needed or not)?
    bool forceCompilerUseQuotes; // use quotes for filenames in compiler command line (needed or not)?
    bool linkerNeedsLibPrefix; // when adding a link library, linker needs prefix?
    bool linkerNeedsLibExtension; // when adding a link library, linker needs extension?
    bool supportsPCH; // supports precompiled headers?
    std::string PCHExtension; // precompiled headers extension
};

/// Compiler abstract base class
class Compiler
{
public:
    Compiler(const std::string& name) : m_name(name) {}
//    virtual ~Compiler();
    
    std::string easyMode(const std::string& str)
    {
        #if WXCONFIG_EASY_MODE == 1
            return str;
        #else
            return std::string();
        #endif
    }
        
    std::string addFlag(const std::string& flag)
    {
        if (flag.empty())
            return "";
        return flag + " ";
    }

    std::string addLib(const std::string& lib)
    {
        std::string result;
        result = m_switches.linkLibs;
        
        if (m_switches.linkerNeedsLibPrefix)
            result += m_switches.libPrefix;
            
        result += lib;
            
        if (m_switches.linkerNeedsLibExtension)
            result += "." + m_switches.libExtension;
            
        result += " ";
        
        if (lib.empty())
            return "";
        return result;
    }
    
    std::string addDefine(const std::string& define)
    {
        if (define.empty())
            return "";
        return m_switches.defines + define + " ";
    }
    
    std::string addResDefine(const std::string& resDefine)
    {
        if (resDefine.empty())
            return "";
        return m_switches.resDefines + " " + resDefine + " ";
    }
    
    std::string addIncludeDir(const std::string& includeDir)
    {
        if (includeDir.empty())
            return "";
        return m_switches.includeDirs + includeDir + " ";
    }
    
    std::string addLinkerDir(const std::string& libDir)
    {
        if (libDir.empty())
            return "";
        return m_switches.libDirs + libDir + " ";
    }
    
    std::string addResIncludeDir(const std::string& resIncludeDir)
    {
        if (resIncludeDir.empty())
            return "";
        return m_switches.resIncludeDirs + " " + resIncludeDir + " ";
    }
/*
    std::string addResLinkerDir(const std::string& resLibDir)
    {
        if (resLibDir.empty())
            return "";
        return m_switches.libDirs + libDir + " ";
    }*/
    
    std::string getName() const
    {
        return m_name;
    }
    
    void process_3(Options& po, const CmdLineOptions& cl, BuildFileOptions& cfg)
    {
        // BASENAME variables
        po["LIB_BASENAME_MSW"]  = "wx" + po["PORTNAME"] + po["WXUNIVNAME"] + po["WX_RELEASE_NODOT"];
        po["LIB_BASENAME_MSW"] += po["WXUNICODEFLAG"] + po["WXDEBUGFLAG"] + cfg["WX_LIB_FLAVOUR"];

        po["LIB_BASENAME_BASE"]  = "wxbase" + po["WX_RELEASE_NODOT"] + po["WXUNICODEFLAG"];
        po["LIB_BASENAME_BASE"] += po["WXDEBUGFLAG"] + cfg["WX_LIB_FLAVOUR"];


        for (size_t i=0; i<cl.getLibs().size(); ++i)
        {
            std::string lib = cl.getLibs()[i];

            if (lib == "base")
            {
                if (cfg["MONOLITHIC"] == "0")
                    po["__WXLIB_BASE_p"] = addLib(po["LIB_BASENAME_BASE"]);
            }
            else if (lib == "net")
            {
                if (cfg["MONOLITHIC"] == "0")
                    po["__WXLIB_NET_p"] = addLib(po["LIB_BASENAME_BASE"] + "_net");
            }
            else if (lib == "xml")
            {
                if (cfg["MONOLITHIC"] == "0")
                    po["__WXLIB_XML_p"] = addLib(po["LIB_BASENAME_BASE"] + "_xml");
            }
            else if (lib == "core")
            {
                if (cfg["MONOLITHIC"] == "0")
                    if (cfg["USE_GUI"] == "1")
                        po["__WXLIB_CORE_p"] = addLib(po["LIB_BASENAME_MSW"] + "_core");
            }
            else if (lib == "adv")
            {
                if (cfg["MONOLITHIC"] == "0")
                    if (cfg["USE_GUI"] == "1")
                        po["__WXLIB_ADV_p"] = addLib(po["LIB_BASENAME_MSW"] + "_adv");
            }
            else if (lib == "qa")
            {
                if (cfg["MONOLITHIC"] == "0")
                {
                    if (cfg["USE_GUI"] == "1")
                    {
                        if (cfg["USE_QA"] == "1")
                        {
                            po["__WXLIB_QA_p"] = addLib(po["LIB_BASENAME_MSW"] + "_qa");
                            po["__WXLIB_CORE_p"] = addLib(po["LIB_BASENAME_MSW"] + "_core");
                            po["__WXLIB_XML_p"] = addLib(po["LIB_BASENAME_BASE"] + "_xml");
                        }
                    }
                }
            }
            else if (lib == "xrc")
            {
                if (cfg["MONOLITHIC"] == "0")
                {
                    if (cfg["USE_GUI"] == "1")
                    {
                        if (cfg["USE_XRC"] == "1")
                        {
                            po["__WXLIB_XRC_p"] = addLib(po["LIB_BASENAME_MSW"] + "_xrc");
                            po["__WXLIB_XML_p"] = addLib(po["LIB_BASENAME_BASE"] + "_xml");
                            po["__WXLIB_ADV_p"] = addLib(po["LIB_BASENAME_MSW"] + "_adv");
                            po["__WXLIB_HTML_p"] = addLib(po["LIB_BASENAME_MSW"] + "_html");
                        }
                    }
                }
            }
            else if (lib == "aui")
            {
                if (cfg["MONOLITHIC"] == "0")
                    if (cfg["USE_GUI"] == "1")
                        if (cfg["USE_AUI"] == "1")
                            po["__WXLIB_AUI_p"] = addLib(po["LIB_BASENAME_MSW"] + "_aui");
            }
            else if (lib == "html")
            {
                if (cfg["MONOLITHIC"] == "0")
                    if (cfg["USE_GUI"] == "1")
                        if (cfg["USE_HTML"] == "1")
                            po["__WXLIB_HTML_p"] = addLib(po["LIB_BASENAME_MSW"] + "_html");
            }
            else if (lib == "media")
            {
                if (cfg["MONOLITHIC"] == "0")
                    if (cfg["USE_GUI"] == "1")
                        // if (cfg["USE_MEDIA"] == "1") // TODO: wx2.7 CVS haves an USE_MEDIA
                        po["__WXLIB_MEDIA_p"] = addLib(po["LIB_BASENAME_MSW"] + "_media");
            }
            else if (lib == "odbc")
            {
                if (cfg["MONOLITHIC"] == "0")
                    po["__WXLIB_ODBC_p"] = addLib(po["LIB_BASENAME_BASE"] + "_odbc");
            }
            else if (lib == "dbgrid")
            {
                if (cfg["MONOLITHIC"] == "0")
                {
                    if (cfg["USE_GUI"] == "1")
                    {
                        po["__WXLIB_DBGRID_p"] = addLib(po["LIB_BASENAME_MSW"] + "_dbgrid");
                        po["__WXLIB_ODBC_p"] = addLib(po["LIB_BASENAME_BASE"] + "_odbc");
                        po["__WXLIB_ADV_p"] = addLib(po["LIB_BASENAME_MSW"] + "_adv");
                    }
                }
            }
            else if (lib == "opengl" || lib == "gl")
            {
                // TODO: it's opengl or gl?
                /// Doesn't matter if it's monolithic or not
                if (cfg["USE_OPENGL"] == "1")
                    if (cfg["USE_GUI"] == "1")
                        po["__WXLIB_OPENGL_p"]  = addLib(po["LIB_BASENAME_MSW"] + "_gl");
                        po["__WXLIB_OPENGL_p"] += addLib("opengl32");
                        po["__WXLIB_OPENGL_p"] += addLib("glu32");
            }
            else
            {
                /// Doesn't matter if it's monolithic or not
                po["__WXLIB_ARGS_p"] += addLib(po["LIB_BASENAME_MSW"] + "_" + lib);
            }
        }

        if (cfg["MONOLITHIC"] == "1")
            po["__WXLIB_MONO_p"]  = addLib(po["LIB_BASENAME_MSW"]);




        if (cfg["USE_GUI"] == "1")
            po["__LIB_TIFF_p"] = addLib("wxtiff" + po["WXDEBUGFLAG"]);

        if (cfg["USE_GUI"] == "1")
            po["__LIB_JPEG_p"] = addLib("wxjpeg" + po["WXDEBUGFLAG"]);

        if (cfg["USE_GUI"] == "1")
            po["__LIB_PNG_p"] = addLib("wxpng" + po["WXDEBUGFLAG"]);

        if (cfg["MSLU"] == "1")
            po["__UNICOWS_LIB_p"] = addLib("unicows");
    }
    
protected:
    // set the following members in your class
    std::string m_name;
    CompilerPrograms m_programs;
    CompilerSwitches m_switches;
};

// -------------------------------------------------------------------------------------------------

/// MinGW compiler
class CompilerMinGW : public Compiler
{
public:
    CompilerMinGW() : Compiler("gcc")
    {
        m_programs.cc = "mingw32-gcc";
        m_programs.cxx = "mingw32-g++";
        m_programs.ld = "mingw32-g++ -shared -fPIC -o ";
        m_programs.lib = "ar.exe";
        m_programs.windres = "windres";

        m_switches.includeDirs = "-I";
        m_switches.resIncludeDirs = "--include-dir";
        m_switches.libDirs = "-L";
        m_switches.linkLibs = "-l";
        m_switches.libPrefix = "lib";
        m_switches.libExtension = "a";
        m_switches.defines = "-D";
        m_switches.resDefines = "--define";
        m_switches.genericSwitch = "-";
        m_switches.forceCompilerUseQuotes = false;
        m_switches.forceLinkerUseQuotes = false;
        m_switches.linkerNeedsLibPrefix = false;
        m_switches.linkerNeedsLibExtension = false;
        m_switches.supportsPCH = true;
        m_switches.PCHExtension = "h.gch";
    }
    
    void process(Options& po, const CmdLineOptions& cl)
    {
        /// Searchs for '<prefix>\build\msw\config.*' first
        std::string cfg_first = po["prefix"] + "\\build\\msw\\config." + getName();

        /// config.* options
        BuildFileOptions cfg(cfg_first);

        /// build.cfg options
        cfg.parse(po["wxcfgfile"]);


        // ### Variables: ###
        po["WX_RELEASE_NODOT"] = cfg["WXVER_MAJOR"] + cfg["WXVER_MINOR"];
        if (po["WX_RELEASE_NODOT"].empty())
            po["WX_RELEASE_NODOT"] = "26";

        // ### Conditionally set variables: ###
        if (cfg["GCC_VERSION"] == "2.95")
            po["GCCFLAGS"] = addFlag("-fvtable-thunks");

        if (cfg["USE_GUI"] == "0")
            po["PORTNAME"] = "base";

        if (cfg["USE_GUI"] == "1")
            po["PORTNAME"] = "msw";

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_FLAG"] == "default")
            po["WXDEBUGFLAG"] = "d";

        if (cfg["DEBUG_FLAG"] == "1")
            po["WXDEBUGFLAG"] = "d";

        if (cfg["UNICODE"] == "1")
            po["WXUNICODEFLAG"] = "u";

        if (cfg["WXUNIV"] == "1")
            po["WXUNIVNAME"] = "univ";

        if (cfg["SHARED"] == "1")
            po["WXDLLFLAG"] = "dll";

        if (cfg["SHARED"] == "0")
            po["LIBTYPE_SUFFIX"] = "lib";

        if (cfg["SHARED"] == "1")
            po["LIBTYPE_SUFFIX"] = "dll";

        if (cfg["MONOLITHIC"] == "0")
            po["EXTRALIBS_FOR_BASE"] = "";

        if (cfg["MONOLITHIC"] == "1")
            po["EXTRALIBS_FOR_BASE"] = "";

        if (cfg["BUILD"] == "debug")
            po["__OPTIMIZEFLAG_2"] = addFlag("-O0");

        if (cfg["BUILD"] == "release")
            po["__OPTIMIZEFLAG_2"] = addFlag("-O2");

        if (cfg["USE_RTTI"] == "1")
            po["__RTTIFLAG_5"] = addFlag("");

        if (cfg["USE_EXCEPTIONS"] == "0")
            po["__EXCEPTIONSFLAG_6"] = addFlag("-fno-exceptions");

        if (cfg["USE_EXCEPTIONS"] == "1")
            po["__EXCEPTIONSFLAG_6"] = addFlag("");

        if (cfg["WXUNIV"] == "1")
            po["__WXUNIV_DEFINE_p"] = addDefine("__WXUNIVERSAL__");

        if (cfg["WXUNIV"] == "1")
            po["__WXUNIV_DEFINE_p_1"] = addResDefine("__WXUNIVERSAL__");

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_FLAG"] == "default")
            po["__DEBUG_DEFINE_p"] = addDefine("__WXDEBUG__");

        if (cfg["DEBUG_FLAG"] == "1")
            po["__DEBUG_DEFINE_p"] = addDefine("__WXDEBUG__");

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_FLAG"] == "default")
            po["__DEBUG_DEFINE_p_1"] = addResDefine("__WXDEBUG__");

        if (cfg["DEBUG_FLAG"] == "1")
            po["__DEBUG_DEFINE_p_1"] = addResDefine("__WXDEBUG__");

        if (cfg["USE_EXCEPTIONS"] == "0")
            po["__EXCEPTIONS_DEFINE_p"] = addDefine("wxNO_EXCEPTIONS");

        if (cfg["USE_EXCEPTIONS"] == "0")
            po["__EXCEPTIONS_DEFINE_p_1"] = addResDefine("wxNO_EXCEPTIONS");

        if (cfg["USE_RTTI"] == "0")
            po["__RTTI_DEFINE_p"] = addDefine("wxNO_RTTI");

        if (cfg["USE_RTTI"] == "0")
            po["__RTTI_DEFINE_p_1"] = addResDefine("wxNO_RTTI");

        if (cfg["USE_THREADS"] == "0")
            po["__THREAD_DEFINE_p"] = addDefine("wxNO_THREADS");

        if (cfg["USE_THREADS"] == "0")
            po["__THREAD_DEFINE_p_1"] = addResDefine("wxNO_THREADS");

        if (cfg["UNICODE"] == "1")
            po["__UNICODE_DEFINE_p"] = addDefine("_UNICODE");

        if (cfg["UNICODE"] == "1")
            po["__UNICODE_DEFINE_p_1"] = addResDefine("_UNICODE");

        if (cfg["MSLU"] == "1")
            po["__MSLU_DEFINE_p"] = addDefine("wxUSE_UNICODE_MSLU=1");

        if (cfg["MSLU"] == "1")
            po["__MSLU_DEFINE_p_1"] = addResDefine("wxUSE_UNICODE_MSLU=1");

        if (cfg["SHARED"] == "1")
            po["__DLLFLAG_p"] = addDefine("WXUSINGDLL");

        if (cfg["SHARED"] == "1")
            po["__DLLFLAG_p_1"] = addResDefine("WXUSINGDLL");

        process_3(po, cl, cfg);
//----------------------------------------------------

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_INFO"] == "default")
            po["__DEBUGINFO"] = addFlag("-g");

        if (cfg["BUILD"] == "release" && cfg["DEBUG_INFO"] == "default")
            po["__DEBUGINFO"] = addFlag("");

        if (cfg["DEBUG_INFO"] == "0")
            po["__DEBUGINFO"] = addFlag("");

        if (cfg["DEBUG_INFO"] == "1")
            po["__DEBUGINFO"] = addFlag("-g");

        if (cfg["USE_THREADS"] == "0")
            po["__THREADSFLAG"] = addFlag("");

        if (cfg["USE_THREADS"] == "1")
            po["__THREADSFLAG"] = addFlag("-mthreads");
            
//----------------------------------------------------

        // ### Variables, Part 2: ###
        po["LIBDIRNAME"] = po["prefix"] + "\\lib\\" + getName() + "_" + po["LIBTYPE_SUFFIX"] + cfg["CFG"];

        po["SETUPHDIR"]  = po["LIBDIRNAME"] + "\\" + po["PORTNAME"] + po["WXUNIVNAME"];
        po["SETUPHDIR"] += po["WXUNICODEFLAG"] + po["WXDEBUGFLAG"];

        po["cflags"]  = easyMode(po["__DEBUGINFO"]) + easyMode(po["__OPTIMIZEFLAG_2"]) + po["__THREADSFLAG"];
        po["cflags"] += po["GCCFLAGS"] + addDefine("HAVE_W32API_H") + addDefine("__WXMSW__") + po["__WXUNIV_DEFINE_p"];
        po["cflags"] += po["__DEBUG_DEFINE_p"] + po["__EXCEPTIONS_DEFINE_p"] + po["__RTTI_DEFINE_p"];
        po["cflags"] += po["__THREAD_DEFINE_p"] + po["__UNICODE_DEFINE_p"] + po["__MSLU_DEFINE_p"];
        po["cflags"] += addIncludeDir(po["SETUPHDIR"]) + addIncludeDir(po["prefix"] + "\\include");/*-W */
        po["cflags"] += easyMode(addFlag("-Wall")) + easyMode(addIncludeDir(".")) + po["__DLLFLAG_p"];
        po["cflags"] += easyMode(addIncludeDir(".\\..\\..\\samples")) + /*addDefine("NOPCH") +*/ po["__RTTIFLAG_5"] + po["__EXCEPTIONSFLAG_6"];
        po["cflags"] += addFlag("-Wno-ctor-dtor-privacy") + addFlag("-pipe") + addFlag("-fmessage-length=0");
        po["cflags"] += cfg["CPPFLAGS"] + " " + cfg["CXXFLAGS"] + " ";

        po["libs"]  = cfg["LDFLAGS"] + " ";
        po["libs"] += easyMode(po["__DEBUGINFO"]) + po["__THREADSFLAG"];
        po["libs"] += addLinkerDir(po["LIBDIRNAME"]);
        po["libs"] += easyMode(addFlag("-Wl,--subsystem,windows")) + easyMode(addFlag("-mwindows"));
        po["libs"] += po["__WXLIB_ARGS_p"] + po["__WXLIB_OPENGL_p"] + po["__WXLIB_MEDIA_p"];
        po["libs"] += po["__WXLIB_DBGRID_p"] + po["__WXLIB_ODBC_p"] + po["__WXLIB_XRC_p"];
        po["libs"] += po["__WXLIB_QA_p"] + po["__WXLIB_AUI_p"] + po["__WXLIB_HTML_p"] + po["__WXLIB_ADV_p"];
        po["libs"] += po["__WXLIB_CORE_p"] + po["__WXLIB_XML_p"] + po["__WXLIB_NET_p"];
        po["libs"] += po["__WXLIB_BASE_p"] + po["__WXLIB_MONO_p"];
        po["libs"] += po["__LIB_TIFF_p"] + po["__LIB_JPEG_p"] + po["__LIB_PNG_p"];
        po["libs"] += addLib("wxzlib" + po["WXDEBUGFLAG"]) + addLib("wxregex" + po["WXUNICODEFLAG"] + po["WXDEBUGFLAG"]);
        po["libs"] += addLib("wxexpat" + po["WXDEBUGFLAG"]) + po["EXTRALIBS_FOR_BASE"] + po["__UNICOWS_LIB_p"];
        po["libs"] += addLib("kernel32") + addLib("user32") + addLib("gdi32") + addLib("comdlg32") + addLib("winspool");
        po["libs"] += addLib("winmm") + addLib("shell32") + addLib("comctl32") + addLib("ole32") + addLib("oleaut32");
        po["libs"] += addLib("uuid") + addLib("rpcrt4") + addLib("advapi32") + addLib("wsock32") + addLib("odbc32");

        po["rcflags"]  = addFlag("--use-temp-file") + addResDefine("__WXMSW__") + po["__WXUNIV_DEFINE_p_1"];
        po["rcflags"] += po["__DEBUG_DEFINE_p_1"] + po["__EXCEPTIONS_DEFINE_p_1"];
        po["rcflags"] += po["__RTTI_DEFINE_p_1"] + po["__THREAD_DEFINE_p_1"] + po["__UNICODE_DEFINE_p_1"];
        po["rcflags"] += po["__MSLU_DEFINE_p_1"] + addResIncludeDir(po["SETUPHDIR"]);
        po["rcflags"] += addResIncludeDir(po["prefix"] + "\\include") + easyMode(addResIncludeDir("."));
        po["rcflags"] += po["__DLLFLAG_p_1"];
        po["rcflags"] += easyMode(addResIncludeDir(po["prefix"] + "\\samples"));

        po["release"] = cfg["WXVER_MAJOR"] + "." + cfg["WXVER_MINOR"];
        po["version"] = cfg["WXVER_MAJOR"] + "." + cfg["WXVER_MINOR"] + "." + cfg["WXVER_RELEASE"];
        po["cc"] = m_programs.cc;
        po["cxx"] = m_programs.cxx;
        po["ld"] = m_programs.ld;
    }
};

// -------------------------------------------------------------------------------------------------

/// DMars compiler
class CompilerDMC : public Compiler
{
public:
    CompilerDMC() : Compiler("dmc")
    {
        m_programs.cc = "dmc";
        m_programs.cxx = "dmc";
        m_programs.ld = "link";
        m_programs.lib = "lib";
        m_programs.windres = "rcc";

        m_switches.includeDirs = "-I";
        m_switches.resIncludeDirs = "--include-dir"; ////////////
        m_switches.libDirs = "";
        m_switches.linkLibs = "";
        m_switches.libPrefix = "";
        m_switches.libExtension = "lib";
        m_switches.defines = "-D";
        m_switches.resDefines = "-D";//////////////////////////////////(doesnt uses space)
        m_switches.genericSwitch = "-";
        m_switches.forceCompilerUseQuotes = false;
        m_switches.forceLinkerUseQuotes = true;
        m_switches.linkerNeedsLibPrefix = false;
        m_switches.linkerNeedsLibExtension = true;
    }
    
    void process(Options& po, const CmdLineOptions& cl)
    {
        std::string cfg_first = po["prefix"] + "\\build\\msw\\config.dmc";

        /// DMars config.dmc options
        BuildFileOptions cfg(cfg_first);

        // WARNING: HEAVY hardcoded, and not much can be done about that.
        // dmc (unlike dms smake) doesn't support conditional compilation.

        // ### Variables: ###
        po["WX_RELEASE_NODOT"] = "26";

        po["LIBTYPE_SUFFIX"] = "lib";

        po["LIBDIRNAME"] = po["prefix"] + "\\lib\\dmc_" + po["LIBTYPE_SUFFIX"] + cfg["CFG"];

        po["SETUPHDIR"] = po["LIBDIRNAME"] + "\\mswd";

        po["cflags"]  = easyMode(addFlag("-g")) + easyMode(addFlag("-o+none")) + addDefine("_WIN32_WINNT=0x0400");
        po["cflags"] += addDefine("__WXMSW__") + addDefine("__WXDEBUG__") + addIncludeDir(po["prefix"] + "\\include");
        po["cflags"] += addIncludeDir(po["SETUPHDIR"]) + addFlag("-w-") + easyMode(addIncludeDir(".")) + addFlag("-WA");
        po["cflags"] += addIncludeDir(po["prefix"] + "\\samples") + addDefine("NOPCH") + addFlag("-Ar") + addFlag("-Ae");
        po["cflags"] += cfg["CPPFLAGS"] + " " + cfg["CXXFLAGS"] + " ";

        po["libs"]  = addFlag("/NOLOGO") + addFlag("/SILENT") + addFlag("/NOI") + addFlag("/DELEXECUTABLE") + addFlag("/EXETYPE:NT") + cfg["LDFLAGS"] + " ";
        po["libs"] += addFlag("/DEBUG") + addFlag("/CODEVIEW") + addFlag("/su:windows:4.0") + addLinkerDir(po["LIBDIRNAME"]);
        po["libs"] += addLib("wxmsw" + po["WX_RELEASE_NODOT"] + "d" + cfg["WX_LIB_FLAVOUR"] + "_core");
        po["libs"] += addLib("wxbase" + po["WX_RELEASE_NODOT"] + "d" + cfg["WX_LIB_FLAVOUR"]);
        po["libs"] += "wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib  wxregexd.lib wxexpatd.lib ";
        po["libs"] += "kernel32.lib user32.lib gdi32.lib comdlg32.lib winspool.lib winmm.lib shell32.lib ";
        po["libs"] += "comctl32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib advapi32.lib wsock32.lib odbc32.lib ";

        po["rcflags"]  = addResDefine("_WIN32_WINNT=0x0400") + addResDefine("__WXMSW__") + addResDefine("__WXDEBUG__") + addResIncludeDir(po["prefix"] + "\\include");
        po["rcflags"] += addResIncludeDir(po["SETUPHDIR"]) + easyMode(addResIncludeDir(".")) + addResIncludeDir(po["prefix"] + "\\samples") + addFlag("-32") + addFlag("-v-") + " ";

        po["release"] = cfg["WXVER_MAJOR"] + "." + cfg["WXVER_MINOR"];
        po["version"] = cfg["WXVER_MAJOR"] + "." + cfg["WXVER_MINOR"] + "." + cfg["WXVER_RELEASE"];
        po["cc"] = m_programs.cc;
        po["cxx"] = m_programs.cxx;
        po["ld"] = m_programs.ld;
    }
};

// -------------------------------------------------------------------------------------------------

/// Visual Studio compiler
class CompilerVC : public Compiler
{
public:
    CompilerVC() : Compiler("vc")
    {
        m_programs.cc = "cl";
        m_programs.cxx = "cl";
        m_programs.ld = "link";
        m_programs.lib = "link";
        m_programs.windres = "rc";

        m_switches.includeDirs = "/I";
        m_switches.resIncludeDirs = "--include-dir"; ////////////
        m_switches.libDirs = "/LIBPATH:";
        m_switches.linkLibs = "";
        m_switches.libPrefix = "";
        m_switches.libExtension = "lib";
        m_switches.defines = "/D";
        m_switches.resDefines = "/d";
        m_switches.genericSwitch = "/";
        m_switches.forceCompilerUseQuotes = false;
        m_switches.forceLinkerUseQuotes = false;
        m_switches.linkerNeedsLibPrefix = false;
        m_switches.linkerNeedsLibExtension = true;
    }
    
    void process(Options& po, const CmdLineOptions& cl)
    {
        /// Searchs for '<prefix>\build\msw\config.*' first
        std::string cfg_first = po["prefix"] + "\\build\\msw\\config." + getName();

        /// config.* options
        BuildFileOptions cfg(cfg_first);

        /// build.cfg options
        cfg.parse(po["wxcfgfile"]);


        // ### Variables: ###
        po["WX_RELEASE_NODOT"] = cfg["WXVER_MAJOR"] + cfg["WXVER_MINOR"];
        if (po["WX_RELEASE_NODOT"].empty())
            po["WX_RELEASE_NODOT"] = "26";

        // ### Conditionally set variables: ###
        if (cfg["USE_GUI"] == "0")
            po["PORTNAME"] = "base";

        if (cfg["USE_GUI"] == "1")
            po["PORTNAME"] = "msw";

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_FLAG"] == "default")
            po["WXDEBUGFLAG"] = "d";

        if (cfg["DEBUG_FLAG"] == "1")
            po["WXDEBUGFLAG"] = "d";

        if (cfg["UNICODE"] == "1")
            po["WXUNICODEFLAG"] = "u";

        if (cfg["WXUNIV"] == "1")
            po["WXUNIVNAME"] = "univ";

        if (cfg["SHARED"] == "1")
            po["WXDLLFLAG"] = "dll";

        if (cfg["SHARED"] == "0")
            po["LIBTYPE_SUFFIX"] = "lib";

        if (cfg["SHARED"] == "1")
            po["LIBTYPE_SUFFIX"] = "dll";

        if (cfg["MONOLITHIC"] == "0")
            po["EXTRALIBS_FOR_BASE"] = "";

        if (cfg["MONOLITHIC"] == "1")
            po["EXTRALIBS_FOR_BASE"] = "";
            
        if (cfg["TARGET_CPU"] == "amd64")
            po["DIR_SUFFIX_CPU"] = "_amd64";

        if (cfg["TARGET_CPU"] == "ia64")
            po["DIR_SUFFIX_CPU"] = "_ia64";

        // TODO: TARGET_CPU uses $(CPU), so it will be ignored unless explicitely specified
        if (cfg["TARGET_CPU"] == "$(CPU)")
            po["LINK_TARGET_CPU"] = "";

        if (cfg["TARGET_CPU"] == "amd64")
            po["LINK_TARGET_CPU"] = addFlag("/MACHINE:AMD64");

        if (cfg["TARGET_CPU"] == "ia64")
            po["LINK_TARGET_CPU"] = addFlag("/MACHINE:IA64");

//----------------------------------------------------

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_INFO"] == "default")
            po["__DEBUGINFO_0"] = addFlag("/Zi");

        if (cfg["BUILD"] == "release" && cfg["DEBUG_INFO"] == "default")
            po["__DEBUGINFO_0"] = addFlag("");

        if (cfg["DEBUG_INFO"] == "0")
            po["__DEBUGINFO_0"] = addFlag("");

        if (cfg["DEBUG_INFO"] == "1")
            po["__DEBUGINFO_0"] = addFlag("/Zi");

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_INFO"] == "default")
            po["__DEBUGINFO_1"] = addFlag("/DEBUG");

        if (cfg["BUILD"] == "release" && cfg["DEBUG_INFO"] == "default")
            po["__DEBUGINFO_1"] = addFlag("");

        if (cfg["DEBUG_INFO"] == "0")
            po["__DEBUGINFO_1"] = addFlag("");

        if (cfg["DEBUG_INFO"] == "1")
            po["__DEBUGINFO_1"] = addFlag("/DEBUG");

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_RUNTIME_LIBS"] == "default")
            po["____DEBUGRUNTIME_2_p"] = addDefine("_DEBUG");

        if (cfg["BUILD"] == "release" && cfg["DEBUG_RUNTIME_LIBS"] == "default")
            po["____DEBUGRUNTIME_2_p"] = addDefine("");

        if (cfg["DEBUG_RUNTIME_LIBS"] == "0")
            po["____DEBUGRUNTIME_2_p"] = addDefine("");

        if (cfg["DEBUG_RUNTIME_LIBS"] == "1")
            po["____DEBUGRUNTIME_2_p"] = addDefine("_DEBUG");

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_RUNTIME_LIBS"] == "default")
            po["____DEBUGRUNTIME_2_p_1"] = addResDefine("_DEBUG");

        if (cfg["BUILD"] == "release" && cfg["DEBUG_RUNTIME_LIBS"] == "default")
            po["____DEBUGRUNTIME_2_p_1"] = addResDefine("");

        if (cfg["DEBUG_RUNTIME_LIBS"] == "0")
            po["____DEBUGRUNTIME_2_p_1"] = addResDefine("");

        if (cfg["DEBUG_RUNTIME_LIBS"] == "1")
            po["____DEBUGRUNTIME_2_p_1"] = addResDefine("_DEBUG");

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_RUNTIME_LIBS"] == "default")
            po["__DEBUGRUNTIME_3"] = "d";

        if (cfg["BUILD"] == "release" && cfg["DEBUG_RUNTIME_LIBS"] == "default")
            po["__DEBUGRUNTIME_3"] = "";

        if (cfg["DEBUG_RUNTIME_LIBS"] == "0")
            po["__DEBUGRUNTIME_3"] = "";

        if (cfg["DEBUG_RUNTIME_LIBS"] == "1")
            po["__DEBUGRUNTIME_3"] = "d";
            
//----------------------------------------------------

        if (cfg["BUILD"] == "debug")
            po["__OPTIMIZEFLAG_4"] = addFlag("/Od");

        if (cfg["BUILD"] == "release")
            po["__OPTIMIZEFLAG_4"] = addFlag("/O2");

        if (cfg["USE_THREADS"] == "0")
            po["__THREADSFLAG_7"] = "L";

        if (cfg["USE_THREADS"] == "1")
            po["__THREADSFLAG_7"] = "T";

        if (cfg["RUNTIME_LIBS"] == "dynamic")
            po["__RUNTIME_LIBS_8"] = "D";

        if (cfg["RUNTIME_LIBS"] == "static")
            po["__RUNTIME_LIBS_8"] = po["__THREADSFLAG_7"];

//----------------------------------------------------

        if (cfg["USE_RTTI"] == "0")
            po["__RTTIFLAG_9"] = addFlag("");

        if (cfg["USE_RTTI"] == "1")
            po["__RTTIFLAG_9"] = addFlag("/GR");

        if (cfg["USE_EXCEPTIONS"] == "0")
            po["__EXCEPTIONSFLAG_10"] = addFlag("");

        if (cfg["USE_EXCEPTIONS"] == "1")
            po["__EXCEPTIONSFLAG_10"] = addFlag("/EHsc");
            
//----------------------------------------------------

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_RUNTIME_LIBS"] == "0")
            po["__NO_VC_CRTDBG_p"] = addDefine("__NO_VC_CRTDBG__");

        if (cfg["BUILD"] == "release" && cfg["DEBUG_FLAG"] == "1")
            po["__NO_VC_CRTDBG_p"] = addDefine("__NO_VC_CRTDBG__");

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_RUNTIME_LIBS"] == "0")
            po["__NO_VC_CRTDBG_p_1"] = addResDefine("__NO_VC_CRTDBG__");

        if (cfg["BUILD"] == "release" && cfg["DEBUG_FLAG"] == "1")
            po["__NO_VC_CRTDBG_p_1"] = addResDefine("__NO_VC_CRTDBG__");

        if (cfg["WXUNIV"] == "1")
            po["__WXUNIV_DEFINE_p"] = addDefine("__WXUNIVERSAL__");

        if (cfg["WXUNIV"] == "1")
            po["__WXUNIV_DEFINE_p_1"] = addResDefine("__WXUNIVERSAL__");

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_FLAG"] == "default")
            po["__DEBUG_DEFINE_p"] = addDefine("__WXDEBUG__");

        if (cfg["DEBUG_FLAG"] == "1")
            po["__DEBUG_DEFINE_p"] = addDefine("__WXDEBUG__");

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_FLAG"] == "default")
            po["__DEBUG_DEFINE_p_1"] = addResDefine("__WXDEBUG__");

        if (cfg["DEBUG_FLAG"] == "1")
            po["__DEBUG_DEFINE_p_1"] = addResDefine("__WXDEBUG__");

        if (cfg["USE_EXCEPTIONS"] == "0")
            po["__EXCEPTIONS_DEFINE_p"] = addDefine("wxNO_EXCEPTIONS");

        if (cfg["USE_EXCEPTIONS"] == "0")
            po["__EXCEPTIONS_DEFINE_p_1"] = addResDefine("wxNO_EXCEPTIONS");

        if (cfg["USE_RTTI"] == "0")
            po["__RTTI_DEFINE_p"] = addDefine("wxNO_RTTI");

        if (cfg["USE_RTTI"] == "0")
            po["__RTTI_DEFINE_p_1"] = addResDefine("wxNO_RTTI");

        if (cfg["USE_THREADS"] == "0")
            po["__THREAD_DEFINE_p"] = addDefine("wxNO_THREADS");

        if (cfg["USE_THREADS"] == "0")
            po["__THREAD_DEFINE_p_1"] = addResDefine("wxNO_THREADS");

        if (cfg["UNICODE"] == "1")
            po["__UNICODE_DEFINE_p"] = addDefine("_UNICODE");

        if (cfg["UNICODE"] == "1")
            po["__UNICODE_DEFINE_p_1"] = addResDefine("_UNICODE");

        if (cfg["MSLU"] == "1")
            po["__MSLU_DEFINE_p"] = addDefine("wxUSE_UNICODE_MSLU=1");

        if (cfg["MSLU"] == "1")
            po["__MSLU_DEFINE_p_1"] = addResDefine("wxUSE_UNICODE_MSLU=1");

        if (cfg["SHARED"] == "1")
            po["__DLLFLAG_p"] = addDefine("WXUSINGDLL");

        if (cfg["SHARED"] == "1")
            po["__DLLFLAG_p_1"] = addResDefine("WXUSINGDLL");

        process_3(po, cl, cfg);


        // ### Variables, Part 2: ###
        po["LIBDIRNAME"] = po["prefix"] + "\\lib\\" + getName() + po["DIR_SUFFIX_CPU"] + "_" + po["LIBTYPE_SUFFIX"] + cfg["CFG"];

        po["SETUPHDIR"]  = po["LIBDIRNAME"] + "\\" + po["PORTNAME"] + po["WXUNIVNAME"];
        po["SETUPHDIR"] += po["WXUNICODEFLAG"] + po["WXDEBUGFLAG"];

        po["cflags"]  = "/M" + po["__RUNTIME_LIBS_8"] + po["__DEBUGRUNTIME_3"] + " " + addDefine("WIN32");
        po["cflags"] += easyMode(po["__DEBUGINFO_0"]) + easyMode(po["____DEBUGRUNTIME_2_p"]);
        po["cflags"] += easyMode(po["__OPTIMIZEFLAG_4"]) + po["__NO_VC_CRTDBG_p"] + addDefine("__WXMSW__") + po["__WXUNIV_DEFINE_p"];
        po["cflags"] += po["__DEBUG_DEFINE_p"] + po["__EXCEPTIONS_DEFINE_p"] + po["__RTTI_DEFINE_p"];
        po["cflags"] += po["__THREAD_DEFINE_p"] + po["__UNICODE_DEFINE_p"] + po["__MSLU_DEFINE_p"];
        po["cflags"] += addIncludeDir(po["SETUPHDIR"]) + addIncludeDir(po["prefix"] + "\\include") + easyMode(addFlag("/W4")) + easyMode(addIncludeDir(".")) + po["__DLLFLAG_p"] + addDefine("_WINDOWS");
        po["cflags"] += easyMode(addIncludeDir(po["prefix"] + "\\samples")) + easyMode(addDefine("NOPCH")) + po["__RTTIFLAG_9"] + po["__EXCEPTIONSFLAG_10"];
        po["cflags"] += cfg["CPPFLAGS"] + " " + cfg["CXXFLAGS"] + " ";

        po["libs"]  = easyMode(addFlag("/NOLOGO"));
        po["libs"] += cfg["LDFLAGS"] + " ";
        po["libs"] += easyMode(po["__DEBUGINFO_1"]) + " " + po["LINK_TARGET_CPU"] + " ";
        po["libs"] += addLinkerDir(po["LIBDIRNAME"]);
        po["libs"] += easyMode(addFlag("/SUBSYSTEM:WINDOWS"));   
        po["libs"] += po["__WXLIB_ARGS_p"] + po["__WXLIB_OPENGL_p"] + po["__WXLIB_MEDIA_p"];
        po["libs"] += po["__WXLIB_DBGRID_p"] + po["__WXLIB_ODBC_p"] + po["__WXLIB_XRC_p"];
        po["libs"] += po["__WXLIB_QA_p"] + po["__WXLIB_AUI_p"] + po["__WXLIB_HTML_p"] + po["__WXLIB_ADV_p"];
        po["libs"] += po["__WXLIB_CORE_p"] + po["__WXLIB_XML_p"] + po["__WXLIB_NET_p"];
        po["libs"] += po["__WXLIB_BASE_p"] + po["__WXLIB_MONO_p"];
        po["libs"] += po["__LIB_TIFF_p"] + po["__LIB_JPEG_p"] + po["__LIB_PNG_p"];
        po["libs"] += addLib("wxzlib" + po["WXDEBUGFLAG"]) + addLib("wxregex" + po["WXUNICODEFLAG"] + po["WXDEBUGFLAG"]);
        po["libs"] += addLib("wxexpat" + po["WXDEBUGFLAG"]) + po["EXTRALIBS_FOR_BASE"] + po["__UNICOWS_LIB_p"];
        po["libs"] += addLib("kernel32") + addLib("user32") + addLib("gdi32") + addLib("comdlg32") + addLib("winspool"); 
        po["libs"] += addLib("winmm") + addLib("shell32") + addLib("comctl32") + addLib("ole32") + addLib("oleaut32");
        po["libs"] += addLib("uuid") + addLib("rpcrt4") + addLib("advapi32") + addLib("wsock32") + addLib("odbc32");

        po["rcflags"]  = addResDefine("WIN32") + po["____DEBUGRUNTIME_2_p_1"] + po["__NO_VC_CRTDBG_p_1"] + addResDefine("__WXMSW__");
        po["rcflags"] += po["__WXUNIV_DEFINE_p_1"] + po["__DEBUG_DEFINE_p_1"] + po["__EXCEPTIONS_DEFINE_p_1"];
        po["rcflags"] += po["__RTTI_DEFINE_p_1"] + po["__THREAD_DEFINE_p_1"] + po["__UNICODE_DEFINE_p_1"];
        po["rcflags"] += po["__MSLU_DEFINE_p_1"] + addResIncludeDir(po["SETUPHDIR"]);
        po["rcflags"] += addResIncludeDir(po["prefix"] + "\\include") + easyMode(addResIncludeDir("."));
        po["rcflags"] += po["__DLLFLAG_p_1"] + addResDefine("_WINDOWS");
        po["rcflags"] += easyMode(addResIncludeDir(po["prefix"] + "\\samples"));


        po["release"] = cfg["WXVER_MAJOR"] + "." + cfg["WXVER_MINOR"];
        po["version"] = cfg["WXVER_MAJOR"] + "." + cfg["WXVER_MINOR"] + "." + cfg["WXVER_RELEASE"];
        po["cc"] = m_programs.cc;
        po["cxx"] = m_programs.cxx;
        po["ld"] = m_programs.ld;
    }    
};

// -------------------------------------------------------------------------------------------------

/// OpenWatcom compiler
class CompilerWAT : public Compiler
{
public:
    CompilerWAT() : Compiler("wat")
    {
        m_programs.cc = "wcl386";//TODO: wcc386
        m_programs.cxx = "wcl386";//TODO: wpp386
        m_programs.ld = "wcl386";
        m_programs.lib = "wlib";
        m_programs.windres = "wrc";

        m_switches.includeDirs = "-i=";
        m_switches.resIncludeDirs = "-i=";
        m_switches.libDirs = "libp ";
        m_switches.linkLibs = "";
        m_switches.libPrefix = "libr ";
        m_switches.libExtension = "lib";
        m_switches.defines = "-d";
        m_switches.resDefines = "-d";
        m_switches.genericSwitch = "-";
        m_switches.forceCompilerUseQuotes = false;
        m_switches.forceLinkerUseQuotes = false;
        m_switches.linkerNeedsLibPrefix = true;//TODO:!
        m_switches.linkerNeedsLibExtension = true;
    }
    
    void process(Options& po, const CmdLineOptions& cl)
    {
        /// Searchs for '<prefix>\build\msw\config.*' first
        std::string cfg_first = po["prefix"] + "\\build\\msw\\config." + getName();

        /// config.* options
        BuildFileOptions cfg(cfg_first);

        /// build.cfg options
        cfg.parse(po["wxcfgfile"]);


        // ### Variables: ###
        po["WX_RELEASE_NODOT"] = cfg["WXVER_MAJOR"] + cfg["WXVER_MINOR"];
        if (po["WX_RELEASE_NODOT"].empty())
            po["WX_RELEASE_NODOT"] = "26";

        // ### Conditionally set variables: ###
        if (cfg["USE_GUI"] == "0")
            po["PORTNAME"] = "base";

        if (cfg["USE_GUI"] == "1")
            po["PORTNAME"] = "msw";
            
        if (cfg["BUILD"] == "debug" && cfg["DEBUG_FLAG"] == "default")
            po["WXDEBUGFLAG"] = "d";

        if (cfg["DEBUG_FLAG"] == "1")
            po["WXDEBUGFLAG"] = "d";

        if (cfg["UNICODE"] == "1")
            po["WXUNICODEFLAG"] = "u";

        if (cfg["WXUNIV"] == "1")
            po["WXUNIVNAME"] = "univ";

        if (cfg["SHARED"] == "1")
            po["WXDLLFLAG"] = "dll";

        if (cfg["SHARED"] == "0")
            po["LIBTYPE_SUFFIX"] = "lib";

        if (cfg["SHARED"] == "1")
            po["LIBTYPE_SUFFIX"] = "dll";

        if (cfg["MONOLITHIC"] == "0")
            po["EXTRALIBS_FOR_BASE"] = "";

        if (cfg["MONOLITHIC"] == "1")
            po["EXTRALIBS_FOR_BASE"] = "";
//---------till here, the same
        if (cfg["BUILD"] == "debug" && cfg["DEBUG_INFO"] == "default")
            po["__DEBUGINFO_0"] = addFlag("-d2");

        if (cfg["BUILD"] == "release" && cfg["DEBUG_INFO"] == "default")
            po["__DEBUGINFO_0"] = addFlag("-d0");

        if (cfg["DEBUG_INFO"] == "0")
            po["__DEBUGINFO_0"] = addFlag("-d0");

        if (cfg["DEBUG_INFO"] == "1")
            po["__DEBUGINFO_0"] = addFlag("-d2");

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_INFO"] == "default")
            po["__DEBUGINFO_1"] = addFlag("debug all"); //TODO: ???

        if (cfg["BUILD"] == "release" && cfg["DEBUG_INFO"] == "default")
            po["__DEBUGINFO_1"] = addFlag(""); //TODO: ???

        if (cfg["DEBUG_INFO"] == "0")
            po["__DEBUGINFO_1"] = addFlag(""); //TODO: ???

        if (cfg["DEBUG_INFO"] == "1")
            po["__DEBUGINFO_1"] = addFlag("debug all"); //TODO: ???
//---------from here, the same
        if (cfg["BUILD"] == "debug")
            po["__OPTIMIZEFLAG_2"] = addFlag("-od");

        if (cfg["BUILD"] == "release")
            po["__OPTIMIZEFLAG_2"] = addFlag("-ot -ox");
//------from here, almost the same
        if (cfg["USE_THREADS"] == "0")
            po["__THREADSFLAG_5"] = addFlag("");

        if (cfg["USE_THREADS"] == "1")
            po["__THREADSFLAG_5"] = addFlag("-bm");
//---------from here, simmilar to VC
        if (cfg["RUNTIME_LIBS"] == "dynamic")
            po["__RUNTIME_LIBS_6"] = addFlag("-br");
            
        if (cfg["RUNTIME_LIBS"] == "static")
            po["__RUNTIME_LIBS_6"] = addFlag("");
//---------simmilar:
        if (cfg["USE_RTTI"] == "0")
            po["__RTTIFLAG_7"] = addFlag("");
        
        if (cfg["USE_RTTI"] == "1")
            po["__RTTIFLAG_7"] = addFlag("-xr");
       
        if (cfg["USE_EXCEPTIONS"] == "0")
            po["__EXCEPTIONSFLAG_8"] = addFlag("");

        if (cfg["USE_EXCEPTIONS"] == "1")
            po["__EXCEPTIONSFLAG_8"] = addFlag("-xs");

        process_3(po, cl, cfg);

//---------the same, but without resDefines:
        if (cfg["WXUNIV"] == "1")
            po["__WXUNIV_DEFINE_p"] = addDefine("__WXUNIVERSAL__");

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_FLAG"] == "default")
            po["__DEBUG_DEFINE_p"] = addDefine("__WXDEBUG__");

        if (cfg["DEBUG_FLAG"] == "1")
            po["__DEBUG_DEFINE_p"] = addDefine("__WXDEBUG__");

        if (cfg["USE_EXCEPTIONS"] == "0")
            po["__EXCEPTIONS_DEFINE_p"] = addDefine("wxNO_EXCEPTIONS");

        if (cfg["USE_RTTI"] == "0")
            po["__RTTI_DEFINE_p"] = addDefine("wxNO_RTTI");

        if (cfg["USE_THREADS"] == "0")
            po["__THREAD_DEFINE_p"] = addDefine("wxNO_THREADS");

        if (cfg["UNICODE"] == "1")
            po["__UNICODE_DEFINE_p"] = addDefine("_UNICODE");

        if (cfg["MSLU"] == "1")
            po["__MSLU_DEFINE_p"] = addDefine("wxUSE_UNICODE_MSLU=1");

        if (cfg["SHARED"] == "1")
            po["__DLLFLAG_p"] = addDefine("WXUSINGDLL");


        // ### Variables, Part 2: ###
        po["LIBDIRNAME"] = po["prefix"] + "\\lib\\" + getName() + "_" + po["LIBTYPE_SUFFIX"] + cfg["CFG"];

        po["SETUPHDIR"]  = po["LIBDIRNAME"] + "\\" + po["PORTNAME"] + po["WXUNIVNAME"];
        po["SETUPHDIR"] += po["WXUNICODEFLAG"] + po["WXDEBUGFLAG"];

        po["cflags"]  = easyMode(po["__DEBUGINFO_0"]) + easyMode(po["__OPTIMIZEFLAG_2"]) + po["__THREADSFLAG_5"];
        po["cflags"] += po["__RUNTIME_LIBS_6"] + addDefine("__WXMSW__") + po["__WXUNIV_DEFINE_p"];
        po["cflags"] += po["__DEBUG_DEFINE_p"] + po["__EXCEPTIONS_DEFINE_p"] + po["__RTTI_DEFINE_p"];
        po["cflags"] += po["__THREAD_DEFINE_p"] + po["__UNICODE_DEFINE_p"] + po["__MSLU_DEFINE_p"];
        po["cflags"] += addIncludeDir(po["SETUPHDIR"]) + addIncludeDir(po["prefix"] + "\\include");
        po["cflags"] += addFlag("-wx") + addFlag("-wcd=549") + addFlag("-wcd=656") + addFlag("-wcd=657") + addFlag("-wcd=667");
        po["cflags"] += easyMode(addIncludeDir(".")) + po["__DLLFLAG_p"];
        po["cflags"] += easyMode(addIncludeDir(po["prefix"] + "\\samples")) + addDefine("NOPCH") + po["__RTTIFLAG_7"] + po["__EXCEPTIONSFLAG_8"];
        po["cflags"] += cfg["CPPFLAGS"] + " " + cfg["CXXFLAGS"] + " ";

        po["lbc"]  = "option quiet\n";
        po["lbc"] += "name $^@\n";
        po["lbc"] += "option caseexact\n";
        po["libs"]  = cfg["LDFLAGS"] + " ";
        po["libs"] += easyMode(po["__DEBUGINFO_1"]);
        po["libs"] += addLinkerDir(po["LIBDIRNAME"]);
        po["lbc"] += "    libpath " + po["LIBDIRNAME"] + " system nt_win ref '_WinMain@16'";     
        po["libs"] += po["__WXLIB_ARGS_p"] + po["__WXLIB_OPENGL_p"] + po["__WXLIB_MEDIA_p"];
        po["libs"] += po["__WXLIB_DBGRID_p"] + po["__WXLIB_ODBC_p"] + po["__WXLIB_XRC_p"];
        po["libs"] += po["__WXLIB_QA_p"] + po["__WXLIB_AUI_p"] + po["__WXLIB_HTML_p"] + po["__WXLIB_ADV_p"];
        po["libs"] += po["__WXLIB_CORE_p"] + po["__WXLIB_XML_p"] + po["__WXLIB_NET_p"];
        po["libs"] += po["__WXLIB_BASE_p"] + po["__WXLIB_MONO_p"];
        po["libs"] += po["__LIB_TIFF_p"] + po["__LIB_JPEG_p"] + po["__LIB_PNG_p"];
        po["libs"] += addLib("wxzlib" + po["WXDEBUGFLAG"]) + addLib("wxregex" + po["WXUNICODEFLAG"] + po["WXDEBUGFLAG"]);
        po["libs"] += addLib("wxexpat" + po["WXDEBUGFLAG"]) + po["EXTRALIBS_FOR_BASE"] + po["__UNICOWS_LIB_p"];
        po["libs"] += addLib("kernel32") + addLib("user32") + addLib("gdi32") + addLib("comdlg32") + addLib("winspool");
        po["libs"] += addLib("winmm") + addLib("shell32") + addLib("comctl32") + addLib("ole32") + addLib("oleaut32");
        po["libs"] += addLib("uuid") + addLib("rpcrt4") + addLib("advapi32") + addLib("wsock32") + addLib("odbc32");

        po["rcflags"]  = addFlag("-q") + addFlag("-ad") + addFlag("-bt=nt") + addFlag("-r");
        po["rcflags"] += addResDefine("__WXMSW__") + po["__WXUNIV_DEFINE_p"];
        po["rcflags"] += po["__DEBUG_DEFINE_p"] + po["__EXCEPTIONS_DEFINE_p"];
        po["rcflags"] += po["__RTTI_DEFINE_p"] + po["__THREAD_DEFINE_p"] + po["__UNICODE_DEFINE_p"];
        po["rcflags"] += po["__MSLU_DEFINE_p_1"] + addResIncludeDir(po["SETUPHDIR"]);
        po["rcflags"] += addResIncludeDir(po["prefix"] + "\\include") + easyMode(addResIncludeDir("."));
        po["rcflags"] += po["__DLLFLAG_p_1"];
        po["rcflags"] += easyMode(addResIncludeDir(po["prefix"] + "\\samples"));

        po["release"] = cfg["WXVER_MAJOR"] + "." + cfg["WXVER_MINOR"];
        po["version"] = cfg["WXVER_MAJOR"] + "." + cfg["WXVER_MINOR"] + "." + cfg["WXVER_RELEASE"];
        po["cc"] = m_programs.cc;
        po["cxx"] = m_programs.cxx;
        po["ld"] = m_programs.ld;    
    }
};

// -------------------------------------------------------------------------------------------------

/// Borland compiler
class CompilerBCC : public Compiler
{
public:
    CompilerBCC() : Compiler("bcc")
    {
        m_programs.cc = "bcc32";
        m_programs.cxx = "bcc32";
        m_programs.ld = "ilink32";
        m_programs.lib = "tlib";
        m_programs.windres = "brcc32";

        m_switches.includeDirs = "-I";
        m_switches.resIncludeDirs = "--include-dir"; ////////////
        m_switches.libDirs = "-L";
        m_switches.linkLibs = "";
        m_switches.libPrefix = "";
        m_switches.libExtension = "lib";
        m_switches.defines = "-D";
        m_switches.resDefines = "/d";  ////////
        m_switches.genericSwitch = "-";
        m_switches.forceCompilerUseQuotes = false;
        m_switches.forceLinkerUseQuotes = true;
        m_switches.linkerNeedsLibPrefix = false;
        m_switches.linkerNeedsLibExtension = true;
    }
    
    void process(Options& po, const CmdLineOptions& cl) {}
};

// -------------------------------------------------------------------------------------------------

void normalizePath(std::string& path)
{
    // converts all slashes to backslashes
    std::replace(path.begin(), path.end(), '/', '\\');
    
    // removes the first slash (if any) from the given path
    std::string::iterator firstChar = path.begin();
    if (*firstChar == '\\')
        path.erase(firstChar);
    
    // removes the last slash (if any) from the given path
    std::string::iterator lastChar = --path.end();
    if (*lastChar == '\\')
        path.erase(lastChar);
}

// -------------------------------------------------------------------------------------------------

void autodetectConfiguration(Options& po)
{      
    std::vector<std::string> cfgs;
    /// Searchs for '<prefix>\lib\gcc_[dll|lib][CFG]\[msw|base][univ][u][d]\build.cfg' next
    // TODO: account for $(CFG), $(DIR_SUFFIX_CPU)
    cfgs.push_back("gcc_dll\\msw");
    cfgs.push_back("gcc_dll\\mswu");
    cfgs.push_back("gcc_dll\\mswud");
    cfgs.push_back("gcc_dll\\mswd");
    cfgs.push_back("gcc_dll\\mswuniv");
    cfgs.push_back("gcc_dll\\mswunivu");
    cfgs.push_back("gcc_dll\\mswunivud");
    cfgs.push_back("gcc_lib\\msw");
    cfgs.push_back("gcc_lib\\mswu");
    cfgs.push_back("gcc_lib\\mswud");
    cfgs.push_back("gcc_lib\\mswd");
    cfgs.push_back("gcc_lib\\mswuniv");
    cfgs.push_back("gcc_lib\\mswunivu");
    cfgs.push_back("gcc_lib\\mswunivud");
    cfgs.push_back("gcc_dll\\base");
    cfgs.push_back("gcc_dll\\baseu");
    cfgs.push_back("gcc_dll\\baseud");
    cfgs.push_back("gcc_dll\\based");
    cfgs.push_back("gcc_dll\\baseuniv");
    cfgs.push_back("gcc_dll\\baseunivu");
    cfgs.push_back("gcc_dll\\baseunivud");
    cfgs.push_back("gcc_lib\\base");
    cfgs.push_back("gcc_lib\\baseu");
    cfgs.push_back("gcc_lib\\baseud");
    cfgs.push_back("gcc_lib\\based");
    cfgs.push_back("gcc_lib\\baseuniv");
    cfgs.push_back("gcc_lib\\baseunivu");
    cfgs.push_back("gcc_lib\\baseunivud");
    
    /// Searchs for '<prefix>\lib\dmc_lib\mswd\build.cfg' next
    // TODO: check if a build.cfg really exists
    cfgs.push_back("dmc_lib\\mswd");
    
    /// Searchs for '<prefix>\lib\vc[cpu]_[dll|lib][CFG]\[msw|base][univ][u][d]\build.cfg' next
    // TODO: account for $(CFG), $(DIR_SUFFIX_CPU)
    cfgs.push_back("vc_dll\\msw");
    cfgs.push_back("vc_dll\\mswu");
    cfgs.push_back("vc_dll\\mswud");
    cfgs.push_back("vc_dll\\mswd");
    cfgs.push_back("vc_dll\\mswuniv");
    cfgs.push_back("vc_dll\\mswunivu");
    cfgs.push_back("vc_dll\\mswunivud");
    cfgs.push_back("vc_lib\\msw");
    cfgs.push_back("vc_lib\\mswu");
    cfgs.push_back("vc_lib\\mswud");
    cfgs.push_back("vc_lib\\mswd");
    cfgs.push_back("vc_lib\\mswuniv");
    cfgs.push_back("vc_lib\\mswunivu");
    cfgs.push_back("vc_lib\\mswunivud");
    cfgs.push_back("vc_dll\\base");
    cfgs.push_back("vc_dll\\baseu");
    cfgs.push_back("vc_dll\\baseud");
    cfgs.push_back("vc_dll\\based");
    cfgs.push_back("vc_dll\\baseuniv");
    cfgs.push_back("vc_dll\\baseunivu");
    cfgs.push_back("vc_dll\\baseunivud");
    cfgs.push_back("vc_lib\\base");
    cfgs.push_back("vc_lib\\baseu");
    cfgs.push_back("vc_lib\\baseud");
    cfgs.push_back("vc_lib\\based");
    cfgs.push_back("vc_lib\\baseuniv");
    cfgs.push_back("vc_lib\\baseunivu");
    cfgs.push_back("vc_lib\\baseunivud");
    
    /// Searchs for '<prefix>\lib\wat_[dll|lib][CFG]\[msw|base][univ][u][d]\build.cfg' next
    // TODO: account for $(CFG)
    cfgs.push_back("wat_dll\\msw");
    cfgs.push_back("wat_dll\\mswu");
    cfgs.push_back("wat_dll\\mswud");
    cfgs.push_back("wat_dll\\mswd");
    cfgs.push_back("wat_dll\\mswuniv");
    cfgs.push_back("wat_dll\\mswunivu");
    cfgs.push_back("wat_dll\\mswunivud");
    cfgs.push_back("wat_lib\\msw");
    cfgs.push_back("wat_lib\\mswu");
    cfgs.push_back("wat_lib\\mswud");
    cfgs.push_back("wat_lib\\mswd");
    cfgs.push_back("wat_lib\\mswuniv");
    cfgs.push_back("wat_lib\\mswunivu");
    cfgs.push_back("wat_lib\\mswunivud");
    cfgs.push_back("wat_dll\\base");
    cfgs.push_back("wat_dll\\baseu");
    cfgs.push_back("wat_dll\\baseud");
    cfgs.push_back("wat_dll\\based");
    cfgs.push_back("wat_dll\\baseuniv");
    cfgs.push_back("wat_dll\\baseunivu");
    cfgs.push_back("wat_dll\\baseunivud");
    cfgs.push_back("wat_lib\\base");
    cfgs.push_back("wat_lib\\baseu");
    cfgs.push_back("wat_lib\\baseud");
    cfgs.push_back("wat_lib\\based");
    cfgs.push_back("wat_lib\\baseuniv");
    cfgs.push_back("wat_lib\\baseunivu");
    cfgs.push_back("wat_lib\\baseunivud");    

    // reads the first build.cfg it founds
    for(std::vector<std::string>::const_iterator it = cfgs.begin(); it != cfgs.end(); ++it)
    {
        std::string file = po["prefix"] + "\\lib\\" + *it + "\\build.cfg";
        std::ifstream build_cfg(file.c_str());
        if (build_cfg.is_open())
        {
            if (!po.keyExists("wxcfg"))
                po["wxcfg"] = *it;
            else
            {
                std::cout << "   *** Warning: multiple compiled configurations of wxWidgets have been detected." << std::endl;
                std::cout << std::endl;
                std::cout << "       Please use the --wxcfg flag (as in `wx-config --wxcfg=gcc_dll\\mswud`)" << std::endl;
                std::cout << "       or set the environment variable WXCFG (as in WXCFG=gcc_dll\\mswud)" << std::endl;
                std::cout << "       to specify which configuration exactly you want to use." << std::endl;
                
                std::exit(1);
            }
        }
    }

    if (!po.keyExists("wxcfg"))
    {
        std::cout << "   *** Error: No build.cfg file has been auto-detected." << std::endl;
        std::cout << std::endl;
        std::cout << "       Please use the --wxcfg flag (as in `wx-config --wxcfg=gcc_dll\\mswud`)" << std::endl;
        std::cout << "       or set the environment variable WXCFG (as in WXCFG=gcc_dll\\mswud)" << std::endl;
        std::cout << "       to specify which configuration exactly you want to use." << std::endl;
        
        std::exit(1);
    }    
}

// -------------------------------------------------------------------------------------------------

void checkAdditionalFlags(Options& po, CmdLineOptions& cl)
{
    /// Modifies wxcfg as 'vc[cpu]_[dll|lib][CFG]\[msw|base][univ][u][d]' accordingly
    /// or 'gcc_[dll|lib][CFG]\[msw|base][univ][u][d]'

    if (cl.keyExists("--universal"))
    {
        // Pattern: Replace /(msw|base)/ to /(msw|base)univ/
        
        size_t univ = po["wxcfg"].find("univ");
        if (univ == std::string::npos)
        {
            size_t msw = po["wxcfg"].find("msw");
            if (msw != std::string::npos)
                po["wxcfg"].replace(msw, std::string("mswuniv").length(), "mswuniv");
                
            size_t base = po["wxcfg"].find("base");
            if (base != std::string::npos)
                po["wxcfg"].replace(base, std::string("baseuniv").length(), "baseuniv");
        }
    }
        
    if (cl.keyExists("--unicode"))
    {
        // Pattern: Add /.*u/ if it's not already
        // or /.*ud/ if --debug is specified
        
        // TODO: std::find will be better
        std::string::iterator lastChar = --po["wxcfg"].end();
        if (*lastChar != 'u' && *lastChar != 'd')
            po["wxcfg"] += "u";
        else if ((*lastChar - 1) != 'u' && *lastChar == 'd')
        {
            *lastChar = 'u';
            po["wxcfg"] += "d";
        }        
    }
        
    if (cl.keyExists("--debug"))
    {
        // Pattern: Add /.*d/ if it's not already
        std::string::iterator lastChar = --po["wxcfg"].end();
        if (*lastChar != 'd')
            po["wxcfg"] += "d";
    }

    if (cl.keyExists("--static"))
    {
        // Pattern: Replace /.*_dll/ to /.*_lib/
        
        size_t loc = po["wxcfg"].find("_dll");
        if (loc != std::string::npos)
            po["wxcfg"].replace(loc, std::string("_lib").length(), "_lib");
    }
    
}

// -------------------------------------------------------------------------------------------------


void detectCompiler(Options& po, const CmdLineOptions& cl)
{
    // input example of po["wxcfg"]:
    // gcc_dll\\mswud
    // vc_lib\\msw
    
    if (po["wxcfg"].find("gcc_") != std::string::npos)
    {
        CompilerMinGW compiler;
        compiler.process(po, cl);
        return;
    }
    else if (po["wxcfg"].find("dmc_") != std::string::npos)
    {
        CompilerDMC compiler;
        compiler.process(po, cl);
        return;
    }
    else if (po["wxcfg"].find("vc_") != std::string::npos)
    {
        CompilerVC compiler;
        compiler.process(po, cl);
        return;
    }
    else if (po["wxcfg"].find("wat_") != std::string::npos)
    {
        CompilerWAT compiler;
        compiler.process(po, cl);
        return;
    }
    else if (po["wxcfg"].find("bcc_") != std::string::npos)
    {
        CompilerBCC compiler;
        compiler.process(po, cl);
        return;
    }
    else
    {
        std::cout << "   *** Error: No supported compiler has been detected in the configuration '" << po["wxcfg"] << "'." << std::endl;
        std::cout << std::endl;
        std::cout << "       The specified wxcfg must start with a 'gcc_', 'dmc_' or 'vc_'" << std::endl;
        std::cout << "       to be successfully detected." << std::endl;

        std::exit(1);
    }
}

// -------------------------------------------------------------------------------------------------

void validatePrefix(const std::string& prefix)
{
    // tests if prefix is a valid dir. checking if there is an \include\wx\wx.h
    const std::string testfile = prefix + "\\include\\wx\\wx.h";
    std::ifstream prefixIsValid(testfile.c_str());
    if (!prefixIsValid.is_open())
    {
        std::cout << "   *** Error: wxWidgets hasn't been found installed at '" << prefix << "'." << std::endl;
        std::cout << std::endl;
        std::cout << "       Please use the --prefix flag (as in `wx-config --prefix=C:\\wxWidgets`)" << std::endl;
        std::cout << "       or set the environment variable WXWIN (as in WXWIN=C:\\wxWidgets)" << std::endl;
        std::cout << "       to specify where is your installation of wxWidgets." << std::endl;

        std::exit(1);
    }    
}

// -------------------------------------------------------------------------------------------------

void validateConfiguration(const std::string& wxcfgfile)
{   
    std::ifstream build_cfg(wxcfgfile.c_str());
    if (!build_cfg.is_open())
    {
        std::cout << "   *** Error: No valid configuration of wxWidgets has been specified." << std::endl;
        std::cout << std::endl;
        std::cout << "       Please use the --wxcfg flag (as in `wx-config --wxcfg=gcc_dll\\mswud`)" << std::endl;
        std::cout << "       or set the environment variable WXCFG (as in WXCFG=gcc_dll\\mswud)" << std::endl;
        std::cout << "       to specify which configuration exactly you want to use." << std::endl;
        
        std::exit(1);
    }
}

// -------------------------------------------------------------------------------------------------

//TODO: why const qualifier for cl doesn't works
void outputFlags(Options& po, CmdLineOptions& cl)
{
    /// Outputs flags to console
    if (cl.keyExists("--cc"))
        std::cout << po["cc"];
    if (cl.keyExists("--cxx"))
        std::cout << po["cxx"];
    if (cl.keyExists("--ld"))
        std::cout << po["ld"];
    if (cl.keyExists("--cflags") || cl.keyExists("--cxxflags") || cl.keyExists("--cppflags"))
        std::cout << po["cflags"] << std::endl;
    if (cl.keyExists("--libs"))
        std::cout << po["libs"] << std::endl;
    if (cl.keyExists("--rcflags"))
        std::cout << po["rcflags"] << std::endl;
    if (cl.keyExists("--release"))
        std::cout << po["release"] << std::endl;
    if (cl.keyExists("-v"))
        std::cout << "wx-config revision " << getSvnRevision() << " " << getSvnDate() << std::endl;

#if 0 // not implemented
    if (cl.keyExists("--version"))
        std::cout << po["version"];
    if (cl.keyExists("--list"))
        std::cout << po["list"];
    if (cl.keyExists("--basename"))
        std::cout << po["basename"];
#endif
   
}

// -------------------------------------------------------------------------------------------------


int main(int argc, char* argv[])
{
    /// Command line options
    CmdLineOptions cl(argc, argv);

    /// Program options
    Options po;

    if (!cl.validArgs())
        return 1;

    if (cl.keyExists("--prefix"))
        po["prefix"] = cl["--prefix"];
    else if (std::getenv("WXWIN"))
        po["prefix"] = std::getenv("WXWIN");
    else
        po["prefix"] = "C:\\wxWidgets";

    normalizePath(po["prefix"]);
    
    validatePrefix(po["prefix"]);

    if (cl.keyExists("--wxcfg"))
        po["wxcfg"] = cl["--wxcfg"];
    else if (std::getenv("WXCFG") && cl["--prefix"].empty())
        po["wxcfg"] = std::getenv("WXCFG");
    else
        autodetectConfiguration(po);

    normalizePath(po["wxcfg"]);
    
    checkAdditionalFlags(po, cl);
    
    po["wxcfgfile"] = po["prefix"] + "\\lib\\" + po["wxcfg"] + "\\build.cfg";

    validateConfiguration(po["wxcfgfile"]);
    
    detectCompiler(po, cl);

    outputFlags(po, cl);

    return 0;
}
