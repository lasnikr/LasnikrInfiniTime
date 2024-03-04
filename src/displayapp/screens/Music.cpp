/*  Copyright (C) 2020 JF, Adam Pigg, Avamander

    This file is part of InfiniTime.

    InfiniTime is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    InfiniTime is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "displayapp/screens/Music.h"
#include "displayapp/screens/Symbols.h"
#include <cstdint>
#include "displayapp/DisplayApp.h"
#include "components/ble/MusicService.h"


using namespace Pinetime::Applications::Screens;

static void event_handler(lv_obj_t* obj, lv_event_t event) {
  Music* screen = static_cast<Music*>(obj->user_data);
  screen->OnObjectEvent(obj, event);
}

/**
 * Music control watchapp
 *
 * TODO: Investigate Apple Media Service and AVRCPv1.6 support for seamless integration
 */
Music::Music(Pinetime::Controllers::MusicService& music) : musicService(music) {
  btnPlayPause = lv_btn_create(lv_scr_act(), nullptr);
  btnPlayPause->user_data = this;
  lv_obj_set_event_cb(btnPlayPause, event_handler);
  lv_obj_set_size(btnPlayPause, 76, 76);
  lv_obj_align(btnPlayPause, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, -10);
  txtPlayPause = lv_label_create(btnPlayPause, nullptr);
  lv_label_set_text_static(txtPlayPause, Symbols::play);

  txtTrackDuration = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_long_mode(txtTrackDuration, LV_LABEL_LONG_SROLL);
  lv_obj_set_style_local_text_font(txtTrackDuration, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_label_set_text_static(txtTrackDuration, "--:--/--:--");
  lv_obj_set_width(txtTrackDuration, LV_HOR_RES);
  lv_obj_align(txtTrackDuration, nullptr, LV_ALIGN_IN_TOP_MID, 0, 30);
  lv_label_set_align(txtTrackDuration, LV_ALIGN_IN_LEFT_MID);
  
  txtArtist = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_long_mode(txtArtist, LV_LABEL_LONG_SROLL_CIRC);
  lv_obj_align(txtArtist, nullptr, LV_ALIGN_CENTER, 0, 20);
  lv_obj_set_width(txtArtist, LV_HOR_RES - 10);
  lv_label_set_align(txtArtist, LV_ALIGN_IN_LEFT_MID);
  lv_label_set_text_static(txtArtist, "Artist Name");

  txtTrack = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_long_mode(txtTrack, LV_LABEL_LONG_SROLL_CIRC);
  lv_obj_set_style_local_text_font(txtTrack, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_obj_set_width(txtTrack, LV_HOR_RES - 10);
  lv_label_set_align(txtTrack, LV_ALIGN_IN_LEFT_MID);
  lv_label_set_text_static(txtTrack, "This is a very long getTrack name");
  lv_obj_align(txtTrack, nullptr, LV_ALIGN_CENTER, 5, 40);

  musicService.event(Controllers::MusicService::EVENT_MUSIC_OPEN);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
}

Music::~Music() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void Music::Refresh() {
  if (artist != musicService.getArtist()) {
    artist = musicService.getArtist();
    lv_label_set_text(txtArtist, artist.data());
  }

  if (track != musicService.getTrack()) {
    track = musicService.getTrack();
    lv_label_set_text(txtTrack, track.data());
  }

  if (album != musicService.getAlbum()) {
    album = musicService.getAlbum();
  }

  if (playing != musicService.isPlaying()) {
    playing = musicService.isPlaying();
  }

  if (currentPosition != musicService.getProgress()) {
    currentPosition = musicService.getProgress();
    UpdateLength();
  }

  if (totalLength != musicService.getTrackLength()) {
    totalLength = musicService.getTrackLength();
    UpdateLength();
  }

  if (playing) {
    lv_label_set_text_static(txtPlayPause, Symbols::pause);
  } else {
    lv_label_set_text_static(txtPlayPause, Symbols::play);
  }
}

void Music::UpdateLength() {
  if (totalLength > (99 * 60 * 60)) {
    lv_label_set_text_static(txtTrackDuration, "Inf/Inf");
  } else if (totalLength > (99 * 60)) {
    lv_label_set_text_fmt(txtTrackDuration,
                          "%02d:%02d/%02d:%02d",
                          (currentPosition / (60 * 60)) % 100,
                          ((currentPosition % (60 * 60)) / 60) % 100,
                          (totalLength / (60 * 60)) % 100,
                          ((totalLength % (60 * 60)) / 60) % 100);
  } else {
    lv_label_set_text_fmt(txtTrackDuration,
                          "%02d:%02d/%02d:%02d",
                          (currentPosition / 60) % 100,
                          (currentPosition % 60) % 100,
                          (totalLength / 60) % 100,
                          (totalLength % 60) % 100);
  }
}

void Music::OnObjectEvent(lv_obj_t* obj, lv_event_t event) {
  if (event == LV_EVENT_CLICKED && obj == btnPlayPause) {
    if (playing == Controllers::MusicService::Playing) {
      musicService.event(Controllers::MusicService::EVENT_MUSIC_PAUSE);

      // Let's assume it stops playing instantly
      playing = Controllers::MusicService::NotPlaying;
    } else {
      musicService.event(Controllers::MusicService::EVENT_MUSIC_PLAY);

      // Let's assume it starts playing instantly
      // TODO: In the future should check for BT connection for better UX
      playing = Controllers::MusicService::Playing;
    }
  }
}

bool Music::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  switch (event) {
    case TouchEvents::SwipeLeft: {
      musicService.event(Controllers::MusicService::EVENT_MUSIC_NEXT);
      return true;
    }
    case TouchEvents::SwipeRight: {
      musicService.event(Controllers::MusicService::EVENT_MUSIC_PREV);
      return true;
    }
    case TouchEvents::SwipeUp: {
      musicService.event(Controllers::MusicService::EVENT_MUSIC_VOLUP);
      return true;
    }
    case TouchEvents::SwipeDown: {
      musicService.event(Controllers::MusicService::EVENT_MUSIC_VOLDOWN);
      return true;
    }
    default: {
      return false;
    }
  }
}
