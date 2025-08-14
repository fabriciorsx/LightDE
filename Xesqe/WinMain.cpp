#include "pch.h"
#include "Application.h"
#include "Exception.h"

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
#if defined(DEBUG) || defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        Application theApp(hInstance);
        if (!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
    catch (std::exception& e)
    {
        MessageBoxA(nullptr, e.what(), "Error", MB_OK);
        return 0;
    }
}