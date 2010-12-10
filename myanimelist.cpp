/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "std.h"
#include "animelist.h"
#include "common.h"
#include "http.h"
#include "myanimelist.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"

CMyAnimeList MAL;

// =============================================================================

CMyAnimeList::CMyAnimeList() {
};

void CMyAnimeList::CheckProfile() {
  if (!Taiga.LoggedIn) return;
  MainClient.Get(L"myanimelist.net", 
    L"/editprofile.php?go=privacy", L"", 
    HTTP_MAL_Profile);
}

bool CMyAnimeList::GetList(bool login) {
  if (Settings.Account.MAL.User.empty()) return false;
  return MainClient.Get(L"myanimelist.net", 
    L"/malappinfo.php?u=" + Settings.Account.MAL.User + L"&status=all", 
    Taiga.GetDataPath() + Settings.Account.MAL.User + L".xml",
    login ? HTTP_MAL_RefreshAndLogin : HTTP_MAL_RefreshList);
}

bool CMyAnimeList::Login() {
  if (Taiga.LoggedIn || Settings.Account.MAL.User.empty() || Settings.Account.MAL.Password.empty()) return false;
  switch (Settings.Account.MAL.API) {
    case MAL_API_OFFICIAL: {
      return MainClient.Connect(L"myanimelist.net", 
        L"/api/account/verify_credentials.xml", 
        L"", 
        L"GET", 
        L"Authorization: Basic " + 
        GetUserPassEncoded(Settings.Account.MAL.User, Settings.Account.MAL.Password), 
        L"myanimelist.net", 
        L"", HTTP_MAL_Login);
    }
    case MAL_API_NONE: {
      return MainClient.Post(L"myanimelist.net", L"/login.php",
        L"username=" + Settings.Account.MAL.User + 
        L"&password=" + Settings.Account.MAL.Password + 
        L"&cookie=1&sublogin=Login", 
        L"", HTTP_MAL_Login);
    }
  }
  return false;
}

void CMyAnimeList::DownloadImage(CAnime* pAnimeItem) {
  wstring server, object, file;
  server = pAnimeItem->Series_Image;
  if (server.empty()) return;

  EraseLeft(server, L"http://", true);
  int pos = InStr(server, L"/");
  if (pos > -1) {
    object = server.substr(pos);
    server.resize(pos);
  }
  file = Taiga.GetDataPath() + L"Image\\";
  CreateDirectory(file.c_str(), NULL);
  file += ToWSTR(pAnimeItem->Series_ID) + L".jpg";
  
  ImageClient.Get(server, object, file, HTTP_MAL_Image, 
    reinterpret_cast<LPARAM>(pAnimeItem));
}

BOOL CMyAnimeList::SearchAnime(wstring title, CAnime* pAnimeItem) {
  if (title.empty()) return FALSE;
  Replace(title, L"+", L"%2B", true);
  //ReplaceChars(title, L"☆★", L" ");
  //title = EncodeURL(title);

  switch (Settings.Account.MAL.API) {
    case MAL_API_OFFICIAL: {
      if (Settings.Account.MAL.User.empty() || Settings.Account.MAL.Password.empty()) return FALSE;
      return SearchClient.Connect(L"myanimelist.net", 
        L"/api/anime/search.xml?q=" + title, 
        L"", 
        L"GET", 
        L"Authorization: Basic " + 
        GetUserPassEncoded(Settings.Account.MAL.User, Settings.Account.MAL.Password), 
        L"myanimelist.net", 
        L"", HTTP_MAL_SearchAnime, 
        reinterpret_cast<LPARAM>(pAnimeItem));
    }
    case MAL_API_NONE: {
      if (!pAnimeItem) {
        ViewAnimeSearch(title); // TEMP
      }
      return TRUE;
    }
  }

  return FALSE;
}

void CMyAnimeList::Update(int index, int id, int episode, int score, int status, wstring tags, int mode) {
  #define ANIME AnimeList.Item[index]
  #define ADD_DATA(name, value) { data += L"\r\n\t<" ##name L">" + value + L"</" ##name L">"; }

  switch (Settings.Account.MAL.API) {
    // Use official MAL API
    case MAL_API_OFFICIAL: {
      wstring data = L"data=<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<entry>";
      
      if (episode > -1) ADD_DATA(L"episode", ToWSTR(episode));
      if (status > -1) ADD_DATA(L"status", ToWSTR(status));
      if (score > -1) ADD_DATA(L"score", ToWSTR(score));
      if (tags != EMPTY_STR) ADD_DATA(L"tags", tags);
      
      switch (mode) {
        case HTTP_MAL_AnimeEdit: {
          if (ANIME.My_StartDate != L"0000-00-00" && !ANIME.My_StartDate.empty()) {
            ADD_DATA(L"date_start", ANIME.My_StartDate.substr(5, 2) + 
              ANIME.My_StartDate.substr(8, 2) + 
              ANIME.My_StartDate.substr(0, 4));
          }
          if (ANIME.My_FinishDate != L"0000-00-00" && !ANIME.My_FinishDate.empty()) {
            ADD_DATA(L"date_finish", ANIME.My_FinishDate.substr(5, 2) + 
              ANIME.My_FinishDate.substr(8, 2) + 
              ANIME.My_FinishDate.substr(0, 4));
          }
          break;
        }
      }

      data += L"\r\n</entry>";
      switch (mode) {
        // Add anime
        case HTTP_MAL_AnimeAdd: {
          MainClient.Connect(L"myanimelist.net", 
            L"/api/animelist/add/" + ToWSTR(id) + L".xml", 
            data, L"POST", 
            L"Authorization: Basic " + 
            GetUserPassEncoded(Settings.Account.MAL.User, Settings.Account.MAL.Password), 
            L"myanimelist.net", 
            L"", mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        }
        // Delete anime
        case HTTP_MAL_AnimeDelete: {
          MainClient.Connect(L"myanimelist.net", 
            L"/api/animelist/delete/" + ToWSTR(ANIME.Series_ID) + L".xml", 
            data, L"POST", 
            L"Authorization: Basic " + 
            GetUserPassEncoded(Settings.Account.MAL.User, Settings.Account.MAL.Password), 
            L"myanimelist.net", 
            L"", mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        }
        // Update anime
        default: {
          MainClient.Connect(L"myanimelist.net", 
            L"/api/animelist/update/" + ToWSTR(ANIME.Series_ID) + L".xml", 
            data, L"POST", 
            L"Authorization: Basic " + 
            GetUserPassEncoded(Settings.Account.MAL.User, Settings.Account.MAL.Password), 
            L"myanimelist.net", 
            L"", mode, reinterpret_cast<LPARAM>(&ANIME));
        }
      }
      break;
    }
  
    // Use default update method  
    case MAL_API_NONE: {
      switch (mode) {
        // Update episode
        case HTTP_MAL_AnimeUpdate:
          MainClient.Post(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=79", 
            L"anime_id=" + ToWSTR(ANIME.Series_ID) + 
            L"&ep_val="  + ToWSTR(episode), 
            L"", mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        // Update score
        case HTTP_MAL_ScoreUpdate:
          MainClient.Post(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=63", 
            L"id="     + ToWSTR(ANIME.My_ID) + 
            L"&score=" + ToWSTR(score), 
            L"", mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        // Update tags
        case HTTP_MAL_TagUpdate:
          MainClient.Get(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=22" \
            L"&aid="  + ToWSTR(ANIME.Series_ID) + 
            L"&tags=" + EncodeURL(tags), 
            L"", mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        // Add anime
        case HTTP_MAL_AnimeAdd:
          MainClient.Post(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=61", 
            L"aid="      + ToWSTR(id) + 
            L"&score=0"
            L"&status="  + ToWSTR(status) + 
            L"&epsseen=" + ToWSTR(episode), 
            L"", mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        // Delete anime
        case HTTP_MAL_AnimeDelete: // TODO
          /*MainClient.Post(L"myanimelist.net", 
            L"/editlist.php?type=anime",
            L"id=" + ToWSTR(ANIME.My_ID) + L"&submitIt=3", 
            L"", mode, reinterpret_cast<LPARAM>(&ANIME));*/
          break;
        // Update status
        case HTTP_MAL_StatusUpdate:
          MainClient.Post(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=62", 
            L"aid="      + ToWSTR(ANIME.Series_ID) + 
            L"&alistid=" + ToWSTR(ANIME.My_ID) + 
            L"&score="   + ToWSTR(ANIME.My_Score) + 
            L"&status="  + ToWSTR(status) + 
            L"&epsseen=" + ToWSTR(episode > -1 ? episode : ANIME.My_WatchedEpisodes), 
            L"", mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        // Edit anime
        case HTTP_MAL_AnimeEdit: {
          wstring buffer = 
            L"series_id="           + ToWSTR(ANIME.My_ID) + 
            L"&anime_db_series_id=" + ToWSTR(ANIME.Series_ID) + 
            L"&series_title="       + ToWSTR(ANIME.Series_ID) + 
            L"&aeps="               + ToWSTR(ANIME.Series_Episodes) + 
            L"&astatus="            + ToWSTR(ANIME.Series_Status) + 
            L"&close_on_update=true"
            L"&status="             + ToWSTR(status) + 
            L"&rewatch_ep=0"
            L"&last_status="        + ToWSTR(ANIME.My_Status) + 
            L"&completed_eps="      + ToWSTR(episode > -1 ? episode : ANIME.My_WatchedEpisodes) + 
            L"&last_completed_eps=" + ToWSTR(ANIME.My_WatchedEpisodes) + 
            L"&score="              + ToWSTR(score > -1 ? score : ANIME.My_Score);
          if (ANIME.My_StartDate == L"0000-00-00" || ANIME.My_StartDate.empty()) {
            buffer += 
            L"&unknownStart=1";
          } else {
            buffer += 
            L"&startMonth=" + ANIME.My_StartDate.substr(5, 2) + 
            L"&startDay="   + ANIME.My_StartDate.substr(8, 2) + 
            L"&startYear="  + ANIME.My_StartDate.substr(0, 4);
          }
          if (ANIME.My_FinishDate == L"0000-00-00" || ANIME.My_FinishDate.empty()) {
            buffer += 
            L"&unknownEnd=1";
          } else {
            buffer += 
            L"&endMonth=" + ANIME.My_FinishDate.substr(5, 2) + 
            L"&endDay="   + ANIME.My_FinishDate.substr(8, 2) + 
            L"&endYear="  + ANIME.My_FinishDate.substr(0, 4);
          }
          buffer += L"&submitIt=2";
          MainClient.Post(L"myanimelist.net", 
            L"/editlist.php?type=anime&id=" + ToWSTR(ANIME.My_ID),
            //L"/panel.php?keepThis=true&go=edit&id=" + ToWSTR(ANIME.My_ID) + L"&hidenav=true", 
            buffer, L"", mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        }
      }
      break;
    }
  }
}

bool CMyAnimeList::UpdateSucceeded(const wstring& data, int update_mode, int episode, const wstring& tags) {
  switch (Settings.Account.MAL.API) {
    case MAL_API_OFFICIAL: {
      switch (update_mode) {
        case HTTP_MAL_AnimeAdd:
          return IsNumeric(data);
        default:
          return data == L"Updated";
      }
      break;
    }
    case MAL_API_NONE: {
      switch (update_mode) {
        case HTTP_MAL_AnimeAdd:
          return true; // TODO
        case HTTP_MAL_AnimeUpdate:
          return ToINT(data) == episode;
        case HTTP_MAL_ScoreUpdate:
          return InStr(data, L"Updated score", 0) > -1;
        case HTTP_MAL_TagUpdate:
          return tags.empty() ? data.empty() : InStr(data, L"/animelist/", 0) > -1;
        case HTTP_MAL_AnimeEdit:
        case HTTP_MAL_StatusUpdate:
          return InStr(data, L"Success", 0) > -1;
      }
      break;
    }
  }
  return false;
}

// =============================================================================

void CMyAnimeList::DecodeSynopsis(wstring& text) {
  Replace(text, L"<br />", L"\r\n", true);
  Replace(text, L"&Aring;&laquo;", L"\u016B"); // TODO: Remove when MAL fixes its encoding
  Replace(text, L"&Aring;\uFFFD", L"\u014D");  // TODO: Remove when MAL fixes its encoding
  StripHTML(text);
  DecodeHTML(text);
}

bool CMyAnimeList::IsValidEpisode(int episode, int watched, int total) {
  if (episode < 0) {
    return false;
  } else if (episode < watched) {
    return false;
  } else if (episode == watched && total != 1) {
    return false;
  } else if (episode > total && total != 0) {
    return false;
  } else {
    return true;
  }
}

wstring CMyAnimeList::TranslateDate(wstring value, bool reverse) {
  if (value == L"0000-00-00" || value.empty()) {
    return L"Unknown";
  } else {
    int year  = ToINT(value.substr(0, 4));
    int month = ToINT(value.substr(5, 2));
    int day   = ToINT(value.substr(8, 2));

    if (month < 3) {
      value = L"Winter";
      year--;
    } else if (month < 6) {
      value = L"Spring";
    } else if (month < 9) {
      value = L"Summer";
    } else if (month < 11) {
      value = L"Fall";
    } else {
      value = L"Winter";
    }
    
    if (reverse) {
      value = ToWSTR(year) + L" " + value;
    } else {
      value = value + L" " + ToWSTR(year);
    }
    
    return value;
  }
}

wstring CMyAnimeList::TranslateMyStatus(int value, bool add_count) {
  #define ADD_COUNT() (add_count ? L" (" + ToWSTR(AnimeList.User.GetItemCount(value)) + L")" : L"")
  switch (value) {
    case MAL_NOTINLIST: return L"Not in list";
    case MAL_WATCHING: return L"Currently watching" + ADD_COUNT();
    case MAL_COMPLETED: return L"Completed" + ADD_COUNT();
    case MAL_ONHOLD: return L"On hold" + ADD_COUNT();
    case MAL_DROPPED: return L"Dropped" + ADD_COUNT();
    case MAL_PLANTOWATCH: return L"Plan to watch" + ADD_COUNT();
    default: return L"";
  }
  #undef ADD_COUNT
}

int CMyAnimeList::TranslateMyStatus(const wstring& value) {
  if (IsEqual(value, L"Currently watching")) {
    return MAL_WATCHING;
  } else if (IsEqual(value, L"Completed")) {
    return MAL_COMPLETED;
  } else if (IsEqual(value, L"On hold")) {
    return MAL_ONHOLD;
  } else if (IsEqual(value, L"Dropped")) {
    return MAL_DROPPED;
  } else if (IsEqual(value, L"Plan to watch")) {
    return MAL_PLANTOWATCH;
  } else {
    return 0;
  }
}

wstring CMyAnimeList::TranslateNumber(int value, LPCWSTR default_char) {
  return value == 0 ? default_char : ToWSTR(value);
}

wstring CMyAnimeList::TranslateStatus(int value) {
  switch (value) {
    case MAL_AIRING: return L"Currently airing";
    case MAL_FINISHED: return L"Finished airing";
    case MAL_NOTYETAIRED: return L"Not yet aired";
    default: return ToWSTR(value);
  }
}

int CMyAnimeList::TranslateStatus(const wstring& value) {
  if (IsEqual(value, L"Currently airing")) {
    return MAL_AIRING;
  } else if (IsEqual(value, L"Finished airing")) {
    return MAL_FINISHED;
  } else if (IsEqual(value, L"Not yet aired")) {
    return MAL_NOTYETAIRED;
  } else {
    return 0;
  }
}

wstring CMyAnimeList::TranslateType(int value) {
  switch (value) {
    case MAL_TV: return L"TV";
    case MAL_OVA: return L"OVA";
    case MAL_MOVIE: return L"Movie";
    case MAL_SPECIAL: return L"Special";
    case MAL_ONA: return L"ONA";
    case MAL_MUSIC: return L"Music";
    default: return L"";
  }
}

int CMyAnimeList::TranslateType(const wstring& value) {
  if (IsEqual(value, L"TV")) {
    return MAL_TV;
  } else if (IsEqual(value, L"OVA")) {
    return MAL_OVA;
  } else if (IsEqual(value, L"Movie")) {
    return MAL_MOVIE;
  } else if (IsEqual(value, L"Special")) {
    return MAL_SPECIAL;
  } else if (IsEqual(value, L"ONA")) {
    return MAL_ONA;
  } else if (IsEqual(value, L"Music")) {
    return MAL_MUSIC;
  } else {
    return 0;
  }
}

// =============================================================================

void CMyAnimeList::ViewAnimePage(int index) {
  ExecuteLink(L"http://myanimelist.net/anime/" + ToWSTR(AnimeList.Item[index].Series_ID) + L"/");
}

void CMyAnimeList::ViewAnimeSearch(wstring title) {
  ExecuteLink(L"http://myanimelist.net/anime.php?q=" + title + L"&referer=" + APP_NAME);
}

void CMyAnimeList::ViewHistory() {
  ExecuteLink(L"http://myanimelist.net/history/" + Settings.Account.MAL.User);
}

void CMyAnimeList::ViewMessages() {
  ExecuteLink(L"http://myanimelist.net/mymessages.php");
}

void CMyAnimeList::ViewPanel() {
  ExecuteLink(L"http://myanimelist.net/panel.php");
}

void CMyAnimeList::ViewProfile() {
  ExecuteLink(L"http://myanimelist.net/profile/" + Settings.Account.MAL.User);
}