#include "StdAfx.h"

#include <windows.h>

#include "DarkMode.h"

#include "IatHook.h"

#include <uxtheme.h>
#include <vssym32.h>

#include <unordered_set>
#include <mutex>

#if defined(__GNUC__) && (__GNUC__ > 8)
#define WINAPI_LAMBDA_RETURN(return_t) -> return_t WINAPI
#elif defined(__GNUC__)
#define WINAPI_LAMBDA_RETURN(return_t) WINAPI -> return_t
#else
#define WINAPI_LAMBDA_RETURN(return_t) -> return_t
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4191)
#endif

enum IMMERSIVE_HC_CACHE_MODE
{
	IHCM_USE_CACHED_VALUE,
	IHCM_REFRESH
};

// 1903 18362
enum class PreferredAppMode
{
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};

enum WINDOWCOMPOSITIONATTRIB
{
	WCA_UNDEFINED = 0,
	WCA_NCRENDERING_ENABLED = 1,
	WCA_NCRENDERING_POLICY = 2,
	WCA_TRANSITIONS_FORCEDISABLED = 3,
	WCA_ALLOW_NCPAINT = 4,
	WCA_CAPTION_BUTTON_BOUNDS = 5,
	WCA_NONCLIENT_RTL_LAYOUT = 6,
	WCA_FORCE_ICONIC_REPRESENTATION = 7,
	WCA_EXTENDED_FRAME_BOUNDS = 8,
	WCA_HAS_ICONIC_BITMAP = 9,
	WCA_THEME_ATTRIBUTES = 10,
	WCA_NCRENDERING_EXILED = 11,
	WCA_NCADORNMENTINFO = 12,
	WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
	WCA_VIDEO_OVERLAY_ACTIVE = 14,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
	WCA_DISALLOW_PEEK = 16,
	WCA_CLOAK = 17,
	WCA_CLOAKED = 18,
	WCA_ACCENT_POLICY = 19,
	WCA_FREEZE_REPRESENTATION = 20,
	WCA_EVER_UNCLOAKED = 21,
	WCA_VISUAL_OWNER = 22,
	WCA_HOLOGRAPHIC = 23,
	WCA_EXCLUDED_FROM_DDA = 24,
	WCA_PASSIVEUPDATEMODE = 25,
	WCA_USEDARKMODECOLORS = 26,
	WCA_LAST = 27
};

struct WINDOWCOMPOSITIONATTRIBDATA
{
	WINDOWCOMPOSITIONATTRIB Attrib;
	PVOID pvData;
	SIZE_T cbData;
};

template <typename P>
bool ptrFn(HMODULE handle, P& pointer, const char* name)
{
	auto p = reinterpret_cast<P>(::GetProcAddress(handle, name));
	if (p != nullptr)
	{
		pointer = p;
		return true;
	}
	return false;
}

template <typename P>
bool ptrFn(HMODULE handle, P& pointer, WORD index)
{
    return ptrFn(handle, pointer, MAKEINTRESOURCEA(index));
}

using fnRtlGetNtVersionNumbers = void (WINAPI *)(LPDWORD major, LPDWORD minor, LPDWORD build);
using fnSetWindowCompositionAttribute = BOOL (WINAPI *)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
// 1809 17763
using fnShouldAppsUseDarkMode = bool (WINAPI *)(); // ordinal 132
using fnAllowDarkModeForWindow = bool (WINAPI *)(HWND hWnd, bool allow); // ordinal 133
using fnAllowDarkModeForApp = bool (WINAPI *)(bool allow); // ordinal 135, in 1809
using fnFlushMenuThemes = void (WINAPI *)(); // ordinal 136
using fnRefreshImmersiveColorPolicyState = void (WINAPI *)(); // ordinal 104
using fnIsDarkModeAllowedForWindow = bool (WINAPI *)(HWND hWnd); // ordinal 137
using fnGetIsImmersiveColorUsingHighContrast = bool (WINAPI *)(IMMERSIVE_HC_CACHE_MODE mode); // ordinal 106
using fnOpenNcThemeData = HTHEME (WINAPI *)(HWND hWnd, LPCWSTR pszClassList); // ordinal 49
// 1903 18362
using fnShouldSystemUseDarkMode = bool (WINAPI *)(); // ordinal 138
using fnSetPreferredAppMode = PreferredAppMode (WINAPI *)(PreferredAppMode appMode); // ordinal 135, in 1903
using fnIsDarkModeAllowedForApp = bool (WINAPI *)(); // ordinal 139

static fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = nullptr;
static fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode = nullptr;
static fnAllowDarkModeForWindow _AllowDarkModeForWindow = nullptr;
static fnAllowDarkModeForApp _AllowDarkModeForApp = nullptr;
static fnFlushMenuThemes _FlushMenuThemes = nullptr;
static fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState = nullptr;
static fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow = nullptr;
static fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast = nullptr;
static fnOpenNcThemeData _OpenNcThemeData = nullptr;
// 1903 18362
//fnShouldSystemUseDarkMode _ShouldSystemUseDarkMode = nullptr;
fnSetPreferredAppMode _SetPreferredAppMode = nullptr;

bool g_darkModeSupported = false;
bool g_darkModeEnabled = false;
DWORD g_buildNumber = 0;

bool ShouldAppsUseDarkMode()
{
	if (!_ShouldAppsUseDarkMode)
	{
		return false;
	}

	return _ShouldAppsUseDarkMode();
}

bool AllowDarkModeForWindow(HWND hWnd, bool allow)
{
	if (g_darkModeSupported && _AllowDarkModeForWindow)
		return _AllowDarkModeForWindow(hWnd, allow);
	return false;
}

bool IsHighContrast()
{
	HIGHCONTRASTW highContrast{};
	highContrast.cbSize = sizeof(HIGHCONTRASTW);
	if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRASTW), &highContrast, FALSE))
		return (highContrast.dwFlags & HCF_HIGHCONTRASTON) == HCF_HIGHCONTRASTON;
	return false;
}

void SetTitleBarThemeColor(HWND hWnd, BOOL dark)
{
	if (g_buildNumber < 18362)
		SetPropW(hWnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<intptr_t>(dark)));
	else if (_SetWindowCompositionAttribute)
	{
		WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
		_SetWindowCompositionAttribute(hWnd, &data);
	}
}

void RefreshTitleBarThemeColor(HWND hWnd)
{
	BOOL dark = FALSE;
	if (_IsDarkModeAllowedForWindow && _ShouldAppsUseDarkMode)
	{
		if (_IsDarkModeAllowedForWindow(hWnd) && _ShouldAppsUseDarkMode() && !IsHighContrast())
		{
			dark = TRUE;
		}
	}

	SetTitleBarThemeColor(hWnd, dark);
}

bool IsColorSchemeChangeMessage(LPARAM lParam)
{
	bool is = false;
	if (lParam && (0 == lstrcmpi(reinterpret_cast<LPCWCH>(lParam), L"ImmersiveColorSet")) && _RefreshImmersiveColorPolicyState)
	{
		_RefreshImmersiveColorPolicyState();
		is = true;
	}
	if (_GetIsImmersiveColorUsingHighContrast)
		_GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
	return is;
}

bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam)
{
	if (message == WM_SETTINGCHANGE)
		return IsColorSchemeChangeMessage(lParam);
	return false;
}

void AllowDarkModeForApp(bool allow)
{
	if (_AllowDarkModeForApp)
		_AllowDarkModeForApp(allow);
	else if (_SetPreferredAppMode)
		_SetPreferredAppMode(allow ? PreferredAppMode::ForceDark : PreferredAppMode::Default);
}

void FlushMenuThemes()
{
	if (_FlushMenuThemes)
	{
		_FlushMenuThemes();
	}
}

// limit dark scroll bar to specific windows and their children

std::unordered_set<HWND> g_darkScrollBarWindows;
std::mutex g_darkScrollBarMutex;

void EnableDarkScrollBarForWindowAndChildren(HWND hwnd)
{
	std::lock_guard<std::mutex> lock(g_darkScrollBarMutex);
	g_darkScrollBarWindows.insert(hwnd);
}

bool IsWindowOrParentUsingDarkScrollBar(HWND hwnd)
{
	HWND hwndRoot = GetAncestor(hwnd, GA_ROOT);

	std::lock_guard<std::mutex> lock(g_darkScrollBarMutex);
	if (g_darkScrollBarWindows.count(hwnd)) {
		return true;
	}
	if (hwnd != hwndRoot && g_darkScrollBarWindows.count(hwndRoot)) {
		return true;
	}

	return false;
}

void FixDarkScrollBar()
{
	HMODULE hComctl = LoadLibraryEx(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hComctl)
	{
		auto addr = FindDelayLoadThunkInModule(hComctl, "uxtheme.dll", 49); // OpenNcThemeData
		if (addr)
		{
			DWORD oldProtect = 0;
			if (VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect) && _OpenNcThemeData)
			{
				auto MyOpenThemeData = [](HWND hWnd, LPCWSTR classList) WINAPI_LAMBDA_RETURN(HTHEME) {
					if (wcscmp(classList, WC_SCROLLBAR) == 0)
					{
						if (IsWindowOrParentUsingDarkScrollBar(hWnd)) {
							hWnd = nullptr;
							classList = L"Explorer::ScrollBar";
						}
					}
					return _OpenNcThemeData(hWnd, classList);
				};

				addr->u1.Function = reinterpret_cast<uintptr_t>(static_cast<fnOpenNcThemeData>(MyOpenThemeData));
				VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
			}
		}
		FreeLibrary(hComctl);
	}
}

constexpr bool CheckBuildNumber(DWORD buildNumber)
{
	return (buildNumber == 17763 || // 1809
		buildNumber == 18362 || // 1903
		buildNumber == 18363 || // 1909
		buildNumber == 19041 || // 2004
		buildNumber == 19042 || // 20H2
		buildNumber == 19043 || // 21H1
		buildNumber == 19044 || // 21H2
		(buildNumber > 19044 && buildNumber < 22000) || // Windows 10 any version > 21H2 
		buildNumber >= 22000);  // Windows 11 builds
}

bool IsWindows10() // or later OS version
{
	return (g_buildNumber >= 17763);
}

bool IsWindows11() // or later OS version
{
	return (g_buildNumber >= 22000);
}

DWORD GetWindowsBuildNumber()
{
	return g_buildNumber;
}

void InitDarkMode()
{
	static bool isInit = false;
	if (isInit)
		return;

	fnRtlGetNtVersionNumbers RtlGetNtVersionNumbers = nullptr;
	HMODULE hNtdllModule = GetModuleHandle(L"ntdll.dll");
	if (hNtdllModule)
	{
		ptrFn(hNtdllModule, RtlGetNtVersionNumbers, "RtlGetNtVersionNumbers");
	}

	if (RtlGetNtVersionNumbers)
	{
		DWORD major, minor;
		RtlGetNtVersionNumbers(&major, &minor, &g_buildNumber);
		g_buildNumber &= ~0xF0000000;
		if (major == 10 && minor == 0 && CheckBuildNumber(g_buildNumber))
		{
			HMODULE hUxtheme = LoadLibraryEx(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
			if (hUxtheme)
			{
				ptrFn(hUxtheme, _OpenNcThemeData, 49);
				ptrFn(hUxtheme, _RefreshImmersiveColorPolicyState, 104);
				ptrFn(hUxtheme, _GetIsImmersiveColorUsingHighContrast, 106);
				ptrFn(hUxtheme, _ShouldAppsUseDarkMode, 132);
				ptrFn(hUxtheme, _AllowDarkModeForWindow, 133);

				if (g_buildNumber < 18362)
					ptrFn(hUxtheme, _AllowDarkModeForApp, 135);
				else
					ptrFn(hUxtheme, _SetPreferredAppMode, 135);
				
				ptrFn(hUxtheme, _FlushMenuThemes, 136);
				ptrFn(hUxtheme, _IsDarkModeAllowedForWindow, 137);

				HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
				if (hUser32)
				{
					ptrFn(hUser32, _SetWindowCompositionAttribute, "SetWindowCompositionAttribute");
				}

				isInit = true;

				if (_OpenNcThemeData &&
					_RefreshImmersiveColorPolicyState &&
					_ShouldAppsUseDarkMode &&
					_AllowDarkModeForWindow &&
					(_AllowDarkModeForApp || _SetPreferredAppMode) &&
					_FlushMenuThemes &&
					_IsDarkModeAllowedForWindow)
				{
					g_darkModeSupported = true;
				}
			}
		}
	}
}

void SetDarkMode(bool useDark, bool fixDarkScrollbar)
{
	if (g_darkModeSupported)
	{
		AllowDarkModeForApp(useDark);
		//_RefreshImmersiveColorPolicyState();
		FlushMenuThemes();
		if (fixDarkScrollbar)
		{
			FixDarkScrollBar();
		}
		g_darkModeEnabled = useDark && ShouldAppsUseDarkMode() && !IsHighContrast();
	}
}

// Hooking GetSysColor for comboboxex listbox

using fnGetSysColor = DWORD (WINAPI*)(int nIndex);

static fnGetSysColor _GetSysColor = nullptr;

static COLORREF _clrWindow = RGB(32, 32, 32);
static COLORREF _clrText = RGB(224, 224, 224);

static bool isGetSysColorHooked = false;

void SetMySysColor(int nIndex, COLORREF clr)
{
	switch (nIndex)
	{
		case COLOR_WINDOW:
		{
			_clrWindow = clr;
			break;
		}

		case COLOR_WINDOWTEXT:
		{
			_clrText = clr;
			break;
		}

		default:
			break;
	}
}

DWORD MyGetSysColor(int nIndex)
{
	if( !(g_darkModeEnabled))
			return GetSysColor(nIndex);

	switch (nIndex)
	{
		case COLOR_WINDOW:
			return _clrWindow;

		case COLOR_WINDOWTEXT: 
			return _clrText;
		
		default:
			return GetSysColor(nIndex);
	}
}

template <typename P>
P ReplaceFunction(IMAGE_THUNK_DATA* addr, P newFunction)
{
	DWORD oldProtect = 0;
	if (!VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect))
		return 0;
	uintptr_t oldFunction = addr->u1.Function;
	addr->u1.Function = reinterpret_cast<uintptr_t>(newFunction);
	VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
	return reinterpret_cast<P>(oldFunction);
}

bool HookSysColor()
{
	HMODULE hComctl = LoadLibraryEx(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hComctl)
	{
		if (_GetSysColor == nullptr || !isGetSysColorHooked)
		{
			auto addr = FindIatThunkInModule(hComctl, "user32.dll", "GetSysColor");
			if (addr)
			{
				_GetSysColor = ReplaceFunction(addr, static_cast<fnGetSysColor>(MyGetSysColor));
				isGetSysColorHooked = true;
			}
			else
			{
				FreeLibrary(hComctl);
				return false;
			}
		}

		FreeLibrary(hComctl);
		return true;
	}
	return false;
}

void UnhookSysColor()
{
	HMODULE hComctl = LoadLibraryEx(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hComctl)
	{
		if (isGetSysColorHooked)
		{
			auto addr = FindIatThunkInModule(hComctl, "user32.dll", "GetSysColor");
			if (addr)
			{
				ReplaceFunction(addr, _GetSysColor);
				isGetSysColorHooked = false;
			}
		}

		FreeLibrary(hComctl);
	}
}
