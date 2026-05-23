#pragma once

// Central inventory of every NVS (Preferences) key touched by the firmware.
// Historically these were scattered as raw string literals across call sites,
// which allowed silent typos ("miningPollStats", "showNoestr") to read the
// NVS default value forever without any compile-time signal.
//
// Two constraints shape this list:
//   1. Arduino's Preferences library uses an 8-bit slot for names internally
//      but the underlying NVS store caps keys at 15 characters. None of the
//      constants below may exceed that limit without silently truncating,
//      so NEVER rename a key here to something longer than 15 characters
//      without a migration path for existing installs.
//   2. The WebUI sends and receives the raw key names in /api/settings
//      payloads, so the *values* of these constants (the string literals)
//      are effectively part of the firmware's public API contract. Do not
//      change them casually.
//
// Conventions:
//   - Every constant is namespaced under `PrefKeys::` and kept as an
//     `inline constexpr const char*` so it can be used in constant
//     expressions without ODR issues.
//   - Deprecated keys that must still be read (for migration) are tagged
//     with a trailing `_DEPRECATED` suffix and an adjacent comment
//     pointing to the replacement.

namespace PrefKeys {

inline constexpr const char *ActCurrencies = "actCurrencies";
inline constexpr const char *BgColor = "bgColor";
inline constexpr const char *BitaxeEnabled = "bitaxeEnabled";
inline constexpr const char *BitaxeHostname = "bitaxeHostname";
inline constexpr const char *BlockFeeDec = "blockFeeDec";
inline constexpr const char *BlockFlashColor = "blockFlashColor";
inline constexpr const char *BlockHeight = "blockHeight";
inline constexpr const char *CeDisableSSL = "ceDisableSSL";
inline constexpr const char *CeEndpoint = "ceEndpoint";
inline constexpr const char *CurrentScreen = "currentScreen";
inline constexpr const char *DataSource = "dataSource";
inline constexpr const char *DisableLeds = "disableLeds";
inline constexpr const char *DisplayText = "displayText";
inline constexpr const char *DndEnabled = "dndEnabled";
inline constexpr const char *DndEndHour = "dndEndHour";
inline constexpr const char *DndEndMin = "dndEndMin";
inline constexpr const char *DndStartHour = "dndStartHour";
inline constexpr const char *DndStartMin = "dndStartMin";
inline constexpr const char *DndTimeEnabled = "dndTimeEnabled";
inline constexpr const char *EnableDebugLog = "enableDebugLog";
inline constexpr const char *FgColor = "fgColor";
inline constexpr const char *FlAlwaysOn = "flAlwaysOn";
inline constexpr const char *FlDisable = "flDisable";
inline constexpr const char *FlEffectDelay = "flEffectDelay";
inline constexpr const char *FlFlashOnUpd = "flFlashOnUpd";
inline constexpr const char *FlFlashOnZap = "flFlashOnZap";
inline constexpr const char *FlMaxBrightness = "flMaxBrightness";
inline constexpr const char *FlOffWhenDark = "flOffWhenDark";
inline constexpr const char *FontName = "fontName";
inline constexpr const char *FullRefreshMin = "fullRefreshMin";
inline constexpr const char *GitReleaseUrl = "gitReleaseUrl";
inline constexpr const char *GmtOffset = "gmtOffset";
inline constexpr const char *HostnamePrefix = "hostnamePrefix";
inline constexpr const char *HttpAuthEnabled = "httpAuthEnabled";
inline constexpr const char *HttpAuthPass = "httpAuthPass";
inline constexpr const char *HttpAuthUser = "httpAuthUser";
inline constexpr const char *InverseButtons = "inverseButtons";
inline constexpr const char *InvertedColor = "invertedColor";
inline constexpr const char *LastCurrency = "lastCurrency";
inline constexpr const char *LastPrice = "lastPrice";
inline constexpr const char *LedBrightness = "ledBrightness";
inline constexpr const char *LedFlashOnUpd = "ledFlashOnUpd";
inline constexpr const char *LedFlashOnZap = "ledFlashOnZap";
inline constexpr const char *LedStatus = "ledStatus";
inline constexpr const char *LedTestOnPower = "ledTestOnPower";
inline constexpr const char *LocalPoolHost = "localPoolHost";
inline constexpr const char *LuxLightToggle = "luxLightToggle";
inline constexpr const char *McapBigChar = "mcapBigChar";
inline constexpr const char *MdnsEnabled = "mdnsEnabled";
inline constexpr const char *MempoolInstance = "mempoolInstance";
inline constexpr const char *MempoolSecure = "mempoolSecure";
inline constexpr const char *MinSecPriceUpd = "minSecPriceUpd";
inline constexpr const char *MiningPoolName = "miningPoolName";
inline constexpr const char *MiningPoolStats = "miningPoolStats";
inline constexpr const char *MiningPoolUser = "miningPoolUser";
inline constexpr const char *MowMode = "mowMode";
inline constexpr const char *NostrPubKey = "nostrPubKey";
inline constexpr const char *NostrRelay = "nostrRelay";
inline constexpr const char *NostrZapNotify = "nostrZapNotify";
inline constexpr const char *NostrZapPubkey = "nostrZapPubkey";
// Plural form existed in prior firmware but is not currently written.
inline constexpr const char *NostrZapPubkeys_Legacy = "nostrZapPubkeys";
inline constexpr const char *OtaEnabled = "otaEnabled";
inline constexpr const char *OtaPass = "otaPass";
inline constexpr const char *PoolGlobalStats = "poolGlobalStats";
inline constexpr const char *PoolLogosUrl = "poolLogosUrl";
inline constexpr const char *RefrScrnChange = "refrScrnChange";
inline constexpr const char *ScreenOrder = "screenOrder";
inline constexpr const char *ScrnRestoreZap = "scrnRestoreZap";
inline constexpr const char *StealFocus = "stealFocus";
inline constexpr const char *SuffixPrice = "suffixPrice";
inline constexpr const char *SuffixShareDot = "suffixShareDot";
inline constexpr const char *SupplyPercent = "supplyPercent";
inline constexpr const char *TimerActive = "timerActive";
inline constexpr const char *TimerSeconds = "timerSeconds";
inline constexpr const char *TxPower = "txPower";
inline constexpr const char *TzString = "tzString";
inline constexpr const char *UseBlkCountdown = "useBlkCountdown";
inline constexpr const char *UseMscwTime = "useMscwTime";
inline constexpr const char *UseSatsSymbol = "useSatsSymbol";
inline constexpr const char *VerticalDesc = "verticalDesc";
inline constexpr const char *WifiConfigured = "wifiConfigured";
inline constexpr const char *WpTimeout = "wpTimeout";

} // namespace PrefKeys
