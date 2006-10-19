////////////////////////////////////////////////////////////////////////////////////////////////////
// Name:        wx-config-win.cpp
// Purpose:     A wx-config implementation for Windows
// Author:      Takeshi Miya
// Created:     2006-03-23
// Copyright:   (c) Takeshi Miya
// Licence:     wxWidgets licence
// $Rev$
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

std::string getSvnRevision()
{
    std::string str = "$Rev$";
    if (str.length() > 8)
        return str.substr(6, str.length()-8);
    else
        return "X";
}

std::string getSvnDate()
{
    std::string str = "$Date$";
    if (str.length() > 16)
        return str.substr(7, 10);
    else
        return "2006-XX-XX";
}

static const std::string g_tokError = "wx-config Error: ";
static const std::string g_tokWarning = "wx-config Warning: ";

// -------------------------------------------------------------------------------------------------

/// Program options
class Options
{
public:
    std::string& operator[](const std::string& key)
    {
        return m_vars[key];
    }

    bool keyExists(const std::string& key) const
    {
        return m_vars.count(key) != 0;
    }
    
    const std::string& keyValue(const std::string& key) const
    {
        return m_vars.find(key)->second;
    }

    std::map<std::string,std::string>& getVars()
    {
        return m_vars;
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
            std::cout << g_tokError << "Unable to open file '" << filepath.c_str() << "'." << std::endl;

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

/// File setup.h options
class SetupHOptions
{
public:
    bool& operator[](const std::string& key)
    {
        return m_vars[key];
    }

    bool keyExists(const std::string& key) const
    {
        return m_vars.count(key) != 0;
    }
    
    bool keyValue(const std::string& key) const
    {
        return m_vars.find(key)->second;
    }
    
    typedef std::map<std::string,bool> StringBoolMap;
    
    StringBoolMap& getVars()
    {
        return m_vars;
    }
protected:
    StringBoolMap m_vars;

    
public:
    SetupHOptions(const std::string& filepath)
    {
        parse(filepath);
    }

    bool parse(const std::string& filepath)
    {
        std::string line;
        std::ifstream file(filepath.c_str());
        if (file.is_open())
        {
            while (!file.eof())
            {
                std::getline(file, line);
                
                // does the splitting/parsing
                split(line);
            }
            file.close();

            if (!m_vars.empty())
                return true;
        }
        else
            std::cout << g_tokError << "Unable to open file '" << filepath.c_str() << "'." << std::endl;

        return false;
    }
    
    void printDebug()
    {
        std::cout << "DEBUG: setup.h contents BEGIN -------------------------------------------------" << std::endl;
        for (StringBoolMap::iterator it = m_vars.begin(); it != m_vars.end(); ++it)
            std::cout << it->first << "=" << it->second << std::endl;
        std::cout << "DEBUG: setup.h contents END ---------------------------------------------------" << std::endl;
    }

protected:
    void split(std::string& line)
    {
        // it's a comment line
        if (line.find_first_of('/') != std::string::npos)
            return; // skips the line
            
        // strip spaces and tabs
        line.erase( std::remove(line.begin(), line.end(), ' '), line.end() );
        line.erase( std::remove(line.begin(), line.end(), '\t'), line.end() );
        
        std::string tokDefine("#define");
        size_t posDefine = line.find_first_of(tokDefine);
                        
        // it's a #define line
        if (posDefine != std::string::npos)
        {
            std::string key;
            bool val;

            // resolves val, checking if last char is 0 or 1
            char lastChar = line.at(line.length()-1);       // TODO: I don't like this line :P
            if (lastChar == '0')
                val = false;
            else if (lastChar == '1')
                val = true;
            else
                return; // skips the line
            
            // resolves key
            size_t startPos = posDefine + tokDefine.length();
            key = line.substr(startPos, line.length() - startPos - 1);

            // finally saves the parsed data!
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
        bool valid = keyExists("--compiler") ||
                     keyExists("--easymode") ||
                     keyExists("--variable") ||
                     keyExists("--define-variable") ||
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

        // TODO: not all flags are being validated
        if(!valid)
        {
            if (m_vars.size() > 1 && !keyExists("--help"))
                std::cout << g_tokError << "Unrecognised option: '" << m_vars.begin()->first << "'\n" << std::endl;

            std::cerr << "Usage: wx-config [options]\n";
            std::cerr << "Options:\n";
            std::cerr << "  --prefix[=DIR]              Path of the wxWidgets installation (ie. C:\\wxWidgets2.6.3)\n";
            std::cerr << "  --wxcfg[=DIR]               Relative path of the build.cfg file (ie. gcc_dll\\mswud)\n";
//          std::cerr << "  --list                      Lists all the library configurations. [NOT IMPLEMENTED]\n";
            std::cerr << "  --cflags                    Outputs all pre-processor and compiler flags.\n";
            std::cerr << "  --cxxflags                  Same as --cflags but for C++.\n";
            std::cerr << "  --rcflags                   Outputs all resource compiler flags. [UNTESTED]\n";
            std::cerr << "  --libs                      Outputs all linker flags.\n";
            std::cerr << std::endl;
            std::cerr << "  --debug[=yes|no]            Uses a debug configuration if found.\n";
            std::cerr << "  --unicode[=yes|no]          Uses an unicode configuration if found.\n";
            std::cerr << "  --static[=yes|no]           Uses a static configuration if found.\n";
            std::cerr << "  --universal[=yes|no]        Uses an universal configuration if found.\n";
            std::cerr << "  --easymode[=yes|no]         Outputs warnings, and optimize flags.\n";
            std::cerr << "  --compiler[=gcc,dmc,vc]     Selects the compiler.\n";
//          std::cerr << "  --variable=NAME             Returns the value of a defined variable.\n";
//          std::cerr << "  --define-variable=NAME=VAL  Sets a global value for a variable.\n";
            std::cerr << "  --release                   Outputs the wxWidgets release number.\n";
            std::cerr << "  --cc                        Outputs the name of the C compiler.\n";
            std::cerr << "  --cxx                       Outputs the name of the C++ compiler.\n";
            std::cerr << "  --ld                        Outputs the linker command.\n";
            std::cerr << "  -v                          Outputs the revision of wx-config.\n";


            std::cerr << std::endl;
            std::cerr << "  Note that using --prefix is not needed if you have defined the \n";
            std::cerr << "  environmental variable WXWIN.\n";
            std::cerr << std::endl;
            std::cerr << "  Also note that using --wxcfg is not needed if you have defined the \n";
            std::cerr << "  environmental variable WXCFG.\n";
            std::cerr << std::endl;
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

static bool g_sEasyMode = false;

void checkEasyMode(CmdLineOptions& cl)
{
    if (cl.keyExists("--easymode"))
    {
        if (cl["--easymode"] == "no")
            g_sEasyMode = false;
        else if (cl["--easymode"] == "yes" || cl["--easymode"].empty())
            g_sEasyMode = true;
    }
}

/// Compiler abstract base class
class Compiler
{
public:
    Compiler(const std::string& name) : m_name(name) {}
    // ~Compiler();

    std::string easyMode(const std::string& str)
    {
        if (g_sEasyMode)
            return str;
        else
            return std::string();
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
        SetupHOptions sho(po["wxcfgsetuphfile"]);
        
        // FIXME: proper place of this would be in a first hook, say process_1();
        if (cl.keyExists("--define-variable"))
        {   
            std::string strDef = cl.keyValue("--define-variable");
            size_t sep = strDef.find("=");
            if (sep != std::string::npos)
            {
                std::string key = strDef.substr(0, sep);
                std::string val = strDef.substr(sep+1, strDef.size()-sep-1);
                
                po[key] = val;
                cfg[key] = val;
                
                if (val == "1" || val == "true")
                    sho[key] = true;
                else if (val == "0" || val == "false")
                    sho[key] = false;
            }
            else
            {
                std::cout << g_tokError << "Failed to define a variable as '" << cl.keyValue("--define-variable") << "'." << std::endl;
                std::cout << "The syntax is --define-variable=VARIABLENAME=VARIABLEVALUE" << std::endl;
                exit(1);
            }
        }
        

        /// Overriding flags sho->cfg!!
        /// This makes sho variables haves more privilege than cfg ones
        //-------------------------------------------------------------
        if (sho.keyExists("wxUSE_UNICODE_MSLU"))
            sho["wxUSE_UNICODE_MSLU"] ? cfg["MSLU"] = "1" : cfg["MSLU"] = "0";
            
        // TODO: probably better!!!:
        if (cfg.keyExists("MSLU"))
            sho["wxUSE_UNICODE_MSLU"] ? cfg["MSLU"] = "1" : cfg["MSLU"] = "0";
        
        //-------------------------------------------------------------
        
        /// Overriding compiler programs
        if (cfg.keyExists("CC"))
            m_programs.cc = cfg["CC"];
        
        if (cfg.keyExists("CXX"))
            m_programs.cxx = cfg["CXX"];

        if (cfg.keyExists("LD"))
            m_programs.ld = cfg["LD"];

        if (cfg.keyExists("LIB"))
            m_programs.lib = cfg["LIB"];
            
        if (cfg.keyExists("WINDRES"))
            m_programs.windres = cfg["WINDRES"];

        //-------------------------------------------------------------

        
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
                    if (sho["wxUSE_ODBC"])
                        po["__WXLIB_ODBC_p"] = addLib(po["LIB_BASENAME_BASE"] + "_odbc");
            }
            else if (lib == "dbgrid")
            {
                if (cfg["MONOLITHIC"] == "0")
                {
                    if (cfg["USE_GUI"] == "1")
                    {
                        if (sho["wxUSE_ODBC"])
                        {
                            po["__WXLIB_DBGRID_p"] = addLib(po["LIB_BASENAME_MSW"] + "_dbgrid");
                            po["__WXLIB_ODBC_p"] = addLib(po["LIB_BASENAME_BASE"] + "_odbc");
                            po["__WXLIB_ADV_p"] = addLib(po["LIB_BASENAME_MSW"] + "_adv");
                        }
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


        /// External libs (to wxWidgets)

        if (cfg["USE_GUI"] == "1")
            if (sho["wxUSE_LIBTIFF"])
                po["__LIB_TIFF_p"] = addLib("wxtiff" + po["WXDEBUGFLAG"]);

        if (cfg["USE_GUI"] == "1")
            if (sho["wxUSE_LIBJPEG"])
                po["__LIB_JPEG_p"] = addLib("wxjpeg" + po["WXDEBUGFLAG"]);

        if (cfg["USE_GUI"] == "1")
            if (sho["wxUSE_LIBPNG"] && sho["wxUSE_ZLIB"])
                po["__LIB_PNG_p"] = addLib("wxpng" + po["WXDEBUGFLAG"]);


        if (sho["wxUSE_ZLIB"])
            po["__LIB_ZLIB_p"] = addLib("wxzlib" + po["WXDEBUGFLAG"]);
            
        if (sho["wxUSE_REGEX"])
            po["__LIB_REGEX_p"] = addLib("wxregex" + po["WXUNICODEFLAG"] + po["WXDEBUGFLAG"]);

        // FIXME: in truth the check should be for wxUSE_XML but... the sho parser is very simple :P
        if (sho["wxUSE_XRC"])
            po["__LIB_EXPAT_p"] = addLib("wxexpat" + po["WXDEBUGFLAG"]);
            
        if (cfg["MSLU"] == "1")
            po["__LIB_UNICOWS_p"] = addLib("unicows");

        po["__LIB_KERNEL32_p"] = addLib("kernel32");
        
        po["__LIB_USER32_p"] = addLib("user32");
        
        po["__LIB_GDI32_p"] = addLib("gdi32");
        
        po["__LIB_COMDLG32_p"] = addLib("comdlg32");
        
        po["__LIB_WINSPOOL_p"] = addLib("winspool");
        
        po["__LIB_WINMM_p"] = addLib("winmm");
        
        po["__LIB_SHELL32_p"] = addLib("shell32");
        
        po["__LIB_COMCTL32_p"] = addLib("comctl32");
        
        if (sho["wxUSE_OLE"])
            po["__LIB_OLE32_p"] = addLib("ole32");
        
        if (sho["wxUSE_OLE"])
            po["__LIB_OLEAUT32_p"] = addLib("oleaut32");
            
        if (sho["wxUSE_OLE"])
            po["__LIB_OLEACC_p"] = addLib("oleacc");    // NOTE: not being used
            
        if (sho["wxUSE_OLE"])
            po["__LIB_OLE2W32_p"] = addLib("ole2w32");  // NOTE: not being used
        
        po["__LIB_UUID_p"] = addLib("uuid");
        
        po["__LIB_RPCRT4_p"] = addLib("rpcrt4");
        
        po["__LIB_ADVAPI32_p"] = addLib("advapi32");
              
        if (sho["wxUSE_SOCKETS"])
            po["__LIB_WSOCK32_p"] = addLib("wsock32");
            
        if (sho["wxUSE_ODBC"])
            po["__LIB_ODBC32_p"] = addLib("odbc32");
            
           
/*      TODO: From BAKEFILE
        <!-- link-in system libs that wx depends on: -->
        <!-- If on borland, we don't need to do much            -->
        <if cond="FORMAT=='borland'">
            <sys-lib>ole2w32</sys-lib>
            <sys-lib>odbc32</sys-lib>
        </if>

        <!-- Non-borland, on the other hand...                  -->
        <if cond="FORMAT not in ['borland','msevc4prj']">
            <sys-lib>kernel32</sys-lib>
            <sys-lib>user32</sys-lib>
            <sys-lib>gdi32</sys-lib>
            <sys-lib>comdlg32</sys-lib>
            <sys-lib>winspool</sys-lib>
            <sys-lib>winmm</sys-lib>
            <sys-lib>shell32</sys-lib>
            <sys-lib>comctl32</sys-lib>
            <sys-lib>ole32</sys-lib>
            <sys-lib>oleaut32</sys-lib>
            <sys-lib>uuid</sys-lib>
            <sys-lib>rpcrt4</sys-lib>
            <sys-lib>advapi32</sys-lib>
            <sys-lib>wsock32</sys-lib>
            <sys-lib>odbc32</sys-lib>
        </if>

        <!-- Libs common to both borland and MSVC               -->
        <if cond="FORMAT=='msvc' or FORMAT=='msvc6prj' or FORMAT=='borland'">
            <sys-lib>oleacc</sys-lib>
*/     
    }
    
    std::string getAllLibs(Options& po)
    {
        std::string libs;
        libs += po["__WXLIB_ARGS_p"] + po["__WXLIB_OPENGL_p"] + po["__WXLIB_MEDIA_p"];
        libs += po["__WXLIB_DBGRID_p"] + po["__WXLIB_ODBC_p"] + po["__WXLIB_XRC_p"];
        libs += po["__WXLIB_QA_p"] + po["__WXLIB_AUI_p"] + po["__WXLIB_HTML_p"] + po["__WXLIB_ADV_p"];
        libs += po["__WXLIB_CORE_p"] + po["__WXLIB_XML_p"] + po["__WXLIB_NET_p"];
        libs += po["__WXLIB_BASE_p"] + po["__WXLIB_MONO_p"];
        libs += po["__LIB_TIFF_p"] + po["__LIB_JPEG_p"] + po["__LIB_PNG_p"];
        libs += po["__LIB_ZLIB_p"] + po["__LIB_REGEX_p"] + po["__LIB_EXPAT_p"];
        libs += po["EXTRALIBS_FOR_BASE"] + po["__UNICOWS_LIB_p"];
        libs += po["__LIB_KERNEL32_p"] + po["__LIB_USER32_p"] + po["__LIB_GDI32_p"];
        libs += po["__LIB_COMDLG32_p"] + po["__LIB_REGEX_p"] + po["__LIB_WINSPOOL_p"];
        libs += po["__LIB_WINMM_p"] + po["__LIB_SHELL32_p"] + po["__LIB_COMCTL32_p"];
        libs += po["__LIB_OLE32_p"] + po["__LIB_OLEAUT32_p"] + po["__LIB_UUID_p"];
        libs += po["__LIB_RPCRT4_p"] + po["__LIB_ADVAPI32_p"] + po["__LIB_WSOCK32_p"];
        libs += po["__LIB_ODBC32_p"];
        
        return libs;
    }

    void getVariablesValues(Options& po, const CmdLineOptions& cl, BuildFileOptions& cfg)
    {
        if (cl.keyExists("--variable"))
        {
            std::string var = cl.keyValue("--variable");
            
            if (po.keyExists(var))
                po["variable"] += "PO: " + var + "=" + po[var] + "\n";
            else
                po["variable"] += "PO: " + var + " does not exist.\n";

            if (cfg.keyExists(var))
                po["variable"] += "CFG: " + var + "=" + cfg[var] + "\n";
            else
                po["variable"] += "CFG: " + var + " does not exist.\n";
        }
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
        po["libs"] += getAllLibs(po);
        
        /*
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
*/
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
        
        getVariablesValues(po, cl, cfg);
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

//----------------------------------------------------

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_INFO"] == "default")
            po["__DEBUGINFO_0"] = addFlag("-g");

        if (cfg["BUILD"] == "release" && cfg["DEBUG_INFO"] == "default")
            po["__DEBUGINFO_0"] = addFlag("");

        if (cfg["DEBUG_INFO"] == "0")
            po["__DEBUGINFO_0"] = addFlag("");

        if (cfg["DEBUG_INFO"] == "1")
            po["__DEBUGINFO_0"] = addFlag("-g");

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_INFO"] == "default")
            po["__DEBUGINFO_1"] = addFlag("/DEBUG /CODEVIEW");

        if (cfg["BUILD"] == "release" && cfg["DEBUG_INFO"] == "default")
            po["__DEBUGINFO_1"] = addFlag("");

        if (cfg["DEBUG_INFO"] == "0")
            po["__DEBUGINFO_1"] = addFlag("");

        if (cfg["DEBUG_INFO"] == "1")
            po["__DEBUGINFO_1"] = addFlag("/DEBUG /CODEVIEW");
/*
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
*/
//----------------------------------------------------

        if (cfg["BUILD"] == "debug")
            po["__OPTIMIZEFLAG_4"] = addFlag("-o+none");//2

        if (cfg["BUILD"] == "release")
            po["__OPTIMIZEFLAG_4"] = addFlag("-o");//2
/*
        if (cfg["USE_THREADS"] == "0")
            po["__THREADSFLAG_7"] = "L";

        if (cfg["USE_THREADS"] == "1")
            po["__THREADSFLAG_7"] = "T";
*/
        if (cfg["RUNTIME_LIBS"] == "dynamic")
            po["__RUNTIME_LIBS_8"] = "-ND";//5 // TODO: addFlag?

        if (cfg["RUNTIME_LIBS"] == "static")
            po["__RUNTIME_LIBS_8"] = "";//5

//----------------------------------------------------

        if (cfg["USE_RTTI"] == "0")
            po["__RTTIFLAG_9"] = addFlag("");//6

        if (cfg["USE_RTTI"] == "1")
            po["__RTTIFLAG_9"] = addFlag("-Ar");//6

        if (cfg["USE_EXCEPTIONS"] == "0")
            po["__EXCEPTIONSFLAG_10"] = addFlag("");//7

        if (cfg["USE_EXCEPTIONS"] == "1")
            po["__EXCEPTIONSFLAG_10"] = addFlag("-Ae");//7

//----------------------------------------------------
/*
        if (cfg["BUILD"] == "debug" && cfg["DEBUG_RUNTIME_LIBS"] == "0")
            po["__NO_VC_CRTDBG_p"] = addDefine("__NO_VC_CRTDBG__");

        if (cfg["BUILD"] == "release" && cfg["DEBUG_FLAG"] == "1")
            po["__NO_VC_CRTDBG_p"] = addDefine("__NO_VC_CRTDBG__");

        if (cfg["BUILD"] == "debug" && cfg["DEBUG_RUNTIME_LIBS"] == "0")
            po["__NO_VC_CRTDBG_p_1"] = addResDefine("__NO_VC_CRTDBG__");

        if (cfg["BUILD"] == "release" && cfg["DEBUG_FLAG"] == "1")
            po["__NO_VC_CRTDBG_p_1"] = addResDefine("__NO_VC_CRTDBG__");
*/
        if (cfg["WXUNIV"] == "1")
            po["__WXUNIV_DEFINE_p"] = addDefine("__WXUNIVERSAL__");
/*
        if (cfg["WXUNIV"] == "1")
            po["__WXUNIV_DEFINE_p_1"] = addResDefine("__WXUNIVERSAL__");
*/
        if (cfg["BUILD"] == "debug" && cfg["DEBUG_FLAG"] == "default")
            po["__DEBUG_DEFINE_p"] = addDefine("__WXDEBUG__");

        if (cfg["DEBUG_FLAG"] == "1")
            po["__DEBUG_DEFINE_p"] = addDefine("__WXDEBUG__");
/*
        if (cfg["BUILD"] == "debug" && cfg["DEBUG_FLAG"] == "default")
            po["__DEBUG_DEFINE_p_1"] = addResDefine("__WXDEBUG__");

        if (cfg["DEBUG_FLAG"] == "1")
            po["__DEBUG_DEFINE_p_1"] = addResDefine("__WXDEBUG__");
*/
        if (cfg["USE_EXCEPTIONS"] == "0")
            po["__EXCEPTIONS_DEFINE_p"] = addDefine("wxNO_EXCEPTIONS");
/*
        if (cfg["USE_EXCEPTIONS"] == "0")
            po["__EXCEPTIONS_DEFINE_p_1"] = addResDefine("wxNO_EXCEPTIONS");
*/
        if (cfg["USE_RTTI"] == "0")
            po["__RTTI_DEFINE_p"] = addDefine("wxNO_RTTI");
/*
        if (cfg["USE_RTTI"] == "0")
            po["__RTTI_DEFINE_p_1"] = addResDefine("wxNO_RTTI");
*/
        if (cfg["USE_THREADS"] == "0")
            po["__THREAD_DEFINE_p"] = addDefine("wxNO_THREADS");
/*
        if (cfg["USE_THREADS"] == "0")
            po["__THREAD_DEFINE_p_1"] = addResDefine("wxNO_THREADS");
*/
        if (cfg["UNICODE"] == "1")
            po["__UNICODE_DEFINE_p"] = addDefine("_UNICODE");
/*
        if (cfg["UNICODE"] == "1")
            po["__UNICODE_DEFINE_p_1"] = addResDefine("_UNICODE");
*/
        if (cfg["MSLU"] == "1")
            po["__MSLU_DEFINE_p"] = addDefine("wxUSE_UNICODE_MSLU=1");
/*
        if (cfg["MSLU"] == "1")
            po["__MSLU_DEFINE_p_1"] = addResDefine("wxUSE_UNICODE_MSLU=1");
*/
        if (cfg["SHARED"] == "1")
            po["__DLLFLAG_p"] = addDefine("WXUSINGDLL");
/*
        if (cfg["SHARED"] == "1")
            po["__DLLFLAG_p_1"] = addResDefine("WXUSINGDLL");
*/
        process_3(po, cl, cfg);


        // ### Variables, Part 2: ###
        po["LIBDIRNAME"] = po["prefix"] + "\\lib\\" + getName() + "_" + po["LIBTYPE_SUFFIX"] + cfg["CFG"];

        po["SETUPHDIR"]  = po["LIBDIRNAME"] + "\\" + po["PORTNAME"] + po["WXUNIVNAME"];
        po["SETUPHDIR"] += po["WXUNICODEFLAG"] + po["WXDEBUGFLAG"];

        po["cflags"]  = easyMode(po["__DEBUGINFO_0"]) + easyMode(po["__OPTIMIZEFLAG_4"]);
        po["cflags"] += po["__RUNTIME_LIBS_8"] + " " + addDefine("_WIN32_WINNT=0x0400");
        po["cflags"] += addDefine("__WXMSW__") + po["__WXUNIV_DEFINE_p"];
        po["cflags"] += po["__DEBUG_DEFINE_p"] + po["__EXCEPTIONS_DEFINE_p"] + po["__RTTI_DEFINE_p"];
        po["cflags"] += po["__THREAD_DEFINE_p"] + po["__UNICODE_DEFINE_p"] + po["__MSLU_DEFINE_p"];
        po["cflags"] += addIncludeDir(po["SETUPHDIR"]) + addIncludeDir(po["prefix"] + "\\include") + easyMode(addFlag("-w-")) + easyMode(addIncludeDir(".")) + po["__DLLFLAG_p"] + easyMode(addFlag("-WA"));
        po["cflags"] += easyMode(addIncludeDir(po["prefix"] + "\\samples")) + easyMode(addDefine("NOPCH")) + po["__RTTIFLAG_9"] + po["__EXCEPTIONSFLAG_10"];
        po["cflags"] += cfg["CPPFLAGS"] + " " + cfg["CXXFLAGS"] + " ";



        po["libs"]  = easyMode(addFlag("/NOLOGO")) + easyMode(addFlag("/SILENT"));
        po["libs"] += easyMode(addFlag("/NOI")) + easyMode(addFlag("/DELEXECUTABLE"));
        po["libs"] += easyMode(addFlag("/EXETYPE:NT"));
        po["libs"] += cfg["LDFLAGS"] + " ";
        po["libs"] += easyMode(po["__DEBUGINFO_1"]);
        po["libs"] += addLinkerDir(po["LIBDIRNAME"] + "\\");
        po["libs"] += easyMode(addFlag("/su:windows:4.0"));
        po["libs"] += getAllLibs(po);        

        po["rcflags"]  = addResDefine("_WIN32_WINNT=0x0400") + addResDefine("__WXMSW__");
        po["rcflags"] += po["__WXUNIV_DEFINE_p"] + po["__DEBUG_DEFINE_p"] + po["__EXCEPTIONS_DEFINE_p"];
        po["rcflags"] += po["__RTTI_DEFINE_p"] + po["__THREAD_DEFINE_p"] + po["__UNICODE_DEFINE_p"];
        po["rcflags"] += po["__MSLU_DEFINE_p"] + addResIncludeDir(po["SETUPHDIR"]);
        po["rcflags"] += addResIncludeDir(po["prefix"] + "\\include") + easyMode(addResIncludeDir("."));
        po["rcflags"] += po["__DLLFLAG_p"];
        po["rcflags"] += easyMode(addResIncludeDir(po["prefix"] + "\\samples"));
        po["rcflags"] += easyMode(addFlag("-32")) + easyMode(addFlag("-v-"));


        po["release"] = cfg["WXVER_MAJOR"] + "." + cfg["WXVER_MINOR"];
        po["version"] = cfg["WXVER_MAJOR"] + "." + cfg["WXVER_MINOR"] + "." + cfg["WXVER_RELEASE"];
        po["cc"] = m_programs.cc;
        po["cxx"] = m_programs.cxx;
        po["ld"] = m_programs.ld;
        
        getVariablesValues(po, cl, cfg);
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
        po["libs"] += getAllLibs(po);

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
        
        getVariablesValues(po, cl, cfg);
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
        po["libs"] += getAllLibs(po);

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
        
        getVariablesValues(po, cl, cfg);
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
    std::string::iterator lastChar = path.end() - 1;
    if (*lastChar == '\\')
        path.erase(lastChar);
}

// -------------------------------------------------------------------------------------------------

void autodetectConfiguration(Options& po)
{
    // TODO: instead of this prebuild list that doesn't even take in account
    // checkAdditionalFlags() and that doesn't admits multiple wx configurations
    // a "dir list" needs to be done


    std::vector<std::string> cfgs;
    /// Searchs for '<prefix>\lib\gcc_[dll|lib][CFG]\[msw|base][univ][u][d]\\wx\setup.h' next
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

    /// Searchs for '<prefix>\lib\dmc_lib\mswd\wx\setup.h' next
    // TODO: account for $(CFG)
    cfgs.push_back("dmc_dll\\msw");
    cfgs.push_back("dmc_dll\\mswu");
    cfgs.push_back("dmc_dll\\mswud");
    cfgs.push_back("dmc_dll\\mswd");
    cfgs.push_back("dmc_dll\\mswuniv");
    cfgs.push_back("dmc_dll\\mswunivu");
    cfgs.push_back("dmc_dll\\mswunivud");
    cfgs.push_back("dmc_lib\\msw");
    cfgs.push_back("dmc_lib\\mswu");
    cfgs.push_back("dmc_lib\\mswud");
    cfgs.push_back("dmc_lib\\mswd");
    cfgs.push_back("dmc_lib\\mswuniv");
    cfgs.push_back("dmc_lib\\mswunivu");
    cfgs.push_back("dmc_lib\\mswunivud");
    cfgs.push_back("dmc_dll\\base");
    cfgs.push_back("dmc_dll\\baseu");
    cfgs.push_back("dmc_dll\\baseud");
    cfgs.push_back("dmc_dll\\based");
    cfgs.push_back("dmc_dll\\baseuniv");
    cfgs.push_back("dmc_dll\\baseunivu");
    cfgs.push_back("dmc_dll\\baseunivud");
    cfgs.push_back("dmc_lib\\base");
    cfgs.push_back("dmc_lib\\baseu");
    cfgs.push_back("dmc_lib\\baseud");
    cfgs.push_back("dmc_lib\\based");
    cfgs.push_back("dmc_lib\\baseuniv");
    cfgs.push_back("dmc_lib\\baseunivu");
    cfgs.push_back("dmc_lib\\baseunivud");

    /// Searchs for '<prefix>\lib\vc[cpu]_[dll|lib][CFG]\[msw|base][univ][u][d]\wx\setup.h' next
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

    /// Searchs for '<prefix>\lib\wat_[dll|lib][CFG]\[msw|base][univ][u][d]\wx\setup.h' next
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

    // reads the first setup.h it founds
    for(std::vector<std::string>::const_iterator it = cfgs.begin(); it != cfgs.end(); ++it)
    {
        std::string file = po["prefix"] + "\\lib\\" + *it + "\\wx\\setup.h";
        std::ifstream setupH(file.c_str());
        if (setupH.is_open())
        {
            if (!po.keyExists("wxcfg"))
                po["wxcfg"] = *it;
            else
            {
                std::cout << g_tokWarning << "multiple compiled configurations of wxWidgets have been detected." << std::endl;
                std::cerr << std::endl;
                std::cerr << "Please use the --wxcfg flag (as in wx-config --wxcfg=gcc_dll\\mswud)" << std::endl;
                std::cerr << "or set the environment variable WXCFG (as in WXCFG=gcc_dll\\mswud)" << std::endl;
                std::cerr << "to specify which configuration exactly you want to use." << std::endl;

                exit(1);
            }
        }
    }

    if (!po.keyExists("wxcfg"))
    {
        // TODO: this never reaches thanks to the new autodetection algorithm

        std::cout << g_tokError << "No setup.h file has been auto-detected." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Please use the --wxcfg flag (as in wx-config --wxcfg=gcc_dll\\mswud)" << std::endl;
        std::cerr << "or set the environment variable WXCFG (as in WXCFG=gcc_dll\\mswud)" << std::endl;
        std::cerr << "to specify which configuration exactly you want to use." << std::endl;

        exit(1);
    }
}

// -------------------------------------------------------------------------------------------------

void replaceCompilerIfFound(std::string& wxcfg, const std::string& compiler)
{
    std::vector<std::string> compilers;
    compilers.push_back("gcc_");
    compilers.push_back("dmc_");
    compilers.push_back("vc_");
    compilers.push_back("wat_");
    compilers.push_back("bcc_");

    for(std::vector<std::string>::const_iterator it = compilers.begin(); it != compilers.end(); ++it)
    {
        size_t loc = wxcfg.find(*it);
        if (loc != std::string::npos)
            wxcfg.replace(loc, (*it).length(), compiler + "_");
    }
}

// -------------------------------------------------------------------------------------------------

void checkAdditionalFlags(Options& po, const CmdLineOptions& cl)
{
    /// Modifies wxcfg as 'vc[cpu]_[dll|lib][CFG]\[msw|base][univ][u][d]' accordingly
    /// or 'gcc_[dll|lib][CFG]\[msw|base][univ][u][d]'

    if (cl.keyExists("--universal"))
    {
        if (cl.keyValue("--universal") == "no")
        {
            // Pattern: Remove /univ/ in /(msw|base)univ/

            const std::string univStr("univ");
            size_t univPos = po["wxcfg"].rfind(univStr);
            if (univPos != std::string::npos)
                po["wxcfg"].erase(univPos, univStr.length());
        }
        else if (cl.keyValue("--universal") == "yes" || cl.keyValue("--universal").empty())
        {
            // Pattern: Replace /(msw|base)/ to /(msw|base)univ/

            const std::string univStr("univ");
            size_t univPos = po["wxcfg"].rfind(univStr);
            if (univPos == std::string::npos)
            {
                const std::string mswStr("msw");
                size_t mswPos = po["wxcfg"].rfind(mswStr);
                if (mswPos != std::string::npos)
                    po["wxcfg"].insert(mswPos + mswStr.length(), univStr);
                else
                {
                    const std::string baseStr("base");
                    size_t basePos = po["wxcfg"].rfind(baseStr);
                    if (basePos != std::string::npos)
                        po["wxcfg"].insert(basePos + baseStr.length(), univStr);
                }
            }
        }
    }

    if (cl.keyExists("--unicode"))
    {
        if (cl.keyValue("--unicode") == "no")
        {
            // Pattern: Remove /.*u/ if it's present
            // or /.*ud/ if --debug is specified

            std::string::iterator lastChar = po["wxcfg"].end() - 1;
            if (*lastChar == 'u')
                po["wxcfg"].erase(lastChar);
            else if (*(lastChar - 1) == 'u' && *lastChar == 'd')
                po["wxcfg"].erase(lastChar - 1);
        }
        else if (cl.keyValue("--unicode") == "yes" || cl.keyValue("--unicode").empty())
        {
            // Pattern: Add /.*u/ if it's not already
            // or /.*ud/ if --debug is specified

            // TODO: string::find will be better
            std::string::iterator lastChar = po["wxcfg"].end() - 1;
            if (*lastChar != 'u' && *lastChar != 'd')
                po["wxcfg"] += "u";
            else if (*(lastChar - 1) != 'u' && *lastChar == 'd')
            {
                *lastChar = 'u';
                po["wxcfg"] += "d";
            }
        }
    }

    if (cl.keyExists("--debug"))
    {
        if (cl.keyValue("--debug") == "no")
        {
            // Pattern: Remove /.*d/ if it's present
            std::string::iterator lastChar = po["wxcfg"].end() - 1;
            if (*lastChar == 'd')
                po["wxcfg"].erase(lastChar);
        }
        else if (cl.keyValue("--debug") == "yes" || cl.keyValue("--debug").empty())
        {
            // Pattern: Add /.*d/ if it's not already
            std::string::iterator lastChar = po["wxcfg"].end() - 1;
            if (*lastChar != 'd')
                po["wxcfg"] += "d";
        }
    }

    if (cl.keyExists("--static"))
    {
        if (cl.keyValue("--static") == "no")
        {
            // Pattern: Replace /.*_lib/ to /.*_dll/

            size_t loc = po["wxcfg"].find("_lib");
            if (loc != std::string::npos)
                po["wxcfg"].replace(loc, std::string("_dll").length(), "_dll");
        }
        else if (cl.keyValue("--static") == "yes" || cl.keyValue("--static").empty())
        {
            // Pattern: Replace /.*_dll/ to /.*_lib/

            size_t loc = po["wxcfg"].find("_dll");
            if (loc != std::string::npos)
                po["wxcfg"].replace(loc, std::string("_lib").length(), "_lib");
        }
    }

    if (cl.keyExists("--compiler"))
    {
        if (cl.keyValue("--compiler") == "gcc")
            replaceCompilerIfFound(po["wxcfg"], "gcc");
        else if (cl.keyValue("--compiler") == "dmc")
            replaceCompilerIfFound(po["wxcfg"], "dmc");
        else if (cl.keyValue("--compiler") == "vc")
            replaceCompilerIfFound(po["wxcfg"], "vc");
        else if (cl.keyValue("--compiler") == "wat")
            replaceCompilerIfFound(po["wxcfg"], "wat");
        else if (cl.keyValue("--compiler") == "bcc")
            replaceCompilerIfFound(po["wxcfg"], "bcc");
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
        // TODO: this never reaches thanks to the new autodetection algorithm

        std::cout << g_tokError << "No supported compiler has been detected in the configuration '" << po["wxcfg"] << "'." << std::endl;
        std::cerr << std::endl;
        std::cerr << "The specified wxcfg must start with a 'gcc_', 'dmc_' or 'vc_'" << std::endl;
        std::cerr << "to be successfully detected." << std::endl;

        exit(1);
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
        std::cout << g_tokError << "wxWidgets hasn't been found installed at '" << prefix << "'." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Please use the --prefix flag (as in wx-config --prefix=C:\\wxWidgets)" << std::endl;
        std::cerr << "or set the environment variable WXWIN (as in WXWIN=C:\\wxWidgets)" << std::endl;
        std::cerr << "to specify where is your installation of wxWidgets." << std::endl;

        exit(1);
    }
}

// -------------------------------------------------------------------------------------------------

bool validateConfiguration(const std::string& wxcfgfile, const std::string& wxcfgsetuphfile, bool exitIfError = true)
{
    std::ifstream build_cfg(wxcfgfile.c_str());
    std::ifstream setup_h(wxcfgsetuphfile.c_str());

    bool isBuildCfgOpen = build_cfg.is_open();
    bool isSetupHOpen = setup_h.is_open();

    if (!isBuildCfgOpen && exitIfError)
    {
        if (!isSetupHOpen && exitIfError)
        {
            std::cout << g_tokError << "No valid setup.h of wxWidgets has been found at location: " << wxcfgsetuphfile << std::endl;
            std::cerr << std::endl;
            std::cerr << "Please use the --wxcfg flag (as in wx-config --wxcfg=gcc_dll\\mswud)" << std::endl;
            std::cerr << "or set the environment variable WXCFG (as in WXCFG=gcc_dll\\mswud)" << std::endl;
            std::cerr << "to specify which configuration exactly you want to use." << std::endl;

            exit(1);
        }

        std::cout << g_tokError << "No valid configuration of wxWidgets has been found at location: " << wxcfgfile << std::endl;
        std::cerr << std::endl;
        std::cerr << "Please use the --wxcfg flag (as in wx-config --wxcfg=gcc_dll\\mswud)" << std::endl;
        std::cerr << "or set the environment variable WXCFG (as in WXCFG=gcc_dll\\mswud)" << std::endl;
        std::cerr << "to specify which configuration exactly you want to use." << std::endl;

        exit(1);
    }
    return isBuildCfgOpen && isSetupHOpen;
}

// -------------------------------------------------------------------------------------------------

void outputFlags(Options& po, const CmdLineOptions& cl)
{
    /// Outputs flags to console
    if (cl.keyExists("--variable"))
    {
        std::cout << po["variable"];
        exit(0);
    }
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

    if (cl.keyExists("-v"))
    {
        std::cout << "wx-config revision " << getSvnRevision() << " " << getSvnDate() << std::endl;
        return 0;
    }

    checkEasyMode(cl);

    if (cl.keyExists("--prefix"))
        po["prefix"] = cl["--prefix"];
    else if (getenv("WXWIN"))
        po["prefix"] = getenv("WXWIN");
    else
        po["prefix"] = "C:\\wxWidgets";

    normalizePath(po["prefix"]);

    validatePrefix(po["prefix"]);

    if (cl.keyExists("--wxcfg"))
        po["wxcfg"] = cl["--wxcfg"];
    else if (getenv("WXCFG") && !cl.keyExists("--prefix"))
        po["wxcfg"] = getenv("WXCFG");
    else
    {
        // TODO: this behaviour could be better, directory listing is required

        // Try if something valid can be found trough deriving checkAdditionalFlags() first
        po["wxcfg"] = "gcc_dll\\msw";
        po["wxcfgfile"] = po["prefix"] + "\\lib\\" + po["wxcfg"] + "\\build.cfg";
        po["wxcfgsetuphfile"] = po["prefix"] + "\\lib\\" + po["wxcfg"] + "\\wx\\setup.h";
        checkAdditionalFlags(po, cl);

        if (!validateConfiguration(po["wxcfgfile"], po["wxcfgsetuphfile"], false))
            autodetectConfiguration(po);    // important function
    }

    normalizePath(po["wxcfg"]);
    checkAdditionalFlags(po, cl);
    po["wxcfgfile"] = po["prefix"] + "\\lib\\" + po["wxcfg"] + "\\build.cfg";
    po["wxcfgsetuphfile"] = po["prefix"] + "\\lib\\" + po["wxcfg"] + "\\wx\\setup.h";
    validateConfiguration(po["wxcfgfile"], po["wxcfgsetuphfile"]);
    
    detectCompiler(po, cl);
    outputFlags(po, cl);    
   
    return 0;
}
