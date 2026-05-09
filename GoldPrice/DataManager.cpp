#include "framework.h"
#include "DataManager.h"

#pragma comment(lib, "winhttp.lib")

#ifndef WINHTTP_OPTION_ENABLE_CERT_REVOCATION_CHECK
#define WINHTTP_OPTION_ENABLE_CERT_REVOCATION_CHECK 133
#endif

CDataManager& CDataManager::Instance()
{
    static CDataManager instance;
    return instance;
}

bool CDataManager::HttpGet(const wchar_t* url, std::string& response)
{
    response.clear();
    URL_COMPONENTS urlComp = { sizeof(urlComp) };

    wchar_t host[256] = {};
    wchar_t path[1024] = {};
    urlComp.lpszHostName = host;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = path;
    urlComp.dwUrlPathLength = 1024;

    if (!WinHttpCrackUrl(url, 0, 0, &urlComp))
        return false;

    HINTERNET hSession = WinHttpOpen(L"GoldPricePlugin/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, host, urlComp.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    DWORD timeout = 10000;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    timeout = 15000;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    // TLS 1.2+ 限制
    DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));

    // 证书吊销检查
    BOOL revokeCheck = TRUE;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_ENABLE_CERT_REVOCATION_CHECK, &revokeCheck, sizeof(revokeCheck));

    bool success = false;
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr))
    {
        DWORD bytesRead = 0;
        char buffer[4096];
        while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead))
        {
            if (bytesRead == 0) break;
            buffer[bytesRead] = '\0';
            response.append(buffer, bytesRead);
        }
        success = !response.empty();
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return success;
}

bool CDataManager::ParseGoldJson(const std::string& json)
{
    const char* p = json.c_str();

    const char* priceKey = strstr(p, "\"price\"");
    if (!priceKey) priceKey = strstr(p, "price");
    if (!priceKey) return false;

    const char* colon = strchr(priceKey, ':');
    if (!colon) return false;
    colon++;
    while (*colon == ' ' || *colon == '\t') colon++;

    char* end = nullptr;
    double price = strtod(colon, &end);
    if (end == colon) return false;

    // 金价范围校验 (100 ~ 100,000 USD/oz)
    if (price < 100.0 || price > 100000.0)
        return false;

    m_goldPriceUsd = price;
    m_goldPriceCny = (m_goldPriceUsd * m_exchangeRate) / 31.1035;

    GetSystemTime(&m_updateTime);
    return true;
}

bool CDataManager::ParseRateJson(const std::string& json)
{
    // open.er-api.com 格式: {"result":"success",...,"rates":{"CNY":6.8156,...}}
    const char* p = json.c_str();

    const char* cnyKey = strstr(p, "\"CNY\"");
    if (!cnyKey) cnyKey = strstr(p, "CNY");
    if (!cnyKey) return false;

    const char* colon = strchr(cnyKey, ':');
    if (!colon) return false;
    colon++;
    while (*colon == ' ' || *colon == '\t') colon++;

    char* end = nullptr;
    double rate = strtod(colon, &end);
    if (end == colon) return false;

    if (rate >= 1.0 && rate <= 20.0)
    {
        m_exchangeRate = rate;
        // 重新计算人民币价格
        m_goldPriceCny = (m_goldPriceUsd * m_exchangeRate) / 31.1035;
        m_lastRateFetchTick = GetTickCount();
        return true;
    }
    return false;
}

bool CDataManager::FetchGoldPrice()
{
    std::string response;

    if (!HttpGet(L"https://api.gold-api.com/price/XAU", response))
    {
        m_failed = true;
        return false;
    }

    if (!ParseGoldJson(response))
    {
        m_failed = true;
        return false;
    }

    if (m_goldPriceUsd > 0.01)
        m_prevPrice = m_goldPriceUsd;

    m_failed = false;
    m_lastFetchTick = GetTickCount();
    return true;
}

bool CDataManager::FetchExchangeRate()
{
    std::string response;

    if (!HttpGet(L"https://open.er-api.com/v6/latest/USD", response))
        return false;

    if (!ParseRateJson(response))
        return false;

    return true;
}

std::wstring CDataManager::FormatPrice(double price, const wchar_t* prefix) const
{
    long long intPart = (long long)price;
    int decimal = (int)((price - intPart) * 100.0 + 0.5);
    if (decimal >= 100) { intPart += 1; decimal = 0; }

    wchar_t intBuf[64] = {};
    swprintf_s(intBuf, L"%lld", intPart);

    std::wstring result;
    int len = (int)wcslen(intBuf);
    for (int i = 0; i < len; i++)
    {
        if (i > 0 && (len - i) % 3 == 0)
            result += L',';
        result += intBuf[i];
    }

    wchar_t out[128];
    swprintf_s(out, L"%s%s.%02d", prefix, result.c_str(), decimal);
    return out;
}

std::wstring CDataManager::GetValueText(int itemIndex) const
{
    if (m_failed || m_goldPriceUsd < 0.01)
        return L"N/A";

    if (itemIndex == 0)
        return FormatPrice(m_goldPriceUsd, L"$");

    return FormatPrice(m_goldPriceCny, L"\uFFE5");
}

std::wstring CDataManager::GetTooltipInfo() const
{
    if (m_failed || m_goldPriceUsd < 0.01)
        return L"金价: 网络错误";

    wchar_t buf[256];
    SYSTEMTIME st;
    GetLocalTime(&st);

    wchar_t usd[64], cny[64];
    swprintf_s(usd, L"$%.2f/oz", m_goldPriceUsd);
    swprintf_s(cny, L"\uFFE5%.2f/g", m_goldPriceCny);
    swprintf_s(buf, L"实时金价 | 美元: %s | 人民币: %s | 汇率: %.4f | %02d:%02d:%02d",
        usd, cny, m_exchangeRate, st.wHour, st.wMinute, st.wSecond);
    return buf;
}

void CDataManager::LoadConfig(const std::wstring& configDir)
{
    m_configPath = configDir + L"\\GoldPrice.ini";

    m_refreshInterval = GetPrivateProfileIntW(L"Settings", L"RefreshInterval", 60, m_configPath.c_str());
    m_showChange = GetPrivateProfileIntW(L"Settings", L"ShowChange", 1, m_configPath.c_str()) != 0;

    wchar_t buf[32] = {};
    GetPrivateProfileStringW(L"Settings", L"ExchangeRate", L"7.25", buf, 32, m_configPath.c_str());
    double rate = _wtof(buf);
    if (rate >= 1.0 && rate <= 20.0)
        m_exchangeRate = rate;
    else
        m_exchangeRate = 7.25;
}

void CDataManager::SaveConfig() const
{
    if (m_configPath.empty()) return;

    wchar_t buf[32];
    swprintf_s(buf, L"%d", m_refreshInterval);
    WritePrivateProfileStringW(L"Settings", L"RefreshInterval", buf, m_configPath.c_str());

    swprintf_s(buf, L"%d", m_showChange ? 1 : 0);
    WritePrivateProfileStringW(L"Settings", L"ShowChange", buf, m_configPath.c_str());

    swprintf_s(buf, L"%.4f", m_exchangeRate);
    WritePrivateProfileStringW(L"Settings", L"ExchangeRate", buf, m_configPath.c_str());
}
