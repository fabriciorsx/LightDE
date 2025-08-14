#pragma once

// --- Windows e DirectX ---
#include <windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include "d3dx12.h" // Utilitários do DirectX 12 (d3dx12.h)

// --- Biblioteca Padrão C++ ---
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
#include <tuple> // Incluído para std::tuple usado no parser de OBJ

// --- Depuração ---
#if defined(DEBUG) || defined(_DEBUG)
#include <dxgidebug.h>
#endif

// --- Utilitários do Projeto ---
#include "Exception.h" // Sua classe de exceção personalizada

// --- Linker Directives ---
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")