#pragma once

#include <string_view>
#include <array>
#include <utility>

namespace timezone_data {

// Enum for unique timezone strings
enum class TimezoneValue : uint8_t {
    plus000plus02_2_M3_5_0_1_M10_5_0_3 = 0,
    plus01_1 = 1,
    plus02_2 = 2,
    plus0330_330 = 3,
    plus03_3 = 4,
    plus0430_430 = 5,
    plus04_4 = 6,
    plus0530_530 = 7,
    plus0545_545 = 8,
    plus05_5 = 9,
    plus0630_630 = 10,
    plus06_6 = 11,
    plus07_7 = 12,
    plus0845_845 = 13,
    plus08_8 = 14,
    plus09_9 = 15,
    plus1030_1030plus11_11_M10_1_0_M4_1_0 = 16,
    plus10_10 = 17,
    plus11_11 = 18,
    plus11_11plus12_M10_1_0_M4_1_0_3 = 19,
    plus1245_1245plus1345_M9_5_0_245_M4_1_0_345 = 20,
    plus12_12 = 21,
    plus13_13 = 22,
    plus14_14 = 23,
    TZ_011 = 24,
    TZ_011plus00_M3_5_0_0_M10_5_0_1 = 25,
    TZ_022 = 26,
    TZ_022_01_M3_5_0__1_M10_5_0_0 = 27,
    TZ_033 = 28,
    TZ_033_02_M3_2_0_M11_1_0 = 29,
    TZ_044 = 30,
    TZ_044_03_M9_1_6_24_M4_1_6_24 = 31,
    TZ_055 = 32,
    TZ_066 = 33,
    TZ_066_05_M9_1_6_22_M4_1_6_22 = 34,
    TZ_077 = 35,
    TZ_088 = 36,
    TZ_0930930 = 37,
    TZ_099 = 38,
    TZ_1010 = 39,
    TZ_1111 = 40,
    TZ_1212 = 41,
    ACST_930 = 42,
    ACST_930ACDT_M10_1_0_M4_1_0_3 = 43,
    AEST_10 = 44,
    AEST_10AEDT_M10_1_0_M4_1_0_3 = 45,
    AKST9AKDT_M3_2_0_M11_1_0 = 46,
    AST4 = 47,
    AST4ADT_M3_2_0_M11_1_0 = 48,
    AWST_8 = 49,
    CAT_2 = 50,
    CET_1 = 51,
    CET_1CEST_M3_5_0_M10_5_0_3 = 52,
    CST_8 = 53,
    CST5CDT_M3_2_0_0_M11_1_0_1 = 54,
    CST6 = 55,
    CST6CDT_M3_2_0_M11_1_0 = 56,
    ChST_10 = 57,
    EAT_3 = 58,
    EET_2 = 59,
    EET_2EEST_M3_4_4_50_M10_4_4_50 = 60,
    EET_2EEST_M3_5_0_M10_5_0_3 = 61,
    EET_2EEST_M3_5_0_0_M10_5_0_0 = 62,
    EET_2EEST_M3_5_0_3_M10_5_0_4 = 63,
    EET_2EEST_M4_5_5_0_M10_5_4_24 = 64,
    EST5 = 65,
    EST5EDT_M3_2_0_M11_1_0 = 66,
    GMT0 = 67,
    GMT0BST_M3_5_0_1_M10_5_0 = 68,
    HKT_8 = 69,
    HST10 = 70,
    HST10HDT_M3_2_0_M11_1_0 = 71,
    IST_1GMT0_M10_5_0_M3_5_0_1 = 72,
    IST_2IDT_M3_4_4_26_M10_5_0 = 73,
    IST_530 = 74,
    JST_9 = 75,
    KST_9 = 76,
    MSK_3 = 77,
    MST7 = 78,
    MST7MDT_M3_2_0_M11_1_0 = 79,
    NST330NDT_M3_2_0_M11_1_0 = 80,
    NZST_12NZDT_M9_5_0_M4_1_0_3 = 81,
    PKT_5 = 82,
    PST_8 = 83,
    PST8PDT_M3_2_0_M11_1_0 = 84,
    SAST_2 = 85,
    SST11 = 86,
    UTC0 = 87,
    WAT_1 = 88,
    WET0WEST_M3_5_0_1_M10_5_0 = 89,
    WIB_7 = 90,
    WIT_9 = 91,
    WITA_8 = 92,
};

// Key-value pair type
using TimezoneEntry = std::pair<std::string_view, TimezoneValue>;

// Lookup table
constexpr std::array<TimezoneEntry, 484> TIMEZONE_DATA = {{
    {"Africa/Abidjan", TimezoneValue::GMT0},
    {"Africa/Accra", TimezoneValue::GMT0},
    {"Africa/Addis_Ababa", TimezoneValue::EAT_3},
    {"Africa/Algiers", TimezoneValue::CET_1},
    {"Africa/Asmara", TimezoneValue::EAT_3},
    {"Africa/Bamako", TimezoneValue::GMT0},
    {"Africa/Bangui", TimezoneValue::WAT_1},
    {"Africa/Banjul", TimezoneValue::GMT0},
    {"Africa/Bissau", TimezoneValue::GMT0},
    {"Africa/Blantyre", TimezoneValue::CAT_2},
    {"Africa/Brazzaville", TimezoneValue::WAT_1},
    {"Africa/Bujumbura", TimezoneValue::CAT_2},
    {"Africa/Cairo", TimezoneValue::EET_2EEST_M4_5_5_0_M10_5_4_24},
    {"Africa/Casablanca", TimezoneValue::plus01_1},
    {"Africa/Ceuta", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Africa/Conakry", TimezoneValue::GMT0},
    {"Africa/Dakar", TimezoneValue::GMT0},
    {"Africa/Dar_es_Salaam", TimezoneValue::EAT_3},
    {"Africa/Djibouti", TimezoneValue::EAT_3},
    {"Africa/Douala", TimezoneValue::WAT_1},
    {"Africa/El_Aaiun", TimezoneValue::plus01_1},
    {"Africa/Freetown", TimezoneValue::GMT0},
    {"Africa/Gaborone", TimezoneValue::CAT_2},
    {"Africa/Harare", TimezoneValue::CAT_2},
    {"Africa/Johannesburg", TimezoneValue::SAST_2},
    {"Africa/Juba", TimezoneValue::CAT_2},
    {"Africa/Kampala", TimezoneValue::EAT_3},
    {"Africa/Khartoum", TimezoneValue::CAT_2},
    {"Africa/Kigali", TimezoneValue::CAT_2},
    {"Africa/Kinshasa", TimezoneValue::WAT_1},
    {"Africa/Lagos", TimezoneValue::WAT_1},
    {"Africa/Libreville", TimezoneValue::WAT_1},
    {"Africa/Lome", TimezoneValue::GMT0},
    {"Africa/Luanda", TimezoneValue::WAT_1},
    {"Africa/Lubumbashi", TimezoneValue::CAT_2},
    {"Africa/Lusaka", TimezoneValue::CAT_2},
    {"Africa/Malabo", TimezoneValue::WAT_1},
    {"Africa/Maputo", TimezoneValue::CAT_2},
    {"Africa/Maseru", TimezoneValue::SAST_2},
    {"Africa/Mbabane", TimezoneValue::SAST_2},
    {"Africa/Mogadishu", TimezoneValue::EAT_3},
    {"Africa/Monrovia", TimezoneValue::GMT0},
    {"Africa/Nairobi", TimezoneValue::EAT_3},
    {"Africa/Ndjamena", TimezoneValue::WAT_1},
    {"Africa/Niamey", TimezoneValue::WAT_1},
    {"Africa/Nouakchott", TimezoneValue::GMT0},
    {"Africa/Ouagadougou", TimezoneValue::GMT0},
    {"Africa/Porto-Novo", TimezoneValue::WAT_1},
    {"Africa/Sao_Tome", TimezoneValue::GMT0},
    {"Africa/Timbuktu", TimezoneValue::GMT0},
    {"Africa/Tripoli", TimezoneValue::EET_2},
    {"Africa/Tunis", TimezoneValue::CET_1},
    {"Africa/Windhoek", TimezoneValue::CAT_2},
    {"America/Adak", TimezoneValue::HST10HDT_M3_2_0_M11_1_0},
    {"America/Anchorage", TimezoneValue::AKST9AKDT_M3_2_0_M11_1_0},
    {"America/Anguilla", TimezoneValue::AST4},
    {"America/Antigua", TimezoneValue::AST4},
    {"America/Araguaina", TimezoneValue::TZ_033},
    {"America/Argentina/Buenos_Aires", TimezoneValue::TZ_033},
    {"America/Argentina/Catamarca", TimezoneValue::TZ_033},
    {"America/Argentina/Cordoba", TimezoneValue::TZ_033},
    {"America/Argentina/Jujuy", TimezoneValue::TZ_033},
    {"America/Argentina/La_Rioja", TimezoneValue::TZ_033},
    {"America/Argentina/Mendoza", TimezoneValue::TZ_033},
    {"America/Argentina/Rio_Gallegos", TimezoneValue::TZ_033},
    {"America/Argentina/Salta", TimezoneValue::TZ_033},
    {"America/Argentina/San_Juan", TimezoneValue::TZ_033},
    {"America/Argentina/San_Luis", TimezoneValue::TZ_033},
    {"America/Argentina/Tucuman", TimezoneValue::TZ_033},
    {"America/Argentina/Ushuaia", TimezoneValue::TZ_033},
    {"America/Aruba", TimezoneValue::AST4},
    {"America/Asuncion", TimezoneValue::TZ_033},
    {"America/Atikokan", TimezoneValue::EST5},
    {"America/Atka", TimezoneValue::HST10HDT_M3_2_0_M11_1_0},
    {"America/Bahia", TimezoneValue::TZ_033},
    {"America/Bahia_Banderas", TimezoneValue::CST6},
    {"America/Barbados", TimezoneValue::AST4},
    {"America/Belem", TimezoneValue::TZ_033},
    {"America/Belize", TimezoneValue::CST6},
    {"America/Blanc-Sablon", TimezoneValue::AST4},
    {"America/Boa_Vista", TimezoneValue::TZ_044},
    {"America/Bogota", TimezoneValue::TZ_055},
    {"America/Boise", TimezoneValue::MST7MDT_M3_2_0_M11_1_0},
    {"America/Cambridge_Bay", TimezoneValue::MST7MDT_M3_2_0_M11_1_0},
    {"America/Campo_Grande", TimezoneValue::TZ_044},
    {"America/Cancun", TimezoneValue::EST5},
    {"America/Caracas", TimezoneValue::TZ_044},
    {"America/Cayenne", TimezoneValue::TZ_033},
    {"America/Cayman", TimezoneValue::EST5},
    {"America/Chicago", TimezoneValue::CST6CDT_M3_2_0_M11_1_0},
    {"America/Chihuahua", TimezoneValue::CST6},
    {"America/Ciudad_Juarez", TimezoneValue::MST7MDT_M3_2_0_M11_1_0},
    {"America/Coral_Harbour", TimezoneValue::EST5},
    {"America/Costa_Rica", TimezoneValue::CST6},
    {"America/Coyhaique", TimezoneValue::TZ_033},
    {"America/Creston", TimezoneValue::MST7},
    {"America/Cuiaba", TimezoneValue::TZ_044},
    {"America/Curacao", TimezoneValue::AST4},
    {"America/Danmarkshavn", TimezoneValue::GMT0},
    {"America/Dawson", TimezoneValue::MST7},
    {"America/Dawson_Creek", TimezoneValue::MST7},
    {"America/Denver", TimezoneValue::MST7MDT_M3_2_0_M11_1_0},
    {"America/Detroit", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Dominica", TimezoneValue::AST4},
    {"America/Edmonton", TimezoneValue::MST7MDT_M3_2_0_M11_1_0},
    {"America/Eirunepe", TimezoneValue::TZ_055},
    {"America/El_Salvador", TimezoneValue::CST6},
    {"America/Ensenada", TimezoneValue::PST8PDT_M3_2_0_M11_1_0},
    {"America/Fort_Nelson", TimezoneValue::MST7},
    {"America/Fortaleza", TimezoneValue::TZ_033},
    {"America/Glace_Bay", TimezoneValue::AST4ADT_M3_2_0_M11_1_0},
    {"America/Goose_Bay", TimezoneValue::AST4ADT_M3_2_0_M11_1_0},
    {"America/Grand_Turk", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Grenada", TimezoneValue::AST4},
    {"America/Guadeloupe", TimezoneValue::AST4},
    {"America/Guatemala", TimezoneValue::CST6},
    {"America/Guayaquil", TimezoneValue::TZ_055},
    {"America/Guyana", TimezoneValue::TZ_044},
    {"America/Halifax", TimezoneValue::AST4ADT_M3_2_0_M11_1_0},
    {"America/Havana", TimezoneValue::CST5CDT_M3_2_0_0_M11_1_0_1},
    {"America/Hermosillo", TimezoneValue::MST7},
    {"America/Indiana/Indianapolis", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Indiana/Knox", TimezoneValue::CST6CDT_M3_2_0_M11_1_0},
    {"America/Indiana/Marengo", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Indiana/Petersburg", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Indiana/Tell_City", TimezoneValue::CST6CDT_M3_2_0_M11_1_0},
    {"America/Indiana/Vevay", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Indiana/Vincennes", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Indiana/Winamac", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Inuvik", TimezoneValue::MST7MDT_M3_2_0_M11_1_0},
    {"America/Iqaluit", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Jamaica", TimezoneValue::EST5},
    {"America/Juneau", TimezoneValue::AKST9AKDT_M3_2_0_M11_1_0},
    {"America/Kentucky/Louisville", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Kentucky/Monticello", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Kralendijk", TimezoneValue::AST4},
    {"America/La_Paz", TimezoneValue::TZ_044},
    {"America/Lima", TimezoneValue::TZ_055},
    {"America/Los_Angeles", TimezoneValue::PST8PDT_M3_2_0_M11_1_0},
    {"America/Lower_Princes", TimezoneValue::AST4},
    {"America/Maceio", TimezoneValue::TZ_033},
    {"America/Managua", TimezoneValue::CST6},
    {"America/Manaus", TimezoneValue::TZ_044},
    {"America/Marigot", TimezoneValue::AST4},
    {"America/Martinique", TimezoneValue::AST4},
    {"America/Matamoros", TimezoneValue::CST6CDT_M3_2_0_M11_1_0},
    {"America/Mazatlan", TimezoneValue::MST7},
    {"America/Menominee", TimezoneValue::CST6CDT_M3_2_0_M11_1_0},
    {"America/Merida", TimezoneValue::CST6},
    {"America/Metlakatla", TimezoneValue::AKST9AKDT_M3_2_0_M11_1_0},
    {"America/Mexico_City", TimezoneValue::CST6},
    {"America/Miquelon", TimezoneValue::TZ_033_02_M3_2_0_M11_1_0},
    {"America/Moncton", TimezoneValue::AST4ADT_M3_2_0_M11_1_0},
    {"America/Monterrey", TimezoneValue::CST6},
    {"America/Montevideo", TimezoneValue::TZ_033},
    {"America/Montreal", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Montserrat", TimezoneValue::AST4},
    {"America/Nassau", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/New_York", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Nipigon", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Nome", TimezoneValue::AKST9AKDT_M3_2_0_M11_1_0},
    {"America/Noronha", TimezoneValue::TZ_022},
    {"America/North_Dakota/Beulah", TimezoneValue::CST6CDT_M3_2_0_M11_1_0},
    {"America/North_Dakota/Center", TimezoneValue::CST6CDT_M3_2_0_M11_1_0},
    {"America/North_Dakota/New_Salem", TimezoneValue::CST6CDT_M3_2_0_M11_1_0},
    {"America/Nuuk", TimezoneValue::TZ_022_01_M3_5_0__1_M10_5_0_0},
    {"America/Ojinaga", TimezoneValue::CST6CDT_M3_2_0_M11_1_0},
    {"America/Panama", TimezoneValue::EST5},
    {"America/Pangnirtung", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Paramaribo", TimezoneValue::TZ_033},
    {"America/Phoenix", TimezoneValue::MST7},
    {"America/Port-au-Prince", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Port_of_Spain", TimezoneValue::AST4},
    {"America/Porto_Acre", TimezoneValue::TZ_055},
    {"America/Porto_Velho", TimezoneValue::TZ_044},
    {"America/Puerto_Rico", TimezoneValue::AST4},
    {"America/Punta_Arenas", TimezoneValue::TZ_033},
    {"America/Rainy_River", TimezoneValue::CST6CDT_M3_2_0_M11_1_0},
    {"America/Rankin_Inlet", TimezoneValue::CST6CDT_M3_2_0_M11_1_0},
    {"America/Recife", TimezoneValue::TZ_033},
    {"America/Regina", TimezoneValue::CST6},
    {"America/Resolute", TimezoneValue::CST6CDT_M3_2_0_M11_1_0},
    {"America/Rio_Branco", TimezoneValue::TZ_055},
    {"America/Santa_Isabel", TimezoneValue::PST8PDT_M3_2_0_M11_1_0},
    {"America/Santarem", TimezoneValue::TZ_033},
    {"America/Santiago", TimezoneValue::TZ_044_03_M9_1_6_24_M4_1_6_24},
    {"America/Santo_Domingo", TimezoneValue::AST4},
    {"America/Sao_Paulo", TimezoneValue::TZ_033},
    {"America/Scoresbysund", TimezoneValue::TZ_022_01_M3_5_0__1_M10_5_0_0},
    {"America/Shiprock", TimezoneValue::MST7MDT_M3_2_0_M11_1_0},
    {"America/Sitka", TimezoneValue::AKST9AKDT_M3_2_0_M11_1_0},
    {"America/St_Barthelemy", TimezoneValue::AST4},
    {"America/St_Johns", TimezoneValue::NST330NDT_M3_2_0_M11_1_0},
    {"America/St_Kitts", TimezoneValue::AST4},
    {"America/St_Lucia", TimezoneValue::AST4},
    {"America/St_Thomas", TimezoneValue::AST4},
    {"America/St_Vincent", TimezoneValue::AST4},
    {"America/Swift_Current", TimezoneValue::CST6},
    {"America/Tegucigalpa", TimezoneValue::CST6},
    {"America/Thule", TimezoneValue::AST4ADT_M3_2_0_M11_1_0},
    {"America/Thunder_Bay", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Tijuana", TimezoneValue::PST8PDT_M3_2_0_M11_1_0},
    {"America/Toronto", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
    {"America/Tortola", TimezoneValue::AST4},
    {"America/Vancouver", TimezoneValue::PST8PDT_M3_2_0_M11_1_0},
    {"America/Virgin", TimezoneValue::AST4},
    {"America/Whitehorse", TimezoneValue::MST7},
    {"America/Winnipeg", TimezoneValue::CST6CDT_M3_2_0_M11_1_0},
    {"America/Yakutat", TimezoneValue::AKST9AKDT_M3_2_0_M11_1_0},
    {"America/Yellowknife", TimezoneValue::MST7MDT_M3_2_0_M11_1_0},
    {"Antarctica/Casey", TimezoneValue::plus08_8},
    {"Antarctica/Davis", TimezoneValue::plus07_7},
    {"Antarctica/DumontDUrville", TimezoneValue::plus10_10},
    {"Antarctica/Macquarie", TimezoneValue::AEST_10AEDT_M10_1_0_M4_1_0_3},
    {"Antarctica/Mawson", TimezoneValue::plus05_5},
    {"Antarctica/McMurdo", TimezoneValue::NZST_12NZDT_M9_5_0_M4_1_0_3},
    {"Antarctica/Palmer", TimezoneValue::TZ_033},
    {"Antarctica/Rothera", TimezoneValue::TZ_033},
    {"Antarctica/Syowa", TimezoneValue::plus03_3},
    {"Antarctica/Troll", TimezoneValue::plus000plus02_2_M3_5_0_1_M10_5_0_3},
    {"Antarctica/Vostok", TimezoneValue::plus05_5},
    {"Arctic/Longyearbyen", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Asia/Aden", TimezoneValue::plus03_3},
    {"Asia/Almaty", TimezoneValue::plus05_5},
    {"Asia/Amman", TimezoneValue::plus03_3},
    {"Asia/Anadyr", TimezoneValue::plus12_12},
    {"Asia/Aqtau", TimezoneValue::plus05_5},
    {"Asia/Aqtobe", TimezoneValue::plus05_5},
    {"Asia/Ashgabat", TimezoneValue::plus05_5},
    {"Asia/Atyrau", TimezoneValue::plus05_5},
    {"Asia/Baghdad", TimezoneValue::plus03_3},
    {"Asia/Bahrain", TimezoneValue::plus03_3},
    {"Asia/Baku", TimezoneValue::plus04_4},
    {"Asia/Bangkok", TimezoneValue::plus07_7},
    {"Asia/Barnaul", TimezoneValue::plus07_7},
    {"Asia/Beirut", TimezoneValue::EET_2EEST_M3_5_0_0_M10_5_0_0},
    {"Asia/Bishkek", TimezoneValue::plus06_6},
    {"Asia/Brunei", TimezoneValue::plus08_8},
    {"Asia/Chita", TimezoneValue::plus09_9},
    {"Asia/Chongqing", TimezoneValue::CST_8},
    {"Asia/Colombo", TimezoneValue::plus0530_530},
    {"Asia/Damascus", TimezoneValue::plus03_3},
    {"Asia/Dhaka", TimezoneValue::plus06_6},
    {"Asia/Dili", TimezoneValue::plus09_9},
    {"Asia/Dubai", TimezoneValue::plus04_4},
    {"Asia/Dushanbe", TimezoneValue::plus05_5},
    {"Asia/Famagusta", TimezoneValue::EET_2EEST_M3_5_0_3_M10_5_0_4},
    {"Asia/Gaza", TimezoneValue::EET_2EEST_M3_4_4_50_M10_4_4_50},
    {"Asia/Harbin", TimezoneValue::CST_8},
    {"Asia/Hebron", TimezoneValue::EET_2EEST_M3_4_4_50_M10_4_4_50},
    {"Asia/Ho_Chi_Minh", TimezoneValue::plus07_7},
    {"Asia/Hong_Kong", TimezoneValue::HKT_8},
    {"Asia/Hovd", TimezoneValue::plus07_7},
    {"Asia/Irkutsk", TimezoneValue::plus08_8},
    {"Asia/Istanbul", TimezoneValue::plus03_3},
    {"Asia/Jakarta", TimezoneValue::WIB_7},
    {"Asia/Jayapura", TimezoneValue::WIT_9},
    {"Asia/Jerusalem", TimezoneValue::IST_2IDT_M3_4_4_26_M10_5_0},
    {"Asia/Kabul", TimezoneValue::plus0430_430},
    {"Asia/Kamchatka", TimezoneValue::plus12_12},
    {"Asia/Karachi", TimezoneValue::PKT_5},
    {"Asia/Kashgar", TimezoneValue::plus06_6},
    {"Asia/Kathmandu", TimezoneValue::plus0545_545},
    {"Asia/Khandyga", TimezoneValue::plus09_9},
    {"Asia/Kolkata", TimezoneValue::IST_530},
    {"Asia/Krasnoyarsk", TimezoneValue::plus07_7},
    {"Asia/Kuala_Lumpur", TimezoneValue::plus08_8},
    {"Asia/Kuching", TimezoneValue::plus08_8},
    {"Asia/Kuwait", TimezoneValue::plus03_3},
    {"Asia/Macau", TimezoneValue::CST_8},
    {"Asia/Magadan", TimezoneValue::plus11_11},
    {"Asia/Makassar", TimezoneValue::WITA_8},
    {"Asia/Manila", TimezoneValue::PST_8},
    {"Asia/Muscat", TimezoneValue::plus04_4},
    {"Asia/Nicosia", TimezoneValue::EET_2EEST_M3_5_0_3_M10_5_0_4},
    {"Asia/Novokuznetsk", TimezoneValue::plus07_7},
    {"Asia/Novosibirsk", TimezoneValue::plus07_7},
    {"Asia/Omsk", TimezoneValue::plus06_6},
    {"Asia/Oral", TimezoneValue::plus05_5},
    {"Asia/Phnom_Penh", TimezoneValue::plus07_7},
    {"Asia/Pontianak", TimezoneValue::WIB_7},
    {"Asia/Pyongyang", TimezoneValue::KST_9},
    {"Asia/Qatar", TimezoneValue::plus03_3},
    {"Asia/Qostanay", TimezoneValue::plus05_5},
    {"Asia/Qyzylorda", TimezoneValue::plus05_5},
    {"Asia/Riyadh", TimezoneValue::plus03_3},
    {"Asia/Sakhalin", TimezoneValue::plus11_11},
    {"Asia/Samarkand", TimezoneValue::plus05_5},
    {"Asia/Seoul", TimezoneValue::KST_9},
    {"Asia/Shanghai", TimezoneValue::CST_8},
    {"Asia/Singapore", TimezoneValue::plus08_8},
    {"Asia/Srednekolymsk", TimezoneValue::plus11_11},
    {"Asia/Taipei", TimezoneValue::CST_8},
    {"Asia/Tashkent", TimezoneValue::plus05_5},
    {"Asia/Tbilisi", TimezoneValue::plus04_4},
    {"Asia/Tehran", TimezoneValue::plus0330_330},
    {"Asia/Tel_Aviv", TimezoneValue::IST_2IDT_M3_4_4_26_M10_5_0},
    {"Asia/Thimphu", TimezoneValue::plus06_6},
    {"Asia/Tokyo", TimezoneValue::JST_9},
    {"Asia/Tomsk", TimezoneValue::plus07_7},
    {"Asia/Ulaanbaatar", TimezoneValue::plus08_8},
    {"Asia/Urumqi", TimezoneValue::plus06_6},
    {"Asia/Ust-Nera", TimezoneValue::plus10_10},
    {"Asia/Vientiane", TimezoneValue::plus07_7},
    {"Asia/Vladivostok", TimezoneValue::plus10_10},
    {"Asia/Yakutsk", TimezoneValue::plus09_9},
    {"Asia/Yangon", TimezoneValue::plus0630_630},
    {"Asia/Yekaterinburg", TimezoneValue::plus05_5},
    {"Asia/Yerevan", TimezoneValue::plus04_4},
    {"Atlantic/Azores", TimezoneValue::TZ_011plus00_M3_5_0_0_M10_5_0_1},
    {"Atlantic/Bermuda", TimezoneValue::AST4ADT_M3_2_0_M11_1_0},
    {"Atlantic/Canary", TimezoneValue::WET0WEST_M3_5_0_1_M10_5_0},
    {"Atlantic/Cape_Verde", TimezoneValue::TZ_011},
    {"Atlantic/Faroe", TimezoneValue::WET0WEST_M3_5_0_1_M10_5_0},
    {"Atlantic/Jan_Mayen", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Atlantic/Madeira", TimezoneValue::WET0WEST_M3_5_0_1_M10_5_0},
    {"Atlantic/Reykjavik", TimezoneValue::GMT0},
    {"Atlantic/South_Georgia", TimezoneValue::TZ_022},
    {"Atlantic/St_Helena", TimezoneValue::GMT0},
    {"Atlantic/Stanley", TimezoneValue::TZ_033},
    {"Australia/Adelaide", TimezoneValue::ACST_930ACDT_M10_1_0_M4_1_0_3},
    {"Australia/Brisbane", TimezoneValue::AEST_10},
    {"Australia/Broken_Hill", TimezoneValue::ACST_930ACDT_M10_1_0_M4_1_0_3},
    {"Australia/Canberra", TimezoneValue::AEST_10AEDT_M10_1_0_M4_1_0_3},
    {"Australia/Currie", TimezoneValue::AEST_10AEDT_M10_1_0_M4_1_0_3},
    {"Australia/Darwin", TimezoneValue::ACST_930},
    {"Australia/Eucla", TimezoneValue::plus0845_845},
    {"Australia/Hobart", TimezoneValue::AEST_10AEDT_M10_1_0_M4_1_0_3},
    {"Australia/Lindeman", TimezoneValue::AEST_10},
    {"Australia/Lord_Howe", TimezoneValue::plus1030_1030plus11_11_M10_1_0_M4_1_0},
    {"Australia/Melbourne", TimezoneValue::AEST_10AEDT_M10_1_0_M4_1_0_3},
    {"Australia/Perth", TimezoneValue::AWST_8},
    {"Australia/Sydney", TimezoneValue::AEST_10AEDT_M10_1_0_M4_1_0_3},
    {"Australia/Yancowinna", TimezoneValue::ACST_930ACDT_M10_1_0_M4_1_0_3},
    {"Etc/GMT", TimezoneValue::GMT0},
    {"Etc/GMT+0", TimezoneValue::GMT0},
    {"Etc/GMT+1", TimezoneValue::TZ_011},
    {"Etc/GMT+10", TimezoneValue::TZ_1010},
    {"Etc/GMT+11", TimezoneValue::TZ_1111},
    {"Etc/GMT+12", TimezoneValue::TZ_1212},
    {"Etc/GMT+2", TimezoneValue::TZ_022},
    {"Etc/GMT+3", TimezoneValue::TZ_033},
    {"Etc/GMT+4", TimezoneValue::TZ_044},
    {"Etc/GMT+5", TimezoneValue::TZ_055},
    {"Etc/GMT+6", TimezoneValue::TZ_066},
    {"Etc/GMT+7", TimezoneValue::TZ_077},
    {"Etc/GMT+8", TimezoneValue::TZ_088},
    {"Etc/GMT+9", TimezoneValue::TZ_099},
    {"Etc/GMT-0", TimezoneValue::GMT0},
    {"Etc/GMT-1", TimezoneValue::plus01_1},
    {"Etc/GMT-10", TimezoneValue::plus10_10},
    {"Etc/GMT-11", TimezoneValue::plus11_11},
    {"Etc/GMT-12", TimezoneValue::plus12_12},
    {"Etc/GMT-13", TimezoneValue::plus13_13},
    {"Etc/GMT-14", TimezoneValue::plus14_14},
    {"Etc/GMT-2", TimezoneValue::plus02_2},
    {"Etc/GMT-3", TimezoneValue::plus03_3},
    {"Etc/GMT-4", TimezoneValue::plus04_4},
    {"Etc/GMT-5", TimezoneValue::plus05_5},
    {"Etc/GMT-6", TimezoneValue::plus06_6},
    {"Etc/GMT-7", TimezoneValue::plus07_7},
    {"Etc/GMT-8", TimezoneValue::plus08_8},
    {"Etc/GMT-9", TimezoneValue::plus09_9},
    {"Etc/GMT0", TimezoneValue::GMT0},
    {"Etc/Greenwich", TimezoneValue::GMT0},
    {"Etc/UCT", TimezoneValue::UTC0},
    {"Etc/UTC", TimezoneValue::UTC0},
    {"Etc/Universal", TimezoneValue::UTC0},
    {"Etc/Zulu", TimezoneValue::UTC0},
    {"Europe/Amsterdam", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Andorra", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Astrakhan", TimezoneValue::plus04_4},
    {"Europe/Athens", TimezoneValue::EET_2EEST_M3_5_0_3_M10_5_0_4},
    {"Europe/Belfast", TimezoneValue::GMT0BST_M3_5_0_1_M10_5_0},
    {"Europe/Belgrade", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Berlin", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Bratislava", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Brussels", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Bucharest", TimezoneValue::EET_2EEST_M3_5_0_3_M10_5_0_4},
    {"Europe/Budapest", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Busingen", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Chisinau", TimezoneValue::EET_2EEST_M3_5_0_M10_5_0_3},
    {"Europe/Copenhagen", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Dublin", TimezoneValue::IST_1GMT0_M10_5_0_M3_5_0_1},
    {"Europe/Gibraltar", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Guernsey", TimezoneValue::GMT0BST_M3_5_0_1_M10_5_0},
    {"Europe/Helsinki", TimezoneValue::EET_2EEST_M3_5_0_3_M10_5_0_4},
    {"Europe/Isle_of_Man", TimezoneValue::GMT0BST_M3_5_0_1_M10_5_0},
    {"Europe/Istanbul", TimezoneValue::plus03_3},
    {"Europe/Jersey", TimezoneValue::GMT0BST_M3_5_0_1_M10_5_0},
    {"Europe/Kaliningrad", TimezoneValue::EET_2},
    {"Europe/Kirov", TimezoneValue::MSK_3},
    {"Europe/Kyiv", TimezoneValue::EET_2EEST_M3_5_0_3_M10_5_0_4},
    {"Europe/Lisbon", TimezoneValue::WET0WEST_M3_5_0_1_M10_5_0},
    {"Europe/Ljubljana", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/London", TimezoneValue::GMT0BST_M3_5_0_1_M10_5_0},
    {"Europe/Luxembourg", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Madrid", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Malta", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Mariehamn", TimezoneValue::EET_2EEST_M3_5_0_3_M10_5_0_4},
    {"Europe/Minsk", TimezoneValue::plus03_3},
    {"Europe/Monaco", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Moscow", TimezoneValue::MSK_3},
    {"Europe/Nicosia", TimezoneValue::EET_2EEST_M3_5_0_3_M10_5_0_4},
    {"Europe/Oslo", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Paris", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Podgorica", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Prague", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Riga", TimezoneValue::EET_2EEST_M3_5_0_3_M10_5_0_4},
    {"Europe/Rome", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Samara", TimezoneValue::plus04_4},
    {"Europe/San_Marino", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Sarajevo", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Saratov", TimezoneValue::plus04_4},
    {"Europe/Simferopol", TimezoneValue::MSK_3},
    {"Europe/Skopje", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Sofia", TimezoneValue::EET_2EEST_M3_5_0_3_M10_5_0_4},
    {"Europe/Stockholm", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Tallinn", TimezoneValue::EET_2EEST_M3_5_0_3_M10_5_0_4},
    {"Europe/Tirane", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Tiraspol", TimezoneValue::EET_2EEST_M3_5_0_M10_5_0_3},
    {"Europe/Ulyanovsk", TimezoneValue::plus04_4},
    {"Europe/Vaduz", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Vatican", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Vienna", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Vilnius", TimezoneValue::EET_2EEST_M3_5_0_3_M10_5_0_4},
    {"Europe/Volgograd", TimezoneValue::MSK_3},
    {"Europe/Warsaw", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Zagreb", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Europe/Zurich", TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3},
    {"Indian/Antananarivo", TimezoneValue::EAT_3},
    {"Indian/Chagos", TimezoneValue::plus06_6},
    {"Indian/Christmas", TimezoneValue::plus07_7},
    {"Indian/Cocos", TimezoneValue::plus0630_630},
    {"Indian/Comoro", TimezoneValue::EAT_3},
    {"Indian/Kerguelen", TimezoneValue::plus05_5},
    {"Indian/Mahe", TimezoneValue::plus04_4},
    {"Indian/Maldives", TimezoneValue::plus05_5},
    {"Indian/Mauritius", TimezoneValue::plus04_4},
    {"Indian/Mayotte", TimezoneValue::EAT_3},
    {"Indian/Reunion", TimezoneValue::plus04_4},
    {"Pacific/Apia", TimezoneValue::plus13_13},
    {"Pacific/Auckland", TimezoneValue::NZST_12NZDT_M9_5_0_M4_1_0_3},
    {"Pacific/Bougainville", TimezoneValue::plus11_11},
    {"Pacific/Chatham", TimezoneValue::plus1245_1245plus1345_M9_5_0_245_M4_1_0_345},
    {"Pacific/Chuuk", TimezoneValue::plus10_10},
    {"Pacific/Easter", TimezoneValue::TZ_066_05_M9_1_6_22_M4_1_6_22},
    {"Pacific/Efate", TimezoneValue::plus11_11},
    {"Pacific/Fakaofo", TimezoneValue::plus13_13},
    {"Pacific/Fiji", TimezoneValue::plus12_12},
    {"Pacific/Funafuti", TimezoneValue::plus12_12},
    {"Pacific/Galapagos", TimezoneValue::TZ_066},
    {"Pacific/Gambier", TimezoneValue::TZ_099},
    {"Pacific/Guadalcanal", TimezoneValue::plus11_11},
    {"Pacific/Guam", TimezoneValue::ChST_10},
    {"Pacific/Honolulu", TimezoneValue::HST10},
    {"Pacific/Johnston", TimezoneValue::HST10},
    {"Pacific/Kanton", TimezoneValue::plus13_13},
    {"Pacific/Kiritimati", TimezoneValue::plus14_14},
    {"Pacific/Kosrae", TimezoneValue::plus11_11},
    {"Pacific/Kwajalein", TimezoneValue::plus12_12},
    {"Pacific/Majuro", TimezoneValue::plus12_12},
    {"Pacific/Marquesas", TimezoneValue::TZ_0930930},
    {"Pacific/Midway", TimezoneValue::SST11},
    {"Pacific/Nauru", TimezoneValue::plus12_12},
    {"Pacific/Niue", TimezoneValue::TZ_1111},
    {"Pacific/Norfolk", TimezoneValue::plus11_11plus12_M10_1_0_M4_1_0_3},
    {"Pacific/Noumea", TimezoneValue::plus11_11},
    {"Pacific/Pago_Pago", TimezoneValue::SST11},
    {"Pacific/Palau", TimezoneValue::plus09_9},
    {"Pacific/Pitcairn", TimezoneValue::TZ_088},
    {"Pacific/Pohnpei", TimezoneValue::plus11_11},
    {"Pacific/Port_Moresby", TimezoneValue::plus10_10},
    {"Pacific/Rarotonga", TimezoneValue::TZ_1010},
    {"Pacific/Saipan", TimezoneValue::ChST_10},
    {"Pacific/Samoa", TimezoneValue::SST11},
    {"Pacific/Tahiti", TimezoneValue::TZ_1010},
    {"Pacific/Tarawa", TimezoneValue::plus12_12},
    {"Pacific/Tongatapu", TimezoneValue::plus13_13},
    {"Pacific/Wake", TimezoneValue::plus12_12},
    {"Pacific/Wallis", TimezoneValue::plus12_12},
    {"Pacific/Yap", TimezoneValue::plus10_10},
    {"UTC", TimezoneValue::UTC0},
    {"posixrules", TimezoneValue::EST5EDT_M3_2_0_M11_1_0},
}};

// Helper function to find timezone value
inline TimezoneValue find_timezone_value(std::string_view key) {
    for (const auto& entry : TIMEZONE_DATA) {
        if (entry.first == key) {
            return entry.second;
        }
    }
    return TimezoneValue::GMT0; // Default fallback
}

// Overload for String
inline TimezoneValue find_timezone_value(const String& key) {
    return find_timezone_value(std::string_view(key.c_str()));
}

// Overload for std::string
inline TimezoneValue find_timezone_value(const std::string& key) {
    return find_timezone_value(std::string_view(key));
}

// Helper function to convert TimezoneValue to string representation
inline String get_timezone_string(TimezoneValue value) {
    for (const auto& entry : TIMEZONE_DATA) {
        if (entry.second == value) {
            return String(entry.first.data(), entry.first.length());
        }
    }
    return String("GMT0"); // Default fallback
}

// Overload for std::string
inline std::string get_timezone_string_std(TimezoneValue value) {
    for (const auto& entry : TIMEZONE_DATA) {
        if (entry.second == value) {
            return std::string(entry.first.data(), entry.first.length());
        }
    }
    return "GMT0"; // Default fallback
}

// Helper function to convert TimezoneValue enum to its string representation
inline String get_timezone_value_string(TimezoneValue value) {
    switch (value) {
        case TimezoneValue::plus000plus02_2_M3_5_0_1_M10_5_0_3: return String("<+00>0<+02>-2,M3.5.0/1,M10.5.0/3");
        case TimezoneValue::plus01_1: return String("<+01>-1");
        case TimezoneValue::plus02_2: return String("<+02>-2");
        case TimezoneValue::plus0330_330: return String("<+0330>-3:30");
        case TimezoneValue::plus03_3: return String("<+03>-3");
        case TimezoneValue::plus0430_430: return String("<+0430>-4:30");
        case TimezoneValue::plus04_4: return String("<+04>-4");
        case TimezoneValue::plus0530_530: return String("<+0530>-5:30");
        case TimezoneValue::plus0545_545: return String("<+0545>-5:45");
        case TimezoneValue::plus05_5: return String("<+05>-5");
        case TimezoneValue::plus0630_630: return String("<+0630>-6:30");
        case TimezoneValue::plus06_6: return String("<+06>-6");
        case TimezoneValue::plus07_7: return String("<+07>-7");
        case TimezoneValue::plus0845_845: return String("<+0845>-8:45");
        case TimezoneValue::plus08_8: return String("<+08>-8");
        case TimezoneValue::plus09_9: return String("<+09>-9");
        case TimezoneValue::plus1030_1030plus11_11_M10_1_0_M4_1_0: return String("<+1030>-10:30<+11>-11,M10.1.0,M4.1.0");
        case TimezoneValue::plus10_10: return String("<+10>-10");
        case TimezoneValue::plus11_11: return String("<+11>-11");
        case TimezoneValue::plus11_11plus12_M10_1_0_M4_1_0_3: return String("<+11>-11<+12>,M10.1.0,M4.1.0/3");
        case TimezoneValue::plus1245_1245plus1345_M9_5_0_245_M4_1_0_345: return String("<+1245>-12:45<+1345>,M9.5.0/2:45,M4.1.0/3:45");
        case TimezoneValue::plus12_12: return String("<+12>-12");
        case TimezoneValue::plus13_13: return String("<+13>-13");
        case TimezoneValue::plus14_14: return String("<+14>-14");
        case TimezoneValue::TZ_011: return String("<-01>1");
        case TimezoneValue::TZ_011plus00_M3_5_0_0_M10_5_0_1: return String("<-01>1<+00>,M3.5.0/0,M10.5.0/1");
        case TimezoneValue::TZ_022: return String("<-02>2");
        case TimezoneValue::TZ_022_01_M3_5_0__1_M10_5_0_0: return String("<-02>2<-01>,M3.5.0/-1,M10.5.0/0");
        case TimezoneValue::TZ_033: return String("<-03>3");
        case TimezoneValue::TZ_033_02_M3_2_0_M11_1_0: return String("<-03>3<-02>,M3.2.0,M11.1.0");
        case TimezoneValue::TZ_044: return String("<-04>4");
        case TimezoneValue::TZ_044_03_M9_1_6_24_M4_1_6_24: return String("<-04>4<-03>,M9.1.6/24,M4.1.6/24");
        case TimezoneValue::TZ_055: return String("<-05>5");
        case TimezoneValue::TZ_066: return String("<-06>6");
        case TimezoneValue::TZ_066_05_M9_1_6_22_M4_1_6_22: return String("<-06>6<-05>,M9.1.6/22,M4.1.6/22");
        case TimezoneValue::TZ_077: return String("<-07>7");
        case TimezoneValue::TZ_088: return String("<-08>8");
        case TimezoneValue::TZ_0930930: return String("<-0930>9:30");
        case TimezoneValue::TZ_099: return String("<-09>9");
        case TimezoneValue::TZ_1010: return String("<-10>10");
        case TimezoneValue::TZ_1111: return String("<-11>11");
        case TimezoneValue::TZ_1212: return String("<-12>12");
        case TimezoneValue::ACST_930: return String("ACST-9:30");
        case TimezoneValue::ACST_930ACDT_M10_1_0_M4_1_0_3: return String("ACST-9:30ACDT,M10.1.0,M4.1.0/3");
        case TimezoneValue::AEST_10: return String("AEST-10");
        case TimezoneValue::AEST_10AEDT_M10_1_0_M4_1_0_3: return String("AEST-10AEDT,M10.1.0,M4.1.0/3");
        case TimezoneValue::AKST9AKDT_M3_2_0_M11_1_0: return String("AKST9AKDT,M3.2.0,M11.1.0");
        case TimezoneValue::AST4: return String("AST4");
        case TimezoneValue::AST4ADT_M3_2_0_M11_1_0: return String("AST4ADT,M3.2.0,M11.1.0");
        case TimezoneValue::AWST_8: return String("AWST-8");
        case TimezoneValue::CAT_2: return String("CAT-2");
        case TimezoneValue::CET_1: return String("CET-1");
        case TimezoneValue::CET_1CEST_M3_5_0_M10_5_0_3: return String("CET-1CEST,M3.5.0,M10.5.0/3");
        case TimezoneValue::CST_8: return String("CST-8");
        case TimezoneValue::CST5CDT_M3_2_0_0_M11_1_0_1: return String("CST5CDT,M3.2.0/0,M11.1.0/1");
        case TimezoneValue::CST6: return String("CST6");
        case TimezoneValue::CST6CDT_M3_2_0_M11_1_0: return String("CST6CDT,M3.2.0,M11.1.0");
        case TimezoneValue::ChST_10: return String("ChST-10");
        case TimezoneValue::EAT_3: return String("EAT-3");
        case TimezoneValue::EET_2: return String("EET-2");
        case TimezoneValue::EET_2EEST_M3_4_4_50_M10_4_4_50: return String("EET-2EEST,M3.4.4/50,M10.4.4/50");
        case TimezoneValue::EET_2EEST_M3_5_0_M10_5_0_3: return String("EET-2EEST,M3.5.0,M10.5.0/3");
        case TimezoneValue::EET_2EEST_M3_5_0_0_M10_5_0_0: return String("EET-2EEST,M3.5.0/0,M10.5.0/0");
        case TimezoneValue::EET_2EEST_M3_5_0_3_M10_5_0_4: return String("EET-2EEST,M3.5.0/3,M10.5.0/4");
        case TimezoneValue::EET_2EEST_M4_5_5_0_M10_5_4_24: return String("EET-2EEST,M4.5.5/0,M10.5.4/24");
        case TimezoneValue::EST5: return String("EST5");
        case TimezoneValue::EST5EDT_M3_2_0_M11_1_0: return String("EST5EDT,M3.2.0,M11.1.0");
        case TimezoneValue::GMT0: return String("GMT0");
        case TimezoneValue::GMT0BST_M3_5_0_1_M10_5_0: return String("GMT0BST,M3.5.0/1,M10.5.0");
        case TimezoneValue::HKT_8: return String("HKT-8");
        case TimezoneValue::HST10: return String("HST10");
        case TimezoneValue::HST10HDT_M3_2_0_M11_1_0: return String("HST10HDT,M3.2.0,M11.1.0");
        case TimezoneValue::IST_1GMT0_M10_5_0_M3_5_0_1: return String("IST-1GMT0,M10.5.0,M3.5.0/1");
        case TimezoneValue::IST_2IDT_M3_4_4_26_M10_5_0: return String("IST-2IDT,M3.4.4/26,M10.5.0");
        case TimezoneValue::IST_530: return String("IST-5:30");
        case TimezoneValue::JST_9: return String("JST-9");
        case TimezoneValue::KST_9: return String("KST-9");
        case TimezoneValue::MSK_3: return String("MSK-3");
        case TimezoneValue::MST7: return String("MST7");
        case TimezoneValue::MST7MDT_M3_2_0_M11_1_0: return String("MST7MDT,M3.2.0,M11.1.0");
        case TimezoneValue::NST330NDT_M3_2_0_M11_1_0: return String("NST3:30NDT,M3.2.0,M11.1.0");
        case TimezoneValue::NZST_12NZDT_M9_5_0_M4_1_0_3: return String("NZST-12NZDT,M9.5.0,M4.1.0/3");
        case TimezoneValue::PKT_5: return String("PKT-5");
        case TimezoneValue::PST_8: return String("PST-8");
        case TimezoneValue::PST8PDT_M3_2_0_M11_1_0: return String("PST8PDT,M3.2.0,M11.1.0");
        case TimezoneValue::SAST_2: return String("SAST-2");
        case TimezoneValue::SST11: return String("SST11");
        case TimezoneValue::UTC0: return String("UTC0");
        case TimezoneValue::WAT_1: return String("WAT-1");
        case TimezoneValue::WET0WEST_M3_5_0_1_M10_5_0: return String("WET0WEST,M3.5.0/1,M10.5.0");
        case TimezoneValue::WIB_7: return String("WIB-7");
        case TimezoneValue::WIT_9: return String("WIT-9");
        case TimezoneValue::WITA_8: return String("WITA-8");
        default: return String("GMT0"); // Default fallback
    }
}

// Overload for std::string
inline std::string get_timezone_value_string_std(TimezoneValue value) {
    return std::string(get_timezone_value_string(value).c_str());
}

} // namespace timezone_data
