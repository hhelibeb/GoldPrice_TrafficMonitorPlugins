#pragma once
#include "..\include\PluginInterface.h"
#include <string>

class CGoldPriceItem : public IPluginItem
{
public:
    enum ItemType { USD_OZ = 0, CNY_GRAM = 1 };

    explicit CGoldPriceItem(ItemType type) : m_type(type) {}

    virtual const wchar_t* GetItemName() const override;
    virtual const wchar_t* GetItemId() const override;
    virtual const wchar_t* GetItemLableText() const override;
    virtual const wchar_t* GetItemValueText() const override;
    virtual const wchar_t* GetItemValueSampleText() const override;

private:
    ItemType m_type;
    mutable std::wstring m_valueText;
};
