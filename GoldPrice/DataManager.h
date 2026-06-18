#pragma once
#include <windows.h>
#include <string>

// 数据管理单例：HTTP请求、JSON解析、配置管理、字符串格式化
class CDataManager
{
public:
    static CDataManager& Instance();

    bool FetchGoldPrice();
    bool FetchExchangeRate();
    std::wstring GetValueText(int itemIndex) const;   // 0=USD, 1=CNY
    std::wstring GetTooltipInfo() const;

    void LoadConfig(const std::wstring& configDir);
    void SaveConfig() const;

    // 选项设置
    int     m_refreshInterval = 60;    // 刷新间隔(秒)
    double  m_exchangeRate = 7.25;     // USD/CNY 汇率（默认，会被自动拉取覆盖）

    // 运行时数据
    double      m_goldPriceUsd = 0.0;
    double      m_goldPriceCny = 0.0;
    bool        m_failed = false;
    SYSTEMTIME  m_updateTime = {};
    DWORD       m_lastFetchTick = 0;
    DWORD       m_lastRateFetchTick = 0;

private:
    CDataManager() = default;
    CDataManager(const CDataManager&) = delete;

    bool HttpGet(const wchar_t* url, std::string& response);
    bool ParseGoldJson(const std::string& json);
    bool ParseRateJson(const std::string& json);
    std::wstring FormatPrice(double price, const wchar_t* prefix) const;

    std::wstring m_configPath;
};
