#include "GoldPriceItem.h"
#include "DataManager.h"

const wchar_t* CGoldPriceItem::GetItemName() const
{
    return (m_type == USD_OZ) ? L"金价(美元/盎司)" : L"金价(人民币/克)";
}

const wchar_t* CGoldPriceItem::GetItemId() const
{
    return (m_type == USD_OZ) ? L"GoldPriceUSD" : L"GoldPriceCNY";
}

const wchar_t* CGoldPriceItem::GetItemLableText() const
{
    return (m_type == USD_OZ) ? L"金价$:" : L"金价¥:";
}

const wchar_t* CGoldPriceItem::GetItemValueText() const
{
    m_valueText = CDataManager::Instance().GetValueText(m_type);
    return m_valueText.c_str();
}

const wchar_t* CGoldPriceItem::GetItemValueSampleText() const
{
    return (m_type == USD_OZ) ? L"$9,999.99" : L"\uFFE5999.99";
}
