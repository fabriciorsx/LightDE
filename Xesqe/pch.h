#pragma once

// --- Windows e DirectX ---
#include <windows.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include "d3dx12.h"
#include "Exception.h"

#include <string>
#include <memory>
#include <vector>
#include <stdexcept>
#include <exception>
#include <array>

#if defined(DEBUG) || defined(_DEBUG)
#include <dxgidebug.h>
#endif

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
