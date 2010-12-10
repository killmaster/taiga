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

#ifndef ANIMELIST_H
#define ANIMELIST_H

#include "std.h"
#include "anime.h"

enum {
  ANIMELIST_EDITUSER,
  ANIMELIST_EDITANIME,
  ANIMELIST_ADDANIME,
  ANIMELIST_DELETEANIME
};

// =============================================================================

class CMALUser {
public:
  int ID, Watching, Completed, OnHold, Dropped, PlanToWatch;
  wstring Name, DaysSpent;
};

class CUser : public CMALUser {
public:
  CUser();
  virtual ~CUser() {}

  void Clear();
  int GetItemCount(int status);
  void IncreaseItemCount(int status, int count = 1);
};

// =============================================================================

class CAnimeList {
public:
  CAnimeList();
  ~CAnimeList() {};

  void AddItem(CAnime& anime_item);
  void AddItem(int series_id, 
    wstring series_title, 
    wstring series_synonyms, 
    int series_type, 
    int series_episodes, 
    int series_status, 
    wstring series_start, 
    wstring series_end, 
    wstring series_image, 
    int my_id, 
    int my_watched_episodes, 
    wstring my_start_date, 
    wstring my_finish_date, 
    int my_score, 
    int my_status, 
    int my_rewatching, 
    int my_rewatching_ep, 
    wstring my_last_updated, 
    wstring my_tags);
  void Clear();
  BOOL Read();
  BOOL Write(int index, wstring child, wstring value, int mode);
  void DeleteItem(int index);
  int FindItemByID(int anime_id);

  CUser User;
  vector<CAnime> Item;
  int Index, Count;

  class CFilter {
  public:
    CFilter();
    virtual ~CFilter() {}
    
    BOOL Check(int item_index);
    void Reset();
    
    BOOL MyStatus[6], Status[3], Type[6], NewEps;
    wstring Text;
  } Filter;
};

extern CAnimeList AnimeList;

#endif // ANIMELIST_H