/* Entice - small and simple image viewer using the EFL
 * Copyright (C) 2021 Vincent Torri
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>

#include <Elementary.h>

#include "entice_private.h"
#include "entice_config.h"
#include "entice_controls.h"
#include "entice_theme.h"
#include "entice_image.h"
#include "entice_settings.h"
#include "entice_exif.h"
#include "entice_key.h"
#include "entice_win.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

static void
_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void
_cb_mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void
_cb_key_down(void *data,
             Evas *evas EINA_UNUSED,
             Evas_Object *obj EINA_UNUSED,
             void *event_info)
{
    EINA_SAFETY_ON_NULL_RETURN(event_info);

    entice_key_handle(data, event_info);
}

static void
_cb_win_del(void *data EINA_UNUSED,
            Evas *_e EINA_UNUSED,
            Evas_Object *win,
            void *_event EINA_UNUSED)
{
    Entice *entice;

    entice = evas_object_data_get(win, "entice");

    evas_object_event_callback_del_full(entice->event_mouse,
                                        EVAS_CALLBACK_MOUSE_DOWN,
                                        _cb_mouse_down, win);

    evas_object_event_callback_del_full(entice->event_mouse,
                                        EVAS_CALLBACK_MOUSE_MOVE,
                                        _cb_mouse_move, win);

    /* FIXME: free images */
    entice_config_del(entice->config);
    free(entice->theme_file);
    evas_object_data_del(win, "entice");
    free(entice);
}

static void
_cb_win_resize(void *data EINA_UNUSED,
               Evas *_e EINA_UNUSED,
               Evas_Object *win,
               void *_event EINA_UNUSED)
{
    Entice *entice;

    INF("win resize");

    entice = evas_object_data_get(win, "entice");
    if (!entice || !entice->image)
        return;

    entice_image_update(entice->image);

    INF("win fin resize");
}

static void
_cb_fullscreen(void *data EINA_UNUSED, Evas_Object *win, void *event EINA_UNUSED)
{
    Entice *entice;

    entice = evas_object_data_get(win, "entice");
    elm_layout_signal_emit(entice->layout, "state,win,fullscreen", "entice");
    elm_win_noblank_set(win, EINA_TRUE);
}

static void
_cb_unfullscreen(void *data EINA_UNUSED, Evas_Object *win, void *event EINA_UNUSED)
{
    Entice *entice;

    entice = evas_object_data_get(win, "entice");
    if (!elm_win_fullscreen_get(win))
    {
        elm_layout_signal_emit(entice->layout, "state,win,normal", "entice");
        elm_win_noblank_set(win, EINA_FALSE);
    }
}

static void
_cb_focused(void *data EINA_UNUSED, Evas_Object *win, void *event EINA_UNUSED)
{
    Entice *entice;

    entice = evas_object_data_get(win, "entice");
    elm_object_style_set(entice->bg, "grad_vert_focus_title_match");
}

static void
_cb_unfocused(void *data EINA_UNUSED, Evas_Object *win, void *event EINA_UNUSED)
{
    Entice *entice;

    entice = evas_object_data_get(win, "entice");
    elm_object_style_set(entice->bg, "default");
}

static void
_cb_mouse_move(void *win, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
    entice_controls_timer_start(win);
}

static void
_cb_mouse_down(void *win, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
    Entice *entice;
    Evas_Event_Mouse_Down *ev;

    ev = (Evas_Event_Mouse_Down *)event_info;
    entice = evas_object_data_get(win, "entice");

    if (ev->button == 3)
    {
        elm_menu_move(entice->menu_menu, ev->canvas.x, ev->canvas.y);
        if (!evas_object_visible_get(entice->menu_menu))
            elm_menu_open(entice->menu_menu);
        else
            elm_menu_close(entice->menu_menu);
    }

    if (ev->button != 1) return;
    if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
    {
        entice_win_fullscreen_toggle(win);
    }
}

static void
_cb_mouse_wheel(void *win,
                Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event)
{
    Entice *entice;
    Evas_Event_Mouse_Wheel *ev;
    Eina_Bool ctrl, alt, shift, winm, meta, hyper; /* modifiers */

    ev = (Evas_Event_Mouse_Wheel *)event;

    ENTICE_MODIFIERS_GET(ev->modifiers);

    entice = evas_object_data_get(win, "entice");

    /* Ctrl modifier */
    if (ctrl && !alt && !shift && !winm && !meta && !hyper)
    {
        if (ev->z == 1)
        {
            entice_image_zoom_increase(entice->image);
            entice_image_update(entice->image);
        }
        else
        {
            entice_image_zoom_decrease(entice->image);
            entice_image_update(entice->image);
        }
    }
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Evas_Object *
entice_win_add(void)
{
    Evas_Object *win;
    Evas_Object *o;
    Entice *entice;

    entice = (Entice *)calloc(1, sizeof(Entice));
    if (!entice)
    {
        ERR(_("could not allocate memory for Entice struct"));
        return NULL;
    }

    entice->config = entice_config_load();
    if (!entice->config)
    {
        ERR(_("could not load \"config\" configuration."));
        free(entice);
        return NULL;
    }
    elm_theme_overlay_add(NULL, entice_config_theme_path_default_get(entice->config));
    elm_theme_overlay_add(NULL, entice_config_theme_path_get(entice->config));

    /* main window */
    win = elm_win_add(NULL, "Entice", ELM_WIN_BASIC);
    if (!win)
    {
        ERR(_("could not create elm window."));
        entice_config_del(entice->config);
        free(entice);
        return NULL;
    }

    elm_win_title_set(win, "Entice");
    /* FIXME: icon name */
    elm_win_autodel_set(win, EINA_TRUE);
    elm_win_focus_highlight_enabled_set(win, EINA_TRUE);

    evas_object_data_set(win, "entice", entice);

    evas_object_event_callback_add(win, EVAS_CALLBACK_DEL, _cb_win_del, entice);
    evas_object_event_callback_add(win, EVAS_CALLBACK_RESIZE, _cb_win_resize, entice);

    evas_object_smart_callback_add(win, "fullscreen", _cb_fullscreen, NULL);
    evas_object_smart_callback_add(win, "unfullscreen", _cb_unfullscreen, NULL);
    evas_object_smart_callback_add(win, "focused", _cb_focused, entice);
    evas_object_smart_callback_add(win, "unfocused", _cb_unfocused, entice);

    /* background */
    o = elm_bg_add(win);
    evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_object_style_set(o, "grad_vert_focus_title_match");
    elm_win_resize_object_add(win, o);
    evas_object_show(o);
    entice->bg = o;

    /* scroller */
    o = elm_scroller_add(win);
    elm_scroller_policy_set(o,
                            ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_AUTO);
    elm_scroller_bounce_set(o, EINA_TRUE, EINA_TRUE);
    evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_win_resize_object_add(win, o);
    evas_object_show(o);
    entice->scroller = o;

    /* gui layout */
    o = elm_layout_add(win);
    evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_fill_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_repeat_events_set(o, EINA_FALSE);
    elm_win_resize_object_add(win, o);
    evas_object_show(o);
    entice->layout = o;
    if (!entice_theme_apply(win, "entice/core"))
    {
        ERR(_("could not apply the theme."));
        _cb_win_del(NULL, NULL, win, NULL);
        return NULL;
    }

    /* image */
    o = entice_image_add(win);
    elm_object_content_set(entice->scroller, o);
    //evas_object_show(o);
    entice->image = o;
    elm_object_part_content_set(entice->layout, "entice.image",
                                entice->scroller);

    /* dummy button to catch mouse events */
    o = elm_button_add(win);
    elm_object_focus_allow_set(o, EINA_FALSE);
    elm_object_focus_move_policy_set(o, ELM_FOCUS_MOVE_POLICY_CLICK);
    evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_object_part_content_set(entice->layout, "entice.events", o);
    evas_object_color_set(o, 0, 0, 0, 0);
    evas_object_repeat_events_set(o, EINA_TRUE);
    evas_object_show(o);
    evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE,
                                   _cb_mouse_move, win);
    evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                   _cb_mouse_down, win);
    evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_WHEEL,
                                   _cb_mouse_wheel, win);
    entice->event_mouse = o;

    o = elm_button_add(win);
    elm_object_focus_move_policy_set(o, ELM_FOCUS_MOVE_POLICY_CLICK);
    evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(win, o);
    evas_object_color_set(o, 0, 0, 0, 0);
    evas_object_repeat_events_set(o, EINA_TRUE);
    elm_object_cursor_set(o, "blank");
    elm_object_cursor_theme_search_enabled_set(o, EINA_TRUE);
    entice->event_blank = o;

    /* dummy button to catch keyboard events */
    o = elm_button_add(win);
    elm_object_focus_highlight_style_set(o, "blank");
    evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(win, o);
    evas_object_lower(o);
    evas_object_show(o);
    elm_object_focus_set(o, EINA_TRUE);
    evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN,
                                   _cb_key_down, win);
    entice->event_kbd = o;

    entice_controls_init(win);
    entice_exif_init(win);

    return win;
}

void
entice_win_title_update(Evas_Object *win)
{
    Entice *entice;
    char *title;

    entice = evas_object_data_get(win, "entice");
    title = entice_image_title_get(entice->image);
    if (title)
    {
        char buf[1024];

        snprintf(buf, sizeof(buf), "Entice - %s", title);
        elm_win_title_set(win, buf);
        elm_layout_text_set(entice->layout, "entice.title", title);
        free(title);
    }
    else
        elm_win_title_set(win, "Entice");
}

void
entice_win_images_set(Evas_Object *win, Eina_List *images, Eina_List *first)
{
    Entice *entice;

    if (!images)
        return;

    entice = evas_object_data_get(win, "entice");
    entice->images = images;
    entice_image_file_set(entice->image, first);
}

void
entice_win_fullscreen_toggle(Evas_Object *win)
{
    Entice *entice;

    entice = evas_object_data_get(win, "entice");
    elm_win_fullscreen_set(win, !elm_win_fullscreen_get(win));
    if (elm_win_fullscreen_get(win))
    {
        elm_icon_standard_set(entice->fullscreen, "view-fullscreen");
        elm_object_signal_emit(entice->layout,
                               "state,title,hide", "entice");
    }
    else
    {
        elm_icon_standard_set(entice->fullscreen, "view-restore");
        elm_object_signal_emit(entice->layout,
                               "state,title,show", "entice");
    }
}

void
entice_win_filename_copy(Evas_Object *win)
{
    Entice *entice;
    const char *filename;

    entice = evas_object_data_get(win, "entice");
    filename = entice_image_file_get(entice->image);
    if (filename)
    {
        elm_cnp_selection_set(win,
                              ELM_SEL_TYPE_CLIPBOARD,
                              ELM_SEL_FORMAT_TEXT,
                              filename, strlen(filename));
    }
}

void
entice_win_file_copy(Evas_Object *win)
{
    Entice *entice;
    Eina_File *f;

    entice = evas_object_data_get(win, "entice");
    f = eina_file_open(entice_image_file_get(entice->image), EINA_FALSE);
    if (f)
    {
        void *base;
        size_t length;

        base = eina_file_map_all(f, EINA_FILE_POPULATE);
        length = eina_file_size_get(f);
        elm_cnp_selection_set(win,
                              ELM_SEL_TYPE_CLIPBOARD,
                              ELM_SEL_FORMAT_IMAGE,
                              base, length);
        eina_file_close(f);
    }
}
