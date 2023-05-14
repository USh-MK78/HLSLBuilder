#define _WIN32_WINNT 0x600
#include <stdio.h>
#include <string>
#include <atlstr.h>
#include <iostream>
#include <msclr/marshal_cppstd.h>
#include <vcclr.h>
#include <d3dcompiler.h>

#using <System.Xml.dll>
#using <System.Xml.Linq.dll>

#pragma comment(lib, "d3dcompiler.lib" )

HRESULT CompileShader(_In_ LPCWSTR srcFile, _In_ LPCSTR entryPoint, _In_ LPCSTR profile, _Outptr_ ID3DBlob** blob)
{
	if (!srcFile || !entryPoint || !profile || !blob) return E_INVALIDARG;

	*blob = nullptr;

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG;
#endif

	const D3D_SHADER_MACRO defines[] = { "EXAMPLE_DEFINE", "1", NULL, NULL };

	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT res = D3DCompileFromFile(srcFile, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, profile, flags, 0, &shaderBlob, &errorBlob);
	if (FAILED(res))
	{
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}

		if (shaderBlob) shaderBlob->Release();
		return res;
	}

	*blob = shaderBlob;

	//Create *.cso
	pin_ptr<const wchar_t> cso_Path = PtrToStringChars(gcnew System::String(srcFile) + ".cso");
	HRESULT w = D3DWriteBlobToFile(shaderBlob, cso_Path, true);
	if (FAILED(w))
	{
		std::string* d = new std::string((char*)cso_Path);
		printf(*"FAILED : " + d->c_str());
		return w;
	}

	return res;
}

using namespace System::Xml;
using namespace System::Xml::Linq;

int main(int argc, char* argv[])
{
	if (!(argc > 1) || argv[0] == "HLSLBuilder.exe" || argv[0] == "HLSLBuilder")
	{
		printf("\r\n");
		printf("Since we have not specified the xml containing the HLSL file path to build, we are starting default mode.\r\n");
		printf("\r\n");
		printf("HLSL Builder v1.0\r\n");
		printf("[ Command list ]\r\n");
		printf("HLSLBuilder.exe *.xml\r\n");
		return 0;
	}

	std::string Path = std::string(argv[1]);
	XDocument^ hlsl_xml = XDocument::Load(msclr::interop::marshal_as<System::String^>(Path));

	XElement^ hlsl_root = hlsl_xml->Element(hlsl_xml->Root->Name);
	System::Collections::Generic::IEnumerable<XElement^>^ ElementData = hlsl_root->Elements();

	for each (XElement ^ data in ElementData)
	{
		//LPCWSTR
		pin_ptr<const wchar_t> sh_Path = new wchar_t();

		//LPCSTR
		std::string entryPoint = std::string();
		std::string shader_profile = std::string();

		System::Collections::Generic::IEnumerable<XElement^>^ chNodes = data->Elements();
		for each (XElement ^ ch_Nodes in chNodes)
		{
			if (ch_Nodes->Name == "ShaderPath")
			{
				sh_Path = PtrToStringChars(data->Element(ch_Nodes->Name)->Value);
			}
			if (ch_Nodes->Name == "EntryPointName")
			{
				entryPoint = msclr::interop::marshal_as<std::string>(data->Element(ch_Nodes->Name)->Value);
			}
			if (ch_Nodes->Name == "Profile")
			{
				shader_profile = msclr::interop::marshal_as<std::string>(data->Element(ch_Nodes->Name)->Value); //vs_4_0_level_9_1 etc...
			}
		}

		ID3DBlob* shBlob = nullptr;
		HRESULT ShaderResult = CompileShader(sh_Path, entryPoint.c_str(), shader_profile.c_str(), &shBlob);

		if (FAILED(ShaderResult))
		{
			printf("Failed compiling HLSL shader | Code: %08X\r\n", ShaderResult);
			printf("*.hlsl Path :" + CStringA(sh_Path) + "\r\n");
			printf("EntryPoint :" + CStringA(entryPoint.c_str()) + "\r\n");
			printf("Profile :" + CStringA(shader_profile.c_str()) + "\r\n");

		}
		else
		{
			printf("Successfuled compiling HLSL shader | Code: %08X\r\n", ShaderResult);
			printf("*.hlsl Path :" + CStringA(sh_Path) + "\r\n");
			printf("EntryPoint :" + CStringA(entryPoint.c_str()) + "\r\n");
			printf("Profile :" + CStringA(shader_profile.c_str()) + "\r\n");
		}

		shBlob = nullptr;
	}

	return 0;
}

