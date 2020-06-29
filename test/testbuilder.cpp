#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <ctype.h>
#include <map>

// ---------------------------------------------------------------------------------------------------------------------
// TODO
// 1. Make 'ModifiedMethodCall' more robust
// 2. 



// ---------------------------------------------------------------------------------------------------------------------
// Terminology
//   methodCall: MethodName(Params)

inline const std::string src_path = "./src/";
inline const std::string c_comment = "\n// generated by TestBuilder\n";
inline const std::string c_tabString = "    ";

namespace TestBuild
{
    std::string currentMethodCall;
    bool insideMethodCall = false;
    std::string outputBuffer;
    int contextLevel;
    int baseContextLevel = 0;
    bool insideMethodDef = false;

    bool AddTrace = false;


    /// true if [c] is a letter, number, or underscore
    bool IsNameComp(char c)
    {
        return std::isalnum(c) || c == '_';
    }

    /// true if line is an assignment operation
    bool LineIsAssignment(const std::string& line)
    {
        return line.find("=") != std::string::npos;
    }

    /// true if line is a typedef operation
    bool LineIsTypeDef(const std::string& line)
    {
        return line.find("typedef") != std::string::npos;
    }

    /// true if line is a template operation
    bool LineIsTemplate(const std::string& line)
    {
        return line.find("template") != std::string::npos;
    }

    // TODO: this is slighly buggy
    /// changes the baseContext level depending on whether the reader is inside a namespace/class
    void ChangeBaseContextLevel(const std::string& line)
    {
        if(line.find("namespace") != std::string::npos && line.find("using") == std::string::npos)
        {
            baseContextLevel = baseContextLevel + 1;
            return;
        }
        // if(contextLevel < baseContextLevel)
        //     baseContextLevel = contextLevel;
    }

    /// sets insideMethodCall to true if the line is the method call part of a method definition or was in a method call
    /// previously
    void LineBeginsMethodCall(const std::string& line)
    {
        insideMethodCall = insideMethodCall || (
            ((contextLevel == baseContextLevel && line.find("(") != std::string::npos) || LineIsTemplate(line))
            && !LineIsAssignment(line) 
            && !LineIsTypeDef(line));
    }

    /// adds whatever part of [line] is a method call to currentMethod and returns true if [excess] is not empty
    /// where [excess] stores line information not part of the method definition. assumes that the reader is
    /// currently parsing through a method definition
    bool MethodCallEnds(const std::string& line, std::string& excess)
    {
        size_t endPos = line.find("{");

        /// if the method block start is found
        if(endPos != std::string::npos)
        {
            insideMethodCall = false;
            insideMethodDef = true;

            currentMethodCall.append(line.c_str(), endPos);
            excess = line.substr(endPos, std::string::npos);

            return true;
        }
        else
        {
            currentMethodCall.append(line);
            currentMethodCall.append("\n");
        return false;
        }
    }

    /// updates the global contextLevel based on what block the head is in
    void ChangeContextLevel(const std::string& line)
    {
        for(auto it = line.begin(); it != line.end(); it++)
        {
            if(*it == '{')
                contextLevel++;
            else if(*it == '}')
                contextLevel--;
        }
    }


    /// makes the currentMethod into an internal definition (i.e. {METHOD_NAME}_INTERNAL)
    std::string ModifiedMethodCall()
    {
        std::string newMethodCall;
        for(size_t i=0; i<currentMethodCall.size(); i++)
        {
            if(currentMethodCall.at(i) == '(')
                newMethodCall += "_INTERNAL";
            newMethodCall += currentMethodCall.at(i);
        }

        return newMethodCall;
    }

    /// return the first contiguous name starting before [pos] in [str]
    std::string GetNameBeforePos(const std::string& str, int pos)
    {   
        std::string name;
        name.reserve(32);

        int i;
        for(i=pos; i>=0 && !IsNameComp(str.at(i)); i--);        // move to end of first 'name' before pos
        for(; i>=0 && IsNameComp(str.at(i)); i--);              // move to head of first 'name' before pos
        i++;                                                    // on first char of 'name'
        for(; static_cast<size_t>(i)<str.size() && IsNameComp(str.at(i)); i++)
            name += str.at(i);

        return name;
    }

    /// returns the name a method from a methodCall
    std::string MethodName(const std::string& methodCall)
    {
        std::string methodName;
        int parenPos = methodCall.find('(');

        return GetNameBeforePos(methodCall, parenPos);
    }

    /// gets the next parameter and sets it to the next non-space symbol
    std::string GetParam(const std::string& methodCall, size_t& i)
    {
        i++;
        while(i < methodCall.size() && methodCall.at(i) != ',' && methodCall.at(i) != ')') i++;

        return GetNameBeforePos(methodCall, i);
    }

    /// true if a methodCall takes no parameters
    bool HasNoParams(const std::string& methodCall)
    {
        size_t i = methodCall.find('(');
        while(++i<methodCall.size() && methodCall.at(i) != ')')
        {
            if(IsNameComp(methodCall.at(i)))
                return false;
        }

        return true;
    }

    std::vector<std::string> MethodParams(const std::string& methodCall)
    {
        std::vector<std::string> params;

        if(HasNoParams(methodCall))
            return params;

        size_t i = methodCall.find("(");
        while(methodCall.at(i) != ')')
        {
            params.push_back(GetParam(methodCall, i));
        }
        
        return params;
    }

    /// true if exited definition block of method
    bool LineFinishesMethodDef(const std::string& line)
    {
        // will be true if returns to baseContextLevel and was previously insideMethodDef
        if(contextLevel == baseContextLevel && insideMethodDef)
        {
            insideMethodDef = false;
            return true;
        }
        return false;
    }

    /// true if method has return type (non-void)
    bool NonVoidReturn(const std::string& call)
    {
        return call.substr(0, call.find("(")).find("void") == std::string::npos;
    }

    /// removes anything past the right paren of a method call
    std::string CleanRightOfMethodCall(const std::string str)
    {
        int i;
        for(i=str.size()-1; i >=0 && str.at(i) != ')'; i--);
        return str.substr(0, i+1);
    }

    /// returns a declaration for the original method
    std::string OriginalMethodDeclaration()
    {
        return "\n" + CleanRightOfMethodCall(currentMethodCall);
    }

    /// generates a method call for the internal method
    std::string GenerateInternalMethodCall()
    {
        std::string methodInterface = c_comment + currentMethodCall;

        std::vector<std::string> methodParams = MethodParams(currentMethodCall);
        std::string modifiedMethodCall = MethodName(ModifiedMethodCall());

        if(methodParams.size() == 0)
            modifiedMethodCall += "();\n";
        else
        {
            modifiedMethodCall += "(" + methodParams.at(0);

            for(size_t i=1; i<methodParams.size(); i++)
                modifiedMethodCall += ", " + methodParams.at(i);

            modifiedMethodCall += ");\n";
        }

        return modifiedMethodCall;            
    }

    /// generates a return to the internal method call
    std::string GenerateReturnForInternalMethodCall()
    {
        if(NonVoidReturn(currentMethodCall))
            return "return " + GenerateInternalMethodCall();
        return GenerateInternalMethodCall();
    }

    std::string GenerateTraceLine()
    {
        if(!AddTrace)
            return "";

        auto methodName = MethodName(currentMethodCall);
        return c_tabString + "LogIt(LogSeverityType::Sev3_Critical, \"" + methodName + "\", \"\");\n";
    }

    std::string GenerateFunctionInjectPoint()
    {
        auto methodName = MethodName(currentMethodCall);
        auto params = MethodParams(currentMethodCall);

        auto str = c_tabString + "if(FunctionInjections.count(\"" + methodName +"\") == 1){\n";
        str += c_tabString + "Params p;\n";

        for(auto param: params)
        {
            str += c_tabString + "p.push_back(static_cast<const void*>(&" + param + "));\n";
        }

        str += c_tabString + "FunctionInjections[\"" + methodName +"\"](p);\n";
        str += c_tabString + "}\n";
        return str;
    }

    std::string GenerateHitCounter()
    {
        auto methodName = MethodName(currentMethodCall);
        return "methodHitMap[\"" + methodName + "\"]++;\n";
    }

    /// generates the stubbed method interface
    std::string GenerateStubbedInterface()
    {
        std::string methodInterface = c_comment + currentMethodCall;
        methodInterface += "{\n";

        methodInterface += GenerateTraceLine();
        methodInterface += c_tabString + GenerateHitCounter();
        methodInterface += GenerateFunctionInjectPoint();
        methodInterface += c_tabString + GenerateReturnForInternalMethodCall();

        methodInterface += "}\n";
        return methodInterface;
    }

    /// adds the stubbed method public interface to the buffer
    void BufferMethodInterface()
    {
        std::string methodInterface = GenerateStubbedInterface();

        outputBuffer.append(methodInterface.c_str(), methodInterface.size());   
        currentMethodCall.clear();
    }

    /// true if [line] is entirely a comment
    bool LineIsComment(const std::string& line)
    {
        size_t i;
        for(i=0; i<line.size() && line.at(i) == ' '; i++);
        return i+1 < line.size() && line.at(i) == '/' && line.at(i+1) == '/';
    }

    /// stubs all methods in [filename] so that all methods become {METHOD_NAME}_INTERNAL
    void StubFile(std::string filename)
    {
        outputBuffer.reserve(2048);
        currentMethodCall.reserve(256);

        std::fstream file;
        file.open(src_path + filename, std::ios::in);
        
        std::fstream outFile;
        outFile.open("./test/src/" + filename, std::ios::out);

        std::string testInclude = "#include \"..\\test.h\"\n";
        outFile.write(testInclude.c_str(), testInclude.size());
        
        std::string line;
        line.reserve(256);
        while(std::getline(file, line))
        {
            ChangeBaseContextLevel(line);                                       // changes baseContextLevel as necessary

            if(LineIsComment(line))                                             // skip comments
                continue;

            LineBeginsMethodCall(line);                                         // decide if the line begins a methodCall

            std::string excess;
            if(insideMethodCall)
            {
                if(MethodCallEnds(line, excess))                                // if the method call ends, add the modified
                {                                                               // version to the buffer
                    outputBuffer.append("\n");
                    outputBuffer.append(ModifiedMethodCall());
                    outputBuffer.append(excess); 
                    outputBuffer.append("\n");
                }
            }
            else                                                                // if not a method call, just buffer line
            {
                outputBuffer.append(line.c_str(), line.size());
                outputBuffer.append("\n");
            }

            ChangeContextLevel(line);                                           // changes context level by presence of { }
            if(LineFinishesMethodDef(line))                                     
            {
                outputBuffer =  OriginalMethodDeclaration() +                   // add a declaration for the original method
                    ";\n" +                                                     // and then the modifed method to the buffer
                    outputBuffer;
                BufferMethodInterface();
            }

            if(!insideMethodDef)                                                // output content when exited method def and
            {                                                                   // clear buffer
                outFile.write(outputBuffer.c_str(), outputBuffer.size());
                outputBuffer.clear();
            }
        }
    }

    /// copy a file directly
    void CopyFile(const std::string& srcFile)
    {
        std::fstream file;
        file.open(src_path + srcFile, std::ios::in);
            
        std::fstream outFile;
        outFile.open("./test/src/" + srcFile, std::ios::out);
        
        std::string line;
        while(std::getline(file, line))
        {
            outFile.write(line.c_str(), line.size());
            outFile.write("\n", 1);
        }

        file.close();
        outFile.close();
    }

    void RewriteMain()
    {
        std::fstream file;
        file.open("./test/src/main.cpp", std::ios::out);

        std::fstream overrideFile;
        overrideFile.open("./assets/test_main.cpp", std::ios::in);

        std::string testInclude = "#include \"..\\test.h\"\n";
        file.write(testInclude.c_str(), testInclude.size());
 
        std::string line;
        while(std::getline(overrideFile, line))
        {
            file.write(line.c_str(), line.size());
            file.write("\n", 1);
        }

        file.close();
        overrideFile.close();
    }
} 

inline const std::string c_traceFlag = "--trace";

/// true it [str] is an h file
bool IsH(const std::string& str)
{
    return str.at(str.size()-1) == 'h';
}

/// special files get copied 
/// 1. utils get copied b/c it's a library
/// 2. diagnostics gets copied b/c use of VA args is not supported
bool IsSpecial(const std::string& str)
{
    return str == "utils.cpp" || str == "diagnostics.cpp";;
}

void HandleFlag(const std::string& str)
{
    if(str == c_traceFlag)
    {
        TestBuild::AddTrace = true;
    }
}

int main(int argc, char* argv[])
{
    for(int i=1; i<argc; i++)
    {
        std::string str = argv[i];
        HandleFlag(str);
    }
    for(int i=1; i<argc; i++)
    {
        std::string str = argv[i];
        if(IsH(str) || IsSpecial(str))
        {
            TestBuild::CopyFile(str);
        }
        else
        {
            TestBuild::StubFile(str);
        }
        
    }
    TestBuild::RewriteMain();
    std::cout << "finished\n";
    return 0;
}
