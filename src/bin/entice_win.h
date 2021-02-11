/* Entice - small and simple image viewer using the EFL
 * Copyright (C) 2021 Vincent Torro
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

#ifndef ENTICE_WIN_H
#define ENTICE_WIN_H

typedef struct Entice Entice;

struct Entice
{
    /* window content */
    Evas_Object *conform;
    Evas_Object *scroller;
    Evas_Object *table;
    Evas_Object *layout;
    Evas_Object *image;
    Evas_Object *event_mouse;
    Evas_Object *event_kbd;
    Eina_Bool focused;

    /* list of opened images */
    Eina_List *images;
    /* current displayed image */
    Eina_List *current_image;

    /* theme */
    Entice_Config *config;
    char *theme_file;
};

Evas_Object *entice_win_add(void);

void entice_win_images_set(Evas_Object *win, Eina_List *images);

#endif /* ENTICE_WIN_H */
