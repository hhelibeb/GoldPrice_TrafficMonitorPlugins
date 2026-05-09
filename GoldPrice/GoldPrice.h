#pragma once
#include "..\include\PluginInterface.h"
#include "GoldPriceItem.h"
#include <string>

class CGoldPrice : public ITMPlugin
{
public:
    static CGoldPrice& Instance();

    virtual IPluginItem* GetItem(int index) override;
    virtual const wchar_t* GetTooltipInfo() override;
    virtual void DataRequired() override;
    virtual OptionReturn ShowOptionsDialog(void* hParent) override;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override;
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) override;

private:
    CGoldPrice();
    static CGoldPrice m_instance;

    CGoldPriceItem m_itemUsd;
    CGoldPriceItem m_itemCny;
    mutable std::wstring m_tooltipInfo;
};

#ifdef __cplusplus
extern "C" {
#endif
    __declspec(dllexport) ITMPlugin* TMPluginGetInstance();
#ifdef __cplusplus
}
#endif
