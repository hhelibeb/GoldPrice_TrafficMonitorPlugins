#include "GoldPrice.h"
#include "DataManager.h"
#include "resource.h"

CGoldPrice CGoldPrice::m_instance;
static HINSTANCE g_hInstance = nullptr;

CGoldPrice::CGoldPrice()
    : m_itemUsd(CGoldPriceItem::USD_OZ)
    , m_itemCny(CGoldPriceItem::CNY_GRAM)
{
}

CGoldPrice& CGoldPrice::Instance()
{
    return m_instance;
}

IPluginItem* CGoldPrice::GetItem(int index)
{
    switch (index)
    {
    case 0: return &m_itemUsd;
    case 1: return &m_itemCny;
    default: return nullptr;
    }
}

const wchar_t* CGoldPrice::GetTooltipInfo()
{
    m_tooltipInfo = CDataManager::Instance().GetTooltipInfo();
    return m_tooltipInfo.c_str();
}

void CGoldPrice::DataRequired()
{
    CDataManager& data = CDataManager::Instance();
    DWORD tick = GetTickCount();

    // 金价刷新
    int intervalMs = data.m_refreshInterval * 1000;
    if (tick - data.m_lastFetchTick >= (DWORD)intervalMs)
    {
        data.FetchGoldPrice();
    }

    // 汇率刷新（每小时拉取一次）
    if (tick - data.m_lastRateFetchTick >= 3600000 || data.m_lastRateFetchTick == 0)
    {
        data.FetchExchangeRate();
    }
}

// Win32 对话框回调
static INT_PTR CALLBACK OptionsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static CDataManager* pData = nullptr;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        pData = &CDataManager::Instance();
        SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        // 刷新间隔下拉框
        HWND hCombo = GetDlgItem(hDlg, IDC_REFRESH_INTERVAL);
        const wchar_t* intervals[] = { L"30", L"60", L"120", L"300" };
        int values[] = { 30, 60, 120, 300 };
        int curSel = 1;
        for (int i = 0; i < 4; i++)
        {
            SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)intervals[i]);
            if (values[i] == pData->m_refreshInterval)
                curSel = i;
        }
        SendMessageW(hCombo, CB_SETCURSEL, curSel, 0);

        // 汇率编辑框
        wchar_t buf[32];
        swprintf_s(buf, L"%.4f", pData->m_exchangeRate);
        SetDlgItemTextW(hDlg, IDC_EXCHANGE_RATE, buf);

        // 显示涨跌复选框
        CheckDlgButton(hDlg, IDC_SHOW_CHANGE, pData->m_showChange ? BST_CHECKED : BST_UNCHECKED);
        return TRUE;
    }

    case WM_COMMAND:
    {
        WORD id = LOWORD(wParam);
        if (id == IDOK)
        {
            wchar_t buf[32];

            HWND hCombo = GetDlgItem(hDlg, IDC_REFRESH_INTERVAL);
            int sel = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
            int values[] = { 30, 60, 120, 300 };
            if (sel >= 0 && sel < 4)
                pData->m_refreshInterval = values[sel];

            GetDlgItemTextW(hDlg, IDC_EXCHANGE_RATE, buf, 32);
            double rate = _wtof(buf);
            if (rate >= 1.0 && rate <= 20.0)
                pData->m_exchangeRate = rate;

            pData->m_showChange = (IsDlgButtonChecked(hDlg, IDC_SHOW_CHANGE) == BST_CHECKED);

            pData->m_lastFetchTick = 0;
            pData->SaveConfig();
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (id == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

ITMPlugin::OptionReturn CGoldPrice::ShowOptionsDialog(void* hParent)
{
    HWND hWndParent = (HWND)hParent;
    INT_PTR result = DialogBoxParamW(g_hInstance,
        MAKEINTRESOURCEW(IDD_OPTIONS_DIALOG),
        hWndParent,
        OptionsDlgProc,
        0);

    return (result == IDOK) ? OR_OPTION_CHANGED : OR_OPTION_UNCHANGED;
}

const wchar_t* CGoldPrice::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:
        return L"实时金价";
    case TMI_DESCRIPTION:
        return L"实时显示 goldprice.org 金价（美元/盎司 & 人民币/克），支持自动汇率";
    case TMI_AUTHOR:
        return L"hhelibeb";
    case TMI_COPYRIGHT:
        return L"Copyright (C) 2026 MIT License";
    case TMI_VERSION:
        return L"1.0.0";
    case TMI_URL:
        return L"https://github.com/hhelibeb/GoldPrice_TrafficMonitorPlugins";
    default:
        return L"";
    }
}

void CGoldPrice::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data)
{
    if (index == ITMPlugin::EI_CONFIG_DIR)
    {
        CDataManager::Instance().LoadConfig(std::wstring(data));
    }
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = (HINSTANCE)hModule;
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

ITMPlugin* TMPluginGetInstance()
{
    return &CGoldPrice::Instance();
}
