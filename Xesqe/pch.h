#pragma once

// --- Windows e DirectX ---
#include <windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include "d3dx12.h" // Utilit�rios do DirectX 12 (d3dx12.h)

// --- Biblioteca Padr�o C++ ---
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <exception>
#include <array>
#include <tuple> // Inclu�do para std::tuple usado no parser de OBJ

// --- Depura��o ---
#if defined(DEBUG) || defined(_DEBUG)
#include <dxgidebug.h>
#endif

// --- Utilit�rios do Projeto ---
#include "Exception.h" // Sua classe de exce��o personalizada

// --- Linker Directives ---
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")